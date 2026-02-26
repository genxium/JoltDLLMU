#include "BaseBattle.h"
#include "CharacterCollideShapeCollector.h"
#include "NpcReactionConsts.h"

#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexShape.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>

#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

#include <Jolt/Physics/Constraints/SliderConstraint.h>

// STL includes
#include <cstdarg>
#include <thread>
#include <string>
#include <climits> // Required for "INT_MAX", "UINT_MAX" and "FLT_MAX"
#include <cmath> // Required for "ceil()"
#include <queue>

#ifndef NDEBUG
#include "DebugLog.h"
#endif

using namespace jtshared;
using namespace JPH;

const float  cUpRotationX = 0;
const float  cUpRotationZ = 0;
const float  cMaxSlopeAngle = DegreesToRadians(45.0f);
const float  cMaxStrength = 100.0f;
const float  cCharacterPadding = 0.02f;
const float  cPenetrationRecoveryspeed = 1.0f;
const float  cPredictiveContactDistance = 0.1f;
const bool   cEnableWalkStairs = true;
const bool   cEnableStickToFloor = true;
const bool   cEnhancedInternalEdgeRemoval = false;
const bool   cCreateInnerBody = false;
const bool   cPlayerCanPushOtherCharacters = true;
const bool   cOtherCharactersCanPushPlayer = true;

const EBackFaceMode cBackFaceMode = EBackFaceMode::CollideWithBackFaces;
static CH_CACHE_KEY_T chCacheKeyHolder = { 0, 0 };
static BL_CACHE_KEY_T blCacheKeyHolder = { 0, 0 };
static TP_CACHE_KEY_T tpCacheKeyHolder(cDefaultTpHalfLength, cDefaultTpHalfLength, EMotionType::Dynamic, false, MyObjectLayers::MOVING);
static TR_CACHE_KEY_T trCacheKeyHolder = { 0, 0 };

BaseBattle::BaseBattle(int renderBufferSize, int inputBufferSize, TempAllocator* inGlobalTempAllocator) : rdfBuffer(renderBufferSize, &pbRdfAllocator, BaseBattle::ArenaAllocRdf, BaseBattle::ArenaFreeRdf), ifdBuffer(inputBufferSize, &pbRdfAllocator, BaseBattle::ArenaAllocIfd, BaseBattle::ArenaFreeIfd), frameLogBuffer((renderBufferSize << 1), &pbTempAllocator, BaseBattle::ArenaAllocFrameLog, BaseBattle::ArenaFreeFrameLog), globalTempAllocator(inGlobalTempAllocator), defaultBplf(ovbLayerFilter, MyObjectLayers::MOVING), defaultOlf(ovoLayerFilter, MyObjectLayers::MOVING), collisionUdHolderStockCache(256, 16), inputInducedMotionStockCache(256) {
    inactiveJoinMask = 0u;
    battleDurationFrames = 0;

    ////////////////////////////////////////////// 2
    bodyIDsToClear.reserve(cMaxBodies);
    bodyIDsToAdd.reserve(cMaxBodies);
    bodyIDsToActivate.reserve(cMaxBodies);

    allConfirmedMask = 0u;
    playersCnt = 0;
    safeDeactiviatedPosition = Vec3::sZero();
    
    phySys = nullptr;
    bi = nullptr;
    biNoLock = nullptr;
    jobSys = nullptr;
    blStockCache = nullptr;
    tpDynamicStockCache = nullptr;
    tpKinematicStockCache = nullptr;
    tpObsIfaceStockCache = nullptr;
    tpHelperStockCache = nullptr;
    trStockCache = nullptr;
}

BaseBattle::~BaseBattle() {
    Clear();
    delete jobSys;
    jobSys = nullptr;
    deallocPhySys();
    // "pbRdfAllocator" will be de-scoped and all memory it occupied will be freed automatically
}

// Backend & Frontend shared functions
int BaseBattle::moveForwardLastConsecutivelyAllConfirmedIfdId(int proposedIfdEdFrameId, uint64_t skippableJoinMask) {
    // [WARNING/BACKEND] This function MUST BE called while "inputBufferLock" is locked!

    int oldLcacIfdId = lcacIfdId;
    int proposedIfdStFrameId = lcacIfdId + 1;
    if (proposedIfdStFrameId >= proposedIfdEdFrameId) {
        return oldLcacIfdId;
    }
    if (proposedIfdStFrameId < ifdBuffer.StFrameId) {
        proposedIfdStFrameId = ifdBuffer.StFrameId;
    }
    for (int inputFrameId = proposedIfdStFrameId; inputFrameId < proposedIfdEdFrameId; inputFrameId++) {
        InputFrameDownsync* ifd = ifdBuffer.GetByFrameId(inputFrameId);

        if (allConfirmedMask != (ifd->confirmed_list() | skippableJoinMask)) {
            break;
        }

        ifd->set_confirmed_list(allConfirmedMask);
        if (lcacIfdId < inputFrameId) {
            // Such that "lastConsecutivelyAllConfirmedIfdId" is monotonic.
            lcacIfdId = inputFrameId;
        }
    }

    return oldLcacIfdId;
}

CH_COLLIDER_T* BaseBattle::getOrCreateCachedPlayerCollider_NotThreadSafe(const uint64_t ud, const PlayerCharacterDownsync& currPlayer, const CharacterConfig* cc, PlayerCharacterDownsync* nextPlayer) {
    JPH_ASSERT(0 != ud);
    float capsuleRadius = 0, capsuleHalfHeight = 0;
    const CharacterDownsync& chd = currPlayer.chd();
    calcChdShape(chd.ch_state(), cc, capsuleRadius, capsuleHalfHeight);
    Vec3 newPos(chd.x(), chd.y(), chd.z());
    Quat newRot(chd.q_x(), chd.q_y(), chd.q_z(), chd.q_w());
    auto res = getOrCreateCachedCharacterCollider_NotThreadSafe(ud, cc, capsuleRadius, capsuleHalfHeight, newPos, newRot);
    transientUdToCurrPlayer[ud] = &currPlayer;
    if (nullptr != nextPlayer) {
        transientUdToNextPlayer[ud] = nextPlayer;
    }
    return res;
}

CH_COLLIDER_T* BaseBattle::getOrCreateCachedNpcCollider_NotThreadSafe(const uint64_t ud, const NpcCharacterDownsync& currNpc, const CharacterConfig* cc, NpcCharacterDownsync* nextNpc) {
    JPH_ASSERT(0 != ud);
    float capsuleRadius = 0, capsuleHalfHeight = 0;
    const CharacterDownsync& chd = currNpc.chd();
    calcChdShape(currNpc.chd().ch_state(), cc, capsuleRadius, capsuleHalfHeight);
    Vec3 newPos(chd.x(), chd.y(), chd.z());
    Quat newRot(chd.q_x(), chd.q_y(), chd.q_z(), chd.q_w());
    auto res = getOrCreateCachedCharacterCollider_NotThreadSafe(ud, cc, capsuleRadius, capsuleHalfHeight, newPos, newRot);
    transientUdToCurrNpc[ud] = &currNpc;
    if (nullptr != nextNpc) {
        transientUdToNextNpc[ud] = nextNpc;
    }
    return res;
}

CH_COLLIDER_T* BaseBattle::getOrCreateCachedCharacterCollider_NotThreadSafe(const uint64_t ud, const CharacterConfig* cc, const float newRadius, const float newHalfHeight, const Vec3Arg& newPos, const QuatArg& newRot) {
    JPH_ASSERT(0 != ud);
    calcChCacheKey(cc, chCacheKeyHolder);
    CH_COLLIDER_T* chCollider = nullptr;
    auto it = cachedChColliders.find(chCacheKeyHolder);
    if (it == cachedChColliders.end()) {
        chCollider = createDefaultCharacterCollider(cc, newPos, newRot, ud, biNoLock);
    } else {
        auto& q = it->second;
        if (q.empty()) {
            chCollider = createDefaultCharacterCollider(cc, newPos, newRot, ud, biNoLock);
        } else {
            chCollider = q.back();
            q.pop_back();
        }
    }

    auto chBodyID = chCollider->GetBodyID();
    const RotatedTranslatedShape* shape = static_cast<const RotatedTranslatedShape*>(chCollider->GetShape());
    const CapsuleShape* innerShape = static_cast<const CapsuleShape*>(shape->GetInnerShape());
    JPH_ASSERT(nullptr != innerShape);
    if (innerShape->GetRadius() != newRadius || innerShape->GetHalfHeightOfCylinder() != newHalfHeight) {
        /*
        [WARNING]

        The feasibility of this hack is based on 3 facts.
        1. There's no shared "Shape" instance between "CH_COLLIDER_T" instances (and no shared "RotatedTranslatedShape::mInnerShape" either).
        2. This function "getOrCreateCachedCharacterCollider_NotThreadSafe" is only used in a single-threaded context (i.e. as a preparation before the multi-threaded "PhysicsSystem::Update").
        3. Operator "=" for "RefConst<Shape>" would NOT call "Release()" when the new pointer address is the same as the old one.
        */
        const Vec3Arg previousShapeCom = shape->GetCenterOfMass();

        int oldShapeRefCnt = shape->GetRefCount();
        int oldInnerShapeRefCnt = innerShape->GetRefCount();

        void* newInnerShapeBuffer = (void*)innerShape;
        CapsuleShape* newInnerShape = new (newInnerShapeBuffer) CapsuleShape(newHalfHeight, newRadius);
        newInnerShape->SetDensity(cDefaultChDensity);

        void* newShapeBuffer = (void*)shape;
        RotatedTranslatedShape* newShape = new (newShapeBuffer) RotatedTranslatedShape(Vec3(0, newHalfHeight + newRadius, 0), Quat::sIdentity(), newInnerShape);
        JPH_ASSERT(nullptr != newShape);
        int newInnerShapeRefCnt = nullptr == newShape->GetInnerShape() ? 0 : newShape->GetInnerShape()->GetRefCount();
        JPH_ASSERT(oldInnerShapeRefCnt == newInnerShapeRefCnt);

        /* [WARNING] 
        I've read the implementation of "JPH::Character::SetShape(...)" and carefully chosen to replace it with the following calls instead.
        
        Kindly note that the heap-memory of BOTH "chCollider->GetShape()" and "chBody->GetShape()" have already been refreshed by the placement-new of "newShape", hence we've already updated "chCollider->mShape" and "chBody->mShape" by far -- when assigned in [Character::Character](https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Physics/Character/Character.cpp#L44) -> [BodyManager::AllocateBody(const BodyCreationSettings& settings)](https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Physics/Body/BodyManager.cpp#L197), the "RefConst<Shape> Character.mShape" is shared to "RefConst<Shape> Body.mShape".
        */
        biNoLock->NotifyShapeChanged(chBodyID, previousShapeCom, true, EActivation::DontActivate); // See comments in "getOrCreateCachedBulletCollider_NotThreadSafe". 

        while (newShape->GetRefCount() < oldShapeRefCnt) newShape->AddRef();
        while (newShape->GetRefCount() > oldShapeRefCnt) newShape->Release();
        int newShapeRefCnt = newShape->GetRefCount();
        JPH_ASSERT(oldShapeRefCnt == newShapeRefCnt);
    } 

    biNoLock->SetPositionAndRotation(chBodyID, newPos, newRot, EActivation::DontActivate); // See comments in "getOrCreateCachedBulletCollider_NotThreadSafe".

    // must be active when called by "getOrCreateCachedCharacterCollider_NotThreadSafe"
    transientUdToChCollider[ud] = chCollider;
    activeChColliders.push_back(chCollider);
    biNoLock->SetUserData(chBodyID, ud);

    return chCollider;
}

BL_COLLIDER_T* BaseBattle::getOrCreateCachedBulletCollider_NotThreadSafe(const uint64_t ud, const float immediateBoxHalfSizeX, const float immediateBoxHalfSizeY, const BulletType blType, const Vec3Arg& newPos, const QuatArg& newRot) {
    calcBlCacheKey(immediateBoxHalfSizeX, immediateBoxHalfSizeY, blCacheKeyHolder);
    EMotionType immediateMotionType = calcBlMotionType(blType);
    bool immediateIsSensor = calcBlIsSensor(blType);
    Vec3 newHalfExtent = Vec3(immediateBoxHalfSizeX, immediateBoxHalfSizeY, cDefaultHalfThickness);
    float newConvexRadius = 0;
    BL_COLLIDER_T* blCollider = nullptr;
    auto it = cachedBlColliders.find(blCacheKeyHolder);
    if (it == cachedBlColliders.end() || it->second.empty()) {
        if (blStockCache->empty()) {
            blCollider = createDefaultBulletCollider(immediateBoxHalfSizeX, immediateBoxHalfSizeY, newConvexRadius, immediateMotionType, immediateIsSensor, newPos, newRot, biNoLock);
            JPH_ASSERT(nullptr != blCollider);
        } else {
            blCollider = blStockCache->back();
            blStockCache->pop_back();
            JPH_ASSERT(nullptr != blCollider);
        }
    } else {
        auto& q = it->second;
        JPH_ASSERT(!q.empty());
        blCollider = q.back();
        q.pop_back();
        JPH_ASSERT(nullptr != blCollider);
    }

    const BodyID& bodyID = blCollider->GetID();
    const BoxShape* shape = static_cast<const BoxShape*>(blCollider->GetShape());
    auto existingHalfExtent = shape->GetHalfExtent();
    auto existingConvexRadius = shape->GetConvexRadius();
    auto existingIsSensor = blCollider->IsSensor();
    auto existingMotionType = blCollider->GetMotionType();
    if (existingHalfExtent != newHalfExtent || existingConvexRadius != newConvexRadius || existingMotionType != immediateMotionType || existingIsSensor != immediateIsSensor) {
        const Vec3Arg previousShapeCom = shape->GetCenterOfMass();
        int oldShapeRefCnt = shape->GetRefCount();
        
        void* newShapeBuffer = (void*)shape;
        BoxShape* newShape = new (newShapeBuffer) BoxShape(newHalfExtent, newConvexRadius);
        newShape->SetDensity(cDefaultBlDensity);

        blCollider->SetIsSensor(immediateIsSensor);
        blCollider->SetMotionType(immediateMotionType);

        /*
        [WARNING] 
        After the placement-new of "newShape" above, "Body::mShape" is effectively updated, now we just have to call 
        - "UpdateCenterOfMassInternal", and 
        - "CalculateWorldSpaceBoundsInternal", and 
        - "BroadPhase::NotifyBodiesAABBChanged" (this is necessary because we're only deactivating/activating cached bodies in "CalcSingleStep", i.e. NOT removing/adding), and   
        - "BodyManager::InvalidateContactCache", with a slight loss of performance in "BodyManager::InvalidateContactCache" because of the use of unnecessary "BodyManager.mBodiesCacheInvalidMutex" in my case.

        Moreover, [the only spot "Body::IsCollisionCacheInvalid()" being used in Jolt Physics engine is together with "PhysicsSettings.mUseBodyPairContactCache"](https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Physics/PhysicsSystem.cpp#L1016).
        */
        biNoLock->NotifyShapeChanged(bodyID, previousShapeCom, true, EActivation::DontActivate);
        /*
        [REMINDER] 

        The following calls are intentionally avoided.
        - "BodyInterface::SetShape(...)"/"Body::SetShapeInternal(...)" because the placement-new of "newShape" has already updated "blCollider->mShape", and "BodyInterface::NotifyShapeChanged" has taken care of the rest.
        */

        while (newShape->GetRefCount() < oldShapeRefCnt) newShape->AddRef();
        while (newShape->GetRefCount() > oldShapeRefCnt) newShape->Release();
        int newShapeRefCnt = newShape->GetRefCount();
        JPH_ASSERT(oldShapeRefCnt == newShapeRefCnt);
    }

    biNoLock->SetPositionAndRotation(bodyID, newPos, newRot, EActivation::DontActivate); // Will call "BroadPhase::NotifyBodiesAABBChanged"

    transientUdToBodyID[ud] = &bodyID;
    blCollider->SetUserData(ud);

    // must be active when called by "getOrCreateCachedBulletCollider_NotThreadSafe"
    activeBlColliders.push_back(blCollider);

    return blCollider;
}

TP_COLLIDER_T* BaseBattle::getOrCreateCachedTrapCollider_NotThreadSafe(uint64_t ud, const float immediateBoxHalfSizeX, const float immediateBoxHalfSizeY, const TrapConfig* tpConfig, const TrapConfigFromTiled* tpConfigFromTile, const bool forConstraintHelperBody, const bool forConstraintObsIfaceBody, const Vec3Arg& newPos, const QuatArg& newRot) {
    // [REMINDER] For a TwoBodyConstraint, "IsActive()" requires at least one of the bodies to be "Dynamic".
    TP_COLLIDER_Q* theStockCache = nullptr;
    EMotionType immediateMotionType;
    ObjectLayer immediateObjectLayer; 
    if (forConstraintHelperBody) {
        immediateMotionType = EMotionType::Static;
        immediateObjectLayer = MyObjectLayers::TRAP_HELPER; 
        theStockCache = tpHelperStockCache;
    } else if (forConstraintObsIfaceBody) {
        immediateMotionType = EMotionType::Dynamic;
        immediateObjectLayer = MyObjectLayers::TRAP_OBSTACLE_INTERFACE; 
        theStockCache = tpObsIfaceStockCache;
    } else {
        if (tpConfig->use_kinematic()) {
            immediateMotionType = EMotionType::Kinematic;
            immediateObjectLayer = MyObjectLayers::MOVING; 
            theStockCache = tpKinematicStockCache;
        } else {
            immediateMotionType = EMotionType::Dynamic;
            immediateObjectLayer = MyObjectLayers::MOVING; 
            theStockCache = tpDynamicStockCache;
        }
    }

    bool immediateIsSensor = forConstraintHelperBody;
    calcTpCacheKey(immediateBoxHalfSizeX, immediateBoxHalfSizeY, immediateMotionType, immediateIsSensor, immediateObjectLayer, tpCacheKeyHolder);
    Vec3 newHalfExtent = Vec3(immediateBoxHalfSizeX, immediateBoxHalfSizeY, cDefaultHalfThickness);
    float newConvexRadius = 0;
    TP_COLLIDER_T* tpCollider = nullptr;
    auto it = cachedTpColliders.find(tpCacheKeyHolder);
    if (it == cachedTpColliders.end() || it->second.empty()) {
        if (theStockCache->empty()) {
            tpCollider = createDefaultTrapCollider(newHalfExtent, newPos, newRot, newConvexRadius, immediateMotionType, immediateIsSensor, immediateObjectLayer, biNoLock);
            JPH_ASSERT(nullptr != tpCollider);
        } else {
            tpCollider = theStockCache->back();
            theStockCache->pop_back();
            JPH_ASSERT(nullptr != tpCollider);
        }
    } else {
        auto& q = it->second;
        JPH_ASSERT(!q.empty());
        tpCollider = q.back();
        q.pop_back();
        JPH_ASSERT(nullptr != tpCollider);
    }

    const BodyID& bodyID = tpCollider->GetID();
    const BoxShape* shape = static_cast<const BoxShape*>(tpCollider->GetShape());
    auto existingHalfExtent = shape->GetHalfExtent();
    auto existingConvexRadius = shape->GetConvexRadius();
    if (existingHalfExtent != newHalfExtent || existingConvexRadius != newConvexRadius) {
        Vec3Arg previousShapeCom = shape->GetCenterOfMass();

        int oldShapeRefCnt = shape->GetRefCount();
        
        void* newShapeBuffer = (void*)shape;
        BoxShape* newShape = new (newShapeBuffer) BoxShape(newHalfExtent, newConvexRadius);
        newShape->SetDensity(cDefaultTpDensity);

        biNoLock->NotifyShapeChanged(bodyID, previousShapeCom, true, EActivation::DontActivate); // See comments in "getOrCreateCachedBulletCollider_NotThreadSafe". 
        while (newShape->GetRefCount() < oldShapeRefCnt) newShape->AddRef();
        while (newShape->GetRefCount() > oldShapeRefCnt) newShape->Release();
        int newShapeRefCnt = newShape->GetRefCount();
        JPH_ASSERT(oldShapeRefCnt == newShapeRefCnt);
    }

    biNoLock->SetPositionAndRotation(bodyID, newPos, newRot, EActivation::DontActivate); 

    if (forConstraintHelperBody) {
        transientUdToConstraintHelperBodyID[ud] = &bodyID;
        transientUdToConstraintHelperBody[ud] = tpCollider;
    } else if (forConstraintObsIfaceBody) {
        transientUdToConstraintObsIfaceBodyID[ud] = &bodyID;
        transientUdToConstraintObsIfaceBody[ud] = tpCollider;
    } else {
        transientUdToBodyID[ud] = &bodyID;
        transientUdToTpCollider[ud] = tpCollider;
    }

    tpCollider->SetUserData(ud);

    // must be active when called by "getOrCreateCachedTrapCollider_NotThreadSafe"
    activeTpColliders.push_back(tpCollider);

    return tpCollider;
}

TR_COLLIDER_T* BaseBattle::getOrCreateCachedTriggerCollider_NotThreadSafe(const uint64_t ud, const float immediateBoxHalfSizeX, const float immediateBoxHalfSizeY, const Vec3Arg& newPos, const QuatArg& newRot) {
    calcTrCacheKey(immediateBoxHalfSizeX, immediateBoxHalfSizeY, trCacheKeyHolder);
    Vec3 newHalfExtent = Vec3(immediateBoxHalfSizeX, immediateBoxHalfSizeY, cDefaultHalfThickness);
    float newConvexRadius = 0;
    TR_COLLIDER_T* trCollider = nullptr;
    auto it = cachedTrColliders.find(trCacheKeyHolder);
    if (it == cachedTrColliders.end() || it->second.empty()) {
        if (trStockCache->empty()) {
            trCollider = createDefaultTriggerCollider(immediateBoxHalfSizeX, immediateBoxHalfSizeY, newConvexRadius, newPos, newRot, biNoLock);
            JPH_ASSERT(nullptr != trCollider);
        } else {
            trCollider = trStockCache->back();
            trStockCache->pop_back();
            JPH_ASSERT(nullptr != trCollider);
        }
    } else {
        auto& q = it->second;
        JPH_ASSERT(!q.empty());
        trCollider = q.back();
        q.pop_back();
        JPH_ASSERT(nullptr != trCollider);
    }

    const BodyID& bodyID = trCollider->GetID();
    const BoxShape* shape = static_cast<const BoxShape*>(trCollider->GetShape());
    auto existingHalfExtent = shape->GetHalfExtent();
    auto existingConvexRadius = shape->GetConvexRadius();
    auto existingIsSensor = trCollider->IsSensor();
    auto existingMotionType = trCollider->GetMotionType();
    if (existingHalfExtent != newHalfExtent || existingConvexRadius != newConvexRadius) {
        const Vec3Arg previousShapeCom = shape->GetCenterOfMass();
        int oldShapeRefCnt = shape->GetRefCount();
        
        void* newShapeBuffer = (void*)shape;
        BoxShape* newShape = new (newShapeBuffer) BoxShape(newHalfExtent, newConvexRadius);

        biNoLock->NotifyShapeChanged(bodyID, previousShapeCom, true, EActivation::DontActivate);

        while (newShape->GetRefCount() < oldShapeRefCnt) newShape->AddRef();
        while (newShape->GetRefCount() > oldShapeRefCnt) newShape->Release();
        int newShapeRefCnt = newShape->GetRefCount();
        JPH_ASSERT(oldShapeRefCnt == newShapeRefCnt);
    }

    biNoLock->SetPositionAndRotation(bodyID, newPos, newRot, EActivation::DontActivate); // Will call "BroadPhase::NotifyBodiesAABBChanged"

    transientUdToBodyID[ud] = &bodyID;
    trCollider->SetUserData(ud);

    // must be active when called by "getOrCreateCachedTriggerCollider_NotThreadSafe"
    activeTrColliders.push_back(trCollider);

    return trCollider;
}

NON_CONTACT_CONSTRAINT_T* BaseBattle::getOrCreateCachedNonContactConstraint_NotThreadSafe(const EConstraintType nonContactConstraintType, const EConstraintSubType nonContactConstraintSubType, Body* body1, Body* body2, JPH::ConstraintSettings* inConstraintSettings) {
    JPH_ASSERT(nullptr != body1);
    uint64_t ud1 = body1->GetUserData();
    uint64_t ud2 = nullptr == body2 ? 0 : body2->GetUserData();
    NON_CONTACT_CONSTRAINT_CACHE_KEY_T k(nonContactConstraintType, nonContactConstraintSubType, ud1, ud2);
    NON_CONTACT_CONSTRAINT_T* nonContactConstraint = nullptr;
    auto it = cachedNonContactConstraints.find(k);
    if (it == cachedNonContactConstraints.end() || it->second.empty()) {
        nonContactConstraint = createDefaultNonContactConstraint(nonContactConstraintType, nonContactConstraintSubType, body1, body2, inConstraintSettings);
        JPH_ASSERT(nullptr != nonContactConstraint);
    } else {
        auto& q = it->second;
        JPH_ASSERT(!q.empty());
        nonContactConstraint = q.back();
        q.pop_back();
        JPH_ASSERT(nullptr != nonContactConstraint);
    }
    
    int oldCRefCnt = nonContactConstraint->c->GetRefCount();
    const BodyID& newBodyID1 = body1->GetID();
    switch (nonContactConstraintType) {
    case EConstraintType::TwoBodyConstraint: {
        JPH_ASSERT(nullptr != body2);
        const BodyID& newBodyID2 = body2->GetID();
        switch (nonContactConstraintSubType) {
        case EConstraintSubType::Slider: {
            Constraint* cachedC = nonContactConstraint->c;
            SliderConstraintSettings* castedExistingSliderSettings = static_cast<SliderConstraintSettings*>(inConstraintSettings);
            void* newNonContactConstraintBuffer = (void*)cachedC;
            SliderConstraint* newSliderConstraint = new (newNonContactConstraintBuffer) SliderConstraint(*body1, *body2, *(castedExistingSliderSettings));
            // [WARNING] No need to call ["SliderConstraint::NotifyShapeChanged(...)"](https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Physics/Constraints/Constraint.h#L161 ) here because "getOrCreateCachedNonContactConstraint_NotThreadSafe" is always called after the creation and update of "Body"s.
            nonContactConstraint->c = newSliderConstraint;
            break;
        }
        default:
            break;
        }
        break;
    }
    default:
        break;
    }

    while (nonContactConstraint->c->GetRefCount() < oldCRefCnt) nonContactConstraint->c->AddRef();
    while (nonContactConstraint->c->GetRefCount() > oldCRefCnt) nonContactConstraint->c->Release();

    nonContactConstraint->ud1 = ud1;
    nonContactConstraint->ud2 = ud2;

    // must be active when called by "getOrCreateCachedNonContactConstraint_NotThreadSafe"
    activeNonContactConstraints.push_back(nonContactConstraint);

    return nonContactConstraint;
}

void BaseBattle::processWallGrabbingPostPhysicsUpdate(const int currRdfId, const CharacterDownsync& currChd, CharacterDownsync* nextChd, const CharacterConfig* cc, const CH_COLLIDER_T* cv, bool inJumpStartupOrJustEnded) {
    switch (nextChd->ch_state()) {
    case Idle1:
    case Walking:
    case InAirIdle1NoJump:
    case InAirIdle1ByJump:
    case InAirIdle2ByJump:
    case InAirIdle1ByWallJump: {
        bool hasBeenOnWall = onWallSet.count(currChd.ch_state());
        // [WARNING] The "magic_frames_to_be_on_wall()" allows "InAirIdle1ByWallJump" to leave the current wall within a reasonable count of rdf count, instead of always forcing "InAirIdle1ByWallJump" to immediately stick back to the wall!
        bool enoughFramesInChState = InAirIdle2ByJump == currChd.ch_state()
            ?
            globalPrimitiveConsts->magic_frames_to_be_on_wall_air_jump() < currChd.frames_in_ch_state()
            :
            globalPrimitiveConsts->magic_frames_to_be_on_wall() < currChd.frames_in_ch_state();

        if (!inJumpStartupOrJustEnded && enoughFramesInChState) {
            nextChd->set_ch_state(OnWallIdle1);
        } else if (!inJumpStartupOrJustEnded && hasBeenOnWall) {
            nextChd->set_ch_state(currChd.ch_state());
        }

        break;
    }
    }
}

bool BaseBattle::transitToDying(const int currRdfId, const CharacterDownsync& currChd, const bool cvInAir, CharacterDownsync* nextChd) {
    if (CharacterState::Dying == currChd.ch_state()) return false;
    nextChd->set_ch_state(CharacterState::Dying);
    nextChd->set_frames_in_ch_state(0);
    nextChd->set_frames_to_recover(globalPrimitiveConsts->dying_frames_to_recover());
    nextChd->set_frames_invinsible(0);
    nextChd->set_hp(0);
    if (!cvInAir) {
        nextChd->set_vel_x(currChd.ground_vel_x());
        nextChd->set_vel_y(currChd.ground_vel_y());
    }
    return true;
}

bool BaseBattle::transitToDying(const int currRdfId, const PlayerCharacterDownsync& currPlayer, const bool cvInAir, PlayerCharacterDownsync* nextPlayer) {
    auto& currChd = currPlayer.chd();
    auto nextChd = nextPlayer->mutable_chd();
#ifndef NDEBUG
    std::ostringstream oss;
    oss << "@currRdfId=" << currRdfId << ", player joinIndex=" << currPlayer.join_index() << " is dead due to fallen death with nextChd->position=(" << nextChd->x() << "," << nextChd->y() << "), orig next_ch_state=" << nextChd->ch_state() << ", orig next_frames_in_ch_state=" << nextChd->frames_in_ch_state() << ", fallenDeathHeight=" << fallenDeathHeight << ", will transit into Dying";
    Debug::Log(oss.str(), DColor::Orange);
#endif
    bool res = transitToDying(currRdfId, currChd, cvInAir, nextChd);
    return res;
}

bool BaseBattle::transitToDying(const int currRdfId, const NpcCharacterDownsync& currNpc, const bool cvInAir, NpcCharacterDownsync* nextNpc) {
    auto& currChd = currNpc.chd();
    auto nextChd = nextNpc->mutable_chd();
    bool res = transitToDying(currRdfId, currChd, cvInAir, nextChd);
    // For NPC should also reset patrol book-keepers
    nextNpc->set_frames_in_patrol_cue(0);
    return res;
}

void BaseBattle::updateChColliderBeforePhysicsUpdate_ThreadSafe(uint64_t ud, CH_COLLIDER_T* chCollider, const float dt, const CharacterDownsync& currChd, const InputInducedMotion* inInputInducedMotion, const bool inGravityDirty, const bool inFrictionDirty) {
        /*
        From the source codes of [JPH::Body](https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Physics/Body/Body.h) and [MotionPropertis](https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Physics/Body/MotionProperties.h#L148) it seems like "accelerations" are only calculated during stepping, not cached.
        */
        auto bodyID = chCollider->GetBodyID();
        const CharacterConfig* cc = getCc(currChd.species_id());
        if (!inGravityDirty) {
            if (onWallSet.count(currChd.ch_state())) {
                bi->SetGravityFactor(bodyID, 0);
            } else if (currChd.omit_gravity() || cc->omit_gravity()) {
                bi->SetGravityFactor(bodyID, 0);
            } else {
                if (currChd.btn_a_holding_rdf_cnt() > globalPrimitiveConsts->jump_holding_rdf_cnt_threshold_1()) {
                    bi->SetGravityFactor(bodyID, 0.75);
                } else {
                    bi->SetGravityFactor(bodyID, 1);
                }
            }
        }
        bi->AddForceAndTorque(bodyID, inInputInducedMotion->forceCOM, inInputInducedMotion->torqueCOM, EActivation::DontActivate);
        bi->SetLinearAndAngularVelocity(bodyID, inInputInducedMotion->velCOM, Vec3::sZero());
        // [REMINDER] "CharacterVirtual" maintains its own "mLinearVelocity" (https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Physics/Character/CharacterVirtual.h#L709) -- and experimentally setting velocity of its "mInnerBodyID" doesn't work (if "mInnerBodyID" was even set).
}

RenderFrame* BaseBattle::CalcSingleStep(const int currRdfId, int delayedIfdId, InputFrameDownsync* delayedIfd) {
    const RenderFrame* currRdf = rdfBuffer.GetByFrameId(currRdfId);
    if (nullptr == currRdf) return nullptr;
    RenderFrame* nextRdf = rdfBuffer.GetByFrameId(currRdfId + 1);
    if (!nextRdf) {
        nextRdf = rdfBuffer.DryPut();
    }

    CopyRdf(currRdf, nextRdf);
    nextRdf->set_id(currRdfId + 1);
    elapse1RdfForRdf(currRdfId, nextRdf);

    float dt = globalPrimitiveConsts->estimated_seconds_per_rdf();

    mNextRdfBulletIdCounter = nextRdf->bullet_id_counter();
    mNextRdfBulletCount = nextRdf->bullet_count();

    mNextRdfNpcIdCounter = nextRdf->npc_id_counter();
    mNextRdfNpcCount = nextRdf->npc_count();

    batchPutIntoPhySysFromCache(currRdfId, currRdf, nextRdf);

    StepResult* stepResult = nextRdf->mutable_prev_rdf_step_result();
    stepResult->Clear();

    for (int i = 0; i < currRdf->triggers_size(); i++) {
        const Trigger& currTrigger = currRdf->triggers(i);
        if (globalPrimitiveConsts->terminating_trigger_id() == currTrigger.id()) break;
        const uint32_t trt = currTrigger.trt();

        Trigger* nextTrigger = nextRdf->mutable_triggers(i);
        auto ud = calcUserData(currTrigger);
        if (globalPrimitiveConsts->trt_indi_wave_npc_spawner() == trt) {
            stepSingleIndiWaveNpcSpawner(currRdfId, currTrigger, nextTrigger, nextRdf, stepResult);
        } else {
            stepOtherSingleTriggerState(currRdfId, currTrigger, nextTrigger, nextRdf, stepResult);
        }
    }

    JobSystem::Barrier* prePhysicsUpdateMTBarrier = jobSys->CreateBarrier();
    for (int i = 0; i < playersCnt; i++) {
        uint64_t singleInput = delayedIfd->input_list(i);
        auto handle = jobSys->CreateJob("player-pre-physics-update", JPH::Color::sBlack, [currRdfId, i, currRdf, nextRdf, this, singleInput, dt]() {
            const PlayerCharacterDownsync& currPlayer = currRdf->players(i);
            auto ud = calcUserData(currPlayer);
            if (!transientUdToChCollider.count(ud)) return;
            if (!transientUdToInputInducedMotion.count(ud)) return;
            InputInducedMotion* inputInducedMotion = transientUdToInputInducedMotion.at(ud);
            CH_COLLIDER_T* chCollider = transientUdToChCollider.at(ud);
            PlayerCharacterDownsync* nextPlayer = nextRdf->mutable_players(i); // [WARNING] The indices of "currRdf->players" and "nextRdf->players" are ALWAYS FULLY ALIGNED.
            const CharacterDownsync& currChd = currPlayer.chd();
            CharacterDownsync* nextChd = nextPlayer->mutable_chd();
            inputInducedMotion->velCOM.Set(currChd.vel_x(), currChd.vel_y(), currChd.vel_z());
            bool gravityDirty = false, frictionDirty = false;
            if (!noOpSet.count(currChd.ch_state())) {
                const CharacterConfig* cc = getCc(currChd.species_id());
                if (onWallSet.count(currChd.ch_state())) {
                    inputInducedMotion->velCOM.SetY(cc->wall_slide_vel_y());
                }
                auto currChState = currChd.ch_state();
                bool currNotDashing = BaseBattleCollisionFilter::chIsNotDashing(currChd);
                bool currDashing = !currNotDashing;
                bool currWalking = walkingSet.count(currChState);
                bool currEffInAir = isEffInAir(currChd, currNotDashing);
                bool currOnWall = onWallSet.count(currChState);
                bool currCrouching = isCrouching(currChState, cc);
                bool currAtked = atkedSet.count(currChState);
                bool currInBlockStun = isInBlockStun(currChd);
                bool currParalyzed = false; // TODO

                int patternId = globalPrimitiveConsts->pattern_id_no_op();
                bool jumpedOrNot = false;
                bool slipJumpedOrNot = false;
                int effDx = 0, effDy = 0;
                InputFrameDecoded ifDecodedHolder;
                decodeInput(singleInput, &ifDecodedHolder);
                Quat currChdQ;
                Vec3 currChdFacing;
                BaseBattleCollisionFilter::calcChdFacing(currChd, currChdQ, currChdFacing);
                deriveCharacterOpPattern(currRdfId, currChd, currChdFacing, cc, nextChd, currEffInAir, currNotDashing, ifDecodedHolder, patternId, jumpedOrNot, slipJumpedOrNot, effDx, effDy);
                bool slowDownToAvoidOverlap = false;
                bool usedSkill = false;
                const RotatedTranslatedShape* shape = static_cast<const RotatedTranslatedShape*>(chCollider->GetShape());
                const MassProperties massProps = shape->GetMassProperties();
                processSingleCharacterInput(currRdfId, dt, patternId, jumpedOrNot, slipJumpedOrNot, effDx, effDy, slowDownToAvoidOverlap, currChd, massProps, currChdFacing, ud, currEffInAir, currCrouching, currOnWall, currDashing, currWalking, currInBlockStun, currAtked, currParalyzed, cc, nextChd, nextRdf, usedSkill, chCollider, inputInducedMotion, gravityDirty, frictionDirty);
            }

            updateChColliderBeforePhysicsUpdate_ThreadSafe(ud, chCollider, dt, currChd, inputInducedMotion, gravityDirty, frictionDirty); 
        }, 0);
        prePhysicsUpdateMTBarrier->AddJob(handle);
    }

    for (int i = 0; i < currRdf->npcs_size(); i++) {
        if (globalPrimitiveConsts->terminating_character_id() == currRdf->npcs(i).id()) break;
        auto handle = jobSys->CreateJob("npc-pre-physics-update", JPH::Color::sBlack, [currRdfId, i, currRdf, nextRdf, this, dt]() {
            const NpcCharacterDownsync& currNpc = currRdf->npcs(i);
            auto ud = calcUserData(currNpc);
            if (!transientUdToChCollider.count(ud)) return;
            if (!transientUdToInputInducedMotion.count(ud)) return;
            InputInducedMotion* inputInducedMotion = transientUdToInputInducedMotion.at(ud);
            CH_COLLIDER_T* chCollider = transientUdToChCollider.at(ud);
            NpcCharacterDownsync* nextNpc = nextRdf->mutable_npcs(i); // [WARNING] By reaching here, we haven't executed "leftShiftDeadNpcs", hence the indices of "currRdf->npcs" and "nextRdf->npcs" are FULLY ALIGNED.
            const CharacterDownsync& currChd = currNpc.chd();
            CharacterDownsync* nextChd = nextNpc->mutable_chd();
            inputInducedMotion->velCOM.Set(currChd.vel_x(), currChd.vel_y(), currChd.vel_z());
            bool gravityDirty = false, frictionDirty = false;
            if (!noOpSet.count(currChd.ch_state())) {
                const CharacterConfig* cc = getCc(currChd.species_id());
                auto currChState = currChd.ch_state();
                bool currNotDashing = BaseBattleCollisionFilter::chIsNotDashing(currChd);
                bool currDashing = !currNotDashing;
                bool currWalking = walkingSet.count(currChState);
                bool currEffInAir = isEffInAir(currChd, currNotDashing);
                bool currOnWall = onWallSet.count(currChState);
                bool currCrouching = isCrouching(currChState, cc);
                bool currAtked = atkedSet.count(currChState);
                bool currInBlockStun = isInBlockStun(currChd);
                bool currParalyzed = false; // TODO

                int patternId = globalPrimitiveConsts->pattern_id_no_op();
                bool jumpedOrNot = false;
                bool slipJumpedOrNot = false;
                int effDx = 0, effDy = 0;
                InputFrameDecoded ifDecodedHolder;
                uint64_t singleInput = nextNpc->cached_cue_cmd();
                decodeInput(singleInput, &ifDecodedHolder); 
                Quat currChdQ;
                Vec3 currChdFacing;
                BaseBattleCollisionFilter::calcChdFacing(currChd, currChdQ, currChdFacing);
                deriveCharacterOpPattern(currRdfId, currChd, currChdFacing, cc, nextChd, currEffInAir, currNotDashing, ifDecodedHolder, patternId, jumpedOrNot, slipJumpedOrNot, effDx, effDy);

                bool slowDownToAvoidOverlap = false; // TODO
                bool usedSkill = false;

                const RotatedTranslatedShape* shape = static_cast<const RotatedTranslatedShape*>(chCollider->GetShape());
                const MassProperties massProps = shape->GetMassProperties();

                processSingleCharacterInput(currRdfId, dt, patternId, jumpedOrNot, slipJumpedOrNot, effDx, effDy, slowDownToAvoidOverlap, currChd, massProps, currChdFacing, ud, currEffInAir, currCrouching, currOnWall, currDashing, currWalking, currInBlockStun, currAtked, currParalyzed, cc, nextChd, nextRdf, usedSkill, chCollider, inputInducedMotion, gravityDirty, frictionDirty);
                
                if (usedSkill) {
                    nextNpc->set_cached_cue_cmd(0);
                }
            }

            updateChColliderBeforePhysicsUpdate_ThreadSafe(ud, chCollider, dt, currChd, inputInducedMotion, gravityDirty, frictionDirty);

        }, 0);
        prePhysicsUpdateMTBarrier->AddJob(handle);
    }

    // Update positions and velocities of active bullets
    for (int i = 0; i < currRdf->bullets_size(); i++) {
        const Bullet& currBl = currRdf->bullets(i);
        if (globalPrimitiveConsts->terminating_bullet_id() == currBl.id()) break;
        auto handle = jobSys->CreateJob("bullet-pre-physics-update", JPH::Color::sBlack, [currRdfId, i, currRdf, nextRdf, this, dt]() {
            const Bullet& currBl = currRdf->bullets(i);
            Bullet* nextBl = nextRdf->mutable_bullets(i); // [WARNING] By reaching here, we haven't executed "leftShiftDeadBullets", hence the indices of "currRdf->bullets" and "nextRdf->bullets" are FULLY ALIGNED.
            auto ud = calcUserData(currBl);
            if (!transientUdToBodyID.count(ud)) return;
            const BodyID bodyID = *(transientUdToBodyID.at(ud));

            bi->SetLinearAndAngularVelocity(bodyID, Vec3(nextBl->vel_x(), nextBl->vel_y(), nextBl->vel_z()), Vec3::sZero());
            const Skill* skill = nullptr;
            const BulletConfig* blConfig = nullptr;
            FindBulletConfig(currBl.skill_id(), currBl.active_skill_hit(), skill, blConfig);
            switch (blConfig->b_type()) {
            case BulletType::Melee:
                bi->SetMotionQuality(bodyID, EMotionQuality::Discrete);
                break;
            default:
                bi->SetMotionQuality(bodyID, EMotionQuality::LinearCast);
                break;
            }
        }, 0);
        prePhysicsUpdateMTBarrier->AddJob(handle);
    }

    for (int i = 0; i < currRdf->dynamic_traps_size(); i++) {
        const Trap& currTp = currRdf->dynamic_traps(i);
        if (globalPrimitiveConsts->terminating_trap_id() == currTp.id()) break;
        auto handle = jobSys->CreateJob("dynamic-trap-pre-physics-update", JPH::Color::sBlack, [currRdfId, i, currRdf, nextRdf, this, dt]() {
            const Trap& currTp = currRdf->dynamic_traps(i);
            Trap* nextTp = nextRdf->mutable_dynamic_traps(i); // [WARNING] By reaching here, we haven't executed "leftShiftDeadTraps", hence the indices of "currRdf->dynamic_traps" and "nextRdf->dynamic_traps" are FULLY ALIGNED.
            auto ud = calcUserData(currTp);
            if (!transientUdToBodyID.count(ud)) return;
            const TrapConfig* tpConfig = nullptr;
            const TrapConfigFromTiled* tpConfigFromTile = nullptr;
            const uint32_t tpt = currTp.tpt();
            FindTrapConfig(tpt, currTp.id(), trapConfigFromTileDict, tpConfig, tpConfigFromTile);
            JPH_ASSERT(nullptr != tpConfig);
            const BodyID bodyID = *(transientUdToBodyID.at(ud));
            bi->SetLinearAndAngularVelocity(bodyID, Vec3(nextTp->vel_x(), nextTp->vel_y(), nextTp->vel_z()), Vec3::sZero());
            if (globalPrimitiveConsts->tpt_sliding_platform() == currTp.tpt()) {
                JPH_ASSERT(nullptr != tpConfigFromTile);
                const BodyID constraintHelperBodyID = *(transientUdToConstraintHelperBodyID.at(ud));
                const BodyID obsIfaceBodyID = *(transientUdToConstraintObsIfaceBodyID.at(ud));
                bi->SetLinearAndAngularVelocity(obsIfaceBodyID, Vec3(nextTp->vel_x(), nextTp->vel_y(), nextTp->vel_z()), Vec3::sZero());
            }
            }, 0);
        prePhysicsUpdateMTBarrier->AddJob(handle);
    }

    jobSys->WaitForJobs(prePhysicsUpdateMTBarrier);
    jobSys->DestroyBarrier(prePhysicsUpdateMTBarrier);

    /*
    [WARNING] Upon constructors of NON_CONTACT_CONSTRAINT_T classes, lots of member variable setups are done based on Body position, rotation and motion properties -- therefore it's best to setup NON_CONTACT_CONSTRAINT_T instances AFTER all the "bi->SetPositionAndRotation/SetLinearAndAngularVelocity" invocations within "prePhysicsUpdateMTBarrier". 
    */
    batchNonContactConstraintsSetupFromCache(currRdfId, currRdf, nextRdf);

    // [REMINDER] The "class CharacterVirtual" instances WOULDN'T participate in "phySys->Update(...)" IF they were NOT filled with valid "mInnerBodyID". See "RuleOfThumb.md" for details.
    phySys->Update(dt, 1, globalTempAllocator, jobSys);

    // [REMINDER] From now on, we can safely use "biNoLock" because there'd be NO USE of "bi->SetXxx(...)"!
    JobSystem::Barrier* postPhysicsUpdateMTBarrier = jobSys->CreateBarrier();
    const BaseBattle* battle = this;
    for (int i = 0; i < playersCnt; i++) {
        auto handle = jobSys->CreateJob("player-post-physics-update", JPH::Color::sBlack, [currRdfId, i, currRdf, nextRdf, this, dt]() {
            auto currPlayer = currRdf->players(i);
            auto nextPlayer = nextRdf->mutable_players(i); // [WARNING] The indices of "currRdf->players" and "nextRdf->players" are ALWAYS FULLY ALIGNED.
            const CharacterDownsync& currChd = currPlayer.chd();
            auto nextChd = nextPlayer->mutable_chd();

            auto ud = calcUserData(currPlayer);
            JPH_ASSERT(transientUdToChCollider.count(ud));

            const CharacterConfig* cc = getCc(currChd.species_id());
            bool groundBodyIsChCollider = false, isDead = false; 
            CH_COLLIDER_T* single = transientUdToChCollider.at(ud);

            bool currNotDashing = BaseBattleCollisionFilter::chIsNotDashing(currChd);
            bool currEffInAir = isEffInAir(currChd, currNotDashing);
            bool oldNextNotDashing = BaseBattleCollisionFilter::chIsNotDashing(*nextChd);
            bool oldNextEffInAir = isEffInAir(*nextChd, oldNextNotDashing);
            //bool isProactivelyJumping = proactiveJumpingSet.count(nextChd->ch_state());

            bool cvOnWall = false, cvSupported = false, cvInAir = true, inJumpStartupOrJustEnded = false; 
            CharacterBase::EGroundState cvGroundState = CharacterBase::EGroundState::InAir;
            InputInducedMotion* inputInducedMotion = transientUdToInputInducedMotion.at(ud);
            stepSingleChdState(currRdfId, currRdf, nextRdf, dt, ud, UDT_PLAYER, cc, single, currChd, nextChd, groundBodyIsChCollider, isDead, cvOnWall, cvSupported, cvInAir, inJumpStartupOrJustEnded, cvGroundState, inputInducedMotion);
            postStepSingleChdStateCorrection(currRdfId, UDT_PLAYER, ud, single, currChd, nextChd, cc, cvSupported, cvInAir, cvOnWall, currNotDashing, currEffInAir, oldNextNotDashing, oldNextEffInAir, inJumpStartupOrJustEnded, cvGroundState, inputInducedMotion);

            if (isDead) {
                if (CharacterState::Dying != nextChd->ch_state()) {
                    transitToDying(currRdfId, currPlayer, cvInAir, nextPlayer);
                } else if (globalPrimitiveConsts->dying_frames_to_recover() < nextChd->frames_in_ch_state()) {
        #ifndef NDEBUG
                    std::ostringstream oss;
                    oss << "@currRdfId=" << currRdfId << ", player joinIndex=" << currPlayer.join_index() << " reviving to nextChd->position=(" << currPlayer.revival_x() << "," << currPlayer.revival_y() << "), orig next_ch_state=" << nextChd->ch_state() << ", orig next_frames_in_ch_state=" << nextChd->frames_in_ch_state();
                    Debug::Log(oss.str(), DColor::Orange);
        #endif
                    nextChd->set_hp(cc->hp());
                    nextChd->set_mp(cc->mp());
                    nextChd->set_ch_state(CharacterState::Idle1);
                    nextChd->set_frames_in_ch_state(0);

                    nextChd->set_x(currPlayer.revival_x());
                    nextChd->set_y(currPlayer.revival_y());
                    nextChd->set_z(currPlayer.revival_z());

                    nextChd->set_q_x(currPlayer.revival_q_x());
                    nextChd->set_q_y(currPlayer.revival_q_y());
                    nextChd->set_q_z(currPlayer.revival_q_z());
                    nextChd->set_q_w(currPlayer.revival_q_w());

                    nextChd->set_aiming_q_x(0);
                    nextChd->set_aiming_q_y(0);
                    nextChd->set_aiming_q_z(0);
                    nextChd->set_aiming_q_w(1);

                    nextChd->set_new_birth_rdf_countdown(5);
                    nextChd->set_vel_x(currChd.ground_vel_x());
                    nextChd->set_vel_y(currChd.ground_vel_y());
                    nextChd->set_vel_z(currChd.ground_vel_z());

                    nextChd->set_ground_vel_x(currChd.ground_vel_x());
                    nextChd->set_ground_vel_y(currChd.ground_vel_y());
                    nextChd->set_ground_vel_z(currChd.ground_vel_z());

                    bool ivOverriddenByBattleSpecificConfigs = false; 
                    if (nullptr != battleSpecificConfig && 0 < battleSpecificConfig->character_overrides_size()) {
                        auto& characterOverrides = battleSpecificConfig->character_overrides(); 
                        if (characterOverrides.count(ud)) {
                            auto& characterOverride = characterOverrides.at(ud); 
                            if (0 < characterOverride.init_inventory_slots_size()) {
                                ivOverriddenByBattleSpecificConfigs = ResetInventory(cc, &characterOverride, nextChd); 
                            }
                        }
                    }

                    if (!ivOverriddenByBattleSpecificConfigs) {
                        ResetInventory(cc, nullptr, nextChd);
                    }
                }
            }
        }, 0);
        postPhysicsUpdateMTBarrier->AddJob(handle);
    }

    for (int i = 0; i < currRdf->npcs_size(); i++) {
        if (globalPrimitiveConsts->terminating_character_id() == currRdf->npcs(i).id()) break;
        auto handle = jobSys->CreateJob("npc-post-physics-update", JPH::Color::sBlack, [currRdfId, i, currRdf, nextRdf, this, dt]() {
            const NpcCharacterDownsync& currNpc = currRdf->npcs(i);
            auto nextNpc = nextRdf->mutable_npcs(i); // [WARNING] By reaching here, we haven't executed "leftShiftDeadNpcs", hence the indices of "currRdf->npcs" and "nextRdf->npcs" are FULLY ALIGNED.

            const CharacterDownsync& currChd = currNpc.chd();
            auto nextChd = nextNpc->mutable_chd();
            auto ud = calcUserData(currNpc);

            JPH_ASSERT(transientUdToChCollider.count(ud));
            const CharacterConfig* cc = getCc(currChd.species_id());
            bool groundBodyIsChCollider = false, isDead = false; 
            CH_COLLIDER_T* single = transientUdToChCollider.at(ud);
            const BodyID& selfNpcBodyID = single->GetBodyID();

            bool currNotDashing = BaseBattleCollisionFilter::chIsNotDashing(currChd);
            bool currEffInAir = isEffInAir(currChd, currNotDashing);
            bool oldNextNotDashing = BaseBattleCollisionFilter::chIsNotDashing(*nextChd);
            bool oldNextEffInAir = isEffInAir(*nextChd, oldNextNotDashing);

            bool cvOnWall = false, cvSupported = false, cvInAir = true, inJumpStartupOrJustEnded = false; 
            CharacterBase::EGroundState cvGroundState = CharacterBase::EGroundState::InAir;
            InputInducedMotion* inputInducedMotion = transientUdToInputInducedMotion.at(ud);
            stepSingleChdState(currRdfId, currRdf, nextRdf, dt, ud, UDT_NPC, cc, single, currChd, nextChd, groundBodyIsChCollider, isDead, cvOnWall, cvSupported, cvInAir, inJumpStartupOrJustEnded, cvGroundState, inputInducedMotion);
            postStepSingleChdStateCorrection(currRdfId, UDT_NPC, ud, single, currChd, nextChd, cc, cvSupported, cvInAir, cvOnWall, currNotDashing, currEffInAir, oldNextNotDashing, oldNextEffInAir, inJumpStartupOrJustEnded, cvGroundState, inputInducedMotion);

            Quat currChdQ;
            Vec3 currChdFacing;
            BaseBattleCollisionFilter::calcChdFacing(currChd, currChdQ, currChdFacing);

            if (isDead) {
                if (CharacterState::Dying != nextChd->ch_state()) {
                    transitToDying(currRdfId, currNpc, cvInAir, nextNpc);
                }
            } else if (!noOpSet.count(nextChd->ch_state())) {
                bool notTurningAround = (currChd.q_x() == nextChd->q_x() && currChd.q_y() == nextChd->q_y() && currChd.q_z() == nextChd->q_z() && currChd.q_w() == nextChd->q_w());
                if (cc->has_vision_reaction() && notTurningAround) {
                    BaseNpcReaction* npcReaction = globalNpcReactionMap.at(cc->species_id());
                    if (nullptr != npcReaction) {
                        NpcGoal currNpcGoal = currNpc.goal_as_npc();
                        uint64_t currNpcCachedCueCmd = currNpc.cached_cue_cmd();
                        NpcGoal newGoal = currNpcGoal;
                        uint64_t newCmd = 0;
                        const RotatedTranslatedShape* shape = static_cast<const RotatedTranslatedShape*>(single->GetShape());
                        const MassProperties massProps = shape->GetMassProperties();
                        npcReaction->postStepDeriveNpcVisionReaction(currRdfId, antiGravityNorm, gravityMagnitude, transientUdToCurrPlayer, transientUdToCurrNpc, transientUdToCurrBl, biNoLock, narrowPhaseQueryNoLock, this, defaultBplf, defaultOlf, single, selfNpcBodyID, ud, currNpcGoal, currNpcCachedCueCmd, currChd, massProps, currChdFacing, cc, nextChd, cvSupported, cvInAir, cvOnWall, currNotDashing, currEffInAir, oldNextNotDashing, oldNextEffInAir, inJumpStartupOrJustEnded, cvGroundState, newGoal, newCmd);
                        nextNpc->set_goal_as_npc(newGoal);
                        nextNpc->set_cached_cue_cmd(newCmd);
                    }   
                }
            }
        }, 0);
        postPhysicsUpdateMTBarrier->AddJob(handle);
    }

    for (int i = 0; i < currRdf->bullets_size(); i++) {
        if (globalPrimitiveConsts->terminating_bullet_id() == currRdf->bullets(i).id()) break;
        auto handle = jobSys->CreateJob("bullet-post-physics-update", JPH::Color::sBlack, [currRdfId, i, currRdf, nextRdf, this, dt]() {
            const Bullet& currBl = currRdf->bullets(i);
            Bullet* nextBl = nextRdf->mutable_bullets(i); // [WARNING] By reaching here, we haven't executed "leftShiftDeadBullets", hence the indices of "currRdf->bullets" and "nextRdf->bullets" are FULLY ALIGNED.

            const Skill* lhsSkill = nullptr;
            const BulletConfig* lhsBlConfig = nullptr;
            FindBulletConfig(currBl.skill_id(), currBl.active_skill_hit(), lhsSkill, lhsBlConfig);

            auto ud = calcUserData(currBl);
            bool shouldVanish = false;
            Vec3 vanishingPos(nextBl->x(), nextBl->y(), nextBl->z());
            Vec3 vanishingPosAdds(0, 0, 0);
            int vanishingPosAddsCnt = 0;

            if (transientUdToCollisionUdHolder.count(ud)) {
                CollisionUdHolder_ThreadSafe* holder = transientUdToCollisionUdHolder.at(ud);
                int cntNow = holder->GetCnt_Realtime();
                uint64_t udRhs;
                ContactPoints contactPointsLhs;                
                for (int j = 0; j < holder->GetCnt_Realtime(); ++j) {
                    bool fetched = holder->GetUd_NotThreadSafe(j, udRhs, contactPointsLhs);
                    if (!fetched) continue;
                    uint64_t udtRhs = getUDT(udRhs);
                    switch (udtRhs) {
                    case UDT_PLAYER:
                    case UDT_NPC:
                    case UDT_TRAP:
                    case UDT_OBSTACLE:
                        switch (lhsBlConfig->b_type()) {
                        case BulletType::MechanicalCartridge: {
                            shouldVanish = true;
                            for (int k = 0; k < contactPointsLhs.size(); ++k) {
                                vanishingPosAdds += contactPointsLhs.at(k);
                                vanishingPosAddsCnt += 1;
                            }
                            break;
                        }
                        case BulletType::MagicalFireball:
                        case BulletType::Melee:
                            break;
                        case BulletType::GroundWave:
                            // TODO: Only explode when it's NOT colliding with the supporting barrier
                            break;
                        default:
                            break;
                        }
                        break;
                    case UDT_TRIGGER:
                        // TODO: Depending on the type of trigger, might NOT explode
                        break;
                    default:
                        break;
                    }
                }
            }

            if (transientUdToBodyID.count(ud)) {
                const BodyID bodyID = *(transientUdToBodyID.at(ud));
                RVec3 newPos;
                Quat newRot;
                biNoLock->GetPositionAndRotation(bodyID, newPos, newRot);
                auto newVel = biNoLock->GetLinearVelocity(bodyID);

                nextBl->set_x(newPos.GetX());
                nextBl->set_y(newPos.GetY());
                nextBl->set_z(0);
                nextBl->set_q_x(newRot.GetX());
                nextBl->set_q_y(newRot.GetY());
                nextBl->set_q_z(newRot.GetZ());
                nextBl->set_q_w(newRot.GetW());
                nextBl->set_vel_x(IsLengthNearZero(newVel.GetX() * dt) ? 0 : newVel.GetX());
                nextBl->set_vel_y(IsLengthNearZero(newVel.GetY() * dt) ? 0 : newVel.GetY());
                nextBl->set_vel_z(0);
            }

            if (shouldVanish) {
                if (0 < vanishingPosAddsCnt) {
                    vanishingPos += (vanishingPosAdds / vanishingPosAddsCnt);
                }
                nextBl->set_bl_state(BulletState::Vanishing);
                nextBl->set_frames_in_bl_state(0);
                nextBl->set_x(vanishingPos.GetX());
                nextBl->set_y(vanishingPos.GetY());
                nextBl->set_z(0);

                nextBl->set_vel_x(0);
                nextBl->set_vel_y(0);
                nextBl->set_vel_z(0);
            }
        }, 0);
        postPhysicsUpdateMTBarrier->AddJob(handle);
    }

    for (int i = 0; i < currRdf->dynamic_traps_size(); i++) {
        if (globalPrimitiveConsts->terminating_trap_id() == currRdf->dynamic_traps(i).id()) break;
        auto handle = jobSys->CreateJob("dynamic-trap-post-physics-update", JPH::Color::sBlack, [currRdfId, i, currRdf, nextRdf, this, dt]() {
                const Trap& currTp = currRdf->dynamic_traps(i);
                Trap* nextTp = nextRdf->mutable_dynamic_traps(i); // [WARNING] By reaching here, we haven't executed "leftShiftDeadTraps", hence the indices of "currRdf->dynamic_traps" and "nextRdf->dynamic_traps" are FULLY ALIGNED.
                auto ud = calcUserData(currTp);
                const uint32_t tpt = currTp.tpt();
                BodyID targetBodyID;
                if (globalPrimitiveConsts->tpt_sliding_platform() == tpt) {
                    if (transientUdToConstraintObsIfaceBodyID.count(ud)) {
                        targetBodyID = *(transientUdToConstraintObsIfaceBodyID.at(ud));
                    }
                } else {
                    if (transientUdToBodyID.count(ud)) {
                        targetBodyID = *(transientUdToBodyID.at(ud));
                    }
                }
                if (!targetBodyID.IsInvalid()) {
                    RVec3 newPos;
                    Quat newRot;
                    biNoLock->GetPositionAndRotation(targetBodyID, newPos, newRot);
                    auto newVel = biNoLock->GetLinearVelocity(targetBodyID);

                    JPH_ASSERT(transientUdToNextTrap.count(ud));
                    Trap* nextTp = transientUdToNextTrap.at(ud);

                    nextTp->set_x(newPos.GetX());
                    nextTp->set_y(newPos.GetY());
                    nextTp->set_z(0);
                    nextTp->set_q_x(newRot.GetX());
                    nextTp->set_q_y(newRot.GetY());
                    nextTp->set_q_z(newRot.GetZ());
                    nextTp->set_q_w(newRot.GetW());
                    nextTp->set_vel_x(IsLengthNearZero(newVel.GetX()* dt) ? 0 : newVel.GetX());
                    nextTp->set_vel_y(IsLengthNearZero(newVel.GetY()* dt) ? 0 : newVel.GetY());
                    nextTp->set_vel_z(0);
                }
            }, 0);
        postPhysicsUpdateMTBarrier->AddJob(handle);
    }

    for (int i = 0; i < currRdf->triggers_size(); i++) {
        if (globalPrimitiveConsts->terminating_trigger_id() == currRdf->triggers(i).id()) break;
        
    }

    jobSys->WaitForJobs(postPhysicsUpdateMTBarrier);
    jobSys->DestroyBarrier(postPhysicsUpdateMTBarrier);
    
    // Special handling for triggers that MUST subscribe to its own "publishing_to_trigger_id_upon_exhausted"
    for (int i = 0; i < currRdf->triggers_size(); i++) {
        const Trigger& currTrigger = currRdf->triggers(i);
        if (globalPrimitiveConsts->terminating_trigger_id() == currTrigger.id()) break;
        if (!directNpcSpawnerTrtSet.count(currTrigger.trt())) {
            continue;
        }
        if (0 >= currTrigger.quota()) {
            continue;
        }
        if (!trActiveMainCycleStates.count(currTrigger.state())) {
            continue;
        }
        if (!triggerConfigFromTileDict.count(currTrigger.id())) {
            continue;
        }
        auto* triggerConfigFromTiled = triggerConfigFromTileDict.at(currTrigger.id());
        if (globalPrimitiveConsts->terminating_trigger_id() == triggerConfigFromTiled->publishing_to_trigger_id_upon_exhausted()) {
            continue;
        }
        if (!stepResult->fulfilled_trigger_ids().count(triggerConfigFromTiled->publishing_to_trigger_id_upon_exhausted())) {
            continue;
        }

        Trigger* nextTrigger = nextRdf->mutable_triggers(i);
        nextTrigger->set_main_cycle_mask_to_fulfill(0);
#ifndef NDEBUG
        std::ostringstream oss;
        oss << "@currRdfId=" << currRdfId << ", trigger id=" << currTrigger.id() << " is pre-fulfilled because it subscribed to anotherTriggerId=" << triggerConfigFromTiled->publishing_to_trigger_id_upon_exhausted() << " being fulfilled" << std::endl;
        Debug::Log(oss.str(), DColor::Orange);
#endif
    }

    calcFallenDeath(currRdf, nextRdf);
    leftShiftDeadNpcs(currRdfId, nextRdf); // Might change "mNextRdfBulletIdCounter" and "mNextRdfBulletCount"
    nextRdf->set_npc_count(mNextRdfNpcCount.load());
    nextRdf->set_npc_id_counter(mNextRdfNpcIdCounter.load());

    leftShiftDeadBullets(currRdfId, nextRdf); // Might change "mNextRdfBulletCount"
    nextRdf->set_bullet_count(mNextRdfBulletCount.load());
    nextRdf->set_bullet_id_counter(mNextRdfBulletIdCounter.load());

    leftShiftDeadPickables(currRdfId, nextRdf);
    leftShiftDeadTriggers(currRdfId, nextRdf);
    batchRemoveFromPhySysAndCache(currRdfId, currRdf);

    return nextRdf;
}

void BaseBattle::Clear() {
    if (nullptr == phySys || nullptr == bi) {
        return;
    }

    while (!activeNonContactConstraints.empty()) { 
        NON_CONTACT_CONSTRAINT_T* single = activeNonContactConstraints.back();
        activeNonContactConstraints.pop_back();
        delete single;
    }

    while (!cachedNonContactConstraints.empty()) {
        for (auto it = cachedNonContactConstraints.begin(); it != cachedNonContactConstraints.end(); ) {
            auto& q = it->second;
            while (!q.empty()) {
                NON_CONTACT_CONSTRAINT_T* single = q.back();
                q.pop_back();
                delete single;
            }
            it = cachedNonContactConstraints.erase(it);
        }
    }

    if (!staticColliderBodyIDs.empty()) {
        biNoLock->RemoveBodies(staticColliderBodyIDs.data(), staticColliderBodyIDs.size());
        biNoLock->DestroyBodies(staticColliderBodyIDs.data(), staticColliderBodyIDs.size());
        staticColliderBodyIDs.clear();
    }
    
    /*
    [WARNING] 
    
    - No need to explicitly remove "CharacterVirtual.mInnerBodyID"s, the destructor "~CharacterVirtual" will take care of it.

    - No need to explicitly remove "Character.mBodyID"s, the destructor "~Character" will take care of it.
    */ 

    while (!activeChColliders.empty()) {
        CH_COLLIDER_T* single = activeChColliders.back();
        activeChColliders.pop_back();
        single->RemoveFromPhysicsSystem();
        delete single;
    }

    while (!cachedChColliders.empty()) {
        for (auto it = cachedChColliders.begin(); it != cachedChColliders.end(); ) {
            auto& q = it->second;
            while (!q.empty()) {
                CH_COLLIDER_T* single = q.back();
                q.pop_back();
                single->RemoveFromPhysicsSystem();
                delete single;
            }
            it = cachedChColliders.erase(it);
        }
    }

    bodyIDsToClear.clear();
    while (!activeBlColliders.empty()) { 
        BL_COLLIDER_T* single = activeBlColliders.back();
        activeBlColliders.pop_back();
        auto bodyID = single->GetID();
        bodyIDsToClear.push_back(bodyID);
    }

    while (!cachedBlColliders.empty()) {
        // [REMINDER] "blStockCache" will be collected into "bodyIDsToClear" here.
        for (auto it = cachedBlColliders.begin(); it != cachedBlColliders.end(); ) {
            auto& q = it->second;
            while (!q.empty()) {
                BL_COLLIDER_T* single = q.back();
                q.pop_back();
                auto bodyID = single->GetID();
                bodyIDsToClear.push_back(bodyID);
            }
            it = cachedBlColliders.erase(it);
        }
    }

    while (!activeTpColliders.empty()) { 
        TP_COLLIDER_T* single = activeTpColliders.back();
        activeTpColliders.pop_back();
        auto bodyID = single->GetID();
        bodyIDsToClear.push_back(bodyID);
    }

    while (!cachedTpColliders.empty()) {
        // [REMINDER] All "tpXxxStockCache"s will be collected into "bodyIDsToClear" here.
        for (auto it = cachedTpColliders.begin(); it != cachedTpColliders.end(); ) {
            auto& q = it->second;
            while (!q.empty()) {
                TP_COLLIDER_T* single = q.back();
                q.pop_back();
                auto bodyID = single->GetID();
                bodyIDsToClear.push_back(bodyID);
            }
            it = cachedTpColliders.erase(it);
        }
    }

    while (!activeTrColliders.empty()) {
        TR_COLLIDER_T* single = activeTrColliders.back();
        activeTrColliders.pop_back();
        auto bodyID = single->GetID();
        bodyIDsToClear.push_back(bodyID);
    }

    while (!cachedTrColliders.empty()) {
        // [REMINDER] "trStockCache" will be collected into "bodyIDsToClear" here.
        for (auto it = cachedTrColliders.begin(); it != cachedTrColliders.end(); ) {
            auto& q = it->second;
            while (!q.empty()) {
                TR_COLLIDER_T* single = q.back();
                q.pop_back();
                auto bodyID = single->GetID();
                bodyIDsToClear.push_back(bodyID);
            }
            it = cachedTrColliders.erase(it);
        }
    }

    if (!bodyIDsToClear.empty()) {
        biNoLock->RemoveBodies(bodyIDsToClear.data(), bodyIDsToClear.size());
        biNoLock->DestroyBodies(bodyIDsToClear.data(), bodyIDsToClear.size());
        bodyIDsToClear.clear();
    }

    blStockCache = nullptr;
    tpDynamicStockCache = nullptr;  
    tpKinematicStockCache = nullptr;
    tpObsIfaceStockCache = nullptr; 
    tpHelperStockCache = nullptr;   
    trStockCache = nullptr;

    battleSpecificConfig = nullptr;
    trapConfigFromTileDict.clear();
    triggerConfigFromTileDict.clear();
    
    // Clear book keeping member variables
    lcacIfdId = -1;
    rdfBuffer.Clear();
    ifdBuffer.Clear();
    if (frameLogBuffer.GetAllocator() == &pbTempAllocator) {
        for (int fid = frameLogBuffer.StFrameId; fid < frameLogBuffer.EdFrameId; fid++) {
            int arrIdx = frameLogBuffer.GetArrIdxByOffset(fid - frameLogBuffer.StFrameId);
            if (-1 == arrIdx) continue;
            ArenaFreeFrameLog(frameLogBuffer.Eles[arrIdx], &pbTempAllocator);
            frameLogBuffer.Eles[arrIdx] = nullptr; // Making sure that next time "frameLogBuffer.DryPut()" is called, "ArenaAllocFrameLog" will be called too -- after "pbTempAllocator.Reset()" within "BaseBattle::Clear()".
        }
    }
    
    frameLogBuffer.Clear();

    playersCnt = 0;

    phySys->ClearBodyManagerFreeList();
    phySys->ValidateBodyManagerContactCacheForAllBodies();

    frameLogEnabled = false;

    // Deallocate temp variables in pb-arena
    pbTempAllocator.Reset();
}

bool BaseBattle::ResetInventorySlot(const InventorySlotConfig& initIvSlot, InventorySlot* ivSlot) {
    ivSlot->set_stock_type(initIvSlot.stock_type());
    ivSlot->set_quota(initIvSlot.quota());
    ivSlot->set_frames_to_recover(0);
    ivSlot->set_gauge_charged(0);
    return true;
}

bool BaseBattle::ResetInventory(const CharacterConfig* cc, const CharacterBattleSpecificConfig* characterOverride, CharacterDownsync* chd) {
    chd->clear_inventory_slots();
    if (nullptr != characterOverride && 0 < characterOverride->init_inventory_slots_size()) {
        for (int t = 0; t < characterOverride->init_inventory_slots_size(); t++) {
            const InventorySlotConfig& initIvSlot = characterOverride->init_inventory_slots(t);
            if (InventorySlotStockType::NoneIv == initIvSlot.stock_type()) break;
            InventorySlot* ivSlot = chd->add_inventory_slots();
            ResetInventorySlot(initIvSlot, ivSlot);
        }
    } else if (0 < cc->init_inventory_slots_size()) {
        for (int t = 0; t < cc->init_inventory_slots_size(); t++) {
            const InventorySlotConfig& initIvSlot = cc->init_inventory_slots(t);
            if (InventorySlotStockType::NoneIv == initIvSlot.stock_type()) break;
            InventorySlot* ivSlot = chd->add_inventory_slots();
            ResetInventorySlot(initIvSlot, ivSlot);
        }
    }
    return true;
}

bool BaseBattle::ResetStartRdf(char* inBytes, int inBytesCnt) {
    WsReq* initializerMapData = google::protobuf::Arena::Create<WsReq>(&pbTempAllocator);
    initializerMapData->ParseFromArray(inBytes, inBytesCnt);
    return ResetStartRdf(initializerMapData);
}

typedef std::list<TriggerConfigFromTiled*> TR_ADJ_ELE;
typedef std::unordered_map<uint32_t, TR_ADJ_ELE> TR_ADJ_LIST;
void BaseBattle::topoSortTriggerConfigFromTiledList(WsReq* initializerMapData) {
    for (int i = 0; i < initializerMapData->trigger_config_from_tile_list_size(); i++) {
        const TriggerConfigFromTiled& c = initializerMapData->trigger_config_from_tile_list(i);
        TriggerConfigFromTiled* mutableTriggerConfigFromTiled = google::protobuf::Arena::Create<TriggerConfigFromTiled>(&pbTempAllocator);
        mutableTriggerConfigFromTiled->CopyFrom(c);
        if (collidableTrtSet.count(c.trt())) {
            if (0 == c.init_q_x() && 0 == c.init_q_y() && 0 == c.init_q_z() && 0 == c.init_q_w()) {
                mutableTriggerConfigFromTiled->set_init_q_x(0);
                mutableTriggerConfigFromTiled->set_init_q_y(0);
                mutableTriggerConfigFromTiled->set_init_q_z(0);
                mutableTriggerConfigFromTiled->set_init_q_w(1);
            }
        }
        if (globalPrimitiveConsts->trt_indi_wave_npc_spawner() == c.trt()) {
            mutableTriggerConfigFromTiled->set_quota(c.character_spawner_time_seq_size()); // [REMINDER] This overwrite takes highest priority over whatever is parsed from UI/Story Editor
        } else {
            if (0 >= c.recovery_frames()) {
                mutableTriggerConfigFromTiled->set_recovery_frames(globalPrimitiveConsts->default_tr_recovery_frames());
            } 
            if (0 >= c.quota()) {
                if (trivialTrtSet.count(c.trt()) || mixedMainAndSubCycleTrtSet.count(c.trt())) {
                    mutableTriggerConfigFromTiled->set_quota(1);
                }
            }
        }

        // [WARNING] Zero quota is meaningless
        JPH_ASSERT(0 < mutableTriggerConfigFromTiled->quota());
        triggerConfigFromTileDict[c.id()] = mutableTriggerConfigFromTiled;
    }

    TR_ADJ_LIST trAdjListForTopoSort;
    std::unordered_map<uint32_t, uint32_t> inDegrees;
    for (int i = 0; i < initializerMapData->trigger_config_from_tile_list_size(); i++) {
        const TriggerConfigFromTiled& c = initializerMapData->trigger_config_from_tile_list(i);
        uint32_t targetTriggerId = c.publishing_to_trigger_id_upon_exhausted();
        if (globalPrimitiveConsts->terminating_trigger_id() == targetTriggerId) continue;
        JPH_ASSERT(triggerConfigFromTileDict.count(c.id()));
        JPH_ASSERT(triggerConfigFromTileDict.count(targetTriggerId));
        TriggerConfigFromTiled* mutableC = triggerConfigFromTileDict[c.id()];
        trAdjListForTopoSort[targetTriggerId].push_back(mutableC); // Build a reverse link for later convenience
        ++inDegrees[c.id()];
    }

    std::queue<uint32_t> q;
    for (int i = 0; i < initializerMapData->trigger_config_from_tile_list_size(); i++) {
        const TriggerConfigFromTiled& c = initializerMapData->trigger_config_from_tile_list(i);
        if (inDegrees.count(c.id()) && 0 < inDegrees.at(c.id())) continue;
        q.push(c.id());
    }
    
    uint32_t currTopoLv = 1; 
    while (!q.empty()) {
        int sz = q.size();
        while (sz--) {
            uint32_t frontId = q.front(); q.pop();
            TriggerConfigFromTiled* mutableC = triggerConfigFromTileDict[frontId];
            mutableC->set_topo_lv(currTopoLv);
            if (!trAdjListForTopoSort.count(frontId)) continue;
            for (auto descendant : trAdjListForTopoSort[frontId]) {
                q.push(descendant->id());
            }
        }
        ++currTopoLv;
    }
}

bool BaseBattle::ResetStartRdf(WsReq* initializerMapData) {
    /* [WARNING] 

    When running unit tests to compare a "reference battle" with a "reused battle" for "rollback-chasing determinism", any mismatch of "BodyID" (i.e. managed by "BodyManager.mBodyIDFreeListStart" in "BodyManager::AddBody" and "BodyManager::RemoveBodyInternal" if the "PhysicsSystem" instance is not re-allocated) impacts the comparison by 
    - [ContactConstraintManager::TemplatedAddContactConstraint](https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Physics/Constraints/ContactConstraintManager.cpp#L1044)
    - [PhysicsSystem::JobSolveVelocityConstraints -> ContactConstraintManager::SortContacts](https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Physics/PhysicsSystem.cpp#L1421)

    , therefore the reference battle must be created with the same "BodyManager.mBodyIDFreeListStart" for fair comparison.

    Per experimental results, executing the following code snippet at the end of "BaseBattle::Clear()" fixes "FrontendTest/runTestCase6", "FrontendTest/runTestCase7" and "FrontendTest/runTestCase11", which are dedicated to testing "rollback-chasing determinism".
    
    ```
    class JPH::BodyManager {
        void BodyManager::ClearFreeList() {
	        JPH_ASSERT(0 == mNumBodies);
	        JPH_ASSERT(0 == mNumActiveBodies[(int)EBodyType::SoftBody]);
	        JPH_ASSERT(0 == mNumActiveBodies[(int)EBodyType::RigidBody]);
	        JPH_ASSERT(0 == mNumActiveCCDBodies);
	        mBodyIDFreeListStart = cBodyIDFreeListEnd;
	        int origSize = mBodySequenceNumbers.size();
	        for (int i = 0; i < origSize; i++) {
		        mBodySequenceNumbers[i] = 0;
	        }
	        mBodies.clear();
        }
    }
    ```

    Also per experimental results, "PhysicsSystem::SaveState" and "PhysicsSystem::RestoreState" would NOT help reset or align "PhysicsSystem.mBodyInterfaceLocking.mBodyManager.mBodyIDFreeListStart".
    */
    fallenDeathHeight = initializerMapData->fallen_death_height();
    if (initializerMapData->has_battle_specific_config()) {
        const BattleSpecificConfig& c = initializerMapData->battle_specific_config();
        battleSpecificConfig = google::protobuf::Arena::Create<BattleSpecificConfig>(&pbTempAllocator);
        battleSpecificConfig->CopyFrom(c);
    }

    RenderFrame* startRdf = initializerMapData->mutable_self_parsed_rdf(); // [REMINDER] Allocated in pb-arena "pbTempAllocator", because "initializerMapData" was allocated by the same arena in "void ResetStartRdf(char* inBytes, int inBytesCnt)".

    for (int i = 0; i < initializerMapData->trap_config_from_tile_list_size(); i++) {
        const TrapConfigFromTiled& c = initializerMapData->trap_config_from_tile_list(i);
        auto* cc = google::protobuf::Arena::Create<TrapConfigFromTiled>(&pbTempAllocator);
        cc->CopyFrom(c);
        if (globalPrimitiveConsts->tpt_sliding_platform() == c.tpt()) {
            Vec3 worldSpaceSliderAxis(c.slider_axis_x(), c.slider_axis_y(), c.slider_axis_z());
            if (0 == worldSpaceSliderAxis.Length()) {
                Vec3 initVel(c.init_vel_x(), c.init_vel_y(), c.init_vel_z());
                if (0 != initVel.Length()) {
                    worldSpaceSliderAxis = initVel.Normalized();
                } else if (!(0 == c.init_q_x() && 0 == c.init_q_y() && 0 == c.init_q_z() && 0 == c.init_q_w())) {
                    Quat initQ(c.init_q_x(), c.init_q_y(), c.init_q_z(), c.init_q_w());
                    worldSpaceSliderAxis = initQ * Vec3::sAxisX();
                } else {
                    worldSpaceSliderAxis = Vec3::sAxisX();
                }
            } else {
                worldSpaceSliderAxis = worldSpaceSliderAxis.Normalized();
            }
            cc->set_slider_axis_x(worldSpaceSliderAxis.GetX());
            cc->set_slider_axis_y(worldSpaceSliderAxis.GetY());
            cc->set_slider_axis_z(worldSpaceSliderAxis.GetZ());
        }
        trapConfigFromTileDict[c.id()] = cc;
    }

    topoSortTriggerConfigFromTiledList(initializerMapData);

    auto& trapConfigs = globalConfigConsts->trap_configs();
    for (int i = 0; i < startRdf->dynamic_traps_size(); i++) {
        Trap* tp = startRdf->mutable_dynamic_traps(i);
        if (globalPrimitiveConsts->terminating_trap_id() == tp->id()) break;
        JPH_ASSERT(trapConfigs.count(tp->tpt()));
        auto& defaultC = trapConfigs.at(tp->tpt());
        tp->set_q_x(0);
        tp->set_q_y(0);
        tp->set_q_z(0);
        tp->set_q_w(1);
        Vec3 initVel(defaultC.default_linear_speed(), 0, 0);
        tp->set_vel_x(initVel.GetX());
        tp->set_vel_y(initVel.GetY());
        tp->set_vel_z(initVel.GetZ());
        tp->set_trap_state(TrapState::TpWalking);

        if (!trapConfigFromTileDict.count(tp->id())) continue;
        const TrapConfigFromTiled* c = trapConfigFromTileDict.at(tp->id());
        tp->set_x(c->init_x());
        tp->set_y(c->init_y());
        tp->set_z(c->init_z());
        if (!(0 == c->init_q_x() && 0 == c->init_q_y() && 0 == c->init_q_z() && 0 == c->init_q_w())) {
            Quat initQ(c->init_q_x(), c->init_q_y(), c->init_q_z(), c->init_q_w());
            tp->set_q_x(initQ.GetX());
            tp->set_q_y(initQ.GetY());
            tp->set_q_z(initQ.GetZ());
            tp->set_q_w(initQ.GetW());
        }
        if (0 != c->init_vel_x() || 0 != c->init_vel_y() || 0 != c->init_vel_z()) {
            initVel.Set(c->init_vel_x(), c->init_vel_y(), c->init_vel_z());
            tp->set_vel_x(initVel.GetX());
            tp->set_vel_y(initVel.GetY());
            tp->set_vel_z(initVel.GetZ());
        }
        if (!c->init_not_moving()) {
            tp->set_trap_state(TrapState::TpWalking);
        } else {
            tp->set_trap_state(TrapState::TpIdle);
        }
    }

    playersCnt = startRdf->players_size();
    allConfirmedMask = (U64_1 << playersCnt) - 1;

    auto stRdfId = startRdf->id();
    while (rdfBuffer.EdFrameId <= stRdfId) {
        int gapRdfId = rdfBuffer.EdFrameId;
        RenderFrame* holder = rdfBuffer.DryPut();
        holder->CopyFrom(*startRdf); // [WARNING] Copied from "pbTempAllocator".
        holder->set_id(gapRdfId);
        initTriggerMainAndSubCycles(holder);
    }

    if (frameLogEnabled) {
        while (frameLogBuffer.EdFrameId <= stRdfId) {
            int gapRdfId = frameLogBuffer.EdFrameId;
            FrameLog* holder = frameLogBuffer.DryPut();
            holder->unsafe_arena_set_allocated_rdf(startRdf);
        }
    }

    prefabbedInputList.assign(playersCnt, 0);
    playerInputFrontIds.assign(playersCnt, 0);
    playerInputFronts.assign(playersCnt, 0);
    playerInputFrontIdsSorted.clear();

    int staticColliderId = 1;
    staticColliderBodyIDs.clear();
    for (int i = 0; i < initializerMapData->serialized_barriers_size(); i++) {
        SerializedBarrierCollider* barrier = initializerMapData->mutable_serialized_barriers(i);
        const BarrierColliderAttr& barrierAttr = barrier->attr(); 
        SerializableConvexPolygon* convexPolygon = barrier->mutable_polygon();
        const uint64_t staticColliderUd = calcStaticColliderUserData(staticColliderId); // As [BodyManager::AddBody](https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Physics/Body/BodyManager.cpp#L285) maintains "BodyID" counting by , in rollback netcode with a reused "BaseBattle" instance, even the same "static collider" might NOT get the same "BodyID" at different battles, we MUST use custom ids to distinguish "Body" instances!
        if (barrierAttr.provides_slip_jump()) {
            transientSlipJumpableUds.insert(staticColliderUd);
        }
        if (barrierAttr.forces_crouching()) {
            transientCrouchForcingUds.insert(staticColliderUd);
        }
        const BodyID* newBodyID = nullptr;
        int pointsCnt = convexPolygon->points_size();
        double recalcAnchorX = 0, recalcAnchorY = 0;
        if (convexPolygon->is_box()) {
            // TODO: Support rotation
            float xExtent = 0.f, yExtent = 0.f;
            for (int i = 0; i < convexPolygon->points_size(); i++) {
                auto& pi = convexPolygon->points(i);
                for (int j = i + 1; j < convexPolygon->points_size(); j++) {
                    auto& pj = convexPolygon->points(j);
                    float dxAbs = std::abs(pj.x() - pi.x());
                    float dyAbs = std::abs(pj.y() - pi.y());
                    if (dxAbs > xExtent) xExtent = dxAbs;
                    if (dyAbs > yExtent) yExtent = dyAbs;
                }
                recalcAnchorX += pi.x();
                recalcAnchorY += pi.y();

                if (pi.y() < fallenDeathHeight) {
                    fallenDeathHeight = pi.y();
                }
            }
            const double anchorX = (recalcAnchorX / pointsCnt);
            const double anchorY = (recalcAnchorY / pointsCnt);
            float xHalfExtent = 0.5f * xExtent, yHalfExtent = 0.5f * yExtent;
            float convexRadius = (xHalfExtent + yHalfExtent) * 0.5;
            if (cDefaultHalfThickness < convexRadius) {
                convexRadius = cDefaultHalfThickness; // Required by the underlying body creation 
            }
            Vec3 halfExtent(xHalfExtent, yHalfExtent, cDefaultHalfThickness);
            BoxShapeSettings bodyShapeSettings(halfExtent, convexRadius); // transient, to be discarded after creating "body"
            BoxShapeSettings::ShapeResult shapeResult;
            /*
                "Body" and "BodyCreationSettings" will handle lifecycle of the following "bodyShape", i.e.
                - by "RefConst<Shape> Body::mShape" (note that "Body" does NOT hold a member variable "BodyCreationSettings") or
                - by "RefConst<ShapeSettings> BodyCreationSettings::mShape".

                See "<proj-root>/JoltBindings/RefConst_destructor_trick.md" for details.
            */
            const BoxShape* bodyShape = new BoxShape(bodyShapeSettings, shapeResult);
            BodyCreationSettings bodyCreationSettings(bodyShape, Vec3(anchorX, anchorY, 0), JPH::Quat::sIdentity(), EMotionType::Static, MyObjectLayers::NON_MOVING);
            bodyCreationSettings.mUserData = staticColliderUd;
            bodyCreationSettings.mFriction = cDefaultBarrierFriction;
            Body* body = biNoLock->CreateBody(bodyCreationSettings);
            newBodyID = &(body->GetID());
            staticColliderBodyIDs.push_back(*newBodyID);
        } else {
            std::vector<PbVec2> effConvexPolygonPoints;
            effConvexPolygonPoints.reserve(pointsCnt);
            for (auto& srcPoint : convexPolygon->points()) {
                effConvexPolygonPoints.push_back(srcPoint);
                recalcAnchorX += srcPoint.x();
                recalcAnchorY += srcPoint.y();
            }
            const double anchorX = (recalcAnchorX / pointsCnt);
            const double anchorY = (recalcAnchorY / pointsCnt);
            std::sort(effConvexPolygonPoints.begin(), effConvexPolygonPoints.end(), [anchorX, anchorY](const PbVec2& a, const PbVec2& b) {
                const double dxA = (a.x() - anchorX);
                const double dyA = (a.y() - anchorY);
                const double dxB = (b.x() - anchorX);
                const double dyB = (b.y() - anchorY);
                double crossProduct = dxA * dyB - dyA * dxB;

                if (crossProduct != 0) {
                    return (crossProduct < 0);
                } else {
                    double distA2 = dxA * dxA + dyA * dyA;
                    double distB2 = dxB * dxB + dyB * dyB;
                    return (distA2 < distB2);
                }
            });

            TriangleList triangles;
            for (int pi = 0; pi < pointsCnt; pi++) {
                auto fromI = pi;
                auto toI = fromI + 1;
                if (toI >= pointsCnt) {
                    toI = 0;
                }
                auto& p1 = effConvexPolygonPoints[fromI];
                auto& p2 = effConvexPolygonPoints[toI];

                auto x1 = p1.x() - anchorX;
                auto y1 = p1.y() - anchorY;
                auto x2 = p2.x() - anchorX;
                auto y2 = p2.y() - anchorY;
                
                // According to https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Physics/Collision/Shape/MeshShape.cpp#L440, the surface normal (outward) of a "JPH::Triangle" is "(v3 - v2).Cross(v1 - v2).Normalized()".
                {
                    Float3 v1(x1, y1, -cDefaultBarrierHalfThickness);
                    Float3 v2(x1, y1, +cDefaultBarrierHalfThickness);
                    Float3 v3(x2, y2, +cDefaultBarrierHalfThickness);
                    triangles.push_back(Triangle(v1, v2, v3));
                }
                {
                    Float3 v1(x2, y2, +cDefaultBarrierHalfThickness);
                    Float3 v2(x2, y2, -cDefaultBarrierHalfThickness);
                    Float3 v3(x1, y1, -cDefaultBarrierHalfThickness);
                    triangles.push_back(Triangle(v1, v2, v3));
                }

                if (p1.y() < fallenDeathHeight) {
                    fallenDeathHeight = p1.y();
                }
                if (p2.y() < fallenDeathHeight) {
                    fallenDeathHeight = p2.y();
                }
            }
            MeshShapeSettings bodyShapeSettings(triangles);
            MeshShapeSettings::ShapeResult shapeResult;
            const MeshShape* bodyShape = new MeshShape(bodyShapeSettings, shapeResult);
            BodyCreationSettings bodyCreationSettings(bodyShape, Vec3(anchorX, anchorY, 0), JPH::Quat::sIdentity(), EMotionType::Static, MyObjectLayers::NON_MOVING);
            bodyCreationSettings.mUserData = staticColliderUd;
            bodyCreationSettings.mFriction = cDefaultBarrierFriction;
            Body* body = biNoLock->CreateBody(bodyCreationSettings);
            newBodyID = &(body->GetID());
            staticColliderBodyIDs.push_back(*newBodyID);
        }
#ifndef NDEBUG
        std::ostringstream oss;
        oss << "The " << i + 1 << "-th static collider with ud=" << staticColliderUd << ", bodyID=" << newBodyID->GetIndexAndSequenceNumber() << ":\n\t";
        for (int pi = 0; pi < convexPolygon->points_size(); pi ++) {
            auto& p = convexPolygon->points(pi);
            auto x = p.x();
            auto y = p.y();
            oss << "(" << x << "," << y << ") ";
        }
        Debug::Log(oss.str(), DColor::Orange);
#endif
        staticColliderId++;
    }

    auto layerState = biNoLock->AddBodiesPrepare(staticColliderBodyIDs.data(), staticColliderBodyIDs.size());
    biNoLock->AddBodiesFinalize(staticColliderBodyIDs.data(), staticColliderBodyIDs.size(), layerState, EActivation::DontActivate);

    phySys->OptimizeBroadPhase();

    transientUdToCurrPlayer.reserve(playersCnt);
    transientUdToNextPlayer.reserve(playersCnt);
    transientUdToCurrNpc.reserve(globalPrimitiveConsts->default_prealloc_npc_capacity());
    transientUdToNextNpc.reserve(globalPrimitiveConsts->default_prealloc_npc_capacity());
    transientUdToCurrBl.reserve(globalPrimitiveConsts->default_prealloc_bullet_capacity());
    transientUdToNextBl.reserve(globalPrimitiveConsts->default_prealloc_bullet_capacity());
    transientUdToCurrTrap.reserve(globalPrimitiveConsts->default_prealloc_trap_capacity());
    transientUdToNextTrap.reserve(globalPrimitiveConsts->default_prealloc_trap_capacity());
    transientUdToCurrTrigger.reserve(globalPrimitiveConsts->default_prealloc_trigger_capacity());
    transientUdToNextTrigger.reserve(globalPrimitiveConsts->default_prealloc_trigger_capacity());
    transientUdToCurrPickable.reserve(globalPrimitiveConsts->default_prealloc_pickable_capacity());
    transientUdToNextPickable.reserve(globalPrimitiveConsts->default_prealloc_pickable_capacity());

    safeDeactiviatedPosition = Vec3(65535.0, -65535.0, 0);

    preallocateBodies(startRdf, initializerMapData->preallocate_npc_species_dict());

    return true;
}

bool BaseBattle::initTriggerMainAndSubCycles(RenderFrame* startRdf) {
    std::unordered_map<uint32_t, uint64_t> collectedPublishingMaskUponExhausted;
    std::unordered_map<uint32_t, uint64_t> collectedDemandedMask;

    for (int i = 0; i < startRdf->npcs_size(); ++i) {
        NpcCharacterDownsync* npc = startRdf->mutable_npcs(i);
        if (globalPrimitiveConsts->terminating_character_id() == npc->id()) break;
        auto targetTriggerId = npc->publishing_to_trigger_id_upon_exhausted();
        if (globalPrimitiveConsts->terminating_trigger_id() == targetTriggerId) continue;
        JPH_ASSERT(triggerConfigFromTileDict.count(targetTriggerId));
        auto* targetTriggerConfigFromTile = triggerConfigFromTileDict.at(targetTriggerId);
        JPH_ASSERT(mixedMainAndSubCycleTrtSet.count(targetTriggerConfigFromTile->trt()));
        if (!collectedPublishingMaskUponExhausted.count(targetTriggerId)) {
            collectedPublishingMaskUponExhausted[targetTriggerId] = 1;
        } else {
            collectedPublishingMaskUponExhausted[targetTriggerId] <<= 1;
        }

        npc->set_publishing_mask_upon_exhausted(collectedPublishingMaskUponExhausted[targetTriggerId]);
        collectedDemandedMask[targetTriggerId] |= collectedPublishingMaskUponExhausted[targetTriggerId];
    }

    for (auto& [triggerId, triggerConfigFromTile] : triggerConfigFromTileDict) {
        auto targetTriggerId = triggerConfigFromTile->publishing_to_trigger_id_upon_exhausted();
        if (globalPrimitiveConsts->terminating_trigger_id() == targetTriggerId) continue;

        if (!collectedPublishingMaskUponExhausted.count(targetTriggerId)) {
            collectedPublishingMaskUponExhausted[targetTriggerId] = 1;
        } else {
            collectedPublishingMaskUponExhausted[targetTriggerId] <<= 1;
        }

        triggerConfigFromTile->set_publishing_mask_upon_exhausted(collectedPublishingMaskUponExhausted[targetTriggerId]);
        collectedDemandedMask[targetTriggerId] |= collectedPublishingMaskUponExhausted[targetTriggerId];

        // [WARNING] Extra quota correction to "triggerConfigFromTile". 
        if (triggerConfigFromTileDict.count(targetTriggerId)) {
            auto* targetTriggerConfigFromTiled = triggerConfigFromTileDict.at(targetTriggerId);
            if (targetTriggerConfigFromTiled->quota() < triggerConfigFromTile->quota()) {
#ifndef NDEBUG
                std::ostringstream oss;
                oss << "Correcting trigger id=" << triggerId << " oldQuota=" << triggerConfigFromTile->quota() << " to newQuota=" << targetTriggerConfigFromTiled->quota() << ", from its publishing_to_trigger_id_upon_exhausted=" << targetTriggerId << std::endl;
                Debug::Log(oss.str(), DColor::Orange);
#endif
                triggerConfigFromTile->set_quota(targetTriggerConfigFromTiled->quota());
            }
        }
    }
    
    for (int i = 0; i < startRdf->triggers_size(); ++i) {
        Trigger* tr = startRdf->mutable_triggers(i);
        if (globalPrimitiveConsts->terminating_trigger_id() == tr->id()) break;
        if (directNpcSpawnerTrtSet.count(tr->trt())) {
            JPH_ASSERT(triggerConfigFromTileDict.count(tr->id()));
        }
        tr->set_state(TriggerState::TrReady);
        tr->set_main_cycle_mask_to_fulfill(1UL); // Meets the need by far
        if (triggerConfigFromTileDict.count(tr->id())) {
            auto* triggerConfigFromTile = triggerConfigFromTileDict.at(tr->id());
            if (!collectedDemandedMask.count(tr->id())) {
                tr->set_sub_cycle_mask_to_fulfill(1);
            } else {
                tr->set_sub_cycle_mask_to_fulfill(collectedDemandedMask[tr->id()]);
            }

            if (directNpcSpawnerTrtSet.count(tr->trt())) {
                JPH_ASSERT(globalPrimitiveConsts->terminating_trigger_id() != triggerConfigFromTile->publishing_to_trigger_id_upon_exhausted()); // [WARNING] Must have, otherwise wouldn't fire
                tr->set_main_cycle_mask_to_fulfill(1); // [WARNING] There should be no other trigger that publishes to a "directNpcSpawnerTrt", regardless of any malformated config.
            }

            tr->set_topo_lv(triggerConfigFromTile->topo_lv());
            tr->set_quota(triggerConfigFromTile->quota());
            tr->set_bullet_team_id(triggerConfigFromTile->bullet_team_id());
#ifndef NDEBUG
            if (1 == startRdf->id()) {
                if (collidableTrtSet.count(tr->trt())) {
                    std::ostringstream oss;
                    oss << "Collidable trigger with id=" << tr->id() << ", trt=" << tr->trt() << " initialized with UI config: pos=(" << tr->x() << ", " << tr->y() << "), topo_lv=" << triggerConfigFromTile->topo_lv() << ", quota=" << triggerConfigFromTile->quota() << ", bullet_team_id=" << triggerConfigFromTile->bullet_team_id() << ", box_half_size=(" << triggerConfigFromTile->box_half_size_x() << ", " << triggerConfigFromTile->box_half_size_y() << "), main_cycle_mask_to_fulfill=" << tr->main_cycle_mask_to_fulfill() << ", sub_cycle_mask_to_fulfill=" << tr->sub_cycle_mask_to_fulfill() << ", publishing_to_trigger_id_upon_exhausted=" << triggerConfigFromTile->publishing_to_trigger_id_upon_exhausted() << std::endl; 
                    Debug::Log(oss.str(), DColor::Orange);
                } else {
                    std::ostringstream oss;
                    oss << "Trigger with id=" << tr->id() << ", trt=" << tr->trt() << " initialized with UI config: pos=(" << tr->x() << ", " << tr->y() << "), topo_lv=" << triggerConfigFromTile->topo_lv() << ", quota=" << triggerConfigFromTile->quota() << ", bullet_team_id=" << triggerConfigFromTile->bullet_team_id() << ", main_cycle_mask_to_fulfill=" << tr->main_cycle_mask_to_fulfill() << ", sub_cycle_mask_to_fulfill=" << tr->sub_cycle_mask_to_fulfill() << ", publishing_to_trigger_id_upon_exhausted=" << triggerConfigFromTile->publishing_to_trigger_id_upon_exhausted() << std::endl;
                    Debug::Log(oss.str(), DColor::Orange);
                }
            }
        } else {
            if (1 == startRdf->id()) {
                std::ostringstream oss;
                oss << "Trigger with id=" << tr->id() << ", trt=" << tr->trt() << " initialized without UI config: pos=(" << tr->x() << ", " << tr->y() << ")";
                Debug::Log(oss.str(), DColor::Orange);
            }
#endif
        }
    }

    return true;
}

bool BaseBattle::isEffInAir(const CharacterDownsync& chd, bool notDashing) {
    return (inAirSet.count(chd.ch_state()) && notDashing);
}

bool BaseBattle::isInJumpStartup(const CharacterDownsync& cd, const CharacterConfig* cc) {
    int effProactiveJumpStartupFrames = (InAirIdle1ByWallJump == cd.ch_state() ? cc->wall_jump_frames_to_recover() : cc->jump_startup_frames());
    if (0 >= effProactiveJumpStartupFrames) {
        effProactiveJumpStartupFrames = 4; // A default
    }
    return proactiveJumpingSet.count(cd.ch_state()) && (effProactiveJumpStartupFrames > cd.frames_in_ch_state());
}

bool BaseBattle::isJumpStartupJustEnded(const CharacterDownsync& currChd, const CharacterDownsync* nextChd, const CharacterConfig* cc) {
    int effProactiveJumpStartupFrames = (InAirIdle1ByWallJump == currChd.ch_state() ? cc->wall_jump_frames_to_recover() : cc->jump_startup_frames());
    if (0 >= effProactiveJumpStartupFrames) {
        effProactiveJumpStartupFrames = 4; // A default
    }
    bool yes1 = currChd.ch_state() == nextChd->ch_state();
    bool yes2 = proactiveJumpingSet.count(currChd.ch_state());
    return (yes1 && yes2 && (effProactiveJumpStartupFrames > currChd.frames_in_ch_state()) && (effProactiveJumpStartupFrames <= nextChd->frames_in_ch_state()));
}

void BaseBattle::transitToGroundDodgedChState(const CharacterDownsync& currChd, const Vec3 currChdFacing, CharacterDownsync* nextChd, const CharacterConfig* cc, bool currParalyzed, InputInducedMotion* ioInputInducedMotion) {
    CharacterState oldNextChState = nextChd->ch_state();
    nextChd->set_ch_state(GroundDodged);
    nextChd->set_frames_in_ch_state(0);
    nextChd->set_frames_to_recover(cc->ground_dodged_frames_to_recover());
    nextChd->set_frames_invinsible(cc->ground_dodged_frames_invinsible());
    nextChd->set_active_skill_id(globalPrimitiveConsts->no_skill());
    nextChd->set_active_skill_hit(globalPrimitiveConsts->no_skill_hit());
    if (0 == currChd.vel_x()) {
        auto effSpeedWrtGround = (0 >= cc->ground_dodged_speed() ? cc->speed() : cc->ground_dodged_speed());
        if (BackDashing == oldNextChState) {
            ioInputInducedMotion->velCOM.SetX((0 > currChdFacing.GetX() ? effSpeedWrtGround : -effSpeedWrtGround) + currChd.ground_vel_x());
        } else {
            ioInputInducedMotion->velCOM.SetX((0 < currChdFacing.GetX() ? effSpeedWrtGround : -effSpeedWrtGround) + currChd.ground_vel_x());
        }
    }

    if (currParalyzed) {
        ioInputInducedMotion->velCOM.SetX(currChd.ground_vel_x());
    }

}

void BaseBattle::jamBtnHolding(CharacterDownsync* nextChd) {
    if (0 < nextChd->btn_a_holding_rdf_cnt()) {
        nextChd->set_btn_a_holding_rdf_cnt(globalPrimitiveConsts->jammed_btn_holding_rdf_cnt());
    }
    if (0 < nextChd->btn_b_holding_rdf_cnt()) {
        nextChd->set_btn_b_holding_rdf_cnt(globalPrimitiveConsts->jammed_btn_holding_rdf_cnt());
    }
    if (0 < nextChd->btn_c_holding_rdf_cnt()) {
        nextChd->set_btn_c_holding_rdf_cnt(globalPrimitiveConsts->jammed_btn_holding_rdf_cnt());
    }
    if (0 < nextChd->btn_d_holding_rdf_cnt()) {
        nextChd->set_btn_d_holding_rdf_cnt(globalPrimitiveConsts->jammed_btn_holding_rdf_cnt());
    }
    if (0 < nextChd->btn_e_holding_rdf_cnt()) {
        nextChd->set_btn_e_holding_rdf_cnt(globalPrimitiveConsts->jammed_btn_holding_rdf_cnt());
    }
}

void BaseBattle::updateBtnHoldingByInput(const CharacterDownsync& currChd, const InputFrameDecoded& decodedInputHolder, CharacterDownsync* nextChd) {
    if (0 == decodedInputHolder.btn_a_level()) {
        nextChd->set_btn_a_holding_rdf_cnt(0);
    } else if (globalPrimitiveConsts->jammed_btn_holding_rdf_cnt() != currChd.btn_a_holding_rdf_cnt() && 0 < decodedInputHolder.btn_a_level()) {
        nextChd->set_btn_a_holding_rdf_cnt(currChd.btn_a_holding_rdf_cnt() + 1);
        if (nextChd->btn_a_holding_rdf_cnt() > globalPrimitiveConsts->max_btn_holding_rdf_cnt()) {
            nextChd->set_btn_a_holding_rdf_cnt(globalPrimitiveConsts->max_btn_holding_rdf_cnt());
        }
    }

    if (0 == decodedInputHolder.btn_b_level()) {
        nextChd->set_btn_b_holding_rdf_cnt(0);
    } else if (globalPrimitiveConsts->jammed_btn_holding_rdf_cnt() != currChd.btn_b_holding_rdf_cnt() && 0 < decodedInputHolder.btn_b_level()) {
        nextChd->set_btn_b_holding_rdf_cnt(currChd.btn_b_holding_rdf_cnt() + 1);
        if (nextChd->btn_b_holding_rdf_cnt() > globalPrimitiveConsts->max_btn_holding_rdf_cnt()) {
            nextChd->set_btn_b_holding_rdf_cnt(globalPrimitiveConsts->max_btn_holding_rdf_cnt());
        }
    }

    if (0 == decodedInputHolder.btn_c_level()) {
        nextChd->set_btn_c_holding_rdf_cnt(0);
    } else if (globalPrimitiveConsts->jammed_btn_holding_rdf_cnt() != currChd.btn_c_holding_rdf_cnt() && 0 < decodedInputHolder.btn_c_level()) {
        nextChd->set_btn_c_holding_rdf_cnt(currChd.btn_c_holding_rdf_cnt() + 1);
        if (nextChd->btn_c_holding_rdf_cnt() > globalPrimitiveConsts->max_btn_holding_rdf_cnt()) {
            nextChd->set_btn_c_holding_rdf_cnt(globalPrimitiveConsts->max_btn_holding_rdf_cnt());
        }
    }

    if (0 == decodedInputHolder.btn_d_level()) {
        nextChd->set_btn_d_holding_rdf_cnt(0);
    } else if (globalPrimitiveConsts->jammed_btn_holding_rdf_cnt() != currChd.btn_d_holding_rdf_cnt() && 0 < decodedInputHolder.btn_d_level()) {
        nextChd->set_btn_d_holding_rdf_cnt(currChd.btn_d_holding_rdf_cnt() + 1);
        if (nextChd->btn_d_holding_rdf_cnt() > globalPrimitiveConsts->max_btn_holding_rdf_cnt()) {
            nextChd->set_btn_d_holding_rdf_cnt(globalPrimitiveConsts->max_btn_holding_rdf_cnt());
        }
    }

    if (0 == decodedInputHolder.btn_e_level()) {
        nextChd->set_btn_e_holding_rdf_cnt(0);
    } else if (globalPrimitiveConsts->jammed_btn_holding_rdf_cnt() != currChd.btn_e_holding_rdf_cnt() && 0 < decodedInputHolder.btn_e_level()) {
        nextChd->set_btn_e_holding_rdf_cnt(currChd.btn_e_holding_rdf_cnt() + 1);
        if (nextChd->btn_e_holding_rdf_cnt() > globalPrimitiveConsts->max_btn_holding_rdf_cnt()) {
            nextChd->set_btn_e_holding_rdf_cnt(globalPrimitiveConsts->max_btn_holding_rdf_cnt());
        }
    }
}

void BaseBattle::prepareJumpStartup(const int currRdfId, const CharacterDownsync& currChd, const uint64_t currChdUd, const MassProperties& massProps, const Vec3& currChdFacing, const bool jumpTriggered, const bool slipJumpTriggered, CharacterDownsync* nextChd, const bool currEffInAir, const CharacterConfig* cc, const bool currParalyzed, const CH_COLLIDER_T* chCollider, const bool currInJumpStartUp, const bool currDashing, InputInducedMotion* ioInputInducedMotion) {
    if (0 < currChd.frames_to_recover()) {
        return;
    }

    if (TransformingInto == currChd.ch_state() || TransformingInto == nextChd->ch_state()) {
        return;
    }

    if (currParalyzed) {
        return;
    }

    if (0 == cc->jump_acc_mag_y()) {
        return;
    }

    bool currOrNextInSlipJump = (InAirIdle1BySlipJump == currChd.ch_state() || InAirIdle1BySlipJump == nextChd->ch_state());
    if ((jumpTriggered || currInJumpStartUp) && !currOrNextInSlipJump) {
        if ((OnWallIdle1 == currChd.ch_state() && jumpTriggered) || (InAirIdle1ByWallJump == currChd.ch_state() && !jumpTriggered)) {
            if (jumpTriggered) {
                // [REMINDER] If you are facing (-1, 0, 0) and keep pressing "effDx == +2" to jump on wall, then after "wall_jump_frames_to_recover" you'll smoothly pickup the wall jump inertia to move faster than ground in x-direction -- special clamping in "postStepSingleChdStateCorrection" and "clampChdVel" is applied.
                nextChd->set_ch_state(InAirIdle1ByWallJump);
                nextChd->set_frames_in_ch_state(0);
                ioInputInducedMotion->jumpTriggered = true;
            }
            float xfac = (0 > currChdFacing.GetX() ? 1 : -1);
            ioInputInducedMotion->forceCOM.SetX(massProps.mMass * xfac * cc->wall_jump_acc_mag_x());
            ioInputInducedMotion->forceCOM.SetY(massProps.mMass * (cc->wall_jump_acc_mag_y() - phySys->GetGravity().GetY()));
/*
#ifndef NDEBUG
            std::ostringstream oss;
            oss << "@currRdfId=" << currRdfId << ", bodyID=" << chCollider->GetBodyID().GetIndexAndSequenceNumber() << " wall jump pumped at pos=(" << currChd.x() << "," << currChd.y() << "), cs=" << currChd.ch_state() << ", fc=" << currChd.frames_in_ch_state() << ", currChdFacingX=" << currChdFacing.GetX() << ", jumpTriggered=" << jumpTriggered << ", vel=(" << currChd.vel_x() << ", " << currChd.vel_y() << "), forceCOM=(" << ioInputInducedMotion->forceCOM.GetX() << "," << ioInputInducedMotion->forceCOM.GetY() << ")" << std::endl;
            Debug::Log(oss.str(), DColor::Orange);
#endif
*/
        } else if (!currChd.omit_gravity()) {
            if (jumpTriggered) {
                if (InAirIdle1ByWallJump == currChd.ch_state() || InAirIdle1ByJump == currChd.ch_state() || InAirIdle1NoJump == currChd.ch_state() || InAirIdle1BySlipJump == currChd.ch_state()) {
                    bool canAirJump = (0 < currChd.remaining_air_jump_quota());
                    if (canAirJump) {
                        if (nullptr != battleSpecificConfig && 0 < battleSpecificConfig->character_overrides_size()) {
                            auto& characterOverrides = battleSpecificConfig->character_overrides(); 
                            if (characterOverrides.count(currChdUd)) {
                                auto& characterOverride = characterOverrides.at(currChdUd); 
                                if (SpecialMoveOverrideStatus::SmusLocked == characterOverride.air_jump_override()) {
                                    canAirJump = false;
                                }
                            }
                        }
                    }
                    if (canAirJump) {
                        nextChd->set_ch_state(InAirIdle2ByJump);
                        nextChd->set_frames_in_ch_state(0);
                        nextChd->set_remaining_air_jump_quota(currChd.remaining_air_jump_quota() - 1);
                        if (!cc->isolated_air_jump_and_dash_quota() && 0 < nextChd->remaining_air_dash_quota()) {
                            nextChd->set_remaining_air_dash_quota(nextChd->remaining_air_dash_quota() - 1);
                        }
                        ioInputInducedMotion->velCOM.SetY(0);
                    }
                } else {
                    nextChd->set_ch_state(InAirIdle1ByJump);
                    nextChd->set_frames_in_ch_state(0);
                }
                ioInputInducedMotion->jumpTriggered = true;
            }
            ioInputInducedMotion->forceCOM.SetY(massProps.mMass * (cc->jump_acc_mag_y() - phySys->GetGravity().GetY()));
/*
#ifndef NDEBUG
            if (InAirIdle2ByJump == nextChd->ch_state()) {          
                std::ostringstream oss;
                oss << "@currRdfId=" << currRdfId << ", bodyID=" << chCollider->GetBodyID().GetIndexAndSequenceNumber() << " air jump pumped at pos=(" << currChd.x() << "," << currChd.y() << "), cs=" << currChd.ch_state() << ", fc=" << currChd.frames_in_ch_state() << ", currChdFacingX=" << currChdFacing.GetX() << ", jumpTriggered=" << jumpTriggered << ", vel=(" << currChd.vel_x() << ", " << currChd.vel_y() << "), forceCOM=(" << ioInputInducedMotion->forceCOM.GetX() << "," << ioInputInducedMotion->forceCOM.GetY() << ")" << std::endl;
                Debug::Log(oss.str(), DColor::Orange);
            }
#endif
*/
        }
    } else if (slipJumpTriggered) { 
        nextChd->set_ch_state(InAirIdle1BySlipJump);
        nextChd->set_frames_in_ch_state(0);
        ioInputInducedMotion->slipJumpTriggered = true;
    } else if (!cc->omit_gravity() && cc->jump_holding_to_fly() && currChd.omit_gravity() && 0 >= currChd.flying_rdf_countdown()) {
        nextChd->set_ch_state(InAirIdle1NoJump);
        nextChd->set_omit_gravity(false);
    } else {
        // Intentionally left blank
    }
}

void BaseBattle::processInertiaWalkingHandleZeroEffDx(const int currRdfId, float dt, const CharacterDownsync& currChd, const MassProperties& massProps, const Vec3& currChdFacing, CharacterDownsync* nextChd, int effDy, const CharacterConfig* cc, bool effInAir, bool currParalyzed, const bool isInWalkingAtkAndNotRecovered, const uint64_t ud, const CH_COLLIDER_T* chCollider, const bool currDashing, InputInducedMotion* ioInputInducedMotion, bool& ioGravityDirty, bool& ioFrictionDirty) {
    if (currParalyzed) {
        return;
    }

    if (walkingSet.count(currChd.ch_state())) {
        if (0 == currChd.walkstopping_rdf_countdown()) {
            int effInertiaRdfCountdown = cc->walkstopping_inertia_rdf_count();
            if (0 >= effInertiaRdfCountdown) {
                effInertiaRdfCountdown = 8;
            }
            nextChd->set_walkstopping_rdf_countdown(effInertiaRdfCountdown);
        } else if (1 == currChd.walkstopping_rdf_countdown()) {
            if (!isInWalkingAtkAndNotRecovered) {
                nextChd->set_ch_state(Idle1);
            }
        }
    } 

    if (isCrouching(currChd.ch_state(), cc)) {
        biNoLock->SetFriction(chCollider->GetBodyID(), cWalkstoppingChFriction); // Will be resumed in "batchRemoveFromPhySysAndCache"
        ioFrictionDirty = true;
    } else if (0 < currChd.walkstopping_rdf_countdown()) {
        biNoLock->SetFriction(chCollider->GetBodyID(), cWalkstoppingChFriction); // Will be resumed in "batchRemoveFromPhySysAndCache"
        ioFrictionDirty = true;
    } else if (0 < currChd.fallstopping_rdf_countdown()) {
        biNoLock->SetFriction(chCollider->GetBodyID(), cFallstoppingChFriction); // Will be resumed in "batchRemoveFromPhySysAndCache"
        ioFrictionDirty = true;
    } else if (0 != currChd.ground_ud()) {
        uint64_t gudt = getUDT(currChd.ground_ud());
        if (UDT_PLAYER == gudt || UDT_NPC == gudt) {
            biNoLock->SetFriction(chCollider->GetBodyID(), 0); // Will be resumed in "batchRemoveFromPhySysAndCache"
            ioFrictionDirty = true;
        } else if (0 == currChd.ground_vel_x() && 0 != currChd.vel_x()) {
            // Being pushed away
            biNoLock->SetFriction(chCollider->GetBodyID(), cAntiPushChFriction); // Will be resumed in "batchRemoveFromPhySysAndCache"
            ioFrictionDirty = true;
        }
    }

    if (proactiveJumpingSet.count(currChd.ch_state())) {
        // [WARNING] In general a character is not permitted to just stop velX during proactive jumping.
        return;
    }

    if (0 < currChd.frames_to_recover() || effInAir || !cc->has_def1() || 0 >= effDy) {
        return;
    }

    nextChd->set_ch_state(Def1);
    if (Def1 == currChd.ch_state()) return;
    nextChd->set_frames_in_ch_state(0);
    nextChd->set_remaining_def1_quota(cc->default_def1_quota());
}

void BaseBattle::processInertiaWalking(const int currRdfId, float dt, const CharacterDownsync& currChd, const MassProperties& massProps, const Vec3& currChdFacing, CharacterDownsync* nextChd, bool currEffInAir, int effDx, int effDy, const CharacterConfig* cc, bool currParalyzed, bool currInBlockStun, const uint64_t ud, const CH_COLLIDER_T* chCollider, const bool currInJumpStartup, const bool nextInJumpStartup, const bool currDashing, InputInducedMotion* ioInputInducedMotion, bool& ioGravityDirty, bool& ioFrictionDirty) {
    if ((TransformingInto == currChd.ch_state() && 0 < currChd.frames_to_recover()) || (TransformingInto == nextChd->ch_state() && 0 < nextChd->frames_to_recover())) {
        return;
    }

    if (currInJumpStartup || nextInJumpStartup) {
        return;
    }

    if (currInBlockStun) {
        return;
    }
 
    bool exactTurningAround = false;
    if (0 > effDx * currChdFacing.GetX()) {
        exactTurningAround = true;
    }
    
    if (0 != effDx) {
        if (onWallSet.count(currChd.ch_state())) {
            ioInputInducedMotion->angVelCOM.SetY(0 > effDx ? cc->wall_ang_y_speed() : -cc->wall_ang_y_speed());
/*
#ifndef NDEBUG 
            std::ostringstream oss;
            oss << "@currRdfId=" << currRdfId << ", characterUd=" << ud << ": on wall currQ=(" << currChd.q_x() << ", " << currChd.q_y() << ", " << currChd.q_z() << ", " << currChd.q_w() << "), effDx=" << effDx << ", angVelCOM=(" << ioInputInducedMotion->angVelCOM.GetX() << ", " << ioInputInducedMotion->angVelCOM.GetY() << ", " << ioInputInducedMotion->angVelCOM.GetZ() << ") #1";
            Debug::Log(oss.str(), DColor::Orange);
#endif
*/
        } else {
            ioInputInducedMotion->angVelCOM.SetY(0 > effDx ? cc->ang_y_speed() : -cc->ang_y_speed());
        }
    }
    
    bool isInWalkingAtk = walkingAtkSet.count(currChd.ch_state());
    bool isInWalkingAtkAndNotRecovered = false;
    if (0 < currChd.frames_to_recover()) {
        if (!isInWalkingAtk) {
            return;
        } else {
            // Otherwise don't change nextChd->ch_state()
            isInWalkingAtkAndNotRecovered = true;
        }
    }
    bool hasNonZeroSpeed = !(0 == cc->speed() && 0 == currChd.speed());
    if (0 != effDx && hasNonZeroSpeed) {
        int xfac = (0 < effDx ? 1 : -1);
        float forceX = 0;   
        if (!currParalyzed) {
            if (InAirIdle1ByWallJump == currChd.ch_state()) {
                if (cc->wall_jump_frames_to_recover() < currChd.frames_in_ch_state()) {
                    const float wallJumpingFreeOpAccX = cc->wall_jump_acc_mag_x()*2;
                    ioInputInducedMotion->forceCOM.SetX(xfac * (wallJumpingFreeOpAccX * massProps.mMass));
                }
            } else if (isCrouching(currChd.ch_state(), cc)) {
                ioInputInducedMotion->forceCOM.SetX(0);
                biNoLock->SetFriction(chCollider->GetBodyID(), cWalkstoppingChFriction); // Will be resumed in "batchRemoveFromPhySysAndCache"
                ioFrictionDirty = true;
            } else {
                ioInputInducedMotion->forceCOM.SetX(xfac * (cc->acc_mag_x() * massProps.mMass));
                if (exactTurningAround && cc->has_turn_around_anim()) {
                    if (currEffInAir) {
                        nextChd->set_ch_state(InAirTurnAround);
                    } else {
                        nextChd->set_ch_state(TurnAround);
                    }
                } else {
                    if (!isInWalkingAtkAndNotRecovered) {
                        nextChd->set_ch_state(Walking);
                    }
                }
            }
        }
    } else {
        // 0 == effDx or speed is zero
        if (0 != effDx) {
            // false == hasNonZeroSpeed, no need to handle velocity lerping
        } else {
            processInertiaWalkingHandleZeroEffDx(currRdfId, dt, currChd, massProps, currChdFacing, nextChd, effDy, cc, currEffInAir, currParalyzed, isInWalkingAtkAndNotRecovered, ud, chCollider, currDashing, ioInputInducedMotion, ioGravityDirty, ioFrictionDirty);
        }
    }

    if (!ioInputInducedMotion->jumpTriggered && !currEffInAir && 0 > effDy && cc->crouching_enabled()) {
        // [WARNING] This particular condition is set to favor a smooth "Sliding -> CrouchIdle1" & "CrouchAtk1 -> CrouchAtk1" transitions, we couldn't use "0 == nextChd->frames_to_recover()" for checking here because "CrouchAtk1 -> CrouchAtk1" transition would break by 1 frame. 
        if (1 >= currChd.frames_to_recover()) {
            nextChd->set_ch_state(CrouchIdle1);
        }
    }

    bool recoverable1 = (isCrouching(currChd.ch_state(), cc) && 0 <= effDy);
    bool recoverable2 = atkedSet.count(currChd.ch_state());
    bool recoverable3 = noOpSet.count(currChd.ch_state());
    bool recoverable4 = !nonAttackingSet.count(currChd.ch_state());
    bool recoverable5 = currDashing;
    if (currChd.ch_state() == nextChd->ch_state() && (recoverable1 || recoverable2 || recoverable3 || recoverable4 || recoverable5)) {
        // Wrap up if none of the above conditions helped transit an attacking or attacked state into Idle1/Walking
        if (!isInWalkingAtkAndNotRecovered) {
            if (0 != ioInputInducedMotion->forceCOM.GetX()) {
                nextChd->set_ch_state(Walking);
            } else {
                nextChd->set_ch_state(Idle1);
            }
            ioInputInducedMotion->velCOM.Set(currChd.ground_vel_x(), currChd.ground_vel_y(), currChd.ground_vel_z());
        }
    }
}

void BaseBattle::processInertiaFlyingHandleZeroEffDxAndDy(const int currRdfId, float dt, const CharacterDownsync& currChd, const MassProperties& massProps, const Vec3& currChdFacing, CharacterDownsync* nextChd, const CharacterConfig* cc, bool currParalyzed, const uint64_t ud, const CH_COLLIDER_T* chCollider, const bool currDashing, InputInducedMotion* ioInputInducedMotion, bool& ioGravityDirty, bool& ioFrictionDirty) {
    // TBD
    return;
}

void BaseBattle::processInertiaFlying(const int currRdfId, float dt, const CharacterDownsync& currChd, const MassProperties& massProps, const Vec3& currChdFacing, CharacterDownsync* nextChd, int effDx, int effDy, const CharacterConfig* cc, bool currParalyzed, bool currInBlockStun, const uint64_t ud, const CH_COLLIDER_T* chCollider, const bool currInJumpStartup, const bool nextInJumpStartup, const bool currDashing, InputInducedMotion* ioInputInducedMotion, bool& ioGravityDirty, bool& ioFrictionDirty) {
    // TBD
    return;
}

bool BaseBattle::addBlHitToNextFrame(const int currRdfId, RenderFrame* nextRdf, const Bullet* referenceBullet, const Vec3& newPos) {
    uint32_t oldNextRdfBulletCount = mNextRdfBulletCount.fetch_add(1, std::memory_order_relaxed);
    if (oldNextRdfBulletCount >= nextRdf->bullets_size()) {
#ifndef  NDEBUG
        std::ostringstream oss;
        oss << "@currRdfId=" << currRdfId << ", bulletId=" << referenceBullet->id() << ": bullet overwhelming when adding vanishing#1";
        Debug::Log(oss.str(), DColor::Orange);
#endif // ! NDEBUG
        --mNextRdfBulletCount;
        return false;
    }

    float newOriginatedX = newPos.GetX(), newOriginatedY = newPos.GetY(), newX = newPos.GetX(), newY = newPos.GetY();
    float dstQx = referenceBullet->q_x();
    float dstQy = referenceBullet->q_y();
    float dstQz = referenceBullet->q_z();
    float dstQw = referenceBullet->q_w();

    auto initBlState = BulletState::Hit;
    int initFramesInBlState = 0;

    int oldNextRdfBulletIdCounter = mNextRdfBulletIdCounter.fetch_add(1, std::memory_order_relaxed);
    auto nextBl = nextRdf->mutable_bullets(oldNextRdfBulletCount);
    CopyBullet(referenceBullet, nextBl);
    nextBl->set_id(oldNextRdfBulletIdCounter);
    nextBl->set_originated_render_frame_id(currRdfId);
    nextBl->set_bl_state(initBlState);
    nextBl->set_frames_in_bl_state(initFramesInBlState);
    nextBl->set_originated_x(newOriginatedX);
    nextBl->set_originated_y(newOriginatedY);
    nextBl->set_x(newX);
    nextBl->set_y(newY);

    return true;
}


bool BaseBattle::addNewBulletToNextFrame(const int currRdfId, const CharacterDownsync& currChd, const Vec3& currChdFacing, const CharacterConfig* cc, bool currParalyzed, bool currEffInAir, const Skill* skillConfig, int activeSkillHit, uint32_t activeSkillId, RenderFrame* nextRdf, const Bullet* referenceBullet, const BulletConfig* referenceBulletConfig, uint64_t offenderUd, int bulletTeamId) {
    if (globalPrimitiveConsts->no_skill_hit() == activeSkillHit || activeSkillHit > skillConfig->hits_size()) return false;
    uint32_t oldNextRdfBulletCount = mNextRdfBulletCount.fetch_add(1, std::memory_order_relaxed);
    if (oldNextRdfBulletCount >= globalPrimitiveConsts->default_prealloc_bullet_capacity()) {
#ifndef  NDEBUG
        std::ostringstream oss;
        oss << "@currRdfId=" << currRdfId << ", offenderUd=" << offenderUd << ", oldNextRdfBulletCount=" << oldNextRdfBulletCount << ": bullet overwhelming#1";
        Debug::Log(oss.str(), DColor::Orange);
#endif // ! NDEBUG
        --mNextRdfBulletCount;
        return false;
    }

    const BulletConfig& bulletConfig = skillConfig->hits(activeSkillHit - 1);
    if (0 == bulletConfig.hitbox_half_size_x() * bulletConfig.hitbox_half_size_y()) {
        // If there's no hit box, don't waste space on "nextRdf->bullets"!
        return true;
    }

    const BulletConfig* prevBulletConfig = referenceBulletConfig;
    if (nullptr == prevBulletConfig) {
        if (2 <= activeSkillHit) {
            const BulletConfig& defaultBulletConfig = skillConfig->hits(activeSkillHit - 2);
            prevBulletConfig = &defaultBulletConfig;
        }
    }

    JPH::Quat offenderEffQ = 0 < currChdFacing.GetX() ? cIdentityQ : cTurnbackAroundYAxis;
    if (OnWallAtk1 == skillConfig->bound_ch_state() && (BulletType::Melee != bulletConfig.b_type())) {
        offenderEffQ = cTurnbackAroundYAxis*offenderEffQ;
    }
    JPH::Quat offenderEffAimingQ = JPH::Quat(currChd.aiming_q_x(), currChd.aiming_q_y(), currChd.aiming_q_z(), currChd.aiming_q_w())*offenderEffQ;

    Vec3 blInitOffset(bulletConfig.hitbox_offset_x(), bulletConfig.hitbox_offset_y(), 0);
    Vec3 blEffOffset = offenderEffAimingQ*blInitOffset; 
    float newOriginatedX = (nullptr == referenceBullet || bulletConfig.use_ch_offset_regardless_of_emission_mh()) ? (currChd.x() + blEffOffset.GetX()) : referenceBullet->originated_x();
    float newOriginatedY = (nullptr == referenceBullet || bulletConfig.use_ch_offset_regardless_of_emission_mh()) ? (currChd.y() + blEffOffset.GetY()) : referenceBullet->originated_y();
    float newX = (nullptr == referenceBullet || (BulletType::Melee == bulletConfig.b_type() && MultiHitType::FromVisionSeekOrDefault != bulletConfig.mh_type())) ? currChd.x() + blEffOffset.GetX() : referenceBullet->x() + blEffOffset.GetX();
    float newY = (nullptr == referenceBullet || (BulletType::Melee == bulletConfig.b_type() && MultiHitType::FromVisionSeekOrDefault != bulletConfig.mh_type())) ? currChd.y() + blEffOffset.GetY() : referenceBullet->y() + blEffOffset.GetY();

    if (nullptr != prevBulletConfig && prevBulletConfig->ground_impact_melee_collision()) {
        newY = currChd.y() + bulletConfig.hitbox_offset_y();
    } else if (BulletType::GroundWave == bulletConfig.b_type()) {
        // TODO: Lower "newY" if on slope and facing down?
    }

    // To favor startup vfx display which is based on bullet originatedVx&originatedVy.
    if (nullptr != referenceBulletConfig && MultiHitType::FromVisionSeekOrDefault == referenceBulletConfig->mh_type()) {
        newX = currChd.x() + bulletConfig.hitbox_offset_x();
        newY = currChd.y() + bulletConfig.hitbox_offset_y();
        newOriginatedX = newX;
        newOriginatedY = newY;
    }

    JPH::Quat blEffQ = JPH::Quat::sIdentity();
    if (bulletConfig.mh_inherits_spin() && nullptr != referenceBullet) {
        blEffQ.Set(referenceBullet->q_x(), referenceBullet->q_y(), referenceBullet->q_z(), referenceBullet->q_w());
    } else {
        if (bulletConfig.has_init_q()) {
            auto& init_q = bulletConfig.init_q();
            blEffQ = JPH::Quat(init_q.x(), init_q.y(), init_q.z(), init_q.w())*offenderEffAimingQ; 
        } else {
            blEffQ = offenderEffAimingQ; 
        }
    }

    auto initBlState = BulletState::StartUp;
    int initFramesInBlState = 0;
    if (bulletConfig.mh_inherits_frames_in_bl_state() && nullptr != referenceBullet) {
        initBlState = referenceBullet->bl_state();
        initFramesInBlState = referenceBullet->frames_in_bl_state() + 1;
        // In this case, we ignore "hitbox offsets"
        newX = referenceBullet->x();
        newY = referenceBullet->y();
        newOriginatedX = newX;
        newOriginatedY = newY;
    }

    int oldNextRdfBulletIdCounter = mNextRdfBulletIdCounter.fetch_add(1, std::memory_order_relaxed);
    Bullet* nextBl = oldNextRdfBulletCount < nextRdf->bullets_size() ? nextRdf->mutable_bullets(oldNextRdfBulletCount) : nextRdf->add_bullets();
    nextBl->Clear(); // [REMINDER] There's no embedded pb-message field in "Bullet", feel free to clear.
    nextBl->set_id(oldNextRdfBulletIdCounter);
    nextBl->set_originated_render_frame_id(currRdfId);
    nextBl->set_bl_state(initBlState);
    nextBl->set_frames_in_bl_state(initFramesInBlState);
    auto ud = calcUserData(*nextBl);
    nextBl->set_offender_ud(offenderUd);
    nextBl->set_ud(ud);
    nextBl->set_originated_x(newOriginatedX);
    nextBl->set_originated_y(newOriginatedY);
    nextBl->set_originated_z(0);
    nextBl->set_x(newX);
    nextBl->set_y(newY);
    nextBl->set_z(0);
    nextBl->set_q_x(blEffQ.GetX());
    nextBl->set_q_y(blEffQ.GetY());
    nextBl->set_q_z(blEffQ.GetZ());
    nextBl->set_q_w(blEffQ.GetW());
    nextBl->set_team_id(currChd.bullet_team_id());

    Vec3Arg blInitVelocity(bulletConfig.speed(), 0, 0);
    auto blEffVelocity = blEffQ*blInitVelocity;
    nextBl->set_vel_x(blEffVelocity.GetX());
    nextBl->set_vel_y(blEffVelocity.GetY());
    nextBl->set_vel_z(0);
    nextBl->set_skill_id(activeSkillId);
    nextBl->set_active_skill_hit(activeSkillHit);
    nextBl->set_repeat_quota_left(bulletConfig.repeat_quota());
    nextBl->set_hit_on_ifc(bulletConfig.ifc());

/*
#ifndef  NDEBUG
    std::ostringstream oss;
    oss << "@currRdfId=" << currRdfId << ", added new bullet with bulletId=" << nextBl->id() << ", pos=(" << nextBl->x() << ", " << nextBl->y() << ", " << nextBl->z() << "), vel=(" << nextBl->vel_x() << ", " << nextBl->vel_y() << ", " << nextBl->vel_z() << ")";
    Debug::Log(oss.str(), DColor::Orange);
#endif // ! NDEBUG
*/
    if (0 < bulletConfig.simultaneous_multi_hit_cnt() && activeSkillHit < skillConfig->hits_size()) {
        return addNewBulletToNextFrame(currRdfId, currChd, currChdFacing, cc, currParalyzed, currEffInAir, skillConfig, activeSkillHit + 1, activeSkillId, nextRdf, referenceBullet, referenceBulletConfig, offenderUd, bulletTeamId);
    } else {
        return true;
    }
}

bool BaseBattle::addNewNpcToNextFrame(int currRdfId, float x, float y, float qx, float qy, float qz, float qw, uint32_t chSpeciesId, int teamId, NpcGoal initGoal, RenderFrame* nextRdf, uint32_t publishingToTriggerIdUponExhausted, uint64_t waveNpcKilledMaskCounter, uint32_t subscribesToTriggerId) {
    uint32_t oldNextRdfNpcCount = mNextRdfNpcCount.fetch_add(1, std::memory_order_relaxed);
    if (oldNextRdfNpcCount >= nextRdf->bullets_size()) {
        --mNextRdfNpcCount;
        return false;
    }
    const CharacterConfig* chConfig = getCc(chSpeciesId);
    CharacterState initChState = chConfig->omit_gravity() ? Walking : Idle1;
    int oldNextRdfNpcIdCounter = mNextRdfNpcIdCounter.fetch_add(1, std::memory_order_relaxed);
    auto nextNpc = oldNextRdfNpcCount < nextRdf->npcs_size() ? nextRdf->mutable_npcs(oldNextRdfNpcCount) : nextRdf->add_npcs();

    nextNpc->Clear(); // [WARNING] This is recursive, and would NOT free memory immediately to save CPU time (i.e. Arena allocation/freeing is used on the whole "rdfBuffer" by "pbRdfAllocator"). 
    
    CharacterDownsync* nextChd = nextNpc->mutable_chd();
    
    nextNpc->set_id(oldNextRdfNpcIdCounter);
    nextNpc->set_activated_rdf_id(currRdfId);
    nextNpc->set_goal_as_npc(initGoal);
    nextNpc->set_publishing_to_trigger_id_upon_exhausted(publishingToTriggerIdUponExhausted);
    nextNpc->set_subscribes_to_trigger_id(subscribesToTriggerId);
    nextNpc->set_publishing_mask_upon_exhausted(waveNpcKilledMaskCounter);

    nextChd->set_species_id(chSpeciesId);
    nextChd->set_x(x);
    nextChd->set_y(y - chConfig->shrinked_capsule_half_height());
    nextChd->set_z(0);

    nextChd->set_q_x(qx);
    nextChd->set_q_y(qy);
    nextChd->set_q_z(qz);
    nextChd->set_q_w(qw);

    nextChd->set_aiming_q_x(0);
    nextChd->set_aiming_q_y(0);
    nextChd->set_aiming_q_z(0);
    nextChd->set_aiming_q_w(1);

    nextChd->set_bullet_team_id(teamId);

    nextChd->set_ground_vel_x(0);
    nextChd->set_ground_vel_y(0);
    nextChd->set_ground_vel_z(0);

    nextChd->set_omit_gravity(false); // [TODO] Make this configurable
    nextChd->set_ch_state(InAirIdle1NoJump);
    nextChd->set_frames_in_ch_state(0);
    nextChd->set_frames_to_recover(0);
    nextChd->set_frames_invinsible(8); // [TODO] Replace this hardcoded value with some flexibly configured value

    nextChd->set_hp(chConfig->hp());
    nextChd->set_mp(chConfig->mp());

    Quat nextChdQ;
    Vec3 nextChdFacing;
    BaseBattleCollisionFilter::calcChdFacing((*nextChd), nextChdQ, nextChdFacing);
    
    switch (initGoal) {
        case NPatrol:
        case NPathPatrol: {
            // [TODO] What if "(true == cc.omit_gravity() || true == nextChd.omit_gravity())"
            if (0 < nextChdFacing.GetX()) {
                nextNpc->set_cached_cue_cmd(3);
                nextChd->set_vel_x(chConfig->speed()*nextChdFacing.GetX());
                nextChd->set_vel_y(chConfig->speed()*nextChdFacing.GetY());
                nextChd->set_vel_z(chConfig->speed()*nextChdFacing.GetZ());
            } else if (0 > nextChdFacing.GetX()) {
                nextNpc->set_cached_cue_cmd(4);
                nextChd->set_vel_x(chConfig->speed()*nextChdFacing.GetX());
                nextChd->set_vel_y(chConfig->speed()*nextChdFacing.GetY());
                nextChd->set_vel_z(chConfig->speed()*nextChdFacing.GetZ());
            } else {
                // [TODO] Log this edge case.
                nextNpc->set_cached_cue_cmd(0);
                nextChd->set_vel_x(0);
                nextChd->set_vel_y(0);
                nextChd->set_vel_z(0);
            } 
            break;
        }
        default: {
            nextNpc->set_cached_cue_cmd(0);
            nextChd->set_vel_x(0);
            nextChd->set_vel_y(0);
            nextChd->set_vel_z(0);
            break;
        }
    }

    ResetInventory(chConfig, nullptr, nextChd); // [TODO] There's currently no way to apply overrides to spawned NPCs. 
    
    return true;
}

void BaseBattle::elapse1RdfForRdf(const int currRdfId, RenderFrame* nextRdf) {
    for (int i = 0; i < playersCnt; i++) {
        auto player = nextRdf->mutable_players(i);
        const CharacterConfig* cc = getCc(player->chd().species_id());
        elapse1RdfForPlayerChd(currRdfId, player, cc);
    }

    for (int i = 0; i < nextRdf->npcs_size(); i++) {
        auto npc = nextRdf->mutable_npcs(i);
        if (globalPrimitiveConsts->terminating_character_id() == npc->id()) break;
        const CharacterConfig* cc = getCc(npc->chd().species_id());
        elapse1RdfForNpcChd(currRdfId, npc, cc);
    }

    for (int i = 0; i < nextRdf->bullets_size(); i++) {
        auto bl = nextRdf->mutable_bullets(i);
        if (globalPrimitiveConsts->terminating_bullet_id() == bl->id()) break;
        const Skill* skill = nullptr;
        const BulletConfig* bulletConfig = nullptr;
        FindBulletConfig(bl->skill_id(), bl->active_skill_hit(), skill, bulletConfig);
        if (nullptr == skill || nullptr == bulletConfig) {
            continue;
        }
        elapse1RdfForBl(currRdfId, bl, skill, bulletConfig);
    }

    for (int i = 0; i < nextRdf->dynamic_traps_size(); i++) {
        auto tp = nextRdf->mutable_dynamic_traps(i);
        if (globalPrimitiveConsts->terminating_trap_id() == tp->id()) break;
        elapse1RdfForTrap(tp);
    }

    for (int i = 0; i < nextRdf->triggers_size(); i++) {
        auto tr = nextRdf->mutable_triggers(i);
        if (globalPrimitiveConsts->terminating_trigger_id() == tr->id()) break;
        elapse1RdfForTrigger(tr);
    }

    for (int i = 0; i < nextRdf->pickables_size(); i++) {
        auto pk = nextRdf->mutable_pickables(i);
        if (globalPrimitiveConsts->terminating_pickable_id() == pk->id()) break;
        elapse1RdfForPickable(pk);
    }
}

void BaseBattle::elapse1RdfForPlayerChd(const int currRdfId, PlayerCharacterDownsync* playerChd, const CharacterConfig* cc) {
    auto chd = playerChd->mutable_chd();
    const uint64_t ud = calcUserData(*playerChd);
    elapse1RdfForChd(currRdfId, ud, chd, cc);
    playerChd->set_not_enough_mp_hint_rdf_countdown(0 < playerChd->not_enough_mp_hint_rdf_countdown() ? playerChd->not_enough_mp_hint_rdf_countdown() - 1 : 0);

}

void BaseBattle::elapse1RdfForNpcChd(const int currRdfId, NpcCharacterDownsync* npcChd, const CharacterConfig* cc) {
    auto chd = npcChd->mutable_chd();
    const uint64_t ud = calcUserData(*npcChd);
    elapse1RdfForChd(currRdfId, ud, chd, cc);
    npcChd->set_frames_in_patrol_cue(0 < npcChd->frames_in_patrol_cue() ? npcChd->frames_in_patrol_cue() - 1 : 0);
}

void BaseBattle::elapse1RdfForChd(const int currRdfId, const uint64_t ud, CharacterDownsync* cd, const CharacterConfig* cc) {
    cd->set_frames_to_recover((0 < cd->frames_to_recover() ? cd->frames_to_recover() - 1 : 0));
    cd->set_frames_in_ch_state(cd->frames_in_ch_state() + 1);
    if (globalPrimitiveConsts->terminating_lower_part_rdf_cnt() != cd->lower_part_rdf_cnt()) {  
        cd->set_lower_part_rdf_cnt(cd->lower_part_rdf_cnt() + 1);
    }
    cd->set_walkstopping_rdf_countdown(0 < cd->walkstopping_rdf_countdown() ? cd->walkstopping_rdf_countdown() - 1 : 0);
    cd->set_fallstopping_rdf_countdown(0 < cd->fallstopping_rdf_countdown() ? cd->fallstopping_rdf_countdown() - 1 : 0);
    cd->set_frames_invinsible(0 < cd->frames_invinsible() ? cd->frames_invinsible() - 1 : 0);
    cd->set_mp_regen_rdf_countdown(0 < cd->mp_regen_rdf_countdown() ? cd->mp_regen_rdf_countdown() - 1 : 0);
    auto mp = cd->mp();
    if (0 >= cd->mp_regen_rdf_countdown()) {
        mp += cc->mp_regen_per_interval();
        if (mp >= cc->mp()) {
            mp = cc->mp();
        }
        cd->set_mp_regen_rdf_countdown(cc->mp_regen_interval());
    }
    cd->set_new_birth_rdf_countdown((0 < cd->new_birth_rdf_countdown() ? cd->new_birth_rdf_countdown() - 1 : 0));
    cd->set_damaged_hint_rdf_countdown((0 < cd->damaged_hint_rdf_countdown() ? cd->damaged_hint_rdf_countdown() - 1 : 0));
    if (0 >= cd->damaged_hint_rdf_countdown()) {
        cd->set_damaged_elemental_attrs(0);
    }
    cd->set_combo_frames_remained((0 < cd->combo_frames_remained() ? cd->combo_frames_remained() - 1 : 0));
    if (0 >= cd->combo_frames_remained()) {
        cd->set_combo_hit_cnt(0);
    }

    cd->set_flying_rdf_countdown((globalPrimitiveConsts->max_flying_rdf_cnt() == cc->flying_quota_rdf_cnt() ? globalPrimitiveConsts->max_flying_rdf_cnt() : (0 < cd->flying_rdf_countdown() ? cd->flying_rdf_countdown() - 1 : 0)));
 
    for (int i = 0; i < cd->bullet_immune_records_size(); ++i) {
        auto cand = cd->mutable_bullet_immune_records(i);
        if (globalPrimitiveConsts->terminating_bullet_id() == cand->bullet_id()) break;
        if (0 >= cand->remaining_lifetime_rdf_count()) {
            continue;
        }
        auto newRemainingLifetimeRdfCount = cand->remaining_lifetime_rdf_count() - 1;
        cand->set_remaining_lifetime_rdf_count(newRemainingLifetimeRdfCount);
    }

    if (cd->has_atk1_magazine()) {
        elapse1RdfForIvSlot(cd->mutable_atk1_magazine(), &(cc->atk1_magazine()));
    }

    if (cd->has_super_atk_gauge()) {
        elapse1RdfForIvSlot(cd->mutable_super_atk_gauge(), &(cc->super_atk_gauge()));
    }

    for (int i = 0; i < cd->inventory_slots_size(); ++i) {
        InventorySlot* cand = cd->mutable_inventory_slots(i);
        if (InventorySlotStockType::NoneIv == cand->stock_type()) break;
        const InventorySlotConfig* ivsConfig = &(cc->init_inventory_slots(i));
        if (nullptr != battleSpecificConfig && 0 < battleSpecificConfig->character_overrides_size()) {
            auto& characterOverrides = battleSpecificConfig->character_overrides(); 
            if (characterOverrides.count(ud)) {
                auto& characterOverride = characterOverrides.at(ud); 
                if (0 < characterOverride.init_inventory_slots_size()) {
                    ivsConfig = &(characterOverride.init_inventory_slots(i));  
                }
            }
        }
        elapse1RdfForIvSlot(cand, ivsConfig);
    }

    leftShiftDeadBlImmuneRecords(currRdfId, cd);
    leftShiftDeadIvSlots(currRdfId, cd);
    leftShiftDeadBuffs(currRdfId, cd);
    leftShiftDeadDebuffs(currRdfId, cd);
}

void BaseBattle::elapse1RdfForBl(const int currRdfId, Bullet* bl, const Skill* skill, const BulletConfig* bulletConfig) {
    int newFramesInBlState = bl->frames_in_bl_state() + 1;
    switch (bl->bl_state())
    {
    case BulletState::StartUp:
        if (newFramesInBlState > bulletConfig->startup_frames()) {
            bl->set_bl_state(BulletState::Active);
            newFramesInBlState = 0;
/*
#ifndef  NDEBUG
            std::ostringstream oss;
            oss << "bulletId=" << bl->id() << ", originatedRenderFrameId=" << bl->originated_render_frame_id() << " just became active at currRdfId=" << currRdfId+1 << ", pos=(" << bl->x() << ", " << bl->y() << ", " << bl->z() << "), vel=(" << bl->vel_x() << ", " << bl->vel_y() << ", " << bl->vel_z() << ")";
            Debug::Log(oss.str(), DColor::Orange);
#endif // ! NDEBUG
*/
        }
        break;
    case BulletState::Active:
        if (newFramesInBlState > bulletConfig->active_frames()) {
            bl->set_bl_state(BulletState::Vanishing);
/*
#ifndef  NDEBUG
                std::ostringstream oss;
                oss << "bulletId=" << bl->id() << ", originatedRenderFrameId=" << bl->originated_render_frame_id() << " just became vanishing at currRdfId=" << currRdfId+1 << ", pos=(" << bl->x() << ", " << bl->y() << ", " << bl->z() << "), vel=(" << bl->vel_x() << ", " << bl->vel_y() << ", " << bl->vel_z() << ")";
                Debug::Log(oss.str(), DColor::Orange);
#endif // ! NDEBUG
*/
            newFramesInBlState = 0;
            bl->set_vel_x(0);
            bl->set_vel_y(0);
        }
        break;
    default:
        break;
    }
    bl->set_frames_in_bl_state(newFramesInBlState);
}

void BaseBattle::elapse1RdfForTrap(Trap* tp) {
    int newFramesInState = tp->frames_in_trap_state() + 1;
    tp->set_frames_in_trap_state(newFramesInState);
}

void BaseBattle::elapse1RdfForTrigger(Trigger* tr) {
    int newFramesToFire = tr->frames_to_fire() - 1; 
    if (newFramesToFire < 0) {
        newFramesToFire = 0;
    }
    tr->set_frames_to_fire(newFramesToFire);

    int newFramesToRecover = tr->frames_to_recover() - 1; 
    if (newFramesToRecover < 0) {
        newFramesToRecover = 0;
    }
    tr->set_frames_to_recover(newFramesToRecover);

    int newFramesInState = tr->frames_in_state() + 1;
    tr->set_frames_in_state(newFramesInState);
}

void BaseBattle::elapse1RdfForPickable(Pickable* pk) {
    pk->set_frames_in_pk_state(pk->frames_in_pk_state()+1);
    int newLifetimeRdfCount = pk->remaining_lifetime_rdf_count() - 1;
    if (0 > newLifetimeRdfCount) {
        newLifetimeRdfCount = 0;
        if (PickableState::PIdle == pk->pk_state()) {
            pk->set_pk_state(PickableState::PDisappearing);
            pk->set_remaining_lifetime_rdf_count(globalPrimitiveConsts->default_pickable_disappearing_anim_frames());
        }
    }
    pk->set_remaining_lifetime_rdf_count(newLifetimeRdfCount);
}

void BaseBattle::elapse1RdfForIvSlot(InventorySlot* ivs, const InventorySlotConfig* ivsConfig) {
    int newFramesToRecover = ivs->frames_to_recover();
    int newQuota = ivs->quota();
    int newGaugeCharged = ivs->gauge_charged();
    switch (ivs->stock_type()) {
        case PocketIv:
        break;
        case TimedIv:
            newQuota -= 1;
            if (0 > newQuota) {
                newQuota = 0;
            }
        break;
        case TimedMagazineIv:
            if (0 >= newQuota) {
                bool isToRefill = (1 == newFramesToRecover);
                newFramesToRecover -= 1;
                if (0 >= newFramesToRecover) {
                    newFramesToRecover = 0;
                }
                if (isToRefill) {
                    newQuota = ivsConfig->quota();
                } 
            }
        break;
        default:
        break;
    }
    ivs->set_frames_to_recover(newFramesToRecover);
    ivs->set_quota(newQuota);
    ivs->set_gauge_charged(newGaugeCharged);
}

InputFrameDownsync* BaseBattle::getOrPrefabInputFrameDownsync(int inIfdId, uint32_t inSingleJoinIndex, uint64_t inSingleInput, bool fromUdp, bool fromTcp, bool& outExistingInputMutated) {
    if (0 >= inSingleJoinIndex) {
        return nullptr;
    }

    if (inIfdId < ifdBuffer.StFrameId) {
        // Obsolete #1
        return nullptr;
    }

    int inSingleJoinIndexArrIdx = (inSingleJoinIndex - 1);
    uint64_t inSingleJoinMask = CalcJoinIndexMask(inSingleJoinIndex);
    auto existingInputFrame = ifdBuffer.GetByFrameId(inIfdId);
    auto previousInputFrameDownsync = ifdBuffer.GetByFrameId(inIfdId - 1);

    bool alreadyTcpConfirmed = existingInputFrame && (0 < (existingInputFrame->confirmed_list() & inSingleJoinMask));
    if (alreadyTcpConfirmed) {
        // Obsolete #2
        return existingInputFrame;
    }

    // By now "false == alreadyTcpConfirmed" 
    bool alreadyUdpConfirmed = existingInputFrame && (0 < (existingInputFrame->udp_confirmed_list() & inSingleJoinMask));
    if (alreadyUdpConfirmed) {
        if (!fromTcp) {
            // [WARNING] Only Tcp incoming inputs can override UdpConfirmed inputs
            return existingInputFrame;
        }
    }

    if (nullptr != existingInputFrame) {
        auto existingSingleInputAtSameIfd = existingInputFrame->input_list(inSingleJoinIndexArrIdx);
        existingInputFrame->set_input_list(inSingleJoinIndexArrIdx, inSingleInput);
        if (fromUdp) {
            existingInputFrame->set_udp_confirmed_list(existingInputFrame->udp_confirmed_list() | inSingleJoinMask);
        }
        if (fromTcp) {
            existingInputFrame->set_confirmed_list(existingInputFrame->confirmed_list() | inSingleJoinMask);
        }
        if (existingSingleInputAtSameIfd != inSingleInput) {
            outExistingInputMutated = true;
        }

        return existingInputFrame;
    }

    JPH_ASSERT(ifdBuffer.EdFrameId <= inIfdId); // Hence any "k" suffices "playerInputFrontIds[k] < ifdBuffer.EdFrameId <= inIfdId"

    memset(prefabbedInputList.data(), 0, playersCnt * sizeof(uint64_t));
    for (int k = 0; k < playersCnt; ++k) {
        if (0 < (inactiveJoinMask & CalcJoinIndexMask(k + 1))) {
            prefabbedInputList[k] = 0;
        } else {
            prefabbedInputList[k] = playerInputFronts[k];
        }
    }

    prefabbedInputList[inSingleJoinIndexArrIdx] = inSingleInput;

    uint64_t initConfirmedList = 0;
    if (fromUdp || fromTcp) {
        initConfirmedList = inSingleJoinMask;
    }

    while (ifdBuffer.EdFrameId <= inIfdId) {
        // Fill the gap
        auto ifdHolder = ifdBuffer.DryPut();
        JPH_ASSERT(nullptr != ifdHolder);
        ifdHolder->set_confirmed_list(0u); // To avoid RingBuffer reuse contamination.
        ifdHolder->set_udp_confirmed_list(0u);

        auto inputList = ifdHolder->mutable_input_list();
        inputList->Clear(); // [REMINDER] This is just a list of integers, feel free to clear.
        for (auto& prefabbedInput : prefabbedInputList) {
            inputList->Add(prefabbedInput);
        }
    }

    auto ret = ifdBuffer.GetLast();
    if (fromTcp) {
        ret->set_confirmed_list(initConfirmedList);
    }
    if (fromUdp) {
        ret->set_udp_confirmed_list(initConfirmedList);
    }

    return ret;
}

void BaseBattle::batchPutIntoPhySysFromCache(const int currRdfId, const RenderFrame* currRdf, RenderFrame* nextRdf) {
    /** [WARNING] 
    Intentionally left "bi->SetLinearVelocity(...)" to the multi-threaded "xxx-pre-physics-update" jobs, because changes to "LinearVelocity" and "AngularVelocity" DON'T induce any "BroadPhaseQuadTree AABB change". 

    Instead "biNoLock->SetPositionAndRotation(...)" is called here within this single-threaded "batchPutIntoPhySysFromCache", because they will induce "BroadPhaseQuadTree AABB change"s. My expectation is to reduce "multi-threaded randomness of BroadPhaseQuadTree AABB change" as much as possible. 
    */
    bodyIDsToActivate.clear();
    for (int i = 0; i < playersCnt; i++) {
        const PlayerCharacterDownsync& currPlayer = currRdf->players(i);
        const CharacterDownsync& currChd = currPlayer.chd();

        PlayerCharacterDownsync* nextPlayer = nextRdf->mutable_players(i);

        const CharacterConfig* cc = getCc(currChd.species_id());
        auto ud = calcUserData(currPlayer);
        CH_COLLIDER_T* chCollider = getOrCreateCachedPlayerCollider_NotThreadSafe(ud, currPlayer, cc, nextPlayer);
        transientUdToCollisionUdHolder[ud] = collisionUdHolderStockCache.Take_ThreadSafe();
        transientUdToInputInducedMotion[ud] = inputInducedMotionStockCache.Take_ThreadSafe();

        auto bodyID = chCollider->GetBodyID();
        bodyIDsToActivate.push_back(bodyID);
        // [REMINDER] "CharacterVirtual" maintains its own "mLinearVelocity" (https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Physics/Character/CharacterVirtual.h#L709) -- and experimentally setting velocity of its "mInnerBodyID" doesn't work (if "mInnerBodyID" was even set).
    }

    for (int i = 0; i < currRdf->npcs_size(); i++) {
        const NpcCharacterDownsync& currNpc = currRdf->npcs(i);
        if (globalPrimitiveConsts->terminating_character_id() == currNpc.id()) break;
        NpcCharacterDownsync* nextNpc = nextRdf->mutable_npcs(i); // [WARNING] By reaching here, we haven't executed "leftShiftDeadNpcs", hence the indices of "currRdf->npcs" and "nextRdf->npcs" are FULLY ALIGNED.
        const CharacterDownsync& currChd = currNpc.chd();
        const CharacterConfig* cc = getCc(currChd.species_id());
        auto ud = calcUserData(currNpc);
        CH_COLLIDER_T* chCollider = getOrCreateCachedNpcCollider_NotThreadSafe(ud, currNpc, cc, nextNpc);
        transientUdToCollisionUdHolder[ud] = collisionUdHolderStockCache.Take_ThreadSafe();
        transientUdToInputInducedMotion[ud] = inputInducedMotionStockCache.Take_ThreadSafe();

        auto bodyID = chCollider->GetBodyID();
        bodyIDsToActivate.push_back(bodyID);
    }

    /*
    [WARNING]

    The callstack "getOrCreateCachedCharacterCollider_NotThreadSafe -> createDefaultCharacterCollider" implicitly adds the "JPH::Character.mBodyID" into "BroadPhase", therefore "bodyIDsToAdd" starts here.
    */ 
    bodyIDsToAdd.clear();
    for (int i = 0; i < currRdf->bullets_size(); i++) {
        const Bullet& currBl = currRdf->bullets(i);
        if (globalPrimitiveConsts->terminating_bullet_id() == currBl.id()) break;
        Bullet* nextBl = nextRdf->mutable_bullets(i); // [WARNING] By reaching here, we haven't executed "leftShiftDeadBullets", hence the indices of "currRdf->bullets" and "nextRdf->bullets" are FULLY ALIGNED.
        auto ud = calcUserData(currBl);
        transientUdToCurrBl[ud] = &currBl;
        transientUdToNextBl[ud] = nextBl;
        const Skill* skill = nullptr;
        const BulletConfig* bulletConfig = nullptr;
        FindBulletConfig(currBl.skill_id(), currBl.active_skill_hit(), skill, bulletConfig);
        if (BulletState::Active == currBl.bl_state()) {
            Vec3 newPos(currBl.x(), currBl.y(), currBl.z());
            Quat newRot(currBl.q_x(), currBl.q_y(), currBl.q_z(), currBl.q_w());
            auto blCollider = getOrCreateCachedBulletCollider_NotThreadSafe(ud, bulletConfig->hitbox_half_size_x(), bulletConfig->hitbox_half_size_y(), bulletConfig->b_type(), newPos, newRot);
            transientUdToCollisionUdHolder[ud] = collisionUdHolderStockCache.Take_ThreadSafe();
            auto bodyID = blCollider->GetID();
            if (!blCollider->IsInBroadPhase()) {
                bodyIDsToAdd.push_back(bodyID);
            }
            bodyIDsToActivate.push_back(bodyID);
        }
    }

    for (int i = 0; i < currRdf->dynamic_traps_size(); i++) {
        const Trap& currTp = currRdf->dynamic_traps(i);
        if (globalPrimitiveConsts->terminating_trap_id() == currTp.id()) break;
        Trap* nextTp = nextRdf->mutable_dynamic_traps(i); // [WARNING] By reaching here, we haven't executed "leftShiftDeadDynamicTraps", hence the indices of "currRdf->bullets" and "nextRdf->bullets" are FULLY ALIGNED.
        auto ud = calcUserData(currTp);
        const TrapConfig* tpConfig = nullptr;
        const TrapConfigFromTiled* tpConfigFromTile = nullptr;
        const uint32_t tpt = currTp.tpt();
        FindTrapConfig(tpt, currTp.id(), trapConfigFromTileDict, tpConfig, tpConfigFromTile);
        JPH_ASSERT(nullptr != tpConfig);
        transientUdToCurrTrap[ud] = &currTp;
        transientUdToNextTrap[ud] = nextTp;

        const float immediateBoxHalfSizeX = (nullptr == tpConfigFromTile ? tpConfig->default_box_half_size_x() : tpConfigFromTile->box_half_size_x());
        const float immediateBoxHalfSizeY = (nullptr == tpConfigFromTile ? tpConfig->default_box_half_size_y() : tpConfigFromTile->box_half_size_y()); 

        Vec3 newTrapPos(currTp.x(), currTp.y(), currTp.z());
        Quat newTrapRot(currTp.q_x(), currTp.q_y(), currTp.q_z(), currTp.q_w());

        TP_COLLIDER_T* tpCollider = getOrCreateCachedTrapCollider_NotThreadSafe(ud, immediateBoxHalfSizeX, immediateBoxHalfSizeY, tpConfig, tpConfigFromTile, false, false, newTrapPos, newTrapRot);
        auto trapBodyID = tpCollider->GetID();
        if (!tpCollider->IsInBroadPhase()) {
            bodyIDsToAdd.push_back(trapBodyID);
        }
        bodyIDsToActivate.push_back(trapBodyID);

        if (globalPrimitiveConsts->tpt_sliding_platform() == currTp.tpt()) {
            JPH_ASSERT(nullptr != tpConfigFromTile);
            
            // [WARNING] The "constraintHelperBody" is added into "activeTpColliders", hence it will be deactivated by "BaseBattle::batchRemoveFromPhySysAndCache" and deallocated by "BaseBattle::Clear" too.
            Vec3Arg newHelperPos(tpConfigFromTile->init_x(), tpConfigFromTile->init_y(), tpConfigFromTile->init_z());
            QuatArg newHelperRot(tpConfigFromTile->init_q_x(), tpConfigFromTile->init_q_y(), tpConfigFromTile->init_q_z(), tpConfigFromTile->init_q_w());
            TP_COLLIDER_T* constraintHelperBody = getOrCreateCachedTrapCollider_NotThreadSafe(ud, immediateBoxHalfSizeX, immediateBoxHalfSizeY, tpConfig, tpConfigFromTile, true, false, newHelperPos, newHelperRot);
            auto constraintHelperBodyID = constraintHelperBody->GetID();
            if (!constraintHelperBody->IsInBroadPhase()) {
                bodyIDsToAdd.push_back(constraintHelperBodyID);
            }
            bodyIDsToActivate.push_back(constraintHelperBodyID);   

            TP_COLLIDER_T* obsIfaceBody = getOrCreateCachedTrapCollider_NotThreadSafe(ud, immediateBoxHalfSizeX, immediateBoxHalfSizeY, tpConfig, tpConfigFromTile, false, true, newTrapPos, newTrapRot);
            auto obsIfaceBodyID = obsIfaceBody->GetID();
            if (!obsIfaceBody->IsInBroadPhase()) {
                bodyIDsToAdd.push_back(obsIfaceBodyID);
            }
            bodyIDsToActivate.push_back(obsIfaceBodyID);
        }
    }

    for (int i = 0; i < currRdf->triggers_size(); i++) {
        const Trigger& currTr = currRdf->triggers(i);
        if (globalPrimitiveConsts->terminating_trigger_id() == currTr.id()) break;
        Trigger* nextTr = nextRdf->mutable_triggers(i); // [WARNING] By reaching here, we haven't executed "leftShiftDeadTriggers", hence the indices of "currRdf->triggers" and "nextRdf->triggers" are FULLY ALIGNED.
        JPH_ASSERT(nullptr != nextTr);

        auto ud = calcUserData(currTr);
        transientUdToCurrTrigger[ud] = &currTr;
        transientUdToNextTrigger[ud] = nextTr;

        if (!triggerConfigFromTileDict.count(currTr.id())) continue;
        if (TriggerState::TrReady != currTr.state()) continue; 
        if (!collidableTrtSet.count(currTr.trt())) continue;
        
        auto* triggerConfigFromTile = triggerConfigFromTileDict.at(currTr.id()); 

        Vec3 newPos(currTr.x(), currTr.y(), currTr.z());
        Quat newRot(triggerConfigFromTile->init_q_x(), triggerConfigFromTile->init_q_y(), triggerConfigFromTile->init_q_z(), triggerConfigFromTile->init_q_w());
        auto trCollider = getOrCreateCachedTriggerCollider_NotThreadSafe(ud, triggerConfigFromTile->box_half_size_x(), triggerConfigFromTile->box_half_size_y(), newPos, newRot);
        transientUdToCollisionUdHolder[ud] = collisionUdHolderStockCache.Take_ThreadSafe();
        auto bodyID = trCollider->GetID();
        if (!trCollider->IsInBroadPhase()) {
            bodyIDsToAdd.push_back(bodyID);
        }
/*
#ifndef NDEBUG
        if (0 == (currRdf->id() & ((1 << 4) - 1))) {
            std::ostringstream oss;
            oss << "Collidable trigger with id=" << currTr.id() << ", trt=" << currTr.trt() << " added bodyId=" << bodyID.GetIndexAndSequenceNumber() << ", pos=(" << newPos.GetX() << ", " << newPos.GetY() << ")"; 
            Debug::Log(oss.str(), DColor::Orange);
        }
#endif
*/
        bodyIDsToActivate.push_back(bodyID);
    }

    if (!bodyIDsToAdd.empty()) {
        auto layerState = biNoLock->AddBodiesPrepare(bodyIDsToAdd.data(), bodyIDsToAdd.size());
        biNoLock->AddBodiesFinalize(bodyIDsToAdd.data(), bodyIDsToAdd.size(), layerState, EActivation::DontActivate);
    }

    if (!bodyIDsToActivate.empty()) {
        biNoLock->ActivateBodies(bodyIDsToActivate.data(), bodyIDsToActivate.size());
    }
}

void BaseBattle::batchNonContactConstraintsSetupFromCache(const int currRdfId, const RenderFrame* currRdf, RenderFrame* nextRdf) {
    JobSystem::Barrier* nonContactConstraintSetupMTBarrier = jobSys->CreateBarrier();
    for (int i = 0; i < currRdf->dynamic_traps_size(); i++) {
        const Trap& currTp = currRdf->dynamic_traps(i);
        if (globalPrimitiveConsts->terminating_trap_id() == currTp.id()) break;
        Trap* nextTp = nextRdf->mutable_dynamic_traps(i); // [WARNING] By reaching here, we haven't executed "leftShiftDeadDynamicTraps", hence the indices of "currRdf->bullets" and "nextRdf->bullets" are FULLY ALIGNED.
        auto ud = calcUserData(currTp);
        const TrapConfig* tpConfig = nullptr;
        const TrapConfigFromTiled* tpConfigFromTile = nullptr;
        const uint32_t tpt = currTp.tpt();
        FindTrapConfig(tpt, currTp.id(), trapConfigFromTileDict, tpConfig, tpConfigFromTile);
        JPH_ASSERT(nullptr != tpConfig);
        TP_COLLIDER_T* tpCollider = tpConfig->use_kinematic() ? transientUdToConstraintObsIfaceBody.at(ud) : transientUdToTpCollider.at(ud); // [WARNING] To suffice "TwoBodyConstraint.IsActive()", one of the bodies MUST BE DYNAMIC!

        if (globalPrimitiveConsts->tpt_sliding_platform() == currTp.tpt()) {
            JPH_ASSERT(nullptr != tpConfigFromTile);
            Body* constraintHelperBody = transientUdToConstraintHelperBody.at(ud);

            const Vec3 worldSpaceSliderAxis(tpConfigFromTile->slider_axis_x(), tpConfigFromTile->slider_axis_y(), tpConfigFromTile->slider_axis_z());
            SliderConstraintSettings sliderSettings;
            sliderSettings.mAutoDetectPoint = false;
            sliderSettings.mPoint1 = tpCollider->GetCenterOfMassPosition();
            sliderSettings.mPoint2 = constraintHelperBody->GetCenterOfMassPosition();
            // The combination of "mAutoDetectPoint & mPoint1 & mPoint2" makes "SliderConstraint.mLocalSpacePosition1 == SliderConstraint.mLocalSpacePosition2 == Vec3::sZero()".
            sliderSettings.SetSliderAxis(worldSpaceSliderAxis);
            sliderSettings.mLimitsMin = tpConfigFromTile->limit_1();
            sliderSettings.mLimitsMax = tpConfigFromTile->limit_2();
            sliderSettings.mLimitsSpringSettings.mFrequency = tpConfigFromTile->limit_3();
            sliderSettings.mLimitsSpringSettings.mDamping = tpConfigFromTile->limit_4();
            NON_CONTACT_CONSTRAINT_T* cachedConstraint = getOrCreateCachedNonContactConstraint_NotThreadSafe(EConstraintType::TwoBodyConstraint, EConstraintSubType::Slider, tpCollider, constraintHelperBody, &sliderSettings);
            JPH_ASSERT(nullptr != cachedConstraint); 
            JPH::Constraint* c = cachedConstraint->c;
            SliderConstraint* sc = static_cast<SliderConstraint*>(c);

            const Vec3 initVel(tpConfigFromTile->init_vel_x(), tpConfigFromTile->init_vel_y(), tpConfigFromTile->init_vel_z());
            const float mD = sc->GetCurrentPosition();
            /*
            [WARNING] The following logic has an assumption that "min < max" and initially the trap goes from "min" to "max", i.e. use "tpConfigFromTile->init_q" to choose init facing direction.

            Moreover, it's weird that without "SliderConstraint.mMotorConstraintPart" the whole "SliderConstraint" DOESN'T enforce "SliderAxis movement" when "mBody1" is far from "mBody2", i.e. "SliderConstraint.mPositionConstraintPart.mEffectiveMass" will decrease along "abs(mD)" making the enforcement weak on the sides (see https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Physics/Constraints/SliderConstraint.cpp#L208 and https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Physics/Constraints/SliderConstraint.cpp#L310C13-L310C36), so I cut the velocity components perpendicular to "SliderAxis" manually.
            */
            int effCooldownRdfCount = 0 >= tpConfigFromTile->cooldown_rdf_count() ? tpConfig->default_cooldown_rdf_count() : tpConfigFromTile->cooldown_rdf_count();
            if (mD <= tpConfigFromTile->limit_1()) {
                if (TrapState::TpWalking == currTp.trap_state() && effCooldownRdfCount <= currTp.frames_in_trap_state()) {
/*
#ifndef NDEBUG
                    std::ostringstream oss;
                    oss << "@currRdfId=" << currRdfId << " currTp ud=" << ud << " at currPos=(" << currTp.x() <<  ", " << currTp.y() << "), currQ=(" << currTp.q_x() << ", " << currTp.q_y() << ", " << currTp.q_z() << ", " << currTp.q_w() << "), mD=" << mD << ", vel=(" << currTp.vel_x() << ", " << currTp.vel_y() << ")" << "; about to transit from walking to idle per limit1=" << tpConfigFromTile->limit_1() << ", initVel=(" << initVel.GetX() << ", " << initVel.GetY() << "), worldSpaceSliderAxis=(" << worldSpaceSliderAxis.GetX() << ", " << worldSpaceSliderAxis.GetY() << ")";
                    Debug::Log(oss.str(), DColor::Orange);
#endif
*/
                    nextTp->set_trap_state(TrapState::TpIdle);
                    nextTp->set_frames_in_trap_state(0);
                } else if (TrapState::TpIdle == currTp.trap_state() && effCooldownRdfCount <= currTp.frames_in_trap_state()) {
/*
#ifndef NDEBUG
                    std::ostringstream oss;
                    oss << "@currRdfId=" << currRdfId << " currTp ud=" << ud << " at currPos=(" << currTp.x() <<  ", " << currTp.y() << "), currQ=(" << currTp.q_x() << ", " << currTp.q_y() << ", " << currTp.q_z() << ", " << currTp.q_w() << "), mD=" << mD << ", vel=(" << currTp.vel_x() << ", " << currTp.vel_y() << ")" << "; about to transit from idle to walking per limit1=" << tpConfigFromTile->limit_1() << ", initVel=(" << initVel.GetX() << ", " << initVel.GetY() << "), worldSpaceSliderAxis=(" << worldSpaceSliderAxis.GetX() << ", " << worldSpaceSliderAxis.GetY() << ")";
                    Debug::Log(oss.str(), DColor::Orange);
#endif
*/
                    const Vec3 projectedVel = -std::abs(initVel.Dot(worldSpaceSliderAxis))*worldSpaceSliderAxis;
                    nextTp->set_trap_state(TrapState::TpWalking);
                    nextTp->set_frames_in_trap_state(0);
                    biNoLock->SetLinearVelocity(tpCollider->GetID(), projectedVel);
                }
            } else if (mD >= tpConfigFromTile->limit_2()) {
                if (TrapState::TpWalking == currTp.trap_state() && effCooldownRdfCount <= currTp.frames_in_trap_state()) {
/*
#ifndef NDEBUG
                    std::ostringstream oss;
                    oss << "@currRdfId=" << currRdfId << " currTp ud=" << ud << " at currPos=(" << currTp.x() <<  ", " << currTp.y() << "), currQ=(" << currTp.q_x() << ", " << currTp.q_y() << ", " << currTp.q_z() << ", " << currTp.q_w() << "), mD=" << mD << ", vel=(" << currTp.vel_x() << ", " << currTp.vel_y() << ")" << "; about to transit from walking to idle per limit2=" << tpConfigFromTile->limit_2() << ", initVel=(" << initVel.GetX() << ", " << initVel.GetY() << "), worldSpaceSliderAxis=(" << worldSpaceSliderAxis.GetX() << ", " << worldSpaceSliderAxis.GetY() << ")";
                    Debug::Log(oss.str(), DColor::Orange);
#endif
*/
                    nextTp->set_trap_state(TrapState::TpIdle);
                    nextTp->set_frames_in_trap_state(0);
                } else if (TrapState::TpIdle == currTp.trap_state() && effCooldownRdfCount <= currTp.frames_in_trap_state()) {
/*
#ifndef NDEBUG
                    std::ostringstream oss;
                    oss << "@currRdfId=" << currRdfId << " currTp ud=" << ud << " at currPos=(" << currTp.x() <<  ", " << currTp.y() << "), currQ=(" << currTp.q_x() << ", " << currTp.q_y() << ", " << currTp.q_z() << ", " << currTp.q_w() << "), mD=" << mD << ", vel=(" << currTp.vel_x() << ", " << currTp.vel_y() << ")" << "; about to transit from idle to walking per limit2=" << tpConfigFromTile->limit_2() << ", initVel=(" << initVel.GetX() << ", " << initVel.GetY() << "), worldSpaceSliderAxis=(" << worldSpaceSliderAxis.GetX() << ", " << worldSpaceSliderAxis.GetY() << ")";
                    Debug::Log(oss.str(), DColor::Orange);
#endif
*/
                    const Vec3 projectedVel = +std::abs(initVel.Dot(worldSpaceSliderAxis))*worldSpaceSliderAxis;
                    nextTp->set_trap_state(TrapState::TpWalking);
                    nextTp->set_frames_in_trap_state(0);
                    biNoLock->SetLinearVelocity(tpCollider->GetID(), projectedVel);
                }
            }
        }
    }

    jobSys->WaitForJobs(nonContactConstraintSetupMTBarrier);
    jobSys->DestroyBarrier(nonContactConstraintSetupMTBarrier);

    if (!activeNonContactConstraints.empty()) {
        for (int i = 0; i < activeNonContactConstraints.size(); i++) {
            NON_CONTACT_CONSTRAINT_T* single = activeNonContactConstraints[i]; 
            JPH::Constraint* singleC = single->c;
            // It's O(1) time, see https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Physics/Constraints/ConstraintManager.cpp#L17
            phySys->AddConstraint(singleC);
        } 
    }
}

static JPH::BodyID invalidBodyID;
static JPH::SubShapeID emptySubShapeID;
void BaseBattle::batchRemoveFromPhySysAndCache(const int currRdfId, const RenderFrame* currRdf) {
    // Remove "NonContactConstraint"s first, just like what we do in databases :)
    while (!activeNonContactConstraints.empty()) {
        NON_CONTACT_CONSTRAINT_T* single = activeNonContactConstraints.back();
        activeNonContactConstraints.pop_back();
        JPH::Constraint* singleC = single->c;
        auto nonContactConstraintType = single->theType; 
        auto nonContactConstraintSubType = single->theSubType;
        auto ud1 = single->ud1;
        auto ud2 = single->ud2;
        NON_CONTACT_CONSTRAINT_CACHE_KEY_T k(nonContactConstraintType, nonContactConstraintSubType, ud1, ud2);

        auto it = cachedNonContactConstraints.find(k);
        if (it == cachedNonContactConstraints.end()) {
            NON_CONTACT_CONSTRAINT_Q q = { single };
            cachedNonContactConstraints.emplace(k, q);
        } else {
            auto& cacheQue = it->second;
            cacheQue.push_back(single);
        }
        /* [REMINDER]

        ["ConstraintManager.Remove(...)"](https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Physics/Constraints/ConstraintManager.cpp#L51) is very efficient due to the use of "Constraint.mConstraintIndex & swapping to tail & pop_back" trick, therefore unlike "Body"s, it's totally OK if we choose to call "phySys->RemoveConstraint(...)" on each single "activeNonContactConstraints.back()".
        */
        phySys->RemoveConstraint(singleC);
    }

    bodyIDsToClear.clear();
    while (!activeChColliders.empty()) {
        CH_COLLIDER_T* single = activeChColliders.back();
        activeChColliders.pop_back();
        auto bodyID = single->GetBodyID();
        uint64_t ud = biNoLock->GetUserData(bodyID);
        uint64_t udt = getUDT(ud);
        const CharacterDownsync& currChd = immutableCurrChdFromUd(udt, ud);
        const CharacterConfig* cc = getCc(currChd.species_id());

        calcChCacheKey(cc, chCacheKeyHolder); // [WARNING] Don't use the "immediate shape" attached to "single" for capsuleKey forming, it's different from the values of corresponding "CharacterConfig*".
        auto it = cachedChColliders.find(chCacheKeyHolder);
        if (it == cachedChColliders.end()) {
            // [REMINDER] Lifecycle of this stack-variable "q" will end after exiting the current closure, thus if "cachedChColliders" is to retain it out of the current closure, some extra space is to be used.
            CH_COLLIDER_Q q = { single };
            cachedChColliders.emplace(chCacheKeyHolder, q);
        } else {
            auto& cacheQue = it->second;
            cacheQue.push_back(single);
        }
        biNoLock->SetFriction(bodyID, cDefaultChFriction); // ALWAYS resume friction here, single-threaded
        single->SetGroundBodyID(invalidBodyID, emptySubShapeID);
        single->SetGroundBodyPosition(Vec3::sZero(), Vec3::sZero());
        single->SetGroundState(JPH::CharacterBase::EGroundState::InAir);
        single->SetGroundBodyUd(JPH::PhysicsMaterial::sDefault, Vec3::sZero(), 0);
        single->SetPositionAndRotation(safeDeactiviatedPosition, Quat::sIdentity(), EActivation::DontActivate, true); // [WARNING] To avoid spurious awakening. See "RuleOfThumb.md" for details.
        single->SetLinearAndAngularVelocity(Vec3::sZero(), Vec3::sZero(), true);
        bodyIDsToClear.push_back(bodyID);
    }

    while (!activeBlColliders.empty()) { 
        BL_COLLIDER_T* single = activeBlColliders.back();
        activeBlColliders.pop_back();
        auto ud = single->GetUserData();
        JPH_ASSERT(0 < transientUdToCurrBl.count(ud));
        const Bullet& bl = *(transientUdToCurrBl.at(ud));
        JPH_ASSERT(globalPrimitiveConsts->terminating_bullet_id() != bl.id());
        const Skill* skill = nullptr;
        const BulletConfig* bc = nullptr;
        FindBulletConfig(bl.skill_id(), bl.active_skill_hit(), skill, bc);

        calcBlCacheKey(bc->hitbox_half_size_x(), bc->hitbox_half_size_y(), blCacheKeyHolder);
        auto it = cachedBlColliders.find(blCacheKeyHolder);

        if (it == cachedBlColliders.end()) {
            BL_COLLIDER_Q q = { single };
            cachedBlColliders.emplace(blCacheKeyHolder, q);
        } else {
            auto& cacheQue = it->second;
            cacheQue.push_back(single);
        }

        biNoLock->SetPositionAndRotation(single->GetID()
            , safeDeactiviatedPosition // [WARNING] To avoid spurious awakening. See "RuleOfThumb.md" for details.
            , Quat::sIdentity()
            , EActivation::DontActivate);
        biNoLock->SetLinearAndAngularVelocity(single->GetID(), Vec3::sZero(), Vec3::sZero());
        bodyIDsToClear.push_back(single->GetID());
    }

    while (!activeTpColliders.empty()) { 
        TP_COLLIDER_T* single = activeTpColliders.back();
        activeTpColliders.pop_back();
        auto ud = single->GetUserData();
        JPH_ASSERT(0 < transientUdToCurrTrap.count(ud));
        if (transientSlipJumpableUds.count(ud)) {
            transientSlipJumpableUds.erase(ud);
        }
        if (transientCrouchForcingUds.count(ud)) {
            transientCrouchForcingUds.erase(ud);
        } 
        const Trap& tp = *(transientUdToCurrTrap.at(ud));
        JPH_ASSERT(globalPrimitiveConsts->terminating_trap_id() != tp.id());
        const TrapConfig* tpConfig = nullptr;
        const TrapConfigFromTiled* tpConfigFromTile = nullptr;
        FindTrapConfig(tp.tpt(), tp.id(), trapConfigFromTileDict, tpConfig, tpConfigFromTile);
        JPH_ASSERT(nullptr != tpConfig);
        if (nullptr == tpConfigFromTile) {
            calcTpCacheKey(tpConfig->default_box_half_size_x(), tpConfig->default_box_half_size_y(), single->GetMotionType(), single->IsSensor(), single->GetObjectLayer(), tpCacheKeyHolder);
        } else {
            calcTpCacheKey(tpConfigFromTile->box_half_size_x(), tpConfigFromTile->box_half_size_y(), single->GetMotionType(), single->IsSensor(), single->GetObjectLayer(), tpCacheKeyHolder);
        }
        auto it = cachedTpColliders.find(tpCacheKeyHolder);

        if (it == cachedTpColliders.end()) {
            TP_COLLIDER_Q q = { single };
            cachedTpColliders.emplace(tpCacheKeyHolder, q);
        } else {
            auto& cacheQue = it->second;
            cacheQue.push_back(single);
        }

        biNoLock->SetPositionAndRotation(single->GetID()
            , safeDeactiviatedPosition // [WARNING] To avoid spurious awakening. See "RuleOfThumb.md" for details.
            , Quat::sIdentity()
            , EActivation::DontActivate);
        biNoLock->SetLinearAndAngularVelocity(single->GetID(), Vec3::sZero(), Vec3::sZero());
        bodyIDsToClear.push_back(single->GetID());
    }

    while (!activeTrColliders.empty()) { 
        TR_COLLIDER_T* single = activeTrColliders.back();
        activeTrColliders.pop_back();
        auto ud = single->GetUserData();
        JPH_ASSERT(0 < transientUdToCurrTrigger.count(ud));
        const Trigger& tr = *(transientUdToCurrTrigger.at(ud));
        JPH_ASSERT(globalPrimitiveConsts->terminating_trigger_id() != tr.id());
        JPH_ASSERT(0 < triggerConfigFromTileDict.count(tr.id()));
        auto* triggerConfigFromTile = triggerConfigFromTileDict.at(tr.id());

        calcTrCacheKey(triggerConfigFromTile->box_half_size_x(), triggerConfigFromTile->box_half_size_y(), trCacheKeyHolder);
        auto it = cachedTrColliders.find(trCacheKeyHolder);

        if (it == cachedTrColliders.end()) {
            TR_COLLIDER_Q q = { single };
            cachedTrColliders.emplace(trCacheKeyHolder, q);
        } else {
            auto& cacheQue = it->second;
            cacheQue.push_back(single);
        }

        biNoLock->SetPositionAndRotation(single->GetID()
            , safeDeactiviatedPosition // [WARNING] To avoid spurious awakening. See "RuleOfThumb.md" for details.
            , Quat::sIdentity()
            , EActivation::DontActivate);
        biNoLock->SetLinearAndAngularVelocity(single->GetID(), Vec3::sZero(), Vec3::sZero());
        bodyIDsToClear.push_back(single->GetID());
    }

    /*
    [WARNING]

    Unlike "BodyInterface::RemoveBodies", the cheaper "BodyInterface::DeactivateBodies" saves (potential) operations of "QuadTree nodes", e.g.
    - merging "QuadTree nodes", or
    - splitting "QuadTree nodes", or
    - moving bodies across "QuadTree nodes".

    In terms of Jolt implementation,
    - [BodyInterface::DeactivateBodies](https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Physics/Body/BodyInterface.cpp#L249) doesn't even touch "BodyInterface.mBroadPhase" (thus no operation on the QuadTree), and
    - [BodyInterface::RemoveBodies](https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Physics/Body/BodyInterface.cpp#L202) contains "BodyInterface::DeactivateBodies".
    */
    if (!bodyIDsToClear.empty()) {
        biNoLock->DeactivateBodies(bodyIDsToClear.data(), bodyIDsToClear.size());
    }

    // This function will remove or deactivate all bodies attached to "phySys", so this mapping cache will certainly become invalid.
    transientUdToChCollider.clear();
    transientUdToBodyID.clear();
    transientUdToTpCollider.clear();
    transientUdToConstraintHelperBodyID.clear();
    transientUdToConstraintHelperBody.clear();
    transientUdToConstraintObsIfaceBodyID.clear();
    transientUdToConstraintObsIfaceBody.clear();
    transientUdToCollisionUdHolder.clear();
    collisionUdHolderStockCache.Clear_ThreadSafe();
    transientUdToInputInducedMotion.clear();
    inputInducedMotionStockCache.Clear_ThreadSafe();

    transientUdToCurrPlayer.clear();
    transientUdToNextPlayer.clear();

    transientUdToCurrNpc.clear();
    transientUdToNextNpc.clear();

    transientUdToCurrBl.clear();
    transientUdToNextBl.clear();

    transientUdToCurrTrap.clear();
    transientUdToNextTrap.clear();

    transientUdToCurrTrigger.clear();
    transientUdToNextTrigger.clear();

    transientUdToCurrPickable.clear();
    transientUdToNextPickable.clear();
    
    mNextRdfBulletIdCounter = 1;
    mNextRdfBulletCount = 0;

    mNextRdfNpcIdCounter = 1;
    mNextRdfNpcCount = 0;
}

void BaseBattle::deriveCharacterOpPattern(const int currRdfId, const CharacterDownsync& currChd, const Vec3& currChdFacing, const CharacterConfig* cc, CharacterDownsync* nextChd, bool currEffInAir, bool notDashing, const InputFrameDecoded& ifDecoded, int& outPatternId, bool& outJumpedOrNot, bool& outSlipJumpedOrNot, int& outEffDx, int& outEffDy) {
    outJumpedOrNot = false;
    outSlipJumpedOrNot = false;
    outEffDx = 0;
    outEffDy = 0;

    updateBtnHoldingByInput(currChd, ifDecoded, nextChd);

    // Jumping is partially allowed within "CapturedByInertia", but moving is only allowed when "0 == frames_to_recover()" (constrained later in "Step")
    if (0 >= currChd.frames_to_recover()) {
        outEffDx = ifDecoded.dx();
        outEffDy = ifDecoded.dy();
    } else if (!currEffInAir && 1 >= currChd.frames_to_recover() && 0 > ifDecoded.dy() && cc->crouching_enabled()) {
        // to favor smooth crouching transition
        outEffDx = ifDecoded.dx();
        outEffDy = ifDecoded.dy();
    } else if (WalkingAtk1 == currChd.ch_state() || WalkingAtk4 == currChd.ch_state()) {
        outEffDx = ifDecoded.dx();
    } else if (isInBlockStun(currChd)) {
        // Reserve only "effDy" for later use by "useSkill", e.g. to break free from block-stun by certain skills.
        outEffDy = ifDecoded.dy();
    }

    outPatternId = globalPrimitiveConsts->pattern_id_no_op();
    int effFrontOrBack = (ifDecoded.dx() * currChdFacing.GetX()); // [WARNING] Deliberately using "ifDecoded.dx()" instead of "effDx (which could be 0 in block stun)" here!
    bool currInJumpStartup = isInJumpStartup(currChd, cc);
    bool canJumpWithinInertia = BaseBattleCollisionFilter::chCanJumpWithInertia(currChd, cc, notDashing, currInJumpStartup);
    if (0 < ifDecoded.btn_a_level()) {
        if (0 == currChd.btn_a_holding_rdf_cnt() && canJumpWithinInertia) {
            if ((!currEffInAir && transientSlipJumpableUds.count(currChd.ground_ud())) && (0 > ifDecoded.dy() && 0 == ifDecoded.dx())) {
                outSlipJumpedOrNot = true;
            } else if (((currEffInAir && currChd.omit_gravity() && !cc->omit_gravity())) && (0 > ifDecoded.dy() && 0 == ifDecoded.dx())) {
                outSlipJumpedOrNot = true;
            } else if ((!currEffInAir || 0 < currChd.remaining_air_jump_quota()) && (!isCrouching(currChd.ch_state(), cc) || !notDashing)) {
                outJumpedOrNot = true;
            } else if (OnWallIdle1 == currChd.ch_state()) {
                outJumpedOrNot = true;
            }
        }
    }

    if (globalPrimitiveConsts->pattern_id_no_op() == outPatternId) {
        if (0 < ifDecoded.btn_b_level()) {
            if (0 == currChd.btn_b_holding_rdf_cnt()) {
                if (0 < ifDecoded.btn_c_level()) {
                    outPatternId = globalPrimitiveConsts->pattern_inventory_slot_bc();
                } else if (0 > ifDecoded.dy()) {
                    outPatternId = globalPrimitiveConsts->pattern_down_b();
                } else if (0 < ifDecoded.dy()) {
                    outPatternId = globalPrimitiveConsts->pattern_up_b();
                } else {
                    outPatternId = globalPrimitiveConsts->pattern_b();
                }
            } else {
                outPatternId = globalPrimitiveConsts->pattern_hold_b();
            }
        } else {
            // 0 >= ifDecoded.btn_b_level()
            if (globalPrimitiveConsts->btn_b_holding_rdf_cnt_threshold_2() <= currChd.btn_b_holding_rdf_cnt()) {
                outPatternId = globalPrimitiveConsts->pattern_released_b();
            }
        }
    }

    if (globalPrimitiveConsts->pattern_hold_b() == outPatternId || globalPrimitiveConsts->pattern_id_no_op() == outPatternId) {
        if (0 < ifDecoded.btn_e_level() && (cc->dashing_enabled() || cc->sliding_enabled())) {
            if (0 == currChd.btn_e_holding_rdf_cnt()) {
                if (notDashing) {
                    if (0 < effFrontOrBack) {
                        outPatternId = (globalPrimitiveConsts->pattern_hold_b() == outPatternId ? globalPrimitiveConsts->pattern_front_e_hold_b() : globalPrimitiveConsts->pattern_front_e());
                    } else if (0 > effFrontOrBack) {
                        outPatternId = (globalPrimitiveConsts->pattern_hold_b() == outPatternId ? globalPrimitiveConsts->pattern_back_e_hold_b() : globalPrimitiveConsts->pattern_back_e());
                        outEffDx = 0; // [WARNING] Otherwise the character will turn around
                    } else if (0 > ifDecoded.dy()) {
                        outPatternId = (globalPrimitiveConsts->pattern_hold_b() == outPatternId ? globalPrimitiveConsts->pattern_down_e_hold_b() : globalPrimitiveConsts->pattern_down_e());
                    } else if (0 < ifDecoded.dy()) {
                        outPatternId = (globalPrimitiveConsts->pattern_hold_b() == outPatternId ? globalPrimitiveConsts->pattern_up_e_hold_b() : globalPrimitiveConsts->pattern_up_e());
                    } else {
                        outPatternId = (globalPrimitiveConsts->pattern_hold_b() == outPatternId ? globalPrimitiveConsts->pattern_e_hold_b() : globalPrimitiveConsts->pattern_e());
                    }
                }
            } else {
                outPatternId = (globalPrimitiveConsts->pattern_hold_b() == outPatternId ? globalPrimitiveConsts->pattern_hold_e_hold_b() : globalPrimitiveConsts->pattern_hold_e());
            }
        }
    }

    if (globalPrimitiveConsts->pattern_id_no_op() == outPatternId) {
        if (0 < ifDecoded.btn_c_level()) {
            if (0 == currChd.btn_c_holding_rdf_cnt()) {
                outPatternId = globalPrimitiveConsts->pattern_inventory_slot_c();
                if (0 < ifDecoded.btn_b_level()) {
                    outPatternId = globalPrimitiveConsts->pattern_inventory_slot_bc();
                } else {
                    outPatternId = globalPrimitiveConsts->pattern_hold_inventory_slot_c();
                    if (0 < ifDecoded.btn_b_level() && 0 == currChd.btn_b_holding_rdf_cnt()) {
                        outPatternId = globalPrimitiveConsts->pattern_inventory_slot_bc();
                    }
                }
            } else if (0 < ifDecoded.btn_d_level()) {
                if (0 == currChd.btn_d_holding_rdf_cnt()) {
                    outPatternId = globalPrimitiveConsts->pattern_inventory_slot_d();
                } else {
                    outPatternId = globalPrimitiveConsts->pattern_hold_inventory_slot_d();
                }
            }
        }
    }
}

void BaseBattle::processSingleCharacterInput(const int currRdfId, float dt, int patternId, bool jumpedOrNot, bool slipJumpedOrNot, int effDx, int effDy, bool slowDownToAvoidOverlap, const CharacterDownsync& currChd, const MassProperties& massProps, const Vec3& currChdFacing, uint64_t ud, bool currEffInAir, bool currCrouching, bool currOnWall, bool currDashing, bool currWalking, bool currInBlockStun, bool currAtked, bool currParalyzed, const CharacterConfig* cc, CharacterDownsync* nextChd, RenderFrame* nextRdf, bool& usedSkill, const CH_COLLIDER_T* chCollider, InputInducedMotion* ioInputInducedMotion, bool& ioGravityDirty, bool& ioFrictionDirty) {
    bool slotUsed = false;
    uint32_t slotLockedSkillId = globalPrimitiveConsts->no_skill();
    bool dodgedInBlockStun = false;
    // TODO: Call "useInventorySlot" appropriately 

    if (globalPrimitiveConsts->jump_holding_rdf_cnt_threshold_2() > currChd.btn_a_holding_rdf_cnt() && globalPrimitiveConsts->jump_holding_rdf_cnt_threshold_2() <= nextChd->btn_a_holding_rdf_cnt() && !currChd.omit_gravity() && cc->jump_holding_to_fly() && proactiveJumpingSet.count(currChd.ch_state())) {
        /*
           (a.) The original "hold-only-to-fly" is prone to "falsely predicted flying" due to not being edge-triggered;
           (b.) However, "releasing BtnA at globalPrimitiveConsts->jump_holding_rdf_cnt_threshold_2() <= currChd.btn_a_holding_rdf_cnt()" makes it counter-intuitive to use when playing, the trade-off is not easy for me...
         */
        nextChd->set_omit_gravity(true);
        nextChd->set_flying_rdf_countdown(cc->flying_quota_rdf_cnt());
    }

    if (0 < currChd.debuff_list_size()) {
        auto existingDebuff = currChd.debuff_list(globalPrimitiveConsts->debuff_array_idx_elemental());
        if (globalPrimitiveConsts->terminating_debuff_species_id() != existingDebuff.species_id()) {
            auto existingDebufConfig = globalConfigConsts->debuff_configs().at(existingDebuff.species_id());
            currParalyzed = (globalPrimitiveConsts->terminating_debuff_species_id() != existingDebuff.species_id() && 0 < existingDebuff.stock() && DebuffType::PositionLockedOnly == existingDebufConfig.type());
        }
    }
    if (dodgedInBlockStun) {
        transitToGroundDodgedChState(currChd, currChdFacing, nextChd, cc, currParalyzed, ioInputInducedMotion);
    }

    int outSkillId = globalPrimitiveConsts->no_skill();
    const Skill* outSkillConfig = nullptr;
    const BulletConfig* outPivotBc = nullptr;

    usedSkill = dodgedInBlockStun ? false : useSkill(currRdfId, nextRdf, currChd, currChdFacing, ud, cc, nextChd, effDx, effDy, patternId, currEffInAir, currCrouching, currOnWall, currDashing, currWalking, currInBlockStun, currAtked, currParalyzed, outSkillId, outSkillConfig, outPivotBc);

    const BodyID chBodyID = chCollider->GetBodyID(); 
    /*
    if (cc->btn_b_auto_unhold_ch_states().contains(nextChd->ch_state())) {
        // [WARNING] For "autofire" skills.
        nextChd->set_btn_b_holding_rdf_cnt(0);
    }
    */

    if (usedSkill) {
        bi->SetFriction(chBodyID, cDefaultChFriction);
        ioFrictionDirty = true;
        if (!outPivotBc->allows_walking()) {
            if (globalPrimitiveConsts->no_lock_vel() != outPivotBc->self_lock_vel_x()) {
                ioInputInducedMotion->velCOM.SetX(currChd.ground_vel_x());
            }
            if (globalPrimitiveConsts->no_lock_vel() != outPivotBc->self_lock_vel_y()) {
                ioInputInducedMotion->velCOM.SetY(currChd.ground_vel_y());
            }
        } else {
            if (0 != effDx) {
                int xfac = (0 < effDx ? 1 : -1);
                ioInputInducedMotion->forceCOM.SetX(xfac * (cc->acc_mag_x() * massProps.mMass));
            }
        }

        if (InAirDashing == outSkillConfig->bound_ch_state()) {
            if (!currChd.omit_gravity() && 0 < currChd.remaining_air_dash_quota()) {
                nextChd->set_remaining_air_dash_quota(currChd.remaining_air_dash_quota() - 1);
                if (!cc->isolated_air_jump_and_dash_quota() && 0 < currChd.remaining_air_jump_quota()) {
                    nextChd->set_remaining_air_jump_quota(currChd.remaining_air_jump_quota() - 1);
                }
            }
            bi->SetGravityFactor(chBodyID, 0);
            ioGravityDirty = true;
        } else {
            bool nextNotDashing = BaseBattleCollisionFilter::chIsNotDashing(*nextChd);
            if (!nextNotDashing) {
                bi->SetFriction(chBodyID, cGroundDashingChFriction);
            }
        }
/*
#ifndef NDEBUG
        std::ostringstream oss1;
        oss1 << "@currRdfId=" << currRdfId << ", set nextChd ch_state=" << nextChd->ch_state() << " and frames_in_ch_state=" << nextChd->frames_in_ch_state();
        Debug::Log(oss1.str(), DColor::Orange);
#endif // !NDEBUG
*/
    } else {
        // [WARNING] This is a necessary cleanup before "processInertiaWalking"!
        if (1 == currChd.frames_to_recover() && 0 == nextChd->frames_to_recover() && atkedSet.count(currChd.ch_state())) {
            ioInputInducedMotion->velCOM.Set(currChd.ground_vel_x(), currChd.ground_vel_y(), 0);
        }

        bool currInJumpStartup = isInJumpStartup(currChd, cc); 
        prepareJumpStartup(currRdfId, currChd, ud, massProps, currChdFacing, jumpedOrNot, slipJumpedOrNot, nextChd, currEffInAir, cc, currParalyzed, chCollider, currInJumpStartup, currDashing, ioInputInducedMotion);
        bool nextInJumpStartup = isInJumpStartup(*nextChd, cc); 

        if (!currChd.omit_gravity() && !cc->omit_gravity()) {
            processInertiaWalking(currRdfId, dt, currChd, massProps, currChdFacing, nextChd, currEffInAir, effDx, effDy, cc, currParalyzed, currInBlockStun, ud, chCollider, currInJumpStartup, nextInJumpStartup, currDashing, ioInputInducedMotion, ioGravityDirty, ioFrictionDirty);
        } else {
            processInertiaFlying(currRdfId, dt, currChd, massProps, currChdFacing, nextChd, effDx, effDy, cc, currParalyzed, currInBlockStun, ud, chCollider, currInJumpStartup, nextInJumpStartup, currDashing, ioInputInducedMotion, ioGravityDirty, ioFrictionDirty);
        }
            
        Quat currChdQRaw(currChd.q_x(), currChd.q_y(), currChd.q_z(), currChd.q_w());
        const JPH::Quat newYRot = JPH::Quat::sRotation(cYAxis, dt*ioInputInducedMotion->angVelCOM.GetY()); 
        JPH::Quat nextChdQ = newYRot*currChdQRaw; 
/*
#ifndef NDEBUG 
        if (onWallSet.count(currChd.ch_state()) && 0 != effDx) {
            std::ostringstream oss;
            oss << "@currRdfId=" << currRdfId << ", characterUd=" << ud << ": on wall nextQ=(" << nextChdQ.GetX() << ", " << nextChdQ.GetY() << ", " << nextChdQ.GetZ() << ", " << nextChdQ.GetW() << "), effDx=" << effDx << ", angVelCOM=(" << ioInputInducedMotion->angVelCOM.GetX() << ", " << ioInputInducedMotion->angVelCOM.GetY() << ", " << ioInputInducedMotion->angVelCOM.GetZ() << ") #2";
            Debug::Log(oss.str(), DColor::Orange);
        }
#endif
*/
        clampChdQ(nextChdQ, effDx);
        nextChd->set_q_x(nextChdQ.GetX());
        nextChd->set_q_y(nextChdQ.GetY());
        nextChd->set_q_z(nextChdQ.GetZ());
        nextChd->set_q_w(nextChdQ.GetW());
/*
#ifndef NDEBUG 
        if (onWallSet.count(currChd.ch_state()) && 0 != effDx) {
            std::ostringstream oss;
            oss << "@currRdfId=" << currRdfId << ", characterUd=" << ud << ": on wall nextQ=(" << nextChdQ.GetX() << ", " << nextChdQ.GetY() << ", " << nextChdQ.GetZ() << ", " << nextChdQ.GetW() << "), effDx=" << effDx << ", angVelCOM=(" << ioInputInducedMotion->angVelCOM.GetX() << ", " << ioInputInducedMotion->angVelCOM.GetY() << ", " << ioInputInducedMotion->angVelCOM.GetZ() << ") #3";
            Debug::Log(oss.str(), DColor::Orange);
        }
#endif
*/
        bool nextNotDashing = BaseBattleCollisionFilter::chIsNotDashing(*nextChd);
        bool nextEffInAir = isEffInAir(*nextChd, nextNotDashing);
        processDelayedBulletSelfVel(currRdfId, currChd, massProps, currChdFacing, nextChd, cc, currParalyzed, nextEffInAir, ioInputInducedMotion);

        if (0 >= currChd.frames_to_recover()) {
            if (globalPrimitiveConsts->pattern_id_unable_to_op() != patternId && cc->anti_gravity_when_idle() && (Walking == nextChd->ch_state() || InAirWalking == nextChd->ch_state()) && cc->anti_gravity_frames_lingering() < nextChd->frames_in_ch_state()) {
                nextChd->set_ch_state(InAirIdle1NoJump);
                nextChd->set_frames_in_ch_state(0);
            } else if (slowDownToAvoidOverlap) {
                ioInputInducedMotion->velCOM *= 0.25;
            }
        }

        if (currDashing) {
            if (InAirDashing == currChd.ch_state()) {
                bi->SetGravityFactor(chBodyID, 0);
                ioGravityDirty = true;
            } else {
                bi->SetFriction(chBodyID, cGroundDashingChFriction);
                ioFrictionDirty = true;
            }
        }
    }

    if (currParalyzed) {
        ioInputInducedMotion->velCOM.Set(currChd.ground_vel_x(), currChd.ground_vel_y(), currChd.ground_vel_z());
    }
}

void BaseBattle::FindBulletConfig(const uint32_t skillId, const uint32_t skillHit, const Skill*& outSkill, const BulletConfig*& outBulletConfig) {
    if (globalPrimitiveConsts->no_skill() == skillId) return;
    if (globalPrimitiveConsts->no_skill_hit() == skillHit) return;
    auto& skillConfigs = globalConfigConsts->skill_configs();
    if (!skillConfigs.count(skillId)) return;
    const Skill& outSkillVal = skillConfigs.at(skillId);
    outSkill = &(outSkillVal);
    if (skillHit > outSkill->hits_size()) {
        outSkill = nullptr;
        outBulletConfig = nullptr;
        return;
    }
    const BulletConfig& targetBlConfig = outSkill->hits(skillHit - 1);
    outBulletConfig = &targetBlConfig;
}

void BaseBattle::FindTrapConfig(const uint32_t trapSpeciesId, const uint32_t trapId, const std::unordered_map<uint32_t, const TrapConfigFromTiled*> inTrapConfigFromTileDict, const TrapConfig*& outTpConfig, const TrapConfigFromTiled*& outTpConfigFromTiled) {
    if (globalPrimitiveConsts->terminating_trap_id() == trapId) return;
    auto& trapConfigs = globalConfigConsts->trap_configs();
    if (!trapConfigs.count(trapSpeciesId)) return;
    const TrapConfig& outTpConfigVal = trapConfigs.at(trapSpeciesId);
    outTpConfig = &(outTpConfigVal);
    if (!inTrapConfigFromTileDict.count(trapId)) return;
    outTpConfigFromTiled = inTrapConfigFromTileDict.at(trapId);
}

void BaseBattle::processDelayedBulletSelfVel(const int currRdfId, const CharacterDownsync& currChd, const MassProperties& massProps, const Vec3& currChdFacing, CharacterDownsync* nextChd, const CharacterConfig* cc, const bool currParalyzed, const bool nextEffInAir, InputInducedMotion* ioInputInducedMotion) {
    const Skill* skill = nullptr;
    const BulletConfig* bulletConfig = nullptr;
    if (globalPrimitiveConsts->no_skill() == currChd.active_skill_id() || globalPrimitiveConsts->no_skill_hit() == currChd.active_skill_hit()) return;
    FindBulletConfig(currChd.active_skill_id(), currChd.active_skill_hit(), skill, bulletConfig);
    if (nullptr == skill || nullptr == bulletConfig) {
        return;
    }
    if (currChd.ch_state() != skill->bound_ch_state()) {
        // This shouldn't happen, but if it does, we don't proceed to set "selfLockVel"
        return;
    }
    if (currParalyzed) {
        return;
    }

    if (bulletConfig->delay_self_vel_to_active()) {
        if (bulletConfig->startup_frames() != currChd.frames_in_ch_state()) {
            return;
        } 
    } else {
        if (0 != currChd.frames_in_ch_state()) {
            return;
        } 
    }

    int xfac = (0 < currChdFacing.GetX() ? 1 : -1);
    if (globalPrimitiveConsts->no_lock_vel() != bulletConfig->self_lock_vel_x()) {
        ioInputInducedMotion->velCOM.SetX(xfac * bulletConfig->self_lock_vel_x() + currChd.ground_vel_x());
    }
    if (globalPrimitiveConsts->no_lock_vel() != bulletConfig->self_lock_vel_y()) {
        if (0 <= bulletConfig->self_lock_vel_y() || nextEffInAir) {
            ioInputInducedMotion->velCOM.SetY(bulletConfig->self_lock_vel_y() + currChd.ground_vel_y());
        }
    }
}

void BaseBattle::postStepSingleChdStateCorrection(const int currRdfId, const uint64_t udt, const uint64_t ud, const CH_COLLIDER_T* chCollider, const CharacterDownsync& currChd, CharacterDownsync* nextChd, const CharacterConfig* cc, bool cvSupported, bool cvInAir, bool cvOnWall, bool currNotDashing, bool currEffInAir, bool oldNextNotDashing, bool oldNextEffInAir, bool inJumpStartupOrJustEnded, CharacterBase::EGroundState cvGroundState, const InputInducedMotion* inputInducedMotion) {
    CharacterState oldNextChState = nextChd->ch_state();
   
    uint32_t activeSkillId = currChd.active_skill_id();
    int activeSkillHit = currChd.active_skill_hit();
    const Skill* activeSkill = nullptr;
    const BulletConfig* activeBulletConfig = nullptr;
    FindBulletConfig(activeSkillId, activeSkillHit, activeSkill, activeBulletConfig);
   
    const BuffConfig* activeSkillBuff = nullptr;
    if (nullptr != activeSkill && activeSkill->has_self_non_stock_buff()) {
        const BuffConfig& cand = activeSkill->self_non_stock_buff();
        if (globalPrimitiveConsts->terminating_buff_species_id() != cand.species_id()) {
            activeSkillBuff = &(cand);
        }
    }

    if (cvInAir) {
        if (!oldNextEffInAir) {
            switch (oldNextChState) {
            case Idle1:
            case Def1:
            case Def1Broken:
            case Walking:
                if (Walking == oldNextChState) {
                    if (cc->omit_gravity()) {
                        // [WARNING] No need to distinguish in this case.
                        break;
                    } else if (nextChd->omit_gravity()) {
                        if (cc->has_in_air_walking_anim()) {
                            nextChd->set_ch_state(InAirWalking);
                        }
                        break;
                    }
                }
                if (Idle1 == oldNextChState) {
                    auto defaultInAirIdleChState = cc->use_idle1_as_flying_idle() ? Idle1 : InAirWalking;
                    if (cc->omit_gravity()) {

                    } else if (nextChd->omit_gravity()) {
                        nextChd->set_ch_state(defaultInAirIdleChState);
                        break;
                    }
                }
                if ((OnWallIdle1 == currChd.ch_state() && inputInducedMotion->jumpTriggered) || InAirIdle1ByWallJump == currChd.ch_state()) {
                    nextChd->set_ch_state(InAirIdle1ByWallJump);
                } else if (inputInducedMotion->jumpTriggered || InAirIdle1ByJump == currChd.ch_state()) {
                    nextChd->set_ch_state(InAirIdle1ByJump);
                } else if (inputInducedMotion->jumpTriggered || InAirIdle2ByJump == currChd.ch_state()) {
                    nextChd->set_ch_state(InAirIdle2ByJump);
                } else {
                    nextChd->set_ch_state(InAirIdle1NoJump);
                }
                if (Def1 == oldNextChState) {
                    nextChd->set_remaining_def1_quota(0);
                }
                break;
            case TurnAround:
                if (cc->omit_gravity()) {
                    // [WARNING] No need to distinguish in this case.
                    break;
                } else {
                    if (cc->has_in_air_turn_around_anim()) {
                        nextChd->set_ch_state(InAirTurnAround);
                    }
                    break;
                }
            }
        }
    } else {
        // next frame NOT in air
        if (oldNextEffInAir && nonAttackingSet.count(oldNextChState) && !onWallSet.count(oldNextChState) && BlownUp1 != oldNextChState) {
            switch (oldNextChState) {
            case InAirIdle1NoJump:
            case InAirIdle2ByJump:
                if (0 != inputInducedMotion->forceCOM.GetX()) {
                    nextChd->set_ch_state(Walking);
                } else {
                    nextChd->set_ch_state(Idle1);
                }
                break;
            case InAirIdle1ByJump:
            case InAirIdle1ByWallJump:
                if (isJumpStartupJustEnded(currChd, nextChd, cc) || isInJumpStartup(*nextChd, cc)) {
                    // [WARNING] Don't change CharacterState in this special case!
                    break;
                }
                if (0 != inputInducedMotion->forceCOM.GetX()) {
                    nextChd->set_ch_state(Walking);
                } else {
                    nextChd->set_ch_state(Idle1);
                }
                break;
            case InAirIdle1BySlipJump:
                if (isCrouching(currChd.ch_state(), cc)) {
                    // An edge case, the character might be trying to slip jump on a non-slippable platform
                    nextChd->set_ch_state(currChd.ch_state());
                } else {
                    if (0 != inputInducedMotion->forceCOM.GetX()) {
                        nextChd->set_ch_state(Walking);
                    } else {
                        nextChd->set_ch_state(Idle1);
                    }
                }
                break;
            case Def1:
            case Def1Broken:
                // Not changing anything.
                break;
            case InAirWalking:
                nextChd->set_ch_state(Walking);
                break;
            case InAirTurnAround:
                nextChd->set_ch_state(TurnAround);
                break;
            default:
                if (0 != inputInducedMotion->forceCOM.GetX()) {
                    nextChd->set_ch_state(Walking);
                } else {
                    nextChd->set_ch_state(Idle1);
                }
                break;
            }
        }
    }

    // Reset "FramesInChState" if "CharacterState" is changed
    if (nextChd->ch_state() != currChd.ch_state()) {
        nextChd->set_frames_in_ch_state(0);
    }
    if (lowerPartForwardTransitionSet.count(std::make_pair<CharacterState, CharacterState>(currChd.ch_state(), nextChd->ch_state()))) {
        nextChd->set_lower_part_rdf_cnt(currChd.ch_state() + 1);
    } else if (lowerPartReverseTransitionSet.count(std::make_pair<CharacterState, CharacterState>(currChd.ch_state(), nextChd->ch_state()))) {
        nextChd->set_frames_in_ch_state(currChd.lower_part_rdf_cnt() + 1);
        nextChd->set_lower_part_rdf_cnt(globalPrimitiveConsts->terminating_lower_part_rdf_cnt());
    } else if (lowerPartInheritTransitionSet.count(std::make_pair<CharacterState, CharacterState>(currChd.ch_state(), nextChd->ch_state()))) {
        nextChd->set_lower_part_rdf_cnt(currChd.lower_part_rdf_cnt() + 1);
    } else {
        nextChd->set_lower_part_rdf_cnt(globalPrimitiveConsts->terminating_lower_part_rdf_cnt());
    }

    // Remove any active skill if not attacking
    bool notDashing = BaseBattleCollisionFilter::chIsNotDashing(*nextChd);
    if (nonAttackingSet.count(nextChd->ch_state()) && notDashing) {
        nextChd->set_active_skill_id(globalPrimitiveConsts->no_skill());
        nextChd->set_active_skill_hit(globalPrimitiveConsts->no_skill_hit());
    }

    if ((InAirAtked1 == nextChd->ch_state() || CrouchAtked1 == nextChd->ch_state() || Atked1 == nextChd->ch_state()) && (UINT_MAX >> 1) < nextChd->frames_to_recover()) {
        nextChd->set_ch_state(BlownUp1);
        if (nextChd->omit_gravity()) {
            if (cc->omit_gravity()) {
                nextChd->set_frames_to_recover(globalPrimitiveConsts->default_blownup_frames_for_flying());
            } else {
                nextChd->set_omit_gravity(false);
            }
        }
    }

    if (Def1 != nextChd->ch_state()) {
        nextChd->set_remaining_def1_quota(0);
    } else {
        bool isWalkingAutoDef1 = (Walking == currChd.ch_state() && cc->walking_auto_def1());
        bool isSkillAutoDef1 = (nullptr != activeSkillBuff && activeSkillBuff->auto_def1());
        if (Def1 != currChd.ch_state() && !isWalkingAutoDef1 && !isSkillAutoDef1) {
            nextChd->set_damaged_hint_rdf_countdown(0); // Clean up for correctly animating "Def1Atked1"
            nextChd->set_damaged_elemental_attrs(0);
        }
    }

    if (cvInAir) {
        bool omitGravity = (currChd.omit_gravity() || cc->omit_gravity());
        if (!omitGravity && globalPrimitiveConsts->no_skill() != currChd.active_skill_id()) {
            if (nullptr != activeSkill) {
                omitGravity |= (nullptr != activeSkillBuff && activeSkillBuff->omit_gravity());
            }
        }
    }

    if (!cvInAir && nullptr != activeSkill && activeSkill->bound_ch_state() == nextChd->ch_state() && nullptr != activeBulletConfig && activeBulletConfig->ground_impact_melee_collision()) {
        // [WARNING] The "bulletCollider" for "activeBulletConfig" in this case might've been annihilated, we should end this bullet regardless of landing on character or hardPushback.
        int origFramesInActiveState = (nextChd->frames_in_ch_state() - activeBulletConfig->startup_frames()); // correct even for "DemonDiverImpactPreJumpBullet -> DemonDiverImpactStarterBullet" sequence
        auto shiftedRdfCnt = (activeBulletConfig->active_frames() - origFramesInActiveState);
        if (0 < shiftedRdfCnt) {
            nextChd->set_frames_in_ch_state(nextChd->frames_in_ch_state() + shiftedRdfCnt);
            nextChd->set_frames_to_recover(nextChd->frames_to_recover() - shiftedRdfCnt);
        }
        if (0 > origFramesInActiveState) {
            nextChd->set_active_skill_id(globalPrimitiveConsts->no_skill());
            nextChd->set_active_skill_hit(globalPrimitiveConsts->no_skill_hit());
        }
        // [WARNING] Leave velocity handling to other code snippets.
    } else if (cvOnWall && nullptr != activeSkill && activeSkill->bound_ch_state() == nextChd->ch_state() && nullptr != activeBulletConfig && activeBulletConfig->wall_impact_melee_collision()) {
        // [WARNING] The "bulletCollider" for "activeBulletConfig" in this case might've been annihilated, we should end this bullet regardless of landing on character or hardPushback.
        int origFramesInActiveState = (nextChd->frames_in_ch_state() - activeBulletConfig->startup_frames()); // correct even for "DemonDiverImpactPreJumpBullet -> DemonDiverImpactStarterBullet" sequence
        int shiftedRdfCnt = (activeBulletConfig->active_frames() - origFramesInActiveState);
        if (0 < shiftedRdfCnt) {
            nextChd->set_frames_in_ch_state(nextChd->frames_in_ch_state() + shiftedRdfCnt);
            nextChd->set_frames_to_recover(nextChd->frames_to_recover() - shiftedRdfCnt);
        }
        if (0 > origFramesInActiveState) {
            nextChd->set_active_skill_id(globalPrimitiveConsts->no_skill());
            nextChd->set_active_skill_hit(globalPrimitiveConsts->no_skill_hit());
        }
        // [WARNING] Leave velocity handling to other code snippets.
    } else if (nullptr != activeSkill && activeSkill->bound_ch_state() == nextChd->ch_state() && nullptr != activeBulletConfig && (MultiHitType::FromEmission == activeBulletConfig->mh_type() || MultiHitType::FromEmissionJustActive == activeBulletConfig->mh_type()) && currChd.frames_in_ch_state() > activeBulletConfig->startup_frames() + activeBulletConfig->active_frames() + activeBulletConfig->finishing_frames()) {
        int origFramesInActiveState = (nextChd->frames_in_ch_state() - activeBulletConfig->startup_frames()); // correct even for "DemonDiverImpactPreJumpBullet -> DemonDiverImpactStarterBullet" sequence
        auto shiftedRdfCnt = (activeBulletConfig->active_frames() - origFramesInActiveState);
        if (0 < shiftedRdfCnt) {
            nextChd->set_frames_in_ch_state(nextChd->frames_in_ch_state() + shiftedRdfCnt);
            nextChd->set_frames_to_recover(nextChd->frames_to_recover() - shiftedRdfCnt);
        }
        if (0 > origFramesInActiveState) {
            nextChd->set_active_skill_id(globalPrimitiveConsts->no_skill());
            nextChd->set_active_skill_hit(globalPrimitiveConsts->no_skill_hit());
        }
    }

    if (cvSupported && currEffInAir && !inJumpStartupOrJustEnded) {
        // fall stopping
/*
#ifndef NDEBUG
        std::ostringstream oss;
        switch (udt) {
        case UDT_PLAYER:
            auto joinIndex = getUDPayload(ud);
            if (1 == joinIndex) {
                
                oss << "@currRdfId=" << currRdfId << " player joinIndex=" << joinIndex << " at currPos=(" << currChd.x() <<  ", " << currChd.y() << "), ch_state=" << (int)currChd.ch_state() << ", vel=(" << currChd.vel_x() << ", " << currChd.vel_y() << ")" << " just landed; about to set nextVel=(" << nextChd->vel_x() << ", " << nextChd->vel_y() << "), nextPos=(" << nextChd->x() << ", " << nextChd->y() << ")";
                Debug::Log(oss.str(), DColor::Orange);
            }
        break;
        }
#endif
*/
        nextChd->set_remaining_air_jump_quota(cc->default_air_jump_quota());
        nextChd->set_remaining_air_dash_quota(cc->default_air_dash_quota());
        int effInertiaRdfCountdown = cc->fallstopping_inertia_rdf_count();
        if (0 >= effInertiaRdfCountdown) {
            effInertiaRdfCountdown = 8;
        }
        nextChd->set_fallstopping_rdf_countdown(effInertiaRdfCountdown);
    } else if (onWallSet.count(nextChd->ch_state()) && !onWallSet.count(currChd.ch_state())) {
        nextChd->set_remaining_air_jump_quota(cc->default_air_jump_quota());
        nextChd->set_remaining_air_dash_quota(cc->default_air_dash_quota());
    }

#ifndef NDEBUG
    /*
    if (Atk1 == oldNextChState || WalkingAtk1 == oldNextChState || InAirAtk1 == oldNextChState) {
        if (oldNextChState != nextChd->ch_state()) {
            std::ostringstream oss1;
            oss1 << "@currRdfId=" << currRdfId << ", postStepSingleChdStateCorrection/set nextChd ch_state=" << nextChd->ch_state() << " and frames_in_ch_state=" << nextChd->frames_in_ch_state() << "; while oldNextChState=" << oldNextChState;
            Debug::Log(oss1.str(), DColor::Orange);
        }
    }

    if (OnWallAtk1 == nextChd->ch_state() || OnWallIdle1 == nextChd->ch_state()) {
        std::ostringstream oss1;
        oss1 << "@currRdfId=" << currRdfId << ", postStepSingleChdStateCorrection/set nextChd ch_state=" << nextChd->ch_state() << " and frames_in_ch_state=" << nextChd->frames_in_ch_state() << "; while currChState=" << currChd.ch_state() << ", currFramesInChState=" << currChd.frames_in_ch_state() << ", oldNextChState=" << oldNextChState << "; cvSupported=" << cvSupported << ", cvInAir=" << cvInAir << ", cvOnWall=" << cvOnWall << ", currNotDashing=" << currNotDashing << ", currEffInAir=" << currEffInAir << ", oldNextNotDashing=" << oldNextNotDashing << ", oldNextEffInAir=" << oldNextEffInAir << ", inJumpStartupOrJustEnded=" << inJumpStartupOrJustEnded << ", cvGroundState=" << CharacterBase::sToString(cvGroundState);
        Debug::Log(oss1.str(), DColor::Orange);
    }
    */
#endif // !NDEBUG
}

void BaseBattle::leftShiftDeadNpcs(const int currRdfId, RenderFrame* nextRdf) {
    int aliveI = 0, candI = 0;
    auto& characterConfigs = globalConfigConsts->character_configs();
    int mNextRdfNpcCountVal = mNextRdfNpcCount.load();
    while (candI < mNextRdfNpcCountVal) {
        const NpcCharacterDownsync* candidate = &(nextRdf->npcs(candI));
        if (globalPrimitiveConsts->terminating_character_id() == candidate->id()) break;
        const CharacterDownsync* chd = &(candidate->chd());
        const CharacterConfig& candidateConfig = characterConfigs.at(chd->species_id());

        /*
        // TODO: Drop pickable
        */
        
        while (candI < mNextRdfNpcCountVal && globalPrimitiveConsts->terminating_character_id() != candidate->id() && isNpcDeadToDisappear(chd)) {
            if (globalPrimitiveConsts->terminating_trigger_id() != candidate->publishing_to_trigger_id_upon_exhausted()) {
                auto triggerUd = calcPublishingToTriggerUd(*candidate);
                auto targetTriggerInNextFrame = transientUdToNextTrigger[triggerUd];
                if (0 == chd->last_damaged_by_ud() && globalPrimitiveConsts->terminating_bullet_team_id() == chd->last_damaged_by_bullet_team_id()) {
                    if (1 == playersCnt) {
                        publishToTrigger(currRdfId, candidate->publishing_mask_upon_exhausted(), 1, 1, targetTriggerInNextFrame);
                    } else {
                        publishToTrigger(currRdfId, candidate->publishing_mask_upon_exhausted(), chd->last_damaged_by_ud(), chd->last_damaged_by_bullet_team_id(), targetTriggerInNextFrame);
                    }
                } else {
                    publishToTrigger(currRdfId, candidate->publishing_mask_upon_exhausted(), chd->last_damaged_by_ud(), chd->last_damaged_by_bullet_team_id(), targetTriggerInNextFrame);
                }
            }
            candI++;
            if (candI >= mNextRdfNpcCountVal) break;
            candidate = &(nextRdf->npcs(candI));
            chd = &(candidate->chd());
        }

        if (candI >= mNextRdfNpcCountVal || globalPrimitiveConsts->terminating_character_id() == nextRdf->npcs(candI).id()) {
            break;
        }

        if (candI != aliveI) {
            const NpcCharacterDownsync& src = nextRdf->npcs(candI);
            NpcCharacterDownsync* dst = nextRdf->mutable_npcs(aliveI);
            CopyNpcChd(&src, dst);
        }

        candI++;
        aliveI++;
    }
    if (aliveI < mNextRdfNpcCountVal) {
        auto terminatingCand = nextRdf->mutable_npcs(aliveI);
        terminatingCand->set_id(globalPrimitiveConsts->terminating_character_id());
    }
    mNextRdfNpcCount = aliveI;
}

void BaseBattle::calcFallenDeath(const RenderFrame* currRdf, RenderFrame* nextRdf) {
    auto& chConfigs = globalConfigConsts->character_configs();

    int currRdfId = currRdf->id();
    for (int i = 0; i < nextRdf->players_size(); i++) {
        if (i >= currRdf->players_size()) break;
        const PlayerCharacterDownsync& currPlayer = currRdf->players(i);
        auto nextPlayer = nextRdf->mutable_players(i);
        auto chd = nextPlayer->mutable_chd();
        auto chConfig = chConfigs.at(chd->species_id());
        float chTop = chd->y() + 2*chConfig.capsule_half_height();
        if (fallenDeathHeight > chTop && Dying != chd->ch_state()) {
            transitToDying(currRdfId, currPlayer, true, nextPlayer);
        }
    }

    for (int i = 0; i < nextRdf->npcs_size(); i++) {
        const NpcCharacterDownsync& currNpc = currRdf->npcs(i);
        if (globalPrimitiveConsts->terminating_character_id() == currNpc.id()) break;
        auto nextNpc = nextRdf->mutable_npcs(i);
        auto chd = nextNpc->mutable_chd();
        const CharacterConfig& chConfig = chConfigs.at(chd->species_id());
        float chTop = chd->y() + 2 * chConfig.capsule_half_height();
        if (fallenDeathHeight > chTop && Dying != chd->ch_state()) {
            transitToDying(currRdfId, currNpc, true, nextNpc);
        }
    }

    for (int i = 0; i < nextRdf->pickables_size(); i++) {
        auto nextPickable = nextRdf->mutable_pickables(i);
        if (globalPrimitiveConsts->terminating_pickable_id() == nextPickable->id()) break;
        auto pickableTop = nextPickable->y() + globalPrimitiveConsts->default_pickable_hurtbox_half_size_y();
        if (fallenDeathHeight > pickableTop && PickableState::PIdle == nextPickable->pk_state()) {
            nextPickable->set_pk_state(PickableState::PDisappearing);
            nextPickable->set_frames_in_pk_state(0);
            nextPickable->set_remaining_lifetime_rdf_count(globalPrimitiveConsts->default_pickable_disappearing_anim_frames());
        }
    }
}

void BaseBattle::leftShiftDeadBullets(const int currRdfId, RenderFrame* nextRdf) {
    int aliveI = 0, candI = 0;
    int mNextRdfBulletCountVal = mNextRdfBulletCount.load();
    while (candI < mNextRdfBulletCountVal) {
        const Bullet* candidate = &(nextRdf->bullets(candI));
        if (globalPrimitiveConsts->terminating_bullet_id() == candidate->id()) {
            break;
        }
        uint32_t skillId = candidate->skill_id();
        int skillHit = candidate->active_skill_hit();
        const Skill* skillConfig = nullptr;
        const BulletConfig* bulletConfig = nullptr;
        FindBulletConfig(skillId, skillHit, skillConfig, bulletConfig);

        while (candI < mNextRdfBulletCountVal && globalPrimitiveConsts->terminating_bullet_id() != candidate->id() && !isBulletAlive(candidate, bulletConfig, currRdfId)) {
            candI++;
            if (candI >= mNextRdfBulletCountVal) break;
            candidate = &(nextRdf->bullets(candI));
        }

        if (candI >= mNextRdfBulletCountVal || globalPrimitiveConsts->terminating_bullet_id() == nextRdf->bullets(candI).id()) {
            break;
        }

        if (candI != aliveI) {
            auto src = nextRdf->bullets(candI);
            Bullet* dst = nextRdf->mutable_bullets(aliveI);
            CopyBullet(&src, dst);
        }

        candI++;
        aliveI++;
    }
    if (aliveI < mNextRdfBulletCountVal) {
        auto terminatingCand = nextRdf->mutable_bullets(aliveI);
        terminatingCand->set_id(globalPrimitiveConsts->terminating_bullet_id());
    }
    mNextRdfBulletCount = aliveI;
}

void BaseBattle::leftShiftDeadPickables(const int currRdfId, RenderFrame* nextRdf) {
    int aliveI = 0, candI = 0;
    while (candI < nextRdf->pickables_size()) {
        const Pickable* cand = &(nextRdf->pickables(candI));
        if (globalPrimitiveConsts->terminating_pickable_id() == cand->id()) {
            break;
        }
        while (candI < nextRdf->pickables_size() && globalPrimitiveConsts->terminating_pickable_id() != cand->id() && !isPickableAlive(cand, currRdfId)) {
            candI++;
            if (candI >= nextRdf->pickables_size()) break;
            cand = &(nextRdf->pickables(candI));
        }

        if (candI >= nextRdf->pickables_size() || globalPrimitiveConsts->terminating_pickable_id() == nextRdf->pickables(candI).id()) {
            break;
        }

        if (candI != aliveI) {
            const Pickable& src = nextRdf->pickables(candI);
            Pickable* dst = nextRdf->mutable_pickables(aliveI);
            CopyPickable(&src, dst);
        }

        candI++;
        aliveI++;
    }
    if (aliveI < nextRdf->pickables_size()) {
        auto terminatingCand = nextRdf->mutable_pickables(aliveI);
        terminatingCand->set_id(globalPrimitiveConsts->terminating_pickable_id());
    }
    nextRdf->set_pickable_count(aliveI);
}

void BaseBattle::leftShiftDeadDynamicTraps(const int currRdfId, RenderFrame* nextRdf) {
    int aliveI = 0, candI = 0;
    while (candI < nextRdf->dynamic_traps_size()) {
        const Trap* cand = &(nextRdf->dynamic_traps(candI));
        if (globalPrimitiveConsts->terminating_trap_id() == cand->id()) {
            break;
        }
        while (candI < nextRdf->dynamic_traps_size() && globalPrimitiveConsts->terminating_trap_id() != cand->id() && !isTrapAlive(cand, currRdfId)) {
            candI++;
            if (candI >= nextRdf->dynamic_traps_size()) break;
            cand = &(nextRdf->dynamic_traps(candI));
        }

        if (candI >= nextRdf->dynamic_traps_size() || globalPrimitiveConsts->terminating_trap_id() == nextRdf->dynamic_traps(candI).id()) {
            break;
        }

        if (candI != aliveI) {
            const Trap& src = nextRdf->dynamic_traps(candI);
            Trap* dst = nextRdf->mutable_dynamic_traps(aliveI);
            CopyTrap(&src, dst);
        }

        candI++;
        aliveI++;
    }
    if (aliveI < nextRdf->dynamic_traps_size()) {
        auto terminatingCand = nextRdf->mutable_dynamic_traps(aliveI);
        terminatingCand->set_id(globalPrimitiveConsts->terminating_trap_id());
    }
    nextRdf->set_dynamic_trap_count(aliveI);
}

void BaseBattle::leftShiftDeadTriggers(const int currRdfId, RenderFrame* nextRdf) {
    int aliveI = 0, candI = 0;
    while (candI < nextRdf->triggers_size()) {
        const Trigger* cand = &(nextRdf->triggers(candI));
        if (globalPrimitiveConsts->terminating_trigger_id() == cand->id()) {
            break;
        }
        while (candI < nextRdf->triggers_size() && globalPrimitiveConsts->terminating_trigger_id() != cand->id() && !isTriggerAlive(cand, currRdfId)) {
            candI++;
            if (candI >= nextRdf->triggers_size()) break;
            cand = &(nextRdf->triggers(candI));
        }

        if (candI >= nextRdf->triggers_size() || globalPrimitiveConsts->terminating_trigger_id() == nextRdf->triggers(candI).id()) {
            break;
        }

        if (candI != aliveI) {
            const Trigger& src = nextRdf->triggers(candI);
            Trigger* dst = nextRdf->mutable_triggers(aliveI);
            CopyTrigger(&src, dst);
        }

        candI++;
        aliveI++;
    }
    if (aliveI < nextRdf->triggers_size()) {
        auto terminatingCand = nextRdf->mutable_triggers(aliveI);
        terminatingCand->set_id(globalPrimitiveConsts->terminating_trigger_id());
    }
    nextRdf->set_trigger_count(aliveI);
}

void BaseBattle::leftShiftDeadBlImmuneRecords(int currRdfId, CharacterDownsync* nextChd) {
    int aliveI = 0, candI = 0;
    while (candI < nextChd->bullet_immune_records_size()) {
        const BulletImmuneRecord* cand = &(nextChd->bullet_immune_records(candI));
        if (globalPrimitiveConsts->terminating_bullet_id() == cand->bullet_id()) {
            break;
        }

        while (globalPrimitiveConsts->terminating_bullet_id() != cand->bullet_id() && 0 >= cand->remaining_lifetime_rdf_count()) {
            // meets a dead "BulletImmuneRecord", moves to next
            candI++;
            if (candI >= nextChd->bullet_immune_records_size()) break;
            cand = &(nextChd->bullet_immune_records(candI));
        }

        if (candI >= nextChd->bullet_immune_records_size() || globalPrimitiveConsts->terminating_bullet_id() == nextChd->bullet_immune_records(candI).bullet_id()) {
            break;
        }

        if (candI != aliveI) {
            const BulletImmuneRecord& src = nextChd->bullet_immune_records(candI);
            auto dst = nextChd->mutable_bullet_immune_records(aliveI);
            dst->CopyFrom(src); // Cheap because all fields of "BulletImmuneRecord" are primitive types
        }

        candI++;
        aliveI++;
    }

    if (aliveI < nextChd->bullet_immune_records_size()) {
        auto terminatingCand = nextChd->mutable_bullet_immune_records(aliveI);
        terminatingCand->set_bullet_id(globalPrimitiveConsts->terminating_bullet_id());
        terminatingCand->set_remaining_lifetime_rdf_count(0);
    }
}

void BaseBattle::leftShiftDeadIvSlots(const int currRdfId, CharacterDownsync* nextChd) {
    int aliveI = 0, candI = 0;
    int ivSlotsCnt = nextChd->inventory_slots_size();
    while (candI < ivSlotsCnt) {
        const InventorySlot* cand = &(nextChd->inventory_slots(candI));
        if (InventorySlotStockType::NoneIv == cand->stock_type()) {
            break;
        }

        while (InventorySlotStockType::NoneIv != cand->stock_type() && 0 >= cand->quota()) {
            candI++;
            if (candI >= ivSlotsCnt) break;
            cand = &(nextChd->inventory_slots(candI));
        }

        if (candI >= ivSlotsCnt || InventorySlotStockType::NoneIv == nextChd->inventory_slots(candI).stock_type()) {
            break;
        }

        if (candI != aliveI) {
            const InventorySlot& src = nextChd->inventory_slots(candI);
            InventorySlot* dst = nextChd->mutable_inventory_slots(aliveI);
            dst->CopyFrom(src); // Cheap because all fields of "InventorySlot" are primitive types
        }

        candI++;
        aliveI++;
    }

    if (aliveI < ivSlotsCnt) {
        auto terminatingCand = nextChd->mutable_inventory_slots(aliveI);
        terminatingCand->set_stock_type(InventorySlotStockType::NoneIv);
        terminatingCand->set_quota(0);
    }
}

void BaseBattle::leftShiftDeadBuffs(const int currRdfId, CharacterDownsync* nextChd) {
    int aliveI = 0, candI = 0;
    while (candI < nextChd->buff_list_size()) {
        const Buff* cand = &(nextChd->buff_list(candI));
        if (globalPrimitiveConsts->terminating_buff_species_id() == cand->species_id()) {
            break;
        }

        while (globalPrimitiveConsts->terminating_buff_species_id() != cand->species_id() && 0 >= cand->stock()) {
            candI++;
            if (candI >= nextChd->buff_list_size()) break;
            cand = &(nextChd->buff_list(candI));
        }

        if (candI >= nextChd->buff_list_size() || globalPrimitiveConsts->terminating_buff_species_id() == nextChd->buff_list(candI).species_id()) {
            break;
        }

        if (candI != aliveI) {
            const Buff& src = nextChd->buff_list(candI);
            Buff* dst = nextChd->mutable_buff_list(aliveI);
            dst->CopyFrom(src); // Cheap because all fields of "Buff" are primitive types
        }

        candI++;
        aliveI++;
    }

    if (aliveI < nextChd->buff_list_size()) {
        auto terminatingCand = nextChd->mutable_buff_list(aliveI);
        terminatingCand->set_species_id(globalPrimitiveConsts->terminating_buff_species_id());
        terminatingCand->set_stock(0);
    }
}

void BaseBattle::leftShiftDeadDebuffs(const int currRdfId, CharacterDownsync* nextChd) {
    int aliveI = 0, candI = 0;
    while (candI < nextChd->debuff_list_size()) {
        const Debuff* cand = &(nextChd->debuff_list(candI));
        if (globalPrimitiveConsts->terminating_debuff_species_id() == cand->species_id()) {
            break;
        }

        while (globalPrimitiveConsts->terminating_debuff_species_id() != cand->species_id() && 0 >= cand->stock()) {
            candI++;
            if (candI >= nextChd->debuff_list_size()) break;
            cand = &(nextChd->debuff_list(candI));
        }

        if (candI >= nextChd->debuff_list_size() || globalPrimitiveConsts->terminating_debuff_species_id() == nextChd->debuff_list(candI).species_id()) {
            break;
        }

        if (candI != aliveI) {
            const Debuff& src = nextChd->debuff_list(candI);
            Debuff* dst = nextChd->mutable_debuff_list(aliveI);
            dst->CopyFrom(src); // Cheap because all fields of "Debuff" are primitive types
        }

        candI++;
        aliveI++;
    }

    if (aliveI < nextChd->debuff_list_size()) {
        auto terminatingCand = nextChd->mutable_debuff_list(aliveI);
        terminatingCand->set_species_id(globalPrimitiveConsts->terminating_debuff_species_id());
        terminatingCand->set_stock(0);
    }
}

void BaseBattle::CopyIfd(const InputFrameDownsync* from, InputFrameDownsync* to) {
    to->CopyFrom(*from);
}

void BaseBattle::CopyRdf(const RenderFrame* from, RenderFrame* to) {
    to->set_id(from->id());
    to->set_countdown_nanos(from->countdown_nanos());
    to->set_bullet_id_counter(from->bullet_id_counter());
    to->set_bullet_count(from->bullet_count());
    to->set_npc_id_counter(from->npc_id_counter());
    to->set_npc_count(from->npc_count());
    to->set_trigger_count(from->trigger_count());
    to->set_pickable_id_counter(from->pickable_id_counter());
    to->set_pickable_count(from->pickable_count());
    to->set_dynamic_trap_count(from->dynamic_trap_count());
    
    if (to->has_prev_rdf_step_result()) {
        to->mutable_prev_rdf_step_result()->Clear();
    }
    to->mutable_prev_rdf_step_result()->MergeFrom(from->prev_rdf_step_result());

    // [WARNING] Repeated fields of "RenderFrame" are all preallocated, therefore their sizes are all aligned.
    for (int i = 0; i < from->players_size(); i++) {
        const PlayerCharacterDownsync* fromSingle = &(from->players(i));
        PlayerCharacterDownsync * toSingle = i < to->players_size() ? to->mutable_players(i) : to->add_players();
        CopyPlayerChd(fromSingle, toSingle);
    }
    if (from->players_size() < to->players_size()) {
        PlayerCharacterDownsync* toSingle = to->mutable_players(from->players_size());
        toSingle->set_join_index(globalPrimitiveConsts->magic_join_index_invalid());
    }

    for (int i = 0; i < from->npcs_size(); i++) {
        const NpcCharacterDownsync* fromSingle = &(from->npcs(i));
        NpcCharacterDownsync* toSingle = i < to->npcs_size() ? to->mutable_npcs(i) : to->add_npcs();
        CopyNpcChd(fromSingle, toSingle);
    }
    if (from->npcs_size() < to->npcs_size()) {
        NpcCharacterDownsync* toSingle = to->mutable_npcs(from->npcs_size());
        toSingle->set_id(globalPrimitiveConsts->terminating_character_id());
    }

    for (int i = 0; i < from->bullets_size(); i++) {
        const Bullet* fromSingle = &(from->bullets(i));
        Bullet* toSingle = i < to->bullets_size() ? to->mutable_bullets(i) : to->add_bullets();
        CopyBullet(fromSingle, toSingle);
    }
    if (from->bullets_size() < to->bullets_size()) {
        Bullet* toSingle = to->mutable_bullets(from->bullets_size());
        toSingle->set_id(globalPrimitiveConsts->terminating_bullet_id());
    }

    for (int i = 0; i < from->dynamic_traps_size(); i++) {
        const Trap* fromSingle = &(from->dynamic_traps(i));
        Trap* toSingle = i < to->dynamic_traps_size() ? to->mutable_dynamic_traps(i) : to->add_dynamic_traps();
        CopyTrap(fromSingle, toSingle);
    }
    if (from->dynamic_traps_size() < to->dynamic_traps_size()) {
        Trap* toSingle = to->mutable_dynamic_traps(from->dynamic_traps_size());
        toSingle->set_id(globalPrimitiveConsts->terminating_trap_id());
    }

    for (int i = 0; i < from->triggers_size(); i++) {
        const Trigger* fromSingle = &(from->triggers(i));
        Trigger* toSingle = i < to->triggers_size() ? to->mutable_triggers(i) : to->add_triggers();
        CopyTrigger(fromSingle, toSingle);
    }
    if (from->triggers_size() < to->triggers_size()) {
        Trigger* toSingle = to->mutable_triggers(from->triggers_size());
        toSingle->set_id(globalPrimitiveConsts->terminating_trigger_id());
    }

    for (int i = 0; i < from->pickables_size(); i++) {
        const Pickable* fromSingle = &(from->pickables(i));
        Pickable* toSingle = i < to->pickables_size() ? to->mutable_pickables(i) : to->add_pickables();
        CopyPickable(fromSingle, toSingle);
    }
    if (from->pickables_size() < to->pickables_size()) {
        Pickable* toSingle = to->mutable_pickables(from->pickables_size());
        toSingle->set_id(globalPrimitiveConsts->terminating_pickable_id());
    }
}

void BaseBattle::CopyPlayerChd(const PlayerCharacterDownsync* from, PlayerCharacterDownsync* to) {
    to->set_join_index(from->join_index());

    to->set_beats_cnt(to->beats_cnt());
    to->set_beaten_cnt(to->beaten_cnt());

    to->set_revival_x(from->revival_x());
    to->set_revival_y(from->revival_y());
    to->set_revival_z(from->revival_z());

    to->set_revival_q_x(from->revival_q_x());
    to->set_revival_q_y(from->revival_q_y());
    to->set_revival_q_z(from->revival_q_z());
    to->set_revival_q_w(from->revival_q_w());

    to->set_not_enough_mp_hint_rdf_countdown(from->not_enough_mp_hint_rdf_countdown());

    to->set_cached_cue_cmd(from->cached_cue_cmd());
    to->set_goal_as_npc(from->goal_as_npc());

    CopyChd(&(from->chd()), to->mutable_chd());
}

void BaseBattle::CopyNpcChd(const NpcCharacterDownsync* from, NpcCharacterDownsync* to) {

    to->set_id(from->id());

    to->set_activated_rdf_id(from->activated_rdf_id());
    to->set_cached_cue_cmd(from->cached_cue_cmd());

    to->set_waiving_patrol_cue_id(from->waiving_patrol_cue_id());
    to->set_goal_as_npc(from->goal_as_npc());

    to->set_publishing_mask_upon_exhausted(from->publishing_mask_upon_exhausted()); 
    to->set_publishing_to_trigger_id_upon_exhausted(from->publishing_to_trigger_id_upon_exhausted());

    to->set_subscribes_to_trigger_id(from->subscribes_to_trigger_id()); 
    to->set_subscribes_to_trigger_group_id(from->subscribes_to_trigger_group_id()); 

    to->set_captured_by_patrol_cue(from->captured_by_patrol_cue());
    to->set_frames_in_patrol_cue(from->frames_in_patrol_cue());

    to->set_exhausted_to_drop_consumable_species_id(from->exhausted_to_drop_consumable_species_id());
    to->set_exhausted_to_drop_buff_species_id(from->exhausted_to_drop_buff_species_id());
    to->set_exhausted_to_drop_pickup_skill_id(from->exhausted_to_drop_pickup_skill_id());

    to->set_is_main_tower_of_team(from->is_main_tower_of_team());

    CopyChd(&(from->chd()), to->mutable_chd());
}

void BaseBattle::CopyChd(const CharacterDownsync* from, CharacterDownsync* to) {
    // After checking its field types, I don't have a better choice at the moment
    to->clear_buff_list();
    to->mutable_buff_list()->MergeFrom(from->buff_list());

    to->clear_debuff_list();
    to->mutable_debuff_list()->MergeFrom(from->debuff_list());

    to->clear_inventory_slots();
    to->mutable_inventory_slots()->MergeFrom(from->inventory_slots());

    to->clear_bullet_immune_records();
    to->mutable_bullet_immune_records()->MergeFrom(from->bullet_immune_records());

    to->clear_kinematic_knobs();
    to->mutable_kinematic_knobs()->MergeFrom(from->kinematic_knobs());
    
    /*
    [REMINDER] Fields "atk1_magazine" and "super_atk_gauage" are why merely "to->CopyFrom(*from)" is not used, I'd like to avoid any "delete" or "re-assign and leave the old field object maintained by pb-arena" behaviour. 
    */
    if (to->has_atk1_magazine()) {
        to->mutable_atk1_magazine()->Clear();
    }
    to->mutable_atk1_magazine()->MergeFrom(from->atk1_magazine());

    if (to->has_super_atk_gauge()) {
        to->mutable_super_atk_gauge()->Clear();
    }
    to->mutable_super_atk_gauge()->MergeFrom(from->super_atk_gauge());

    to->set_x(from->x());
    to->set_y(from->y());
    to->set_z(from->z());

    to->set_q_x(from->q_x());
    to->set_q_y(from->q_y());
    to->set_q_z(from->q_z());
    to->set_q_w(from->q_w());

    to->set_vel_x(from->vel_x());
    to->set_vel_y(from->vel_y());
    to->set_vel_z(from->vel_z());

    to->set_ground_vel_x(from->ground_vel_x());
    to->set_ground_vel_y(from->ground_vel_y());
    to->set_ground_vel_z(from->ground_vel_z());

    to->set_aiming_q_x(from->aiming_q_x());
    to->set_aiming_q_y(from->aiming_q_y());
    to->set_aiming_q_z(from->aiming_q_z());
    to->set_aiming_q_w(from->aiming_q_w());

    to->set_walkstopping_rdf_countdown(from->walkstopping_rdf_countdown());
    to->set_fallstopping_rdf_countdown(from->fallstopping_rdf_countdown());

    to->set_omit_gravity(from->omit_gravity());

    to->set_species_id(from->species_id());

    to->set_ground_ud(from->ground_ud());

    to->set_frames_to_recover(from->frames_to_recover());

    to->set_new_birth_rdf_countdown(from->new_birth_rdf_countdown());

    to->set_frames_invinsible(from->frames_invinsible());

    to->set_bullet_team_id(from->bullet_team_id());
    to->set_remaining_air_jump_quota(from->remaining_air_jump_quota());
    to->set_remaining_air_dash_quota(from->remaining_air_dash_quota());

    to->set_damaged_hint_rdf_countdown(from->damaged_hint_rdf_countdown());
    to->set_damaged_elemental_attrs(from->damaged_elemental_attrs());

    to->set_remaining_def1_quota(from->remaining_def1_quota());

    to->set_combo_hit_cnt(from->combo_hit_cnt());
    to->set_combo_frames_remained(from->combo_frames_remained());

    to->set_last_damaged_by_ud(from->last_damaged_by_ud());
    to->set_last_damaged_by_bullet_team_id(from->last_damaged_by_bullet_team_id());
    
    to->set_active_skill_id(from->active_skill_id());
    to->set_active_skill_hit(from->active_skill_hit());

    to->set_btn_a_holding_rdf_cnt(from->btn_a_holding_rdf_cnt());
    to->set_btn_b_holding_rdf_cnt(from->btn_b_holding_rdf_cnt());
    to->set_btn_c_holding_rdf_cnt(from->btn_c_holding_rdf_cnt());
    to->set_btn_d_holding_rdf_cnt(from->btn_d_holding_rdf_cnt());
    to->set_btn_e_holding_rdf_cnt(from->btn_e_holding_rdf_cnt());
    to->set_parry_prep_rdf_cnt_down(from->parry_prep_rdf_cnt_down());
    to->set_mp_regen_rdf_countdown(from->mp_regen_rdf_countdown());

    to->set_flying_rdf_countdown(from->flying_rdf_countdown());

    to->set_locking_on_ud(from->locking_on_ud());

    to->set_ch_collision_team_id(from->ch_collision_team_id());
    to->set_speed(from->speed());

    to->set_hp(from->hp());
    to->set_mp(from->mp());

    to->set_ch_state(from->ch_state());
    to->set_frames_in_ch_state(from->frames_in_ch_state());   
    to->set_lower_part_rdf_cnt(from->lower_part_rdf_cnt());
}

void BaseBattle::CopyBullet(const Bullet* from, Bullet* to) {
    // All primitive
    to->CopyFrom(*from);
}

void BaseBattle::CopyTrap(const Trap* from, Trap* to) {
    // All primitive
    to->CopyFrom(*from);
}

void BaseBattle::CopyTrigger(const Trigger* from, Trigger* to) {
    // All primitive
    to->CopyFrom(*from);
}

void BaseBattle::CopyPickable(const Pickable* from, Pickable* to) {
    // All primitive
    to->CopyFrom(*from);
}

bool BaseBattle::useSkill(const int currRdfId, RenderFrame* nextRdf, const CharacterDownsync& currChd, const Vec3& currChdFacing, uint64_t ud, const CharacterConfig* cc, CharacterDownsync* nextChd, int effDx, int effDy, int patternId, bool currEffInAir, bool currCrouching, bool currOnWall, bool currDashing, bool currWalking, bool currInBlockStun, bool currAtked, bool currParalyzed, int& outSkillId, const Skill*& outSkill, const BulletConfig*& outPivotBc) {
    if (globalPrimitiveConsts->pattern_id_no_op() == patternId || globalPrimitiveConsts->pattern_id_unable_to_op() == patternId) {
        return false;
    }

    bool inJumpStartupOrJustEnded = (isInJumpStartup(*nextChd, cc) || isJumpStartupJustEnded(currChd, nextChd, cc));
    if (inJumpStartupOrJustEnded) {
        return false;
    }
    
    bool notRecovered = (0 < currChd.frames_to_recover());
    if (CharacterState::Parried == currChd.ch_state()) {
        notRecovered = (currChd.frames_in_ch_state() >= globalPrimitiveConsts->parried_frames_to_start_cancellable());
    }

    const google::protobuf::Map<uint32, Skill>& skillConfigs = globalConfigConsts->skill_configs();
    const Skill* activeSkillConfig = nullptr;
    const BulletConfig* activeBulletConfig = nullptr;
    int currActiveSkillId = currChd.active_skill_id();
    int currActiveSkillHit = currChd.active_skill_hit();
    int targetSkillId = globalPrimitiveConsts->no_skill();
    bool fromCancellation = false;
    if (globalPrimitiveConsts->no_skill() != currActiveSkillId && globalPrimitiveConsts->no_skill_hit() != currActiveSkillHit) {
        FindBulletConfig(currActiveSkillId, currActiveSkillHit, activeSkillConfig, activeBulletConfig);
        if (nullptr == activeSkillConfig || nullptr == activeBulletConfig) {
            return false;
        }

        if (globalPrimitiveConsts->pattern_hold_b() == patternId) {
            if (cc->has_btn_b_charging() && IsChargingAtkChState(currChd.ch_state())) {
                if (0 >= nextChd->frames_to_recover()) {
                    nextChd->set_frames_to_recover(activeSkillConfig->recovery_frames());
                }
            }
            return false;
        }

        if (currChd.frames_in_ch_state() < activeBulletConfig->cancellable_st_frame() || currChd.frames_in_ch_state() > activeBulletConfig->cancellable_ed_frame()) {
/*
#ifndef NDEBUG
            std::ostringstream oss;
            oss << "@currRdfId=" << currRdfId << ", character ud=" << ud << " failed to cancel transit at position=(" << currChd.x() << "," << currChd.y() << "), ch_state=" << currChd.ch_state() << ", frames_in_ch_state=" << currChd.frames_in_ch_state() << ", frames_to_recover=" << currChd.frames_to_recover() << ", currActiveSkillId=" << currActiveSkillId << ", currActiveSkillHit=" << currActiveSkillHit << ", activeBulletConfig.cancellable_st_frames=" << activeBulletConfig->cancellable_st_frame() << ", activeBulletConfig.cancellable_ed_frames=" << activeBulletConfig->cancellable_ed_frame();
            Debug::Log(oss.str(), DColor::Orange);
#endif
*/
            outSkillId = globalPrimitiveConsts->no_skill();
            outSkill = nullptr;
            outPivotBc = nullptr;
            return false; 
        }

        int encodedPattern = EncodePatternForCancelTransit(patternId, currEffInAir, currCrouching, currOnWall, currDashing, currWalking);
        auto cancelTransitDict = activeBulletConfig->cancel_transit();
        if (!cancelTransitDict.count(encodedPattern)) {
/*
#ifndef NDEBUG
            std::ostringstream oss;
            oss << "@currRdfId=" << currRdfId << ", character ud=" << ud << " failed to cancel transit at position=(" << currChd.x() << "," << currChd.y() << "), ch_state=" << currChd.ch_state() << ", frames_in_ch_state=" << currChd.frames_in_ch_state() << ", frames_to_recover=" << currChd.frames_to_recover() << ", currActiveSkillId=" << currActiveSkillId << ", currActiveSkillHit=" << currActiveSkillHit;
            Debug::Log(oss.str(), DColor::Orange);
#endif
*/
            return false;
        }

        targetSkillId = cancelTransitDict[encodedPattern];
        fromCancellation = true;
    } else {
        if (notRecovered) {
            return false;
        }
        int encodedPattern = EncodePatternForInitSkill(patternId, currEffInAir, currCrouching, currOnWall, currDashing, currWalking, currInBlockStun, currAtked, currParalyzed);
/*
#ifndef NDEBUG
        std::ostringstream oss1;
        oss1 << "@currRdfId=" << currRdfId << ", ud=" << ud << " tries to use init skill by (patternId=" << patternId << ", effDx=" << effDx << ", effDy=" << effDy << ", encodedPattern=" << encodedPattern << ")";
        Debug::Log(oss1.str(), DColor::Orange);
#endif // !NDEBUG
*/
        auto initSkillDict = &(cc->init_skill_transit());
        if (nullptr != battleSpecificConfig && 0 < battleSpecificConfig->character_overrides_size()) {
            auto& characterOverrides = battleSpecificConfig->character_overrides(); 
            if (characterOverrides.count(ud)) {
                auto& characterOverride = characterOverrides.at(ud); 
                if (0 < characterOverride.init_skill_transit_size()) {
                    initSkillDict = &(characterOverride.init_skill_transit());  
                }
            }
        }
        if (!initSkillDict->count(encodedPattern)) {
/*
#ifndef NDEBUG
            std::ostringstream oss;
            oss << "@currRdfId=" << currRdfId << ", ud=" << ud << " tries to use init skill by (patternId=" << patternId << ", effDx=" << effDx << ", effDy=" << effDy << ", encodedPattern=" << encodedPattern << "), but initSkillDict doesn't contain it!";
            Debug::Log(oss.str(), DColor::Yellow);
#endif // !NDEBUG
*/
            return false;
        }
        targetSkillId = initSkillDict->at(encodedPattern);
/*
#ifndef NDEBUG
        std::ostringstream oss2;
        oss2 << "@currRdfId=" << currRdfId << ", ud=" << ud << " tries to use init skill by pos=(" << currChd.x() << ", " << currChd.y() << "), currVel=(" << currChd.vel_x() << ", " << currChd.vel_y() << "), patternId=" << patternId << ", effDx=" << effDx << ", effDy=" << effDy << ", encodedPattern=" << encodedPattern << ", targetSkillId=" << targetSkillId << " selected";
        Debug::Log(oss2.str(), DColor::Orange);
#endif // !NDEBUG
*/
    }

    if (!skillConfigs.count(targetSkillId)) {
#ifndef NDEBUG
         std::ostringstream oss;
         oss << "@currRdfId=" << currRdfId << ", ud=" << ud << ", skillConfigs size=" << skillConfigs.size() << " doesn't contain targetSkillId=" << targetSkillId;
         Debug::Log(oss.str(), DColor::Yellow);
#endif // !NDEBUG
        return false;
    }

    const Skill& targetSkillConfig = skillConfigs.at(targetSkillId);

    if (InAirDashing == targetSkillConfig.bound_ch_state()) {
        if (!currChd.omit_gravity() && 0 >= currChd.remaining_air_dash_quota()) {
            return false;
        }
    }

    if (targetSkillConfig.mp_delta() > currChd.mp()) {
        // notEnoughMp = true; // TODO
#ifndef NDEBUG
         std::ostringstream oss;
         oss << "@currRdfId=" << currRdfId << ", ud=" << ud << ", not enough mp to use targetSkillId=" << targetSkillId;
         Debug::Log(oss.str(), DColor::Yellow);
#endif // !NDEBUG
        return false;
    }
 
    outSkillId = targetSkillId;
    outSkill = &(targetSkillConfig);

    nextChd->set_mp(currChd.mp() - targetSkillConfig.mp_delta());
    if (0 >= nextChd->mp()) {
        nextChd->set_mp(0);
    }

    int nextActiveSkillHit = 1;
    const BulletConfig& pivotBulletConfig = targetSkillConfig.hits(nextActiveSkillHit - 1);
    outPivotBc = &pivotBulletConfig;

    nextChd->set_active_skill_id(targetSkillId);
    nextChd->set_frames_to_recover(targetSkillConfig.recovery_frames());

    const Bullet* referenceBullet = nullptr;
    const BulletConfig* referenceBulletConfig = nullptr;
    for (int i = 0; i < pivotBulletConfig.simultaneous_multi_hit_cnt() + 1; i++) {
        if (!addNewBulletToNextFrame(currRdfId, currChd, currChdFacing, cc, currParalyzed, currEffInAir, outSkill, nextActiveSkillHit, outSkillId, nextRdf, referenceBullet, referenceBulletConfig, ud, currChd.bullet_team_id())) {
            break;
        }
        nextChd->set_active_skill_hit(nextActiveSkillHit);
        nextActiveSkillHit++;
    }

    nextChd->set_ch_state(targetSkillConfig.bound_ch_state());
    if (currChd.ch_state() == targetSkillConfig.bound_ch_state() && walkingAtkSet.count(currChd.ch_state())) {
        nextChd->set_frames_in_ch_state(pivotBulletConfig.active_anim_looping_rdf_offset()); 
    } else {
        nextChd->set_frames_in_ch_state(0); // Must reset "frames_in_ch_state()" here to handle the extreme case where a same skill, e.g. "Atk1", is used right after the previous one ended
    }
    if (nextChd->frames_invinsible() < pivotBulletConfig.startup_invinsible_frames()) {
        nextChd->set_frames_invinsible(pivotBulletConfig.startup_invinsible_frames());
    }
/*
#ifndef NDEBUG
    std::ostringstream oss3;
    oss3 << "@currRdfId=" << currRdfId << ", ud=" << ud << " used targetSkillId=" << targetSkillId << " by [currVel=(" << currChd.vel_x() << "," << currChd.vel_y() << "), currChState=" << currChd.ch_state() << ", currFramesInChState=" << currChd.frames_in_ch_state() << ", currEffInAir=" << currEffInAir << "], [nextVel=(" << nextChd->vel_x() << ", " << nextChd->vel_y() << "), nextChState=" << nextChd->ch_state() << ", nextFramesInChState=" << nextChd->frames_in_ch_state() << ", nextPos=(" << nextChd->x() << ", " << nextChd->y() << ")], patternId=" << patternId << ", effDx=" << effDx << ", effDy=" << effDy;
    Debug::Log(oss3.str(), DColor::Orange);
#endif // !NDEBUG
*/
    return true;
}

void BaseBattle::useInventorySlot(const int currRdfId, int slotArrIdx, const CharacterDownsync& currChd, const CharacterConfig* cc, CharacterDownsync* nextChd, bool& outSlotUsed, bool& outDodgedInBlockStun) {
    outSlotUsed = false;
    bool intendToDodgeInBlockStun = false;
    outDodgedInBlockStun = false;

    /*
    auto targetSlotCurr = currChd.Inventory.Slots[slotArrIdx];
    auto targetSlotNext = nextChd->Inventory.Slots[slotArrIdx];
    if (globalPrimitiveConsts->pattern_inventory_slot_bc() == patternId) {
        // Handle full charge skill usage
        if (InventorySlotStockType.GaugedMagazineIv != targetSlotCurr.stock_type() || targetSlotCurr.Quota != targetSlotCurr.DefaultQuota) {
            return (false, globalPrimitiveConsts->no_skill(), false);
        }
        slotLockedSkillId = targetSlotCurr.FullChargeSkillId;

        if (globalPrimitiveConsts->no_skill() == slotLockedSkillId && TERMINATING_BUFF_SPECIES_ID == targetSlotCurr.FullChargeBuffspecies_id()) {
            return (false, globalPrimitiveConsts->no_skill(), false);
        }

        // [WARNING] Deliberately allowing full charge skills to be used in "notRecovered" cases
        targetSlotNext.Quota = 0;
        slotUsed = true;

        // [WARNING] Revert all debuffs
        AssignToDebuff(globalPrimitiveConsts->terminating_debuff_species_id(), 0, nextChd->DebuffList[0]);

        if (TERMINATING_BUFF_SPECIES_ID != targetSlotCurr.FullChargeBuffspecies_id()) {
            auto buffConfig = buffConfigs[targetSlotCurr.FullChargeBuffspecies_id()];
            ApplyBuffToCharacter(currRdfId, buffConfig, currChd, nextChd);
        }

        if (globalPrimitiveConsts->no_skill() != slotLockedSkillId) {
            auto (currSkillConfig, currBulletConfig) = FindBulletConfig(currChd.active_skill_id(), currChd.ActiveSkillHit);
            if (null == currSkillConfig || null == currBulletConfig) return (false, globalPrimitiveConsts->no_skill(), false);

            if (!currBulletConfig.cancellable_by_inventory_slot_c()) return (false, globalPrimitiveConsts->no_skill(), false);
            if (!(currBulletConfig.cancellable_st_frame() <= currChd.frames_in_ch_state() && currChd.frames_in_ch_state() < currBulletConfig.CancellableEdFrame)) return (false, globalPrimitiveConsts->no_skill(), false);
        }

        return (slotUsed, slotLockedSkillId, false);
    } else {
        slotLockedSkillId = intendToDodgeInBlockStun ? globalPrimitiveConsts->no_skill() : (currEffInAir ? targetSlotCurr.skill_id_air() : targetSlotCurr.skill_id());

        if (!intendToDodgeInBlockStun && globalPrimitiveConsts->no_skill() == slotLockedSkillId && TERMINATING_BUFF_SPECIES_ID == targetSlotCurr.Buffspecies_id()) {
            return (false, globalPrimitiveConsts->no_skill(), false);
        }

        bool notRecovered = (0 < currChd.frames_to_recover());
        if (notRecovered && !intendToDodgeInBlockStun) {
            auto (currSkillConfig, currBulletConfig) = FindBulletConfig(currChd.active_skill_id(), currChd.ActiveSkillHit);
            if (null == currSkillConfig || null == currBulletConfig) return (false, globalPrimitiveConsts->no_skill(), false);

            if (globalPrimitiveConsts->pattern_inventory_slot_c() == patternId && !currBulletConfig.cancellable_by_inventory_slot_c()) return (false, globalPrimitiveConsts->no_skill(), false);
            if (globalPrimitiveConsts->pattern_inventory_slot_d() == patternId && !currBulletConfig.cancellable_by_inventory_slot_d()) return (false, globalPrimitiveConsts->no_skill(), false);
            if (!(currBulletConfig.cancellable_st_frame() <= currChd.frames_in_ch_state() && currChd.frames_in_ch_state() < currBulletConfig.CancellableEdFrame)) return (false, globalPrimitiveConsts->no_skill(), false);
        }

        if (InventorySlotStockType.GaugedMagazineIv == targetSlotCurr.stock_type()) {
            if (0 < targetSlotCurr.Quota) {
                targetSlotNext.Quota = targetSlotCurr.Quota - 1;
                slotUsed = true;
                dodgedInBlockStun = intendToDodgeInBlockStun;
            }
        } else if (InventorySlotStockType.QuotaIv == targetSlotCurr.stock_type()) {
            if (0 < targetSlotCurr.Quota) {
                targetSlotNext.Quota = targetSlotCurr.Quota - 1;
                slotUsed = true;
                dodgedInBlockStun = intendToDodgeInBlockStun;
            }
        } else if (InventorySlotStockType.TimedIv == targetSlotCurr.stock_type()) {
            if (0 == targetSlotCurr.frames_to_recover()) {
                targetSlotNext.set_frames_to_recover(targetSlotCurr.default_frames_to_recover());
                slotUsed = true;
                dodgedInBlockStun = intendToDodgeInBlockStun;
            }
        } else if (InventorySlotStockType.TimedMagazineIv == targetSlotCurr.stock_type()) {
            if (0 < targetSlotCurr.Quota) {
                targetSlotNext.Quota = targetSlotCurr.Quota - 1;
                if (0 == targetSlotNext.Quota) {
                    targetSlotNext.set_frames_to_recover(targetSlotCurr.default_frames_to_recover());
                    //logger.LogInfo(String.Format("At currRdfId={0}, player joinIndex={1} starts reloading inventoryBtnB", currRdfId, currChd.JoinIndex));
                }
                slotUsed = true;
                dodgedInBlockStun = intendToDodgeInBlockStun;
            }
        }

        if (slotUsed && !intendToDodgeInBlockStun) {
            if (TERMINATING_BUFF_SPECIES_ID != targetSlotCurr.Buffspecies_id()) {
                auto buffConfig = buffConfigs[targetSlotCurr.Buffspecies_id()];
                ApplyBuffToCharacter(currRdfId, buffConfig, currChd, nextChd);
            }
        }

        return (slotUsed, slotLockedSkillId, dodgedInBlockStun);
    }
    */
}

CH_COLLIDER_T* BaseBattle::createDefaultCharacterCollider(const CharacterConfig* cc, const Vec3Arg& newPos, const QuatArg& newRot, const uint64_t newUd, BodyInterface* inBodyInterface) {
    CapsuleShape* chShapeCenterAnchor = new CapsuleShape(cc->capsule_half_height(), cc->capsule_radius()); // lifecycle to be held by "RefConst<Shape> RotatedTranslatedShape::mInnerShape", will be destructed upon "~RotatedTranslatedShape()"
    chShapeCenterAnchor->SetDensity(cDefaultChDensity);
    RotatedTranslatedShape* chShape = new RotatedTranslatedShape(Vec3(0, cc->capsule_half_height() + cc->capsule_radius(), 0), Quat::sIdentity(), chShapeCenterAnchor); // lifecycle to be held by "RefConst<Shape> CharacterBase::mShape", will be destructed upon "~CharacterBase()"

    Ref<CharacterSettings> settings = new CharacterSettings();
    settings->mMaxSlopeAngle = cMaxSlopeAngle;
    settings->mLayer = MyObjectLayers::MOVING;
    settings->mFriction = cDefaultChFriction;
    settings->mSupportingVolume = Plane(Vec3::sAxisY(), -cc->capsule_radius()); // Accept contacts that touch the lower sphere of the capsule
    settings->mEnhancedInternalEdgeRemoval = cEnhancedInternalEdgeRemoval;
    settings->mShape = chShape;
    settings->mMass = chShape->GetMassProperties().mMass;

    /* 
        [REMINDER] 
        
        The default "mLinearDamping" of "Character.mBodyID" is "0.05" and NOT overridable by the constructor "Character(...)" -- it doesn't bother me much because in "processInertiaWalking/processInertiaFlying" the velocity of a "Character" is constantly "re-pumped" to make "mLinearDamping = 0.05" negligible.
    */
    auto ret = new Character(settings, newPos, newRot, newUd, phySys);
	ret->AddToPhysicsSystem(EActivation::DontActivate, false);
    inBodyInterface->SetMotionQuality(ret->GetBodyID(), EMotionQuality::LinearCast);
    
    return ret;
}

BL_COLLIDER_T* BaseBattle::createDefaultBulletCollider(const float immediateBoxHalfSizeX, const float immediateBoxHalfSizeY, float& outConvexRadius, const EMotionType immediateMotionType, const bool isSensor, const Vec3Arg& newPos, const QuatArg& newRot, BodyInterface* inBodyInterface) {
    outConvexRadius = (immediateBoxHalfSizeX + immediateBoxHalfSizeY) * 0.5;
    if (cDefaultHalfThickness < outConvexRadius) {
        outConvexRadius = cDefaultHalfThickness; // Required by the underlying body creation 
    }
    Vec3 halfExtent(immediateBoxHalfSizeX, immediateBoxHalfSizeY, cDefaultHalfThickness);
    BoxShapeSettings* settings = new BoxShapeSettings(halfExtent, outConvexRadius); // transient, to be discarded after creating "body"
    settings->mDensity = cDefaultBlDensity;
    BodyCreationSettings bodyCreationSettings(settings, safeDeactiviatedPosition, JPH::Quat::sIdentity(), immediateMotionType, MyObjectLayers::MOVING);
    bodyCreationSettings.mAllowDynamicOrKinematic = true;
    bodyCreationSettings.mLinearDamping = 0; // [TODO] The default is "0.05".
    bodyCreationSettings.mGravityFactor = 0; // [TODO]
    bodyCreationSettings.mIsSensor = isSensor;
    bodyCreationSettings.mPosition = newPos;
    bodyCreationSettings.mRotation = newRot;
    Body* body = biNoLock->CreateBody(bodyCreationSettings);
    JPH_ASSERT(nullptr != body);

    inBodyInterface->SetMotionQuality(body->GetID(), EMotionQuality::Discrete);

    return body;
}

TP_COLLIDER_T* BaseBattle::createDefaultTrapCollider(const Vec3Arg& newHalfExtent, const Vec3Arg& newPos, const QuatArg& newRot, float& outConvexRadius, const EMotionType immediateMotionType, const bool isSensor, const ObjectLayer immediateObjectLayer, BodyInterface* inBodyInterface) {
    outConvexRadius = (newHalfExtent.GetX() + newHalfExtent.GetY()) * 0.5;
    if (cDefaultHalfThickness < outConvexRadius) {
        outConvexRadius = cDefaultHalfThickness; // Required by the underlying body creation 
    }
    BoxShapeSettings* settings = new BoxShapeSettings(newHalfExtent, outConvexRadius); // transient, to be discarded after creating "body"
    settings->mDensity = (EMotionType::Static == immediateMotionType ? 0 : cDefaultTpDensity);
    BodyCreationSettings bodyCreationSettings(settings, newPos, newRot, immediateMotionType, immediateObjectLayer);
    bodyCreationSettings.mAllowDynamicOrKinematic = EMotionType::Static == immediateMotionType ? false : true;
    bodyCreationSettings.mLinearDamping = 0; // [TODO] The default is "0.05".
    bodyCreationSettings.mGravityFactor = 0; // [TODO]
    bodyCreationSettings.mIsSensor = isSensor;
    bodyCreationSettings.mPosition = newPos;
    bodyCreationSettings.mRotation = newRot;
    Body* body = biNoLock->CreateBody(bodyCreationSettings);
    JPH_ASSERT(nullptr != body);

    inBodyInterface->SetMotionQuality(body->GetID(), EMotionQuality::Discrete);

    return body;
}

TR_COLLIDER_T* BaseBattle::createDefaultTriggerCollider(const float immediateBoxHalfSizeX, const float immediateBoxHalfSizeY, float& outConvexRadius, const Vec3Arg& newPos, const QuatArg& newRot, BodyInterface* inBodyInterface) {
    outConvexRadius = (immediateBoxHalfSizeX + immediateBoxHalfSizeY) * 0.5;
    if (cDefaultHalfThickness < outConvexRadius) {
        outConvexRadius = cDefaultHalfThickness; // Required by the underlying body creation 
    }
    Vec3 halfExtent(immediateBoxHalfSizeX, immediateBoxHalfSizeY, cDefaultHalfThickness);
    BoxShapeSettings* settings = new BoxShapeSettings(halfExtent, outConvexRadius); // transient, to be discarded after creating "body"
    settings->mDensity = FLT_MIN;
    BodyCreationSettings bodyCreationSettings(settings, safeDeactiviatedPosition, JPH::Quat::sIdentity(), EMotionType::Static, MyObjectLayers::NON_MOVING);
    bodyCreationSettings.mAllowDynamicOrKinematic = true;
    bodyCreationSettings.mLinearDamping = 0;
    bodyCreationSettings.mGravityFactor = 0; 
    bodyCreationSettings.mIsSensor = true;
    bodyCreationSettings.mPosition = newPos;
    bodyCreationSettings.mRotation = newRot;
    Body* body = biNoLock->CreateBody(bodyCreationSettings);
    JPH_ASSERT(nullptr != body);

    inBodyInterface->SetMotionQuality(body->GetID(), EMotionQuality::Discrete);

    return body;
}

NON_CONTACT_CONSTRAINT_T* BaseBattle::createDefaultNonContactConstraint(const EConstraintType nonContactConstraintType, const EConstraintSubType nonContactConstraintSubType, Body* inBody1, Body* inBody2, JPH::ConstraintSettings* inConstraintSettings) {
    JPH_ASSERT(nullptr != inBody1);
    JPH_ASSERT(nullptr != inBody2);
    switch (nonContactConstraintType) {
    case EConstraintType::TwoBodyConstraint: {
        switch (nonContactConstraintSubType) {
        case EConstraintSubType::Slider: {
            const uint64_t ud1 = inBody1->GetUserData();
            const uint64_t ud2 = inBody2->GetUserData();
            const uint64_t udt1 = getUDT(ud1);
            const SliderConstraintSettings* inSliderSettings = static_cast<SliderConstraintSettings*>(inConstraintSettings);  
            SliderConstraint* sliderConstraint = new SliderConstraint(*inBody1, *inBody2, *inSliderSettings);
            NON_CONTACT_CONSTRAINT_T* ret = new NON_CONTACT_CONSTRAINT_T(sliderConstraint, ud1, ud2);
            return ret;
        }
        default:
            break;
        }
        break;
    }
    default:
        break;
    }
    return nullptr;
}


void BaseBattle::preallocateBodies(const RenderFrame* currRdf, const google::protobuf::Map< uint32_t, uint32_t >& preallocateNpcSpeciesDict) {
    // Character starts
    Quat newRot = Quat::sIdentity();
    for (int i = 0; i < playersCnt; i++) {
        const PlayerCharacterDownsync& currPlayer = currRdf->players(i);
        const CharacterDownsync& currChd = currPlayer.chd();
        uint64_t ud = calcUserData(currPlayer);
#ifndef NDEBUG
        std::ostringstream oss2;
        oss2 << "[preallocateBodies] Player joinIndex=" << i+1 << " starts at position=(" << currChd.x() << ", " << currChd.y() << ", " << currChd.z() << "), species_id=" << currChd.species_id() << ", ud=" << ud; 
        Debug::Log(oss2.str(), DColor::Orange);
#endif
        const CharacterConfig* cc = getCc(currChd.species_id());
        calcChCacheKey(cc, chCacheKeyHolder);
        CH_COLLIDER_Q* targetQue = nullptr;
        auto it1 = cachedChColliders.find(chCacheKeyHolder);
        if (it1 == cachedChColliders.end()) {
            CH_COLLIDER_Q q = { };
            cachedChColliders.emplace(chCacheKeyHolder, q);
            it1 = cachedChColliders.find(chCacheKeyHolder);
        }
        targetQue = &(it1->second);

        auto chCollider = createDefaultCharacterCollider(cc, safeDeactiviatedPosition, newRot, ud, biNoLock);
        targetQue->push_back(chCollider);
    }
    
    for (auto it = preallocateNpcSpeciesDict.begin(); it != preallocateNpcSpeciesDict.end(); it++) {
        auto npcSpeciesId = it->first;
        auto npcSpeciesCnt = it->second;
        const CharacterConfig* cc = getCc(npcSpeciesId);
        calcChCacheKey(cc, chCacheKeyHolder);
        CH_COLLIDER_Q* targetQue = nullptr;
        auto it1 = cachedChColliders.find(chCacheKeyHolder);
        if (it1 == cachedChColliders.end()) {
            CH_COLLIDER_Q q = { };
            cachedChColliders.emplace(chCacheKeyHolder, q);
            it1 = cachedChColliders.find(chCacheKeyHolder);
        }
        targetQue = &(it1->second);

        for (int c = 0; c < npcSpeciesCnt; c++) {
            auto chCollider = createDefaultCharacterCollider(cc, safeDeactiviatedPosition, newRot, 0, biNoLock);
            targetQue->push_back(chCollider);
        }
    }
    // Character ends

    bodyIDsToAdd.clear();
    // Bullet starts
    calcBlCacheKey(cDefaultBlHalfLength, cDefaultBlHalfLength, blCacheKeyHolder);
    if (!cachedBlColliders.count(blCacheKeyHolder)) {
        BL_COLLIDER_Q q = { };
        cachedBlColliders.emplace(blCacheKeyHolder, q);
    }
    blStockCache = &cachedBlColliders[blCacheKeyHolder];
    JPH_ASSERT(nullptr != blStockCache);
    for (int i = 0; i < 8; i++) {
        float dummyNewConvexRadius = 0;
        auto preallocatedBlCollider = createDefaultBulletCollider(cDefaultBlHalfLength, cDefaultBlHalfLength, dummyNewConvexRadius, EMotionType::Kinematic, true, safeDeactiviatedPosition, newRot, biNoLock);
        blStockCache->push_back(preallocatedBlCollider);
        bodyIDsToAdd.push_back(preallocatedBlCollider->GetID());
    }
    // Bullet ends

    // Trap starts
    calcTpCacheKey(cDefaultTpHalfLength, cDefaultTpHalfLength, EMotionType::Dynamic, false, MyObjectLayers::MOVING, tpCacheKeyHolder);
    if (!cachedTpColliders.count(tpCacheKeyHolder)) {
        TP_COLLIDER_Q q = { };
        cachedTpColliders.emplace(tpCacheKeyHolder, q);
    }
    tpDynamicStockCache = &cachedTpColliders[tpCacheKeyHolder];
    JPH_ASSERT(nullptr != tpDynamicStockCache);

    calcTpCacheKey(cDefaultTpHalfLength, cDefaultTpHalfLength, EMotionType::Kinematic, false, MyObjectLayers::MOVING, tpCacheKeyHolder);
    if (!cachedTpColliders.count(tpCacheKeyHolder)) {
        TP_COLLIDER_Q q = { };
        cachedTpColliders.emplace(tpCacheKeyHolder, q);
    }
    tpKinematicStockCache = &cachedTpColliders[tpCacheKeyHolder];
    JPH_ASSERT(nullptr != tpKinematicStockCache);

    calcTpCacheKey(cDefaultTpHalfLength, cDefaultTpHalfLength, EMotionType::Static, true, MyObjectLayers::TRAP_HELPER, tpCacheKeyHolder);
    if (!cachedTpColliders.count(tpCacheKeyHolder)) {
        TP_COLLIDER_Q q = { };
        cachedTpColliders.emplace(tpCacheKeyHolder, q);
    }
    tpHelperStockCache = &cachedTpColliders[tpCacheKeyHolder];
    JPH_ASSERT(nullptr != tpHelperStockCache);

    calcTpCacheKey(cDefaultTpHalfLength, cDefaultTpHalfLength, EMotionType::Dynamic, true, MyObjectLayers::TRAP_OBSTACLE_INTERFACE, tpCacheKeyHolder);
    if (!cachedTpColliders.count(tpCacheKeyHolder)) {
        TP_COLLIDER_Q q = { };
        cachedTpColliders.emplace(tpCacheKeyHolder, q);
    }
    tpObsIfaceStockCache = &cachedTpColliders[tpCacheKeyHolder];
    JPH_ASSERT(nullptr != tpObsIfaceStockCache);
    // Trap ends

    // Trigger starts
    calcTrCacheKey(cDefaultBlHalfLength, cDefaultBlHalfLength, trCacheKeyHolder); // Just use "cDefaultBlHalfLength", there're not many types of triggers that need a collider
    if (!cachedTrColliders.count(trCacheKeyHolder)) {
        TR_COLLIDER_Q q = { };
        cachedTrColliders.emplace(trCacheKeyHolder, q);
    }
    trStockCache = &cachedTrColliders[trCacheKeyHolder];
    JPH_ASSERT(nullptr != blStockCache);
    for (int i = 0; i < 8; i++) {
        float dummyNewConvexRadius = 0;
        auto preallocatedTrCollider = createDefaultTriggerCollider(cDefaultBlHalfLength, cDefaultBlHalfLength, dummyNewConvexRadius, safeDeactiviatedPosition, newRot, biNoLock);
        trStockCache->push_back(preallocatedTrCollider);
        bodyIDsToAdd.push_back(preallocatedTrCollider->GetID());
    }
    // Trigger ends

    auto layerState = biNoLock->AddBodiesPrepare(bodyIDsToAdd.data(), bodyIDsToAdd.size());
    biNoLock->AddBodiesFinalize(bodyIDsToAdd.data(), bodyIDsToAdd.size(), layerState, EActivation::DontActivate);

    phySys->OptimizeBroadPhase();

    batchRemoveFromPhySysAndCache(currRdf->id(), currRdf);
}

void BaseBattle::calcChdShape(const CharacterState chState, const CharacterConfig* cc, float& outCapsuleRadius, float& outCapsuleHalfHeight) {
    switch (chState) {
    case LayDown1:
    case GetUp1:
        outCapsuleRadius = cc->lay_down_capsule_radius();
        outCapsuleHalfHeight = cc->lay_down_capsule_half_height();
        break;
    case Dying:
        outCapsuleRadius = cc->dying_capsule_radius();
        outCapsuleHalfHeight = cc->dying_capsule_half_height();
        break;
    case BlownUp1:
    case InAirIdle1NoJump:
    case InAirIdle1ByJump:
    case InAirIdle2ByJump:
    case InAirIdle1ByWallJump:
    case InAirWalking:
    case InAirAtk1:
    case InAirAtked1:
    case OnWallIdle1:
    case OnWallAtk1:
    case Sliding:
    case GroundDodged:
    case CrouchIdle1:
    case CrouchAtk1:
    case CrouchAtked1:
    case Dashing:
        outCapsuleRadius = cc->shrinked_capsule_radius();
        outCapsuleHalfHeight = cc->shrinked_capsule_half_height();
        break;
    case Dimmed:
        if (0 != cc->dimmed_capsule_radius() && 0 != cc->dimmed_capsule_half_height()) {
            outCapsuleRadius = cc->dimmed_capsule_radius();
            outCapsuleHalfHeight = cc->dimmed_capsule_half_height();
        } else {
            outCapsuleRadius = cc->capsule_radius();
            outCapsuleHalfHeight = cc->capsule_half_height();
        }
        break;
    default:
        outCapsuleRadius = cc->capsule_radius();
        outCapsuleHalfHeight = cc->capsule_half_height();
        break;
    }
}

void BaseBattle::stepSingleChdState(const int currRdfId, const RenderFrame* currRdf, RenderFrame* nextRdf, const float dt, const uint64_t ud, const uint64_t udt, const CharacterConfig* cc, CH_COLLIDER_T* single, const CharacterDownsync& currChd, CharacterDownsync* nextChd, bool& groundBodyIsChCollider, bool& isDead, bool& cvOnWall, bool& cvSupported, bool& cvInAir, bool& inJumpStartupOrJustEnded, CharacterBase::EGroundState& cvGroundState, InputInducedMotion* inputInducedMotion) {
    auto bodyID = single->GetBodyID();

    RVec3 newPos;
    Quat newRot;
    single->GetPositionAndRotation(newPos, newRot, false);
    auto newOverallVel = single->GetLinearVelocity(false);
    
    /*
    [TODO] For NPCs with more complicated shapes, use an extra collector with a compound shape and specified hurt-box to pick up damage.
    */

    Vec3 narrowPhaseInBaseOffset = newPos;
    /*
    For "narrowPhaseInBaseOffset", in most cases any value will work BUT it's recommended to choose ONLY among {Vec3::sZero(), centerOfMassTranslationOfBody1InWorldSpace}

    - using "narrowPhaseInBaseOffset = Vec3::sZero()" makes "CollideShapeResult.mContactPointOn[1|2]" in world space, and

    - using "narrowPhaseInBaseOffset = centerOfMassTranslationOfBody1InWorldSpace" makes "CollideShapeResult.mContactPointOn[1|2]" in "body1 local space"
    */
    CharacterCollideShapeCollector collector(currRdfId, nextRdf, biNoLock, ud, udt, &currChd, nextChd, single->GetUp(), narrowPhaseInBaseOffset, this); // Aggregates "CharacterBase.mGroundXxx" properties in a same "KernelThread"

    auto chCOMTransform = biNoLock->GetCenterOfMassTransform(bodyID);

    // Settings for collide shape
    CollideShapeSettings settings;
    settings.mMaxSeparationDistance = cCollisionTolerance;
    settings.mActiveEdgeMode = EActiveEdgeMode::CollideOnlyWithActive;
    settings.mActiveEdgeMovementDirection = newOverallVel;
    settings.mBackFaceMode = EBackFaceMode::IgnoreBackFaces;
    
    ChdPostPhysicsNarrowPhaseBodyFilter chBodyFilter(&currChd, nextChd, bodyID, ud, this);
    narrowPhaseQueryNoLock->CollideShape(single->GetShape(), Vec3::sOne(), chCOMTransform, settings, narrowPhaseInBaseOffset, collector, defaultBplf, defaultOlf, chBodyFilter);
    
    // Copy results
    single->SetGroundBodyID(collector.mGroundBodyID, collector.mGroundBodySubShapeID);
    single->SetGroundBodyPosition(collector.mGroundPosition, collector.mGroundNormal);

    inputInducedMotion->crouchForcedWhileSupported = (collector.mCrouchForced && cvSupported); 

    uint64_t newGroundUd = 0;
    Vec3 newGroundVel = Vec3::sZero();
    if (!collector.mGroundBodyID.IsInvalid()) {
        newGroundUd = biNoLock->GetUserData(collector.mGroundBodyID);
        newGroundVel = biNoLock->GetLinearVelocity(collector.mGroundBodyID); 
        // Update ground state
        RMat44 inv_transform = RMat44::sInverseRotationTranslation(newRot, newPos);
        if (single->GetSupportingVolume()->SignedDistance(Vec3(inv_transform * collector.mGroundPosition)) > 0.0f)
            single->SetGroundState(JPH::CharacterBase::EGroundState::NotSupported);
        else if (single->IsSlopeTooSteep(collector.mGroundNormal))
            single->SetGroundState(JPH::CharacterBase::EGroundState::OnSteepGround);
        else
            single->SetGroundState(JPH::CharacterBase::EGroundState::OnGround);

        // Copy other body properties
        single->SetGroundBodyUd(biNoLock->GetMaterial(collector.mGroundBodyID, collector.mGroundBodySubShapeID), newGroundVel, newGroundUd);
    } else {
        single->SetGroundState(JPH::CharacterBase::EGroundState::InAir);
        single->SetGroundBodyUd(JPH::PhysicsMaterial::sDefault, newGroundVel, newGroundUd);
    }

    inJumpStartupOrJustEnded = (isInJumpStartup(*nextChd, cc) || isJumpStartupJustEnded(currChd, nextChd, cc));
    cvGroundState = single->GetGroundState();

    auto groundBodyID = single->GetGroundBodyID();
    groundBodyIsChCollider = transientUdToChCollider.count(newGroundUd);
    JPH::Quat nextChdQ(nextChd->q_x(), nextChd->q_y(), nextChd->q_z(), nextChd->q_w());
    Vec3 nextChdFacing = nextChdQ*Vec3(1, 0, 0);
    float groundNormalAlignment = nextChdFacing.Dot(single->GetGroundNormal());
    cvOnWall = (0 > groundNormalAlignment && !groundBodyID.IsInvalid() && !groundBodyIsChCollider && !transientSlipJumpableUds.count(newGroundUd) && single->IsSlopeTooSteep(single->GetGroundNormal()));
    cvSupported = (single->IsSupported() && !cvOnWall && !groundBodyID.IsInvalid() && !groundBodyIsChCollider); // [WARNING] "cvOnWall" and  "cvSupported" are mutually exclusive in this game!
    /* [WARNING]
    When a "CapsuleShape" is colliding with a "MeshShape", some unexpected z-offset might be caused by triangular pieces. We have to compensate for such unexpected z-offsets by setting the z-components to 0.

    The process of unexpected z-offsets being introduced can be tracked in
    - [CollideConvexVsTriangles::Collide](https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Physics/Collision/CollideConvexVsTriangles.cpp#L137)
    - [PhysicsSystem::ProcessBodyPair::ReductionCollideShapeCollector::AddHit](https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Physics/PhysicsSystem.cpp#L1105)
    - [PhysicsSystem::ProcessBodyPair](https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Physics/PhysicsSystem.cpp#L1165)
    */
    if (cc->on_wall_enabled()) {
        if (cvOnWall) {
            /*
#ifndef NDEBUG
            std::ostringstream oss;
            oss << "@currRdfId=" << currRdfId << ", character ud=" << ud << ", bodyId=" << single->GetBodyID().GetIndexAndSequenceNumber() << ", currChdChState=" << currChd.ch_state() << ", currFramesInChState=" << currChd.frames_in_ch_state() << "; cvSupported=" << cvSupported << ", cvOnWall=" << cvOnWall << ", inJumpStartupOrJustEnded=" << inJumpStartupOrJustEnded << ", cvGroundState=" << CharacterBase::sToString(cvGroundState) << ", groundUd=" << single->GetGroundUserData() << ", groundBodyId=" << single->GetGroundBodyID().GetIndexAndSequenceNumber() << ", groundNormal=(" << single->GetGroundNormal().GetX() << ", " << single->GetGroundNormal().GetY() << ", " << single->GetGroundNormal().GetZ() << ")";
            Debug::Log(oss.str(), DColor::Orange);
#endif
            */
            processWallGrabbingPostPhysicsUpdate(currRdfId, currChd, nextChd, cc, single, inJumpStartupOrJustEnded);
        } else if (cvSupported && OnWallIdle1 == currChd.ch_state()) {
            nextChd->set_ch_state(InAirIdle1NoJump);
        }
    }

    clampChdVel(nextChd, newOverallVel, cc, newGroundVel);
    nextChd->set_x(newPos.GetX());
    nextChd->set_y(newPos.GetY());
    nextChd->set_z(0);
    nextChd->set_vel_x(IsLengthNearZero(newOverallVel.GetX() * dt) ? 0 : newOverallVel.GetX());
    nextChd->set_vel_y(IsLengthNearZero(newOverallVel.GetY() * dt) ? 0 : newOverallVel.GetY());
    nextChd->set_vel_z(0);
    // [WARNING] Intentionally NOT setting "nextChd->q_*" here by "newRot", but in "processSingleCharacterInput" instead.
    if (cvSupported) {
        nextChd->set_ground_vel_x(newGroundVel.GetX());
        nextChd->set_ground_vel_y(newGroundVel.GetY());
        nextChd->set_ground_vel_z(0);
        nextChd->set_ground_ud(newGroundUd);
    } else if (groundBodyIsChCollider) {
        nextChd->set_ground_vel_x(0);
        nextChd->set_ground_vel_y(0);
        nextChd->set_ground_vel_z(0);
        nextChd->set_ground_ud(newGroundUd);
    } else {
        nextChd->set_ground_vel_x(0);
        nextChd->set_ground_vel_y(0);
        nextChd->set_ground_vel_z(0);
        nextChd->set_ground_ud(0);
    }

    cvInAir = (!cvSupported || cvOnWall);
    bool hasBeenOnWallIdle1 = (OnWallIdle1 == nextChd->ch_state() && OnWallIdle1 == currChd.ch_state());
    bool hasBeenOnWallAtk1 = (OnWallAtk1 == nextChd->ch_state() && OnWallAtk1 == currChd.ch_state());
    if ((hasBeenOnWallIdle1 || hasBeenOnWallAtk1) && nextChd->x() != currChd.x()) {
        nextChd->set_x(currChd.x()); // [WARNING] compensation for this known caveat of Jolt with horizontal-position change while GroundNormal is kept unchanged 
    }

    uint32_t                newEffDebuffSpeciesId = globalPrimitiveConsts->terminating_debuff_species_id();
    int                     newEffDamage = 0;
    bool                    newEffBlownUp = false;
    int                     newEffFramesToRecover = 0;
    int                     newEffDef1QuotaReduction = 0;
    float                   newEffPushbackVelX = globalPrimitiveConsts->no_lock_vel();
    float                   newEffPushbackVelY = globalPrimitiveConsts->no_lock_vel();

    if (transientUdToCollisionUdHolder.count(ud)) {
        CollisionUdHolder_ThreadSafe* holder = transientUdToCollisionUdHolder.at(ud);
        int cntNow = holder->GetCnt_Realtime();
        uint64_t udRhs;
        ContactPoints contactPointsLhs;
        int holderCnt = holder->GetCnt_Realtime();
        for (int j = 0; j < holderCnt; ++j) {
            bool fetched = holder->GetUd_NotThreadSafe(j, udRhs, contactPointsLhs);
            if (!fetched) continue;
            uint64_t udtRhs = getUDT(udRhs);
            if (UDT_BL == udtRhs) {
                handleLhsCharacterCollisionWithRhsBullet(currRdfId, nextRdf, ud, udt, &currChd, nextChd,
                    udRhs, udtRhs, contactPointsLhs,
                    newEffDebuffSpeciesId, newEffDamage, newEffBlownUp, newEffFramesToRecover, newEffDef1QuotaReduction, newEffPushbackVelX, newEffPushbackVelY);
            } else if (UDT_TRIGGER == udtRhs) {
                const Trigger* currTrigger = transientUdToCurrTrigger.at(udRhs);
                Trigger* nextTrigger = transientUdToNextTrigger.at(udRhs);
                if (collidableTrtSet.count(currTrigger->trt()) && TriggerState::TrReady == currTrigger->state() && TriggerState::TrReady == nextTrigger->state() && 0 < currTrigger->quota()) {
                    nextTrigger->set_main_cycle_mask_to_fulfill(0);
#ifndef NDEBUG
                    std::ostringstream oss;
                    oss << "@currRdfId=" << currRdfId << ", character ud=" << ud << ", bodyId=" << single->GetBodyID().GetIndexAndSequenceNumber() << " pre-fulfilled trigger id=" << currTrigger->id() << std::endl;
                    Debug::Log(oss.str(), DColor::Orange);
#endif
                }
            }
        }
    }

    if (0 < newEffDamage) {
        if (newEffBlownUp) {
            nextChd->set_ch_state(BlownUp1);
        } else {
            if (cvOnWall || cvInAir || inAirSet.count(currChd.ch_state()) || inAirSet.count(nextChd->ch_state())) {
                nextChd->set_ch_state(InAirAtked1);
            } else {
                nextChd->set_ch_state(Atked1);
            }
        }
        nextChd->set_hp(nextChd->hp() - newEffDamage);
        nextChd->set_damaged_hint_rdf_countdown(90); // TODO: Remove this hardcoded constant
    }  

    if (0 < newEffFramesToRecover) {
        if (atkedSet.count(currChd.ch_state())) {
            // If transiting from an "atked ch_state", we'd only extend the existing "frames_to_recover"
            if (nextChd->frames_to_recover() < newEffFramesToRecover) {
                nextChd->set_frames_to_recover(newEffFramesToRecover);
            }
        } else {
            nextChd->set_frames_to_recover(newEffFramesToRecover);
        }
    }

    if (globalPrimitiveConsts->no_lock_vel() != newEffPushbackVelX) {
        nextChd->set_vel_x(newEffPushbackVelX);
    } 

    if (globalPrimitiveConsts->no_lock_vel() != newEffPushbackVelY) {
        nextChd->set_vel_y(newEffPushbackVelY);
    }

    isDead = (0 >= nextChd->hp());
}

void BaseBattle::stepOtherSingleTriggerState(const int currRdfId, const Trigger& currTrigger, Trigger* nextTrigger, RenderFrame* nextRdf, StepResult* stepResult) {
    bool mainCycleFulfilled = (trActiveMainCycleStates.count(currTrigger.state()) && 0 == currTrigger.main_cycle_mask_to_fulfill());
    bool subCycleFulfilled = (TriggerState::TrSubCycleReady == currTrigger.state() && 0 == currTrigger.sub_cycle_mask_to_fulfill() && 0 >= currTrigger.frames_to_recover());
    bool mainCycleExhausted = false;

    if (mainCycleFulfilled) {
        int newQuota = currTrigger.quota() - 1;
        if (0 > newQuota) {
            newQuota = 0;
        }

        int newFramesToRecover = globalPrimitiveConsts->default_tr_recovery_frames();
        if (triggerConfigFromTileDict.count(currTrigger.id())) {
            auto* triggerConfigFromTiled = triggerConfigFromTileDict.at(currTrigger.id());
            newFramesToRecover = triggerConfigFromTiled->recovery_frames();
        }

        nextTrigger->set_quota(newQuota);
        if (0 < currTrigger.sub_cycle_mask_to_fulfill() && !mixedMainAndSubCycleTrtSet.count(currTrigger.trt())) {
            // [WARNING] In this case, "mainCycleExhausted" should be triggered by "subCycleFulfilled" instead
            nextTrigger->set_state(TriggerState::TrSubCycleReady);
            nextTrigger->set_frames_to_recover(newFramesToRecover);
        } else {
            if (0 >= newQuota) {
                mainCycleExhausted = true;
                nextTrigger->set_state(TriggerState::TrExhausted);
            } else {
                nextTrigger->set_state(TriggerState::TrCoolingDown);
                nextTrigger->set_frames_to_recover(newFramesToRecover);
            }
        }

        nextTrigger->set_frames_in_state(0);

        auto fulfilledTrigger = stepResult->add_fulfilled_triggers();
        CopyTrigger(&currTrigger, fulfilledTrigger);
        auto mutableFulfilledTriggerIds = stepResult->mutable_fulfilled_trigger_ids();
        mutableFulfilledTriggerIds->insert({currTrigger.id(), true});

#ifndef NDEBUG
        std::ostringstream oss;
        oss << "@currRdfId=" << currRdfId << ", trigger id=" << currTrigger.id() << " added to fulfilled set of this rdf by main-cycle mask matching" << std::endl;
        Debug::Log(oss.str(), DColor::Orange);
#endif
    } else if (subCycleFulfilled) {
        if (0 >= currTrigger.quota()) {
            mainCycleExhausted = true;
            nextTrigger->set_state(TriggerState::TrExhausted);
        } else {
            nextTrigger->set_state(TriggerState::TrReady);
        }
        nextTrigger->set_frames_in_state(0);
    }

    if (mainCycleExhausted) {
        if (triggerConfigFromTileDict.count(currTrigger.id())) {
            auto* triggerConfigFromTiled = triggerConfigFromTileDict.at(currTrigger.id());
            if (globalPrimitiveConsts->terminating_trigger_id() != triggerConfigFromTiled->publishing_to_trigger_id_upon_exhausted()) {
                uint64_t nextReceivingTriggerUd = calcTriggerUserData(triggerConfigFromTiled->publishing_to_trigger_id_upon_exhausted());
                Trigger* nextReceivingTrigger = transientUdToNextTrigger.at(nextReceivingTriggerUd);
                publishToTrigger(currRdfId, triggerConfigFromTiled->publishing_mask_upon_exhausted(), currTrigger.offender_ud(), currTrigger.offender_bullet_team_id(), nextReceivingTrigger);

#ifndef NDEBUG
                std::ostringstream oss;
                oss << "@currRdfId=" << currRdfId << ", trigger id=" << currTrigger.id() << " main-cycle exhausted, publishing_to_trigger_id_upon_exhausted=" << triggerConfigFromTiled->publishing_to_trigger_id_upon_exhausted();
                Debug::Log(oss.str(), DColor::Orange);
#endif
            }
        }
    }
}

void BaseBattle::stepSingleIndiWaveNpcSpawner(const int currRdfId, const Trigger& currTrigger, Trigger* nextTrigger, RenderFrame* nextRdf, StepResult* stepResult) {
    JPH_ASSERT(triggerConfigFromTileDict.count(currTrigger.id())); 
    auto* triggerConfigFromTiled = triggerConfigFromTileDict.at(currTrigger.id());
    
    bool mainCycleFulfilled = (TriggerState::TrReady == currTrigger.state()  && 0 == currTrigger.main_cycle_mask_to_fulfill());
    bool mainCycleExhaustedYetFulfilled = (TriggerState::TrExhaustedYetListening == currTrigger.state() && 0 == currTrigger.main_cycle_mask_to_fulfill());
    bool subCycleFulfilled = (trSubCycleStates.count(currTrigger.state()) && 0 == currTrigger.sub_cycle_mask_to_fulfill());

    bool subCycleCooledDown = (TriggerState::TrSubCycleCoolingDown == currTrigger.state() && 1 >= currTrigger.frames_to_recover());

    int oldQuota = currTrigger.quota();

    bool subCycleTicked = (trActiveSubCycleStates.count(currTrigger.state()) && 0 >= currTrigger.frames_to_fire());
    int chSpawnerConfigIdx = triggerConfigFromTiled->quota() - oldQuota;
    int oldSubCycleIdx = currTrigger.sub_cycle_index();
    
    if (subCycleFulfilled) {
        // Return to listen to main-cycle signal 
        if (0 < oldQuota) {
            nextTrigger->set_state(TriggerState::TrReady);
        } else {
            nextTrigger->set_state(TriggerState::TrExhaustedYetListening);
        }

        nextTrigger->set_main_cycle_mask_to_fulfill(1);

        if (globalPrimitiveConsts->terminating_trigger_id() != triggerConfigFromTiled->publishing_to_trigger_id_upon_exhausted()) {
            // [WARNING] For "trt_indi_wave_npc_spawner", we should report to "publishing_to_trigger_id_upon_exhausted" upon "sub-cycle fulfilled" instead of "main-cycle exhausted".
            uint64_t nextReceivingTriggerUd = calcTriggerUserData(triggerConfigFromTiled->publishing_to_trigger_id_upon_exhausted());
            Trigger* nextReceivingTrigger = transientUdToNextTrigger.at(nextReceivingTriggerUd);
            publishToTrigger(currRdfId, triggerConfigFromTiled->publishing_mask_upon_exhausted(), currTrigger.offender_ud(), currTrigger.offender_bullet_team_id(), nextReceivingTrigger);

#ifndef NDEBUG
            std::ostringstream oss;
            oss << "@currRdfId=" << currRdfId << ", trigger id=" << currTrigger.id() << " sub-cycle fulfilled with oldQuota=" << oldQuota << ", oldSubCycleIdx=" << oldSubCycleIdx << ", publishing_to_trigger_id_upon_exhausted=" << triggerConfigFromTiled->publishing_to_trigger_id_upon_exhausted() << ", next trigger-state=" << nextTrigger->state() << std::endl;
            Debug::Log(oss.str(), DColor::Orange);
#endif
        } else {
#ifndef NDEBUG
            std::ostringstream oss;
            oss << "@currRdfId=" << currRdfId << ", trigger id=" << currTrigger.id() << " sub-cycle fulfilled with oldQuota=" << oldQuota << ", oldSubCycleIdx=" << oldSubCycleIdx << ", no target to publish, next trigger-state=" << nextTrigger->state() << std::endl;
            Debug::Log(oss.str(), DColor::Orange);
#endif
        }

        nextTrigger->set_frames_in_state(0);
    } else if (subCycleTicked) {
#ifndef NDEBUG
        std::ostringstream oss;
        oss << "@currRdfId=" << currRdfId << ", trigger id=" << currTrigger.id() << " sub-cycle ticking at oldSubCycleIdx=" << oldSubCycleIdx << ", chSpawnerConfigIdx=" << chSpawnerConfigIdx << ", oldQuota=" << oldQuota << ", curr trigger_state=" << (int)currTrigger.state() << std::endl;
        Debug::Log(oss.str(), DColor::Orange);
#endif
        int newSubCycleIdx = oldSubCycleIdx + 1;
        nextTrigger->set_sub_cycle_index(newSubCycleIdx);
        nextTrigger->set_frames_in_state(0);
        auto chSpawnerConfig = lowerBoundForSpawnerConfig(chSpawnerConfigIdx, triggerConfigFromTiled->character_spawner_time_seq());
        nextTrigger->set_frames_to_recover(triggerConfigFromTiled->recovery_frames());

        uint32_t chSpeciesId = chSpawnerConfig->species_id_list(oldSubCycleIdx);
        uint64_t initOp = (oldSubCycleIdx < chSpawnerConfig->init_op_list_size() ? chSpawnerConfig->init_op_list(oldSubCycleIdx) : 0);
        float qx = 0, qy = 0, qz = 0, qw = 1;
        if (0 != initOp) {
            InputFrameDecoded ifDecodedHolder;
            decodeInput(initOp, &ifDecodedHolder);
            if (0 > ifDecodedHolder.dx()) {
                qx = 0;
                qy = 1;
                qz = 0;
                qw = 0;
            }
        } else if (0 != triggerConfigFromTiled->init_q_x() || 0 != triggerConfigFromTiled->init_q_y()  || 0 != triggerConfigFromTiled->init_q_z() || 0 != triggerConfigFromTiled->init_q_w()) {
            qx = triggerConfigFromTiled->init_q_x();
            qy = triggerConfigFromTiled->init_q_y();
            qz = triggerConfigFromTiled->init_q_z();
            qw = triggerConfigFromTiled->init_q_w();
        }
       
        addNewNpcToNextFrame(currRdfId, currTrigger.x(), currTrigger.y(), qx, qy, qz, qw, chSpeciesId, currTrigger.bullet_team_id(), NpcGoal::NPatrol /* TODO */, nextRdf, currTrigger.id(), currTrigger.sub_cycle_gen_mask_counter(), globalPrimitiveConsts->terminating_trigger_id());

        if (newSubCycleIdx < chSpawnerConfig->species_id_list_size()) {
            nextTrigger->set_state(TriggerState::TrSubCycleCoolingDown);
            nextTrigger->set_frames_to_fire(triggerConfigFromTiled->sub_cycle_trigger_frames());
            nextTrigger->set_sub_cycle_gen_mask_counter((currTrigger.sub_cycle_gen_mask_counter() << 1));
        } else {
            nextTrigger->set_state(TriggerState::TrSubCycleExhausted);
            nextTrigger->set_frames_to_fire(0);
            nextTrigger->set_sub_cycle_gen_mask_counter(0);
        }

#ifndef NDEBUG
        std::ostringstream oss2;
        oss2 << "@currRdfId=" << currRdfId << ", trigger id=" << currTrigger.id() << " sub-cycle ticked to add new npc oldSubCycleIdx=" << oldSubCycleIdx << ", chSpeciesId=" << chSpeciesId << ", curr gen_mask_counter=" << currTrigger.sub_cycle_gen_mask_counter() << ", mNextRdfNpcCount=" << mNextRdfNpcCount.load() << ", oldQuota=" << oldQuota << ", next main_cycle_mask_to_fulfill=" << nextTrigger->main_cycle_mask_to_fulfill() << ", next sub_cycle_mask_to_fulfill=" << nextTrigger->sub_cycle_mask_to_fulfill() << ", current sub_cycle_quota=" << chSpawnerConfig->species_id_list_size() << ", next trigger_state=" << (int)nextTrigger->state() << std::endl;
        Debug::Log(oss2.str(), DColor::Orange);
#endif
    } else if (mainCycleExhaustedYetFulfilled) {
#ifndef NDEBUG
        std::ostringstream oss;
        oss << "@currRdfId=" << currRdfId << ", trigger id=" << currTrigger.id() << " main-cycle exhausted yet fulfilled, oldQuota=" << oldQuota;
        Debug::Log(oss.str(), DColor::Orange);
#endif

        Trigger* fulfilledTrigger = stepResult->add_fulfilled_triggers();
        CopyTrigger(&currTrigger, fulfilledTrigger); 
        auto mutableFulfilledTriggerIds = stepResult->mutable_fulfilled_trigger_ids();
        mutableFulfilledTriggerIds->insert({currTrigger.id(), true});
    } else if (mainCycleFulfilled) {
        int newQuota = oldQuota - 1;
        if (0 > newQuota) {
            newQuota = 0;
        }
        nextTrigger->set_quota(newQuota);
        nextTrigger->set_state(TriggerState::TrSubCycleReady);
        nextTrigger->set_frames_in_state(0);
        nextTrigger->set_frames_to_fire(triggerConfigFromTiled->delayed_frames());
        auto chSpawnerConfig = lowerBoundForSpawnerConfig(chSpawnerConfigIdx, triggerConfigFromTiled->character_spawner_time_seq());
        nextTrigger->set_sub_cycle_index(0);
        nextTrigger->set_sub_cycle_gen_mask_counter(1); // important initializer
        nextTrigger->set_sub_cycle_mask_to_fulfill((1UL << chSpawnerConfig->species_id_list_size()) - 1);

#ifndef NDEBUG
        std::ostringstream oss;
        oss << "@currRdfId=" << currRdfId << ", trigger id=" << currTrigger.id() << " main-cycle fulfilled chSpawnerConfigIdx=" << chSpawnerConfigIdx << ", frames_to_fire=" << triggerConfigFromTiled->delayed_frames() << ", nextWaveNpcCnt=" << chSpawnerConfig->species_id_list_size() << ", oldQuota=" << oldQuota << ", next main_cycle_mask_to_fulfill=" << nextTrigger->main_cycle_mask_to_fulfill() << ", next sub_cycle_mask_to_fulfill=" << nextTrigger->sub_cycle_mask_to_fulfill() << std::endl;
        Debug::Log(oss.str(), DColor::Orange);
#endif

        Trigger* fulfilledTrigger = stepResult->add_fulfilled_triggers();
        CopyTrigger(&currTrigger, fulfilledTrigger); 
        auto mutableFulfilledTriggerIds = stepResult->mutable_fulfilled_trigger_ids();
        mutableFulfilledTriggerIds->insert({currTrigger.id(), true});
    } else if (subCycleCooledDown) {
        nextTrigger->set_state(TriggerState::TrSubCycleReady);
        nextTrigger->set_frames_in_state(0);
    }
}

void BaseBattle::handleLhsCharacterCollisionWithRhsBullet(
    const int currRdfId, 
    RenderFrame* nextRdf,
    const uint64_t udLhs, const uint64_t udtLhs, const CharacterDownsync* currChd, CharacterDownsync* nextChd,
    const uint64_t udRhs, const uint64_t udtRhs, 
    const ContactPoints& contactPointsLhs,
    uint32_t& outNewEffDebuffSpeciesId, int& outNewDamage, bool& outNewEffBlownUp, int& outNewEffFramesToRecover, int& outEffDef1QuotaReduction, float& outNewEffPushbackVelX, float& outNewEffPushbackVelY) {

    if (!transientUdToCurrBl.count(udRhs)) {
#ifndef NDEBUG
        std::ostringstream oss;
        auto bulletId = getUDPayload(udRhs);
        oss << "handleLhsCharacterCollision/nextBl with currRdfId=" << currRdfId << ", id=" << bulletId << " is NOT FOUND" << std::endl;
        Debug::Log(oss.str(), DColor::Orange);
#endif
        return;
    }

    if (0 < currChd->frames_invinsible()) {
        return;
    }
    const Bullet* rhsCurrBl = transientUdToCurrBl.at(udRhs);
    if (BulletState::Active != rhsCurrBl->bl_state()) {
        return;
    }
    const Skill* rhsSkill = nullptr;
    const BulletConfig* rhsBlConfig = nullptr;
    FindBulletConfig(rhsCurrBl->skill_id(), rhsCurrBl->active_skill_hit(), rhsSkill, rhsBlConfig);

    bool successfulDef1 = false; // TODO
    if (rhsBlConfig->remains_upon_hit()) {
        int immuneRcdI = 0;
        bool shouldBeImmune = false;
        while (immuneRcdI < nextChd->bullet_immune_records_size()) {
            auto& candidate = nextChd->bullet_immune_records(immuneRcdI);
            if (globalPrimitiveConsts->terminating_bullet_id() == candidate.bullet_id()) break;
            if (candidate.bullet_id() == rhsCurrBl->id()) {
                shouldBeImmune = true;
                return;
            }
            immuneRcdI++;
        }

        if (immuneRcdI >= globalPrimitiveConsts->default_per_character_immune_bullet_record_capacity()) {
            // [REMINDER] A special case to prevent a character to take too much damage simultaneously.
            shouldBeImmune = true;
        }

        if (shouldBeImmune) {
            return;
        }
            
        if (!(successfulDef1 && rhsBlConfig->takes_def1_as_hard_pushback())) {
            int nextImmuneRcdI = immuneRcdI;
            int terminatingImmuneRcdI = nextImmuneRcdI + 1;
            if (nextImmuneRcdI == nextChd->bullet_immune_records_size()) {
                nextImmuneRcdI = 0;
                terminatingImmuneRcdI = nextChd->bullet_immune_records_size(); // [WARNING] DON'T update termination in this case! 
            }
            auto nextImmuneRcd = nextImmuneRcdI < nextChd->bullet_immune_records_size() ? nextChd->mutable_bullet_immune_records(nextImmuneRcdI) : nextChd->add_bullet_immune_records();
            nextImmuneRcd->set_bullet_id(rhsCurrBl->id());
            int effImmuneRdfCnt = 2 + std::min(rhsBlConfig->hit_stun_frames(), rhsBlConfig->active_frames());
            nextImmuneRcd->set_remaining_lifetime_rdf_count(effImmuneRdfCnt);

            if (terminatingImmuneRcdI < nextChd->bullet_immune_records_size()) {
                auto terminatingImmuneRcd = nextChd->mutable_bullet_immune_records(terminatingImmuneRcdI);
                terminatingImmuneRcd->set_bullet_id(globalPrimitiveConsts->terminating_bullet_id());
                terminatingImmuneRcd->set_remaining_lifetime_rdf_count(0);
            }
        }
    }

    const CharacterConfig* cc = getCc(currChd->species_id());
    if (!(successfulDef1 && 0 >= cc->def1_damage_yield())) {
        int effDamage = rhsBlConfig->damage();
        if (successfulDef1) {
            effDamage = (int)std::ceil(cc->def1_damage_yield()*rhsBlConfig->damage());
        }
#ifndef NDEBUG
        if (UDT_PLAYER == udtLhs) {
            std::ostringstream oss;
            auto bulletId = rhsCurrBl->id();
            oss << "handleLhsCharacterCollision/nextBl with currRdfId=" << currRdfId << ", bulletId=" << bulletId << " hits Player ud=" << udLhs << " with effDamage=" << effDamage << std::endl;
            Debug::Log(oss.str(), DColor::Orange);
        } else if (UDT_NPC == udtLhs) {
            std::ostringstream oss;
            auto bulletId = rhsCurrBl->id();
            oss << "handleLhsCharacterCollision/nextBl with currRdfId=" << currRdfId << ", bulletId=" << bulletId << " hits Npc ud=" << udLhs << " with effDamage=" << effDamage << std::endl;
            Debug::Log(oss.str(), DColor::Orange);
        }
#endif
        outNewDamage += effDamage; 
        if (rhsBlConfig->blow_up()) {
            outNewEffBlownUp = true;
        }

        if (0 < effDamage) {
            // [REMINDER] Randomness comes from ordering of "transientUdToCollisionUdHolder".
            nextChd->set_last_damaged_by_bullet_team_id(rhsCurrBl->team_id());
            nextChd->set_last_damaged_by_ud(rhsCurrBl->offender_ud());
        }

        if (rhsBlConfig->remains_upon_hit()) {
            Vec3 hitPos(currChd->x(), currChd->y(), currChd->z()); 
            Vec3 hitPosAdds(0, 0, 0); 
            int hitPosAddsCnt = 0; 
            for (int k = 0; k < contactPointsLhs.size(); ++k) {
                auto& contactPoint = contactPointsLhs.at(k);
                if (0 > contactPoint.GetY()) {
                    // [REMINDER] Y-coordinate "currChd->y()" is the bottom of the character, therefore any y-offset less than 0 should be avoided.
                    continue;
                }
                hitPosAdds += contactPoint;
                hitPosAddsCnt += 1;
            }
            if (0 < hitPosAddsCnt) {
                hitPos += (hitPosAdds/hitPosAddsCnt); 
            }
            addBlHitToNextFrame(currRdfId, nextRdf, rhsCurrBl, hitPos);
        }
    }

    JPH::Quat blQ(rhsCurrBl->q_x(), rhsCurrBl->q_y(), rhsCurrBl->q_z(), rhsCurrBl->q_w());
    Vec3Arg blInitPushbackVelocity(rhsBlConfig->pushback_vel_x(), rhsBlConfig->pushback_vel_y(), 0);
    auto blEffPushbackVelocity = blQ*blInitPushbackVelocity;
    if (globalPrimitiveConsts->no_lock_vel() != rhsBlConfig->pushback_vel_x()) {
        if (globalPrimitiveConsts->no_lock_vel() == outNewEffPushbackVelX || std::abs(blEffPushbackVelocity.GetX()) > std::abs(outNewEffPushbackVelX)) {
            outNewEffPushbackVelX = blEffPushbackVelocity.GetX();
        }
    }
    if (globalPrimitiveConsts->no_lock_vel() != rhsBlConfig->pushback_vel_y()) {
        if (globalPrimitiveConsts->no_lock_vel() == outNewEffPushbackVelY || std::abs(blEffPushbackVelocity.GetY()) > std::abs(outNewEffPushbackVelY)) {
            outNewEffPushbackVelY = blEffPushbackVelocity.GetY();
        }
    }

    if (!successfulDef1) {
        if (rhsBlConfig->hit_stun_frames() > outNewEffFramesToRecover) {
            outNewEffFramesToRecover = rhsBlConfig->hit_stun_frames(); 
        }
    } else {
        if (rhsBlConfig->block_stun_frames() > outNewEffFramesToRecover) {
            outNewEffFramesToRecover = rhsBlConfig->block_stun_frames(); 
        }
    }
}

bool BaseBattle::deallocPhySys() {
    if (nullptr == phySys) return false;
    delete phySys;
    phySys = nullptr;
    bi = nullptr;
    biNoLock = nullptr;
    narrowPhaseQueryNoLock = nullptr;

    /*
    [WARNING] Unlike "std::vector" and "std::unordered_map", the customized containers "JPH::StaticArray" and "JPH::UnorderedMap" will deallocate their pointer-typed elements in their destructors.
    - https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Core/Array.h#L337 -> https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Core/Array.h#L249
    - https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Core/HashTable.h#L480 -> https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Core/HashTable.h#L497

    However I only use "dynamicBodyIDs" to hold "BodyID" instances which are NOT pointer-typed and effectively freed once "JPH::StaticArray::clear()" is called, i.e. no need for the heavy duty "JPH::Array".
    */
    return true;
}
