#include "BaseBattle.h"
#include "PbConsts.h"
#include "CppOnlyConsts.h"
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexShape.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>

#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

// STL includes
#include <cstdarg>
#include <thread>
#include <string>

#ifndef NDEBUG
#include "DebugLog.h"
#endif

using namespace jtshared;

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

BaseBattle::BaseBattle(int renderBufferSize, int inputBufferSize, TempAllocator* inGlobalTempAllocator) : rdfBuffer(renderBufferSize), ifdBuffer(inputBufferSize), frameLogBuffer(renderBufferSize << 1), globalTempAllocator(inGlobalTempAllocator) {
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
    jobSys = nullptr;
    blStockCache = nullptr;
}

BaseBattle::~BaseBattle() {
    Clear();
    delete jobSys;
    jobSys = nullptr;
    deallocPhySys();
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

CH_COLLIDER_T* BaseBattle::getOrCreateCachedPlayerCollider(const uint64_t ud, const PlayerCharacterDownsync& currPlayer, const CharacterConfig* cc, PlayerCharacterDownsync* nextPlayer) {
    JPH_ASSERT(0 != ud);
    float capsuleRadius = 0, capsuleHalfHeight = 0;
    calcChdShape(currPlayer.chd().ch_state(), cc, capsuleRadius, capsuleHalfHeight);
    auto res = getOrCreateCachedCharacterCollider(ud, cc, capsuleRadius, capsuleHalfHeight);
    transientUdToCurrPlayer[ud] = &currPlayer;
    if (nullptr != nextPlayer) {
        transientUdToNextPlayer[ud] = nextPlayer;
    }
    return res;
}

CH_COLLIDER_T* BaseBattle::getOrCreateCachedNpcCollider(const uint64_t ud, const NpcCharacterDownsync& currNpc, const CharacterConfig* cc, NpcCharacterDownsync* nextNpc) {
    JPH_ASSERT(0 != ud);
    float capsuleRadius = 0, capsuleHalfHeight = 0;
    calcChdShape(currNpc.chd().ch_state(), cc, capsuleRadius, capsuleHalfHeight);
    auto res = getOrCreateCachedCharacterCollider(ud, cc, capsuleRadius, capsuleHalfHeight);
    transientUdToCurrNpc[ud] = &currNpc;
    if (nullptr != nextNpc) {
        transientUdToNextNpc[ud] = nextNpc;
    }
    return res;
}

CH_COLLIDER_T* BaseBattle::getOrCreateCachedCharacterCollider(const uint64_t ud, const CharacterConfig* cc, float newRadius, float newHalfHeight) {
    JPH_ASSERT(0 != ud);
    calcChCacheKey(cc, chCacheKeyHolder);
    CH_COLLIDER_T* chCollider = nullptr;
    auto it = cachedChColliders.find(chCacheKeyHolder);
    if (it == cachedChColliders.end()) {
        chCollider = createDefaultCharacterCollider(cc);
    } else {
        auto& q = it->second;
        if (q.empty()) {
            chCollider = createDefaultCharacterCollider(cc);
        } else {
            chCollider = q.back();
            q.pop_back();
        }
    }

    const RotatedTranslatedShape* shape = static_cast<const RotatedTranslatedShape*>(chCollider->GetShape());
    const CapsuleShape* innerShape = static_cast<const CapsuleShape*>(shape->GetInnerShape());
    JPH_ASSERT(nullptr != innerShape);
    if (innerShape->GetRadius() != newRadius || innerShape->GetHalfHeightOfCylinder() != newHalfHeight) {
        /*
        [WARNING]

        The feasibility of this hack is based on 3 facts.
        1. There's no shared "Shape" instance between "CH_COLLIDER_T" instances (and no shared "RotatedTranslatedShape::mInnerShape" either).
        2. This function "getOrCreateCachedCharacterCollider" is only used in a single-threaded context (i.e. as a preparation before the multi-threaded "PhysicsSystem::Update" or "Character::PostSimulation").
        3. Operator "=" for "RefConst<Shape>" would NOT call "Release()" when the new pointer address is the same as the old one.
        */
        Vec3Arg previousShapeCom = shape->GetCenterOfMass();

        int oldShapeRefCnt = shape->GetRefCount();
        int oldInnerShapeRefCnt = innerShape->GetRefCount();

        void* newInnerShapeBuffer = (void*)innerShape;
        CapsuleShape* newInnerShape = new (newInnerShapeBuffer) CapsuleShape(newHalfHeight, newRadius);

        void* newShapeBuffer = (void*)shape;
        RotatedTranslatedShape* newShape = new (newShapeBuffer) RotatedTranslatedShape(Vec3(0, newHalfHeight + newRadius, 0), Quat::sIdentity(), newInnerShape);
        JPH_ASSERT(nullptr != newShape);
        int newInnerShapeRefCnt = nullptr == newShape->GetInnerShape() ? 0 : newShape->GetInnerShape()->GetRefCount();
        JPH_ASSERT(oldInnerShapeRefCnt == newInnerShapeRefCnt);

        chCollider->SetShape(newShape,
            FLT_MAX, // Setting FLX_MAX here avoids updating active contacts immediately  
            true);

        while (newShape->GetRefCount() < oldShapeRefCnt) newShape->AddRef();
        while (newShape->GetRefCount() > oldShapeRefCnt) newShape->Release();
        int newShapeRefCnt = newShape->GetRefCount();
        JPH_ASSERT(oldShapeRefCnt == newShapeRefCnt);
    }

    // must be active when called by "getOrCreateCachedCharacterCollider"
    activeChColliders.push_back(chCollider);
    auto bodyID = chCollider->GetBodyID();
    transientUdToChCollider[ud] = chCollider;
    bi->SetUserData(bodyID, ud);

    return chCollider;
}

JPH::Body* BaseBattle::getOrCreateCachedBulletCollider(const uint64_t ud, const float immediateBoxHalfSizeX, const float immediateBoxHalfSizeY, const BulletType blType) {
    calcBlCacheKey(immediateBoxHalfSizeX, immediateBoxHalfSizeY, blCacheKeyHolder);
    EMotionType immediateMotionType = calcBlMotionType(blType);
    Vec3 newHalfExtent = Vec3(immediateBoxHalfSizeX, immediateBoxHalfSizeY, cDefaultHalfThickness);
    float newConvexRadius = 0;
    Body* blCollider = nullptr;
    auto it = cachedBlColliders.find(blCacheKeyHolder);
    if (it == cachedBlColliders.end() || it->second.empty()) {
        if (blStockCache->empty()) {
            blCollider = createDefaultBulletCollider(immediateBoxHalfSizeX, immediateBoxHalfSizeY, newConvexRadius, immediateMotionType);
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
    auto existingMotionType = blCollider->GetMotionType();
    const BoxShape* shape = static_cast<const BoxShape*>(blCollider->GetShape());
    auto existingHalfExtent = shape->GetHalfExtent();
    auto existingConvexRadius = shape->GetConvexRadius();

    if (existingHalfExtent != newHalfExtent || existingConvexRadius != newConvexRadius || existingMotionType != immediateMotionType) {
        Vec3Arg previousShapeCom = shape->GetCenterOfMass();

        int oldShapeRefCnt = shape->GetRefCount();
        
        void* newShapeBuffer = (void*)shape;
        BoxShape* newShape = new (newShapeBuffer) BoxShape(newHalfExtent, newConvexRadius);

        blCollider->SetShapeInternal(newShape, true);
        blCollider->SetMotionType(immediateMotionType);
        bi->NotifyShapeChanged(bodyID, previousShapeCom, true, EActivation::DontActivate);

        while (newShape->GetRefCount() < oldShapeRefCnt) newShape->AddRef();
        while (newShape->GetRefCount() > oldShapeRefCnt) newShape->Release();
        int newShapeRefCnt = newShape->GetRefCount();
        JPH_ASSERT(oldShapeRefCnt == newShapeRefCnt);
    }

    transientUdToBodyID[ud] = &bodyID;
    blCollider->SetUserData(ud);

    // must be active when called by "getOrCreateCachedBulletCollider"
    activeBlColliders.push_back(blCollider);

    return blCollider;
}

void BaseBattle::processWallGrabbingPostPhysicsUpdate(int currRdfId, const CharacterDownsync& currChd, CharacterDownsync* nextChd, const CharacterConfig* cc, const CH_COLLIDER_T* cv, bool inJumpStartupOrJustEnded) {
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
            nextChd->set_vel_y(cc->wall_sliding_vel_y());
            /*
            #ifndef NDEBUG
                            if (InAirIdle2ByJump == nextChd->ch_state()) {
                                Debug::Log("Character at (" + std::to_string(currChd.x()) + ", " + std::to_string(currChd.y()) + ") w/ frames_in_ch_state=" + std::to_string(currChd.frames_in_ch_state()) + ", vel=(" + std::to_string(currChd.vel_x()) + "," + std::to_string(currChd.vel_y()) + ") becomes OnWallIdle1 from InAirIdle2ByJump", DColor::Orange);
                            }
            #endif
            */
        } else if (!inJumpStartupOrJustEnded && hasBeenOnWall) {
            nextChd->set_ch_state(currChd.ch_state());
            nextChd->set_vel_y(cc->wall_sliding_vel_y());
        }

        break;
    }
    case OnWallIdle1:
    case OnWallAtk1: {
        nextChd->set_vel_y(cc->wall_sliding_vel_y());
        break;
    }
    }
}

bool BaseBattle::transitToDying(const CharacterDownsync& currChd, CharacterDownsync* nextChd) {
    if (CharacterState::Dying == currChd.ch_state()) return false;
    nextChd->set_ch_state(CharacterState::Dying);
    nextChd->set_frames_in_ch_state(0);
    nextChd->set_frames_to_recover(0);
    nextChd->set_frames_invinsible(0);
    nextChd->set_frames_captured_by_inertia(0);
    resetJumpStartup(nextChd);
    return true;
}

bool BaseBattle::transitToDying(const PlayerCharacterDownsync& currPlayer, PlayerCharacterDownsync* nextPlayer) {
    auto& currChd = currPlayer.chd();
    auto nextChd = nextPlayer->mutable_chd();
    bool res = transitToDying(currChd, nextChd);
    return res;
}

bool BaseBattle::transitToDying(const NpcCharacterDownsync& currNpc, NpcCharacterDownsync* nextNpc) {
    auto& currChd = currNpc.chd();
    auto nextChd = nextNpc->mutable_chd();
    bool res = transitToDying(currChd, nextChd);
    // For NPC should also reset patrol book-keepers
    nextNpc->set_frames_in_patrol_cue(0);
    return res;
}

void BaseBattle::updateChColliderBeforePhysicsUpdate(uint64_t ud, const CharacterDownsync& currChd, const CharacterDownsync& nextChd) {
    if (transientUdToChCollider.count(ud)) {
        auto chCollider = transientUdToChCollider[ud];
        /*
        From the source codes of [JPH::Body](https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Physics/Body/Body.h) and [MotionPropertis](https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Physics/Body/MotionProperties.h#L148) it seems like "accelerations" are only calculated during stepping, not cached.
        */
        auto bodyID = chCollider->GetBodyID();
        const CharacterConfig* cc = getCc(currChd.species_id());
        bool currNotDashing = isNotDashing(currChd);
        bool currEffInAir = isEffInAir(currChd, currNotDashing);
        bool inJumpStartupOrJustEnded = (isInJumpStartup(nextChd, cc) || isJumpStartupJustEnded(currChd, &nextChd, cc));
        if (currChd.omit_gravity() || nextChd.omit_gravity() || cc->omit_gravity() || inJumpStartupOrJustEnded) {
            bi->SetGravityFactor(bodyID, 0);
        } else {
            bi->SetGravityFactor(bodyID, 1);
        }
        bi->SetMotionQuality(bodyID, EMotionQuality::LinearCast);
        chCollider->SetPositionAndRotation(Vec3(currChd.x(), currChd.y(), currChd.z()), Quat::sIdentity(), EActivation::Activate);
        chCollider->SetLinearAndAngularVelocity(Vec3(nextChd.vel_x(), nextChd.vel_y(), nextChd.vel_z()), Vec3::sZero());
        // [REMINDER] "CharacterVirtual" maintains its own "mLinearVelocity" (https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Physics/Character/CharacterVirtual.h#L709) -- and experimentally setting velocity of its "mInnerBodyID" doesn't work (if "mInnerBodyID" was even set).
    }
}

RenderFrame* BaseBattle::CalcSingleStep(int currRdfId, int delayedIfdId, InputFrameDownsync* delayedIfd) {
    const RenderFrame* currRdf = rdfBuffer.GetByFrameId(currRdfId);
    if (nullptr == currRdf) return nullptr;
    RenderFrame* nextRdf = rdfBuffer.GetByFrameId(currRdfId + 1);
    if (!nextRdf) {
        nextRdf = rdfBuffer.DryPut();
    }

    nextRdf->CopyFrom(*currRdf);
    nextRdf->set_id(currRdfId + 1);
    elapse1RdfForRdf(currRdfId, nextRdf);

    float dt = globalPrimitiveConsts->estimated_seconds_per_rdf();

    processPlayerInputs(currRdfId, currRdf, dt, nextRdf, delayedIfdId, delayedIfd);

    batchPutIntoPhySysFromCache(currRdfId, currRdf, nextRdf); // [WARNING] After "processPlayerInputs" setting proper positions & velocities of "nextChd"s.

    for (int i = 0; i < playersCnt; i++) {
        auto currPlayer = currRdf->players_arr(i);
        auto nextPlayer = nextRdf->mutable_players_arr(i); // [WARNING] The indices of "currRdf->players_arr" and "nextRdf->players_arr" are ALWAYS FULLY ALIGNED.
        auto currChd = currPlayer.chd();
        auto nextChd = nextPlayer->chd();
        auto ud = calcUserData(currPlayer);
        updateChColliderBeforePhysicsUpdate(ud, currChd, nextChd);
    }

    for (int i = 0; i < currRdf->npcs_arr_size(); i++) {
        auto currNpc = currRdf->npcs_arr(i);
        if (globalPrimitiveConsts->terminating_character_id() == currNpc.id()) break;
        auto nextNpc = nextRdf->mutable_npcs_arr(i); // [WARNING] By reaching here, we haven't executed "leftShiftDeadNpcs", hence the indices of "currRdf->npcs_arr" and "nextRdf->npcs_arr" are FULLY ALIGNED.
        
        auto currChd = currNpc.chd();
        auto nextChd = nextNpc->chd();
        auto ud = calcUserData(currNpc);
        updateChColliderBeforePhysicsUpdate(ud, currChd, nextChd);
    }

    // Update positions and velocities of active bullets
    for (int i = 0; i < currRdf->bullets_size(); i++) {
        const Bullet& currBl = currRdf->bullets(i);
        if (globalPrimitiveConsts->terminating_bullet_id() == currBl.id()) break;
        Bullet* nextBl = nextRdf->mutable_bullets(i); // [WARNING] By reaching here, we haven't executed "leftShiftDeadBullets", hence the indices of "currRdf->bullets" and "nextRdf->bullets" are FULLY ALIGNED.
        auto ud = calcUserData(currBl);
        if (transientUdToBodyID.count(ud)) {
            auto bodyID = *(transientUdToBodyID[ud]);
            
            bi->SetPositionAndRotation(bodyID, Vec3(currBl.x(), currBl.y(), currBl.z()), Quat(currBl.q_x(), currBl.q_y(), currBl.q_z(), currBl.q_w()), EActivation::DontActivate); // It was already activated in "batchPutIntoPhySysFromCache"
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
        }
    }

    processNpcInputs(currRdfId, currRdf, dt, nextRdf, delayedIfdId, delayedIfd); // To see entities in vision before deciding Npc inputs

    // [REMINDER] The "class CharacterVirtual" instances WOULDN'T participate in "phySys->Update(...)" IF they were NOT filled with valid "mInnerBodyID". See "RuleOfThumb.md" for details.
    phySys->Update(dt, 1, globalTempAllocator, jobSys); 

    for (int i = 0; i < playersCnt; i++) {
        auto currPlayer = currRdf->players_arr(i);
        auto nextPlayer = nextRdf->mutable_players_arr(i); // [WARNING] The indices of "currRdf->players_arr" and "nextRdf->players_arr" are ALWAYS FULLY ALIGNED.
        const CharacterDownsync& currChd = currPlayer.chd();
        auto nextChd = nextPlayer->mutable_chd();

        auto ud = calcUserData(currPlayer);
        JPH_ASSERT(transientUdToChCollider.count(ud));
        CH_COLLIDER_T* single = transientUdToChCollider[ud];

        auto bodyID = single->GetBodyID(); 

        const CharacterConfig* cc = getCc(currChd.species_id());
        bool isDead = (0 >= nextChd->hp());

        RVec3 newPos;
        Quat newRot;
        single->GetPositionAndRotation(newPos, newRot);
        auto newVel = single->GetLinearVelocity();

        CharacterCollideShapeCollector collector(bi, ud, UDT_PLAYER, &currChd, nextChd, single->GetUp(), newPos, this);

        PostSimulationWithCollector(newPos, newRot, newVel, single, cCollisionTolerance, &collector); // Aggregates "CharacterBase.mGroundXxx" properties in a same "KernelThread"

        bool currNotDashing = isNotDashing(currChd);
        bool currEffInAir = isEffInAir(currChd, currNotDashing);
        bool inJumpStartupOrJustEnded = (isInJumpStartup(*nextChd, cc) || isJumpStartupJustEnded(currChd, nextChd, cc));
        auto cvGroundState = single->GetGroundState();

        bool oldNextNotDashing = isNotDashing(*nextChd);
        bool oldNextEffInAir = isEffInAir(*nextChd, oldNextNotDashing);
        bool isProactivelyJumping = proactiveJumpingSet.count(nextChd->ch_state());
        auto groundBodyID = single->GetGroundBodyID();
        auto groundBodyUd = single->GetGroundUserData();
        bool groundBodyIsChCollider = transientUdToChCollider.count(groundBodyUd);
        bool cvOnWall = (0 > nextChd->dir_x()*single->GetGroundNormal().GetX() && !groundBodyID.IsInvalid() && !groundBodyIsChCollider && single->IsSlopeTooSteep(single->GetGroundNormal()));
        bool cvSupported = single->IsSupported() && !cvOnWall && !groundBodyID.IsInvalid(); // [WARNING] "cvOnWall" and  "cvSupported" are mutually exclusive in this game!
        /* [WARNING]
        When a "CapsuleShape" is colliding with a "MeshShape", some unexpected z-offset might be caused by triangular pieces. We have to compensate for such unexpected z-offsets by setting the z-components to 0. 
            
        The process of unexpected z-offsets being introduced can be tracked in
        - [CollideConvexVsTriangles::Collide](https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Physics/Collision/CollideConvexVsTriangles.cpp#L137)
        - [PhysicsSystem::ProcessBodyPair::ReductionCollideShapeCollector::AddHit](https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Physics/PhysicsSystem.cpp#L1105)
        - [PhysicsSystem::ProcessBodyPair](https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Physics/PhysicsSystem.cpp#L1165)
        */
        nextChd->set_x(IsLengthNearZero(newPos.GetX()) ? 0 : newPos.GetX());
        nextChd->set_y(IsLengthNearZero(newPos.GetY()) ? 0 : newPos.GetY());
        nextChd->set_z(0);
        nextChd->set_vel_x(IsVelocityComponentNearZero(newVel.GetX()) ? 0 : newVel.GetX());
        nextChd->set_vel_y(IsVelocityComponentNearZero(newVel.GetY()) ? 0 : newVel.GetY());
        nextChd->set_vel_z(0);

        if (cvOnWall) {
/*
#ifndef NDEBUG
            if (CharacterState::InAirAtk1 == currChd.ch_state()) {
                std::ostringstream oss;
                oss << "@currRdfId=" << currRdfId << ", character ud=" << ud << ", bodyId=" << single->GetBodyID().GetIndexAndSequenceNumber() << ", currChdChState=" << currChd.ch_state() << ", currFramesInChState=" << currChd.frames_in_ch_state() << "; cvSupported=" << cvSupported << ", cvOnWall=" << cvOnWall << ", currNotDashing=" << currNotDashing << ", currEffInAir=" << currEffInAir << ", inJumpStartupOrJustEnded=" << inJumpStartupOrJustEnded << ", cvGroundState=" << CharacterBase::sToString(cvGroundState) << ", groundUd=" << single->GetGroundUserData() << ", groundBodyId=" << single->GetGroundBodyID().GetIndexAndSequenceNumber() << ", groundNormal=(" << single->GetGroundNormal().GetX() << ", " << single->GetGroundNormal().GetY() << ", " << single->GetGroundNormal().GetZ() << ")";
                Debug::Log(oss.str(), DColor::Orange);
            } 
#endif
*/
            if (cc->on_wall_enabled()) {
                // [WARNING] Will update "nextChd->vel_x() & nextChd->vel_y()".
                processWallGrabbingPostPhysicsUpdate(currRdfId, currChd, nextChd, cc, single, inJumpStartupOrJustEnded);
            }
        }

        bool cvInAir = (!cvSupported || cvOnWall);
        bool hasBeenOnWallIdle1 = (OnWallIdle1 == nextChd->ch_state() && OnWallIdle1 == currChd.ch_state());
        bool hasBeenOnWallAtk1 = (OnWallAtk1 == nextChd->ch_state() && OnWallAtk1 == currChd.ch_state());
        if ((hasBeenOnWallIdle1 || hasBeenOnWallAtk1) && nextChd->x() != currChd.x()) {
            nextChd->set_x(currChd.x()); // [WARNING] compensation for this known caveat of Jolt with horizontal-position change while GroundNormal is kept unchanged 
        }
/*
#ifndef NDEBUG
        if (groundBodyIsChCollider) {
            std::ostringstream oss;
            oss << "@currRdfId=" << currRdfId << ", player joinIndex=" << (i+1) << ", bodyId=" << single->GetBodyID().GetIndexAndSequenceNumber() << ", currChdChState=" << currChd.ch_state() << ", currFramesInChState=" << currChd.frames_in_ch_state() << "; cvSupported=" << cvSupported << ", cvOnWall=" << cvOnWall << ", currNotDashing=" << currNotDashing << ", currEffInAir=" << currEffInAir << ", inJumpStartupOrJustEnded=" << inJumpStartupOrJustEnded << ", cvGroundState=" << CharacterBase::sToString(cvGroundState) << ", groundUd(a character)=" << single->GetGroundUserData() << ", groundBodyId=" << single->GetGroundBodyID().GetIndexAndSequenceNumber() << ", groundNormal=(" << single->GetGroundNormal().GetX() << ", " << single->GetGroundNormal().GetY() << ", " << single->GetGroundNormal().GetZ() << ")";
            Debug::Log(oss.str(), DColor::Orange);
        }
#endif
*/
        postStepSingleChdStateCorrection(currRdfId, UDT_PLAYER, ud, single, currChd, nextChd, cc, cvSupported, cvInAir, cvOnWall, currNotDashing, currEffInAir, oldNextNotDashing, oldNextEffInAir, inJumpStartupOrJustEnded, cvGroundState);

        if (isDead) {
            if (CharacterState::Dying != nextChd->ch_state()) {
#ifndef NDEBUG
                std::ostringstream oss;
                oss << "@currRdfId=" << currRdfId << ", player joinIndex=" << currPlayer.join_index() << " is dead with nextChd->hp()=" << nextChd->hp() << ", will transit into Dying";
                Debug::Log(oss.str(), DColor::Orange);
#endif
                transitToDying(currPlayer, nextPlayer);
            } else if (globalPrimitiveConsts->dying_frames_to_recover() < nextChd->frames_in_ch_state()) {
                nextChd->set_hp(cc->hp());
                nextChd->set_mp(cc->mp());
                nextChd->set_ch_state(CharacterState::Idle1);
                nextChd->set_frames_in_ch_state(0);
                nextChd->set_x(currPlayer.revival_x());
                nextChd->set_y(currPlayer.revival_y());
                nextChd->set_dir_x(currPlayer.revival_dir_x());
                nextChd->set_dir_y(currPlayer.revival_dir_y());
                nextChd->set_vel_x(0);
                nextChd->set_vel_y(0);
            }
        }
    }

    for (int i = 0; i < currRdf->bullets_size(); i++) {
        const Bullet& currBl = currRdf->bullets(i);
        if (globalPrimitiveConsts->terminating_bullet_id() == currBl.id()) break;
        Bullet* nextBl = nextRdf->mutable_bullets(i); // [WARNING] By reaching here, we haven't executed "leftShiftDeadBullets", hence the indices of "currRdf->bullets" and "nextRdf->bullets" are FULLY ALIGNED.
        auto ud = calcUserData(currBl);
        if (transientUdToBodyID.count(ud)) {
            auto bodyID = *(transientUdToBodyID[ud]);

            JPH_ASSERT(transientUdToNextBl.count(ud));
            auto& nextBl = transientUdToNextBl[ud];

            auto newPos = bi->GetPosition(bodyID);
            auto newVel = bi->GetLinearVelocity(bodyID);

            nextBl->set_x(IsLengthNearZero(newPos.GetX()) ? 0 : newPos.GetX());
            nextBl->set_y(IsLengthNearZero(newPos.GetY()) ? 0 : newPos.GetY());
            nextBl->set_z(0);
            nextBl->set_vel_x(IsLengthNearZero(newVel.GetX()) ? 0 : newVel.GetX());
            nextBl->set_vel_y(IsLengthNearZero(newVel.GetY()) ? 0 : newVel.GetY());
            nextBl->set_vel_z(0);
        }
    }

    calcFallenDeath(currRdf, nextRdf);
    leftShiftDeadNpcs(currRdfId, nextRdf);
    leftShiftDeadBullets(currRdfId, nextRdf);
    leftShiftDeadPickables(currRdfId, nextRdf);
    batchRemoveFromPhySysAndCache(currRdfId, currRdf);

    return nextRdf;
}

void BaseBattle::Clear() {
    if (nullptr == phySys || nullptr == bi) {
        return;
    }

    if (!staticColliderBodyIDs.empty()) {
        bi->RemoveBodies(staticColliderBodyIDs.data(), staticColliderBodyIDs.size());
        bi->DestroyBodies(staticColliderBodyIDs.data(), staticColliderBodyIDs.size());
        staticColliderBodyIDs.clear();
    }
    
    /*
    [WARNING] 
    
    - No need to explicitly remove "CharacterVirtual.mInnerBodyID"s, the destructor "~CharacterVirtual" will take care of it.

    - No need to explicitly remove "Character.mBodyID"s, the destructor "~Character" will take care of it.
    */ 
    while (!activeChColliders.empty()) {
        auto& single = activeChColliders.back();
        activeChColliders.pop_back();
        single->RemoveFromPhysicsSystem();
        delete single;
    }

    while (!cachedChColliders.empty()) {
        for (auto it = cachedChColliders.begin(); it != cachedChColliders.end(); ) {
            auto& q = it->second;
            while (!q.empty()) {
                auto& single = q.back();
                q.pop_back();
                single->RemoveFromPhysicsSystem();
                delete single;
            }
            it = cachedChColliders.erase(it);
        }
    }

    bodyIDsToClear.clear();
    while (!activeBlColliders.empty()) {
        auto& single = activeBlColliders.back();
        activeBlColliders.pop_back();
        auto bodyID = single->GetID();
        bodyIDsToClear.push_back(bodyID);
    }

    while (!cachedBlColliders.empty()) {
        for (auto it = cachedBlColliders.begin(); it != cachedBlColliders.end(); ) {
            auto& q = it->second;
            while (!q.empty()) {
                auto& single = q.back();
                q.pop_back();
                auto bodyID = single->GetID();
                bodyIDsToClear.push_back(bodyID);
            }
            it = cachedBlColliders.erase(it);
        }
    }
    if (!bodyIDsToClear.empty()) {
        bi->RemoveBodies(bodyIDsToClear.data(), bodyIDsToClear.size());
        bi->DestroyBodies(bodyIDsToClear.data(), bodyIDsToClear.size());
        bodyIDsToClear.clear();
    }

    blStockCache = nullptr;

    // Deallocate temp variables in Pb arena
    pbTempAllocator.Reset();

    // Clear book keeping member variables
    lcacIfdId = -1;
    rdfBuffer.Clear();
    ifdBuffer.Clear();
    frameLogBuffer.Clear();

    playersCnt = 0;

    phySys->ClearBodyManagerFreeList();

    frameLogEnabled = false;
}

bool BaseBattle::ResetStartRdf(char* inBytes, int inBytesCnt) {
    WsReq* initializerMapData = google::protobuf::Arena::Create<WsReq>(&pbTempAllocator);
    initializerMapData->ParseFromArray(inBytes, inBytesCnt);
    return ResetStartRdf(initializerMapData);
}

bool BaseBattle::ResetStartRdf(const WsReq* initializerMapData) {
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
    auto startRdf = initializerMapData->self_parsed_rdf();
    playersCnt = startRdf.players_arr_size();
    allConfirmedMask = (U64_1 << playersCnt) - 1;

    auto stRdfId = startRdf.id();
    while (rdfBuffer.EdFrameId <= stRdfId) {
        int gapRdfId = rdfBuffer.EdFrameId;
        RenderFrame* holder = rdfBuffer.DryPut();
        holder->CopyFrom(startRdf);
        holder->set_id(gapRdfId);
    }
    if (frameLogEnabled) {
        while (frameLogBuffer.EdFrameId <= stRdfId) {
            int gapRdfId = frameLogBuffer.EdFrameId;
            FrameLog* holder = frameLogBuffer.DryPut();
            RenderFrame* heldRdf = holder->mutable_rdf();
            heldRdf->CopyFrom(startRdf);
            heldRdf->set_id(gapRdfId);
        }
    }

    prefabbedInputList.assign(playersCnt, 0);
    playerInputFrontIds.assign(playersCnt, 0);
    playerInputFronts.assign(playersCnt, 0);
    playerInputFrontIdsSorted.clear();

    int staticColliderId = 1;
    staticColliderBodyIDs.clear();
    for (int i = 0; i < initializerMapData->serialized_barrier_polygons_size(); i++) {
        auto& convexPolygon = initializerMapData->serialized_barrier_polygons(i);
        TriangleList triangles;
        for (int pi = 0; pi < convexPolygon.points_size(); pi += 2) {
            auto fromI = pi;
            auto toI = fromI + 2;
            if (toI >= convexPolygon.points_size()) {
                toI = 0;
            }
            auto x1 = convexPolygon.points(fromI);
            auto y1 = convexPolygon.points(fromI + 1);
            auto x2 = convexPolygon.points(toI);
            auto y2 = convexPolygon.points(toI + 1);

            Float3 v1 = Float3(x1, y1, +cDefaultBarrierHalfThickness);
            Float3 v2 = Float3(x1, y1, -cDefaultBarrierHalfThickness);
            Float3 v3 = Float3(x2, y2, +cDefaultBarrierHalfThickness);
            Float3 v4 = Float3(x2, y2, -cDefaultBarrierHalfThickness);

            triangles.push_back(Triangle(v2, v1, v3, 0)); // y: -, +, +
            triangles.push_back(Triangle(v3, v4, v2, 0)); // y: +, -, - 

            if (y1 < fallenDeathHeight) {
                fallenDeathHeight = y1;
            } 
            if (y2 < fallenDeathHeight) {
                fallenDeathHeight = y2;
            }
        }
        MeshShapeSettings bodyShapeSettings(triangles);
        MeshShapeSettings::ShapeResult shapeResult;
        /*
           "Body" and "BodyCreationSettings" will handle lifecycle of the following "bodyShape", i.e.
           - by "RefConst<Shape> Body::mShape" (note that "Body" does NOT hold a member variable "BodyCreationSettings") or
           - by "RefConst<ShapeSettings> BodyCreationSettings::mShape".

           See "<proj-root>/JoltBindings/RefConst_destructor_trick.md" for details.
         */
        Shape* bodyShape = new MeshShape(bodyShapeSettings, shapeResult);
        BodyCreationSettings bodyCreationSettings(bodyShape, RVec3::sZero(), JPH::Quat::sIdentity(), EMotionType::Static, MyObjectLayers::NON_MOVING);
        bodyCreationSettings.mUserData = calcStaticColliderUserData(staticColliderId); // As [BodyManager::AddBody](https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Physics/Body/BodyManager.cpp#L285) maintains "BodyID" counting by , in rollback netcode with a reused "BaseBattle" instance, even the same "static collider" might NOT get the same "BodyID" at different battles, we MUST use custom ids to distinguish "Body" instances!
        Body* body = bi->CreateBody(bodyCreationSettings);
        staticColliderBodyIDs.push_back(body->GetID());
#ifndef NDEBUG
        std::ostringstream oss;
        oss << "The " << i + 1 << "-th static collider with bodyID=" << body->GetID().GetIndexAndSequenceNumber() << ":\n\t";
        for (int pi = 0; pi < convexPolygon.points_size(); pi += 2) {
            auto x = convexPolygon.points(pi);
            auto y = convexPolygon.points(pi + 1);
            oss << "(" << x << "," << y << ") ";
        }
        Debug::Log(oss.str(), DColor::Orange);
#endif
        staticColliderId++;
    }
    auto layerState = bi->AddBodiesPrepare(staticColliderBodyIDs.data(), staticColliderBodyIDs.size());
    bi->AddBodiesFinalize(staticColliderBodyIDs.data(), staticColliderBodyIDs.size(), layerState, EActivation::DontActivate);

    phySys->OptimizeBroadPhase();

    transientUdToCurrPlayer.reserve(playersCnt);
    transientUdToNextPlayer.reserve(playersCnt);
    transientUdToCurrNpc.reserve(globalPrimitiveConsts->default_prealloc_npc_capacity());
    transientUdToNextNpc.reserve(globalPrimitiveConsts->default_prealloc_npc_capacity());

    safeDeactiviatedPosition = Vec3(65535.0, -65535.0, 0);

    preallocateBodies(&startRdf, initializerMapData->preallocate_npc_species_dict());

    return true;
}

bool BaseBattle::isEffInAir(const CharacterDownsync& chd, bool notDashing) {
    return (inAirSet.count(chd.ch_state()) && notDashing);
}

bool BaseBattle::isInJumpStartup(const CharacterDownsync& cd, const CharacterConfig* cc) {
    if (cd.omit_gravity() && !cc->omit_gravity()) {
        return (InAirIdle1ByJump == cd.ch_state() || InAirIdle1NoJump == cd.ch_state() || InAirWalking == cd.ch_state()) && (0 < cd.frames_to_start_jump());
    }

    return proactiveJumpingSet.count(cd.ch_state()) && (0 < cd.frames_to_start_jump());
}

bool BaseBattle::isJumpStartupJustEnded(const CharacterDownsync& currCd, const CharacterDownsync* nextCd, const CharacterConfig* cc) {
    if (currCd.omit_gravity() && !cc->omit_gravity()) {
        return ((InAirIdle1NoJump == currCd.ch_state() && InAirIdle1NoJump == nextCd->ch_state()) || (InAirWalking == currCd.ch_state() && InAirWalking == nextCd->ch_state()) || (InAirIdle1ByJump == currCd.ch_state() && InAirIdle1ByJump == nextCd->ch_state())) && (1 == currCd.frames_to_start_jump()) && (0 == nextCd->frames_to_start_jump());
    }
    bool yes11 = (InAirIdle1ByJump == currCd.ch_state() && InAirIdle1ByJump == nextCd->ch_state());
    bool yes12 = (InAirIdle1ByWallJump == currCd.ch_state() && InAirIdle1ByWallJump == nextCd->ch_state());
    bool yes13 = (InAirIdle2ByJump == currCd.ch_state() && InAirIdle2ByJump == nextCd->ch_state());
    bool yes1 = yes11 || yes12 || yes13;
    return (yes1 && (1 == currCd.frames_to_start_jump()) && (0 == nextCd->frames_to_start_jump()));
}

void BaseBattle::resetJumpStartup(CharacterDownsync* nextChd, bool putBtnHoldingJammed) {
    nextChd->set_jump_started(false);
    nextChd->set_jump_triggered(false);
    nextChd->set_slip_jump_triggered(false);
    nextChd->set_frames_to_start_jump(0);
    if (putBtnHoldingJammed) {
        jamBtnHolding(nextChd);
    }
}

void BaseBattle::transitToGroundDodgedChState(CharacterDownsync* nextChd, const CharacterConfig* cc, bool currParalyzed) {
    CharacterState oldNextChState = nextChd->ch_state();
    nextChd->set_ch_state(GroundDodged);
    nextChd->set_frames_in_ch_state(0);
    nextChd->set_frames_to_recover(cc->ground_dodged_frames_to_recover());
    nextChd->set_frames_invinsible(cc->ground_dodged_frames_invinsible());
    nextChd->set_frames_captured_by_inertia(0);
    nextChd->set_active_skill_id(globalPrimitiveConsts->no_skill());
    nextChd->set_active_skill_hit(globalPrimitiveConsts->no_skill_hit());
    if (0 == nextChd->vel_x()) {
        auto effSpeed = (0 >= cc->ground_dodged_speed() ? cc->speed() : cc->ground_dodged_speed());
        if (BackDashing == oldNextChState) {
            nextChd->set_vel_x(0 > nextChd->dir_x() ? effSpeed : -effSpeed);
        } else {
            nextChd->set_vel_x(0 < nextChd->dir_x() ? effSpeed : -effSpeed);
        }
    }

    if (currParalyzed) {
        nextChd->set_vel_x(0);
    }

    resetJumpStartup(nextChd);
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

void BaseBattle::prepareJumpStartup(int currRdfId, const CharacterDownsync& currChd, CharacterDownsync* nextChd, bool currEffInAir, const CharacterConfig* cc, bool currParalyzed) {
    if ((TransformingInto == currChd.ch_state() && 0 < currChd.frames_to_recover()) || (TransformingInto == nextChd->ch_state() && 0 < nextChd->frames_to_recover())) {
        return;
    }

    if (currParalyzed) {
        return;
    }

    if (0 == cc->jumping_init_vel_y()) {
        return;
    }

    if (isInJumpStartup(*nextChd, cc)) {
        return;
    }

    // TODO: Interpolate velocity by "{init_jump_vel_y, frames_to_start_jump, proactive_jump_startup_frames}"?
    if (isJumpStartupJustEnded(currChd, nextChd, cc)) {
        nextChd->set_jump_started(true);
    } else if ((nextChd->jump_triggered() || nextChd->slip_jump_triggered()) && (!currChd.jump_started() && !nextChd->jump_started())) {
        // [WARNING] This assignment blocks a lot of CharacterState transition logic, including "processInertiaWalking"!
        if (OnWallIdle1 == currChd.ch_state()) {
            nextChd->set_frames_to_start_jump(cc->proactive_jump_startup_frames() >> 1);
            nextChd->set_ch_state(InAirIdle1ByWallJump);
            nextChd->set_vel_y(0);
        } else if (currEffInAir && !currChd.omit_gravity()) {
            if (0 < currChd.remaining_air_jump_quota()) {
                nextChd->set_frames_to_start_jump(globalPrimitiveConsts->in_air_jump_grace_period_rdf_cnt());
                nextChd->set_ch_state(InAirIdle2ByJump);
                nextChd->set_vel_y(0);
                nextChd->set_remaining_air_jump_quota(currChd.remaining_air_jump_quota() - 1);
                if (!cc->isolated_air_jump_and_dash_quota() && 0 < nextChd->remaining_air_dash_quota()) {
                    nextChd->set_remaining_air_dash_quota(nextChd->remaining_air_dash_quota() - 1);
                }
                /*
                #ifndef NDEBUG
                                Debug::Log("Character at (" + std::to_string(currChd.x()) + ", " + std::to_string(currChd.y()) + ") w/ frames_in_ch_state=" + std::to_string(currChd.frames_in_ch_state()) + ", vel=(" + std::to_string(currChd.vel_x()) + "," + std::to_string(currChd.vel_y()) + ") becomes InAirIdle2ByJump from " + std::to_string(currChd.ch_state()), DColor::Orange);
                #endif
                */
            }
        } else {
            // [WARNING] Including "slip_jump_triggered()" here
            nextChd->set_frames_to_start_jump(cc->proactive_jump_startup_frames());
            nextChd->set_ch_state(InAirIdle1ByJump);
        }
    }
}

void BaseBattle::processJumpStarted(int currRdfId, const CharacterDownsync& currChd, CharacterDownsync* nextChd, bool currEffInAir, const CharacterConfig* cc) {
    bool jumpStarted = nextChd->jump_started();
    if (jumpStarted) {
        if (InAirIdle1ByWallJump == currChd.ch_state()) {
            // Always jump to the opposite direction of wall inward norm
            float xfac = (0 > currChd.dir_x() ? 1 : -1);
            nextChd->set_vel_x(xfac * cc->wall_jumping_init_vel_x());
            nextChd->set_vel_y(cc->wall_jumping_init_vel_y());
            nextChd->set_frames_to_recover(cc->wall_jumping_frames_to_recover());
        } else if (InAirIdle2ByJump == nextChd->ch_state()) {
            nextChd->set_vel_y(cc->jumping_init_vel_y());
            /*
            #ifndef NDEBUG
                        Debug::Log("Character at (" + std::to_string(currChd.x()) + ", " + std::to_string(currChd.y()) + ") w/ frames_in_ch_state=" + std::to_string(currChd.frames_in_ch_state()) + ", vel=(" + std::to_string(currChd.vel_x()) + "," + std::to_string(currChd.vel_y()) + ") sets vel_y for InAirIdle2ByJump = " + std::to_string(nextChd->vel_y()), DColor::Orange);
            #endif
            */
        } else if (currChd.slip_jump_triggered()) {
            nextChd->set_vel_y(0);
            if (currChd.omit_gravity() && !cc->omit_gravity() && cc->jump_holding_to_fly()) {
                nextChd->set_ch_state(InAirIdle1NoJump);
                nextChd->set_omit_gravity(false);
                nextChd->set_flying_rdf_countdown(0);
            }
        } else {
            nextChd->set_vel_y(cc->jumping_init_vel_y());
        }

        resetJumpStartup(nextChd);
    } else if (!cc->omit_gravity() && cc->jump_holding_to_fly() && currChd.omit_gravity() && 0 >= currChd.flying_rdf_countdown()) {
        nextChd->set_ch_state(InAirIdle1NoJump);
        nextChd->set_omit_gravity(false);
    }
}

void BaseBattle::processInertiaWalkingHandleZeroEffDx(int currRdfId, float dt, const CharacterDownsync& currChd, CharacterDownsync* nextChd, int effDy, const CharacterConfig* cc, bool effInAir, bool currParalyzed) {
    if (proactiveJumpingSet.count(currChd.ch_state())) {
        // [WARNING] In general a character is not permitted to just stop velX during proactive jumping.
        return;
    }

    if (currParalyzed) {
        return;
    }

    if (onWallSet.count(currChd.ch_state()) || !inAirSet.count(currChd.ch_state())) {
        float newVelX = 0;
        int xfac = (0 == currChd.vel_x() ? 0 : (0 < currChd.vel_x() ? -1 : +1));
        if (!currParalyzed) {
            newVelX = lerp(currChd.vel_x(), 0, xfac * cc->acc_mag() * dt);
        }
        nextChd->set_vel_x(newVelX);
        if ((Walking == currChd.ch_state() || WalkingAtk1 == currChd.ch_state() || WalkingAtk4 == currChd.ch_state()) && 0 != newVelX && !currParalyzed) {
            // [WARNING] Only keep walking @"0 == effDx" when the character has been proactively walking 
            nextChd->set_ch_state(Walking);
        }
    }

    if (0 < currChd.frames_to_recover() || effInAir || !cc->has_def1() || 0 >= effDy) {
        return;
    }

    nextChd->set_ch_state(Def1);
    if (Def1 == currChd.ch_state()) return;
    nextChd->set_frames_in_ch_state(0);
    nextChd->set_remaining_def1_quota(cc->default_def1_quota());
}

void BaseBattle::processInertiaWalking(int rdfId, float dt, const CharacterDownsync& currChd, CharacterDownsync* nextChd, bool currEffInAir, int effDx, int effDy, const CharacterConfig* cc, bool currParalyzed, bool currInBlockStun) {
    if (0 < currChd.frames_to_recover()) {
        return;
    }

    if ((TransformingInto == currChd.ch_state() && 0 < currChd.frames_to_recover()) || (TransformingInto == nextChd->ch_state() && 0 < nextChd->frames_to_recover())) {
        return;
    }

    if (isInJumpStartup((*nextChd), cc) || isJumpStartupJustEnded(currChd, nextChd, cc)) {
        return;
    }

    if (currInBlockStun) {
        return;
    }
    
    bool alignedWithInertia = true;
    bool exactTurningAround = false;
    bool stoppingFromWalking = false;
    if (0 != effDx && 0 == currChd.vel_x()) {
        alignedWithInertia = false;
    } else if (0 == effDx && 0 != currChd.vel_x()) {
        alignedWithInertia = false;
        stoppingFromWalking = true;
    } else if (0 > effDx * currChd.vel_x()) {
        alignedWithInertia = false;
        exactTurningAround = true;
    }

    nextChd->set_ch_state(Idle1); // The default transition
    bool hasNonZeroSpeed = !(0 == cc->speed() && 0 == currChd.speed());
    bool recoveredFromAirAtk = (0 < currChd.frames_to_recover() && currEffInAir && !nonAttackingSet.count(currChd.ch_state()) && 0 == nextChd->frames_to_recover());
    float newVelX = 0, targetNewVelX = 0;
    if (0 != effDx && hasNonZeroSpeed) {
        int xfac = (0 < effDx ? 1 : -1);
        float velXStep = xfac * cc->acc_mag() * dt;
        if (exactTurningAround) {
            velXStep *= 4;
        }
        if (InAirIdle1ByWallJump == currChd.ch_state()) {
            if (!currParalyzed) {
                if (exactTurningAround) {
                    velXStep *= 8;
                }
                targetNewVelX = xfac * cc->wall_jumping_init_vel_x();
            }
        } else {
            if (!currParalyzed) targetNewVelX = xfac * cc->speed();
        }
        newVelX = lerp(currChd.vel_x(), targetNewVelX, velXStep);
        nextChd->set_vel_x(newVelX);

        if (0 < effDx * newVelX) {
            // Only correct flipping under proactive input.
            if (InAirIdle1ByWallJump == currChd.ch_state()) {
                /*
    #ifndef NDEBUG
                std::ostringstream oss;
                oss << "@rdfId=" << rdfId << "currVelX=" << currChd.vel_x() << ", velXStep=" << velXStep << ", newVelX=" << newVelX << ", targetNewVelX=" << targetNewVelX;
                Debug::Log(oss.str(), DColor::Orange);
    #endif
                */
                if (0 < newVelX) {
                    nextChd->set_dir_x(+2);
                } else if (0 > newVelX) {
                    nextChd->set_dir_x(-2);
                } // Otherwise change is NOT needed
            } else {
                nextChd->set_dir_x(effDx);
                if (OnWallIdle1 == currChd.ch_state()) {
                    nextChd->set_ch_state(InAirIdle1NoJump);
                    nextChd->set_frames_in_ch_state(0);
                }
            }
        }

        if (!currParalyzed) {
            if (0 == newVelX && !proactiveJumpingSet.count(currChd.ch_state())) {
                nextChd->set_ch_state(Idle1);
            } else {
                nextChd->set_ch_state(Walking);
                if (exactTurningAround && cc->has_turn_around_anim() && !currEffInAir) {
                    nextChd->set_ch_state(TurnAround);
                }
            }
        }
    } else {
        // 0 == effDx or speed is zero
        if (0 != effDx) {
            // false == hasNonZeroSpeed, no need to handle velocity lerping
            nextChd->set_dir_x(effDx);
        } else {
            processInertiaWalkingHandleZeroEffDx(rdfId, dt, currChd, nextChd, effDy, cc, currEffInAir, currParalyzed);
        }
    }

    if (!nextChd->jump_triggered() && !currEffInAir && 0 > effDy && cc->crouching_enabled()) {
        // [WARNING] This particular condition is set to favor a smooth "Sliding -> CrouchIdle1" & "CrouchAtk1 -> CrouchAtk1" transitions, we couldn't use "0 == nextChd->frames_to_recover()" for checking here because "CrouchAtk1 -> CrouchAtk1" transition would break by 1 frame. 
        if (1 >= currChd.frames_to_recover()) {
            nextChd->set_vel_x(0);
            nextChd->set_ch_state(CrouchIdle1);
        }
    }
}

void BaseBattle::processInertiaFlyingHandleZeroEffDxAndDy(int rdfId, float dt, const CharacterDownsync& currChd, CharacterDownsync* nextChd, const CharacterConfig* cc, bool currParalyzed) {
    nextChd->set_vel_x(0);
    if (!cc->anti_gravity_when_idle() || InAirIdle1NoJump != currChd.ch_state()) {
        nextChd->set_vel_y(0);
        nextChd->set_dir_y(0);
    }
}

void BaseBattle::processInertiaFlying(int rdfId, float dt, const CharacterDownsync& currChd, CharacterDownsync* nextChd, int effDx, int effDy, const CharacterConfig* cc, bool currParalyzed, bool currInBlockStun) {
    if ((TransformingInto == currChd.ch_state() && 0 < currChd.frames_to_recover()) || (TransformingInto == nextChd->ch_state() && 0 < nextChd->frames_to_recover())) {
        return;
    }
    if (currInBlockStun) {
        return;
    }

    // TODO: Interpolate velocity by "{CharacterConfig.speed, inertia_frames_to_recover, frames_captured_by_inertia}"?
    bool currFreeFromInertia = (0 == currChd.frames_captured_by_inertia());
    bool currBreakingFromInertia = (1 == currChd.frames_captured_by_inertia());

    bool withInertiaBreakingState = (nextChd->jump_triggered() || (InAirIdle1ByWallJump == currChd.ch_state()));
    bool alignedWithInertia = true;
    bool exactTurningAround = false;
    bool stoppingFromWalking = false;
    if ((0 != effDx && 0 == currChd.vel_x()) || (0 != effDy && 0 == currChd.vel_y())) {
        alignedWithInertia = false;
    } else if ((0 == effDx && 0 != currChd.vel_x()) || (0 == effDy && 0 != currChd.vel_y())) {
        alignedWithInertia = false;
        stoppingFromWalking = true;
    } else if ((0 > effDx * currChd.vel_x()) || (0 > effDy * currChd.vel_y())) {
        alignedWithInertia = false;
        exactTurningAround = true;
    }

    bool hasNonZeroSpeed = !(0 == cc->speed() && 0 == currChd.speed());
    if (0 == currChd.frames_to_recover()) {
        auto defaultInAirIdleChState = cc->use_idle1_as_flying_idle() ? Idle1 : Walking;
        nextChd->set_ch_state(((Idle1 == currChd.ch_state() || InAirIdle1NoJump == currChd.ch_state()) && cc->anti_gravity_when_idle()) ? currChd.ch_state() : defaultInAirIdleChState); // When reaching here, the character is at least recovered from "Atked{N}" or "Atk{N}" state, thus revert back to a default action

        if (alignedWithInertia || withInertiaBreakingState || currBreakingFromInertia) {
            if (!alignedWithInertia) {
                // Should reset "frames_captured_by_inertia" in this case!
                nextChd->set_frames_captured_by_inertia(0);
            }

            if ((0 != effDx || 0 != effDy) && hasNonZeroSpeed) {
                nextChd->set_dir_x(0 == effDx ? currChd.dir_x() : (0 > effDx ? -2 : +2));
                nextChd->set_dir_y(0 == effDy ? currChd.dir_y() : (0 > effDy ? -1 : +1));
                int xfac = 0 == effDx ? 0 : 0 > effDx ? -1 : +1;
                int yfac = 0 == effDy ? 0 : 0 > effDy ? -1 : +1;
                nextChd->set_vel_x(currParalyzed ? 0 : xfac * currChd.speed());
                nextChd->set_vel_y(currParalyzed ? 0 : yfac * currChd.speed());
                nextChd->set_ch_state(Walking);
            } else {
                // (0 == effDx && 0 == effDy) or speed is zero
                processInertiaFlyingHandleZeroEffDxAndDy(rdfId, dt, currChd, nextChd, cc, currParalyzed);
            }
        } else if (currFreeFromInertia) {
            auto effInertiaFramesToRecover = (0 < cc->inertia_frames_to_recover() ? cc->inertia_frames_to_recover() : 1);
            auto effInertiaFramesToRecoverForWalkStarting = (0 < (effInertiaFramesToRecover >> 3) ? (effInertiaFramesToRecover >> 3) : 1);
            if (exactTurningAround) {
                // logger.LogInfo(stringifyPlayer(currChd) + " is turning around at currRdfId=" + currRdfId);
                nextChd->set_ch_state(cc->has_turn_around_anim() ? TurnAround : Walking);
                nextChd->set_frames_captured_by_inertia(effInertiaFramesToRecover);
                if (effInertiaFramesToRecover > nextChd->frames_to_recover()) {
                    // [WARNING] Deliberately not setting "nextChd->frames_to_recover()" if not turning around to allow using skills!
                    nextChd->set_frames_to_recover(effInertiaFramesToRecover - 1); // To favor animation playing and prevent skill use when turning-around
                }
            } else if (stoppingFromWalking) {
                // Keeps CharacterState and thus the animation to make user see graphical feedback asap.
                nextChd->set_ch_state(cc->has_walk_stopping_anim() ? WalkStopping : Walking);
                nextChd->set_frames_captured_by_inertia(effInertiaFramesToRecover);
            } else {
                // Updates CharacterState and thus the animation to make user see graphical feedback asap.
                nextChd->set_frames_captured_by_inertia(effInertiaFramesToRecoverForWalkStarting);
                nextChd->set_ch_state(Walking);
            }
        } else {
            // [WARNING] Not free from inertia, just set proper next chState
            if (0 != effDx || 0 != effDy) {
                nextChd->set_ch_state(Walking);
            }
        }
    }
}


bool BaseBattle::addNewBulletToNextFrame(int rdfId, const CharacterDownsync& currChd, CharacterDownsync* nextChd, const CharacterConfig* cc, bool currParalyzed, bool currEffInAir, int xfac, int yfac, const Skill* skillConfig, int activeSkillHit, uint32_t activeSkillId, RenderFrame* nextRdf, const Bullet* referenceBullet, const BulletConfig* referenceBulletConfig, uint64_t offenderUd, int bulletTeamId) {
    if (globalPrimitiveConsts->no_skill_hit() == activeSkillHit || activeSkillHit > skillConfig->hits_size()) return false;
    if (nextRdf->bullet_id_counter() >= nextRdf->bullets_size()) {
#ifndef  NDEBUG
        std::ostringstream oss;
        oss << "@rdfId=" << rdfId << ", bullet overwhelming#1";
        Debug::Log(oss.str(), DColor::Orange);
#endif // ! NDEBUG
        return false;
    }
    const BulletConfig& bulletConfig = skillConfig->hits(activeSkillHit - 1);
    const BulletConfig* prevBulletConfig = referenceBulletConfig;
    if (nullptr == prevBulletConfig) {
        if (2 <= activeSkillHit) {
            const BulletConfig& defaultBulletConfig = skillConfig->hits(activeSkillHit - 2);
            prevBulletConfig = &defaultBulletConfig;
        }
    }

    if (OnWallAtk1 == skillConfig->bound_ch_state() && (BulletType::Melee != bulletConfig.b_type())) {
        xfac *= -1;
    }

    float newOriginatedX = (nullptr == referenceBullet || bulletConfig.use_ch_offset_regardless_of_emission_mh()) ? (currChd.x() + xfac * bulletConfig.hitbox_offset_x()) : referenceBullet->originated_x();
    float newOriginatedY = (nullptr == referenceBullet || bulletConfig.use_ch_offset_regardless_of_emission_mh()) ? (currChd.y() + bulletConfig.hitbox_offset_y()) : referenceBullet->originated_y();
    float newX = (nullptr == referenceBullet || (BulletType::Melee == bulletConfig.b_type() && MultiHitType::FromVisionSeekOrDefault != bulletConfig.mh_type())) ? currChd.x() + xfac * bulletConfig.hitbox_offset_x() : referenceBullet->x() + xfac * bulletConfig.hitbox_offset_x();
    float newY = (nullptr == referenceBullet || (BulletType::Melee == bulletConfig.b_type() && MultiHitType::FromVisionSeekOrDefault != bulletConfig.mh_type())) ? currChd.y() + bulletConfig.hitbox_offset_y() : referenceBullet->y() + bulletConfig.hitbox_offset_y();

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

    float dstQx = 0.f, dstQy = 0.f, dstQz = 0.f, dstQw = 1.f;
    if (bulletConfig.mh_inherits_spin()) {
        if (nullptr != referenceBullet) {
            dstQx = referenceBullet->q_x();
            dstQy = referenceBullet->q_y();
            dstQz = referenceBullet->q_z();
            dstQw = referenceBullet->q_w();
        }
    } else if (bulletConfig.has_init_q()) {
        auto& init_q = bulletConfig.init_q();
        if (0 <= xfac) {
            dstQx = init_q.x();
            dstQy = init_q.y();
            dstQz = init_q.z();
            dstQw = init_q.w();
        } else {
            // Rotated around y-axis, i.e. (x=0, y=1, z=0, w=0), i.e. (0*i + 1*j + 0*k + 0) first, then apply "init_q_*"
            dstQx = -init_q.z();
            dstQy =  init_q.w();
            dstQz =  init_q.x();
            dstQw = -init_q.y();
        }
    } else {
        if (0 <= xfac) {
            dstQx = 0;
            dstQy = 0;
            dstQz = 0;
            dstQw = 1;
        } else {
            dstQx = 0;
            dstQy = 1;
            dstQz = 0;
            dstQw = 0;
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

    int oldBulletIdCounter = nextRdf->bullet_id_counter();
    int oldBulletCount = nextRdf->bullet_count();
    auto nextBl = nextRdf->mutable_bullets(oldBulletCount);
    nextBl->set_id(oldBulletIdCounter);
    nextBl->set_originated_render_frame_id(rdfId);
    nextBl->set_bl_state(initBlState);
    nextBl->set_frames_in_bl_state(initFramesInBlState);
    int newBulletIdCounter = oldBulletIdCounter + 1;
    int newBulletCount = oldBulletCount + 1;
    auto ud = calcUserData(*nextBl);
    nextBl->set_offender_ud(offenderUd);
    nextBl->set_ud(ud);
    nextBl->set_originated_x(newOriginatedX);
    nextBl->set_originated_y(newOriginatedY);
    nextBl->set_x(newX);
    nextBl->set_y(newY);
    nextBl->set_q_x(dstQx);
    nextBl->set_q_y(dstQy);
    nextBl->set_q_z(dstQz);
    nextBl->set_q_w(dstQw);
    nextBl->set_team_id(currChd.bullet_team_id());

    JPH::Quat blInitQ(dstQx, dstQy, dstQz, dstQw);
    Vec3Arg blInitVelocity(bulletConfig.speed(), 0, 0);
    auto blEffVelocity = blInitQ*blInitVelocity;
    nextBl->set_vel_x(blEffVelocity.GetX());
    nextBl->set_vel_y(blEffVelocity.GetY());
    nextBl->set_skill_id(activeSkillId);
    nextBl->set_active_skill_hit(activeSkillHit);
    nextBl->set_repeat_quota_left(bulletConfig.repeat_quota());
    nextBl->set_remaining_hard_pushback_bounce_quota(bulletConfig.default_hard_pushback_bounce_quota());
    nextBl->set_exploded_on_ifc(bulletConfig.ifc());

    nextRdf->set_bullet_id_counter(newBulletIdCounter);
    nextRdf->set_bullet_count(newBulletCount);

    // [WARNING] This part locks velocity by the last bullet in the simultaneous array
    if (!bulletConfig.delay_self_vel_to_active() && !currParalyzed) {
        if (globalPrimitiveConsts->no_lock_vel() != bulletConfig.self_lock_vel_x()) {
            nextChd->set_vel_x(xfac * bulletConfig.self_lock_vel_x());
        }
        if (!currChd.omit_gravity()) {
            if (globalPrimitiveConsts->no_lock_vel() != bulletConfig.self_lock_vel_y()) {
                if (0 <= bulletConfig.self_lock_vel_y() || currEffInAir) {
                    // [WARNING] DON'T assign negative velY to a character not in air!
                    nextChd->set_vel_y(bulletConfig.self_lock_vel_y());
                }
            }
        } else {
            if (globalPrimitiveConsts->no_lock_vel() != bulletConfig.self_lock_vel_y_when_flying()) {
                nextChd->set_vel_y(bulletConfig.self_lock_vel_y_when_flying());
            }
        }
    }

    // Explicitly specify termination of nextRenderFrameBullets
    if (newBulletCount < nextRdf->bullets_size()) {
        auto terminationBl = nextRdf->mutable_bullets(newBulletCount);
        terminationBl->set_id(globalPrimitiveConsts->terminating_bullet_id());
    }
/*
#ifndef  NDEBUG
    std::ostringstream oss;
    oss << "@rdfId=" << rdfId << ", added new bullet with bulletId=" << nextBl->id() << ", pos=(" << nextBl->x() << ", " << nextBl->y() << ", " << nextBl->z() << "), vel=(" << nextBl->vel_x() << ", " << nextBl->vel_y() << ", " << nextBl->vel_z() << ")";
    Debug::Log(oss.str(), DColor::Orange);
#endif // ! NDEBUG
*/
    if (0 < bulletConfig.simultaneous_multi_hit_cnt() && activeSkillHit < skillConfig->hits_size()) {
        return addNewBulletToNextFrame(rdfId, currChd, nextChd, cc, currParalyzed, currEffInAir, xfac, yfac, skillConfig, activeSkillHit + 1, activeSkillId, nextRdf, referenceBullet, referenceBulletConfig, offenderUd, bulletTeamId);
    } else {
        return true;
    }
}

void BaseBattle::elapse1RdfForRdf(int currRdfId, RenderFrame* nextRdf) {
    for (int i = 0; i < playersCnt; i++) {
        auto player = nextRdf->mutable_players_arr(i);
        const CharacterConfig* cc = getCc(player->chd().species_id());
        elapse1RdfForPlayerChd(player, cc);
    }

    for (int i = 0; i < nextRdf->npcs_arr_size(); i++) {
        auto npc = nextRdf->mutable_npcs_arr(i);
        if (globalPrimitiveConsts->terminating_character_id() == npc->id()) break;
        const CharacterConfig* cc = getCc(npc->chd().species_id()); // TODO
        elapse1RdfForNpcChd(npc, cc);
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

    for (int i = 0; i < nextRdf->pickables_size(); i++) {
        auto pk = nextRdf->mutable_pickables(i);
        if (globalPrimitiveConsts->terminating_pickable_id() == pk->id()) break;
        elapse1RdfForPickable(pk);
    }
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

void BaseBattle::elapse1RdfForBl(int currRdfId, Bullet* bl, const Skill* skill, const BulletConfig* bulletConfig) {
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
            oss << "bulletId=" << bl->id() << ", originatedRenderFrameId=" << bl->originated_render_frame_id() << " just became active at rdfId=" << currRdfId+1 << ", pos=(" << bl->x() << ", " << bl->y() << ", " << bl->z() << "), vel=(" << bl->vel_x() << ", " << bl->vel_y() << ", " << bl->vel_z() << ")";
            Debug::Log(oss.str(), DColor::Orange);
#endif // ! NDEBUG
*/
        }
        break;
    case BulletState::Active:
        if (newFramesInBlState > bulletConfig->active_frames()) {
            if (bulletConfig->touch_explosion_bomb_collision()) {
                bl->set_bl_state(BulletState::Exploding);
            } else {
                bl->set_bl_state(BulletState::Vanishing);
/*
#ifndef  NDEBUG
                std::ostringstream oss;
                oss << "bulletId=" << bl->id() << ", originatedRenderFrameId=" << bl->originated_render_frame_id() << " just became vanishing at rdfId=" << currRdfId+1 << ", pos=(" << bl->x() << ", " << bl->y() << ", " << bl->z() << "), vel=(" << bl->vel_x() << ", " << bl->vel_y() << ", " << bl->vel_z() << ")";
                Debug::Log(oss.str(), DColor::Orange);
#endif // ! NDEBUG
*/
            }
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

void BaseBattle::elapse1RdfForPlayerChd(PlayerCharacterDownsync* playerChd, const CharacterConfig* cc) {
    auto chd = playerChd->mutable_chd();
    elapse1RdfForChd(chd, cc);
    playerChd->set_not_enough_mp_hint_rdf_countdown(0 < playerChd->not_enough_mp_hint_rdf_countdown() ? playerChd->not_enough_mp_hint_rdf_countdown() - 1 : 0);

}

void BaseBattle::elapse1RdfForNpcChd(NpcCharacterDownsync* npcChd, const CharacterConfig* cc) {
    auto chd = npcChd->mutable_chd();
    elapse1RdfForChd(chd, cc);
    npcChd->set_frames_in_patrol_cue(0 < npcChd->frames_in_patrol_cue() ? npcChd->frames_in_patrol_cue() - 1 : 0);
}

void BaseBattle::elapse1RdfForChd(CharacterDownsync* cd, const CharacterConfig* cc) {
    cd->set_frames_to_recover((0 < cd->frames_to_recover() ? cd->frames_to_recover() - 1 : 0));
    cd->set_frames_captured_by_inertia(0 < cd->frames_captured_by_inertia() ? cd->frames_captured_by_inertia() - 1 : 0);
    cd->set_frames_in_ch_state(cd->frames_in_ch_state() + 1);
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
    cd->set_frames_to_start_jump((0 < cd->frames_to_start_jump() ? cd->frames_to_start_jump() - 1 : 0));
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
        /*
           [WARNING]

           All "critical input predictions (i.e. BtnA/B/C/D/E)" are now handled only in "UpdateInputFrameInPlaceUponDynamics", which is called just before rendering "playerRdf" -- the only timing that matters for smooth graphcis perception of (human) players.
         */
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
        ifdHolder->set_confirmed_list(0u); // To avoid RingBuff reuse contamination.
        ifdHolder->set_udp_confirmed_list(0u);

        auto inputList = ifdHolder->mutable_input_list();
        inputList->Clear();
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
    bodyIDsToActivate.clear();
    for (int i = 0; i < playersCnt; i++) {
        const PlayerCharacterDownsync& currPlayer = currRdf->players_arr(i);
        const CharacterDownsync& currChd = currPlayer.chd();

        PlayerCharacterDownsync* nextPlayer = nextRdf->mutable_players_arr(i);

        const CharacterConfig* cc = getCc(currChd.species_id());
        auto ud = calcUserData(currPlayer);
        CH_COLLIDER_T* chCollider = getOrCreateCachedPlayerCollider(ud, currPlayer, cc, nextPlayer);

        auto bodyID = chCollider->GetBodyID();
        bodyIDsToActivate.push_back(bodyID);
        // [REMINDER] "CharacterVirtual" maintains its own "mLinearVelocity" (https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Physics/Character/CharacterVirtual.h#L709) -- and experimentally setting velocity of its "mInnerBodyID" doesn't work (if "mInnerBodyID" was even set).
    }

    for (int i = 0; i < currRdf->npcs_arr_size(); i++) {
        const NpcCharacterDownsync& currNpc = currRdf->npcs_arr(i);
        if (globalPrimitiveConsts->terminating_character_id() == currNpc.id()) break;
        NpcCharacterDownsync* nextNpc = nextRdf->mutable_npcs_arr(i); // [WARNING] By reaching here, we haven't executed "leftShiftDeadNpcs", hence the indices of "currRdf->npcs_arr" and "nextRdf->npcs_arr" are FULLY ALIGNED.
        const CharacterDownsync& currChd = currNpc.chd();
        auto ud = calcUserData(currNpc);

        const CharacterConfig* cc = getCc(currChd.species_id());
        CH_COLLIDER_T* chCollider = getOrCreateCachedNpcCollider(ud, currNpc, cc, nextNpc);

        auto bodyID = chCollider->GetBodyID();
        bodyIDsToActivate.push_back(bodyID);
    }

    bodyIDsToAdd.clear();
    for (int i = 0; i < currRdf->bullets_size(); i++) {
        const Bullet& currBl = currRdf->bullets(i);
        if (globalPrimitiveConsts->terminating_bullet_id() == currBl.id()) break;
        Bullet* nextBl = nextRdf->mutable_bullets(i); // [WARNING] By reaching here, we haven't executed "leftShiftDeadBullets", hence the indices of "currRdf->bullets" and "nextRdf->bullets" are FULLY ALIGNED.
        auto ud = calcUserData(currBl);
        const Skill* skill = nullptr;
        const BulletConfig* bulletConfig = nullptr;
        FindBulletConfig(currBl.skill_id(), currBl.active_skill_hit(), skill, bulletConfig);
        if (isBulletActive(&currBl, bulletConfig, currRdf->id())) {
            auto blCollider = getOrCreateCachedBulletCollider(ud, bulletConfig->hitbox_half_size_x(), bulletConfig->hitbox_half_size_y(), bulletConfig->b_type());
            auto bodyID = blCollider->GetID();
            transientUdToCurrBl[ud] = &currBl;
            transientUdToNextBl[ud] = nextBl;
            if (!blCollider->IsInBroadPhase()) {
                bodyIDsToAdd.push_back(bodyID);
            }
            bodyIDsToActivate.push_back(bodyID);
        }
/*
#ifndef  NDEBUG
        else if (!isBulletStartingUp(&currBl, bulletConfig, currRdf->id()) && !isBulletExploding(&currBl, bulletConfig)) {

            std::ostringstream oss;
            oss << "@currRdfId=" << currRdfId << ", bulletId=" << currBl.id() << " is no longer active or in startup, originated_render_frame_id=" << currBl.originated_render_frame_id() << " is at bl_state = " << currBl.bl_state() << ", frames_in_bl_state=" << currBl.frames_in_bl_state() << ", pos=(" << currBl.x() << ", " << currBl.y() << ", " << currBl.z() << "), vel = (" << currBl.vel_x() << ", " << currBl.vel_y() << ", " << currBl.vel_z() << ")";
            Debug::Log(oss.str(), DColor::Orange);

        }
#endif // ! NDEBUG
*/
    }

    if (!bodyIDsToAdd.empty()) {
        auto layerState = bi->AddBodiesPrepare(bodyIDsToAdd.data(), bodyIDsToAdd.size());
        bi->AddBodiesFinalize(bodyIDsToAdd.data(), bodyIDsToAdd.size(), layerState, EActivation::DontActivate);
    }

    if (!bodyIDsToActivate.empty()) {
        bi->ActivateBodies(bodyIDsToActivate.data(), bodyIDsToActivate.size());
    }

}

void BaseBattle::batchRemoveFromPhySysAndCache(const int currRdfId, const RenderFrame* currRdf) {
    // This function will remove or deactivate all bodies attached to "phySys", so this mapping cache will certainly become invalid.
    bodyIDsToClear.clear();
    while (!activeChColliders.empty()) {
        CH_COLLIDER_T* single = activeChColliders.back();
        activeChColliders.pop_back();
        auto bodyID = single->GetBodyID();
        uint64_t ud = bi->GetUserData(bodyID);
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
        
        single->SetPositionAndRotation(safeDeactiviatedPosition, Quat::sIdentity(), EActivation::DontActivate, true); // [WARNING] To avoid spurious awakening. See "RuleOfThumb.md" for details.
        single->SetLinearAndAngularVelocity(Vec3::sZero(), Vec3::sZero(), true);
        bodyIDsToClear.push_back(bodyID);
    }

    while (!activeBlColliders.empty()) {
        Body* single = activeBlColliders.back();
        activeBlColliders.pop_back();
        auto ud = single->GetUserData();
        JPH_ASSERT(0 < transientUdToCurrBl.count(ud));
        const Bullet& bl = *(transientUdToCurrBl[ud]);
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

        bi->SetPositionAndRotation(single->GetID()
            , safeDeactiviatedPosition // [WARNING] To avoid spurious awakening. See "RuleOfThumb.md" for details.
            , Quat::sIdentity()
            , EActivation::DontActivate);
        bi->SetLinearAndAngularVelocity(single->GetID(), Vec3::sZero(), Vec3::sZero());
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
    bi->DeactivateBodies(bodyIDsToClear.data(), bodyIDsToClear.size());

    transientUdToChCollider.clear();
    transientUdToBodyID.clear();

    transientUdToCurrPlayer.clear();
    transientUdToNextPlayer.clear();

    transientUdToCurrNpc.clear();
    transientUdToNextNpc.clear();

    transientUdToCurrBl.clear();
    transientUdToNextBl.clear();

    transientUdToCurrTrap.clear();
    transientUdToNextTrap.clear();
}

void BaseBattle::derivePlayerOpPattern(const int rdfId, const CharacterDownsync& currChd, const CharacterConfig* cc, CharacterDownsync* nextChd, bool currEffInAir, bool notDashing, const InputFrameDecoded& ioIfDecoded, int& outPatternId, bool& outJumpedOrNot, bool& outSlipJumpedOrNot, int& outEffDx, int& outEffDy) {
    outJumpedOrNot = false;
    outSlipJumpedOrNot = false;
    outEffDx = 0;
    outEffDy = 0;

    updateBtnHoldingByInput(currChd, ioIfDecoded, nextChd);

    // Jumping is partially allowed within "CapturedByInertia", but moving is only allowed when "0 == frames_to_recover()" (constrained later in "Step")
    if (0 >= currChd.frames_to_recover()) {
        outEffDx = ioIfDecoded.dx();
        outEffDy = ioIfDecoded.dy();
    } else if (!currEffInAir && 1 >= currChd.frames_to_recover() && 0 > ioIfDecoded.dy() && cc->crouching_enabled()) {
        // to favor smooth crouching transition
        outEffDx = ioIfDecoded.dx();
        outEffDy = ioIfDecoded.dy();
    } else if (WalkingAtk1 == currChd.ch_state()) {
        outEffDx = ioIfDecoded.dx();
    } else if (isInBlockStun(currChd)) {
        // Reserve only "effDy" for later use by "useSkill", e.g. to break free from block-stun by certain skills.
        outEffDy = ioIfDecoded.dy();
    }

    outPatternId = globalPrimitiveConsts->pattern_id_no_op();
    int effFrontOrBack = (ioIfDecoded.dx() * currChd.dir_x()); // [WARNING] Deliberately using "ifDecoded.dx()" instead of "effDx (which could be 0 in block stun)" here!
    bool canJumpWithinInertia = (0 == currChd.frames_to_recover() && ((cc->inertia_frames_to_recover() >> 1) > currChd.frames_captured_by_inertia())) || !notDashing;
    canJumpWithinInertia |= (WalkingAtk1 == currChd.ch_state() || WalkingAtk4 == currChd.ch_state());
    if (0 < ioIfDecoded.btn_a_level()) {
        if (0 == currChd.btn_a_holding_rdf_cnt() && canJumpWithinInertia) {
            if ((currChd.primarily_on_slippable_hard_pushback() || (currEffInAir && currChd.omit_gravity() && !cc->omit_gravity())) && (0 > ioIfDecoded.dy() && 0 == ioIfDecoded.dx())) {
                outSlipJumpedOrNot = true;
            } else if ((!currEffInAir || 0 < currChd.remaining_air_jump_quota()) && (!isCrouching(currChd.ch_state(), cc) || !notDashing)) {
                outJumpedOrNot = true;
            } else if (OnWallIdle1 == currChd.ch_state()) {
                outJumpedOrNot = true;
            }
        }
    }

    if (globalPrimitiveConsts->pattern_id_no_op() == outPatternId) {
        if (0 < ioIfDecoded.btn_b_level()) {
            if (0 == currChd.btn_b_holding_rdf_cnt()) {
                if (0 < ioIfDecoded.btn_c_level()) {
                    outPatternId = globalPrimitiveConsts->pattern_inventory_slot_bc();
                } else if (0 > ioIfDecoded.dy()) {
                    outPatternId = globalPrimitiveConsts->pattern_down_b();
                } else if (0 < ioIfDecoded.dy()) {
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
        if (0 < ioIfDecoded.btn_e_level() && (cc->dashing_enabled() || cc->sliding_enabled())) {
            if (0 == currChd.btn_e_holding_rdf_cnt()) {
                if (notDashing) {
                    if (0 < effFrontOrBack) {
                        outPatternId = (globalPrimitiveConsts->pattern_hold_b() == outPatternId ? globalPrimitiveConsts->pattern_front_e_hold_b() : globalPrimitiveConsts->pattern_front_e());
                    } else if (0 > effFrontOrBack) {
                        outPatternId = (globalPrimitiveConsts->pattern_hold_b() == outPatternId ? globalPrimitiveConsts->pattern_back_e_hold_b() : globalPrimitiveConsts->pattern_back_e());
                        outEffDx = 0; // [WARNING] Otherwise the character will turn around
                    } else if (0 > ioIfDecoded.dy()) {
                        outPatternId = (globalPrimitiveConsts->pattern_hold_b() == outPatternId ? globalPrimitiveConsts->pattern_down_e_hold_b() : globalPrimitiveConsts->pattern_down_e());
                    } else if (0 < ioIfDecoded.dy()) {
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
        if (0 < ioIfDecoded.btn_c_level()) {
            if (0 == currChd.btn_c_holding_rdf_cnt()) {
                outPatternId = globalPrimitiveConsts->pattern_inventory_slot_c();
                if (0 < ioIfDecoded.btn_b_level()) {
                    outPatternId = globalPrimitiveConsts->pattern_inventory_slot_bc();
                } else {
                    outPatternId = globalPrimitiveConsts->pattern_hold_inventory_slot_c();
                    if (0 < ioIfDecoded.btn_b_level() && 0 == currChd.btn_b_holding_rdf_cnt()) {
                        outPatternId = globalPrimitiveConsts->pattern_inventory_slot_bc();
                    }
                }
            } else if (0 < ioIfDecoded.btn_d_level()) {
                if (0 == currChd.btn_d_holding_rdf_cnt()) {
                    outPatternId = globalPrimitiveConsts->pattern_inventory_slot_d();
                } else {
                    outPatternId = globalPrimitiveConsts->pattern_hold_inventory_slot_d();
                }
            }
        }
    }
}

void BaseBattle::deriveNpcOpPattern(int rdfId, const CharacterDownsync& currChd, const CharacterConfig* cc, bool currEffInAir, bool notDashing, const InputFrameDecoded& ifDecoded, int& outPatternId, bool& outJumpedOrNot, bool& outSlipJumpedOrNot, int& outEffDx, int& outEffDy) {
    // TODO
}

std::vector<std::vector<int>> DIRECTION_DECODER = {
    { 0, 0 }, // 0
    { 0, +2 }, // 1
    { 0, -2 }, // 2
    { +2, 0 }, // 3
    { -2, 0 }, // 4
    { +1, +1 }, // 5
    { -1, -1 }, // 6
    { +1, -1 }, // 7
    { -1, +1 }, // 8
    /* The rest is for safe access from malicious inputs */
    { 0, 0 }, // 9
    { 0, 0 }, // 10
    { 0, 0 }, // 11
    { 0, 0 }, // 12
    { 0, 0 }, // 13
    { 0, 0 }, // 14
    { 0, 0 }, // 15
};

bool BaseBattle::decodeInput(uint64_t encodedInput, InputFrameDecoded* holder) {
    holder->Clear();
    int encodedDirection = (int)(encodedInput & 15);
    int btnALevel = (int)((encodedInput >> 4) & 1);
    int btnBLevel = (int)((encodedInput >> 5) & 1);
    int btnCLevel = (int)((encodedInput >> 6) & 1);
    int btnDLevel = (int)((encodedInput >> 7) & 1);
    int btnELevel = (int)((encodedInput >> 8) & 1);

    holder->set_dx(DIRECTION_DECODER[encodedDirection][0]);
    holder->set_dy(DIRECTION_DECODER[encodedDirection][1]);
    holder->set_btn_a_level(btnALevel);
    holder->set_btn_b_level(btnBLevel);
    holder->set_btn_c_level(btnCLevel);
    holder->set_btn_d_level(btnDLevel);
    holder->set_btn_e_level(btnELevel);
    return true;
}

void BaseBattle::processPlayerInputs(const int currRdfId, const RenderFrame* currRdf, float dt, RenderFrame* nextRdf, const int delayedIfdId, const InputFrameDownsync* delayedIfd) {
    for (int i = 0; i < playersCnt; i++) {
        const PlayerCharacterDownsync& currPlayer = currRdf->players_arr(i);
        const CharacterDownsync& currChd = currPlayer.chd();
        const CharacterConfig* cc = getCc(currChd.species_id());
        auto currChState = currChd.ch_state();
        bool currNotDashing = isNotDashing(currChd);
        bool currDashing = !currNotDashing;
        bool currWalking = walkingSet.count(currChState);
        bool currEffInAir = isEffInAir(currChd, currNotDashing);
        bool currOnWall = onWallSet.count(currChState);
        bool currCrouching = isCrouching(currChState, cc);
        bool currAtked = noOpSet.count(currChState);
        bool currInBlockStun = isInBlockStun(currChd);
        bool currParalyzed = false; // TODO

        PlayerCharacterDownsync* nextPlayer = nextRdf->mutable_players_arr(i);
        CharacterDownsync* nextChd = nextPlayer->mutable_chd();
        auto ud = calcUserData(currPlayer);
        auto cv = transientUdToChCollider[ud];

        int patternId = globalPrimitiveConsts->pattern_id_no_op();
        bool jumpedOrNot = false;
        bool slipJumpedOrNot = false;
        int effDx = 0, effDy = 0;
        uint64_t singleInput = delayedIfd->input_list(i);
        decodeInput(singleInput, &ifDecodedHolder);
        derivePlayerOpPattern(currRdfId, currChd, cc, nextChd, currEffInAir, currNotDashing, ifDecodedHolder, patternId, jumpedOrNot, slipJumpedOrNot, effDx, effDy);
        bool slowDownToAvoidOverlap = false;
        processSingleCharacterInput(currRdfId, dt, patternId, jumpedOrNot, slipJumpedOrNot, effDx, effDy, slowDownToAvoidOverlap, currChd, ud, currEffInAir, currCrouching, currOnWall, currDashing, currWalking, currInBlockStun, currAtked, currParalyzed, cc, nextChd, nextRdf);
    }
}

void BaseBattle::processNpcInputs(const int currRdfId, const RenderFrame* currRdf, float dt, RenderFrame* nextRdf, const int delayedIfdId, const InputFrameDownsync* delayedIfd) {
    for (int i = 0; i < currRdf->npcs_arr_size(); i++) {
        const NpcCharacterDownsync& currNpc = currRdf->npcs_arr(i);
        if (globalPrimitiveConsts->terminating_character_id() == currNpc.id()) break;
        const CharacterDownsync& currChd = currNpc.chd();
        const CharacterConfig* cc = getCc(currChd.species_id());
        auto currChState = currChd.ch_state();
        bool currNotDashing = isNotDashing(currChd);
        bool currDashing = !currNotDashing;
        bool currWalking = walkingSet.count(currChState);
        bool currEffInAir = isEffInAir(currChd, currNotDashing);
        bool currOnWall = onWallSet.count(currChState);
        bool currCrouching = isCrouching(currChState, cc);
        bool currAtked = noOpSet.count(currChState);
        bool currInBlockStun = isInBlockStun(currChd);
        bool currParalyzed = false; // TODO

        NpcCharacterDownsync* nextNpc = nextRdf->mutable_npcs_arr(i);
        CharacterDownsync* nextChd = nextNpc->mutable_chd();
        auto ud = calcUserData(currNpc);
        auto cv = transientUdToChCollider[ud];

        int patternId = globalPrimitiveConsts->pattern_id_no_op();
        bool jumpedOrNot = false;
        bool slipJumpedOrNot = false;
        int effDx = 0, effDy = 0;
        deriveNpcOpPattern(currRdfId, currChd, cc, currEffInAir, currNotDashing, ifDecodedHolder, patternId, jumpedOrNot, slipJumpedOrNot, effDx, effDy);

        bool slowDownToAvoidOverlap = false; // TODO
        processSingleCharacterInput(currRdfId, dt, patternId, jumpedOrNot, slipJumpedOrNot, effDx, effDy, slowDownToAvoidOverlap, currChd, ud, currEffInAir, currCrouching, currOnWall, currDashing, currWalking, currInBlockStun, currAtked, currParalyzed, cc, nextChd, nextRdf);
        /*
        if (usedSkill) {
            nextNpc->set_cached_cue_cmd(0);
        }
        */
    }
}

void BaseBattle::processSingleCharacterInput(int rdfId, float dt, int patternId, bool jumpedOrNot, bool slipJumpedOrNot, int effDx, int effDy, bool slowDownToAvoidOverlap, const CharacterDownsync& currChd, uint64_t ud, bool currEffInAir, bool currCrouching, bool currOnWall, bool currDashing, bool currWalking, bool currInBlockStun, bool currAtked, bool currParalyzed, const CharacterConfig* cc, CharacterDownsync* nextChd, RenderFrame* nextRdf) {
    bool slotUsed = false;
    uint32_t slotLockedSkillId = globalPrimitiveConsts->no_skill();
    bool dodgedInBlockStun = false;
    // TODO: Call "useInventorySlot" properly 

    nextChd->set_jump_triggered(jumpedOrNot);
    nextChd->set_slip_jump_triggered(nextChd->slip_jump_triggered() || slipJumpedOrNot);

    if (globalPrimitiveConsts->jump_holding_rdf_cnt_threshold_2() > currChd.btn_a_holding_rdf_cnt() && globalPrimitiveConsts->jump_holding_rdf_cnt_threshold_2() <= nextChd->btn_a_holding_rdf_cnt() && !currChd.omit_gravity() && cc->jump_holding_to_fly() && proactiveJumpingSet.count(currChd.ch_state())) {
        /*
           (a.) The original "hold-only-to-fly" is prone to "falsely predicted flying" due to not being edge-triggered;
           (b.) However, "releasing BtnA at globalPrimitiveConsts->jump_holding_rdf_cnt_threshold_2() <= currChd.btn_a_holding_rdf_cnt()" makes it counter-intuitive to use when playing, the trade-off is not easy for me...
         */
        nextChd->set_omit_gravity(true);
        nextChd->set_primarily_on_slippable_hard_pushback(false);
        nextChd->set_flying_rdf_countdown(cc->flying_quota_rdf_cnt());
        if (0 >= nextChd->vel_y()) {
            nextChd->set_vel_y(0);
        }
    }

    if (0 < currChd.debuff_list_size()) {
        auto existingDebuff = currChd.debuff_list(globalPrimitiveConsts->debuff_arr_idx_elemental());
        if (globalPrimitiveConsts->terminating_debuff_species_id() != existingDebuff.species_id()) {
            auto existingDebufConfig = globalConfigConsts->debuff_configs().at(existingDebuff.species_id());
            currParalyzed = (globalPrimitiveConsts->terminating_debuff_species_id() != existingDebuff.species_id() && 0 < existingDebuff.stock() && DebuffType::PositionLockedOnly == existingDebufConfig.type());
        }
    }
    if (dodgedInBlockStun) {
        transitToGroundDodgedChState(nextChd, cc, currParalyzed);
    }

    int outSkillId = globalPrimitiveConsts->no_skill();
    const Skill* outSkillConfig = nullptr;
    const BulletConfig* outPivotBc = nullptr;

    bool usedSkill = dodgedInBlockStun ? false : useSkill(rdfId, nextRdf, currChd, ud, cc, nextChd, effDx, effDy, patternId, currEffInAir, currCrouching, currOnWall, currDashing, currWalking, currInBlockStun, currAtked, currParalyzed, outSkillId, outSkillConfig, outPivotBc);

    /*
    if (cc->btn_b_auto_unhold_ch_states().contains(nextChd->ch_state())) {
        // [WARNING] For "autofire" skills.
        nextChd->set_btn_b_holding_rdf_cnt(0);
    }
    */

    if (usedSkill) {
        nextChd->set_frames_captured_by_inertia(0); // The use of a skill should break "CapturedByInertia"
        resetJumpStartup(nextChd);

        if (Dashing == outSkillConfig->bound_ch_state() && currEffInAir) {
            if (!currChd.omit_gravity() && 0 < nextChd->remaining_air_dash_quota()) {
                nextChd->set_remaining_air_dash_quota(nextChd->remaining_air_dash_quota() - 1);
                if (!cc->isolated_air_jump_and_dash_quota() && 0 < nextChd->remaining_air_jump_quota()) {
                    nextChd->set_remaining_air_jump_quota(nextChd->remaining_air_jump_quota() - 1);
                }
            }
        }
        if (isCrouching(currChd.ch_state(), cc) && Atk1 == nextChd->ch_state()) {
            if (cc->crouching_atk_enabled()) {
                nextChd->set_ch_state(CrouchAtk1);
            }
        }
/*
#ifndef NDEBUG
        std::ostringstream oss1;
        oss1 << "@rdfId=" << rdfId << ", set nextChd ch_state=" << nextChd->ch_state() << " and frames_in_ch_state=" << nextChd->frames_in_ch_state();
        Debug::Log(oss1.str(), DColor::Orange);
#endif // !NDEBUG
*/
        return; // Don't allow movement if skill is used
    }

    prepareJumpStartup(rdfId, currChd, nextChd, currEffInAir, cc, currParalyzed);

    // [WARNING] This is a necessary cleanup before "processInertiaWalking"!
    if (1 == currChd.frames_to_recover() && 0 == nextChd->frames_to_recover() && (Atked1 == currChd.ch_state() || InAirAtked1 == currChd.ch_state() || CrouchAtked1 == currChd.ch_state())) {
        nextChd->set_vel_x(0);
        nextChd->set_vel_y(0);
    }

    if (!currChd.omit_gravity() && !cc->omit_gravity()) {
        processInertiaWalking(rdfId, dt, currChd, nextChd, currEffInAir, effDx, effDy, cc, currParalyzed, currInBlockStun);
    } else {
        processInertiaFlying(rdfId, dt, currChd, nextChd, effDx, effDy, cc, currParalyzed, currInBlockStun);
    }
    bool nextNotDashing = isNotDashing(*nextChd);
    bool nextEffInAir = isEffInAir(*nextChd, nextNotDashing);
    processDelayedBulletSelfVel(rdfId, currChd, nextChd, cc, currParalyzed, nextEffInAir);

    processJumpStarted(rdfId, currChd, nextChd, currEffInAir, cc);

    if (globalPrimitiveConsts->pattern_id_unable_to_op() != patternId && cc->anti_gravity_when_idle() && (Walking == nextChd->ch_state() || InAirWalking == nextChd->ch_state()) && cc->anti_gravity_frames_lingering() < nextChd->frames_in_ch_state()) {
        nextChd->set_ch_state(InAirIdle1NoJump);
        nextChd->set_frames_in_ch_state(0);
        nextChd->set_vel_x(0);
    } else if (slowDownToAvoidOverlap) {
        nextChd->set_vel_x(nextChd->vel_x() * 0.25);
        nextChd->set_vel_y(nextChd->vel_y() * 0.25);
    }
}

void BaseBattle::FindBulletConfig(uint32_t skillId, uint32_t skillHit, const Skill*& outSkill, const BulletConfig*& outBulletConfig) {
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

void BaseBattle::processDelayedBulletSelfVel(int rdfId, const CharacterDownsync& currChd, CharacterDownsync* nextChd, const CharacterConfig* cc, bool currParalyzed, bool nextEffInAir) {
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
    if (currChd.frames_in_ch_state() != bulletConfig->startup_frames()) {
        return;
    }
    if (currParalyzed) {
        return;
    }
    int xfac = (0 < currChd.dir_x() ? 1 : -1);
    if (globalPrimitiveConsts->no_lock_vel() != bulletConfig->self_lock_vel_x()) {
        nextChd->set_vel_x(xfac * bulletConfig->self_lock_vel_x());
    }
    if (globalPrimitiveConsts->no_lock_vel() != bulletConfig->self_lock_vel_y()) {
        if (0 <= bulletConfig->self_lock_vel_y() || nextEffInAir) {
            nextChd->set_vel_y(bulletConfig->self_lock_vel_y());
        }
    }
}

void BaseBattle::postStepSingleChdStateCorrection(const int currRdfId, const uint64_t udt, const uint64_t ud, const CH_COLLIDER_T* chCollider, const CharacterDownsync& currChd, CharacterDownsync* nextChd, const CharacterConfig* cc, bool cvSupported, bool cvInAir, bool cvOnWall, bool currNotDashing, bool currEffInAir, bool oldNextNotDashing, bool oldNextEffInAir, bool inJumpStartupOrJustEnded, CharacterBase::EGroundState cvGroundState) {
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
            case TurnAround:
                if (Walking == oldNextChState) {
                    if (cc->omit_gravity()) {
                        // [WARNING] No need to distinguish in this case.
                        break;
                    } else if (nextChd->omit_gravity()) {
                        nextChd->set_ch_state(InAirWalking);
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
                if ((OnWallIdle1 == currChd.ch_state() && currChd.jump_triggered()) || InAirIdle1ByWallJump == currChd.ch_state()) {
                    nextChd->set_ch_state(InAirIdle1ByWallJump);
                } else if (currChd.jump_triggered() || InAirIdle1ByJump == currChd.ch_state()) {
                    nextChd->set_ch_state(InAirIdle1ByJump);
                } else if (currChd.jump_triggered() || InAirIdle2ByJump == currChd.ch_state()) {
                    nextChd->set_ch_state(InAirIdle2ByJump);
                } else {
                    nextChd->set_ch_state(InAirIdle1NoJump);
                }
                if (Def1 == oldNextChState) {
                    nextChd->set_remaining_def1_quota(0);
                }
                break;
            case WalkStopping:
                if (cc->omit_gravity()) {
                    // [WARNING] No need to distinguish in this case.
                    break;
                } else if (nextChd->omit_gravity()) {
                    nextChd->set_ch_state(cc->has_in_air_walk_stopping_anim() ? InAirWalkStopping : InAirWalking);
                    break;
                }
                break;
            case Atk1:
                nextChd->set_ch_state(InAirAtk1);
                // No inAir transition for ATK2/ATK3 for now
                break;
            case Atked1:
                nextChd->set_ch_state(InAirAtked1);
                break;
            }
        }
    } else {
        // next frame NOT in air
        if (oldNextEffInAir && BlownUp1 != oldNextChState && OnWallIdle1 != oldNextChState && Dashing != oldNextChState) {
            switch (oldNextChState) {
            case InAirIdle1NoJump:
            case InAirIdle2ByJump:
                nextChd->set_ch_state(Idle1);
                break;
            case InAirIdle1ByJump:
            case InAirIdle1ByWallJump:
                if (isJumpStartupJustEnded(currChd, nextChd, cc) || isInJumpStartup(*nextChd, cc)) {
                    // [WARNING] Don't change CharacterState in this special case!
                    break;
                }
                nextChd->set_ch_state(Idle1);
                break;
            case InAirAtked1:
                nextChd->set_ch_state(Atked1);
                break;
            case InAirAtk1:
            case InAirAtk2:
                if (nullptr != activeBulletConfig && activeBulletConfig->remains_upon_hit()) {
                    nextChd->set_frames_to_recover(currChd.frames_to_recover() - 1);
                }
                break;
            case Def1:
            case Def1Broken:
                // Not changing anything.
                break;
            default:
                nextChd->set_ch_state(Idle1);
                break;
            }
        } else if (nextChd->forced_crouching() && cc->crouching_enabled() && !isCrouching(oldNextChState, cc)) {
            switch (oldNextChState) {
            case Idle1:
            case InAirIdle1ByJump:
            case InAirIdle2ByJump:
            case InAirIdle1NoJump:
            case InAirIdle1ByWallJump:
            case Walking:
            case GetUp1:
            case TurnAround:
                nextChd->set_ch_state(CrouchIdle1);
                break;
            case Atk1:
            case Atk2:
                if (cc->crouching_atk_enabled()) {
                    nextChd->set_ch_state(CrouchAtk1);
                } else {
                    nextChd->set_ch_state(CrouchIdle1);
                }
                break;
            case Atked1:
            case InAirAtked1:
                nextChd->set_ch_state(CrouchAtked1);
                break;
            case BlownUp1:
            case LayDown1:
            case Dying:
                break;
            /*
            default:
                // TODO
                throw new ArgumentException(String.Format("At rdf.Id={0}, unable to force crouching for character {1}", currRenderFrame.Id, i < roomCapacity ? stringifyPlayer(nextChd) : stringifyNpc(nextChd) ));
            */
            }
        }
    }

    // Reset "FramesInChState" if "CharacterState" is changed
    if (nextChd->ch_state() != currChd.ch_state()) {
        if (Walking == currChd.ch_state() && (WalkingAtk1 == nextChd->ch_state() || WalkingAtk4 == nextChd->ch_state())) {
            uint32_t nextActiveSkillId = nextChd->active_skill_id();
            int nextActiveSkillHit = nextChd->active_skill_hit();
            const Skill* nextActiveSkill = nullptr;
            const BulletConfig* nextActiveBulletConfig = nullptr;
            FindBulletConfig(nextActiveSkillId, nextActiveSkillHit, nextActiveSkill, nextActiveBulletConfig);
            JPH_ASSERT(nullptr != nextActiveSkill && nullptr != nextActiveBulletConfig);
            int newFramesInChState = currChd.frames_in_ch_state() + 1;
            newFramesInChState %= nextActiveSkill->recovery_frames(); // [TODO] Enhance this for GUI smoothness.
            nextChd->set_frames_in_ch_state(newFramesInChState);
        } else if ((WalkingAtk1 == currChd.ch_state() || WalkingAtk4 == currChd.ch_state()) && Walking == nextChd->ch_state()) {
            nextChd->set_frames_in_ch_state(currChd.frames_in_ch_state() + 1);
        } else if ((Atk1 == currChd.ch_state() && WalkingAtk1 == nextChd->ch_state()) || (Atk4 == currChd.ch_state() && WalkingAtk4 == nextChd->ch_state())) {
            nextChd->set_frames_in_ch_state(currChd.frames_in_ch_state() + 1);
        } else if ((WalkingAtk1 == currChd.ch_state() && Atk1 == nextChd->ch_state()) || (WalkingAtk4 == currChd.ch_state() && Atk4 == nextChd->ch_state())) {
            nextChd->set_frames_in_ch_state(currChd.frames_in_ch_state() + 1);
        } else {
            bool isAtk1Transition = (Atk1 == currChd.ch_state() && InAirAtk1 == nextChd->ch_state()) || (InAirAtk1 == currChd.ch_state() && Atk1 == nextChd->ch_state());
            bool isAtked1Transition = (Atked1 == currChd.ch_state() && InAirAtked1 == nextChd->ch_state()) || (InAirAtked1 == currChd.ch_state() && Atked1 == nextChd->ch_state());
            if (!isAtk1Transition && !isAtked1Transition) {
                nextChd->set_frames_in_ch_state(0);
            }
        }
    }

    // Remove any active skill if not attacking
    bool notDashing = isNotDashing(*nextChd);
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
        resetJumpStartup(nextChd);
    } else if (onWallSet.count(nextChd->ch_state()) && !onWallSet.count(currChd.ch_state())) {
        nextChd->set_remaining_air_jump_quota(cc->default_air_jump_quota());
        nextChd->set_remaining_air_dash_quota(cc->default_air_dash_quota());
        resetJumpStartup(nextChd);
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

void BaseBattle::leftShiftDeadNpcs(int currRdfId, RenderFrame* nextRdf) {
    int aliveSlotI = 0, candidateI = 0;
    auto characterConfigs = globalConfigConsts->character_configs();
    while (candidateI < nextRdf->npcs_arr_size()) {
        auto candidate = nextRdf->npcs_arr(candidateI);
        if (globalPrimitiveConsts->terminating_character_id() == candidate.id()) break;
        auto chd = candidate.chd();
        auto candidateConfig = characterConfigs.at(chd.species_id());

        /*
        // TODO: Drop pickable
        if (isNpcJustDead(candidate)) {
            if ((TERMINATING_CONSUMABLE_SPECIES_ID != candidate.KilledToDropConsumableSpeciesId || TERMINATING_BUFF_SPECIES_ID != candidate.KilledToDropBuffSpeciesId || NO_SKILL != candidate.KilledToDropPickupSkillId)) {
                addNewPickableToNextFrame(rdfId, candidate.VirtualGridX, candidate.VirtualGridY, 0, +1, MAX_INT, 0, true, MAX_UINT, MAX_UINT, (NO_SKILL == candidate.KilledToDropPickupSkillId ? PickupType.Immediate : PickupType.PutIntoInventory), 1, nextRenderFramePickables, candidate.KilledToDropConsumableSpeciesId, candidate.KilledToDropBuffSpeciesId, candidate.KilledToDropPickupSkillId, ref pickableLocalIdCounter, ref nextRdfPickableCnt);
            }
        }
        */
        
        while (candidateI < nextRdf->npcs_arr_size() && globalPrimitiveConsts->terminating_character_id() != candidate.id() && isNpcDeadToDisappear(&chd)) {
            if (globalPrimitiveConsts->terminating_trigger_id() != candidate.publishing_to_trigger_id_upon_killed()) {
                auto targetTriggerInNextFrame = nextRdf->mutable_triggers_arr(candidate.publishing_to_trigger_id_upon_killed()-1);
                if (0 == chd.last_damaged_by_ud() && globalPrimitiveConsts->terminating_bullet_team_id() == chd.last_damaged_by_bullet_team_id()) {
                    // TODO
                    if (1 == playersCnt) {
                        publishNpcKilledEvt(currRdfId, candidate.publishing_evt_mask_upon_killed(), 1, 1, targetTriggerInNextFrame);
                    } else {
#ifndef  NDEBUG
                        // Debug::Log("@rdfId=" + rdfId + " publishing evtMask=" + candidate.PublishingEvtMaskUponKilled + " to trigger " + targetTriggerInNextFrame.ToString() + " with no join index and no bullet team id!");
#endif // ! NDEBUG
                        publishNpcKilledEvt(currRdfId, candidate.publishing_evt_mask_upon_killed(), chd.last_damaged_by_ud(), chd.last_damaged_by_bullet_team_id(), targetTriggerInNextFrame);
                    }
                } else {
                    publishNpcKilledEvt(currRdfId, candidate.publishing_evt_mask_upon_killed(), chd.last_damaged_by_ud(), chd.last_damaged_by_bullet_team_id(), targetTriggerInNextFrame);
                }
            }
            candidateI++;
        }

        if (candidateI >= nextRdf->npcs_arr_size() || globalPrimitiveConsts->terminating_character_id() == nextRdf->npcs_arr(candidateI).id()) {
            break;
        }

        if (candidateI != aliveSlotI) {
            auto src = nextRdf->npcs_arr(candidateI);
            auto dst = nextRdf->mutable_npcs_arr(aliveSlotI);
            dst->Clear();
            dst->CopyFrom(src);
        }

        candidateI++;
        aliveSlotI++;
    }
    if (aliveSlotI < nextRdf->npcs_arr_size()) {
        nextRdf->mutable_npcs_arr(aliveSlotI)->set_id(globalPrimitiveConsts->terminating_character_id());
    }
    nextRdf->set_npc_count(aliveSlotI);
}

void BaseBattle::calcFallenDeath(const RenderFrame* currRdf, RenderFrame* nextRdf) {
    auto chConfigs = globalConfigConsts->character_configs();

    for (int i = 0; i < nextRdf->players_arr_size(); i++) {
        auto& currPlayer = currRdf->players_arr(i);
        auto nextPlayer = nextRdf->mutable_players_arr(i);
        auto chd = nextPlayer->mutable_chd();
        auto chConfig = chConfigs.at(chd->species_id());
        float chTop = chd->y() + 2*chConfig.capsule_half_height();
        if (fallenDeathHeight > chTop && Dying != chd->ch_state()) {
#ifndef NDEBUG
                std::ostringstream oss;
                oss << "@currRdfId=" << currRdf->id() << ", player joinIndex=" << currPlayer.join_index() << " is dead due to fallen death with chTop=" << chTop << ", fallenDeathHeight=" << fallenDeathHeight << ", will transit into Dying";
                Debug::Log(oss.str(), DColor::Orange);
#endif
            transitToDying(currPlayer, nextPlayer);
        }
    }

    for (int i = 0; i < nextRdf->npcs_arr_size(); i++) {
        auto& currNpc = currRdf->npcs_arr(i);
        if (globalPrimitiveConsts->terminating_character_id() == currNpc.id()) break;
        auto nextNpc = nextRdf->mutable_npcs_arr(i);
        auto chd = nextNpc->mutable_chd();
        auto chConfig = chConfigs.at(chd->species_id());
        float chTop = chd->y() + 2 * chConfig.capsule_half_height();
        if (fallenDeathHeight > chTop && Dying != chd->ch_state()) {
            transitToDying(currNpc, nextNpc);
        }
    }

    for (int i = 0; i < nextRdf->pickables_size(); i++) {
        auto pickable = nextRdf->mutable_pickables(i);
        if (globalPrimitiveConsts->terminating_pickable_id() == pickable->id()) break;
        auto pickableTop = pickable->y() + globalPrimitiveConsts->default_pickable_hitbox_half_size_y();
        if (fallenDeathHeight > pickableTop && PickableState::PIdle == pickable->pk_state()) {
            pickable->set_pk_state(PickableState::PDisappearing);
            pickable->set_frames_in_pk_state(0);
            pickable->set_remaining_lifetime_rdf_count(globalPrimitiveConsts->default_pickable_disappearing_anim_frames());
        }
    }
}

void BaseBattle::leftShiftDeadBullets(int currRdfId, RenderFrame* nextRdf) {
    int aliveSlotI = 0, candidateI = 0;
    while (candidateI < nextRdf->bullets_size()) {
        const Bullet* candidate = &(nextRdf->bullets(candidateI));
        if (globalPrimitiveConsts->terminating_bullet_id() == candidate->id()) {
            break;
        }
        uint32_t skillId = candidate->skill_id();
        int skillHit = candidate->active_skill_hit();
        const Skill* skillConfig = nullptr;
        const BulletConfig* bulletConfig = nullptr;
        FindBulletConfig(skillId, skillHit, skillConfig, bulletConfig);

        while (candidateI < nextRdf->bullets_size() && globalPrimitiveConsts->terminating_bullet_id() != candidate->id() && !isBulletAlive(candidate, bulletConfig, currRdfId)) {
#ifndef NDEBUG
            auto bulletId = candidate->id();
            std::ostringstream oss1;
            oss1 << "@currRdfId=" << currRdfId << ", bulletId=" << bulletId << " is dead and left shifted, bl_state=" << candidate->bl_state() << ", frames_in_bl_state=" << candidate->frames_in_bl_state();
            Debug::Log(oss1.str(), DColor::Orange);
#endif // !NDEBUG
            candidateI++;
            candidate = &(nextRdf->bullets(candidateI));
        }

        if (candidateI >= nextRdf->bullets_size() || globalPrimitiveConsts->terminating_bullet_id() == nextRdf->bullets(candidateI).id()) {
            break;
        }

        if (candidateI != aliveSlotI) {
            auto src = nextRdf->bullets(candidateI);
            auto dst = nextRdf->mutable_bullets(aliveSlotI);
            dst->Clear();
            dst->CopyFrom(src);
        }

        candidateI++;
        aliveSlotI++;
    }
    if (aliveSlotI < nextRdf->bullets_size()) {
        nextRdf->mutable_bullets(aliveSlotI)->set_id(globalPrimitiveConsts->terminating_bullet_id());
    }
    nextRdf->set_bullet_count(aliveSlotI);
}

void BaseBattle::leftShiftDeadPickables(int currRdfId, RenderFrame* nextRdf) {
    int aliveSlotI = 0, candidateI = 0;
    while (candidateI < nextRdf->pickables_size()) {
        auto candidate = nextRdf->pickables(candidateI);
        if (globalPrimitiveConsts->terminating_pickable_id() == candidate.id()) {
            break;
        }
        while (candidateI < nextRdf->pickables_size() && globalPrimitiveConsts->terminating_pickable_id() != candidate.id() && !isPickableAlive(&candidate, currRdfId)) {
            // TODO: Handle self recurring pickables
            candidateI++;
        }

        if (candidateI >= nextRdf->pickables_size() || globalPrimitiveConsts->terminating_pickable_id() == nextRdf->pickables(candidateI).id()) {
            break;
        }

        if (candidateI != aliveSlotI) {
            auto src = nextRdf->bullets(candidateI);
            auto dst = nextRdf->mutable_bullets(aliveSlotI);
            dst->Clear();
            dst->CopyFrom(src);
        }

        candidateI++;
        aliveSlotI++;
    }
    if (aliveSlotI < nextRdf->pickables_size()) {
        nextRdf->mutable_pickables(aliveSlotI)->set_id(globalPrimitiveConsts->terminating_pickable_id());
    }
    nextRdf->set_pickable_count(aliveSlotI);
}

bool BaseBattle::useSkill(int rdfId, RenderFrame* nextRdf, const CharacterDownsync& currChd, uint64_t ud, const CharacterConfig* cc, CharacterDownsync* nextChd, int effDx, int effDy, int patternId, bool currEffInAir, bool currCrouching, bool currOnWall, bool currDashing, bool currWalking, bool currInBlockStun, bool currAtked, bool currParalyzed, int& outSkillId, const Skill*& outSkill, const BulletConfig*& outPivotBc) {
    if (globalPrimitiveConsts->pattern_id_no_op() == patternId || globalPrimitiveConsts->pattern_id_unable_to_op() == patternId) {
        return false;
    }
    bool notRecovered = (0 < currChd.frames_to_recover());
    if (CharacterState::Parried == currChd.ch_state()) {
        notRecovered = (currChd.frames_in_ch_state() >= globalPrimitiveConsts->parried_frames_to_start_cancellable());
    }

    const ::google::protobuf::Map<uint32, Skill>& skillConfigs = globalConfigConsts->skill_configs();
    const Skill* activeSkillConfig = nullptr;
    const BulletConfig* activeBulletConfig = nullptr;
    int currActiveSkillId = currChd.active_skill_id();
    int currActiveSkillHit = currChd.active_skill_hit();
    int targetSkillId = globalPrimitiveConsts->no_skill();
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

        int encodedPattern = EncodePatternForCancelTransit(patternId, currEffInAir, currCrouching, currOnWall, currDashing, currWalking);
        auto cancelTransitDict = activeBulletConfig->cancel_transit();
        if (!cancelTransitDict.count(encodedPattern)) {
            return false;
        }
        targetSkillId = cancelTransitDict[encodedPattern];
    } else {
        if (notRecovered) {
            return false;
        }
        int encodedPattern = EncodePatternForInitSkill(patternId, currEffInAir, currCrouching, currOnWall, currDashing, currWalking, currInBlockStun, currAtked, currParalyzed);
/*
#ifndef NDEBUG
        std::ostringstream oss1;
        oss1 << "@rdfId=" << rdfId << ", ud=" << ud << " tries to use init skill by (patternId=" << patternId << ", effDx=" << effDx << ", effDy=" << effDy << ", encodedPattern=" << encodedPattern << ")";
        Debug::Log(oss1.str(), DColor::Orange);
#endif // !NDEBUG
*/
        auto initSkillDict = cc->init_skill_transit();
        if (!initSkillDict.count(encodedPattern)) {
/*
#ifndef NDEBUG
            std::ostringstream oss;
            oss << "@rdfId=" << rdfId << ", ud=" << ud << " tries to use init skill by (patternId=" << patternId << ", effDx=" << effDx << ", effDy=" << effDy << ", encodedPattern=" << encodedPattern << "), but initSkillDict doesn't contain it!";
            Debug::Log(oss.str(), DColor::Yellow);
#endif // !NDEBUG
*/
            return false;
        }
        targetSkillId = initSkillDict[encodedPattern];
/*
#ifndef NDEBUG
        std::ostringstream oss2;
        oss2 << "@rdfId=" << rdfId << ", ud=" << ud << " tries to use init skill by (patternId=" << patternId << ", effDx=" << effDx << ", effDy=" << effDy << ", encodedPattern=" << encodedPattern << "), targetSkillId=" << targetSkillId << " selected";
        Debug::Log(oss2.str(), DColor::Orange);
#endif // !NDEBUG
*/
    }

    if (!skillConfigs.count(targetSkillId)) {
#ifndef NDEBUG
         std::ostringstream oss;
         oss << "@rdfId=" << rdfId << ", ud=" << ud << ", targetSkillId=" << targetSkillId << " not in the global skillConfigs with keys: [ ";
        for (const auto& pair : skillConfigs) {
            oss << pair.first << " "; 
        }
        oss << "]";
         Debug::Log(oss.str(), DColor::Yellow);
#endif // !NDEBUG
        return false;
    }

    auto targetSkillConfig = skillConfigs.at(targetSkillId);

    if (targetSkillConfig.mp_delta() > currChd.mp()) {
        // notEnoughMp = true; // TODO
/*
#ifndef NDEBUG
         std::ostringstream oss;
         oss << "@rdfId=" << rdfId << ", ud=" << ud << ", not enough mp to use targetSkillId=" << targetSkillId;
         Debug::Log(oss.str(), DColor::Yellow);
#endif // !NDEBUG
*/
        return false;
    }

    outSkillId = targetSkillId;
    outSkill = &(targetSkillConfig);

    nextChd->set_mp(currChd.mp() - targetSkillConfig.mp_delta());
    if (0 >= nextChd->mp()) {
        nextChd->set_mp(0);
    }

    nextChd->set_dir_x(0 == effDx ? nextChd->dir_x() : effDx); // Upon successful skill use, allow abrupt turn-around regardless of inertia!
    int xfac = (0 < nextChd->dir_x() ? 1 : -1);
    int yfac = (!cc->omit_gravity() ? 0 : (0 < nextChd->dir_y() ? 1 : -1));

    nextChd->set_active_skill_id(targetSkillId);
    nextChd->set_frames_to_recover(targetSkillConfig.recovery_frames());

    int nextActiveSkillHit = 1;
    auto pivotBulletConfig = targetSkillConfig.hits(nextActiveSkillHit - 1);
    outPivotBc = &pivotBulletConfig;
    const Bullet* referenceBullet = nullptr;
    const BulletConfig* referenceBulletConfig = nullptr;
    for (int i = 0; i < pivotBulletConfig.simultaneous_multi_hit_cnt() + 1; i++) {
        if (!addNewBulletToNextFrame(rdfId, currChd, nextChd, cc, currParalyzed, currEffInAir, xfac, yfac, outSkill, nextActiveSkillHit, outSkillId, nextRdf, referenceBullet, referenceBulletConfig, ud, currChd.bullet_team_id())) {
            break;
        }
        nextChd->set_active_skill_hit(nextActiveSkillHit);
        nextActiveSkillHit++;
    }

    if (false == currEffInAir) {
        if (pivotBulletConfig.delay_self_vel_to_active() || !pivotBulletConfig.allows_walking()) {
            nextChd->set_vel_x(0);
        }
    }

    if (currParalyzed) {
        nextChd->set_vel_x(0);
    }

    nextChd->set_ch_state(targetSkillConfig.bound_ch_state());
    nextChd->set_frames_in_ch_state(0); // Must reset "frames_in_ch_state()" here to handle the extreme case where a same skill, e.g. "Atk1", is used right after the previous one ended
    if (nextChd->frames_invinsible() < pivotBulletConfig.startup_invinsible_frames()) {
        nextChd->set_frames_invinsible(pivotBulletConfig.startup_invinsible_frames());
    }
    return true;
}

void BaseBattle::useInventorySlot(int rdfId, int patternId, const CharacterDownsync& currChd, const CharacterConfig* cc, CharacterDownsync* nextChd, bool& outSlotUsed, bool& outDodgedInBlockStun) {
    outSlotUsed = false;
    bool intendToDodgeInBlockStun = false;
    outDodgedInBlockStun = false;

    /*

    int slotIdx = -1;
    if (globalPrimitiveConsts->pattern_inventory_slot_c() == patternId || globalPrimitiveConsts->pattern_inventory_slot_bc() == patternId) {
        slotIdx = 0;
    } else if (globalPrimitiveConsts->pattern_inventory_slot_d() == patternId) {
        slotIdx = 1;
    } else if (cc->UseInventoryBtnB && (globalPrimitiveConsts->pattern_b() == patternId || globalPrimitiveConsts->pattern_down_b() == patternId || globalPrimitiveConsts->pattern_released_b() == patternId)) {
        slotIdx = 2;
    } else if (IsInBlockStun(currChd)
               &&
               cmdPatternContainsEdgeTriggeredBtnE(patternId)
               &&
               !currEffInAir
              ) {
        slotIdx = 0;
        intendToDodgeInBlockStun = true;
    } else {
        return (false, globalPrimitiveConsts->no_skill(), false);
    }

    auto targetSlotCurr = currChd.Inventory.Slots[slotIdx];
    auto targetSlotNext = nextChd->Inventory.Slots[slotIdx];
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
            ApplyBuffToCharacter(rdfId, buffConfig, currChd, nextChd);
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
                ApplyBuffToCharacter(rdfId, buffConfig, currChd, nextChd);
            }
        }

        return (slotUsed, slotLockedSkillId, dodgedInBlockStun);
    }
    */
}

CH_COLLIDER_T* BaseBattle::createDefaultCharacterCollider(const CharacterConfig* cc) {
    CapsuleShape* chShapeCenterAnchor = new CapsuleShape(cc->capsule_half_height(), cc->capsule_radius()); // lifecycle to be held by "RotatedTranslatedShape::mInnerShape"
    RotatedTranslatedShape* chShape = new RotatedTranslatedShape(Vec3(0, cc->capsule_half_height() + cc->capsule_radius(), 0), Quat::sIdentity(), chShapeCenterAnchor); // lifecycle to be held by "CharacterVirtual::mShape"

    Ref<CharacterSettings> settings = new CharacterSettings();
    settings->mMaxSlopeAngle = cMaxSlopeAngle;
    settings->mLayer = MyObjectLayers::MOVING;
    settings->mFriction = cDefaultFriction;
    settings->mSupportingVolume = Plane(Vec3::sAxisY(), -cc->capsule_radius()); // Accept contacts that touch the lower sphere of the capsule
    settings->mEnhancedInternalEdgeRemoval = cEnhancedInternalEdgeRemoval;
    settings->mShape = chShape;
    
    auto ret = new Character(settings, safeDeactiviatedPosition, Quat::sIdentity(), 0, phySys);
	ret->AddToPhysicsSystem(EActivation::DontActivate);
    
    return ret;
}

Body* BaseBattle::createDefaultBulletCollider(const float immediateBoxHalfSizeX, const float immediateBoxHalfSizeY, float& outConvexRadius, const EMotionType immediateMotionType) {
    outConvexRadius = (immediateBoxHalfSizeX + immediateBoxHalfSizeY) * 0.5;
    if (cDefaultHalfThickness < outConvexRadius) {
        outConvexRadius = cDefaultHalfThickness; // Required by the underlying body creation 
    }
    Vec3 halfExtent(immediateBoxHalfSizeX, immediateBoxHalfSizeY, cDefaultHalfThickness);
    BoxShapeSettings* settings = new BoxShapeSettings(halfExtent, outConvexRadius); // transient, to be discarded after creating "body"
    BodyCreationSettings bodyCreationSettings(settings, safeDeactiviatedPosition, JPH::Quat::sIdentity(), immediateMotionType, MyObjectLayers::MOVING);
    bodyCreationSettings.mAllowDynamicOrKinematic = true;
    bodyCreationSettings.mGravityFactor = 0; // TODO
    Body* body = bi->CreateBody(bodyCreationSettings);
    JPH_ASSERT(nullptr != body);
    blStockCache->push_back(body);

    return body;
}

void BaseBattle::preallocateBodies(const RenderFrame* currRdf, const ::google::protobuf::Map<::google::protobuf::int32, ::google::protobuf::int32 >& preallocateNpcSpeciesDict) {
    for (int i = 0; i < playersCnt; i++) {
        const PlayerCharacterDownsync& currPlayer = currRdf->players_arr(i);
        const CharacterDownsync& currChd = currPlayer.chd();
        uint64_t ud = calcUserData(currPlayer);
#ifndef NDEBUG
        std::ostringstream oss2;
        oss2 << "[preallocateBodies] Player joinIndex=" << i+1 << " starts at position=(" << currChd.x() << ", " << currChd.y() << ", " << currChd.z() << "), species_id=" << currChd.species_id() << ", ud=" << ud; 
        Debug::Log(oss2.str(), DColor::Orange);
#endif
        const CharacterConfig* cc = getCc(currChd.species_id());
        float capsuleRadius = 0, capsuleHalfHeight = 0;
        calcChdShape(CharacterState::Idle1, cc, capsuleRadius, capsuleHalfHeight);
        auto chCollider = getOrCreateCachedPlayerCollider(ud, currPlayer, cc);

        chCollider->SetPositionAndRotation(safeDeactiviatedPosition, Quat::sIdentity());
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
            auto chCollider = createDefaultCharacterCollider(cc);
            chCollider->SetPositionAndRotation(safeDeactiviatedPosition, Quat::sIdentity());
            targetQue->push_back(chCollider);
        }
    }

    bodyIDsToAdd.clear();
    calcBlCacheKey(cDefaultBlHalfLength, cDefaultBlHalfLength, blCacheKeyHolder);
    if (!cachedBlColliders.count(blCacheKeyHolder)) {
        BL_COLLIDER_Q q = { };
        cachedBlColliders.emplace(blCacheKeyHolder, q);
    }
    blStockCache = &cachedBlColliders[blCacheKeyHolder];
    JPH_ASSERT(nullptr != blStockCache);
    for (int i = 0; i < currRdf->bullets_size(); i++) {
        float dummyNewConvexRadius = 0;
        auto preallocatedBlCollider = createDefaultBulletCollider(cDefaultBlHalfLength, cDefaultBlHalfLength, dummyNewConvexRadius);
        bodyIDsToAdd.push_back(preallocatedBlCollider->GetID());
    }
    auto layerState = bi->AddBodiesPrepare(bodyIDsToAdd.data(), bodyIDsToAdd.size());
    bi->AddBodiesFinalize(bodyIDsToAdd.data(), bodyIDsToAdd.size(), layerState, EActivation::DontActivate);

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

void BaseBattle::NewPreallocatedBullet(Bullet* single, int bulletId, int originatedRenderFrameId, int teamId, BulletState blState, int framesInBlState) {
    single->set_id(bulletId);
    single->set_bl_state(blState);
    single->set_frames_in_bl_state(framesInBlState);
    single->set_originated_render_frame_id(originatedRenderFrameId);
    single->set_team_id(teamId);
}

void BaseBattle::NewPreallocatedCharacterDownsync(CharacterDownsync* single, int buffCapacity, int debuffCapacity, int inventoryCapacity, int bulletImmuneRecordCapacity) {
    for (int i = 0; i < buffCapacity; i++) {
        Buff* singleBuff = single->add_buff_list();
        singleBuff->set_species_id(globalPrimitiveConsts->terminating_buff_species_id());
        singleBuff->set_originated_render_frame_id(globalPrimitiveConsts->terminating_render_frame_id());
        singleBuff->set_orig_ch_species_id(globalPrimitiveConsts->species_none_ch());
    }
    for (int i = 0; i < debuffCapacity; i++) {
        Buff* singleDebuff = single->add_buff_list();
        singleDebuff->set_species_id(globalPrimitiveConsts->terminating_debuff_species_id());
    }
    if (0 < inventoryCapacity) {
        Inventory* inv = single->mutable_inventory();
        for (int i = 0; i < inventoryCapacity; i++) {
            auto singleSlot = inv->add_slots();
            singleSlot->set_stock_type(InventorySlotStockType::NoneIv);
        }
    }

    for (int i = 0; i < bulletImmuneRecordCapacity; i++) {
        auto singleRecord = single->add_bullet_immune_records();
        singleRecord->set_bullet_id(globalPrimitiveConsts->terminating_bullet_id());
        singleRecord->set_remaining_lifetime_rdf_count(0);
    }
}

void BaseBattle::NewPreallocatedPlayerCharacterDownsync(PlayerCharacterDownsync* single, int buffCapacity, int debuffCapacity, int inventoryCapacity, int bulletImmuneRecordCapacity) {
    single->set_join_index(globalPrimitiveConsts->magic_join_index_invalid());
    NewPreallocatedCharacterDownsync(single->mutable_chd(), buffCapacity, debuffCapacity, inventoryCapacity, bulletImmuneRecordCapacity);
}

void BaseBattle::NewPreallocatedNpcCharacterDownsync(NpcCharacterDownsync* single, int buffCapacity, int debuffCapacity, int inventoryCapacity, int bulletImmuneRecordCapacity) {
    single->set_id(globalPrimitiveConsts->terminating_character_id());
    NewPreallocatedCharacterDownsync(single->mutable_chd(), buffCapacity, debuffCapacity, inventoryCapacity, bulletImmuneRecordCapacity);
}

RenderFrame* BaseBattle::NewPreallocatedRdf(int roomCapacity, int preallocNpcCount, int preallocBulletCount) {
    auto ret = new RenderFrame();
    ret->set_id(globalPrimitiveConsts->terminating_render_frame_id());
    ret->set_bullet_id_counter(0);

    for (int i = 0; i < roomCapacity; i++) {
        auto single = ret->add_players_arr();
        NewPreallocatedPlayerCharacterDownsync(single, globalPrimitiveConsts->default_per_character_buff_capacity(), globalPrimitiveConsts->default_per_character_debuff_capacity(), globalPrimitiveConsts->default_per_character_inventory_capacity(), globalPrimitiveConsts->default_per_character_immune_bullet_record_capacity());
    }

    for (int i = 0; i < preallocNpcCount; i++) {
        auto single = ret->add_npcs_arr();
        NewPreallocatedNpcCharacterDownsync(single, globalPrimitiveConsts->default_per_character_buff_capacity(), globalPrimitiveConsts->default_per_character_debuff_capacity(), 1, globalPrimitiveConsts->default_per_character_immune_bullet_record_capacity());
    }

    for (int i = 0; i < preallocBulletCount; i++) {
        auto single = ret->add_bullets();
        NewPreallocatedBullet(single, globalPrimitiveConsts->terminating_bullet_id(), 0, 0, BulletState::StartUp, 0);
    }

    return ret;
}

void BaseBattle::OnContactCommon(
    const JPH::Body& inBody1,
    const JPH::Body& inBody2,
    const JPH::ContactManifold& inManifold,
    const JPH::ContactSettings& ioSettings) {

    uint64_t ud1 = inBody1.GetUserData();
    uint64_t ud2 = inBody2.GetUserData();

    uint64_t udt1 = getUDT(ud1);
    uint64_t udt2 = getUDT(ud2);

    switch (udt1)
    {
    case UDT_BL:
        handleLhsBulletCollisionMT(ud1, inBody1, ud2, udt2, inBody2, inManifold.SwapShapes(), ioSettings);
        break;
    default:
        switch (udt2) {
        case UDT_BL: {
            handleLhsBulletCollisionMT(ud2, inBody2, ud1, udt1, inBody1, inManifold, ioSettings);
            break;
        }
        default:
            break;
        }
        break;
    }
}

void BaseBattle::PostSimulationWithCollector(const RVec3& char_pos, const Quat& char_rot, const Vec3& char_vel, CH_COLLIDER_T* chCollider, float inMaxSeparationDistance, CharacterCollideShapeCollector* collector) {
    
    // Collide shape
    chCollider->CheckCollision(char_pos, char_rot, char_vel, inMaxSeparationDistance, chCollider->GetShape(), char_pos, *collector, false);

    // Copy results
    chCollider->SetGroundBodyID(collector->mGroundBodyID, collector->mGroundBodySubShapeID);
    chCollider->SetGroundBodyPosition(collector->mGroundPosition, collector->mGroundNormal);

    if (!collector->mGroundBodyID.IsInvalid()) {
        // Update ground state
        RMat44 inv_transform = RMat44::sInverseRotationTranslation(char_rot, char_pos);
        if (chCollider->GetSupportingVolume()->SignedDistance(Vec3(inv_transform * collector->mGroundPosition)) > 0.0f)
            chCollider->SetGroundState(JPH::CharacterBase::EGroundState::NotSupported);
        else if (chCollider->IsSlopeTooSteep(collector->mGroundNormal))
            chCollider->SetGroundState(JPH::CharacterBase::EGroundState::OnSteepGround);
        else
            chCollider->SetGroundState(JPH::CharacterBase::EGroundState::OnGround);

        // Copy other body properties
        chCollider->SetGroundBodyUd(bi->GetMaterial(collector->mGroundBodyID, collector->mGroundBodySubShapeID), bi->GetPointVelocity(collector->mGroundBodyID, collector->mGroundPosition), bi->GetUserData(collector->mGroundBodyID));
    } else {
        chCollider->SetGroundState(JPH::CharacterBase::EGroundState::InAir);
        chCollider->SetGroundBodyUd(JPH::PhysicsMaterial::sDefault, Vec3::sZero(), 0);
    }
}

void BaseBattle::handleLhsCharacterCollision(
    const uint64_t udLhs, const uint64_t udtLhs, const CharacterDownsync* currChd, CharacterDownsync* nextChd,
    const uint64_t udRhs, const uint64_t udtRhs) {

    switch (udtRhs) {
    case UDT_BL:
        if (!transientUdToNextBl.count(udRhs)) {
#ifndef NDEBUG
            std::ostringstream oss;
            auto bulletId = getUDPayload(udRhs);
            oss << "handleLhsCharacterCollision/nextBl with id=" << bulletId << " is NOT FOUND";
            Debug::Log(oss.str(), DColor::Orange);
#endif
            return;
        }
        auto rhsNextBl = transientUdToNextBl[udRhs];
        const Skill* rhsSkill = nullptr;
        const BulletConfig* rhsBlConfig = nullptr;
        FindBulletConfig(rhsNextBl->skill_id(), rhsNextBl->active_skill_hit(), rhsSkill, rhsBlConfig);

        rhsNextBl->set_bl_state(BulletState::Exploding);
        rhsNextBl->set_frames_in_bl_state(0);
        rhsNextBl->set_vel_x(0);
        rhsNextBl->set_vel_y(0);

        if (0 < currChd->frames_invinsible()) {
            break;
        }
        // [TODO] Check currChd.bullet_immune_records

        nextChd->set_hp(nextChd->hp() - rhsBlConfig->damage());

        // [TODO] Add to nextChd.bullet_immune_records

        break;
    }
}

void BaseBattle::handleLhsBulletCollisionMT(
    const uint64_t udLhs,
    const JPH::Body& lhs, // the "Bullet"
    const uint64_t udRhs, const uint64_t udtRhs, const JPH::Body& rhs,
    const JPH::ContactManifold& inManifold,
    const JPH::ContactSettings& ioSettings) {
    /*
    [WARNING] This function MUST BE implemented in a thread-safe manner, otherwise please move out unsafe codes to elsewhere like "BaseBattle::PostSimulationWithCollector". Be very careful about thread-safety if you want to call "BaseBattle::addNewBulletToNextFrame(...)" here.
   
    See comments for "OnContactAdded" and "OnContactPersisted" above.
    */
    if (!transientUdToCurrBl.count(udLhs)) {
#ifndef NDEBUG
        std::ostringstream oss;
        auto bulletId = getUDPayload(udLhs);
        oss << "handleLhsBulletCollision/currBl with id=" << bulletId << " is NOT FOUND";
        Debug::Log(oss.str(), DColor::Orange);
#endif // !NDEBUG
        return;
    }
    if (!transientUdToNextBl.count(udLhs)) {
#ifndef NDEBUG
        std::ostringstream oss;
        auto bulletId = getUDPayload(udLhs);
        oss << "handleLhsBulletCollision/nextBl with id=" << bulletId << " is NOT FOUND";
        Debug::Log(oss.str(), DColor::Orange);
#endif // !NDEBUG
        return;
    }
    auto lhsCurrBl = transientUdToCurrBl[udLhs];
    auto lhsNextBl = transientUdToNextBl[udLhs];
    const Skill* lhsSkill = nullptr;
    const BulletConfig* lhsBlConfig = nullptr;
    FindBulletConfig(lhsCurrBl->skill_id(), lhsCurrBl->active_skill_hit(), lhsSkill, lhsBlConfig);
/*
#ifndef NDEBUG
    std::ostringstream oss;
    auto bulletId = getUDPayload(udLhs);
    oss << "handleLhsBulletCollision/colliding udLhs=" << udLhs << " with udRhs=" << udRhs;
    Debug::Log(oss.str(), DColor::Orange);
#endif // !NDEBUG
*/
    switch (udtRhs) {
    case UDT_OBSTACLE:
        switch (lhsBlConfig->b_type()) {
        case BulletType::MechanicalCartridge: {
            lhsNextBl->set_bl_state(BulletState::Exploding);
            lhsNextBl->set_frames_in_bl_state(0);
            lhsNextBl->set_vel_x(0);
            lhsNextBl->set_vel_y(0);
#ifndef NDEBUG
            std::ostringstream oss;
            auto bulletId = getUDPayload(udLhs);
            oss << "handleLhsBulletCollision/nextBl with id=" << bulletId << ", ud=" << udLhs << ", bodyID=" << lhs.GetID().GetIndexAndSequenceNumber() << " explodes on obstacle";
            Debug::Log(oss.str(), DColor::Orange);
#endif // !NDEBUG
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
    case UDT_BL:
        // TODO
        break;
    default:
        break;
    }
}

bool BaseBattle::deallocPhySys() {
    if (nullptr == phySys) return false;
    delete phySys;
    phySys = nullptr;
    bi = nullptr;
    
    /*
    [WARNING] Unlike "std::vector" and "std::unordered_map", the customized containers "JPH::StaticArray" and "JPH::UnorderedMap" will deallocate their pointer-typed elements in their destructors.
    - https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Core/Array.h#L337 -> https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Core/Array.h#L249
    - https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Core/HashTable.h#L480 -> https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Core/HashTable.h#L497

    However I only use "dynamicBodyIDs" to hold "BodyID" instances which are NOT pointer-typed and effectively freed once "JPH::StaticArray::clear()" is called, i.e. no need for the heavy duty "JPH::Array".
    */
    return true;
}
