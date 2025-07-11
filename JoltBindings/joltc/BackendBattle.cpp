#include "BackendBattle.h"

bool BackendBattle::ResetStartRdf(const WsReq* initializerMapData) {
    bool res = BaseBattle::ResetStartRdf(initializerMapData);
    currDynamicsRdfId = timerRdfId;
    return res;
}

DownsyncSnapshot* BackendBattle::produceDownsyncSnapshot(uint64_t unconfirmedMask, int stIfdId, int edIfdId, int withRefRdf) {
    JPH_ASSERT(edIfdId > stIfdId);
    JPH_ASSERT(stIfdId >= ifdBuffer.StFrameId);
    JPH_ASSERT(edIfdId <= ifdBuffer.EdFrameId);
    DownsyncSnapshot* result = google::protobuf::Arena::Create<DownsyncSnapshot>(&pbTempAllocator);
    result->set_unconfirmed_mask(unconfirmedMask);
    auto resultIfdBatchHolder = result->mutable_ifd_batch();
    for (int ifdId = stIfdId; ifdId < edIfdId; ++ifdId) {
        InputFrameDownsync* ifd = ifdBuffer.GetByFrameId(ifdId); 
        resultIfdBatchHolder->UnsafeArenaAddAllocated(ifd); // Intentionally NOT using "RepeatedField<>::AddAllocated" because the "resultIfdBatchHolder" is arena-allocated but "ifd" is not
    }

    if (withRefRdf) {
        int refRdfId = ConvertToLastUsedRenderFrameId(edIfdId-1); // Such that the "RefRdf" always has EXACTLY 1 InputFrameDownsync to use upon reception on frontend.
        result->set_ref_rdf_id(refRdfId);
        RenderFrame* refRdf = rdfBuffer.GetByFrameId(refRdfId); // NOT arena-allocated, needs later manual release of ownership
        JPH_ASSERT (nullptr != refRdf);
        result->set_allocated_ref_rdf(refRdf); // No copy because "refRdf" is NOT arena-allocated.
    }
    return result;
}

void BackendBattle::releaseDownsyncSnapshotArenaOwnership(DownsyncSnapshot* downsyncSnapshot) {
    if (downsyncSnapshot->has_ref_rdf()) {
        downsyncSnapshot->release_ref_rdf(); // To avoid auto deallocation of the rdf which I still need
    }
    auto resultIfdBatchHolder = downsyncSnapshot->mutable_ifd_batch();
    while (!resultIfdBatchHolder->empty()) {
        resultIfdBatchHolder->UnsafeArenaReleaseLast(); // To avoid auto deallocation of the ifd which I still need
    }
}

bool BackendBattle::OnUpsyncSnapshotReceived(char* inBytes, int inBytesCnt, bool fromUdp, bool fromTcp, char* outBytesPreallocatedStart, long* outBytesCntLimit) {
    UpsyncSnapshot* upsyncSnapshot = google::protobuf::Arena::Create<UpsyncSnapshot>(&pbTempAllocator);
    upsyncSnapshot->ParseFromArray(inBytes, inBytesCnt);
    return OnUpsyncSnapshotReceived(upsyncSnapshot, fromUdp, fromTcp, outBytesPreallocatedStart, outBytesCntLimit);
}

bool BackendBattle::OnUpsyncSnapshotReceived(const UpsyncSnapshot* upsyncSnapshot, bool fromUdp, bool fromTcp, char* outBytesPreallocatedStart, long* outBytesCntLimit) {
    int peerJoinIndex = upsyncSnapshot->join_index();

    DownsyncSnapshot* result = nullptr;
    bool inNewAllConfirmedTrend = true;
    int cmdListSize = upsyncSnapshot->cmd_list_size();
    bool tooAdvanced = (ifdBuffer.StFrameId + ifdBuffer.N) < (upsyncSnapshot->st_ifd_id()); // When "ifdBuffer" is not full, we have "ifdBuffer.StFrameId + ifdBuffer.N >= ifdBuffer.EdFrameId"  
    if (tooAdvanced) {
        *outBytesCntLimit = 0;
        return false;
    }
    uint64_t inactiveJoinMaskVal = inactiveJoinMask.load();

    for (int i = 0; i < cmdListSize; ++i) {
        const uint64_t cmd = upsyncSnapshot->cmd_list(i);
        int ifdId = upsyncSnapshot->st_ifd_id() + i;
        if (ifdId <= lcacIfdId) {
            // obsolete
            continue;
        }
        /*
        [WARNING/BACKEND] it's maintained that "lcacIfdId + 1 >= ifdBuffer.StFrameId", hence "ifdId > lcacIfdId" implies that "ifdId >= ifdBuffer.StFrameId", no need to check. 
        */
        bool willUpdateExisting = (ifdId < ifdBuffer.EdFrameId);
        bool ifdBufferFull = (0 < ifdBuffer.Cnt && ifdBuffer.Cnt >= ifdBuffer.N);
        if (!willUpdateExisting && ifdBufferFull) {
            inNewAllConfirmedTrend = false;
            if (lcacIfdId >= ifdBuffer.StFrameId) {
                // This edge case makes no contribution to "DownsyncSnapshot* result", because it hasn't called "moveForwardLastConsecutivelyAllConfirmedIfdId" by far and all "<= lcacIfdId" elements should've already been broadcasted.
            } else {
                // This is the worst possible case, we have to start evicting from "ifdBuffer.StFrameId" as well as forcing the advancement of "lcacIfdId & currDynamicsRdfId".
                int gapCnt = (ifdId - ifdBuffer.EdFrameId + 1);
                JPH_ASSERT(1 == gapCnt || (1 < gapCnt && 0 == i)); // The only case where "1 < gapCnt" should come with "0 == i". 
                int oldLcacIfdId = lcacIfdId; 
                JPH_ASSERT(oldLcacIfdId+1 == ifdBuffer.StFrameId);
                lcacIfdId += gapCnt; // i.e. "ifdBuffer.StFrameId" will be incremented by the same amount later in "ifdBuffer.DryPut()".
                if (nullptr == result) {
                    // No need to specify an accurate "unconfirmedMask" in this worst case
                    result = produceDownsyncSnapshot(allConfirmedMask, oldLcacIfdId+1, lcacIfdId+1, false);
                } else {
                    auto resultIfdBatchHolder = result->mutable_ifd_batch();
                    resultIfdBatchHolder->UnsafeArenaAddAllocated(ifdBuffer.GetFirst()); // Intentionally NOT using "RepeatedField<>::AddAllocated" because the "resultIfdBatchHolder" is arena-allocated but "ifdBuffer.GetFirst()" is not
                }
            }
            int nextDynamicsRdfId = ConvertToLastUsedRenderFrameId(lcacIfdId) + 1;
            Step(currDynamicsRdfId, nextDynamicsRdfId);

            // Now that we're all set for "StFrameId eviction upon DryPut() of ifdBuffer"
        }
        bool outExistingInputMutated = false;
        bool canConfirmTcp = (fromUdp || fromTcp); // Backend doesn't care about whether it's from TCP or UDP, either can set "ifd.confirmed_list"
        InputFrameDownsync* ifd = GetOrPrefabInputFrameDownsync(ifdId, peerJoinIndex, cmd, canConfirmTcp, canConfirmTcp, outExistingInputMutated);
        if (inNewAllConfirmedTrend) {
            int oldLcacIfdId = moveForwardLastConsecutivelyAllConfirmedIfdId(ifdBuffer.EdFrameId, inactiveJoinMaskVal);
            if ((oldLcacIfdId == lcacIfdId) || (allConfirmedMask != ifd->confirmed_list())) {
                inNewAllConfirmedTrend = false;
                // From now on, "StFrameId eviction upon DryPut() of ifdBuffer" might occur, therefore we should proactively increment "lcacIfdId" along with "currDynamicsRdfId" to guarantee "perfect continuity of ifdId sequence in broadcasted `DownsyncSnapshot`s from backend".
            }
            if (oldLcacIfdId < lcacIfdId) {
                if (nullptr == result) {
                    uint64_t unconfirmedMask = inactiveJoinMaskVal; // No eviction occurred yet, thus "unconfirmedMask" is just the immediate "inactiveJoinMaskVal"
                    result = produceDownsyncSnapshot(unconfirmedMask, oldLcacIfdId + 1, lcacIfdId + 1, false);
                } else {
                    auto resultIfdBatchHolder = result->mutable_ifd_batch();
                    for (int ifdId = oldLcacIfdId + 1; ifdId <= lcacIfdId; ++ifdId) {
                        resultIfdBatchHolder->UnsafeArenaAddAllocated(ifdBuffer.GetByFrameId(ifdId));
                    }
                }
                // Only make room for exhausting "rdfBuffer"
                int nextDynamicsRdfId = ConvertToLastUsedRenderFrameId(lcacIfdId) + 1;
                if (nextDynamicsRdfId >= rdfBuffer.EdFrameId) {
                    Step(currDynamicsRdfId, nextDynamicsRdfId);
                }
            }
        }

        JPH_ASSERT(lcacIfdId+1 >= ifdBuffer.StFrameId); // A backend-specific constraint.
    }

    if (inNewAllConfirmedTrend) {
        // Wrap up
        int oldLcacIfdId = moveForwardLastConsecutivelyAllConfirmedIfdId(ifdBuffer.EdFrameId, inactiveJoinMaskVal);
        if (oldLcacIfdId < lcacIfdId) {
            uint64_t unconfirmedMask = inactiveJoinMaskVal; // No eviction occurred yet, thus "unconfirmedMask" is just the immediate "inactiveJoinMaskVal"
            result = produceDownsyncSnapshot(unconfirmedMask, oldLcacIfdId + 1, lcacIfdId + 1, false);
            // [WARNING] No longer needed to call "Step(...)" because traversal of the whole "cmd_list" has ended
        }
    }

    if (nullptr != result) {
        long byteSize = result->ByteSizeLong();
        if (byteSize > *outBytesCntLimit) {
            releaseDownsyncSnapshotArenaOwnership(result);
            return false;
        }
        *outBytesCntLimit = byteSize;
        result->SerializeToArray(outBytesPreallocatedStart, byteSize);
        releaseDownsyncSnapshotArenaOwnership(result);
    } else {
        *outBytesCntLimit = 0;
    }

    return true;
}

bool BackendBattle::ProduceDownsyncSnapshotAndSerialize(uint64_t unconfirmedMask, int stIfdId, int edIfdId, bool withRefRdf, char* outBytesPreallocatedStart, long* outBytesCntLimit) {
    DownsyncSnapshot* result = produceDownsyncSnapshot(unconfirmedMask, stIfdId, edIfdId, withRefRdf);

    long byteSize = result->ByteSizeLong();
    if (byteSize > *outBytesCntLimit) {
        releaseDownsyncSnapshotArenaOwnership(result);
        return false;
    }
    *outBytesCntLimit = byteSize;
    result->SerializeToArray(outBytesPreallocatedStart, byteSize);
    releaseDownsyncSnapshotArenaOwnership(result);
    return true;
}

void BackendBattle::Step(int fromRdfId, int toRdfId) {
    BaseBattle::Step(fromRdfId, toRdfId);
    currDynamicsRdfId = toRdfId;
}
