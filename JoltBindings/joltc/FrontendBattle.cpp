#include "FrontendBattle.h"
#include <string>

bool FrontendBattle::UpsertSelfCmd(uint64_t inSingleInput) {
    int toGenIfdId = BaseBattle::ConvertToGeneratingIfdId(timerRdfId);
    int nextRdfToGenIfdId = BaseBattle::ConvertToGeneratingIfdId(timerRdfId+1);
    bool isLastRdfInIfdCoverage = (nextRdfToGenIfdId > toGenIfdId);

    bool outExistingInputMutated = false;
    InputFrameDownsync* result = getOrPrefabInputFrameDownsync(toGenIfdId, selfJoinIndex, inSingleInput, isLastRdfInIfdCoverage, false, outExistingInputMutated);
    if (outExistingInputMutated) {
        handleIncorrectlyRenderedPrediction(toGenIfdId, true);
    }

    if (!result) {
        return false;
    }

    return true;
}

bool FrontendBattle::OnDownsyncSnapshotReceived(char* inBytes, int inBytesCnt, int* outPostTimerRdfEvictedCnt, int* outPostTimerRdfDelayedIfdEvictedCnt) {
    if (nullptr == downsyncSnapshotHolder) {
        downsyncSnapshotHolder = google::protobuf::Arena::Create<DownsyncSnapshot>(&pbTempAllocator);
    }
    downsyncSnapshotHolder->ParseFromArray(inBytes, inBytesCnt);
    return OnDownsyncSnapshotReceived(downsyncSnapshotHolder, outPostTimerRdfEvictedCnt, outPostTimerRdfDelayedIfdEvictedCnt);
}

bool FrontendBattle::OnDownsyncSnapshotReceived(const DownsyncSnapshot* downsyncSnapshot, int* outPostTimerRdfEvictedCnt, int* outPostTimerRdfDelayedIfdEvictedCnt) {
    /*
    Assuming that rdfBuffer & ifdBuffer are both sufficiently large (e.g. 5 seconds) such that when 
    - "timerRdfId" is to be evicted from "rdfBuffer.StFrameId", or
    - "ConvertToDelayedInputFrameId(timerRdfId)" is to be evicted from "ifdBuffer.StFrameId"
    , it's considered TOO LAGGY on frontend, thus reassigning "timerRdfId = chaserRdfIdLowerBound = refRdfId" will be allowed.

    Moreover if no such eviction is to occur, the "refRdf & ifdBatch" handlings are fully de-coupled.

    Edge cases
    - If there's no "refRdf" BUT "0 < postTimerRdfDelayedIfdToEvictCnt", drop the exceeding "ifd_batch".
    */

    bool shouldDragTimerRdfIdForward = false;
    *outPostTimerRdfEvictedCnt = 0;
    *outPostTimerRdfDelayedIfdEvictedCnt = 0;
    int refRdfId = downsyncSnapshot->ref_rdf_id();
    int oldChaserRdfIdLowerBound = chaserRdfIdLowerBound;
    if (downsyncSnapshot->has_ref_rdf()) {
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
            int lastUsedRdfId = BaseBattle::ConvertToLastUsedRenderFrameId(ifdId);
            if (lastUsedRdfId <= oldChaserRdfIdLowerBound) {
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
                        if (0 < (inactiveJoinMask & calcJoinIndexMask(k + 1))) {
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
            targetHolder->CopyFrom(refIfd);
            targetHolder->set_confirmed_list(allConfirmedMask);
            
            for (int k = 0; k < playersCnt; ++k) {
                if (ifdId > playerInputFrontIds[k]) {
                    playerInputFrontIds[k] = ifdId;
                    playerInputFronts[k] = refIfd.input_list(k);
                }
            }

            if (-1 == firstIncorrectlyPredictedIfdId && existingInputMutated) {
                firstIncorrectlyPredictedIfdId = ifdId;
            }
        
            lcacIfdId = ifdId;
        }
    }

    if (shouldDragTimerRdfIdForward || shouldDragTimerRdfUsingDelayedIfdIdForward) {
        timerRdfId = refRdfId;
    }

    if (-1 != firstIncorrectlyPredictedIfdId) {
        handleIncorrectlyRenderedPrediction(firstIncorrectlyPredictedIfdId, false);
    }

    return true;
}

bool FrontendBattle::OnUpsyncSnapshotReceived(char* inBytes, int inBytesCnt) {
    if (nullptr == upsyncSnapshotHolder) {
        upsyncSnapshotHolder = google::protobuf::Arena::Create<UpsyncSnapshot>(&pbTempAllocator);
    }
    upsyncSnapshotHolder->ParseFromArray(inBytes, inBytesCnt);
    return OnUpsyncSnapshotReceived(upsyncSnapshotHolder);
}

bool FrontendBattle::OnUpsyncSnapshotReceived(const UpsyncSnapshot* upsyncSnapshot) {
    // See "BackendBattle::OnUpsyncSnapshotReceived" for reference.
    bool fromUdp = true; // by design
    int peerJoinIndex = upsyncSnapshot->join_index();
    int delayedIfdId = BaseBattle::ConvertToDelayedInputFrameId(timerRdfId);
    int firstIncorrectlyPredictedIfdId = -1;
    int cmdListSize = upsyncSnapshot->cmd_list_size();
    for (int i = 0; i < cmdListSize; ++i) {
        int ifdId = upsyncSnapshot->st_ifd_id() + i;
        if (ifdId <= lcacIfdId) {
            // obsolete
            continue;
        }
        int lastUsedRdfId = BaseBattle::ConvertToLastUsedRenderFrameId(ifdId);
        if (lastUsedRdfId <= chaserRdfIdLowerBound) {
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

        const uint64_t cmd = upsyncSnapshot->cmd_list(i);
        bool outExistingInputMutated = false;
        InputFrameDownsync* ifd = getOrPrefabInputFrameDownsync(ifdId, peerJoinIndex, cmd, fromUdp, false, outExistingInputMutated);
        if (-1 == firstIncorrectlyPredictedIfdId && outExistingInputMutated) {
            firstIncorrectlyPredictedIfdId = ifdId;
        }
    }

    if (-1 != firstIncorrectlyPredictedIfdId) {
        handleIncorrectlyRenderedPrediction(firstIncorrectlyPredictedIfdId, fromUdp);
    }

    return true;
}

void FrontendBattle::Step(int fromRdfId, int toRdfId, bool isChasing) {
    if (!isChasing) {
        JPH_ASSERT(fromRdfId == timerRdfId && toRdfId == timerRdfId+1); // NOT supporting multi-step in this case.
        regulateCmdBeforeRender();
    }
    BaseBattle::Step(fromRdfId, toRdfId);
    if (isChasing) {
       chaserRdfId = toRdfId;
    } else {
       timerRdfId = toRdfId;
    }
}

void FrontendBattle::regulateCmdBeforeRender() {
    int delayedIfdId = BaseBattle::ConvertToDelayedInputFrameId(timerRdfId);
    if (delayedIfdId <= lcacIfdId) {
        return;
    }

    RenderFrame* currRdf = rdfBuffer.GetByFrameId(timerRdfId); 
    InputFrameDownsync* delayedIfd = ifdBuffer.GetByFrameId(delayedIfdId); // No need to prefab, it must exist
    for (int i = 0; i < playersCnt; i++) {
        int joinIndex = (i + 1); 
        if (joinIndex == selfJoinIndex) {
            continue;
        }
        uint64_t newVal = delayedIfd->input_list(i);
        uint64_t joinMask = (U64_1 << i);
        auto& playerChd = currRdf->players_arr(i);
        auto& chd = playerChd.chd();
        // [WARNING] Remove any predicted "rising edge" or a "falling edge" of any critical button before rendering.
        bool shouldPredictBtnAHold = (globalPrimitiveConsts->jammed_btn_holding_rdf_cnt() == chd.btn_a_holding_rdf_cnt()) || (0 < chd.btn_a_holding_rdf_cnt());
        bool shouldPredictBtnBHold = (globalPrimitiveConsts->jammed_btn_holding_rdf_cnt() == chd.btn_b_holding_rdf_cnt()) || (0 < chd.btn_b_holding_rdf_cnt());
        bool shouldPredictBtnCHold = (globalPrimitiveConsts->jammed_btn_holding_rdf_cnt() == chd.btn_c_holding_rdf_cnt()) || (0 < chd.btn_c_holding_rdf_cnt());
        bool shouldPredictBtnDHold = (globalPrimitiveConsts->jammed_btn_holding_rdf_cnt() == chd.btn_d_holding_rdf_cnt()) || (0 < chd.btn_d_holding_rdf_cnt());
        bool shouldPredictBtnEHold = (globalPrimitiveConsts->jammed_btn_holding_rdf_cnt() == chd.btn_e_holding_rdf_cnt()) || (0 < chd.btn_e_holding_rdf_cnt());
      
        if (0 < (delayedIfd->confirmed_list() & joinMask)) {
            // This regulation is only valid when "delayed input for this player is not yet confirmed"
        } else if (0 < (delayedIfd->udp_confirmed_list() & joinMask)) {
            // Received from UDP, better than local prediction though "InputFrameDownsync.confirmed_list()" is not set until confirmed by TCP path
        } else {
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
        }

        if (newVal != delayedIfd->input_list(i)) {
            delayedIfd->set_input_list(i, newVal);
            handleIncorrectlyRenderedPrediction(delayedIfdId, true);
        }
    }
}

void FrontendBattle::handleIncorrectlyRenderedPrediction(int mismatchedInputFrameId, bool fromUdp) {
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
    chaserRdfId = timerRdfId1;
}

bool FrontendBattle::ResetStartRdf(char* inBytes, int inBytesCnt, int inSelfJoinIndex) {
    WsReq* initializerMapData = google::protobuf::Arena::Create<WsReq>(&pbTempAllocator);
    initializerMapData->ParseFromArray(inBytes, inBytesCnt);
    return ResetStartRdf(initializerMapData, inSelfJoinIndex);
}

bool FrontendBattle::ResetStartRdf(const WsReq* initializerMapData, int inSelfJoinIndex) {
    chaserRdfId = globalPrimitiveConsts->terminating_render_frame_id();
    chaserRdfIdLowerBound = globalPrimitiveConsts->terminating_render_frame_id();
    lastSentIfdId = -1; 
    selfJoinIndex = inSelfJoinIndex;
    selfJoinIndexArrIdx = inSelfJoinIndex - 1;
    selfJoinIndexMask = (U64_1 << selfJoinIndexArrIdx);
    
    return BaseBattle::ResetStartRdf(initializerMapData);
}
