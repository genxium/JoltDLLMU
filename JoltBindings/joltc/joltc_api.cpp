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
#include "FrontendBattle.h"
#include "BackendBattle.h"

#ifndef NDEBUG
#include "DebugLog.h"
#endif

JPH_SUPPRESS_WARNING_PUSH
JPH_SUPPRESS_WARNINGS

// All Jolt symbols are in the JPH namespace
using namespace JPH;

const PrimitiveConsts* globalPrimitiveConsts = nullptr;
const ConfigConsts* globalConfigConsts = nullptr;

bool PrimitiveConsts_Init(char* inBytes, int inBytesCnt) {
    if (nullptr != globalPrimitiveConsts) {
        delete globalPrimitiveConsts;
    }
    PrimitiveConsts tmp;
    tmp.ParseFromArray(inBytes, inBytesCnt);
    globalPrimitiveConsts = new PrimitiveConsts(tmp);
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
    JPH::Factory::sInstance = new JPH::Factory();

    // Register all Jolt physics types
    JPH::RegisterTypes();

    // Init temp allocator
    globalTempAllocator = new TempAllocatorImplWithMallocFallback(nBytesForTempAllocator);

    return true;
}

bool JPH_Shutdown(void)
{
    delete globalTempAllocator;
    globalTempAllocator = nullptr;

    // Unregisters all types with the factory and cleans up the default material
    JPH::UnregisterTypes();

    // Destroy the factory
    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;

    return true;
}

void* FRONTEND_CreateBattle(bool isOnlineArenaMode) {
    FrontendBattle* result = new FrontendBattle(512, globalPrimitiveConsts->default_backend_input_buffer_size(), globalTempAllocator, isOnlineArenaMode);
#ifndef NDEBUG
    Debug::Log("FRONTEND_CreateBattle/C++", DColor::Green);
#endif
    return result;
}

bool FRONTEND_ResetStartRdf(void* inBattle, char* inBytes, int inBytesCnt, uint32_t inSelfJoinIndex) {
    auto frontendBattle = static_cast<FrontendBattle*>(inBattle);
    return frontendBattle->ResetStartRdf(inBytes, inBytesCnt, inSelfJoinIndex);
}

void* BACKEND_CreateBattle() {
    int rdfBufferSize = 2; // There's NO rollback on backend, so no need for a big "rdfBufferSize". 
    BackendBattle* result = new BackendBattle(rdfBufferSize, globalPrimitiveConsts->default_backend_input_buffer_size(), globalTempAllocator);
#ifndef NDEBUG
    Debug::Log("BACKEND_CreateBattle/C++", DColor::Green);
#endif
    return result;
}

bool BACKEND_ResetStartRdf(void* inBattle, char* inBytes, int inBytesCnt) {
    auto backendBattle = static_cast<BackendBattle*>(inBattle);
    return backendBattle->ResetStartRdf(inBytes, inBytesCnt);
}

int BACKEND_GetDynamicsRdfId(void* inBattle) {
    auto backendBattle = static_cast<BackendBattle*>(inBattle);
    return backendBattle->dynamicsRdfId;
}

bool BACKEND_MoveForwardLcacIfdIdAndStep(void* inBattle, bool withRefRdf, int* outOldLcacIfdId, int* outNewLcacIfdId, int* outOldDynamicsRdfId, int* outNewDynamicsRdfId, char* outBytesPreallocatedStart, long* outBytesCntLimit) {
    auto backendBattle = static_cast<BackendBattle*>(inBattle);
    return backendBattle->MoveForwardLcacIfdIdAndStep(withRefRdf, outOldLcacIfdId, outNewLcacIfdId, outOldDynamicsRdfId, outNewDynamicsRdfId, outBytesPreallocatedStart, outBytesCntLimit);
}

bool APP_DestroyBattle(void* inBattle) {
    delete static_cast<BaseBattle*>(inBattle);
    return true;
}

bool APP_GetRdf(void* inBattle, int inRdfId, char* outBytesPreallocatedStart, long* outBytesCntLimit) {
    BaseBattle* battle = static_cast<BaseBattle*>(inBattle);
    RenderFrame* rdf = battle->rdfBuffer.GetByFrameId(inRdfId);
    long byteSize = rdf->ByteSizeLong();
    if (byteSize > *outBytesCntLimit) {
        return false;
    }
    *outBytesCntLimit = byteSize;
    rdf->SerializeToArray(outBytesPreallocatedStart, byteSize);
    return true;
}

uint64_t APP_SetPlayerActive(void* inBattle, uint32_t joinIndex) {
    auto battle = static_cast<BaseBattle*>(inBattle);
    return battle->SetPlayerActive(joinIndex);
}

uint64_t APP_SetPlayerInactive(void* inBattle, uint32_t joinIndex) {
    auto battle = static_cast<BaseBattle*>(inBattle);
    return battle->SetPlayerInactive(joinIndex);
}

uint64_t APP_GetInactiveJoinMask(void* inBattle) {
    BaseBattle* battle = static_cast<BaseBattle*>(inBattle);
    return battle->GetInactiveJoinMask();
}

uint64_t APP_SetInactiveJoinMask(void* inBattle, uint64_t value) {
    BaseBattle* battle = static_cast<BaseBattle*>(inBattle);
    return battle->SetInactiveJoinMask(value);
}

bool BACKEND_ProduceDownsyncSnapshot(void* inBattle, uint64_t unconfirmedMask, int stIfdId, int edIfdId, bool withRefRdf, char* outBytesPreallocatedStart, long* outBytesCntLimit) {
    auto backendBattle = static_cast<BackendBattle*>(inBattle);
    return backendBattle->ProduceDownsyncSnapshotAndSerialize(unconfirmedMask, stIfdId, edIfdId, withRefRdf, outBytesPreallocatedStart, outBytesCntLimit);
}

bool BACKEND_OnUpsyncSnapshotReceived(void* inBattle, char* inBytes, int inBytesCnt, bool fromUdp, bool fromTcp, char* outBytesPreallocatedStart, long* outBytesCntLimit, int* outStEvictedCnt) {
    auto backendBattle = static_cast<BackendBattle*>(inBattle);
    return backendBattle->OnUpsyncSnapshotReceived(inBytes, inBytesCnt, fromUdp, fromTcp, outBytesPreallocatedStart, outBytesCntLimit, outStEvictedCnt);
}

void BACKEND_Step(void* inBattle, int fromRdfId, int toRdfId) {
    auto backendBattle = static_cast<BackendBattle*>(inBattle);
    backendBattle->Step(fromRdfId, toRdfId);
}

bool FRONTEND_UpsertSelfCmd(void* inBattle, uint64_t inSingleInput) {
    auto frontendBattle = static_cast<FrontendBattle*>(inBattle);
    return frontendBattle->UpsertSelfCmd(inSingleInput);
}

bool FRONTEND_OnUpsyncSnapshotReceived(void* inBattle, char* inBytes, int inBytesCnt) {
    auto frontendBattle = static_cast<FrontendBattle*>(inBattle);
    return frontendBattle->OnUpsyncSnapshotReceived(inBytes, inBytesCnt);
}

bool FRONTEND_ProduceUpsyncSnapshot(void* inBattle, int proposedBatchIfdIdSt, int proposedBatchIfdIdEd, char* outBytesPreallocatedStart, long* outBytesCntLimit) {
    auto frontendBattle = static_cast<FrontendBattle*>(inBattle);
    return frontendBattle->ProduceUpsyncSnapshot(proposedBatchIfdIdSt, proposedBatchIfdIdEd, outBytesPreallocatedStart, outBytesCntLimit);
}

bool FRONTEND_OnDownsyncSnapshotReceived(void* inBattle, char* inBytes, int inBytesCnt, int* outPostTimerRdfEvictedCnt, int* outPostTimerRdfDelayedIfdEvictedCnt, int* outNewLcacIfdId) {
    auto frontendBattle = static_cast<FrontendBattle*>(inBattle);
    return frontendBattle->OnDownsyncSnapshotReceived(inBytes, inBytesCnt, outPostTimerRdfEvictedCnt, outPostTimerRdfDelayedIfdEvictedCnt, outNewLcacIfdId);
}

void FRONTEND_Step(void* inBattle, int fromRdfId, int toRdfId, bool isChasing) {
    auto frontendBattle = static_cast<FrontendBattle*>(inBattle);
    frontendBattle->Step(fromRdfId, toRdfId, isChasing);
}

void FRONTEND_GetRdfAndIfdIds(void* inBattle, int* outTimerRdfId, int* outChaserRdfId, int* outChaserRdfIdLowerBound, int* outPlayerInputFrontIds, int* outLcacIfdId, int* outTimerRdfIdGenIfdId, int* outTimerRdfIdToUseIfdId) {
    auto frontendBattle = static_cast<FrontendBattle*>(inBattle);
    frontendBattle->GetRdfAndIfdIds(outTimerRdfId, outChaserRdfId, outChaserRdfIdLowerBound, outPlayerInputFrontIds, outLcacIfdId, outTimerRdfIdGenIfdId, outTimerRdfIdToUseIfdId);
}

JPH_SUPPRESS_WARNING_POP
