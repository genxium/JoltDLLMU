#include "FrontendBattle.h"
#include <string>

void FrontendBattle::HandleIncorrectlyRenderedPrediction(int mismatchedInputFrameId, bool fromUDP) {
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

void FrontendBattle::OnWsRespReceived(char* inBytes, int inBytesCnt) {
    /*
    For downsynced "RenderFrame pbRdf", just call "rdfBuffer.DryPut" till "rdfBuffer.EdFrameId > pbRdf.id()" -- if "timerRdfId" is to be evicted we have no choice but reassign "timerRdfId++" along eviction (no need to consider lastAllConfirmed as a bound for later chasing in this case, because the "pbRdf.id()" will be the new "chaserRdfIdLowerBound"). 
    
    Make "rdfBuffer.N" large enough such that the aforementioned "eviction-induced timerRdfId++" doesn't happen too often.
    */
}

bool FrontendBattle::preprocessIfdStEviction(int inputFrameId) {
    if (!onlineArenaMode) {
        return true;
    }
    int toEvictCnt = (inputFrameId - ifdBuffer.EdFrameId + 1) - (ifdBuffer.N - ifdBuffer.Cnt);
    int timerRdfIdToUseIfdId = ConvertToDelayedInputFrameId(timerRdfId);
    int toEvictIfdIdUpperBound = (timerRdfIdToUseIfdId < lastConsecutivelyAllConfirmedIfdId ? timerRdfIdToUseIfdId : lastConsecutivelyAllConfirmedIfdId);
    if (toEvictIfdIdUpperBound < 0) {
        toEvictIfdIdUpperBound = 0;
    }
    if (toEvictIfdIdUpperBound < ifdBuffer.StFrameId + toEvictCnt) {
        return false;
    }
    return true;
}

void FrontendBattle::postprocessIfdStEviction() {
}
