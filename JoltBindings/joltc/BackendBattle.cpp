#include "BackendBattle.h"

bool BackendBattle::preprocessIfdStEviction(int inputFrameId) {
    int toEvictCnt = (inputFrameId - ifdBuffer.EdFrameId + 1) - (ifdBuffer.N - ifdBuffer.Cnt);
    int currDyanmicsRdfIdToUseIfdId = ConvertToDelayedInputFrameId(currDynamicsRdfId);
    int toEvictIfdIdUpperBound = (currDyanmicsRdfIdToUseIfdId < lastConsecutivelyAllConfirmedIfdId ? currDyanmicsRdfIdToUseIfdId : lastConsecutivelyAllConfirmedIfdId);
    if (toEvictIfdIdUpperBound < ifdBuffer.StFrameId + toEvictCnt) {
        /*
        When frontend freezing is carefully implemented, there should be no backend ifdBuffer eviction, see [comments in DLLMU-v2.3.4](https://github.com/genxium/DelayNoMoreUnity/blob/v2.3.4/frontend/Assets/Scripts/OnlineMapController.cs#L52). 
        */
        return false;
    }
    return true;
}

void BackendBattle::postprocessIfdStEviction() {
    int oldLastConsecutivelyAllConfirmedIfdId  = lastConsecutivelyAllConfirmedIfdId;
    // Try moving forward "lastConsecutivelyAllConfirmedIfdId" to favor the next call of "preprocessIfdStEviction(...)"
    int incCnt = moveForwardLastConsecutivelyAllConfirmedIfdId(ifdBuffer.EdFrameId, inactiveJoinMask);
    if (0 >= incCnt) return;
    if (0 > lastConsecutivelyAllConfirmedIfdId) return; 
    int nextDynamicsRdfId = (0 <= lastConsecutivelyAllConfirmedIfdId ? ConvertToLastUsedRenderFrameId(lastConsecutivelyAllConfirmedIfdId) + 1 : -1);
    if (0 >= nextDynamicsRdfId || nextDynamicsRdfId <= currDynamicsRdfId) return;
    // Try moving forward "currDynamicsRdfId" to favor the next call of "preprocessIfdStEviction(...)"
    Step(currDynamicsRdfId, nextDynamicsRdfId, globalTempAllocator);
}
