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
        timerRdfId = globalPrimitiveConsts->starting_input_frame_id();
        onlineArenaMode = isOnlineArenaMode;
    }

    virtual ~FrontendBattle() {
        // Calls base destructor (implicitly)
#ifndef NDEBUG
        Debug::Log("~FrontendBattle/C++", DColor::Green);
#endif
    }

public:
    /*
    At any point of time it's maintained that "timerRdfId >= rdfBuffer.StFrameId", for obvious reason. 

    Backend also has a "backendTimerRdfId" maintained on C# side, which is NOT coupled with "Step(...)" or "UpsertSelfCmd(...)".
    */
    int timerRdfId;
    int localExtraInputDelayFrames = 0;
    int chaserRdfId = globalPrimitiveConsts->terminating_render_frame_id();
    int chaserRdfIdLowerBound = globalPrimitiveConsts->terminating_render_frame_id();
    uint32_t selfJoinIndex = globalPrimitiveConsts->magic_join_index_invalid();
    int selfJoinIndexInt = (int)selfJoinIndex;
    int selfJoinIndexArrIdx = selfJoinIndexInt-1;
    uint64_t selfJoinIndexMask = 0u;
    const char* selfPlayerId = nullptr;
    int selfCmdAuthKey = 0;

    bool UpsertSelfCmd(uint64_t inSingleInput, int* outChaserRdfId);

    bool ProduceUpsyncSnapshotRequest(int seqNo, int proposedBatchIfdIdSt, int proposedBatchIfdIdEd, int* outLastIfdId, char* outBytesPreallocatedStart, long* outBytesCntLimit);

    bool OnUpsyncSnapshotReqReceived(char* inBytes, int inBytesCnt, int* outChaserRdfId, int* outMaxPlayerInputFrontId, int* outMinPlayerInputFrontId);
    bool OnUpsyncSnapshotReceived(const uint32_t peerJoinIndex, const UpsyncSnapshot& upsyncSnapshot, int* outChaserRdfId, int* outMaxPlayerInputFrontId, int* outMinPlayerInputFrontId);

    bool OnDownsyncSnapshotReceived(char* inBytes, int inBytesCnt, int* outPostTimerRdfEvictedCnt, int* outPostTimerRdfDelayedIfdEvictedCnt, int* outChaserRdfId, int* outLcacIfdId, int* outMaxPlayerInputFrontId, int* outMinPlayerInputFrontId);
    bool OnDownsyncSnapshotReceived(const DownsyncSnapshot* downsyncSnapshot, int* outPostTimerRdfEvictedCnt, int* outPostTimerRdfDelayedIfdEvictedCnt, int* outChaserRdfId, int* outLcacIfdId, int* outMaxPlayerInputFrontId, int* outMinPlayerInputFrontId);

    bool WriteSingleStepFrameLog(int currRdfId, RenderFrame* nextRdf, int fromRdfId, int toRdfId, int delayedIfdId, InputFrameDownsync* delayedIfd, bool isChasing);
    bool Step(); // [WARNING] Implicitly calls "handleIncorrectlyRenderedPrediction" if needed
    bool ChaseRolledBackRdfs(int* outNewChaserRdfId, bool toTimerRdfId = false);

    inline bool GetRdfAndIfdIds(int* outTimerRdfId, int* outChaserRdfId, int* outChaserRdfIdLowerBound, int* outLcacIfdId, int* outTimerRdfIdGenIfdId, int* outTimerRdfIdToUseIfdId) {
        *outTimerRdfId = timerRdfId;
        *outChaserRdfId = chaserRdfId;
        *outChaserRdfIdLowerBound = chaserRdfIdLowerBound;
        *outLcacIfdId = lcacIfdId;
        *outTimerRdfIdGenIfdId = BaseBattle::ConvertToGeneratingIfdId(timerRdfId);
        *outTimerRdfIdToUseIfdId = BaseBattle::ConvertToDelayedInputFrameId(timerRdfId);
        return true;
    }

    bool ResetStartRdf(char* inBytes, int inBytesCnt, const uint32_t inSelfJoinIndex, const char * const inSelfPlayerId, const int inSelfCmdAuthKey);
    bool ResetStartRdf(const WsReq* initializerMapData, const uint32_t inSelfJoinIndex, const char * const inSelfPlayerId, const int inSelfCmdAuthKey);

protected:
    bool onlineArenaMode = false;

    void regulateCmdBeforeRender(); // [WARNING] Implicitly calls "handleIncorrectlyRenderedPrediction" if needed

    void handleIncorrectlyRenderedPrediction(int inputFrameId, bool fromSelf, bool fromUdp, bool fromRegulateBeforeRender);

    virtual void postStepSingleChdStateCorrection(const int steppingRdfId, const uint64_t udt, const uint64_t ud, const CH_COLLIDER_T* chCollider, const CharacterDownsync& currChd, CharacterDownsync* nextChd, const CharacterConfig* cc, bool cvSupported, bool cvInAir, bool cvOnWall, bool currNotDashing, bool currEffInAir, bool oldNextNotDashing, bool oldNextEffInAir, bool inJumpStartupOrJustEnded, CharacterBase::EGroundState cvGroundState);

    DownsyncSnapshot* downsyncSnapshotHolder = nullptr;
    WsReq* upsyncSnapshotReqHolder = nullptr;
};

#endif
