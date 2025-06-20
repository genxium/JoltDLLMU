#include "FrontendBattle.h"
#include <string>

void FrontendBattle::_handleIncorrectlyRenderedPrediction(int mismatchedInputFrameId, bool fromUDP) {
    if (TERMINATING_INPUT_FRAME_ID == mismatchedInputFrameId) return;
    int timerRdfId1 = ConvertToFirstUsedRenderFrameId(mismatchedInputFrameId);
    if (timerRdfId1 >= chaserRdfId) return;
    // By now timerRdfId1 < chaserRdfId, it's pretty impossible that "timerRdfId1 > timerRdfId" but we're still checking.
    if (timerRdfId1 > timerRdfId) return; // The incorrect prediction is not yet rendered, no visual impact for player.
    int timerRdfId2 = ConvertToLastUsedRenderFrameId(mismatchedInputFrameId);
    if (timerRdfId2 < chaserRdfIdLowerBound) {
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

    // The actual rollback-and-chase would later be executed in "Update()". 
    chaserRdfId = timerRdfId1;
}

void FrontendBattle::getOrPrefabInputFrameUpsync(int inputFrameId, bool canConfirmSelf, uint64_t inRealtimeSelfCmd, uint64_t* outSelfCmd, uint64_t* outPrevSelfCmd, uint64_t* outConfirmedList) {
    if (JOIN_INDEX_NOT_INITIALIZED == selfJoinIndex) {
        throw std::runtime_error("getOrPrefabInputFrameUpsync called with 'JOIN_INDEX_NOT_INITIALIZED == selfJoinIndex' is invalid!");
    }

    auto existingInputFrame = ifdBuffer.GetByFrameId(inputFrameId);
    auto previousInputFrameDownsync = ifdBuffer.GetByFrameId(inputFrameId - 1);
    *outPrevSelfCmd = !previousInputFrameDownsync ? 0 : previousInputFrameDownsync->input_list(selfJoinIndexArrIdx);

    bool selfConfirmedInExistingInputFrame = existingInputFrame && (0 < (existingInputFrame->confirmed_list() & selfJoinIndexMask) || 0 < (existingInputFrame->udp_confirmed_list() & selfJoinIndexMask));
    if (selfConfirmedInExistingInputFrame) {
        /*
           [WARNING] 

           As shown in "https://github.com/genxium/DelayNoMoreUnity/blob/v1.6.5/frontend/Assets/Scripts/Abstract/AbstractMapController.cs#L1180", "timerRdfId" would NEVER be rewinded even under the most clumsy condition, i.e. "NON_CONSECUTIVE_SET == dumpRenderCacheRet" is carry-forth only (see "https://github.com/genxium/DelayNoMoreUnity/blob/v1.6.5/shared/FrameRingBuffer.cs#L80").

           The only possibility that "true == selfConfirmedInExistingInputFrame" is met here would be due to "putting `getOrPrefabInputFrameUpsync(..., canConfirmSelf=true, ...) > sendInputFrameUpsyncBatch(...)` before `lockstep`" by mistake -- in that case, "timerRdfId" is stuck at the same value thus we might be overwriting already confirmed input history for self (yet backend and other peers will certainly reject the overwrite!).
         */ 
        *outSelfCmd = existingInputFrame->input_list(selfJoinIndexArrIdx);
        *outConfirmedList = selfJoinIndexMask;
        return;
    }
    if (existingInputFrame && !canConfirmSelf) {
        // By reaching here, it's implied that "false == selfConfirmedInExistingInputFrame" 
        *outSelfCmd = existingInputFrame->input_list(selfJoinIndexArrIdx);
        *outConfirmedList = 0;
        return;
    }

    prefabbedInputList.assign(playersCnt, 0);
    for (int k = 0; k < playersCnt; ++k) {
        if (existingInputFrame) {
            // When "nullptr != existingInputFrame", it implies that "true == canConfirmSelf" here
            prefabbedInputList[k] = existingInputFrame->input_list(k);
        } else if (playerInputFrontIds[k] <= inputFrameId) {
            prefabbedInputList[k] = playerInputFronts[k];
        } else if (previousInputFrameDownsync) {
            // When "self.playerInputFrontIds[k] > inputFrameId", don't use it to predict a historical input!
            prefabbedInputList[k] = previousInputFrameDownsync->input_list(k);
        }

        /*
           [WARNING]

           All "critical input predictions (i.e. BtnA/B/C/D/E)" are now handled only in "UpdateInputFrameInPlaceUponDynamics", which is called just before rendering "playerRdf" -- the only timing that matters for smooth graphcis perception of (human) players.
         */
    }

    // [WARNING] Do not blindly use "selfJoinIndexMask" here, as the "actuallyUsedInput for self" couldn't be confirmed while prefabbing, otherwise we'd have confirmed a wrong self input by "_markConfirmationIfApplicable()"!
    prefabbedInputList[selfJoinIndexArrIdx] = inRealtimeSelfCmd;

    uint64_t initConfirmedList = 0;
    if (canConfirmSelf) {
        bool shouldSetConfirmedMask = (*outPrevSelfCmd != inRealtimeSelfCmd || 15u < inRealtimeSelfCmd);
        if (!shouldSetConfirmedMask) {
            bool isLastRdfInIfdCoverage = (ConvertToDynamicallyGeneratedDelayInputFrameId(timerRdfId+1, localExtraInputDelayFrames) == (inputFrameId+1));
            shouldSetConfirmedMask = isLastRdfInIfdCoverage; 
        }
        if (shouldSetConfirmedMask) {
            initConfirmedList = selfJoinIndexMask;
            if (nullptr != existingInputFrame) {
                initConfirmedList |= existingInputFrame->confirmed_list();
            }
        }
    }

    if (nullptr != existingInputFrame) {
        auto existingSelfInputAtSameIfd = existingInputFrame->input_list(selfJoinIndexArrIdx);
        existingInputFrame->set_input_list(selfJoinIndexArrIdx, inRealtimeSelfCmd);
        existingInputFrame->set_udp_confirmed_list(existingInputFrame->udp_confirmed_list() | initConfirmedList);
        if (existingSelfInputAtSameIfd != inRealtimeSelfCmd) {
            _handleIncorrectlyRenderedPrediction(inputFrameId, false);
        }
    } else {
        while (ifdBuffer.EdFrameId <= inputFrameId) {
            // Fill the gap
            int gapInputFrameId = ifdBuffer.EdFrameId;
            auto ifdHolder = ifdBuffer.DryPut();
            if (nullptr == ifdHolder) {
                std::string builder;
                throw std::runtime_error("ifdBuffer was not fully pre-allocated for gapInputFrameId");
            }
            ifdHolder->set_confirmed_list(0u); // [WARNING] Important to provision first to avoid RingBuff reuse contamination!
            ifdHolder->set_input_frame_id(gapInputFrameId);
            for (int k = 0; k < playersCnt; ++k) {
                ifdHolder->set_input_list(k, prefabbedInputList[k]);
            }
            ifdHolder->set_udp_confirmed_list(initConfirmedList);
        }
    }

    *outConfirmedList = initConfirmedList;
}
