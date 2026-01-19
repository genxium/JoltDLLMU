#include "FrontendBattle.h"
#include <string>

bool FrontendBattle::UpsertSelfCmd(uint64_t inSingleInput, int* outChaserRdfId) {
    int toGenIfdId = BaseBattle::ConvertToGeneratingIfdId(timerRdfId);
    int nextRdfToGenIfdId = BaseBattle::ConvertToGeneratingIfdId(timerRdfId+1);
    bool isLastRdfInIfdCoverage = (nextRdfToGenIfdId > toGenIfdId);
    bool changedFromPrevSelfCmd = false;
    auto prevIfd = ifdBuffer.GetByFrameId(toGenIfdId-1);
    if (nullptr != prevIfd) {
        changedFromPrevSelfCmd = (prevIfd->input_list(selfJoinIndexArrIdx) != inSingleInput);
    }
    bool canConfirmUDP = (isLastRdfInIfdCoverage || changedFromPrevSelfCmd);
    bool outExistingInputMutated = false;
    InputFrameDownsync* result = getOrPrefabInputFrameDownsync(toGenIfdId, selfJoinIndex, inSingleInput, canConfirmUDP, false, outExistingInputMutated);
    if (outExistingInputMutated) {
        handleIncorrectlyRenderedPrediction(toGenIfdId, true, false, false);
    }

    *outChaserRdfId = chaserRdfId;
    if (!result) {
        return false;
    }
/*
#ifndef NDEBUG
    if (0 < (selfJoinIndexMask & result->confirmed_list()) && inSingleInput != result->input_list(selfJoinIndexArrIdx)) {
        std::ostringstream oss;
        oss << "@timerRdfId=" << timerRdfId << ", @toGenIfdId=" << toGenIfdId << ", @lcacIfdId=" << lcacIfdId << ", @chaserRdfId=" << chaserRdfId << ", @chaserRdfIdLowerBound=" << chaserRdfIdLowerBound << ", rejected inSingleInput=" << inSingleInput << " due to alreadyTcpConfirmed selfInput=" << result->input_list(selfJoinIndexArrIdx);
        Debug::Log(oss.str(), DColor::Orange);
    }
#endif
*/
    return true;
}

bool FrontendBattle::OnDownsyncSnapshotReceived(char* inBytes, int inBytesCnt, int* outPostTimerRdfEvictedCnt, int* outPostTimerRdfDelayedIfdEvictedCnt, int* outChaserRdfId, int* outLcacIfdId, int* outMaxPlayerInputFrontId, int* outMinPlayerInputFrontId) {
    if (nullptr == downsyncSnapshotHolder) {
        downsyncSnapshotHolder = google::protobuf::Arena::Create<DownsyncSnapshot>(&pbTempAllocator);
    }
    downsyncSnapshotHolder->ParseFromArray(inBytes, inBytesCnt);
    return OnDownsyncSnapshotReceived(downsyncSnapshotHolder, outPostTimerRdfEvictedCnt, outPostTimerRdfDelayedIfdEvictedCnt, outChaserRdfId, outLcacIfdId, outMaxPlayerInputFrontId, outMinPlayerInputFrontId);
}

bool FrontendBattle::OnDownsyncSnapshotReceived(const DownsyncSnapshot* downsyncSnapshot, int* outPostTimerRdfEvictedCnt, int* outPostTimerRdfDelayedIfdEvictedCnt, int* outChaserRdfId, int* outLcacIfdId, int* outMaxPlayerInputFrontId, int* outMinPlayerInputFrontId) {
    /*
    Assuming that rdfBuffer & ifdBuffer are both sufficiently large (e.g. 5 seconds) such that when 
    - "timerRdfId" is to be evicted from "rdfBuffer.StFrameId", or
    - "ConvertToDelayedInputFrameId(timerRdfId)" is to be evicted from "ifdBuffer.StFrameId"
    , it's considered TOO LAGGY on frontend, thus reassigning "timerRdfId = chaserRdfIdLowerBound = refRdfId" will be allowed.

    Moreover if no such eviction is to occur, the "refRdf & ifdBatch" handlings are fully de-coupled.

    Edge cases
    - If there's no "refRdf" BUT "0 < postTimerRdfDelayedIfdToEvictCnt", drop the exceeding "ifd_batch".
    */
    *outChaserRdfId = chaserRdfId;
    *outLcacIfdId = lcacIfdId;
    bool shouldDragTimerRdfIdForward = false;
    *outPostTimerRdfEvictedCnt = 0;
    *outPostTimerRdfDelayedIfdEvictedCnt = 0;
    int refRdfId = downsyncSnapshot->ref_rdf_id();
    int oldChaserRdfIdLowerBound = chaserRdfIdLowerBound;
    if (downsyncSnapshot->has_ref_rdf() && refRdfId != globalPrimitiveConsts->terminating_render_frame_id()) {
        const RenderFrame& refRdf = downsyncSnapshot->ref_rdf();
        if (refRdfId >= oldChaserRdfIdLowerBound) {
            bool willEvictRdfSt = (refRdfId >= rdfBuffer.StFrameId + rdfBuffer.N);
            if (willEvictRdfSt) {
                int toEvictRdfCnt = (refRdfId - rdfBuffer.StFrameId - rdfBuffer.N + 1);
                int postEvictionRdfStFrameId = rdfBuffer.StFrameId + toEvictRdfCnt;
                shouldDragTimerRdfIdForward = (timerRdfId < postEvictionRdfStFrameId);
                if (shouldDragTimerRdfIdForward) {
                    *outPostTimerRdfEvictedCnt = (postEvictionRdfStFrameId - timerRdfId);
                }
            }
            
            while (rdfBuffer.EdFrameId <= refRdfId) {
                int gapRdfId = rdfBuffer.EdFrameId; 
                RenderFrame* holder = rdfBuffer.DryPut();
                holder->set_id(gapRdfId);
            }
            
            RenderFrame* targetHolder = rdfBuffer.GetByFrameId(refRdfId);
            JPH_ASSERT(nullptr != targetHolder);
            targetHolder->CopyFrom(refRdf);

            chaserRdfId = refRdfId;
            chaserRdfIdLowerBound = refRdfId;
/*
#ifndef NDEBUG
            std::ostringstream oss;
            oss << "@timerRdfId=" << timerRdfId << ", @oldLcacIfdId=" << lcacIfdId << ", updated chaserRdfIdLowerBound=chaserRdfId=" << refRdfId << " from refRdf, willEvictRdfSt=" << willEvictRdfSt << ", shouldDragTimerRdfIdForward=" << shouldDragTimerRdfIdForward << ", postTimerRdfEvictedCnt=" << *outPostTimerRdfEvictedCnt;
            Debug::Log(oss.str(), DColor::Orange);
#endif
*/
        }
    }

    bool shouldDragTimerRdfUsingDelayedIfdIdForward = false; 
    int firstIncorrectlyPredictedIfdId = -1;
    int ifdBatchSize = downsyncSnapshot->ifd_batch_size();
    if (0 < ifdBatchSize) {
        int delayedIfdId = BaseBattle::ConvertToDelayedInputFrameId(timerRdfId);
        for (int i = 0; i < ifdBatchSize; ++i) {
            int ifdId = downsyncSnapshot->st_ifd_id() + i;
            if (ifdId <= lcacIfdId) {
                // obsolete
                continue;
            }
            bool existingInputMutated = false;
            InputFrameDownsync* targetHolder = ifdBuffer.GetByFrameId(ifdId); 
            const InputFrameDownsync& refIfd = downsyncSnapshot->ifd_batch(i);
            if (nullptr != targetHolder && -1 == firstIncorrectlyPredictedIfdId) {
                for (int k = 0; k < playersCnt; ++k) {
                    if (targetHolder->input_list(k) != refIfd.input_list(k)) {
                        existingInputMutated = true; 
                        break;
                    }
                }
            } else {
                bool willEvictIfdSt = (ifdId >= ifdBuffer.StFrameId + ifdBuffer.N);
                if (willEvictIfdSt) {
                    int toEvictIfdCnt = (ifdId - ifdBuffer.StFrameId - ifdBuffer.N + 1);
                    JPH_ASSERT(1 == toEvictIfdCnt || (1 < toEvictIfdCnt && 0 == i));  
                    int postEvictionIfdStFrameId = ifdBuffer.StFrameId + toEvictIfdCnt;
                    bool willDragTimerRdfUsingDelayedIfdIdForward = (delayedIfdId < postEvictionIfdStFrameId);
                    if (willDragTimerRdfUsingDelayedIfdIdForward) {
                        if (!downsyncSnapshot->has_ref_rdf()) {
                            break;
                        } else {
                            shouldDragTimerRdfUsingDelayedIfdIdForward = true;
                            *outPostTimerRdfDelayedIfdEvictedCnt = (postEvictionIfdStFrameId - delayedIfdId); // Will pick the LAST value
                        }
                    }
                    // [WARNING] No need to check "shouldDragLcacIfdIdForward" here, it'll be assigned by "ifdId" at the current iteration.
                }

                while (ifdBuffer.EdFrameId <= ifdId) {
                    InputFrameDownsync* holder = ifdBuffer.DryPut();
                    auto inputList = holder->mutable_input_list();
                    inputList->Clear();
                    for (int k = 0; k < playersCnt; ++k) {
                        if (0 < (inactiveJoinMask & CalcJoinIndexMask(k + 1))) {
                            inputList->Add(0);
                        } else {
                            inputList->Add(playerInputFronts[k]);
                        }
                    }
                    holder->set_confirmed_list(0);
                    holder->set_udp_confirmed_list(0);
                }

                targetHolder = ifdBuffer.GetByFrameId(ifdId); 
            }
/*
#ifndef NDEBUG
            if ((0 < (targetHolder->udp_confirmed_list() & selfJoinIndexMask) || 0 != targetHolder->input_list(selfJoinIndexArrIdx)) && targetHolder->input_list(selfJoinIndexArrIdx) != refIfd.input_list(selfJoinIndexArrIdx)) {
                std::ostringstream oss;
                oss << "@timerRdfId=" << timerRdfId << ", @refRdfId=" << refRdfId << ", @lcacIfdId=" << lcacIfdId << ", @chaserRdfId=" << chaserRdfId << ", @chaserRdfIdLowerBound=" << chaserRdfIdLowerBound << " overriding localSelfInput=" <<  targetHolder->input_list(selfJoinIndexArrIdx) << " by backend generated " << refIfd.input_list(selfJoinIndexArrIdx) << " of refIfdId=" << ifdId;
                Debug::Log(oss.str(), DColor::Orange);
            }
#endif
*/
            targetHolder->CopyFrom(refIfd);
            targetHolder->set_confirmed_list(allConfirmedMask);
            
            for (int k = 0; k < playersCnt; ++k) {
                if (k == selfJoinIndexArrIdx) continue;
                if (ifdId <= playerInputFrontIds[k]) { 
/*
#ifndef NDEBUG
                    Debug::Log("OnDownsyncSnapshotReceived/C++ playerInputFrontIds[" + std::to_string(k) + "] NOT updated for ifdId=" + std::to_string(ifdId) + ", origIfdId=" + std::to_string(playerInputFrontIds[k]), DColor::Orange);
#endif
*/
                    continue; 
                }
                auto it = playerInputFrontIdsSorted.find(playerInputFrontIds[k]);
                if (it != playerInputFrontIdsSorted.end()) {
                    playerInputFrontIdsSorted.erase(it);
                }
                playerInputFrontIds[k] = ifdId;
                playerInputFronts[k] = refIfd.input_list(k);
                playerInputFrontIdsSorted.insert(ifdId);
/*
#ifndef NDEBUG
                Debug::Log("OnDownsyncSnapshotReceived/C++ playerInputFrontIds[" + std::to_string(k) + "] is updated to " + std::to_string(ifdId) + ", playerInputFrontIdsSorted.size=" + std::to_string(playerInputFrontIdsSorted.size()), DColor::Orange);
#endif
*/
            }

            if (-1 == firstIncorrectlyPredictedIfdId && existingInputMutated) {
                firstIncorrectlyPredictedIfdId = ifdId;
            }
        
            lcacIfdId = ifdId;
        }
    }

    if (-1 != firstIncorrectlyPredictedIfdId) {
        handleIncorrectlyRenderedPrediction(firstIncorrectlyPredictedIfdId, false, false, false);
    }

    if (shouldDragTimerRdfIdForward || shouldDragTimerRdfUsingDelayedIfdIdForward) {
        timerRdfId = refRdfId;
    }

    if (!playerInputFrontIdsSorted.empty()) {
        *outMaxPlayerInputFrontId = *playerInputFrontIdsSorted.rbegin();
        *outMinPlayerInputFrontId = *playerInputFrontIdsSorted.begin();
/*
#ifndef NDEBUG
        Debug::Log("OnDownsyncSnapshotReceived/C++ updated maxPlayerInputFrontId=" + std::to_string(*outMaxPlayerInputFrontId) + ", minPlayerInputFrontId=" + std::to_string(*outMinPlayerInputFrontId) + " after handling with playerInputFrontIdsSorted.size=" + std::to_string(playerInputFrontIdsSorted.size()), DColor::Orange);
#endif
*/
    }
/*
#ifndef NDEBUG
    else {
        Debug::Log("OnDownsyncSnapshotReceived/C++ got empty playerInputFrontIdsSorted after handling, sth is wrong" , DColor::Orange);
     }
#endif
*/

    *outChaserRdfId = chaserRdfId;
    *outLcacIfdId = lcacIfdId;

    return true;
}

bool FrontendBattle::OnUpsyncSnapshotReqReceived(char* inBytes, int inBytesCnt, int* outChaserRdfId, int* outMaxPlayerInputFrontId, int* outMinPlayerInputFrontId) {
    if (nullptr == upsyncSnapshotReqHolder) {
        upsyncSnapshotReqHolder = google::protobuf::Arena::Create<WsReq>(&pbTempAllocator);
    }
    upsyncSnapshotReqHolder->ParseFromArray(inBytes, inBytesCnt);

    uint32_t peerJoinIndex = upsyncSnapshotReqHolder->join_index();
    if (0 >= peerJoinIndex) return false;
    if (selfJoinIndex == peerJoinIndex) return false;
    if (!upsyncSnapshotReqHolder->has_upsync_snapshot()) return false;
    return OnUpsyncSnapshotReceived(peerJoinIndex, upsyncSnapshotReqHolder->upsync_snapshot(), outChaserRdfId, outMaxPlayerInputFrontId, outMinPlayerInputFrontId);
}

bool FrontendBattle::OnUpsyncSnapshotReceived(const uint32_t peerJoinIndex, const UpsyncSnapshot& upsyncSnapshot, int* outChaserRdfId, int* outMaxPlayerInputFrontId, int* outMinPlayerInputFrontId) {
    // See "BackendBattle::OnUpsyncSnapshotReceived" for reference.
    bool fromUdp = true; // by design
    int delayedIfdId = BaseBattle::ConvertToDelayedInputFrameId(timerRdfId);
    int firstIncorrectlyPredictedIfdId = -1;
    int cmdListSize = upsyncSnapshot.cmd_list_size();
    *outChaserRdfId = chaserRdfId;
    for (int i = 0; i < cmdListSize; ++i) {
        int ifdId = upsyncSnapshot.st_ifd_id() + i;
        if (ifdId <= lcacIfdId) {
            // obsolete
            continue;
        }

        bool willEvictSt = (ifdId >= ifdBuffer.StFrameId + ifdBuffer.N);
        if (willEvictSt) {
            int toEvictCnt = (ifdId - ifdBuffer.StFrameId - ifdBuffer.N + 1);
            JPH_ASSERT(1 == toEvictCnt || (1 < toEvictCnt && 0 == i));  
            int postEvictionStFrameId = ifdBuffer.StFrameId + toEvictCnt;
            
            if (postEvictionStFrameId > delayedIfdId) {
                // [WARNING] Discard if it's about to evict the "delayedIfdId" to be used by "timerRdfId", such that we don't have to call "Step(...)" in a tricky way just for handling UDP "upsyncSnapshot".
                break;
            }
            bool shouldDragLcacIfdIdForward = (lcacIfdId + 1 < postEvictionStFrameId);
            if (!shouldDragLcacIfdIdForward) {
            } else {
                lcacIfdId = postEvictionStFrameId-1; // i.e. "ifdBuffer.StFrameId" will be incremented by the same amount later in "ifdBuffer.DryPut()".
                JPH_ASSERT(lcacIfdId < ifdId);
            }

            // Now that we're all set for "StFrameId eviction upon DryPut() of ifdBuffer"
        }

        const uint64_t cmd = upsyncSnapshot.cmd_list(i);
        bool outExistingInputMutated = false;
        InputFrameDownsync* ifd = getOrPrefabInputFrameDownsync(ifdId, peerJoinIndex, cmd, fromUdp, false, outExistingInputMutated);
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
        if (-1 == firstIncorrectlyPredictedIfdId && outExistingInputMutated) {
            firstIncorrectlyPredictedIfdId = ifdId;
        }
    }

    if (-1 != firstIncorrectlyPredictedIfdId) {
        handleIncorrectlyRenderedPrediction(firstIncorrectlyPredictedIfdId, false, fromUdp, false);
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
/*
#ifndef NDEBUG
    else {
        Debug::Log("OnUpsyncSnapshotReceived/C++ got empty playerInputFrontIdsSorted after handling, sth is wrong" , DColor::Orange);
     }
#endif
*/
    *outChaserRdfId = chaserRdfId;

    return true;
}

bool FrontendBattle::ProduceUpsyncSnapshotRequest(int seqNo, int proposedBatchIfdIdSt, int proposedBatchIfdIdEd, int* outLastIfdId, char* outBytesPreallocatedStart, long* outBytesCntLimit) {
    int batchIfdIdSt = proposedBatchIfdIdSt;
    if (batchIfdIdSt < ifdBuffer.StFrameId) {
        batchIfdIdSt = ifdBuffer.StFrameId;
    }
    
    int batchIfdIdEd = proposedBatchIfdIdEd;
    if (batchIfdIdEd > ifdBuffer.EdFrameId) {
        batchIfdIdEd = ifdBuffer.EdFrameId;
    }

    if (batchIfdIdEd < batchIfdIdSt) {
        *outBytesCntLimit = 0;
        return false;
    }

    std::vector<uint64> cmdList;
    cmdList.reserve(batchIfdIdEd - batchIfdIdSt + 1);
    for (int i = batchIfdIdSt; i < batchIfdIdEd; i++) {
        auto ifd = ifdBuffer.GetByFrameId(i);
        if (nullptr == ifd) {
            break;
        }
        bool selfConfirmed = (0 < (selfJoinIndexMask & (ifd->confirmed_list() | ifd->udp_confirmed_list())));
        if (!selfConfirmed) {
            break;
        }
        cmdList.emplace_back(ifd->input_list(selfJoinIndexArrIdx));
        *outLastIfdId = i;
    }

    if (cmdList.empty()) {
        *outBytesCntLimit = 0;
        return false;
    }

    auto result = google::protobuf::Arena::Create<WsReq>(&pbTempAllocator);
    result->set_join_index(selfJoinIndex);
    result->set_auth_key(selfCmdAuthKey);
    result->set_seq_no(seqNo);
    result->set_act(UpsyncAct::UA_CMD);
    auto upsyncSnapshot = result->mutable_upsync_snapshot();
    upsyncSnapshot->set_st_ifd_id(batchIfdIdSt);
    for (auto& cmd : cmdList) {
        upsyncSnapshot->add_cmd_list(cmd);
    }
    long byteSize = result->ByteSizeLong();
    if (byteSize > *outBytesCntLimit) {
        *outBytesCntLimit = 0;
        return false;
    }
    *outBytesCntLimit = byteSize;

    result->SerializeToArray(outBytesPreallocatedStart, byteSize);
    return true;
}

bool FrontendBattle::WriteSingleStepFrameLog(int currRdfId, RenderFrame* nextRdf, int fromRdfId, int toRdfId, int delayedIfdId, InputFrameDownsync* delayedIfd, bool isChasing, bool snatched) {
    JPH_ASSERT(currRdfId + 1 == nextRdf->id());
    FrameLog* nextFrameLog = frameLogBuffer.GetByFrameId(currRdfId + 1);
    if (!nextFrameLog) {
        nextFrameLog = frameLogBuffer.DryPut();
    }
    if (nextFrameLog->has_rdf()) {
        auto res = nextFrameLog->release_rdf();
    }
    auto inputListHolder = nextFrameLog->mutable_used_ifd_input_list();
    inputListHolder->Clear();
    uint64_t tcpConfirmedList = delayedIfd->confirmed_list();
    uint64_t udpConfirmedList = delayedIfd->udp_confirmed_list();
    nextFrameLog->set_used_ifd_confirmed_list(tcpConfirmedList);
    nextFrameLog->set_used_ifd_udp_confirmed_list(udpConfirmedList);
    nextFrameLog->set_actually_used_ifd_id(delayedIfdId);
    inputListHolder->CopyFrom(delayedIfd->input_list());
    nextFrameLog->set_chaser_rdf_id_lower_bound_snatched(snatched);
    nextFrameLog->set_allocated_rdf(nextRdf); // No copy, neither is arena-allocated.
    nextFrameLog->set_timer_rdf_id(timerRdfId);
    nextFrameLog->set_chaser_rdf_id(chaserRdfId);
    nextFrameLog->set_chaser_rdf_id_lower_bound(chaserRdfIdLowerBound);
    nextFrameLog->set_lcac_ifd_id(lcacIfdId);
    if (isChasing) {
        nextFrameLog->set_chaser_st_rdf_id(fromRdfId);
        nextFrameLog->set_chaser_ed_rdf_id(toRdfId);
    } else {
        nextFrameLog->set_chaser_st_rdf_id(0);
        nextFrameLog->set_chaser_ed_rdf_id(0);
    }

    /*
#ifndef NDEBUG
    if (delayedIfdId > lcacIfdId && allConfirmedMask == tcpConfirmedList) {
        std::ostringstream oss;
        oss << "Why @timerRdfId=" << timerRdfId << ", @currRdfId=" << currRdfId << ", (delayedIfdId:" << delayedIfdId << ") > (lcacIfdId:" << lcacIfdId << ") but delayedIfd is all confirmed? @chaserRdfId=" << chaserRdfId << ", chaserRdfIdLowerBound=" << chaserRdfIdLowerBound;
        Debug::Log(oss.str(), DColor::Orange);
    } else {
        std::ostringstream oss;
        oss << "Written @timerRdfId=" << timerRdfId << ", @currRdfId=" << currRdfId << ", (delayedIfdId:" << delayedIfdId << ", lcacIfdId:" << lcacIfdId << ", tcpConfirmedList=" << tcpConfirmedList << ", udpConfirmedList=" << udpConfirmedList << ") @chaserRdfId=" << chaserRdfId << ", chaserRdfIdLowerBound=" << chaserRdfIdLowerBound;
        Debug::Log(oss.str(), DColor::White);
    }
#endif
    */
    return true;
}

bool FrontendBattle::Step() {
    int delayedIfdId = ConvertToDelayedInputFrameId(timerRdfId);
    InputFrameDownsync* delayedIfd = nullptr;

    RenderFrame* nextRdf = nullptr;
    bool snatched = (timerRdfId + 1 == chaserRdfIdLowerBound);
    if (snatched) {
        delayedIfd = ifdBuffer.GetByFrameId(delayedIfdId);
        JPH_ASSERT(nullptr != delayedIfd);
        nextRdf = rdfBuffer.GetByFrameId(chaserRdfIdLowerBound); // [WARNING] DON'T re-calculate if an advanced "chaserRdfIdLowerBound" has been received;
    } else {
        regulateCmdBeforeRender();
        delayedIfd = ifdBuffer.GetByFrameId(delayedIfdId);
        JPH_ASSERT(nullptr != delayedIfd);

        nextRdf = BaseBattle::CalcSingleStep(timerRdfId, delayedIfdId, delayedIfd);
    }
    JPH_ASSERT(nullptr != nextRdf);

    if (frameLogEnabled) {
        WriteSingleStepFrameLog(timerRdfId, nextRdf, timerRdfId, timerRdfId + 1, delayedIfdId, delayedIfd, false, snatched);
    }

    if (chaserRdfId == timerRdfId) {
        timerRdfId++;
        chaserRdfId++;
    } else {
        timerRdfId++;
    }

    return true;
}

bool FrontendBattle::ChaseRolledBackRdfs(int* outChaserRdfId, bool toTimerRdfId) {
    *outChaserRdfId = chaserRdfId;
    int fromRdfId = chaserRdfId, toRdfId = chaserRdfId + globalPrimitiveConsts->max_chasing_render_frames_per_update();
    if (toTimerRdfId) {
        toRdfId = timerRdfId;
    } else if (toRdfId > timerRdfId) {
        toRdfId = timerRdfId;
    }
    for (int currRdfId = fromRdfId; currRdfId < toRdfId; ++currRdfId) {
        int delayedIfdId = ConvertToDelayedInputFrameId(currRdfId);
        InputFrameDownsync* delayedIfd = ifdBuffer.GetByFrameId(delayedIfdId);
        JPH_ASSERT(nullptr != delayedIfd);
        auto nextRdf = BaseBattle::CalcSingleStep(currRdfId, delayedIfdId, delayedIfd);
        if (frameLogEnabled) {
            WriteSingleStepFrameLog(currRdfId, nextRdf, fromRdfId, toRdfId, delayedIfdId, delayedIfd, true);
        }
        chaserRdfId++;
    }
    *outChaserRdfId = chaserRdfId;
    return true;
}

void FrontendBattle::regulateCmdBeforeRender() {
    int delayedIfdId = BaseBattle::ConvertToDelayedInputFrameId(timerRdfId);
    if (delayedIfdId <= lcacIfdId) {
        return;
    }

    RenderFrame* currRdf = rdfBuffer.GetByFrameId(timerRdfId); 
    InputFrameDownsync* delayedIfd = ifdBuffer.GetByFrameId(delayedIfdId); // No need to prefab, it must exist
    bool existingInputMutated = false;
    for (int i = 0; i < playersCnt; i++) {
        int joinIndex = (i + 1); 
        if (joinIndex == selfJoinIndex) {
            continue;
        }
        uint64_t newVal = delayedIfd->input_list(i);
        uint64_t joinMask = (U64_1 << i);
        auto& playerChd = currRdf->players(i);
        auto& chd = playerChd.chd();
        // [WARNING] Remove any predicted "rising edge" or a "falling edge" of any critical button before rendering.
        bool shouldPredictBtnAHold = (globalPrimitiveConsts->jammed_btn_holding_rdf_cnt() == chd.btn_a_holding_rdf_cnt()) || (0 < chd.btn_a_holding_rdf_cnt());
        bool shouldPredictBtnBHold = (globalPrimitiveConsts->jammed_btn_holding_rdf_cnt() == chd.btn_b_holding_rdf_cnt()) || (0 < chd.btn_b_holding_rdf_cnt());
        bool shouldPredictBtnCHold = (globalPrimitiveConsts->jammed_btn_holding_rdf_cnt() == chd.btn_c_holding_rdf_cnt()) || (0 < chd.btn_c_holding_rdf_cnt());
        bool shouldPredictBtnDHold = (globalPrimitiveConsts->jammed_btn_holding_rdf_cnt() == chd.btn_d_holding_rdf_cnt()) || (0 < chd.btn_d_holding_rdf_cnt());
        bool shouldPredictBtnEHold = (globalPrimitiveConsts->jammed_btn_holding_rdf_cnt() == chd.btn_e_holding_rdf_cnt()) || (0 < chd.btn_e_holding_rdf_cnt());
      
        if (0 < (delayedIfd->confirmed_list() & joinMask)) {
            // This regulation is only valid when "delayed input for this player is not yet confirmed"
            continue;
        } 

        if (0 < (delayedIfd->udp_confirmed_list() & joinMask)) {
            // Received from UDP, better than local prediction though "InputFrameDownsync.confirmed_list()" is not set until confirmed by TCP path
            continue;
        }

        // Local prediction
        uint64_t refCmd = 0;
        InputFrameDownsync* previousDelayedIfd = ifdBuffer.GetByFrameId(delayedIfdId - 1);
        if (playerInputFrontIds[i] < delayedIfdId) {
            refCmd = playerInputFronts[i];
        } else if (nullptr != previousDelayedIfd) {
            refCmd = previousDelayedIfd->input_list(i);
        }

        newVal = (refCmd & U64_15);
        if (shouldPredictBtnAHold) newVal |= (refCmd & U64_16);
        if (shouldPredictBtnBHold) newVal |= (refCmd & U64_32);
        if (shouldPredictBtnCHold) newVal |= (refCmd & U64_64);
        if (shouldPredictBtnDHold) newVal |= (refCmd & U64_128);
        if (shouldPredictBtnEHold) newVal |= (refCmd & U64_256);

        if (newVal != delayedIfd->input_list(i)) {
            existingInputMutated = true;
            delayedIfd->set_input_list(i, newVal);
        }
    }

    if (existingInputMutated) {
        handleIncorrectlyRenderedPrediction(delayedIfdId, false, false, true);
    }
}

void FrontendBattle::handleIncorrectlyRenderedPrediction(int mismatchedInputFrameId, bool fromSelf, bool fromUdp, bool fromRegulateBeforeRender) {
    if (0 > mismatchedInputFrameId) return;
    int timerRdfId1 = ConvertToFirstUsedRenderFrameId(mismatchedInputFrameId);
    if (timerRdfId1 >= chaserRdfId) return;
    // By now timerRdfId1 < chaserRdfId, it's pretty impossible that "timerRdfId1 > timerRdfId" but we're still checking.
    if (timerRdfId1 > timerRdfId) return; // The incorrect prediction is not yet rendered, no visual impact for player.
    int timerRdfId2 = ConvertToLastUsedRenderFrameId(mismatchedInputFrameId);
    if (timerRdfId2 <= chaserRdfIdLowerBound) {
        /*
           [WARNING]

           There's no need to reset "chaserRdfId" if the impact of this input mismatch couldn't even reach "chaserRdfIdLowerBound".
         */
        return;
    }
    /*
       A typical case is as follows.
       --------------------------------------------------------
       <timerRdfId1>                           :              36


       <this.chaserRdfId>                 :              62

       [this.timerRdfId]                       :              64
       --------------------------------------------------------
     */

    // The actual rollback-and-chase would later be executed in "Step(...)".
    if (timerRdfId1 < rdfBuffer.StFrameId) {
        timerRdfId1 = rdfBuffer.StFrameId; // [WARNING] One of the edge cases here, just wait for the next "refRdf".
    }
/*
#ifndef NDEBUG
    std::ostringstream oss;
    int localToGenIfdId = ConvertToGeneratingIfdId(timerRdfId);
    int localRequiredIfdId = ConvertToDelayedInputFrameId(timerRdfId);
    oss << "@timerRdfId=" << timerRdfId << ", @localToGenIfdId=" << localToGenIfdId << ", @localRequiredIfdId=" << localRequiredIfdId << ", @maxPlayerInputFrontId=" << *playerInputFrontIdsSorted.rbegin() << ", @minPlayerInputFrontId=" << *playerInputFrontIdsSorted.begin() << ", @lcacIfdId=" << lcacIfdId << ", rewinding chaserRdfId from " << chaserRdfId << " to " << timerRdfId1 << " due to mismatchedInputFrameId=" << mismatchedInputFrameId << ", fromSelf=" << fromSelf << ", peerInputSource=" << (fromUdp ? "UDP" : "TCP") << ", fromRegulateBeforeRender=" << fromRegulateBeforeRender;

    Debug::Log(oss.str(), DColor::Orange);
#endif
*/
    chaserRdfId = timerRdfId1;
}

void FrontendBattle::Clear() {
    BaseBattle::Clear();
    upsyncSnapshotReqHolder = nullptr;
    downsyncSnapshotHolder = nullptr;
}

bool FrontendBattle::ResetStartRdf(char* inBytes, int inBytesCnt, const uint32_t inSelfJoinIndex, const char * const inSelfPlayerId, const int inSelfCmdAuthKey) {
    WsReq* initializerMapData = google::protobuf::Arena::Create<WsReq>(&pbTempAllocator);
    initializerMapData->ParseFromArray(inBytes, inBytesCnt);
    return ResetStartRdf(initializerMapData, inSelfJoinIndex, inSelfPlayerId, inSelfCmdAuthKey);
}

bool FrontendBattle::ResetStartRdf(WsReq* initializerMapData, const uint32_t inSelfJoinIndex, const char * const inSelfPlayerId, const int inSelfCmdAuthKey) {
    if (0 >= inSelfJoinIndex) {
        return false;
    }
    selfJoinIndex = inSelfJoinIndex;
    selfJoinIndexInt = (int)inSelfJoinIndex;
    selfJoinIndexArrIdx = selfJoinIndexInt-1;
    selfJoinIndexMask = (U64_1 << selfJoinIndexArrIdx);
    selfPlayerId = inSelfPlayerId;
    selfCmdAuthKey = inSelfCmdAuthKey;
    
    bool res = BaseBattle::ResetStartRdf(initializerMapData);
    timerRdfId = rdfBuffer.GetLast()->id();
    chaserRdfId = chaserRdfIdLowerBound = timerRdfId;
    return res;
}

void FrontendBattle::postStepSingleChdStateCorrection(const int steppingRdfId, const uint64_t udt, const uint64_t ud, const CH_COLLIDER_T* chCollider, const CharacterDownsync& currChd, CharacterDownsync* nextChd, const CharacterConfig* cc, bool cvSupported, bool cvInAir, bool cvOnWall, bool currNotDashing, bool currEffInAir, bool oldNextNotDashing, bool oldNextEffInAir, bool inJumpStartupOrJustEnded, CharacterBase::EGroundState cvGroundState, const InputInducedMotion* inputInducedMotion) {
    BaseBattle::postStepSingleChdStateCorrection(steppingRdfId, udt, ud, chCollider, currChd, nextChd, cc, cvSupported, cvInAir, cvOnWall, currNotDashing, currEffInAir, oldNextNotDashing, oldNextEffInAir, inJumpStartupOrJustEnded, cvGroundState, inputInducedMotion);
/*
#ifndef NDEBUG
    auto udPayload = getUDPayload(ud);
    if (udt == UDT_PLAYER && udPayload == selfJoinIndex && CrouchIdle1 == currChd.ch_state() && CrouchIdle1 != nextChd->ch_state()) {
        std::ostringstream oss;
        int localToGenIfdId = ConvertToGeneratingIfdId(timerRdfId);
        int localRequiredIfdId = ConvertToDelayedInputFrameId(steppingRdfId);
        oss << "@timerRdfId=" << timerRdfId << ", @localToGenIfdId=" << localToGenIfdId << ", @steppingRdfId=" << steppingRdfId << ", @localRequiredIfdId=" << localRequiredIfdId << ", @maxPlayerInputFrontId=" << *playerInputFrontIdsSorted.rbegin() << ", @minPlayerInputFrontId=" << *playerInputFrontIdsSorted.begin() << ", self character at(" << currChd.x() << ", " << currChd.y() << ") w / frames_in_ch_state = " << currChd.frames_in_ch_state() << ", vel = (" << currChd.vel_x() << ", " << currChd.vel_y() << ") revoked from " << currChd.ch_state() << " to " << nextChd->ch_state() << ", nextVel = (" << nextChd->vel_x() << ", " << nextChd->vel_y() << ")" << ", cvSupported = " << cvSupported << ", cvOnWall = " << cvOnWall << ", cvInAir = " << cvInAir << ", cvGroundState = " << (int)cvGroundState << " | self ifdBuffer = \n";

        stringifyPlayerInputsInIfdBuffer(oss, selfJoinIndexArrIdx);
        Debug::Log(oss.str(), DColor::Orange);
    }
#endif
*/
}
