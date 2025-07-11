#pragma once

#include "BaseBattle.h"
#include <map>

using namespace JPH;
using namespace jtshared;

class JOLTC_EXPORT FrontendBattle : public BaseBattle {
public:
    FrontendBattle(char* inBytes, int inBytesCnt, int renderBufferSize, int inputBufferSize, TempAllocator* inGlobalTempAllocator, bool isOnlineArenaMode, int inSelfJoinIndex) : BaseBattle(inBytes, inBytesCnt, renderBufferSize, inputBufferSize, inGlobalTempAllocator) {
        onlineArenaMode = isOnlineArenaMode;

        selfJoinIndex = inSelfJoinIndex;
        selfJoinIndexArrIdx = inSelfJoinIndex - 1;
        selfJoinIndexMask = (U64_1 << selfJoinIndexArrIdx);
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

    void RegulateCmdBeforeRender(); // [WARNING] Implicitly calls "HandleIncorrectlyRenderedPrediction" if needed
    void HandleIncorrectlyRenderedPrediction(int inputFrameId, bool fromUdp);
    bool OnDownsyncSnapshotReceived(char* inBytes, int inBytesCnt);
    bool OnUpsyncSnapshotReceived(char* inBytes, int inBytesCnt, bool fromUdp, bool fromTcp);

    void Step(int fromRdfId, int toRdfId, bool isChasing);

protected:
    bool onlineArenaMode = false;
};
