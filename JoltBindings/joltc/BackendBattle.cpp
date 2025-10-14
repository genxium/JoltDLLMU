#include "BackendBattle.h"

bool BackendBattle::ResetStartRdf(char* inBytes, int inBytesCnt) {
    return BaseBattle::ResetStartRdf(inBytes, inBytesCnt);
}

bool BackendBattle::ResetStartRdf(const WsReq* initializerMapData) {
    bool res = BaseBattle::ResetStartRdf(initializerMapData);
    dynamicsRdfId = rdfBuffer.GetLast()->id();
    return res;
}

void BackendBattle::produceDownsyncSnapshot(uint64_t unconfirmedMask, int stIfdId, int edIfdId, bool withRefRdf, DownsyncSnapshot** pOutResult) {
    JPH_ASSERT(stIfdId >= ifdBuffer.StFrameId);
    JPH_ASSERT(edIfdId <= ifdBuffer.EdFrameId);
    JPH_ASSERT(stIfdId <= edIfdId);
    if (nullptr == *pOutResult) {
        downsyncSnapshotHolder->set_unconfirmed_mask(unconfirmedMask);
        downsyncSnapshotHolder->set_st_ifd_id(stIfdId);
        downsyncSnapshotHolder->set_act(DownsyncAct::DA_REGULAR);
        if (withRefRdf) {
            RenderFrame* refRdf = rdfBuffer.GetByFrameId(dynamicsRdfId); // NOT arena-allocated, needs later manual release of ownership
            JPH_ASSERT(nullptr != refRdf);
            JPH_ASSERT(!downsyncSnapshotHolder->has_ref_rdf());
            downsyncSnapshotHolder->set_ref_rdf_id(dynamicsRdfId); // [WARNING] Unlike [DLLMU-v2.3.4](https://github.com/genxium/DelayNoMoreUnity/blob/v2.3.4/backend/Battle/Room.cs#L1248), we're only sure that "dynamicsRdfId" exists in "rdfBuffer" in the extreme case of "StFrameId eviction upon DryPut()". Moreover the use of "DownsyncSnapshot.ref_rdf" is de-coupled from "DownsyncSnapshot.ifd_batch", i.e. no need to guarantee that "DownsyncSnapshot.ref_rdf" is using one of "DownsyncSnapshot.ifd_batch" for frontend. 
            downsyncSnapshotHolder->set_allocated_ref_rdf(refRdf); // No copy because "refRdf" is NOT arena-allocated.
        }
        *pOutResult = downsyncSnapshotHolder; 
    }
    if (stIfdId < edIfdId) {
        auto resultIfdBatchHolder = (*pOutResult)->mutable_ifd_batch();
        for (int ifdId = stIfdId; ifdId < edIfdId; ++ifdId) {
            InputFrameDownsync* ifd = ifdBuffer.GetByFrameId(ifdId); 
            resultIfdBatchHolder->AddAllocated(ifd); // No copy because "ifd" is NOT arena-allocated
        }
    }
}

void BackendBattle::releaseDownsyncSnapshotArenaOwnership(DownsyncSnapshot* downsyncSnapshot) {
    if (downsyncSnapshot->has_ref_rdf()) {
        auto res = downsyncSnapshot->release_ref_rdf(); // To avoid auto deallocation of the rdf which I still need
    }
    auto resultIfdBatchHolder = downsyncSnapshot->mutable_ifd_batch();
    while (!resultIfdBatchHolder->empty()) {
        auto res = resultIfdBatchHolder->ReleaseLast(); // To avoid auto deallocation of the ifd which I still need
    }
    downsyncSnapshot->Clear();
}

bool BackendBattle::OnUpsyncSnapshotReqReceived(char* inBytes, int inBytesCnt, bool fromUdp, bool fromTcp, char* outBytesPreallocatedStart, long* outBytesCntLimit, int* outForceConfirmedStEvictedCnt, int* outOldLcacIfdId, int* outNewLcacIfdId, int* outOldDynamicsRdfId, int* outNewDynamicsRdfId, int* outMaxPlayerInputFrontId, int* outMinPlayerInputFrontId) {
    wsReqHolder->Clear();
    wsReqHolder->ParseFromArray(inBytes, inBytesCnt);
    uint32_t peerJoinIndex = wsReqHolder->join_index();
    if (0 >= peerJoinIndex) return false;
    if (!wsReqHolder->has_upsync_snapshot()) return false;
    return OnUpsyncSnapshotReceived(peerJoinIndex, wsReqHolder->upsync_snapshot(), fromUdp, fromTcp, outBytesPreallocatedStart, outBytesCntLimit, outForceConfirmedStEvictedCnt, outOldLcacIfdId, outNewLcacIfdId, outOldDynamicsRdfId, outNewDynamicsRdfId, outMaxPlayerInputFrontId, outMinPlayerInputFrontId);
}

bool BackendBattle::OnUpsyncSnapshotReceived(const uint32_t peerJoinIndex, const UpsyncSnapshot& upsyncSnapshot, bool fromUdp, bool fromTcp, char* outBytesPreallocatedStart, long* outBytesCntLimit, int* outForceConfirmedStEvictedCnt, int* outOldLcacIfdId, int* outNewLcacIfdId, int* outOldDynamicsRdfId, int* outNewDynamicsRdfId, int* outMaxPlayerInputFrontId, int* outMinPlayerInputFrontId) {
    (*outForceConfirmedStEvictedCnt) = 0;
    DownsyncSnapshot* result = nullptr;
    int cmdListSize = upsyncSnapshot.cmd_list_size();
    *outOldLcacIfdId = lcacIfdId;
    *outOldDynamicsRdfId = dynamicsRdfId;
    *outNewLcacIfdId = lcacIfdId;
    *outNewDynamicsRdfId = dynamicsRdfId;
    bool isTooAdvanced = (ifdBuffer.StFrameId + ifdBuffer.N + globalPrimitiveConsts->upsync_st_ifd_id_tolerance()) < (upsyncSnapshot.st_ifd_id()); // When "ifdBuffer" is not full, we have "ifdBuffer.StFrameId + ifdBuffer.N >= ifdBuffer.EdFrameId"  
    if (isTooAdvanced) {
        *outBytesCntLimit = 0;
#ifndef NDEBUG
        std::ostringstream oss;
        oss << "dynamicsRdfId: " << dynamicsRdfId << ", @lcacIfdId=" << lcacIfdId << ", upsyncSnapshot from peerJoinIndex==" << peerJoinIndex << " is discarded due to being too advanced, ifdBuffer.StFrameId=" << ifdBuffer.StFrameId << ", upsyncSnapshot.StIfdId=" << upsyncSnapshot.st_ifd_id(); 
        Debug::Log(oss.str());
#endif
        return false;
    }
    uint64_t inactiveJoinMaskVal = inactiveJoinMask.load();
    bool isConsecutiveAllConfirmingCandidate = (upsyncSnapshot.st_ifd_id() <= lcacIfdId + 1);
    bool inNewAllConfirmedTrend = isConsecutiveAllConfirmingCandidate;

    for (int i = 0; i < cmdListSize; ++i) {
        const uint64_t cmd = upsyncSnapshot.cmd_list(i);
        int ifdId = upsyncSnapshot.st_ifd_id() + i;
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
                // This case makes no contribution to "DownsyncSnapshot* result", because by far all "<= lcacIfdId" elements should've already been broadcasted and used by "dynamicsRdfId".
            } else {
                // This is the worst possible case, we have to start evicting from "ifdBuffer.StFrameId" as well as forcing the advancement of "lcacIfdId & dynamicsRdfId" to guarantee "perfect continuity of ifdId sequence in broadcasted `DownsyncSnapshot`s from backend".
                int oldLcacIfdId = lcacIfdId;
                /*
                [EXAMPLE] 

                At ifdBuffer = "-1 | [0, ..., 450] 451)", given incoming "ifdId = 459", the ifdBuffer should become "8 | [9, ..., 459] 460)".
                */
                int alreadyConfirmedCntThisRound = (lcacIfdId + 1 - ifdBuffer.StFrameId);
                (*outForceConfirmedStEvictedCnt) += toEvictCnt - alreadyConfirmedCntThisRound;
                lcacIfdId = postEvictionStFrameId-1; // i.e. "ifdBuffer.StFrameId" will be incremented by the same amount later in "ifdBuffer.DryPut()".
                JPH_ASSERT(lcacIfdId < ifdId);
                int downsyncSnapshotStThisRound = oldLcacIfdId+1;
                int targetDownsyncSnapshotEdThisRound = lcacIfdId+1;
                int initDownsyncSnapshotEdThisRound = (targetDownsyncSnapshotEdThisRound < ifdBuffer.EdFrameId ? targetDownsyncSnapshotEdThisRound : ifdBuffer.EdFrameId);
                produceDownsyncSnapshot(allConfirmedMask, downsyncSnapshotStThisRound, initDownsyncSnapshotEdThisRound, false, &result); // No need to specify an accurate "unconfirmedMask" in this worst case
                int gapCntThisRound = (targetDownsyncSnapshotEdThisRound - initDownsyncSnapshotEdThisRound);
#ifndef NDEBUG
                std::ostringstream oss;
                oss << "@dynamicsRdfId=" << dynamicsRdfId << ", @oldLcacIfdId=" << oldLcacIfdId << ", @fromTcp=" << fromTcp << ", incomingIfdId=" << ifdId << " triggers insitu force confirmation, will evict " << toEvictCnt << " from StFrameId=" << ifdBuffer.StFrameId << " with initGapCntThisRound=" << gapCntThisRound; 
                Debug::Log(oss.str());
#endif
                while (0 < gapCntThisRound) {
                    auto resultIfdBatchHolder = result->mutable_ifd_batch();
                    InputFrameDownsync* virtualIfd = resultIfdBatchHolder->Add(); // [REMINDER] Will allocate in the same arena
                    for (int k = 0; k < playersCnt; ++k) {
                        if (0 < (inactiveJoinMask & CalcJoinIndexMask(k + 1))) {
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

            // [WARNING] As "!willUpdateExisting && ifdBufferFull", we should call "Step(...)" to advance "dynamicsRdfId" such that all "<= lcacIfdId" are used by "dynamicsRdfId" before being any "StFrameId eviction upon DryPut() of ifdBuffer".
            int nextDynamicsRdfId = ConvertToLastUsedRenderFrameId(lcacIfdId) + 1;
            bool stepped = Step(dynamicsRdfId, nextDynamicsRdfId, result);
            if (!stepped) {
                if (nullptr != result) {
                    *outBytesCntLimit = 0;
                    releaseDownsyncSnapshotArenaOwnership(result);
                }
                return false;
            }

            // Now that we're all set for "StFrameId eviction upon DryPut() of ifdBuffer"
        }
        bool outExistingInputMutated = false;
        bool canConfirmTcp = (fromUdp || fromTcp); // Backend doesn't care about whether it's from TCP or UDP, either can set "ifd.confirmed_list"
        InputFrameDownsync* ifd = getOrPrefabInputFrameDownsync(ifdId, peerJoinIndex, cmd, canConfirmTcp, canConfirmTcp, outExistingInputMutated);
        int peerJoinIndexArrIdx = peerJoinIndex - 1;
        bool frontsUpdated = updatePlayerInputFronts(ifdId, peerJoinIndexArrIdx, cmd);
/*
#ifndef NDEBUG
        if (frontsUpdated) {
            Debug::Log("OnUpsyncSnapshotReceived/C++ playerInputFrontIds[" + std::to_string(peerJoinIndexArrIdx) + "] is updated to " + std::to_string(ifdId) + ", playerInputFrontIdsSorted.size=" + std::to_string(playerInputFrontIdsSorted.size()), DColor::Orange);
        } else {
            Debug::Log("OnUpsyncSnapshotReceived/C++ playerInputFrontIds[" + std::to_string(peerJoinIndexArrIdx) + "] NOT updated for ifdId=" + std::to_string(ifdId) + ", origIfdId=" + std::to_string(playerInputFrontIds[peerJoinIndexArrIdx]) + ", playerInputFrontIdsSorted.size=" + std::to_string(playerInputFrontIdsSorted.size()), DColor::Orange);
        }
#endif
*/
        if (inNewAllConfirmedTrend && ifd->confirmed_list() != allConfirmedMask) {
            // [WARNING] "StFrameId eviction upon DryPut() of ifdBuffer" might occur even during "inNewAllConfirmedTrend", but that'll be captured in the "if (!willUpdateExisting && ifdBufferFull)" block. 
            inNewAllConfirmedTrend = false;
            int oldLcacIfdId = moveForwardLastConsecutivelyAllConfirmedIfdId(ifdBuffer.EdFrameId, inactiveJoinMaskVal); // If "ifdBuffer" is not full, then "lcacIfdId" might've never advanced during the current traversal
            if (oldLcacIfdId < lcacIfdId) {
                uint64_t unconfirmedMask = inactiveJoinMaskVal; // No eviction occurred yet, thus "unconfirmedMask" is just the immediate "inactiveJoinMaskVal"
                produceDownsyncSnapshot(unconfirmedMask, oldLcacIfdId + 1, lcacIfdId + 1, false, &result);
                // As long as "dynamicsRdfId" can still find its "delayedIfd" to use, i.e. no "StFrameId eviction upon DryPut() of ifdBuffer" occurs, we try to AVOID calling "Step(...)" as much as possible.
            }
        }

        JPH_ASSERT(lcacIfdId+1 >= ifdBuffer.StFrameId); // A backend-specific constraint.
    }

    if (!playerInputFrontIdsSorted.empty()) {
        *outMaxPlayerInputFrontId = *playerInputFrontIdsSorted.rbegin();
        *outMinPlayerInputFrontId = *playerInputFrontIdsSorted.begin();
/*
#ifndef NDEBUG
        Debug::Log("OnUpsyncSnapshotReceived/C++ updated maxPlayerInputFrontId=" + std::to_string(*outMaxPlayerInputFrontId) + ", minPlayerInputFrontId=" + std::to_string(*outMinPlayerInputFrontId) + " after handling with playerInputFrontIdsSorted.size=" + std::to_string(playerInputFrontIdsSorted.size()), DColor::Orange);
#endif
*/
    }

    if (inNewAllConfirmedTrend) {
        // Wrap up
        int oldLcacIfdId = moveForwardLastConsecutivelyAllConfirmedIfdId(ifdBuffer.EdFrameId, inactiveJoinMaskVal);
        if (oldLcacIfdId < lcacIfdId) {
            uint64_t unconfirmedMask = inactiveJoinMaskVal; // No eviction occurred yet, thus "unconfirmedMask" is just the immediate "inactiveJoinMaskVal"
            produceDownsyncSnapshot(unconfirmedMask, oldLcacIfdId + 1, lcacIfdId + 1, false, &result);
        }
    }

    *outNewLcacIfdId = lcacIfdId;
    *outNewDynamicsRdfId = dynamicsRdfId;

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

bool BackendBattle::WriteSingleStepFrameLog(int currRdfId, RenderFrame* nextRdf, int delayedIfdId, InputFrameDownsync* delayedIfd) {
    FrameLog* nextFrameLog = frameLogBuffer.GetByFrameId(currRdfId + 1);
    if (!nextFrameLog) {
        nextFrameLog = frameLogBuffer.DryPut();
    }
    if (nextFrameLog->has_rdf()) {
        auto res = nextFrameLog->release_rdf();
    }
    nextFrameLog->set_allocated_rdf(nextRdf); // No copy, neither is arena-allocated.
    nextFrameLog->set_actually_used_ifd_id(delayedIfdId);
    nextFrameLog->set_used_ifd_confirmed_list(delayedIfd->confirmed_list());
    nextFrameLog->set_used_ifd_udp_confirmed_list(delayedIfd->udp_confirmed_list());
    nextFrameLog->set_timer_rdf_id(currRdfId);
    nextFrameLog->set_lcac_ifd_id(lcacIfdId);
    nextFrameLog->set_chaser_st_rdf_id(0);
    nextFrameLog->set_chaser_ed_rdf_id(0);
    nextFrameLog->set_chaser_rdf_id_lower_bound(0);
    auto inputListHolder = nextFrameLog->mutable_used_ifd_input_list();
    inputListHolder->Clear();
    inputListHolder->CopyFrom(delayedIfd->input_list());
    return true;
}

bool BackendBattle::Step(int fromRdfId, int toRdfId, DownsyncSnapshot* virtualIfds) {
    for (int currRdfId = fromRdfId; currRdfId < toRdfId; ++currRdfId) {
        int delayedIfdId = ConvertToDelayedInputFrameId(dynamicsRdfId);
        InputFrameDownsync* delayedIfd = ifdBuffer.GetByFrameId(delayedIfdId);
        if (nullptr == delayedIfd && nullptr != virtualIfds) {
            auto ifdBatchPayload = virtualIfds->mutable_ifd_batch();
            delayedIfd = ifdBatchPayload->Mutable(delayedIfdId - virtualIfds->st_ifd_id());
        }
        JPH_ASSERT(nullptr != delayedIfd);
        RenderFrame* nextRdf = CalcSingleStep(currRdfId, delayedIfdId, delayedIfd);
        if (frameLogEnabled) {
            WriteSingleStepFrameLog(currRdfId, nextRdf, delayedIfdId, delayedIfd);
        }
        dynamicsRdfId = currRdfId + 1;
    }
    return true;
}

bool BackendBattle::MoveForwardLcacIfdIdAndStep(bool withRefRdf, int* outOldLcacIfdId, int* outNewLcacIfdId, int* outOldDynamicsRdfId, int* outNewDynamicsRdfId, char* outBytesPreallocatedStart, long* outBytesCntLimit) {
    uint64_t inactiveJoinMaskVal = inactiveJoinMask.load();
    *outOldDynamicsRdfId = dynamicsRdfId;
    int oldLcacIfdId = moveForwardLastConsecutivelyAllConfirmedIfdId(ifdBuffer.EdFrameId, inactiveJoinMaskVal); // If "ifdBuffer" is not full, then "lcacIfdId" might've never advanced during the current traversal
    *outOldLcacIfdId = oldLcacIfdId;
    *outNewLcacIfdId = lcacIfdId;
    *outNewDynamicsRdfId = dynamicsRdfId;

    *outNewDynamicsRdfId = ConvertToLastUsedRenderFrameId(lcacIfdId) + 1;
    if (0 <= lcacIfdId && *outOldDynamicsRdfId < *outNewDynamicsRdfId) {
        bool stepped = Step(*outOldDynamicsRdfId, *outNewDynamicsRdfId);
        if (!stepped) {
            *outBytesCntLimit = 0;
            return false;
        }
/*
#ifndef NDEBUG
        std::ostringstream oss;
        oss << "dynamicsRdfId: " << *outOldDynamicsRdfId << " -> " << *outNewDynamicsRdfId << ", @oldLcacIfdId=" << oldLcacIfdId << ", @newLcacIfdId=" << lcacIfdId << ", stepped."; 
        Debug::Log(oss.str());
#endif
*/
        uint64_t unconfirmedMask = inactiveJoinMaskVal;
        DownsyncSnapshot* result = nullptr;
        produceDownsyncSnapshot(unconfirmedMask, oldLcacIfdId + 1, lcacIfdId + 1, withRefRdf, &result);

        long byteSize = result->ByteSizeLong();
        if (byteSize > *outBytesCntLimit) {
            releaseDownsyncSnapshotArenaOwnership(result);
            return false;
        }
        *outBytesCntLimit = byteSize;
        result->SerializeToArray(outBytesPreallocatedStart, byteSize);
        releaseDownsyncSnapshotArenaOwnership(result);

        return true;
    } else {
        *outBytesCntLimit = 0;
        return true;
    }
}

int BackendBattle::GetDynamicsRdfId() {
    return dynamicsRdfId;
}
