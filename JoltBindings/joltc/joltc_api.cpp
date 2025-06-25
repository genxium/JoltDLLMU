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

void* APP_CreateBattle(char* inBytes, int inBytesCnt, bool isFrontend, bool isOnlineArenaMode) {
    BaseBattle* result = nullptr;
    if (isFrontend) {
        result = new FrontendBattle(inBytes, inBytesCnt, 512, globalPrimitiveConsts->default_backend_input_buffer_size(), globalTempAllocator, isOnlineArenaMode);
    } else {
        result = new BackendBattle(inBytes, inBytesCnt, 512, globalPrimitiveConsts->default_backend_input_buffer_size(), globalTempAllocator);
    }

    return result;
}

bool APP_DestroyBattle(void* inBattle, bool isFrontend) {
    if (isFrontend) {
        delete static_cast<FrontendBattle*>(inBattle);
    } else {
        delete static_cast<BackendBattle*>(inBattle);
    }
    return true;
}

bool APP_Step(void* inBattle, int fromRdfId, int toRdfId, bool isChasing, bool isFrontend) {
    auto battle = static_cast<BaseBattle*>(inBattle);
    battle->Step(fromRdfId, toRdfId, globalTempAllocator);
    if (isChasing) {
        auto frontendBattle = static_cast<FrontendBattle*>(inBattle);
        frontendBattle->chaserRdfId = toRdfId;
    } else {
        if (isFrontend) {
            auto frontendBattle = static_cast<FrontendBattle*>(inBattle);
            frontendBattle->timerRdfId = toRdfId;
        } else {
            auto backendBattle = static_cast<BackendBattle*>(inBattle);
            backendBattle->currDynamicsRdfId = toRdfId;
        }
    }
    return true;
}

bool APP_GetRdf(void* inBattle, int inRdfId, char* outBytesPreallocatedStart, int* outBytesCntLimit) {
    BaseBattle* battle = static_cast<BaseBattle*>(inBattle);
    RenderFrame* rdf = battle->rdfBuffer.GetByFrameId(inRdfId);
    int byteSize = rdf->ByteSize();
    if (byteSize > *outBytesCntLimit) {
        return false;
    }
    *outBytesCntLimit = byteSize;
    rdf->SerializeToArray(outBytesPreallocatedStart, byteSize);
    return true;
}

bool APP_UpsertCmd(void* inBattle, int inIfdId, uint32_t inSingleJoinIndex, uint64_t inSingleInput, char* outBytesPreallocatedStart, int* outBytesCntLimit, bool fromUdp, bool fromTcp, bool isFrontend) {
    InputFrameDownsync* result = nullptr;
    if (isFrontend) {
        auto frontendBattle = static_cast<FrontendBattle*>(inBattle);
        bool outExistingInputMutated = false;
        result = frontendBattle->GetOrPrefabInputFrameDownsync(inIfdId, inSingleJoinIndex, inSingleInput, true, false, outExistingInputMutated);
        if (outExistingInputMutated) {
            frontendBattle->HandleIncorrectlyRenderedPrediction(inIfdId, true);
        }
    } else {
        auto backendBattle = static_cast<BackendBattle*>(inBattle);
        bool outExistingInputMutated = false;
        result = backendBattle->GetOrPrefabInputFrameDownsync(inIfdId, inSingleJoinIndex, inSingleInput, fromUdp, fromTcp, outExistingInputMutated);
    }

    if (!result) {
        return false;
    }

    int byteSize = result->ByteSize();
    if (byteSize > *outBytesCntLimit) {
        return false;
    }
    *outBytesCntLimit = byteSize;
    result->SerializeToArray(outBytesPreallocatedStart, byteSize);
    return true;
}

JPH_SUPPRESS_WARNING_POP
