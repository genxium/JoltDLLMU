#include "BaseBattle.h"

#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

// STL includes
#include <cstdarg>
#include <thread>
#include <string>

using namespace jtshared;

static inline uint cMaxBodies = 1024;
static inline uint cNumBodyMutexes = 0;
static inline uint cMaxBodyPairs = 1024;
static inline uint cMaxContactConstraints = 1024;

static inline float cDeltaTime = 1.0f / 60.0f;
static inline float defaultThickness = 2.0f;
static inline float defaultHalfThickness = defaultThickness * 0.5f;

static inline EBackFaceMode sBackFaceMode = EBackFaceMode::CollideWithBackFaces;
static inline float		sUpRotationX = 0;
static inline float		sUpRotationZ = 0;
static inline float		sMaxSlopeAngle = DegreesToRadians(45.0f);
static inline float		sMaxStrength = 100.0f;
static inline float		sCharacterPadding = 0.02f;
static inline float		sPenetrationRecoverySpeed = 1.0f;
static inline float		sPredictiveContactDistance = 0.1f;
static inline bool		sEnableWalkStairs = true;
static inline bool		sEnableStickToFloor = true;
static inline bool		sEnhancedInternalEdgeRemoval = false;
static inline bool		sCreateInnerBody = false;
static inline bool		sPlayerCanPushOtherCharacters = true;
static inline bool		sOtherCharactersCanPushPlayer = true;


BaseBattle::BaseBattle(char* inBytes, int inBytesCnt, int renderBufferSize, int inputBufferSize) : rdfBuffer(renderBufferSize), ifdBuffer(inputBufferSize) {
    jtshared::WsReq initializerMapData;
    std::string initializerMapDataStr(inBytes, inBytesCnt);
    initializerMapData.ParseFromString(initializerMapDataStr);

    ////////////////////////////////////////////// 1
    auto startRdf = initializerMapData.self_parsed_rdf();
    playersCnt = startRdf.players_arr_size();
    allConfirmedMask = (U64_1 << playersCnt) - 1;
    inactiveJoinMask = 0u;
    timerRdfId = 0;
    battleDurationFrames = 0;

    RenderFrame* holder = rdfBuffer.DryPut();
    holder->MergeFrom(startRdf);

    prefabbedInputList.assign(playersCnt, 0);   
    playerInputFrontIds.assign(playersCnt, 0); 
    playerInputFronts.assign(playersCnt, 0); 

    ////////////////////////////////////////////// 2
    phySys = new PhysicsSystem();
    phySys->Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, bpLayerInterface, ovbLayerFilter, ovoLayerFilter);
    phySys->SetBodyActivationListener(&bodyActivationListener);
    phySys->SetContactListener(&contactListener);
    //phySys->SetSimCollideBodyVsBody(&myBodyCollisionPipe); // To omit unwanted body collisions, e.g. same team
    BodyInterface &bi = phySys->GetBodyInterface();

    auto staticColliderShapesFromTiled = initializerMapData.serialized_barrier_polygons();
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
        BodyCreationSettings bodyCreationSettings(bodyShape, RVec3::sZero(), JPH::Quat::sIdentity(), EMotionType::Static, MyObjectLayers::NON_MOVING);
        bi.CreateAndAddBody(bodyCreationSettings, EActivation::DontActivate);
    }
    phySys->OptimizeBroadPhase();

    ////////////////////////////////////////////// 3
    jobSys = new JobSystemThreadPool(cMaxPhysicsJobs, cMaxPhysicsBarriers, thread::hardware_concurrency() - 1);

    ////////////////////////////////////////////// 4
    transientCurrJoinIndexToChdMap.reserve(playersCnt + DEFAULT_PREALLOC_NPC_CAPACITY);

    dummyCc.set_capsule_radius(3.0);
    dummyCc.set_capsule_half_height(10.0);
}

BaseBattle::~BaseBattle() {
    playersCnt = 0;
    delete phySys;
    delete jobSys;

    /*
    [WARNING] Unlike "std::vector" and "std::unordered_map", the customized containers "JPH::StaticArray" and "JPH::UnorderedMap" will deallocate their pointer-typed elements in their destructors.
    - https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Core/Array.h#L337 -> https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Core/Array.h#L249
    - https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Core/HashTable.h#L480 -> https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Core/HashTable.h#L497

    However I only use "dynamicBodyIDs" to hold "BodyID" instances which are NOT pointer-typed and effectively freed once "JPH::StaticArray::clear()" is called, i.e. no need for the heavy duty "JPH::Array".
    */
}

// Backend & Frontend shared functions
int BaseBattle::moveForwardlastConsecutivelyAllConfirmedIfdId(int proposedIfdEdFrameId, uint64_t skippableJoinMask, uint64_t& unconfirmedMask) {
    // [WARNING/BACKEND] This function MUST BE called while "inputBufferLock" is locked!

    int incCnt = 0;
    int proposedIfdStFrameId = lastConsecutivelyAllConfirmedIfdId + 1;
    if (proposedIfdStFrameId >= proposedIfdEdFrameId) {
        return incCnt;
    }
    
    unconfirmedMask |= inactiveJoinMask;
    for (int inputFrameId = proposedIfdStFrameId; inputFrameId < proposedIfdEdFrameId; inputFrameId++) {
        // See comments for the traversal in "markConfirmationIfApplicable".
        if (inputFrameId < ifdBuffer.StFrameId) {
            continue;
        }
        InputFrameDownsync* ifd = ifdBuffer.GetByFrameId(inputFrameId);
        if (nullptr == ifd) {
            throw std::runtime_error("[_moveForwardlastConsecutivelyAllConfirmedIfdId] inputFrameId=" + std::to_string(inputFrameId) + " doesn't exist for lastConsecutivelyAllConfirmedIfdId=" + std::to_string(lastConsecutivelyAllConfirmedIfdId) + ", proposedIfdStFrameId = " + std::to_string(proposedIfdStFrameId) + ", proposedIfdEdFrameId = " + std::to_string(proposedIfdEdFrameId) + ", inputBuffer = " + ifdBuffer.toSimpleStat());
        }

        if (allConfirmedMask != (ifd->confirmed_list() | inactiveJoinMask | skippableJoinMask)) {
            break;
        }

        incCnt += 1;
        ifd->set_confirmed_list(allConfirmedMask);
        if (lastConsecutivelyAllConfirmedIfdId < inputFrameId) {
            // Such that "lastConsecutivelyAllConfirmedIfdId" is monotonic.
            lastConsecutivelyAllConfirmedIfdId = inputFrameId;
        }
    }

    return incCnt;
}

CharacterVirtual* BaseBattle::getOrCreateCachedCharacterCollider(CharacterDownsync* cd, CharacterConfig* cc) {
    auto capsuleKey = Vec3(1.0, cc->capsule_radius(), cc->capsule_half_height());
    RVec3Arg initialPos = RVec3Arg(cd->x(), cd->y(), 0);
    CharacterVirtual* chCollider = nullptr;
    auto itChCollider = cachedChColliders.find(capsuleKey);
    if (itChCollider == cachedChColliders.end()) {
        CapsuleShape* chShapeCenterAnchor = new CapsuleShape(dummyCc.capsule_half_height(), dummyCc.capsule_radius()); // transient, only created on stack for immediately transform
        RefConst<Shape> chShape = RotatedTranslatedShapeSettings(Vec3(0, dummyCc.capsule_half_height() + dummyCc.capsule_radius(), 0), Quat::sIdentity(), chShapeCenterAnchor).Create().Get();
        Ref<CharacterVirtualSettings> settings = new CharacterVirtualSettings();
        settings->mMaxSlopeAngle = sMaxSlopeAngle;
        settings->mMaxStrength = sMaxStrength;
        settings->mShape = chShape;
        settings->mBackFaceMode = sBackFaceMode;
        settings->mCharacterPadding = sCharacterPadding;
        settings->mPenetrationRecoverySpeed = sPenetrationRecoverySpeed;
        settings->mPredictiveContactDistance = sPredictiveContactDistance;
        settings->mSupportingVolume = Plane(Vec3::sAxisY(), -dummyCc.capsule_radius()); // Accept contacts that touch the lower sphere of the capsule
        settings->mEnhancedInternalEdgeRemoval = sEnhancedInternalEdgeRemoval;
        settings->mInnerBodyShape = chShape;
        settings->mInnerBodyLayer = MyObjectLayers::MOVING;

        // Create character
        chCollider = new CharacterVirtual(settings, initialPos, Quat::sIdentity(), 0, phySys);
        //chCollider->SetKinematic();
        //chCollider->SetVelocity();
        auto [it2, res2] = cachedChColliders.try_emplace(capsuleKey, chCollider);
        if (!res2) {
            throw std::runtime_error("[_getOrCreateCachedCharacterCollider] failed to insert into preallocatedChColliders");
        }
    } else {
        chCollider = itChCollider->second;
        chCollider->SetPosition(initialPos);
        //chCollider->SetKinematic();
        //chCollider->SetVelocity();
        auto innerBodyID = chCollider->GetInnerBodyID();
        phySys->GetBodyInterface().AddBody(innerBodyID, EActivation::Activate);
    }
    return chCollider;
}

void BaseBattle::Step(int fromRdfId, int toRdfId, TempAllocator* tempAllocator) {
    for (int rdfId = fromRdfId; rdfId < toRdfId; rdfId++) {
        auto rdf = rdfBuffer.GetByFrameId(rdfId);
        for (int i = 0; i < playersCnt; i++) {
            auto player = rdf->players_arr(i);
            auto chCollider = getOrCreateCachedCharacterCollider(&player, &dummyCc);
            dynamicBodyIDs.push_back(chCollider->GetInnerBodyID());
        }
        phySys->Update(ESTIMATED_SECONDS_PER_FRAME, 1, tempAllocator, jobSys);
        // TODO: Update nextRdf by (rdf, phySys current body states of "dynamicBodyIDs"), then set back into "rdfBuffer" for rendering.
        phySys->GetBodyInterface().RemoveBodies(dynamicBodyIDs.data(), dynamicBodyIDs.size());
        dynamicBodyIDs.clear();
    }
}

void BaseBattle::elapse1RdfForChd(CharacterDownsync* chd) {
    
}
