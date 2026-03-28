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
    FrontendBattle(int renderBufferSize, int inputBufferSize, TempAllocator* inGlobalTempAllocator, bool isOnlineArenaMode) : BaseBattle(renderBufferSize, inputBufferSize, inGlobalTempAllocator, FrontendBattle::ArenaAllocStepResult) {
        timerRdfId = globalPrimitiveConsts->starting_input_frame_id();
        onlineArenaMode = isOnlineArenaMode;

        downsyncSnapshotHolder = google::protobuf::Arena::Create<DownsyncSnapshot>(&pbSemiPermAllocator);
        peerUpsyncSnapshotHolder = google::protobuf::Arena::Create<WsReq>(&pbSemiPermAllocator);
        selfUpsyncReqHolder = google::protobuf::Arena::Create<WsReq>(&pbSemiPermAllocator);
        JPH_ASSERT(nullptr != downsyncSnapshotHolder->GetArena()); // [WARNING] Otherwise too inefficient in memory usage, e.g. when calling "unsafe_arena_set_allocated_xxx" would "delete" existing field first
        JPH_ASSERT(nullptr != peerUpsyncSnapshotHolder->GetArena()); // [WARNING] Otherwise too inefficient in memory usage, e.g. when calling "unsafe_arena_set_allocated_xxx" would "delete" existing field first
        JPH_ASSERT(nullptr != selfUpsyncReqHolder->GetArena()); // [WARNING] Otherwise too inefficient in memory usage, e.g. when calling "unsafe_arena_set_allocated_xxx" would "delete" existing field first

        allocPhySys();
        jobSys = new JobSystemThreadPool(cMaxPhysicsJobs, cMaxPhysicsBarriers, thread::hardware_concurrency() - 1);
    }

    virtual ~FrontendBattle() {
        // Calls base destructor (implicitly)
        if (nullptr != downsyncSnapshotHolder) {
            downsyncSnapshotHolder = nullptr;
        }
        if (nullptr != peerUpsyncSnapshotHolder) {
            peerUpsyncSnapshotHolder = nullptr;
        }
        if (nullptr != selfUpsyncReqHolder) {
            selfUpsyncReqHolder = nullptr;
        }
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
    int udpLcacIfdId = -1; // ALWAYS maintained "udpLcacIfdId >= lcacIfdId"
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

    bool OnUpsyncSnapshotReqReceived(char* inBytes, int inBytesCnt, int* outChaserRdfId, int* outUdpLcacIfdId, int* outMaxPlayerInputFrontId, int* outMinPlayerInputFrontId);
    bool OnUpsyncSnapshotReceived(const uint32_t peerJoinIndex, const UpsyncSnapshot& upsyncSnapshot, int* outChaserRdfId, int* outUdpLcacIfdId, int* outMaxPlayerInputFrontId, int* outMinPlayerInputFrontId);

    bool OnDownsyncSnapshotReceived(char* inBytes, int inBytesCnt, int* outPostTimerRdfEvictedCnt, int* outPostTimerRdfDelayedIfdEvictedCnt, int* outChaserRdfId, int* outLcacIfdId, int* outUdpLcacIfdId, int* outMaxPlayerInputFrontId, int* outMinPlayerInputFrontId);
    bool OnDownsyncSnapshotReceived(const DownsyncSnapshot* downsyncSnapshot, int* outPostTimerRdfEvictedCnt, int* outPostTimerRdfDelayedIfdEvictedCnt, int* outChaserRdfId, int* outLcacIfdId, int* outUdpLcacIfdId, int* outMaxPlayerInputFrontId, int* outMinPlayerInputFrontId);

    bool WriteSingleStepFrameLog(int currRdfId, RenderFrame* nextRdf, int fromRdfId, int toRdfId, int delayedIfdId, InputFrameDownsync* delayedIfd, bool isChasing, bool snatched=false);
    bool Step(); // [WARNING] Implicitly calls "handleIncorrectlyRenderedPrediction" if needed
    bool ChaseRolledBackRdfs(int* outNewChaserRdfId, bool toTimerRdfId = false);

    inline bool GetRdfAndIfdIds(int* outTimerRdfId, int* outChaserRdfId, int* outChaserRdfIdLowerBound, int* outLcacIfdId, int* outUdpLcacIfdId, int* outTimerRdfIdGenIfdId, int* outTimerRdfIdToUseIfdId) {
        *outTimerRdfId = timerRdfId;
        *outChaserRdfId = chaserRdfId;
        *outChaserRdfIdLowerBound = chaserRdfIdLowerBound;
        *outLcacIfdId = lcacIfdId;
        *outUdpLcacIfdId = udpLcacIfdId;
        *outTimerRdfIdGenIfdId = BaseBattle::ConvertToGeneratingIfdId(timerRdfId);
        *outTimerRdfIdToUseIfdId = BaseBattle::ConvertToDelayedInputFrameId(timerRdfId);
        return true;
    }

    virtual void Clear();

    bool ResetStartRdf(char* inBytes, int inBytesCnt, const uint32_t inSelfJoinIndex, const char * const inSelfPlayerId, const int inSelfCmdAuthKey);
    bool ResetStartRdf(WsReq* initializerMapData, const uint32_t inSelfJoinIndex, const char * const inSelfPlayerId, const int inSelfCmdAuthKey);

    static inline StepResult* ArenaAllocStepResult(google::protobuf::Arena* theAllocator) {
        auto* stepResult = google::protobuf::Arena::Create<StepResult>(theAllocator);
        // Preallocate aiming rays 
        while (stepResult->aiming_rays_size() < globalPrimitiveConsts->default_prealloc_bullet_capacity()) {
            stepResult->add_aiming_rays();
        }
        return stepResult;
    }

protected:
    bool onlineArenaMode = false;

    void regulateCmdBeforeRender(const int currRdfId, const int delayedIfdId, InputFrameDownsync* delayedIfd); // [WARNING] Implicitly calls "handleIncorrectlyRenderedPrediction" if needed

    void handleIncorrectlyRenderedPrediction(int inputFrameId, bool fromSelf, bool fromUdp, bool fromRegulateBeforeRender);

    int moveForwardUdpLastConsecutivelyAllConfirmedIfdId(int proposedIfdEdFrameId, uint64_t skippableJoinMask = 0);

    virtual void postStepSingleChdStateCorrection(const int steppingRdfId, const uint64_t udt, const uint64_t ud, const CH_COLLIDER_T* chCollider, const CharacterDownsync& currChd, CharacterDownsync* nextChd, const CharacterConfig* cc, bool cvSupported, bool cvInAir, bool cvOnWall, bool currNotDashing, bool currEffInAir, bool oldNextNotDashing, bool oldNextEffInAir, bool inJumpStartupOrJustEnded, CharacterBase::EGroundState cvGroundState, const InputInducedMotion* inputInducedMotion, StepResult* stepResult);

    DownsyncSnapshot* downsyncSnapshotHolder = nullptr;
    WsReq* peerUpsyncSnapshotHolder = nullptr;
    WsReq* selfUpsyncReqHolder = nullptr;

    virtual bool allocPhySys() override {
        if (nullptr != phySys) return false;
        phySys = new PhysicsSystem();
        phySys->Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, bpLayerInterface, ovbLayerFilter, ovoLayerFilter);
        phySys->SetBodyActivationListener(&bodyActivationListener);
        phySys->SetContactListener(&contactListener);
        phySys->SetGravity(Vec3(0, globalPrimitiveConsts->gravity_y(), 0));
        phySys->SetContactListener(this);

        /*
        - When "PhysicsSettings.mUseBodyPairContactCache == true", Jolt will use caches extensively and might impact some "traversal-order-sensitive floating number calculation" during "rollback-chasing".
            - ["CachedBodyPair"](https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Physics/Constraints/ContactConstraintManager.cpp#L823) of ["ManifoldCache"](https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Physics/Constraints/ContactConstraintManager.h#L416)
        - When "PhysicsSettings.mDeterministicSimulation == true", the engine sorts "regular Constraints" and "ContactConstraints" before solving them](https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Physics/PhysicsSystem.cpp#L1415).
        - When "PhysicsSettings.mConstraintWarmStart == true", Jolt uses "warm start" to solve constraints (https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Physics/Constraints/ContactConstraintManager.cpp), which will certainly impact determinism in "rollback-chasing".
            - [One of the calls to ContactConstraintManager::WarmStartVelocityConstraints](https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Physics/PhysicsSystem.cpp#L1348)
        - Moreover, the impact of "PhysicsSettings.mUseManifoldReduction" precedes the solver of "ContactConstraintManager".
       */
        PhysicsSettings clonedPhySettings = phySys->GetPhysicsSettings();
        clonedPhySettings.mUseManifoldReduction = true;

        if (onlineArenaMode) {
            clonedPhySettings.mConstraintWarmStart = false; // [WARNING] A practical test case to verify that this setting matters is "FrontendTest/runTestCase11".
            clonedPhySettings.mUseBodyPairContactCache = false; // [WARNING] By keeping "mConstraintWarmStart = false", a practical test case to verify that this setting matters is "FrontendTest/runTestCase11" with "cLengthEps <= 1e-10" (i.e. if "cLengthEps >= 1e-2" this setting doesn't matter).
        } else {
            clonedPhySettings.mConstraintWarmStart = true;
            clonedPhySettings.mUseBodyPairContactCache = true;
        }
      
        phySys->SetPhysicsSettings(clonedPhySettings);
        antiGravityNorm = (-1.0f * phySys->GetGravity()).Normalized();
        gravityMagnitude = phySys->GetGravity().Length();

        bi = &(phySys->GetBodyInterface());
        biNoLock = &(phySys->GetBodyInterfaceNoLock());
        narrowPhaseQueryNoLock = &(phySys->GetNarrowPhaseQueryNoLock());

        return true;
    }
};

#endif
