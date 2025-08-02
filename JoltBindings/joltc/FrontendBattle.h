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
    uint32_t selfJoinIndex = globalPrimitiveConsts->magic_join_index_invalid();
    int selfJoinIndexInt = (int)selfJoinIndex;
    int selfJoinIndexArrIdx = selfJoinIndexInt-1;
    uint64_t selfJoinIndexMask = 0u;

    bool UpsertSelfCmd(uint64_t inSingleInput);

    bool ProduceUpsyncSnapshot(int proposedBatchIfdIdSt, int proposedBatchIfdIdEd, char* outBytesPreallocatedStart, long* outBytesCntLimit);

    bool OnUpsyncSnapshotReceived(char* inBytes, int inBytesCnt, int* outMaxPlayerInputFrontId, int* outMinPlayerInputFrontId);
    bool OnUpsyncSnapshotReceived(const UpsyncSnapshot* upsyncSnapshot, int* outMaxPlayerInputFrontId, int* outMinPlayerInputFrontId);

    bool OnDownsyncSnapshotReceived(char* inBytes, int inBytesCnt, int* outPostTimerRdfEvictedCnt, int* outPostTimerRdfDelayedIfdEvictedCnt, int* outNewLcacIfdId, int* outMaxPlayerInputFrontId, int* outMinPlayerInputFrontId);
    bool OnDownsyncSnapshotReceived(const DownsyncSnapshot* downsyncSnapshot, int* outPostTimerRdfEvictedCnt, int* outPostTimerRdfDelayedIfdEvictedCnt, int* outNewLcacIfdId, int* outMaxPlayerInputFrontId, int* outMinPlayerInputFrontId);

    void Step(int fromRdfId, int toRdfId, bool isChasing); // [WARNING] Implicitly calls "handleIncorrectlyRenderedPrediction" if needed

    inline void GetRdfAndIfdIds(int* outTimerRdfId, int* outChaserRdfId, int* outChaserRdfIdLowerBound, int* outLcacIfdId, int* outTimerRdfIdGenIfdId, int* outTimerRdfIdToUseIfdId) {
        *outTimerRdfId = timerRdfId;
        *outChaserRdfId = chaserRdfId;
        *outChaserRdfIdLowerBound = chaserRdfIdLowerBound;
        *outLcacIfdId = lcacIfdId;
        *outTimerRdfIdGenIfdId = BaseBattle::ConvertToGeneratingIfdId(timerRdfId);
        *outTimerRdfIdToUseIfdId = BaseBattle::ConvertToDelayedInputFrameId(timerRdfId);
    }

    bool ResetStartRdf(char* inBytes, int inBytesCnt, uint32_t inSelfJoinIndex);
    bool ResetStartRdf(const WsReq* initializerMapData, uint32_t inSelfJoinIndex);

protected:
    bool onlineArenaMode = false;

    void regulateCmdBeforeRender(); // [WARNING] Implicitly calls "handleIncorrectlyRenderedPrediction" if needed

    void handleIncorrectlyRenderedPrediction(int inputFrameId, bool fromUdp);

    DownsyncSnapshot* downsyncSnapshotHolder = nullptr;
    UpsyncSnapshot* upsyncSnapshotHolder = nullptr;
};

#endif
