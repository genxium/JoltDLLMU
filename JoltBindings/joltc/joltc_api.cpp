#include "joltc_api.h"

// The Jolt headers don't include Jolt.h. Always include Jolt.h before including any other Jolt header.
// You can use Jolt.h in your precompiled header to speed up compilation.
#include <Jolt/Jolt.h>

// Jolt includes
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/RTTI.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

#include <Jolt/Physics/Character/CharacterVirtual.h>

// STL includes
#include <cstdarg>
#include <thread>
#include "BattleConsts.h"
#include "BaseBattle.h"
#include "FrontendBattle.h"
#include "BackendBattle.h"
#include "CollisionLayers.h"
#include "CollisionCallbacks.h"
#include "serializable_data.pb.h"

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

float const cDeltaTime = 1.0f / 60.0f;
float const defaultThickness = 2.0f;
float const defaultHalfThickness = defaultThickness * 0.5f;
void* APP_CreateBattle(char* inBytes, int inBytesCnt, bool isFrontend) {
    shared::WsReq initializerMapData;
    std::string initializerMapDataStr(inBytes, inBytesCnt);
    initializerMapData.ParseFromString(initializerMapDataStr);
    auto staticColliderShapesFromTiled = initializerMapData.serialized_barrier_polygons();

    const uint cMaxBodies = 1024;
    const uint cNumBodyMutexes = 0;
    const uint cMaxBodyPairs = 1024;

    const uint cMaxContactConstraints = 1024;

    BPLayerInterfaceImpl broad_phase_layer_interface;
    ObjectVsBroadPhaseLayerFilterImpl object_vs_broadphase_layer_filter;
    ObjectLayerPairFilterImpl object_vs_object_layer_filter;
    PhysicsSystem* physics_system = new PhysicsSystem();
    physics_system->Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, broad_phase_layer_interface, object_vs_broadphase_layer_filter, object_vs_object_layer_filter);

    MyBodyActivationListener body_activation_listener;
    physics_system->SetBodyActivationListener(&body_activation_listener);

    MyContactListener contact_listener;
    physics_system->SetContactListener(&contact_listener);
    //physics_system->SetSimCollideBodyVsBody(&my_body_collision_pipe); // To omit unwanted body collisions, e.g. same team
    BodyInterface &body_interface = physics_system->GetBodyInterface();
    
    for (auto convexPolygon : staticColliderShapesFromTiled) {
        TriangleList triangles; 
        for (int pi = 0; pi < convexPolygon.points_size(); pi += 2) {
            auto fromI = pi;
            auto toI = fromI + 2;
            if (toI >= convexPolygon.points_size()) {
                toI = 0;
            }
            auto x1 = convexPolygon.points(fromI); 
            auto y1 = convexPolygon.points(fromI+1);
            auto x2 = convexPolygon.points(toI);
            auto y2 = convexPolygon.points(toI+1);

            Float3 v1 = Float3(x1, y1, +defaultHalfThickness);
            Float3 v2 = Float3(x1, y1, -defaultHalfThickness);
            Float3 v3 = Float3(x2, y2, +defaultHalfThickness);
            Float3 v4 = Float3(x2, y2, -defaultHalfThickness);

            triangles.push_back(Triangle(v2, v1, v3, 0)); // y: -, +, +
            triangles.push_back(Triangle(v3, v4, v2, 0)); // y: +, -, - 
        }
        MeshShapeSettings bodyShapeSettings(triangles);
        MeshShapeSettings::ShapeResult shapeResult;
        /*
        "Body" and "BodyCreationSettings" will handle lifecycle of the following "bodyShape", i.e. 
        - by "RefConst<Shape> Body::mShape" (note that "Body" does NOT hold a member variable "BodyCreationSettings") or 
        - by "RefConst<ShapeSettings> BodyCreationSettings::mShape". 
        
        See "MonsterInsight/joltphysics/RefConst_destructor_trick.md" for details.
        */
        RefConst<Shape> bodyShape = new MeshShape(bodyShapeSettings, shapeResult); 
        BodyCreationSettings bodyCreationSettings(bodyShape, RVec3::sZero(), JPH::Quat::sIdentity(), EMotionType::Static, MonInsObjectLayers::NON_MOVING);
        body_interface.CreateAndAddBody(bodyCreationSettings, EActivation::DontActivate);
    }

    auto startRdf = initializerMapData.self_parsed_rdf();
   
    physics_system->OptimizeBroadPhase();

    JobSystemThreadPool* job_system = new JobSystemThreadPool(cMaxPhysicsJobs, cMaxPhysicsBarriers, thread::hardware_concurrency() - 1);
     
    int nPlayerCnt = startRdf.players_arr_size();
    BaseBattle* result = nullptr;
    if (isFrontend) {
        result = new FrontendBattle(nPlayerCnt, DEFAULT_BACKEND_RENDER_BUFFER_SIZE, DEFAULT_BACKEND_INPUT_BUFFER_SIZE, physics_system, job_system);
    } else {
        result = new BackendBattle(nPlayerCnt, DEFAULT_BACKEND_RENDER_BUFFER_SIZE, DEFAULT_BACKEND_INPUT_BUFFER_SIZE, physics_system, job_system);
    }

    result->rdfBuffer.Put(startRdf);
    
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
