#include "BackendBattle.h"

bool BackendBattle::preprocessIfdStEviction(int inputFrameId) {
    int toEvictCnt = (inputFrameId - ifdBuffer.EdFrameId + 1) - (ifdBuffer.N - ifdBuffer.Cnt);
    int currDyanmicsRdfIdToUseIfdId = ConvertToDelayedInputFrameId(currDynamicsRdfId);
    int toEvictIfdIdUpperBound = (currDyanmicsRdfIdToUseIfdId < lcacIfdId ? currDyanmicsRdfIdToUseIfdId : lcacIfdId);
    if (toEvictIfdIdUpperBound < ifdBuffer.StFrameId + toEvictCnt) {
        /*
        When frontend freezing is carefully implemented, there should be no backend ifdBuffer eviction, see [comments in DLLMU-v2.3.4](https://github.com/genxium/DelayNoMoreUnity/blob/v2.3.4/frontend/Assets/Scripts/OnlineMapController.cs#L52). 
        */
        return false;
    }
    return true;
}

int BackendBattle::postprocessIfdStEviction() {
    // Try moving forward "lastConsecutivelyAllConfirmedIfdId" to favor the next call of "preprocessIfdStEviction(...)"
    int oldLcacIfdId = moveForwardLastConsecutivelyAllConfirmedIfdId(ifdBuffer.EdFrameId, inactiveJoinMask);
    if (oldLcacIfdId == lcacIfdId) return oldLcacIfdId;
    if (0 > lcacIfdId) return oldLcacIfdId;
    int nextDynamicsRdfId = (0 <= lcacIfdId ? ConvertToLastUsedRenderFrameId(lcacIfdId) + 1 : -1);
    if (0 >= nextDynamicsRdfId || nextDynamicsRdfId <= currDynamicsRdfId) return oldLcacIfdId;
    // Try moving forward "currDynamicsRdfId" to favor the next call of "preprocessIfdStEviction(...)"
    Step(currDynamicsRdfId, nextDynamicsRdfId);
    return oldLcacIfdId;
}

DownsyncSnapshot* BackendBattle::produceDownsyncSnapshot(uint64_t unconfirmedMask, int stIfdId, int edIfdId, int withRefRdfId) {
    DownsyncSnapshot* result = google::protobuf::Arena::Create<DownsyncSnapshot>(&pbTempAllocator);
    result->set_unconfirmed_mask(unconfirmedMask);
    result->set_ref_rdf_id(withRefRdfId);
    if (globalPrimitiveConsts->terminating_render_frame_id() != withRefRdfId) {
        RenderFrame* refRdf = rdfBuffer.GetByFrameId(withRefRdfId); // NOT arena-allocated, needs later manual release of ownership
        result->set_allocated_ref_rdf(refRdf);
    }
    if (stIfdId < ifdBuffer.StFrameId) {
        stIfdId = ifdBuffer.StFrameId;
    }
    if (edIfdId > ifdBuffer.EdFrameId) {
        edIfdId = ifdBuffer.EdFrameId;
    }
    auto resultIfdBatchHolder = result->mutable_ifd_batch();
    for (int ifdId = stIfdId; ifdId < edIfdId; ++ifdId) {
        InputFrameDownsync* ifd = ifdBuffer.GetByFrameId(ifdId); 
        resultIfdBatchHolder->UnsafeArenaAddAllocated(ifd); // Intentionally NOT using "RepeatedField<>::AddAllocated" because the "resultIfdBatchHolder" is arena-allocated but "ifd" is not
    }

    return result;
}

void BackendBattle::releaseDownsyncSnapshotArenaOwnership(DownsyncSnapshot* downsyncSnapshot) {
    downsyncSnapshot->release_ref_rdf(); // To avoid auto deallocation of the rdf which I still need
    auto resultIfdBatchHolder = downsyncSnapshot->mutable_ifd_batch();
    while (!resultIfdBatchHolder->empty()) {
        resultIfdBatchHolder->UnsafeArenaReleaseLast(); // To avoid auto deallocation of the ifd which I still need
    }
}


bool BackendBattle::OnUpsyncSnapshotReceived(char* inBytes, int inBytesCnt, bool fromUdp, bool fromTcp, char* outBytesPreallocatedStart, long* outBytesCntLimit) {
    UpsyncSnapshot* upsyncSnapshot = google::protobuf::Arena::Create<UpsyncSnapshot>(&pbTempAllocator);
    upsyncSnapshot->ParseFromArray(inBytes, inBytesCnt);
    int peerJoinIndex = upsyncSnapshot->join_index();

    DownsyncSnapshot* result = nullptr;
    bool inNewAllConfirmedTrend = true;
    int ifuBatchSize = upsyncSnapshot->ifu_batch_size();
    uint64_t inactiveJoinMaskVal = inactiveJoinMask.load();

    for (int i = 0; i < ifuBatchSize; ++i) {
        const InputFrameUpsync& ifu = upsyncSnapshot->ifu_batch(i);
        int ifdId = ifu.input_frame_id();
        if (ifdId <= lcacIfdId) {
            // obsolete
            continue;
        }
        if (ifdId < ifdBuffer.StFrameId) {
            continue;
        }
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
                    result = produceDownsyncSnapshot(allConfirmedMask, oldLcacIfdId+1, lcacIfdId+1, globalPrimitiveConsts->terminating_render_frame_id());
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
        InputFrameDownsync* ifd = GetOrPrefabInputFrameDownsync(ifdId, peerJoinIndex, ifu.encoded(), fromUdp, fromTcp, outExistingInputMutated);
        if (inNewAllConfirmedTrend && allConfirmedMask != ifd->confirmed_list()) {
            inNewAllConfirmedTrend = false;
            // From now on, "StFrameId eviction upon DryPut() of ifdBuffer" might occur, therefore we should proactively increment "lcacIfdId" along with "currDynamicsRdfId" to guarantee "perfect continuity of ifdId sequence in broadcasted `DownsyncSnapshot`s from backend".
            int oldLcacIfdId = moveForwardLastConsecutivelyAllConfirmedIfdId(ifdBuffer.EdFrameId, inactiveJoinMaskVal);
            if (oldLcacIfdId < lcacIfdId) {
                uint64_t unconfirmedMask = inactiveJoinMaskVal; // No eviction occurred yet, thus "unconfirmedMask" is just the immediate "inactiveJoinMaskVal"
                result = produceDownsyncSnapshot(unconfirmedMask, oldLcacIfdId+1, lcacIfdId+1, globalPrimitiveConsts->terminating_render_frame_id());
                // Make room for "StFrameId eviction upon DryPut() of ifdBuffer"
                int nextDynamicsRdfId = ConvertToLastUsedRenderFrameId(lcacIfdId) + 1;
                Step(currDynamicsRdfId, nextDynamicsRdfId);
            }
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
    } else {
        *outBytesCntLimit = 0;
    }

    return true;
}

bool BackendBattle::ProduceDownsyncSnapshotAndSerialize(uint64_t unconfirmedMask, int stIfdId, int edIfdId, char* outBytesPreallocatedStart, long* outBytesCntLimit, int withRefRdfId) {
    DownsyncSnapshot* result = produceDownsyncSnapshot(unconfirmedMask, stIfdId, edIfdId, withRefRdfId);

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
