#pragma once

#include "BaseBattle.h"
#include <map>

using namespace JPH;
using namespace jtshared;

class JOLTC_EXPORT FrontendBattle : public BaseBattle {
public:
    FrontendBattle(char* inBytes, int inBytesCnt, int renderBufferSize, int inputBufferSize) : BaseBattle(inBytes, inBytesCnt, renderBufferSize, inputBufferSize) {
    }

    virtual ~FrontendBattle() {
        // Calls base destructor (implicitly)
    }

public:
    int localExtraInputDelayFrames = 0;
    int chaserRdfId;
    int chaserRdfIdLowerBound;
    std::unordered_map<int, RenderFrame> peerDownsyncedRdf;

    int selfJoinIndex = JOIN_INDEX_NOT_INITIALIZED;
    int selfJoinIndexArrIdx = JOIN_INDEX_ARR_IDX_NOT_INITIALIZED;
    uint64_t selfJoinIndexMask = 0u;
    uint64_t allConfirmedMask = 0u;

protected:
    void _handleIncorrectlyRenderedPrediction(int inputFrameId, bool fromUDP);
    void getOrPrefabInputFrameUpsync(int inputFrameId, bool canConfirmSelf, uint64_t inRealtimeSelfCmd, uint64_t* outSelfCmd, uint64_t* outPrevSelfCmd, uint64_t* outConfirmedList);
};
