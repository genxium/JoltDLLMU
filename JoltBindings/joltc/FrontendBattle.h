#ifndef FRONTEND_BATTLE_H_
#define FRONTEND_BATTLE_H_ 1

#include "BaseBattle.h"
#include <map>
#ifndef NDEBUG
#include "DebugLog.h"
#endif

using namespace JPH;
using namespace jtshared;

class JOLTC_EXPORT FrontendBattle : public BaseBattle {
public:
    FrontendBattle(int renderBufferSize, int inputBufferSize, TempAllocator* inGlobalTempAllocator, bool isOnlineArenaMode) : BaseBattle(renderBufferSize, inputBufferSize, inGlobalTempAllocator) {
        onlineArenaMode = isOnlineArenaMode;
    }

    virtual ~FrontendBattle() {
        // Calls base destructor (implicitly)
#ifndef NDEBUG
        Debug::Log("~FrontendBattle/C++", DColor::Green);
#endif
    }

public:
    int localExtraInputDelayFrames = 0;
    int chaserRdfId = globalPrimitiveConsts->terminating_render_frame_id();
    int chaserRdfIdLowerBound = globalPrimitiveConsts->terminating_render_frame_id();
    int lastSentIfdId = -1;
    int selfJoinIndex = globalPrimitiveConsts->magic_join_index_invalid();
    int selfJoinIndexArrIdx = globalPrimitiveConsts->magic_join_index_invalid()-1;
    uint64_t selfJoinIndexMask = 0u;

    bool UpsertSelfCmd(uint64_t inSingleInput);

    UpsyncSnapshot* ProduceUpsyncSnapshot();

    bool OnUpsyncSnapshotReceived(char* inBytes, int inBytesCnt);
    bool OnUpsyncSnapshotReceived(const UpsyncSnapshot* upsyncSnapshot);

    bool OnDownsyncSnapshotReceived(char* inBytes, int inBytesCnt, int* outPostTimerRdfEvictedCnt, int* outPostTimerRdfDelayedIfdEvictedCnt);
    bool OnDownsyncSnapshotReceived(const DownsyncSnapshot* downsyncSnapshot, int* outPostTimerRdfEvictedCnt, int* outPostTimerRdfDelayedIfdEvictedCnt);

    void Step(int fromRdfId, int toRdfId, bool isChasing); // [WARNING] Implicitly calls "handleIncorrectlyRenderedPrediction" if needed

    bool ResetStartRdf(char* inBytes, int inBytesCnt, int inSelfJoinIndex);
    bool ResetStartRdf(const WsReq* initializerMapData, int inSelfJoinIndex);

protected:
    bool onlineArenaMode = false;

    void regulateCmdBeforeRender(); // [WARNING] Implicitly calls "handleIncorrectlyRenderedPrediction" if needed

    void handleIncorrectlyRenderedPrediction(int inputFrameId, bool fromUdp);

    DownsyncSnapshot* downsyncSnapshotHolder = nullptr;
    UpsyncSnapshot* upsyncSnapshotHolder = nullptr;
};

#endif
