#pragma once

#include "BaseBattle.h"
#include <map>
#include <tuple>

using namespace JPH;

class JOLTC_EXPORT FrontendBattle : public BaseBattle {
public:
    inline FrontendBattle(const int aPlayersCnt, const int renderBufferSize, const int inputBufferSize, PhysicsSystem* aPhySys, JobSystemThreadPool* aJobSys) : BaseBattle(aPlayersCnt, renderBufferSize, inputBufferSize, aPhySys, aJobSys) {
    }

    virtual ~FrontendBattle() {
        // Calls base destructor (implicitly)
    }

public:
    int localExtraInputDelayFrames = 0;
    int chaserRdfId;
    int chaserRdfIdLowerBound;
    std::unordered_map<int, shared::RenderFrame> peerDownsyncedRdf;

    int selfJoinIndex = JOIN_INDEX_NOT_INITIALIZED;
    int selfJoinIndexArrIdx = JOIN_INDEX_ARR_IDX_NOT_INITIALIZED;
    uint64_t selfJoinIndexMask = 0u;
    uint64_t allConfirmedMask = 0u;

protected:
    void _handleIncorrectlyRenderedPrediction(int inputFrameId, bool fromUDP);
    void getOrPrefabInputFrameUpsync(int inputFrameId, bool canConfirmSelf, uint64_t inRealtimeSelfCmd, uint64_t* outSelfCmd, uint64_t* outPrevSelfCmd, uint64_t* outConfirmedList);
};
