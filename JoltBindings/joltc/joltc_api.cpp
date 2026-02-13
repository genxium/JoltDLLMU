#include "joltc_api.h"

// The Jolt headers don't include Jolt.h. Always include Jolt.h before including any other Jolt header.
// You can use Jolt.h in your precompiled header to speed up compilation.
#include <Jolt/Jolt.h>

// Jolt includes
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/RTTI.h>
#include <Jolt/Core/TempAllocator.h>

#include "CppOnlyConsts.h"
#include "PbConsts.h"
#include "NpcReactionConsts.h"
#include "FrontendBattle.h"
#include "BackendBattle.h"
#include <unordered_set>

#ifndef NDEBUG
#include "DebugLog.h"
#endif

JPH_SUPPRESS_WARNING_PUSH
JPH_SUPPRESS_WARNINGS

// All Jolt symbols are in the JPH namespace
using namespace JPH;

const PrimitiveConsts* globalPrimitiveConsts = nullptr;
const ConfigConsts* globalConfigConsts = nullptr;
std::unordered_map<uint32_t, BaseNpcReaction*> globalNpcReactionMap;
std::unordered_set<uint32_t> trivialTrtSet;
std::unordered_set<uint32_t> timedTrtSet;

bool PrimitiveConsts_Init(char* inBytes, int inBytesCnt) {
    if (nullptr != globalPrimitiveConsts) {
        delete globalPrimitiveConsts;
    }
    PrimitiveConsts tmp;
    tmp.ParseFromArray(inBytes, inBytesCnt);
    globalPrimitiveConsts = new PrimitiveConsts(tmp);

    auto& chSpecies = globalPrimitiveConsts->ch_species();
    globalNpcReactionMap[chSpecies.blacksaber1()] = new BlackSaber1NpcReaction();
    globalNpcReactionMap[chSpecies.blacksaber_test_with_vision()] = new BlackSaberTestWithVisionNpcReaction();

    trivialTrtSet = {
        globalPrimitiveConsts->trt_victory(),
        globalPrimitiveConsts->trt_by_movement(),
        globalPrimitiveConsts->trt_by_attack(),
        globalPrimitiveConsts->trt_save_point_only(),
        globalPrimitiveConsts->trt_story_point_only(),
        globalPrimitiveConsts->trt_save_and_story_point(),
    };

    timedTrtSet = {
        globalPrimitiveConsts->trt_cyclic_timed(),
    };

    return true;
}

bool ConfigConsts_Init(char* inBytes, int inBytesCnt) {
    if (nullptr != globalConfigConsts) {
        delete globalConfigConsts;
    }
    ConfigConsts tmp;
    tmp.ParseFromArray(inBytes, inBytesCnt);
    globalConfigConsts = new ConfigConsts(tmp);
    return true;
}

static TempAllocatorImplWithMallocFallback* globalTempAllocator = nullptr;
bool JPH_Init(int nBytesForTempAllocator)
{
    // Register allocation hook. by far we'll just let Jolt use malloc / free but you can override these if you want (see Memory.h).
    // This needs to be done before any other Jolt function is called.
    JPH::RegisterDefaultAllocator();

    // Create a factory
    if (nullptr == JPH::Factory::sInstance) {
        JPH::Factory::sInstance = new JPH::Factory();
    }

    // Register all Jolt physics types
    JPH::RegisterTypes();

    // Init temp allocator
    if (nullptr == globalTempAllocator) {
        globalTempAllocator = new TempAllocatorImplWithMallocFallback(nBytesForTempAllocator);
    } 

    return true;
}

bool JPH_Shutdown(void)
{
    if (nullptr != globalTempAllocator) {
        delete globalTempAllocator;
        globalTempAllocator = nullptr;
    } 

    // Unregisters all types with the factory and cleans up the default material
    JPH::UnregisterTypes();

    // Destroy the factory
    if (nullptr != JPH::Factory::sInstance) {
        delete JPH::Factory::sInstance;
        JPH::Factory::sInstance = nullptr;
    }

    return true;
}

void APP_ClearBattle(void* inBattle) {
    auto battle = static_cast<BaseBattle*>(inBattle);
    if (nullptr == battle) return;
    battle->Clear();
}

bool APP_DestroyBattle(void* inBattle) {
    if (nullptr == inBattle) return false;
    delete static_cast<BaseBattle*>(inBattle);
    return true;
}

bool APP_GetRdf(void* inBattle, int inRdfId, char* outBytesPreallocatedStart, long* outBytesCntLimit) {
    BaseBattle* battle = static_cast<BaseBattle*>(inBattle);
    if (nullptr == battle) return false;
    RenderFrame* rdf = battle->rdfBuffer.GetByFrameId(inRdfId);
    long byteSize = rdf->ByteSizeLong();
    if (byteSize > *outBytesCntLimit) {
        return false;
    }
    *outBytesCntLimit = byteSize;
    rdf->SerializeToArray(outBytesPreallocatedStart, byteSize);
    return true;
}

 bool APP_GetRdfBufferBounds(void* inBattle, int* outStRdfId, int* outEdRdfId) {
    BaseBattle* battle = static_cast<BaseBattle*>(inBattle);
    if (nullptr == battle) return false;
    *outStRdfId = battle->rdfBuffer.StFrameId; 
    *outEdRdfId = battle->rdfBuffer.EdFrameId; 
    return true;
}

 bool APP_SetFrameLogEnabled(void* inBattle, bool val) {
     BaseBattle* battle = static_cast<BaseBattle*>(inBattle);
     if (nullptr == battle) return false;
     return battle->SetFrameLogEnabled(val);
}

bool APP_GetFrameLog(void* inBattle, int inRdfId, char* outBytesPreallocatedStart, long* outBytesCntLimit) {
    BaseBattle* battle = static_cast<BaseBattle*>(inBattle);
    if (nullptr == battle) return false;
    FrameLog* frameLog = battle->frameLogBuffer.GetByFrameId(inRdfId);
    if (nullptr == frameLog) {
#ifndef NDEBUG
        std::ostringstream oss;
        oss << "@inRdfId=" << inRdfId << ", couldn't get frameLog, frameLogBuff stat=" << battle->frameLogBuffer.toSimpleStat();
        Debug::Log(oss.str(), DColor::Orange);
#endif
        return false;
    }

    long byteSize = frameLog->ByteSizeLong();
    if (byteSize > *outBytesCntLimit) {
#ifndef NDEBUG
        std::ostringstream oss;
        oss << "@inRdfId=" << inRdfId << ", couldn't serialize frameLog, frameLogBuff stat=" << battle->frameLogBuffer.toSimpleStat() << ", byteSize=" << byteSize << ".";
        Debug::Log(oss.str(), DColor::Orange);
#endif
        return false;
    }
    *outBytesCntLimit = byteSize;
    frameLog->SerializeToArray(outBytesPreallocatedStart, byteSize);
    return true;
}

uint64_t APP_SetPlayerActive(void* inBattle, uint32_t joinIndex) {
    auto battle = static_cast<BaseBattle*>(inBattle);
    if (nullptr == battle) return 0;
    return battle->SetPlayerActive(joinIndex);
}

uint64_t APP_SetPlayerInactive(void* inBattle, uint32_t joinIndex) {
    auto battle = static_cast<BaseBattle*>(inBattle);
    if (nullptr == battle) return 0;
    return battle->SetPlayerInactive(joinIndex);
}

uint64_t APP_GetInactiveJoinMask(void* inBattle) {
    BaseBattle* battle = static_cast<BaseBattle*>(inBattle);
    if (nullptr == battle) return 0;
    return battle->GetInactiveJoinMask();
}

uint64_t APP_SetInactiveJoinMask(void* inBattle, uint64_t value) {
    BaseBattle* battle = static_cast<BaseBattle*>(inBattle);
    if (nullptr == battle) return 0;
    return battle->SetInactiveJoinMask(value);
}

uint32_t APP_GetUDPayload(uint64_t ud) {
    return BaseBattleCollisionFilter::getUDPayload(ud);
}

uint64_t APP_CalcStaticColliderUserData(uint32_t staticColliderId) {
    return BaseBattleCollisionFilter::calcStaticColliderUserData(staticColliderId);
}

uint64_t APP_CalcPlayerUserData(uint32_t joinIndex) {
    return BaseBattleCollisionFilter::calcPlayerUserData(joinIndex);
}

uint64_t APP_CalcNpcUserData(uint32_t npcId) {
    return BaseBattleCollisionFilter::calcNpcUserData(npcId);
}

uint64_t APP_CalcBulletUserData(uint32_t bulletId) {
    return BaseBattleCollisionFilter::calcBulletUserData(bulletId);
}

uint64_t APP_CalcTriggerUserData(uint32_t triggerId) {
    return BaseBattleCollisionFilter::calcTriggerUserData(triggerId);
}

uint64_t APP_CalcTrapUserData(uint32_t trapId) {
    return BaseBattleCollisionFilter::calcTrapUserData(trapId);
}

uint64_t APP_CalcPickableUserData(uint32_t pickableId) {
    return BaseBattleCollisionFilter::calcPickableUserData(pickableId);
}

void* BACKEND_CreateBattle(int rdfBufferSize) {
    // There's NO rollback on backend, so no need for a big "rdfBufferSize". 
    BackendBattle* result = new BackendBattle(rdfBufferSize, globalPrimitiveConsts->default_backend_input_buffer_size(), globalTempAllocator);
#ifndef NDEBUG
    Debug::Log("BACKEND_CreateBattle/C++", DColor::Green);
#endif
    return result;
} 

bool BACKEND_OnUpsyncSnapshotReqReceived(void* inBattle, char* inBytes, int inBytesCnt, bool fromUdp, bool fromTcp, char* outBytesPreallocatedStart, long* outBytesCntLimit, int* outStEvictedCnt, int* outOldLcacIfdId, int* outNewLcacIfdId, int* outOldDynamicsRdfId, int* outNewDynamicsRdfId, int* outMaxPlayerInputFrontId, int* outMinPlayerInputFrontId) {
    auto backendBattle = static_cast<BackendBattle*>(inBattle);
    if (nullptr == backendBattle) return false;
    return backendBattle->OnUpsyncSnapshotReqReceived(inBytes, inBytesCnt, fromUdp, fromTcp, outBytesPreallocatedStart, outBytesCntLimit, outStEvictedCnt, outOldLcacIfdId, outNewLcacIfdId, outOldDynamicsRdfId, outNewDynamicsRdfId, outMaxPlayerInputFrontId, outMinPlayerInputFrontId);
}

int BACKEND_Step(void* inBattle, int fromRdfId, int toRdfId) {
    auto backendBattle = static_cast<BackendBattle*>(inBattle);
    if (nullptr == backendBattle) return false;
    return backendBattle->Step(fromRdfId, toRdfId);
}

int BACKEND_MoveForwardLcacIfdIdAndStep(void* inBattle, bool withRefRdf, int* outOldLcacIfdId, int* outNewLcacIfdId, int* outOldDynamicsRdfId, int* outNewDynamicsRdfId, char* outBytesPreallocatedStart, long* outBytesCntLimit) {
    auto backendBattle = static_cast<BackendBattle*>(inBattle);
    if (nullptr == backendBattle) return false;
    return backendBattle->MoveForwardLcacIfdIdAndStep(withRefRdf, outOldLcacIfdId, outNewLcacIfdId, outOldDynamicsRdfId, outNewDynamicsRdfId, outBytesPreallocatedStart, outBytesCntLimit);
}

bool BACKEND_ResetStartRdf(void* inBattle, char* inBytes, int inBytesCnt) {
    auto backendBattle = static_cast<BackendBattle*>(inBattle);
    if (nullptr == backendBattle) return false;
    return backendBattle->ResetStartRdf(inBytes, inBytesCnt);
}

int BACKEND_GetDynamicsRdfId(void* inBattle) {
    auto backendBattle = static_cast<BackendBattle*>(inBattle);
    if (nullptr == backendBattle) return false;
    return backendBattle->GetDynamicsRdfId();
}

void* FRONTEND_CreateBattle(int rdfBufferSize, bool isOnlineArenaMode) {
    FrontendBattle* result = new FrontendBattle(rdfBufferSize, (rdfBufferSize >> (globalPrimitiveConsts->input_scale_frames() >> 1)) + 1, globalTempAllocator, isOnlineArenaMode);
#ifndef NDEBUG
    Debug::Log("FRONTEND_CreateBattle/C++", DColor::Green);
#endif
    return result;
}

bool FRONTEND_ResetStartRdf(void* inBattle, char* inBytes, int inBytesCnt, const uint32_t inSelfJoinIndex, const char * const inSelfPlayerId, const int inSelfCmdAuthKey) {
    auto frontendBattle = static_cast<FrontendBattle*>(inBattle);
    if (nullptr == frontendBattle) return false;
    return frontendBattle->ResetStartRdf(inBytes, inBytesCnt, inSelfJoinIndex, inSelfPlayerId, inSelfCmdAuthKey);
}

bool FRONTEND_UpsertSelfCmd(void* inBattle, uint64_t inSingleInput, int* outChaserRdfId) {
    auto frontendBattle = static_cast<FrontendBattle*>(inBattle);
    if (nullptr == frontendBattle) return false;
    return frontendBattle->UpsertSelfCmd(inSingleInput, outChaserRdfId);
}

bool FRONTEND_ProduceUpsyncSnapshotRequest(void* inBattle, int seqNo, int proposedBatchIfdIdSt, int proposedBatchIfdIdEd, int* outLastIfdId, char* outBytesPreallocatedStart, long* outBytesCntLimit) {
    auto frontendBattle = static_cast<FrontendBattle*>(inBattle);
    if (nullptr == frontendBattle) return false;
    return frontendBattle->ProduceUpsyncSnapshotRequest(seqNo, proposedBatchIfdIdSt, proposedBatchIfdIdEd, outLastIfdId, outBytesPreallocatedStart, outBytesCntLimit);
}

bool FRONTEND_OnUpsyncSnapshotReqReceived(void* inBattle, char* inBytes, int inBytesCnt, int* outChaserRdfId, int* outMaxPlayerInputFrontId, int* outMinPlayerInputFrontId) {
    auto frontendBattle = static_cast<FrontendBattle*>(inBattle);
    if (nullptr == frontendBattle) return false;
    return frontendBattle->OnUpsyncSnapshotReqReceived(inBytes, inBytesCnt, outChaserRdfId, outMaxPlayerInputFrontId, outMinPlayerInputFrontId);
}

bool FRONTEND_OnDownsyncSnapshotReceived(void* inBattle, char* inBytes, int inBytesCnt, int* outPostTimerRdfEvictedCnt, int* outPostTimerRdfDelayedIfdEvictedCnt, int* outChaserRdfId, int* outLcacIfdId, int* outMaxPlayerInputFrontId, int* outMinPlayerInputFrontId) {
    auto frontendBattle = static_cast<FrontendBattle*>(inBattle);
    if (nullptr == frontendBattle) return false;
    return frontendBattle->OnDownsyncSnapshotReceived(inBytes, inBytesCnt, outPostTimerRdfEvictedCnt, outPostTimerRdfDelayedIfdEvictedCnt, outChaserRdfId, outLcacIfdId, outMaxPlayerInputFrontId, outMinPlayerInputFrontId);
}

bool FRONTEND_Step(void* inBattle) {
    auto frontendBattle = static_cast<FrontendBattle*>(inBattle);
    if (nullptr == frontendBattle) return false;
    return frontendBattle->Step();
}

bool FRONTEND_ChaseRolledBackRdfs(void* inBattle, int* outChaserRdfId, bool toTimerRdfId) {
    auto frontendBattle = static_cast<FrontendBattle*>(inBattle);
    if (nullptr == frontendBattle) return false;
    return frontendBattle->ChaseRolledBackRdfs(outChaserRdfId, toTimerRdfId);
}

bool FRONTEND_GetRdfAndIfdIds(void* inBattle, int* outTimerRdfId, int* outChaserRdfId, int* outChaserRdfIdLowerBound, int* outLcacIfdId, int* outTimerRdfIdGenIfdId, int* outTimerRdfIdToUseIfdId) {
    auto frontendBattle = static_cast<FrontendBattle*>(inBattle);
    if (nullptr == frontendBattle) return false;
    return frontendBattle->GetRdfAndIfdIds(outTimerRdfId, outChaserRdfId, outChaserRdfIdLowerBound, outLcacIfdId, outTimerRdfIdGenIfdId, outTimerRdfIdToUseIfdId);
}

JPH_SUPPRESS_WARNING_POP
