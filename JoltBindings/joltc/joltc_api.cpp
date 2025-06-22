#include "joltc_api.h"

// The Jolt headers don't include Jolt.h. Always include Jolt.h before including any other Jolt header.
// You can use Jolt.h in your precompiled header to speed up compilation.
#include <Jolt/Jolt.h>

// Jolt includes
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/RTTI.h>
#include <Jolt/Core/TempAllocator.h>

#include "BattleConsts.h"
#include "FrontendBattle.h"
#include "BackendBattle.h"

JPH_SUPPRESS_WARNING_PUSH
JPH_SUPPRESS_WARNINGS

// All Jolt symbols are in the JPH namespace
using namespace JPH;

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

void* APP_CreateBattle(char* inBytes, int inBytesCnt, bool isFrontend) {
    BaseBattle* result = nullptr;
    if (isFrontend) {
        result = new FrontendBattle(inBytes, inBytesCnt, DEFAULT_BACKEND_RENDER_BUFFER_SIZE, DEFAULT_BACKEND_INPUT_BUFFER_SIZE);
    } else {
        result = new BackendBattle(inBytes, inBytesCnt, DEFAULT_BACKEND_RENDER_BUFFER_SIZE, DEFAULT_BACKEND_INPUT_BUFFER_SIZE);
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

bool APP_Step(void* inBattle, int fromRdfId, int toRdfId, bool isChasing) {
    auto battle = static_cast<BaseBattle*>(inBattle);
    battle->Step(fromRdfId, toRdfId, globalTempAllocator);
    if (isChasing) {
        auto frontendBattle = static_cast<FrontendBattle*>(inBattle);
        frontendBattle->chaserRdfId = toRdfId;
    }
    return true;
}

bool APP_GetRdf(void* inBattle, int inRdfId, char* outBytesPreallocatedStart, int outBytesCntLimit) {
    return false;
}

JPH_SUPPRESS_WARNING_POP
