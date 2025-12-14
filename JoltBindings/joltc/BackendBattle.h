#ifndef BACKEND_BATTLE_H_
#define BACKEND_BATTLE_H_ 1

#include "BaseBattle.h"
#ifndef NDEBUG
#include "DebugLog.h"
#endif

using namespace JPH;

class JOLTC_EXPORT BackendBattle : public BaseBattle {
public:
    BackendBattle(int renderBufferSize, int inputBufferSize, TempAllocator* inGlobalTempAllocator) : BaseBattle(renderBufferSize, inputBufferSize, inGlobalTempAllocator)  {
        downsyncSnapshotHolder = new DownsyncSnapshot();
        wsReqHolder = new WsReq();

        allocPhySys();
        jobSys = new JobSystemThreadPool(cMaxPhysicsJobs, cMaxPhysicsBarriers, thread::hardware_concurrency() - 1);
    }

    virtual ~BackendBattle() {
        // Calls base destructor (implicitly)
        if (nullptr != downsyncSnapshotHolder) {
            delete downsyncSnapshotHolder;
            downsyncSnapshotHolder = nullptr;
        }
        if (nullptr != wsReqHolder) {
            delete wsReqHolder;
            wsReqHolder = nullptr;
        }
#ifndef NDEBUG
        Debug::Log("~BackendBattle/C++", DColor::Green);
#endif
    }
public:
    int elongatedBattleDurationFrames;
    bool elongatedBattleDurationFramesShortenedOnce;
    int dynamicsRdfId;
    int lastForceResyncedRdfId;
    int nstDelayFrames;

public:
    bool OnUpsyncSnapshotReqReceived(char* inBytes, int inBytesCnt, bool fromUdp, bool fromTcp, char* outBytesPreallocatedStart, long* outBytesCntLimit, int* outForceConfirmedStEvictedCnt, int* outOldLcacIfdId, int* outNewLcacIfdId, int* outOldDynamicsRdfId, int* outNewDynamicsRdfId, int* outMaxPlayerInputFrontId, int* outMinPlayerInputFrontId);

    bool OnUpsyncSnapshotReceived(const uint32_t peerJoinIndex, const UpsyncSnapshot& upsyncSnapshot, bool fromUdp, bool fromTcp, char* outBytesPreallocatedStart, long* outBytesCntLimit, int* outForceConfirmedStEvictedCnt, int* outOldLcacIfdId, int* outNewLcacIfdId, int* outOldDynamicsRdfId, int* outNewDynamicsRdfId, int* outMaxPlayerInputFrontId, int* outMinPlayerInputFrontId);

    bool WriteSingleStepFrameLog(int currRdfId, RenderFrame* nextRdf, int delayedIfdId, InputFrameDownsync* delayedIfd);
    int Step(int fromRdfId, int toRdfId, DownsyncSnapshot* virtualIfds = nullptr);
    int MoveForwardLcacIfdIdAndStep(bool withRefRdf, int* outOldLcacIfdId, int* outNewLcacIfdId, int* outOldDynamicsRdfId, int* outNewDynamicsRdfId, char* outBytesPreallocatedStart, long* outBytesCntLimit);

    virtual bool ResetStartRdf(char* inBytes, int inBytesCnt);
    virtual bool ResetStartRdf(WsReq* initializerMapData);

    int GetDynamicsRdfId();

protected:
    void produceDownsyncSnapshot(uint64_t unconfirmedMask, int stIfdId, int edIfdId, bool withRefRdf, DownsyncSnapshot** outResult);
    void releaseDownsyncSnapshotArenaOwnership(DownsyncSnapshot* downsyncSnapshot);
    DownsyncSnapshot* downsyncSnapshotHolder = nullptr;
    WsReq* wsReqHolder = nullptr;

    virtual bool allocPhySys() override {
        if (nullptr != phySys) return false;
        phySys = new PhysicsSystem();
        phySys->Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, bpLayerInterface, ovbLayerFilter, ovoLayerFilter);
        phySys->SetBodyActivationListener(&bodyActivationListener);
        phySys->SetContactListener(&contactListener);
        phySys->SetGravity(Vec3(0, globalPrimitiveConsts->gravity_y(), 0));
        phySys->SetContactListener(this);

        PhysicsSettings clonedPhySettings = phySys->GetPhysicsSettings();
        clonedPhySettings.mUseManifoldReduction = true;

        clonedPhySettings.mConstraintWarmStart = true; 
        clonedPhySettings.mUseBodyPairContactCache = true;

        phySys->SetPhysicsSettings(clonedPhySettings);

        bi = &(phySys->GetBodyInterface());
        biNoLock = &(phySys->GetBodyInterfaceNoLock());
        narrowPhaseQueryNoLock = &(phySys->GetNarrowPhaseQueryNoLock());
       
        return true;
    }
};

/*
The "ifdBufferLock" will be managed on C# side because "sending IfdBatchSnapshot to BlockingCollection<ArraySegment<byte>> playerWsDownsyncQue" should be protected by "ifdBufferLock"
- "OnBattleCmdReceived" should be on C#
- "markConfirmationIfApplicable" & "produceIfdBatchSnapshot" should be on C++
- "inactiveMask" should be maintained by C#/OnPlayerXxx callbacks
*/

#endif
