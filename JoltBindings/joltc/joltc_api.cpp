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

void* FRONTEND_CreateBattle(char* inBytes, int inBytesCnt, bool isOnlineArenaMode, int inSelfJoinIndex) {
    FrontendBattle* result = new FrontendBattle(inBytes, inBytesCnt, 512, globalPrimitiveConsts->default_backend_input_buffer_size(), globalTempAllocator, isOnlineArenaMode, inSelfJoinIndex);
#ifndef NDEBUG
    Debug::Log("FRONTEND_CreateBattle/C++", DColor::Green);
#endif
    return result;
}

void* BACKEND_CreateBattle() {
    BackendBattle* result = new BackendBattle(512, globalPrimitiveConsts->default_backend_input_buffer_size(), globalTempAllocator);
#ifndef NDEBUG
    Debug::Log("BACKEND_CreateBattle/C++", DColor::Green);
#endif
    return result;
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

bool BACKEND_ProduceDownsyncSnapshot(void* inBattle, uint64_t unconfirmedMask, int stIfdId, int edIfdId, bool withRefRdf, char* outBytesPreallocatedStart, long* outBytesCntLimit) {
    auto backendBattle = static_cast<BackendBattle*>(inBattle);
    return backendBattle->ProduceDownsyncSnapshotAndSerialize(unconfirmedMask, stIfdId, edIfdId, withRefRdf, outBytesPreallocatedStart, outBytesCntLimit);
}

bool BACKEND_OnUpsyncSnapshotReceived(void* inBattle, char* inBytes, int inBytesCnt, bool fromUdp, bool fromTcp, char* outBytesPreallocatedStart, long* outBytesCntLimit) {
    auto backendBattle = static_cast<BackendBattle*>(inBattle);
    return backendBattle->OnUpsyncSnapshotReceived(inBytes, inBytesCnt, fromUdp, fromTcp, outBytesPreallocatedStart, outBytesCntLimit);
}

bool BACKEND_Step(void* inBattle, int fromRdfId, int toRdfId) {
    auto backendBattle = static_cast<BackendBattle*>(inBattle);
    backendBattle->Step(fromRdfId, toRdfId);
    return true;
}

bool FRONTEND_UpsertSelfCmd(void* inBattle, uint64_t inSingleInput) {
    auto frontendBattle = static_cast<FrontendBattle*>(inBattle);

    int toGenIfdId = BaseBattle::ConvertToGeneratingIfdId(frontendBattle->timerRdfId);
    int nextRdfToGenIfdId = BaseBattle::ConvertToGeneratingIfdId(frontendBattle->timerRdfId+1);
    bool isLastRdfInIfdCoverage = (nextRdfToGenIfdId > toGenIfdId);

    InputFrameDownsync* result = nullptr;
    bool outExistingInputMutated = false;
    result = frontendBattle->GetOrPrefabInputFrameDownsync(toGenIfdId, frontendBattle->selfJoinIndex, inSingleInput, isLastRdfInIfdCoverage, false, outExistingInputMutated);
    if (outExistingInputMutated) {
        frontendBattle->HandleIncorrectlyRenderedPrediction(toGenIfdId, true);
    }

    if (!result) {
        return false;
    }

    return true;
}

bool FRONTEND_OnUpsyncSnapshotReceived(void* inBattle, char* inBytes, int inBytesCnt, bool fromUdp, bool fromTcp) {
    auto frontendBattle = static_cast<FrontendBattle*>(inBattle);
    return frontendBattle->OnUpsyncSnapshotReceived(inBytes, inBytesCnt, fromUdp, fromTcp);
}

bool FRONTEND_OnDownsyncSnapshotReceived(void* inBattle, char* inBytes, int inBytesCnt) {
    auto frontendBattle = static_cast<FrontendBattle*>(inBattle);
    return frontendBattle->OnDownsyncSnapshotReceived(inBytes, inBytesCnt);
}

bool FRONTEND_Step(void* inBattle, int fromRdfId, int toRdfId, bool isChasing) {
    auto frontendBattle = static_cast<FrontendBattle*>(inBattle);
    frontendBattle->Step(fromRdfId, toRdfId, isChasing);
    return true;
}

JPH_SUPPRESS_WARNING_POP
