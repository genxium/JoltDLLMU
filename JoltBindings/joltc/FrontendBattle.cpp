#include "FrontendBattle.h"
#include <string>

void FrontendBattle::RegulateCmdBeforeRender() {
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
            // This in-place update is only valid when "delayed input for this player is not yet confirmed"
        } else if (playerInputFrontIds[i] == delayedIfdId) {
            // Exactly received from UDP, better than local prediction though "inputFrameDownsync.ConfirmedList" is not set till confirmed by TCP path
            newVal = playerInputFronts[i];
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
            HandleIncorrectlyRenderedPrediction(delayedIfdId, true);
        }
    }
}

void FrontendBattle::HandleIncorrectlyRenderedPrediction(int mismatchedInputFrameId, bool fromUdp) {
    if (globalPrimitiveConsts->terminating_input_frame_id() == mismatchedInputFrameId) return;
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

bool FrontendBattle::OnDownsyncSnapshotReceived(char* inBytes, int inBytesCnt) {
    /*
    For "non-null refRdf", just call "rdfBuffer.DryPut" till "rdfBuffer.EdFrameId > pbRdf.id()" -- if "timerRdfId" is to be evicted we have no choice but reassign "timerRdfId++" along eviction (no need to consider lastAllConfirmed as a bound for later chasing in this case, because the "pbRdf.id()" will be the new "chaserRdfIdLowerBound"). 
    
    Make "rdfBuffer.N" large enough such that the aforementioned "eviction-induced timerRdfId++" doesn't happen too often.
    */
    return false;
}

bool FrontendBattle::OnUpsyncSnapshotReceived(char* inBytes, int inBytesCnt, bool fromUdp, bool fromTcp) {
    UpsyncSnapshot* upsyncSnapshot = google::protobuf::Arena::Create<UpsyncSnapshot>(&pbTempAllocator);
    upsyncSnapshot->ParseFromArray(inBytes, inBytesCnt);
    int peerJoinIndex = upsyncSnapshot->join_index();

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
        const uint64_t cmd = upsyncSnapshot->cmd_list(i);
        bool outExistingInputMutated = false;
        InputFrameDownsync* ifd = GetOrPrefabInputFrameDownsync(ifdId, peerJoinIndex, cmd, fromUdp, fromTcp, outExistingInputMutated);
        if (-1 == firstIncorrectlyPredictedIfdId && outExistingInputMutated) {
            firstIncorrectlyPredictedIfdId = ifdId;
            HandleIncorrectlyRenderedPrediction(ifdId, fromUdp);
        }
    }

    return true;
}

void FrontendBattle::Step(int fromRdfId, int toRdfId, bool isChasing) {
    BaseBattle::Step(fromRdfId, toRdfId);
    if (isChasing) {
       chaserRdfId = toRdfId;
    } else {
       timerRdfId = toRdfId;
    }
}
