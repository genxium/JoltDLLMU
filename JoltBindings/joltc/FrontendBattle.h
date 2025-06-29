#pragma once

#include "BaseBattle.h"
#include <map>

using namespace JPH;
using namespace jtshared;

class JOLTC_EXPORT FrontendBattle : public BaseBattle {
public:
    FrontendBattle(char* inBytes, int inBytesCnt, int renderBufferSize, int inputBufferSize, TempAllocator* inGlobalTempAllocator, bool isOnlineArenaMode) : BaseBattle(inBytes, inBytesCnt, renderBufferSize, inputBufferSize, inGlobalTempAllocator) {
        onlineArenaMode = isOnlineArenaMode;
    }

    virtual ~FrontendBattle() {
        // Calls base destructor (implicitly)
    }

public:
    int localExtraInputDelayFrames = 0;
    int chaserRdfId;
    int chaserRdfIdLowerBound;
    std::unordered_map<int, RenderFrame> peerDownsyncedRdf;

    int selfJoinIndex = globalPrimitiveConsts->magic_join_index_invalid();
    int selfJoinIndexArrIdx = globalPrimitiveConsts->magic_join_index_invalid()-1;
    uint64_t selfJoinIndexMask = 0u;

    void HandleIncorrectlyRenderedPrediction(int inputFrameId, bool fromUDP);
    void OnWsRespReceived(char* inBytes, int inBytesCnt);

protected:
    bool onlineArenaMode = false;
    virtual bool preprocessIfdStEviction(int inputFrameId);
    virtual void postprocessIfdStEviction();
};
