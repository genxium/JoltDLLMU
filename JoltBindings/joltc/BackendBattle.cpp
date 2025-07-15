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
    result->set_st_ifd_id(stIfdId);
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

bool BackendBattle::OnUpsyncSnapshotReceived(char* inBytes, int inBytesCnt, bool fromUdp, bool fromTcp, char* outBytesPreallocatedStart, long* outBytesCntLimit, int* outStEvictedCnt) {
    UpsyncSnapshot* upsyncSnapshot = google::protobuf::Arena::Create<UpsyncSnapshot>(&pbTempAllocator);
    upsyncSnapshot->ParseFromArray(inBytes, inBytesCnt);
    return OnUpsyncSnapshotReceived(upsyncSnapshot, fromUdp, fromTcp, outBytesPreallocatedStart, outBytesCntLimit, outStEvictedCnt);
}

bool BackendBattle::OnUpsyncSnapshotReceived(const UpsyncSnapshot* upsyncSnapshot, bool fromUdp, bool fromTcp, char* outBytesPreallocatedStart, long* outBytesCntLimit, int* outStEvictedCnt) {
    int peerJoinIndex = upsyncSnapshot->join_index();
    (*outStEvictedCnt) = 0;
    DownsyncSnapshot* result = nullptr;
    int cmdListSize = upsyncSnapshot->cmd_list_size();
    bool isTooAdvanced = (ifdBuffer.StFrameId + ifdBuffer.N + globalPrimitiveConsts->upsync_st_ifd_id_tolerance()) < (upsyncSnapshot->st_ifd_id()); // When "ifdBuffer" is not full, we have "ifdBuffer.StFrameId + ifdBuffer.N >= ifdBuffer.EdFrameId"  
    if (isTooAdvanced) {
        *outBytesCntLimit = 0;
        return false;
    }
    uint64_t inactiveJoinMaskVal = inactiveJoinMask.load();
    bool isConsecutiveAllConfirmingCandidate = (upsyncSnapshot->st_ifd_id() <= lcacIfdId + 1);
    bool inNewAllConfirmedTrend = isConsecutiveAllConfirmingCandidate;

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
        bool willEvictSt = (ifdId >= ifdBuffer.StFrameId + ifdBuffer.N);
        if (willEvictSt) {
            int toEvictCnt = (ifdId - ifdBuffer.StFrameId - ifdBuffer.N + 1);
            JPH_ASSERT(1 == toEvictCnt || (1 < toEvictCnt && 0 == i)); // The only case where "1 < toEvictCnt" MUST come with "0 == i". 
            int postEvictionStFrameId = ifdBuffer.StFrameId + toEvictCnt;
            bool shouldDragLcacIfdIdForward = (lcacIfdId + 1 < postEvictionStFrameId);
            if (!shouldDragLcacIfdIdForward) {
                // This case makes no contribution to "DownsyncSnapshot* result", because by far all "<= lcacIfdId" elements should've already been broadcasted and used by "currDynamicsRdfId".
            } else {
                // This is the worst possible case, we have to start evicting from "ifdBuffer.StFrameId" as well as forcing the advancement of "lcacIfdId & currDynamicsRdfId" to guarantee "perfect continuity of ifdId sequence in broadcasted `DownsyncSnapshot`s from backend".
                int oldLcacIfdId = lcacIfdId;
                /*
                [EXAMPLE] 

                At ifdBuffer = "-1 | [0, ..., 450] 451)", given incoming "ifdId = 459", the ifdBuffer should become "8 | [9, ..., 459] 460)".
                */
                
                lcacIfdId = postEvictionStFrameId-1; // i.e. "ifdBuffer.StFrameId" will be incremented by the same amount later in "ifdBuffer.DryPut()".
                JPH_ASSERT(lcacIfdId < ifdId);
                (*outStEvictedCnt) += toEvictCnt;
                int downsyncSnapshotStThisRound = oldLcacIfdId+1;
                int targetDownsyncSnapshotEdThisRound = lcacIfdId+1;
                int initDownsyncSnapshotEdThisRound = (targetDownsyncSnapshotEdThisRound < ifdBuffer.EdFrameId ? targetDownsyncSnapshotEdThisRound : ifdBuffer.EdFrameId);
                if (nullptr == result) {
                    result = google::protobuf::Arena::Create<DownsyncSnapshot>(&pbTempAllocator);
                    result->set_unconfirmed_mask(allConfirmedMask); // No need to specify an accurate "unconfirmedMask" in this worst case
                    result->set_st_ifd_id(downsyncSnapshotStThisRound);
                }
                auto resultIfdBatchHolder = result->mutable_ifd_batch();
                for (int ifdId = downsyncSnapshotStThisRound; ifdId < initDownsyncSnapshotEdThisRound; ++ifdId) {
                    resultIfdBatchHolder->UnsafeArenaAddAllocated(ifdBuffer.GetByFrameId(ifdId)); // Intentionally NOT using "RepeatedField<>::AddAllocated" because the "resultIfdBatchHolder" is arena-allocated but "ifdBuffer.GetByFrameId(ifdId)" is not
                }
                int gapCntThisRound = targetDownsyncSnapshotEdThisRound - initDownsyncSnapshotEdThisRound;
                while (0 < gapCntThisRound) {
                    auto resultIfdBatchHolder = result->mutable_ifd_batch();
                    InputFrameDownsync* virtualIfd = resultIfdBatchHolder->Add(); // [REMINDER] Will allocate in the same arena
                    for (int k = 0; k < playersCnt; ++k) {
                        if (0 < (inactiveJoinMask & calcJoinIndexMask(k + 1))) {
                            virtualIfd->add_input_list(0);
                        } else {
                            virtualIfd->add_input_list(playerInputFronts[k]);
                        }
                    }
                    virtualIfd->set_confirmed_list(allConfirmedMask);
                    virtualIfd->set_udp_confirmed_list(allConfirmedMask);
                    --gapCntThisRound;
                }
            }

            // [WARNING] As "!willUpdateExisting && ifdBufferFull", we should call "Step(...)" to advance "currDynamicsRdfId" such that all "<= lcacIfdId" are used by "currDynamicsRdfId" before being any "StFrameId eviction upon DryPut() of ifdBuffer".
            int nextDynamicsRdfId = ConvertToLastUsedRenderFrameId(lcacIfdId) + 1;
            Step(currDynamicsRdfId, nextDynamicsRdfId, result);

            // Now that we're all set for "StFrameId eviction upon DryPut() of ifdBuffer"
        }
        bool outExistingInputMutated = false;
        bool canConfirmTcp = (fromUdp || fromTcp); // Backend doesn't care about whether it's from TCP or UDP, either can set "ifd.confirmed_list"
        InputFrameDownsync* ifd = GetOrPrefabInputFrameDownsync(ifdId, peerJoinIndex, cmd, canConfirmTcp, canConfirmTcp, outExistingInputMutated);
        if (inNewAllConfirmedTrend && ifd->confirmed_list() != allConfirmedMask) {
            // [WARNING] "StFrameId eviction upon DryPut() of ifdBuffer" might occur even during "inNewAllConfirmedTrend", but that'll be captured in the "if (!willUpdateExisting && ifdBufferFull)" block. 
            inNewAllConfirmedTrend = false;
            int oldLcacIfdId = moveForwardLastConsecutivelyAllConfirmedIfdId(ifdBuffer.EdFrameId, inactiveJoinMaskVal); // If "ifdBuffer" is not full, then "lcacIfdId" might've never advanced during the current traversal
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
                // As long as "currDynamicsRdfId" can still find its "delayedIfd" to use, i.e. no "StFrameId eviction upon DryPut() of ifdBuffer" occurs, we try to AVOID calling "Step(...)" as much as possible.
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

void BackendBattle::Step(int fromRdfId, int toRdfId, DownsyncSnapshot* virtualIfds) {
    BaseBattle::Step(fromRdfId, toRdfId, virtualIfds);
    currDynamicsRdfId = toRdfId;
}
