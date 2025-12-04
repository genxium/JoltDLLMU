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
#include <climits> // Required for "INT_MAX"
#include <cmath> // Required for "ceil()"

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
        2. This function "getOrCreateCachedCharacterCollider" is only used in a single-threaded context (i.e. as a preparation before the multi-threaded "PhysicsSystem::Update").
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
    auto bodyID = chCollider->GetBodyID();
    transientUdToChCollider[ud] = chCollider;
    activeChColliders.push_back(chCollider);
    bi->SetUserData(bodyID, ud);

    return chCollider;
}

JPH::Body* BaseBattle::getOrCreateCachedBulletCollider(const uint64_t ud, const float immediateBoxHalfSizeX, const float immediateBoxHalfSizeY, const BulletType blType) {
    calcBlCacheKey(immediateBoxHalfSizeX, immediateBoxHalfSizeY, blCacheKeyHolder);
    EMotionType immediateMotionType = calcBlMotionType(blType);
    bool immediateIsSensor = calcBlIsSensor(blType);
    Vec3 newHalfExtent = Vec3(immediateBoxHalfSizeX, immediateBoxHalfSizeY, cDefaultHalfThickness);
    float newConvexRadius = 0;
    Body* blCollider = nullptr;
    auto it = cachedBlColliders.find(blCacheKeyHolder);
    if (it == cachedBlColliders.end() || it->second.empty()) {
        if (blStockCache->empty()) {
            blCollider = createDefaultBulletCollider(immediateBoxHalfSizeX, immediateBoxHalfSizeY, newConvexRadius, immediateMotionType, immediateIsSensor);
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
    auto existingIsSensor = blCollider->IsSensor();
    if (existingHalfExtent != newHalfExtent || existingConvexRadius != newConvexRadius || existingMotionType != immediateMotionType || existingIsSensor != immediateIsSensor) {
        Vec3Arg previousShapeCom = shape->GetCenterOfMass();

        int oldShapeRefCnt = shape->GetRefCount();
        
        void* newShapeBuffer = (void*)shape;
        BoxShape* newShape = new (newShapeBuffer) BoxShape(newHalfExtent, newConvexRadius);

        blCollider->SetShapeInternal(newShape, true);
        blCollider->SetIsSensor(immediateIsSensor);
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

bool BaseBattle::transitToDying(const int currRdfId, const CharacterDownsync& currChd, const bool cvInAir, CharacterDownsync* nextChd) {
    if (CharacterState::Dying == currChd.ch_state()) return false;
    nextChd->set_ch_state(CharacterState::Dying);
    nextChd->set_frames_in_ch_state(0);
    nextChd->set_frames_to_recover(globalPrimitiveConsts->dying_frames_to_recover());
    nextChd->set_frames_invinsible(0);
    nextChd->set_hp(0);
    if (!cvInAir) {
        nextChd->set_vel_x(0);
        nextChd->set_vel_y(0);
    }
    resetJumpStartup(nextChd);
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
        chCollider->SetPositionAndRotation(Vec3(currChd.x(), currChd.y(), currChd.z()), Quat(currChd.q_x(), currChd.q_y(), currChd.q_z(), currChd.q_w()), EActivation::Activate);
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
    mNextRdfBulletIdCounter = nextRdf->bullet_id_counter();
    mNextRdfBulletCount = nextRdf->bullet_count();

    batchPutIntoPhySysFromCache(currRdfId, currRdf, nextRdf);

    auto stepResult = nextRdf->mutable_prev_rdf_step_result();
    stepResult->clear_fulfilled_triggers();
    for (int i = 0; i < currRdf->triggers_arr_size(); i++) {
        const Trigger& currTrigger = currRdf->triggers_arr(i);
        if (globalPrimitiveConsts->terminating_trigger_id() == currTrigger.id()) break;
        Trigger* nextTrigger = nextRdf->mutable_triggers_arr(i);
        auto ud = calcUserData(currTrigger);
        transientUdToCurrTrigger[ud] = &currTrigger;
        transientUdToNextTrigger[ud] = nextTrigger;

        if (0 != currTrigger.demanded_evt_mask() && currTrigger.fulfilled_evt_mask() == currTrigger.demanded_evt_mask()) {
            auto fulfilledTrigger = stepResult->add_fulfilled_triggers();
            fulfilledTrigger->CopyFrom(currTrigger);

            int newQuota = currTrigger.quota() - 1;
            if (0 > newQuota) {
                newQuota = 0;
            }
            nextTrigger->set_quota(newQuota);
            nextTrigger->set_state(TriggerState::TCoolingDown);
            nextTrigger->set_frames_in_state(0);

            // TODO: Reset "demanded_evt_mask" for certain "TriggerType"s 
            auto triggerConfig = getTriggerConfig(currTrigger.trigger_type());
            if (triggerConfigFromTileDict.count(currTrigger.id())) {
                auto triggerConfigFromTiled = triggerConfigFromTileDict[currTrigger.id()];
                int newFramesToRecover = triggerConfigFromTiled->recovery_frames();
                nextTrigger->set_frames_to_recover(newFramesToRecover);
            }
        }
    }

    JobSystem::Barrier* prePhysicsUpdateMTBarrier = jobSys->CreateBarrier();
    for (int i = 0; i < playersCnt; i++) {
        uint64_t singleInput = delayedIfd->input_list(i);
        auto handle = jobSys->CreateJob("player-pre-physics-update", JPH::Color::sBlack, [currRdfId, i, currRdf, nextRdf, this, singleInput, dt]() {
            const PlayerCharacterDownsync& currPlayer = currRdf->players_arr(i);
            PlayerCharacterDownsync* nextPlayer = nextRdf->mutable_players_arr(i); // [WARNING] The indices of "currRdf->players_arr" and "nextRdf->players_arr" are ALWAYS FULLY ALIGNED.
            auto ud = calcUserData(currPlayer);
            const CharacterDownsync& currChd = currPlayer.chd();
            CharacterDownsync* nextChd = nextPlayer->mutable_chd();
            if (!noOpSet.count(currChd.ch_state())) {
                const CharacterConfig* cc = getCc(currChd.species_id());
                auto currChState = currChd.ch_state();
                bool currNotDashing = isNotDashing(currChd);
                bool currDashing = !currNotDashing;
                bool currWalking = walkingSet.count(currChState);
                bool currEffInAir = isEffInAir(currChd, currNotDashing);
                bool currOnWall = onWallSet.count(currChState);
                bool currCrouching = isCrouching(currChState, cc);
                bool currAtked = atkedSet.count(currChState);
                bool currInBlockStun = isInBlockStun(currChd);
                bool currParalyzed = false; // TODO

                auto cv = transientUdToChCollider[ud];

                int patternId = globalPrimitiveConsts->pattern_id_no_op();
                bool jumpedOrNot = false;
                bool slipJumpedOrNot = false;
                int effDx = 0, effDy = 0;
                InputFrameDecoded ifDecodedHolder;
                decodeInput(singleInput, &ifDecodedHolder);
                derivePlayerOpPattern(currRdfId, currChd, cc, nextChd, currEffInAir, currNotDashing, ifDecodedHolder, patternId, jumpedOrNot, slipJumpedOrNot, effDx, effDy);
                bool slowDownToAvoidOverlap = false;
                bool usedSkill = false;
                processSingleCharacterInput(currRdfId, dt, patternId, jumpedOrNot, slipJumpedOrNot, effDx, effDy, slowDownToAvoidOverlap, currChd, ud, currEffInAir, currCrouching, currOnWall, currDashing, currWalking, currInBlockStun, currAtked, currParalyzed, cc, nextChd, nextRdf, usedSkill);
            }

            updateChColliderBeforePhysicsUpdate(ud, currChd, *nextChd); // After "processSingleCharacterInput" setting proper "velocity" of "nextChd"
        }, 0);
        prePhysicsUpdateMTBarrier->AddJob(handle);
    }

    for (int i = 0; i < currRdf->npcs_arr_size(); i++) {
        if (globalPrimitiveConsts->terminating_character_id() == currRdf->npcs_arr(i).id()) break;
        auto handle = jobSys->CreateJob("npc-pre-physics-update", JPH::Color::sBlack, [currRdfId, i, currRdf, nextRdf, this, dt]() {
            const NpcCharacterDownsync& currNpc = currRdf->npcs_arr(i);
            NpcCharacterDownsync* nextNpc = nextRdf->mutable_npcs_arr(i); // [WARNING] By reaching here, we haven't executed "leftShiftDeadNpcs", hence the indices of "currRdf->npcs_arr" and "nextRdf->npcs_arr" are FULLY ALIGNED.
            auto ud = calcUserData(currNpc);
            const CharacterDownsync& currChd = currNpc.chd();
            CharacterDownsync* nextChd = nextNpc->mutable_chd();
            if (!noOpSet.count(currChd.ch_state())) {
                // TODO: Narrowphase collision check for vision to derive npc reaction first
                const CharacterConfig* cc = getCc(currChd.species_id());
                auto currChState = currChd.ch_state();
                bool currNotDashing = isNotDashing(currChd);
                bool currDashing = !currNotDashing;
                bool currWalking = walkingSet.count(currChState);
                bool currEffInAir = isEffInAir(currChd, currNotDashing);
                bool currOnWall = onWallSet.count(currChState);
                bool currCrouching = isCrouching(currChState, cc);
                bool currAtked = atkedSet.count(currChState);
                bool currInBlockStun = isInBlockStun(currChd);
                bool currParalyzed = false; // TODO

                auto cv = transientUdToChCollider[ud];

                int patternId = globalPrimitiveConsts->pattern_id_no_op();
                bool jumpedOrNot = false;
                bool slipJumpedOrNot = false;
                int effDx = 0, effDy = 0;
                deriveNpcOpPattern(currRdfId, currChd, cc, currEffInAir, currNotDashing, ifDecodedHolder, patternId, jumpedOrNot, slipJumpedOrNot, effDx, effDy);

                bool slowDownToAvoidOverlap = false; // TODO
                bool usedSkill = false;
                processSingleCharacterInput(currRdfId, dt, patternId, jumpedOrNot, slipJumpedOrNot, effDx, effDy, slowDownToAvoidOverlap, currChd, ud, currEffInAir, currCrouching, currOnWall, currDashing, currWalking, currInBlockStun, currAtked, currParalyzed, cc, nextChd, nextRdf, usedSkill);
                
                if (usedSkill) {
                    nextNpc->set_cached_cue_cmd(0);
                }
            }
            updateChColliderBeforePhysicsUpdate(ud, currChd, *nextChd);
        }, 0);
        prePhysicsUpdateMTBarrier->AddJob(handle);
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

    jobSys->WaitForJobs(prePhysicsUpdateMTBarrier);
    jobSys->DestroyBarrier(prePhysicsUpdateMTBarrier);

    // [REMINDER] The "class CharacterVirtual" instances WOULDN'T participate in "phySys->Update(...)" IF they were NOT filled with valid "mInnerBodyID". See "RuleOfThumb.md" for details.
    phySys->Update(dt, 1, globalTempAllocator, jobSys); 

    JobSystem::Barrier* postPhysicsUpdateMTBarrier = jobSys->CreateBarrier();
    const BaseBattle* battle = this;
    for (int i = 0; i < playersCnt; i++) {
        auto handle = jobSys->CreateJob("player-post-physics-update", JPH::Color::sBlack, [currRdfId, i, currRdf, nextRdf, this, dt]() {
            auto currPlayer = currRdf->players_arr(i);
            auto nextPlayer = nextRdf->mutable_players_arr(i); // [WARNING] The indices of "currRdf->players_arr" and "nextRdf->players_arr" are ALWAYS FULLY ALIGNED.
            const CharacterDownsync& currChd = currPlayer.chd();
            auto nextChd = nextPlayer->mutable_chd();

            auto ud = calcUserData(currPlayer);
            JPH_ASSERT(transientUdToChCollider.count(ud));

            const CharacterConfig* cc = getCc(currChd.species_id());
            bool groundBodyIsChCollider = false, isDead = false; 
            CH_COLLIDER_T* single = transientUdToChCollider[ud];

            bool currNotDashing = isNotDashing(currChd);
            bool currEffInAir = isEffInAir(currChd, currNotDashing);
            bool oldNextNotDashing = isNotDashing(*nextChd);
            bool oldNextEffInAir = isEffInAir(*nextChd, oldNextNotDashing);
            //bool isProactivelyJumping = proactiveJumpingSet.count(nextChd->ch_state());

            bool cvOnWall = false, cvSupported = false, cvInAir = true, inJumpStartupOrJustEnded = false; 
            CharacterBase::EGroundState cvGroundState = CharacterBase::EGroundState::InAir;
            stepSingleChdState(currRdfId, currRdf, nextRdf, dt, ud, UDT_PLAYER, cc, single, currChd, nextChd, groundBodyIsChCollider, isDead, cvOnWall, cvSupported, cvInAir, inJumpStartupOrJustEnded, cvGroundState);
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
                    nextChd->set_vel_x(0);
                    nextChd->set_vel_y(0);
                    nextChd->set_vel_z(0);
                }
            }
        }, 0);
        postPhysicsUpdateMTBarrier->AddJob(handle);
    }

    for (int i = 0; i < currRdf->npcs_arr_size(); i++) {
        if (globalPrimitiveConsts->terminating_character_id() == currRdf->npcs_arr(i).id()) break;
        auto handle = jobSys->CreateJob("npc-post-physics-update", JPH::Color::sBlack, [currRdfId, i, currRdf, nextRdf, this, dt]() {
            const NpcCharacterDownsync& currNpc = currRdf->npcs_arr(i);
            auto nextNpc = nextRdf->mutable_npcs_arr(i); // [WARNING] By reaching here, we haven't executed "leftShiftDeadNpcs", hence the indices of "currRdf->npcs_arr" and "nextRdf->npcs_arr" are FULLY ALIGNED.

            const CharacterDownsync& currChd = currNpc.chd();
            auto nextChd = nextNpc->mutable_chd();
            auto ud = calcUserData(currNpc);

            JPH_ASSERT(transientUdToChCollider.count(ud));
            const CharacterConfig* cc = getCc(currChd.species_id());
            bool groundBodyIsChCollider = false, isDead = false; 
            CH_COLLIDER_T* single = transientUdToChCollider[ud];

            bool currNotDashing = isNotDashing(currChd);
            bool currEffInAir = isEffInAir(currChd, currNotDashing);
            bool oldNextNotDashing = isNotDashing(*nextChd);
            bool oldNextEffInAir = isEffInAir(*nextChd, oldNextNotDashing);
            //bool isProactivelyJumping = proactiveJumpingSet.count(nextChd->ch_state());

            bool cvOnWall = false, cvSupported = false, cvInAir = true, inJumpStartupOrJustEnded = false; 
            CharacterBase::EGroundState cvGroundState = CharacterBase::EGroundState::InAir;
            stepSingleChdState(currRdfId, currRdf, nextRdf, dt, ud, UDT_NPC, cc, single, currChd, nextChd, groundBodyIsChCollider, isDead, cvOnWall, cvSupported, cvInAir, inJumpStartupOrJustEnded, cvGroundState);
            postStepSingleChdStateCorrection(currRdfId, UDT_NPC, ud, single, currChd, nextChd, cc, cvSupported, cvInAir, cvOnWall, currNotDashing, currEffInAir, oldNextNotDashing, oldNextEffInAir, inJumpStartupOrJustEnded, cvGroundState);

            if (isDead) {
                if (CharacterState::Dying != nextChd->ch_state()) {
                    transitToDying(currRdfId, currNpc, cvInAir, nextNpc);
                }
            }
        }, 0);
        postPhysicsUpdateMTBarrier->AddJob(handle);
    }

    for (int i = 0; i < currRdf->bullets_size(); i++) {
        if (globalPrimitiveConsts->terminating_bullet_id() == currRdf->bullets(i).id()) break;
        auto handle = jobSys->CreateJob("bullet-post-physics-update", JPH::Color::sBlack, [currRdfId, i, currRdf, nextRdf, this, dt]() {
            const NarrowPhaseQuery& narrowPhaseQueryNoLock = phySys->GetNarrowPhaseQueryNoLock(); // no need to lock after physics update
            const Bullet& currBl = currRdf->bullets(i);
            Bullet* nextBl = nextRdf->mutable_bullets(i); // [WARNING] By reaching here, we haven't executed "leftShiftDeadBullets", hence the indices of "currRdf->bullets" and "nextRdf->bullets" are FULLY ALIGNED.
            auto ud = calcUserData(currBl);
            
            if (transientUdToBodyID.count(ud)) {
                auto bodyID = *(transientUdToBodyID[ud]);
                auto blShape = bi->GetShape(bodyID);
                
                auto blCOMTransform = bi->GetCenterOfMassTransform(bodyID);
                auto newPos = bi->GetPosition(bodyID);
                auto newVel = bi->GetLinearVelocity(bodyID);
                
                // Settings for collide shape
                CollideShapeSettings settings;
                settings.mMaxSeparationDistance = cCollisionTolerance;
                settings.mActiveEdgeMode = EActiveEdgeMode::CollideOnlyWithActive;
                settings.mActiveEdgeMovementDirection = newVel;
                settings.mBackFaceMode = EBackFaceMode::IgnoreBackFaces;

                class BulletBodyFilter : public IgnoreSingleBodyFilter {
                public:
                    using			IgnoreSingleBodyFilter::IgnoreSingleBodyFilter;

                    virtual bool	ShouldCollideLocked(const Body& inBody) const override
                    {
                        return !inBody.IsSensor();
                    }
                };
                BulletBodyFilter blBodyFilter(bodyID);
                BulletCollideShapeCollector collector(currRdfId, nextRdf, bi, ud, UDT_BL, &currBl, nextBl, this);
                narrowPhaseQueryNoLock.CollideShape(blShape, Vec3::sOne(), blCOMTransform, settings, newPos, collector, phySys->GetDefaultBroadPhaseLayerFilter(MyObjectLayers::MOVING), phySys->GetDefaultLayerFilter(MyObjectLayers::MOVING), blBodyFilter);

                JPH_ASSERT(transientUdToNextBl.count(ud));
                auto& nextBl = transientUdToNextBl[ud];

                nextBl->set_x(newPos.GetX());
                nextBl->set_y(newPos.GetY());
                nextBl->set_z(0);
                nextBl->set_vel_x(IsLengthNearZero(newVel.GetX()*dt) ? 0 : newVel.GetX());
                nextBl->set_vel_y(IsLengthNearZero(newVel.GetY()*dt) ? 0 : newVel.GetY());
                nextBl->set_vel_z(0);
            }
        }, 0);
        postPhysicsUpdateMTBarrier->AddJob(handle);
    }

    jobSys->WaitForJobs(postPhysicsUpdateMTBarrier);
    jobSys->DestroyBarrier(postPhysicsUpdateMTBarrier);
    
    calcFallenDeath(currRdf, nextRdf);
    leftShiftDeadNpcs(currRdfId, nextRdf); // Might change "mNextRdfBulletIdCounter" and "mNextRdfBulletCount"
    leftShiftDeadBullets(currRdfId, nextRdf); // Might change "mNextRdfBulletCount"
    leftShiftDeadPickables(currRdfId, nextRdf);
    leftShiftDeadTriggers(currRdfId, nextRdf);
    batchRemoveFromPhySysAndCache(currRdfId, currRdf);

    nextRdf->set_bullet_id_counter(mNextRdfBulletIdCounter.load());

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
    if (!bodyIDsToClear.empty()) {
        bi->RemoveBodies(bodyIDsToClear.data(), bodyIDsToClear.size());
        bi->DestroyBodies(bodyIDsToClear.data(), bodyIDsToClear.size());
        bodyIDsToClear.clear();
    }

    blStockCache = nullptr;

    while (!activeNonContactConstraints.empty()) {
        NON_CONTACT_CONSTRAINT_T* single = activeNonContactConstraints.back();
        activeNonContactConstraints.pop_back();
        phySys->RemoveConstraint(single);
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
        initializeTriggerDemandedEvtMask(holder);
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
    for (int i = 0; i < initializerMapData->serialized_barriers_size(); i++) {
        auto& barrier = initializerMapData->serialized_barriers(i);
        auto& convexPolygon = barrier.polygon();
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

bool BaseBattle::initializeTriggerDemandedEvtMask(RenderFrame* startRdf) {
    std::unordered_map<uint32_t, uint64_t> collectedPublishingEvtMaskUponExhausted;
    std::unordered_map<uint32_t, uint64_t> collectedDemandedEvtMask;

    for (int i = 0; i < startRdf->npcs_arr_size(); ++i) {
        NpcCharacterDownsync* npc = startRdf->mutable_npcs_arr(i);
        if (globalPrimitiveConsts->terminating_character_id() == npc->id()) break;
        auto targetTriggerId = npc->publishing_to_trigger_id_upon_exhausted();
        if (globalPrimitiveConsts->terminating_trigger_id() == targetTriggerId) continue;
        
        if (!collectedPublishingEvtMaskUponExhausted.count(targetTriggerId)) {
            collectedPublishingEvtMaskUponExhausted[targetTriggerId] = 1;
        } else {
            collectedPublishingEvtMaskUponExhausted[targetTriggerId] <<= 1;
        }

        npc->set_publishing_evt_mask_upon_exhausted(collectedPublishingEvtMaskUponExhausted[targetTriggerId]);
        collectedDemandedEvtMask[targetTriggerId] |= collectedPublishingEvtMaskUponExhausted[targetTriggerId];
    }

    for (int i = 0; i < startRdf->triggers_arr_size(); ++i) {
        Trigger* tr = startRdf->mutable_triggers_arr(i);
        if (globalPrimitiveConsts->terminating_trigger_id() == tr->id()) break;
        auto targetTriggerId = tr->publishing_to_trigger_id_upon_exhausted();
        if (globalPrimitiveConsts->terminating_trigger_id() == targetTriggerId) continue;

        if (!collectedPublishingEvtMaskUponExhausted.count(targetTriggerId)) {
            collectedPublishingEvtMaskUponExhausted[targetTriggerId] = 1;
        } else {
            collectedPublishingEvtMaskUponExhausted[targetTriggerId] <<= 1;
        }

        tr->set_publishing_evt_mask_upon_exhausted(collectedPublishingEvtMaskUponExhausted[targetTriggerId]);
        collectedDemandedEvtMask[targetTriggerId] |= collectedPublishingEvtMaskUponExhausted[targetTriggerId];
    }
    
    for (int i = 0; i < startRdf->triggers_arr_size(); ++i) {
        Trigger* tr = startRdf->mutable_triggers_arr(i);
        if (globalPrimitiveConsts->terminating_trigger_id() == tr->id()) break;
        if (!collectedDemandedEvtMask.count(tr->id())) {
            // For "TtByMovement" or "TtByAttack"
            tr->set_demanded_evt_mask(1);
        } else {
            tr->set_demanded_evt_mask(collectedDemandedEvtMask[tr->id()]);
        }
    }

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

void BaseBattle::transitToGroundDodgedChState(const CharacterDownsync& currChd, CharacterDownsync* nextChd, const CharacterConfig* cc, bool currParalyzed) {
    CharacterState oldNextChState = nextChd->ch_state();
    nextChd->set_ch_state(GroundDodged);
    nextChd->set_frames_in_ch_state(0);
    nextChd->set_frames_to_recover(cc->ground_dodged_frames_to_recover());
    nextChd->set_frames_invinsible(cc->ground_dodged_frames_invinsible());
    nextChd->set_active_skill_id(globalPrimitiveConsts->no_skill());
    nextChd->set_active_skill_hit(globalPrimitiveConsts->no_skill_hit());
    if (0 == nextChd->vel_x()) {
        JPH::Quat currChdQ(currChd.q_x(), currChd.q_y(), currChd.q_z(), currChd.q_w());
        Vec3 currChdFacing = currChdQ*Vec3(1, 0, 0);
        auto effSpeed = (0 >= cc->ground_dodged_speed() ? cc->speed() : cc->ground_dodged_speed());
        if (BackDashing == oldNextChState) {
            nextChd->set_vel_x(0 > currChdFacing.GetX() ? effSpeed : -effSpeed);
        } else {
            nextChd->set_vel_x(0 < currChdFacing.GetX() ? effSpeed : -effSpeed);
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
    if (0 < currChd.frames_to_recover()) {
        return;
    }

    if (TransformingInto == currChd.ch_state() || TransformingInto == nextChd->ch_state()) {
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
    if (0 < currChd.frames_to_recover()) {
        return;
    }

    bool jumpStarted = nextChd->jump_started();
    if (jumpStarted) {
        if (InAirIdle1ByWallJump == currChd.ch_state()) {
            // Always jump to the opposite direction of wall inward norm
            JPH::Quat currChdQ(currChd.q_x(), currChd.q_y(), currChd.q_z(), currChd.q_w());
            Vec3 currChdFacing = currChdQ*Vec3(1, 0, 0);
            float xfac = (0 > currChdFacing.GetX() ? 1 : -1);
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

    // [WARNING] No change of "chd.q_*" within this function!
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
                    nextChd->set_q_x(0);
                    nextChd->set_q_y(0);
                    nextChd->set_q_z(0);
                    nextChd->set_q_w(1);
                } else {
                    // 0 > newVelX, as "0 < effDx * newVelX" it wouldn't be 0
                    nextChd->set_q_x(cTurnbackAroundYAxis.GetX());
                    nextChd->set_q_y(cTurnbackAroundYAxis.GetY());
                    nextChd->set_q_z(cTurnbackAroundYAxis.GetZ());
                    nextChd->set_q_w(cTurnbackAroundYAxis.GetW());
                }

            } else {
                if (0 < effDx) {
                    nextChd->set_q_x(0);
                    nextChd->set_q_y(0);
                    nextChd->set_q_z(0);
                    nextChd->set_q_w(1);
                } else {
                    // 0 > effDx, as "0 < effDx * newVelX" it wouldn't be 0
                    nextChd->set_q_x(cTurnbackAroundYAxis.GetX());
                    nextChd->set_q_y(cTurnbackAroundYAxis.GetY());
                    nextChd->set_q_z(cTurnbackAroundYAxis.GetZ());
                    nextChd->set_q_w(cTurnbackAroundYAxis.GetW());
                }

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
                if (exactTurningAround && cc->has_turn_around_anim()) {
                    if (currEffInAir) {
                        nextChd->set_ch_state(InAirTurnAround);
                    } else {
                        nextChd->set_ch_state(TurnAround);
                    }
                }
            }
        }
    } else {
        // 0 == effDx or speed is zero
        if (0 != effDx) {
            // false == hasNonZeroSpeed, no need to handle velocity lerping
            if (0 < effDx) {
                nextChd->set_q_x(0);
                nextChd->set_q_y(0);
                nextChd->set_q_z(0);
                nextChd->set_q_w(1);
            } else {
                // 0 > effDx, as "0 < effDx * newVelX" it wouldn't be 0
                nextChd->set_q_x(cTurnbackAroundYAxis.GetX());
                nextChd->set_q_y(cTurnbackAroundYAxis.GetY());
                nextChd->set_q_z(cTurnbackAroundYAxis.GetZ());
                nextChd->set_q_w(cTurnbackAroundYAxis.GetW());
            }
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
    }
}

void BaseBattle::processInertiaFlying(int rdfId, float dt, const CharacterDownsync& currChd, CharacterDownsync* nextChd, int effDx, int effDy, const CharacterConfig* cc, bool currParalyzed, bool currInBlockStun) {
    // TBD
    return;
}

bool BaseBattle::addNewExplosionToNextFrame(int currRdfId, RenderFrame* nextRdf, const Bullet* referenceBullet, const CollideShapeResult& inResult) {
    uint32_t oldNextRdfBulletCount = mNextRdfBulletCount.fetch_add(1, std::memory_order_relaxed);
    if (oldNextRdfBulletCount >= nextRdf->bullets_size()) {
#ifndef  NDEBUG
        std::ostringstream oss;
        oss << "@currRdfId=" << currRdfId << ", bulletId=" << referenceBullet->id() << ": bullet overwhelming when adding explosion#1";
        --mNextRdfBulletCount;
        Debug::Log(oss.str(), DColor::Orange);
#endif // ! NDEBUG
        return false;
    }

    auto newPos = inResult.mContactPointOn1 + Vec3(referenceBullet->x(), referenceBullet->y(), 0);
    float newOriginatedX = newPos.GetX(), newOriginatedY = newPos.GetY(), newX = newPos.GetX(), newY = newPos.GetY();
    float dstQx = referenceBullet->q_x();
    float dstQy = referenceBullet->q_y();
    float dstQz = referenceBullet->q_z();
    float dstQw = referenceBullet->q_w();

    auto initBlState = BulletState::Exploding;
    int initFramesInBlState = 0;

    int oldNextRdfBulletIdCounter = mNextRdfBulletIdCounter.fetch_add(1, std::memory_order_relaxed);
    auto nextBl = nextRdf->mutable_bullets(oldNextRdfBulletCount);
    nextBl->CopyFrom(*referenceBullet);
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


bool BaseBattle::addNewBulletToNextFrame(int currRdfId, const CharacterDownsync& currChd, CharacterDownsync* nextChd, const CharacterConfig* cc, bool currParalyzed, bool currEffInAir, const Skill* skillConfig, int activeSkillHit, uint32_t activeSkillId, RenderFrame* nextRdf, const Bullet* referenceBullet, const BulletConfig* referenceBulletConfig, uint64_t offenderUd, int bulletTeamId) {
    if (globalPrimitiveConsts->no_skill_hit() == activeSkillHit || activeSkillHit > skillConfig->hits_size()) return false;
    uint32_t oldNextRdfBulletCount = mNextRdfBulletCount.fetch_add(1, std::memory_order_relaxed);
    if (oldNextRdfBulletCount >= nextRdf->bullets_size()) {
#ifndef  NDEBUG
        std::ostringstream oss;
        oss << "@currRdfId=" << currRdfId << ", offenderUd=" << offenderUd << ", oldNextRdfBulletCount=" << oldNextRdfBulletCount << ": bullet overwhelming#1";
        --mNextRdfBulletCount;
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

    JPH::Quat offenderEffQ(currChd.q_x(), currChd.q_y(), currChd.q_z(), currChd.q_w());
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
    auto nextBl = nextRdf->mutable_bullets(oldNextRdfBulletCount);
    nextBl->set_id(oldNextRdfBulletIdCounter);
    nextBl->set_originated_render_frame_id(currRdfId);
    nextBl->set_bl_state(initBlState);
    nextBl->set_frames_in_bl_state(initFramesInBlState);
    auto ud = calcUserData(*nextBl);
    nextBl->set_offender_ud(offenderUd);
    nextBl->set_ud(ud);
    nextBl->set_originated_x(newOriginatedX);
    nextBl->set_originated_y(newOriginatedY);
    nextBl->set_x(newX);
    nextBl->set_y(newY);
    nextBl->set_q_x(blEffQ.GetX());
    nextBl->set_q_y(blEffQ.GetY());
    nextBl->set_q_z(blEffQ.GetZ());
    nextBl->set_q_w(blEffQ.GetW());
    nextBl->set_team_id(currChd.bullet_team_id());

    Vec3Arg blInitVelocity(bulletConfig.speed(), 0, 0);
    auto blEffVelocity = blEffQ*blInitVelocity;
    nextBl->set_vel_x(blEffVelocity.GetX());
    nextBl->set_vel_y(blEffVelocity.GetY());
    nextBl->set_skill_id(activeSkillId);
    nextBl->set_active_skill_hit(activeSkillHit);
    nextBl->set_repeat_quota_left(bulletConfig.repeat_quota());
    nextBl->set_remaining_hard_pushback_bounce_quota(bulletConfig.default_hard_pushback_bounce_quota());
    nextBl->set_exploded_on_ifc(bulletConfig.ifc());

    // [WARNING] This part locks velocity by the last bullet in the simultaneous array
    if (!bulletConfig.delay_self_vel_to_active() && !currParalyzed) {
        if (globalPrimitiveConsts->no_lock_vel() != bulletConfig.self_lock_vel_x()) {
            Vec3 initSelfLockVel(bulletConfig.self_lock_vel_x(), bulletConfig.self_lock_vel_y(), 0);
            Vec3 effSelfLockVel = offenderEffQ*initSelfLockVel;
            nextChd->set_vel_x(effSelfLockVel.GetX());
            nextChd->set_vel_y(effSelfLockVel.GetY());
            nextChd->set_vel_z(effSelfLockVel.GetZ());
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

/*
#ifndef  NDEBUG
    std::ostringstream oss;
    oss << "@currRdfId=" << currRdfId << ", added new bullet with bulletId=" << nextBl->id() << ", pos=(" << nextBl->x() << ", " << nextBl->y() << ", " << nextBl->z() << "), vel=(" << nextBl->vel_x() << ", " << nextBl->vel_y() << ", " << nextBl->vel_z() << ")";
    Debug::Log(oss.str(), DColor::Orange);
#endif // ! NDEBUG
*/
    if (0 < bulletConfig.simultaneous_multi_hit_cnt() && activeSkillHit < skillConfig->hits_size()) {
        return addNewBulletToNextFrame(currRdfId, currChd, nextChd, cc, currParalyzed, currEffInAir, skillConfig, activeSkillHit + 1, activeSkillId, nextRdf, referenceBullet, referenceBulletConfig, offenderUd, bulletTeamId);
    } else {
        return true;
    }
}

void BaseBattle::elapse1RdfForRdf(int currRdfId, RenderFrame* nextRdf) {
    for (int i = 0; i < playersCnt; i++) {
        auto player = nextRdf->mutable_players_arr(i);
        const CharacterConfig* cc = getCc(player->chd().species_id());
        elapse1RdfForPlayerChd(currRdfId, player, cc);
    }

    for (int i = 0; i < nextRdf->npcs_arr_size(); i++) {
        auto npc = nextRdf->mutable_npcs_arr(i);
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

    for (int i = 0; i < nextRdf->triggers_arr_size(); i++) {
        auto tr = nextRdf->mutable_triggers_arr(i);
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
    elapse1RdfForChd(currRdfId, chd, cc);
    playerChd->set_not_enough_mp_hint_rdf_countdown(0 < playerChd->not_enough_mp_hint_rdf_countdown() ? playerChd->not_enough_mp_hint_rdf_countdown() - 1 : 0);

}

void BaseBattle::elapse1RdfForNpcChd(const int currRdfId, NpcCharacterDownsync* npcChd, const CharacterConfig* cc) {
    auto chd = npcChd->mutable_chd();
    elapse1RdfForChd(currRdfId, chd, cc);
    npcChd->set_frames_in_patrol_cue(0 < npcChd->frames_in_patrol_cue() ? npcChd->frames_in_patrol_cue() - 1 : 0);
}

void BaseBattle::elapse1RdfForChd(const int currRdfId, CharacterDownsync* cd, const CharacterConfig* cc) {
    cd->set_frames_to_recover((0 < cd->frames_to_recover() ? cd->frames_to_recover() - 1 : 0));
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

    if (cd->has_inventory()) {
        Inventory* iv = cd->mutable_inventory(); 
        for (int i = 0; i < iv->slots_size(); ++i) {
            InventorySlot* cand = iv->mutable_slots(i);
            if (InventorySlotStockType::NoneIv == cand->stock_type()) break;
            const InventorySlotConfig* ivsConfig = &(cc->init_inventory_slots(i));
            elapse1RdfForIvSlot(cand, ivsConfig);
        }
    }

    leftShiftDeadBlImmuneRecords(currRdfId, cd);
    leftShiftDeadIvSlots(currRdfId, cd);
    leftShiftDeadBuffs(currRdfId, cd);
    leftShiftDeadDebuffs(currRdfId, cd);
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
        ifdHolder->set_confirmed_list(0u); // To avoid RingBuffer reuse contamination.
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

    /*
    [WARNING]

    The callstack "getOrCreateCachedCharacterCollider -> createDefaultCharacterCollider" implicitly adds the "JPH::Character.mBodyID" into "BroadPhase", therefore "bodyIDsToAdd" starts here.
    */ 
    bodyIDsToAdd.clear();
    for (int i = 0; i < currRdf->bullets_size(); i++) {
        const Bullet& currBl = currRdf->bullets(i);
        if (globalPrimitiveConsts->terminating_bullet_id() == currBl.id()) break;
        Bullet* nextBl = nextRdf->mutable_bullets(i); // [WARNING] By reaching here, we haven't executed "leftShiftDeadBullets", hence the indices of "currRdf->bullets" and "nextRdf->bullets" are FULLY ALIGNED.
        auto ud = calcUserData(currBl);
        const Skill* skill = nullptr;
        const BulletConfig* bulletConfig = nullptr;
        FindBulletConfig(currBl.skill_id(), currBl.active_skill_hit(), skill, bulletConfig);
        if (isBulletActive(&currBl)) {
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
        BL_COLLIDER_T* single = activeBlColliders.back();
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
    if (!bodyIDsToClear.empty()) {
        bi->DeactivateBodies(bodyIDsToClear.data(), bodyIDsToClear.size());
    }

    while (!activeNonContactConstraints.empty()) {
        NON_CONTACT_CONSTRAINT_T* single = activeNonContactConstraints.back();
        activeNonContactConstraints.pop_back();
        NON_CONTACT_CONSTRAINT_KEY_T nonContactConstraintCacheKey = std::make_pair<JPH::EConstraintType, JPH::EConstraintSubType>(single->GetType(), single->GetSubType());

        /*
        [WARNING] This removal operation is O(1) time-complexity, see the underlying implementation of "ConstraintManager::Remove".

        I've also considered using "JPH::Constraint.SetEnabled(false)" as an alternative here, yet "JPH::PhysicsSystem.RemoveConstraint(single)" seems fast enough as well as more comforting for the same purpose. 
        */
        phySys->RemoveConstraint(single);

        auto it = cachedNonContactConstraints.find(nonContactConstraintCacheKey);
        if (it == cachedNonContactConstraints.end()) {
            NON_CONTACT_CONSTRAINT_Q q = { single };
            cachedNonContactConstraints.emplace(nonContactConstraintCacheKey, q);
        } else {
            auto& cacheQue = it->second;
            cacheQue.push_back(single);
        }
    }

    // This function will remove or deactivate all bodies attached to "phySys", so this mapping cache will certainly become invalid.
    transientUdToChCollider.clear();
    transientUdToBodyID.clear();

    transientUdToCurrPlayer.clear();
    transientUdToNextPlayer.clear();

    transientUdToCurrNpc.clear();
    transientUdToNextNpc.clear();

    transientUdToCurrBl.clear();
    transientUdToNextBl.clear();

    transientUdToCurrTrigger.clear();
    transientUdToNextTrigger.clear();

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
    } else if (WalkingAtk1 == currChd.ch_state() || WalkingAtk4 == currChd.ch_state()) {
        outEffDx = ioIfDecoded.dx();
    } else if (isInBlockStun(currChd)) {
        // Reserve only "effDy" for later use by "useSkill", e.g. to break free from block-stun by certain skills.
        outEffDy = ioIfDecoded.dy();
    }

    outPatternId = globalPrimitiveConsts->pattern_id_no_op();
    JPH::Quat currChdQ(currChd.q_x(), currChd.q_y(), currChd.q_z(), currChd.q_w());
    Vec3 currChdFacing = currChdQ*Vec3(1, 0, 0);
    int effFrontOrBack = (ioIfDecoded.dx() * currChdFacing.GetX()); // [WARNING] Deliberately using "ifDecoded.dx()" instead of "effDx (which could be 0 in block stun)" here!
    bool canJumpWithinInertia = (0 == currChd.frames_to_recover()) || !notDashing;
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

void BaseBattle::processSingleCharacterInput(int rdfId, float dt, int patternId, bool jumpedOrNot, bool slipJumpedOrNot, int effDx, int effDy, bool slowDownToAvoidOverlap, const CharacterDownsync& currChd, uint64_t ud, bool currEffInAir, bool currCrouching, bool currOnWall, bool currDashing, bool currWalking, bool currInBlockStun, bool currAtked, bool currParalyzed, const CharacterConfig* cc, CharacterDownsync* nextChd, RenderFrame* nextRdf, bool& usedSkill) {
    bool slotUsed = false;
    uint32_t slotLockedSkillId = globalPrimitiveConsts->no_skill();
    bool dodgedInBlockStun = false;
    // TODO: Call "useInventorySlot" appropriately 

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
        transitToGroundDodgedChState(currChd, nextChd, cc, currParalyzed);
    }

    int outSkillId = globalPrimitiveConsts->no_skill();
    const Skill* outSkillConfig = nullptr;
    const BulletConfig* outPivotBc = nullptr;

    usedSkill = dodgedInBlockStun ? false : useSkill(rdfId, nextRdf, currChd, ud, cc, nextChd, effDx, effDy, patternId, currEffInAir, currCrouching, currOnWall, currDashing, currWalking, currInBlockStun, currAtked, currParalyzed, outSkillId, outSkillConfig, outPivotBc);

    /*
    if (cc->btn_b_auto_unhold_ch_states().contains(nextChd->ch_state())) {
        // [WARNING] For "autofire" skills.
        nextChd->set_btn_b_holding_rdf_cnt(0);
    }
    */

    if (usedSkill) {
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
    } else {
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

        if (0 >= currChd.frames_to_recover()) {
            if (globalPrimitiveConsts->pattern_id_unable_to_op() != patternId && cc->anti_gravity_when_idle() && (Walking == nextChd->ch_state() || InAirWalking == nextChd->ch_state()) && cc->anti_gravity_frames_lingering() < nextChd->frames_in_ch_state()) {
                nextChd->set_ch_state(InAirIdle1NoJump);
                nextChd->set_frames_in_ch_state(0);
                nextChd->set_vel_x(0);
            } else if (slowDownToAvoidOverlap) {
                nextChd->set_vel_x(nextChd->vel_x() * 0.25);
                nextChd->set_vel_y(nextChd->vel_y() * 0.25);
            }
        }
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

    JPH::Quat currChdQ(currChd.q_x(), currChd.q_y(), currChd.q_z(), currChd.q_w());
    Vec3 currChdFacing = currChdQ*Vec3(1, 0, 0);
    int xfac = (0 < currChdFacing.GetX() ? 1 : -1);
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
            case WalkStopping:
                if (cc->omit_gravity()) {
                    // [WARNING] No need to distinguish in this case.
                    break;
                } else {
                    if (cc->has_in_air_walk_stopping_anim()) {
                        nextChd->set_ch_state(InAirWalkStopping);
                    }
                    break;
                }
                break;
            }
        }
    } else {
        // next frame NOT in air
        if (oldNextEffInAir && nonAttackingSet.count(oldNextChState) && !onWallSet.count(oldNextChState) && BlownUp1 != oldNextChState) {
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
            case InAirWalkStopping:
                nextChd->set_ch_state(WalkStopping);
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
            if (globalPrimitiveConsts->no_skill() != nextActiveSkillId && globalPrimitiveConsts->no_skill_hit() != nextActiveSkillHit) {
                const Skill* nextActiveSkill = nullptr;
                const BulletConfig* nextActiveBulletConfig = nullptr;
                FindBulletConfig(nextActiveSkillId, nextActiveSkillHit, nextActiveSkill, nextActiveBulletConfig);
                JPH_ASSERT(nullptr != nextActiveSkill && nullptr != nextActiveBulletConfig);
                int newFramesInChState = currChd.frames_in_ch_state() + 1;
                newFramesInChState %= nextActiveSkill->recovery_frames(); // [TODO] Enhance this for GUI smoothness.
                nextChd->set_frames_in_ch_state(newFramesInChState);
            } else {
#ifndef NDEBUG
                std::ostringstream oss1;
                oss1 << "@currRdfId=" << currRdfId << ", chd ud=" << ud << " will transit back into Walking, orig next_ch_state=" << nextChd->ch_state() << ", orig next_frames_in_ch_state=" << nextChd->frames_in_ch_state();
                Debug::Log(oss1.str(), DColor::Orange);
#endif // !NDEBUG
                nextChd->set_ch_state(Walking);
            }
        } else if ((WalkingAtk1 == currChd.ch_state() || WalkingAtk4 == currChd.ch_state()) && Walking == nextChd->ch_state()) {
            nextChd->set_frames_in_ch_state(currChd.frames_in_ch_state() + 1);
        } else if ((Atk1 == currChd.ch_state() && WalkingAtk1 == nextChd->ch_state()) || (Atk4 == currChd.ch_state() && WalkingAtk4 == nextChd->ch_state())) {
            nextChd->set_frames_in_ch_state(currChd.frames_in_ch_state() + 1);
        } else if ((WalkingAtk1 == currChd.ch_state() && Atk1 == nextChd->ch_state()) || (WalkingAtk4 == currChd.ch_state() && Atk4 == nextChd->ch_state())) {
            nextChd->set_frames_in_ch_state(currChd.frames_in_ch_state() + 1);
        } else if ((Walking == currChd.ch_state() && InAirWalking == nextChd->ch_state()) || (InAirWalking == currChd.ch_state() && Walking == nextChd->ch_state())) {
            nextChd->set_frames_in_ch_state(currChd.frames_in_ch_state() + 1);
        } else if ((TurnAround == currChd.ch_state() && InAirTurnAround == nextChd->ch_state()) || (InAirTurnAround == currChd.ch_state() && TurnAround == nextChd->ch_state())) {
            nextChd->set_frames_in_ch_state(currChd.frames_in_ch_state() + 1);
        } else if ((WalkStopping == currChd.ch_state() && InAirWalkStopping == nextChd->ch_state()) || (InAirWalkStopping == currChd.ch_state() && WalkStopping == nextChd->ch_state())) {
            nextChd->set_frames_in_ch_state(currChd.frames_in_ch_state() + 1);
        } else {
            nextChd->set_frames_in_ch_state(0);
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
    int aliveI = 0, candI = 0;
    auto& characterConfigs = globalConfigConsts->character_configs();
    while (candI < nextRdf->npcs_arr_size()) {
        const NpcCharacterDownsync* candidate = &(nextRdf->npcs_arr(candI));
        if (globalPrimitiveConsts->terminating_character_id() == candidate->id()) break;
        const CharacterDownsync& chd = candidate->chd();
        const CharacterConfig& candidateConfig = characterConfigs.at(chd.species_id());

        /*
        // TODO: Drop pickable
        */
        
        while (candI < nextRdf->npcs_arr_size() && globalPrimitiveConsts->terminating_character_id() != candidate->id() && isNpcDeadToDisappear(&chd)) {
            if (globalPrimitiveConsts->terminating_trigger_id() != candidate->publishing_to_trigger_id_upon_exhausted()) {
                auto triggerUd = calcPublishingToTriggerUd(*candidate);
                auto targetTriggerInNextFrame = transientUdToNextTrigger[triggerUd];
                if (0 == chd.last_damaged_by_ud() && globalPrimitiveConsts->terminating_bullet_team_id() == chd.last_damaged_by_bullet_team_id()) {
                    if (1 == playersCnt) {
                        publishNpcExhaustedEvt(currRdfId, candidate->publishing_evt_mask_upon_exhausted(), 1, 1, targetTriggerInNextFrame);
                    } else {
#ifndef  NDEBUG
                        // Debug::Log("@rdfId=" + rdfId + " publishing evtMask=" + candidate.PublishingEvtMaskUponExhausted + " to trigger " + targetTriggerInNextFrame.ToString() + " with no join index and no bullet team id!");
#endif // ! NDEBUG
                        publishNpcExhaustedEvt(currRdfId, candidate->publishing_evt_mask_upon_exhausted(), chd.last_damaged_by_ud(), chd.last_damaged_by_bullet_team_id(), targetTriggerInNextFrame);
                    }
                } else {
                    publishNpcExhaustedEvt(currRdfId, candidate->publishing_evt_mask_upon_exhausted(), chd.last_damaged_by_ud(), chd.last_damaged_by_bullet_team_id(), targetTriggerInNextFrame);
                }
            }
            candI++;
            if (candI >= nextRdf->npcs_arr_size()) break;
            candidate = &(nextRdf->npcs_arr(candI));
        }

        if (candI >= nextRdf->npcs_arr_size() || globalPrimitiveConsts->terminating_character_id() == nextRdf->npcs_arr(candI).id()) {
            break;
        }

        if (candI != aliveI) {
            const NpcCharacterDownsync& src = nextRdf->npcs_arr(candI);
            auto dst = nextRdf->mutable_npcs_arr(aliveI);
            dst->Clear();
            dst->CopyFrom(src);
        }

        candI++;
        aliveI++;
    }
    if (aliveI < nextRdf->npcs_arr_size()) {
        auto terminatingCand = nextRdf->mutable_npcs_arr(aliveI);
        terminatingCand->set_id(globalPrimitiveConsts->terminating_character_id());
    }
    nextRdf->set_npc_count(aliveI);
}

void BaseBattle::calcFallenDeath(const RenderFrame* currRdf, RenderFrame* nextRdf) {
    auto& chConfigs = globalConfigConsts->character_configs();

    int currRdfId = currRdf->id();
    for (int i = 0; i < nextRdf->players_arr_size(); i++) {
        const PlayerCharacterDownsync& currPlayer = currRdf->players_arr(i);
        auto nextPlayer = nextRdf->mutable_players_arr(i);
        auto chd = nextPlayer->mutable_chd();
        auto chConfig = chConfigs.at(chd->species_id());
        float chTop = chd->y() + 2*chConfig.capsule_half_height();
        if (fallenDeathHeight > chTop && Dying != chd->ch_state()) {
            transitToDying(currRdfId, currPlayer, true, nextPlayer);
        }
    }

    for (int i = 0; i < nextRdf->npcs_arr_size(); i++) {
        const NpcCharacterDownsync& currNpc = currRdf->npcs_arr(i);
        if (globalPrimitiveConsts->terminating_character_id() == currNpc.id()) break;
        auto nextNpc = nextRdf->mutable_npcs_arr(i);
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
        auto pickableTop = nextPickable->y() + globalPrimitiveConsts->default_pickable_hitbox_half_size_y();
        if (fallenDeathHeight > pickableTop && PickableState::PIdle == nextPickable->pk_state()) {
            nextPickable->set_pk_state(PickableState::PDisappearing);
            nextPickable->set_frames_in_pk_state(0);
            nextPickable->set_remaining_lifetime_rdf_count(globalPrimitiveConsts->default_pickable_disappearing_anim_frames());
        }
    }
}

void BaseBattle::leftShiftDeadBullets(int currRdfId, RenderFrame* nextRdf) {
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
            if (candI >= nextRdf->bullets_size()) break;
            candidate = &(nextRdf->bullets(candI));
        }

        if (candI >= nextRdf->bullets_size() || globalPrimitiveConsts->terminating_bullet_id() == nextRdf->bullets(candI).id()) {
            break;
        }

        if (candI != aliveI) {
            auto src = nextRdf->bullets(candI);
            auto dst = nextRdf->mutable_bullets(aliveI);
            dst->Clear();
            dst->CopyFrom(src);
        }

        candI++;
        aliveI++;
    }
    if (aliveI < nextRdf->bullets_size()) {
        auto terminatingCand = nextRdf->mutable_bullets(aliveI);
        terminatingCand->set_id(globalPrimitiveConsts->terminating_bullet_id());
    }
    mNextRdfBulletCount = aliveI;
    nextRdf->set_bullet_count(aliveI);
}

void BaseBattle::leftShiftDeadPickables(int currRdfId, RenderFrame* nextRdf) {
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
            auto dst = nextRdf->mutable_pickables(aliveI);
            dst->Clear();
            dst->CopyFrom(src);
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

void BaseBattle::leftShiftDeadTriggers(int currRdfId, RenderFrame* nextRdf) {
    int aliveI = 0, candI = 0;
    while (candI < nextRdf->triggers_arr_size()) {
        const Trigger* cand = &(nextRdf->triggers_arr(candI));
        if (globalPrimitiveConsts->terminating_trigger_id() == cand->id()) {
            break;
        }
        while (candI < nextRdf->triggers_arr_size() && globalPrimitiveConsts->terminating_trigger_id() != cand->id() && !isTriggerAlive(cand, currRdfId)) {
            candI++;
            if (candI >= nextRdf->triggers_arr_size()) break;
            cand = &(nextRdf->triggers_arr(candI));
        }

        if (candI >= nextRdf->triggers_arr_size() || globalPrimitiveConsts->terminating_trigger_id() == nextRdf->triggers_arr(candI).id()) {
            break;
        }

        if (candI != aliveI) {
            const Trigger& src = nextRdf->triggers_arr(candI);
            auto dst = nextRdf->mutable_triggers_arr(aliveI);
            dst->Clear();
            dst->CopyFrom(src);
        }

        candI++;
        aliveI++;
    }
    if (aliveI < nextRdf->triggers_arr_size()) {
        auto terminatingCand = nextRdf->mutable_triggers_arr(aliveI);
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
            dst->Clear();
            dst->CopyFrom(src);
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

void BaseBattle::leftShiftDeadIvSlots(int currRdfId, CharacterDownsync* nextChd) {
    int aliveI = 0, candI = 0;
    auto nextInventory = nextChd->mutable_inventory(); 
    while (candI < nextInventory->slots_size()) {
        const InventorySlot* cand = &(nextInventory->slots(candI));
        if (InventorySlotStockType::NoneIv == cand->stock_type()) {
            break;
        }

        while (InventorySlotStockType::NoneIv != cand->stock_type() && 0 >= cand->quota()) {
            candI++;
            if (candI >= nextInventory->slots_size()) break;
            cand = &(nextInventory->slots(candI));
        }

        if (candI >= nextInventory->slots_size() || InventorySlotStockType::NoneIv == nextInventory->slots(candI).stock_type()) {
            break;
        }

        if (candI != aliveI) {
            const InventorySlot& src = nextInventory->slots(candI);
            auto dst = nextInventory->mutable_slots(aliveI);
            dst->Clear();
            dst->CopyFrom(src);
        }

        candI++;
        aliveI++;
    }

    if (aliveI < nextInventory->slots_size()) {
        auto terminatingCand = nextInventory->mutable_slots(aliveI);
        terminatingCand->set_stock_type(InventorySlotStockType::NoneIv);
        terminatingCand->set_quota(0);
    }
}

void BaseBattle::leftShiftDeadBuffs(int currRdfId, CharacterDownsync* nextChd) {
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
            auto dst = nextChd->mutable_buff_list(aliveI);
            dst->Clear();
            dst->CopyFrom(src);
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

void BaseBattle::leftShiftDeadDebuffs(int currRdfId, CharacterDownsync* nextChd) {
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
            auto dst = nextChd->mutable_debuff_list(aliveI);
            dst->Clear();
            dst->CopyFrom(src);
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

bool BaseBattle::useSkill(int currRdfId, RenderFrame* nextRdf, const CharacterDownsync& currChd, uint64_t ud, const CharacterConfig* cc, CharacterDownsync* nextChd, int effDx, int effDy, int patternId, bool currEffInAir, bool currCrouching, bool currOnWall, bool currDashing, bool currWalking, bool currInBlockStun, bool currAtked, bool currParalyzed, int& outSkillId, const Skill*& outSkill, const BulletConfig*& outPivotBc) {
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

    const ::google::protobuf::Map<uint32, Skill>& skillConfigs = globalConfigConsts->skill_configs();
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
        auto initSkillDict = cc->init_skill_transit();
        if (!initSkillDict.count(encodedPattern)) {
/*
#ifndef NDEBUG
            std::ostringstream oss;
            oss << "@currRdfId=" << currRdfId << ", ud=" << ud << " tries to use init skill by (patternId=" << patternId << ", effDx=" << effDx << ", effDy=" << effDy << ", encodedPattern=" << encodedPattern << "), but initSkillDict doesn't contain it!";
            Debug::Log(oss.str(), DColor::Yellow);
#endif // !NDEBUG
*/
            return false;
        }
        targetSkillId = initSkillDict[encodedPattern];
#ifndef NDEBUG
        std::ostringstream oss2;
        oss2 << "@currRdfId=" << currRdfId << ", ud=" << ud << " tries to use init skill by (currX=" << currChd.x() << ", currY=" << currChd.y() << ", currVelX=" << currChd.vel_x() << ", " << ", currVelY=" << currChd.vel_y() << ", patternId=" << patternId << ", effDx=" << effDx << ", effDy=" << effDy << ", encodedPattern=" << encodedPattern << "), targetSkillId=" << targetSkillId << " selected";
        Debug::Log(oss2.str(), DColor::Orange);
#endif // !NDEBUG
    }

    if (!skillConfigs.count(targetSkillId)) {
#ifndef NDEBUG
         std::ostringstream oss;
         oss << "@currRdfId=" << currRdfId << ", ud=" << ud << ", skillConfigs size=" << skillConfigs.size() << " doesn't contain targetSkillId=" << targetSkillId;
         Debug::Log(oss.str(), DColor::Yellow);
#endif // !NDEBUG
        return false;
    }

    auto targetSkillConfig = skillConfigs.at(targetSkillId);

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
    auto pivotBulletConfig = targetSkillConfig.hits(nextActiveSkillHit - 1);
    outPivotBc = &pivotBulletConfig;

    nextChd->set_active_skill_id(targetSkillId);
    nextChd->set_frames_to_recover(targetSkillConfig.recovery_frames());

    const Bullet* referenceBullet = nullptr;
    const BulletConfig* referenceBulletConfig = nullptr;
    for (int i = 0; i < pivotBulletConfig.simultaneous_multi_hit_cnt() + 1; i++) {
        if (!addNewBulletToNextFrame(currRdfId, currChd, nextChd, cc, currParalyzed, currEffInAir, outSkill, nextActiveSkillHit, outSkillId, nextRdf, referenceBullet, referenceBulletConfig, ud, currChd.bullet_team_id())) {
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

#ifndef NDEBUG
    std::ostringstream oss3;
    oss3 << "@currRdfId=" << currRdfId << ", ud=" << ud << " used targetSkillId=" << targetSkillId << " by (currChState=" << currChd.ch_state() << ", currFramesInChState=" << currChd.frames_in_ch_state() << ", currEffInAir=" << currEffInAir << ", framesToStartJump=" << currChd.frames_to_start_jump() << "), (nextChState=" << nextChd->ch_state() << ", nextFramesInChState=" << nextChd->frames_in_ch_state() << ", nextX=" << nextChd->x() << ", nextY=" << nextChd->y() << ", nextVelX=" << nextChd->vel_x() << ", " << ", nextVelY=" << nextChd->vel_y() << ", patternId=" << patternId << ", effDx=" << effDx << ", effDy=" << effDy << ")";
    Debug::Log(oss3.str(), DColor::Orange);
#endif // !NDEBUG
    return true;
}

void BaseBattle::useInventorySlot(int currRdfId, int patternId, const CharacterDownsync& currChd, const CharacterConfig* cc, CharacterDownsync* nextChd, bool& outSlotUsed, bool& outDodgedInBlockStun) {
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
    chShapeCenterAnchor->SetDensity(cDefaultChDensity);
    RotatedTranslatedShape* chShape = new RotatedTranslatedShape(Vec3(0, cc->capsule_half_height() + cc->capsule_radius(), 0), Quat::sIdentity(), chShapeCenterAnchor); // lifecycle to be held by "CharacterVirtual::mShape"

    Ref<CharacterSettings> settings = new CharacterSettings();
    settings->mMaxSlopeAngle = cMaxSlopeAngle;
    settings->mLayer = MyObjectLayers::MOVING;
    settings->mFriction = cDefaultFriction;
    settings->mSupportingVolume = Plane(Vec3::sAxisY(), -cc->capsule_radius()); // Accept contacts that touch the lower sphere of the capsule
    settings->mEnhancedInternalEdgeRemoval = cEnhancedInternalEdgeRemoval;
    settings->mShape = chShape;
    settings->mMass = chShape->GetMassProperties().mMass;
    auto ret = new Character(settings, safeDeactiviatedPosition, Quat::sIdentity(), 0, phySys);
	ret->AddToPhysicsSystem(EActivation::DontActivate);
    
    return ret;
}

Body* BaseBattle::createDefaultBulletCollider(const float immediateBoxHalfSizeX, const float immediateBoxHalfSizeY, float& outConvexRadius, const EMotionType immediateMotionType, bool isSensor) {
    outConvexRadius = (immediateBoxHalfSizeX + immediateBoxHalfSizeY) * 0.5;
    if (cDefaultHalfThickness < outConvexRadius) {
        outConvexRadius = cDefaultHalfThickness; // Required by the underlying body creation 
    }
    Vec3 halfExtent(immediateBoxHalfSizeX, immediateBoxHalfSizeY, cDefaultHalfThickness);
    BoxShapeSettings* settings = new BoxShapeSettings(halfExtent, outConvexRadius); // transient, to be discarded after creating "body"
    settings->mDensity = cDefaultBlDensity;
    BodyCreationSettings bodyCreationSettings(settings, safeDeactiviatedPosition, JPH::Quat::sIdentity(), immediateMotionType, MyObjectLayers::MOVING);
    bodyCreationSettings.mAllowDynamicOrKinematic = true;
    bodyCreationSettings.mGravityFactor = 0; // TODO
    bodyCreationSettings.mIsSensor = isSensor; // TODO
    Body* body = bi->CreateBody(bodyCreationSettings);
    JPH_ASSERT(nullptr != body);
    blStockCache->push_back(body);

    return body;
}

void BaseBattle::preallocateBodies(const RenderFrame* currRdf, const ::google::protobuf::Map< uint32_t, uint32_t >& preallocateNpcSpeciesDict) {
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
    single->set_q_x(0);
    single->set_q_y(0);
    single->set_q_z(0);
    single->set_q_w(1);
}

void BaseBattle::NewPreallocatedCharacterDownsync(CharacterDownsync* single, int buffCapacity, int debuffCapacity, int inventoryCapacity, int bulletImmuneRecordCapacity) {
    single->set_q_x(0);
    single->set_q_y(0);
    single->set_q_z(0);
    single->set_q_w(1);

    single->set_aiming_q_x(0);
    single->set_aiming_q_y(0);
    single->set_aiming_q_z(0);
    single->set_aiming_q_w(1);

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

    // Intentionally left blank by the time of writing.
}

void BaseBattle::stepSingleChdState(const int currRdfId, const RenderFrame* currRdf, RenderFrame* nextRdf, const float dt, const uint64_t ud, const uint64_t udt, const CharacterConfig* cc, CH_COLLIDER_T* single, const CharacterDownsync& currChd, CharacterDownsync* nextChd, bool& groundBodyIsChCollider, bool& isDead, bool& cvOnWall, bool& cvSupported, bool& cvInAir, bool& inJumpStartupOrJustEnded, CharacterBase::EGroundState& cvGroundState) {
    auto bodyID = single->GetBodyID();

    RVec3 newPos;
    Quat newRot;
    single->GetPositionAndRotation(newPos, newRot);
    auto newVel = single->GetLinearVelocity();
    CharacterCollideShapeCollector collector(currRdfId, nextRdf, bi, ud, udt, &currChd, nextChd, single->GetUp(), newPos, this); // Aggregates "CharacterBase.mGroundXxx" properties in a same "KernelThread"

    const NarrowPhaseQuery& narrowPhaseQueryNoLock = phySys->GetNarrowPhaseQueryNoLock(); // no need to lock after physics update

    // Collide shape
    auto chCOMTransform = bi->GetCenterOfMassTransform(bodyID);

    // Settings for collide shape
    CollideShapeSettings settings;
    settings.mMaxSeparationDistance = cCollisionTolerance;
    settings.mActiveEdgeMode = EActiveEdgeMode::CollideOnlyWithActive;
    settings.mActiveEdgeMovementDirection = newVel;
    settings.mBackFaceMode = EBackFaceMode::IgnoreBackFaces;

    class CharacterBodyFilter : public IgnoreSingleBodyFilter {
    public:
        using			IgnoreSingleBodyFilter::IgnoreSingleBodyFilter;

        virtual bool	ShouldCollideLocked(const Body& inBody) const override
        {
            return true; // Don't skip when "inBody" is sensor!
        }
    };
    CharacterBodyFilter chBodyFilter(bodyID);
    narrowPhaseQueryNoLock.CollideShape(single->GetShape(), Vec3::sOne(), chCOMTransform, settings, newPos, collector, phySys->GetDefaultBroadPhaseLayerFilter(MyObjectLayers::MOVING), phySys->GetDefaultLayerFilter(MyObjectLayers::MOVING), chBodyFilter);
    
    // Copy results
    single->SetGroundBodyID(collector.mGroundBodyID, collector.mGroundBodySubShapeID);
    single->SetGroundBodyPosition(collector.mGroundPosition, collector.mGroundNormal);

    if (!collector.mGroundBodyID.IsInvalid()) {
        // Update ground state
        RMat44 inv_transform = RMat44::sInverseRotationTranslation(newRot, newPos);
        if (single->GetSupportingVolume()->SignedDistance(Vec3(inv_transform * collector.mGroundPosition)) > 0.0f)
            single->SetGroundState(JPH::CharacterBase::EGroundState::NotSupported);
        else if (single->IsSlopeTooSteep(collector.mGroundNormal))
            single->SetGroundState(JPH::CharacterBase::EGroundState::OnSteepGround);
        else
            single->SetGroundState(JPH::CharacterBase::EGroundState::OnGround);

        // Copy other body properties
        single->SetGroundBodyUd(bi->GetMaterial(collector.mGroundBodyID, collector.mGroundBodySubShapeID), bi->GetPointVelocity(collector.mGroundBodyID, collector.mGroundPosition), bi->GetUserData(collector.mGroundBodyID));
    } else {
        single->SetGroundState(JPH::CharacterBase::EGroundState::InAir);
        single->SetGroundBodyUd(JPH::PhysicsMaterial::sDefault, Vec3::sZero(), 0);
    }

    inJumpStartupOrJustEnded = (isInJumpStartup(*nextChd, cc) || isJumpStartupJustEnded(currChd, nextChd, cc));
    cvGroundState = single->GetGroundState();

    auto groundBodyID = single->GetGroundBodyID();
    auto groundBodyUd = single->GetGroundUserData();
    groundBodyIsChCollider = transientUdToChCollider.count(groundBodyUd);
    JPH::Quat nextChdQ(nextChd->q_x(), nextChd->q_y(), nextChd->q_z(), nextChd->q_w());
    Vec3 nextChdFacing = nextChdQ*Vec3(1, 0, 0);
    float groundNormalAlignment = nextChdFacing.Dot(single->GetGroundNormal());
    cvOnWall = (0 > groundNormalAlignment && !groundBodyID.IsInvalid() && !groundBodyIsChCollider && single->IsSlopeTooSteep(single->GetGroundNormal()));
    cvSupported = single->IsSupported() && !cvOnWall && !groundBodyID.IsInvalid(); // [WARNING] "cvOnWall" and  "cvSupported" are mutually exclusive in this game!
    /* [WARNING]
    When a "CapsuleShape" is colliding with a "MeshShape", some unexpected z-offset might be caused by triangular pieces. We have to compensate for such unexpected z-offsets by setting the z-components to 0.

    The process of unexpected z-offsets being introduced can be tracked in
    - [CollideConvexVsTriangles::Collide](https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Physics/Collision/CollideConvexVsTriangles.cpp#L137)
    - [PhysicsSystem::ProcessBodyPair::ReductionCollideShapeCollector::AddHit](https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Physics/PhysicsSystem.cpp#L1105)
    - [PhysicsSystem::ProcessBodyPair](https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Physics/PhysicsSystem.cpp#L1165)
    */
    nextChd->set_x(newPos.GetX());
    nextChd->set_y(newPos.GetY());
    nextChd->set_z(0);
    nextChd->set_vel_x(IsLengthNearZero(newVel.GetX()*dt) ? 0 : newVel.GetX());
    nextChd->set_vel_y(IsLengthNearZero(newVel.GetY()*dt) ? 0 : newVel.GetY());
    nextChd->set_vel_z(0);

    isDead = (0 >= nextChd->hp());

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

    cvInAir = (!cvSupported || cvOnWall);
    bool hasBeenOnWallIdle1 = (OnWallIdle1 == nextChd->ch_state() && OnWallIdle1 == currChd.ch_state());
    bool hasBeenOnWallAtk1 = (OnWallAtk1 == nextChd->ch_state() && OnWallAtk1 == currChd.ch_state());
    if ((hasBeenOnWallIdle1 || hasBeenOnWallAtk1) && nextChd->x() != currChd.x()) {
        nextChd->set_x(currChd.x()); // [WARNING] compensation for this known caveat of Jolt with horizontal-position change while GroundNormal is kept unchanged 
    }

    if (0 < collector.newEffDamage) {
        if (!noOpSet.count(nextChd->ch_state())) {
            if (collector.newEffBlownUp) {
                nextChd->set_ch_state(BlownUp1);
            } else {
                if (cvOnWall || cvInAir || inAirSet.count(currChd.ch_state()) || inAirSet.count(nextChd->ch_state())) {
                    nextChd->set_ch_state(InAirAtked1);
                } else {
                    nextChd->set_ch_state(Atked1);
                }
            }
            nextChd->set_frames_to_recover(collector.newEffFramesToRecover);
            if (globalPrimitiveConsts->no_lock_vel() != collector.newEffPushbackVelX) {
                nextChd->set_vel_x(collector.newEffPushbackVelX);
            } 
            if (globalPrimitiveConsts->no_lock_vel() != collector.newEffPushbackVelY) {
                nextChd->set_vel_y(collector.newEffPushbackVelY);
            }
            nextChd->set_hp(nextChd->hp() - collector.newEffDamage);
        } else if (Atked1 == nextChd->ch_state() || InAirAtked1 == nextChd->ch_state()) {
            if (collector.newEffBlownUp) {
                nextChd->set_ch_state(BlownUp1);
            }
            if (collector.newEffFramesToRecover > nextChd->frames_to_recover()) {
                nextChd->set_frames_to_recover(collector.newEffFramesToRecover);
                if (globalPrimitiveConsts->no_lock_vel() != collector.newEffPushbackVelX) {
                    nextChd->set_vel_x(collector.newEffPushbackVelX);
                } 
                if (globalPrimitiveConsts->no_lock_vel() != collector.newEffPushbackVelY) {
                    nextChd->set_vel_y(collector.newEffPushbackVelY);
                }
            }
            nextChd->set_hp(nextChd->hp() - collector.newEffDamage);
        } else if (BlownUp1 == nextChd->ch_state()){
            nextChd->set_hp(nextChd->hp() - collector.newEffDamage);
        } 
        nextChd->set_damaged_hint_rdf_countdown(90); // TODO: Remove this hardcoded constant
    }
}

void BaseBattle::stepSingleTriggerState(int currRdfId, const Trigger& currTrigger, Trigger* nextTrigger) {
    /*
    bool isSubcycleConfigured = (0 < triggerConfigFromTiled.SubCycleQuota);
    var triggerConfig = triggerConfigs[triggerConfigFromTiled.SpeciesId];
    // [WARNING] The ORDER of zero checks of "currTrigger.FramesToRecover" and "currTrigger.FramesToFire" below is important, because we want to avoid "wrong SubCycleQuotaLeft replenishing when 0 == currTrigger.FramesToRecover"!
    
    bool mainCycleFulfilled = (EVTSUB_NO_DEMAND_MASK != currTrigger.DemandedEvtMask && currTrigger.DemandedEvtMask == currTrigger.FulfilledEvtMask);

    if (TRIGGER_SPECIES_TIMED_WAVE_DOOR_1 == triggerConfig.SpeciesId || TRIGGER_SPECIES_TIMED_WAVE_PICKABLE_DROPPER == triggerConfig.SpeciesId) {
        if (0 >= currTrigger.FramesToRecover) {
            if (EVTSUB_NO_DEMAND_MASK == currTrigger.DemandedEvtMask) {
                // If the TimedWaveDoor doesn't subscribe to any other trigger, it should just tick on its own
                if (0 < currTrigger.Quota) {
                    mainCycleFulfilled = true;
                }
                // The exhaust should still be triggered by NPC-exhausted
            } else {
                if (triggerConfigFromTiled.QuotaCap > currTrigger.Quota && 0 < currTrigger.Quota) {
                    // If the TimedWaveDoor subscribes to any other trigger, it should ONLY respect that for the initial firing, the rest firings should still be done by ticking on its own
                    mainCycleFulfilled = true;
                }
                // The exhaust should still be triggered by NPC-exhausted
            }
        } else {
            // [WARNING] Regardless of whether or not the TimedWaveDoor subscribers to any other trigger, when (0 < currTrigger.FramesToRecover), it shouldn't fire EXCEPT FOR all spawned NPC-exhausted 
            if (0 == currTrigger.Quota && EVTSUB_NO_DEMAND_MASK != currTrigger.DemandedEvtMask) {
            } else {
                mainCycleFulfilled = false;
            } 
        }
    }

    bool subCycleFulfilled = (0 >= currTrigger.FramesToFire);
    switch (triggerConfig.SpeciesId) {
    case TRIGGER_SPECIES_VICTORY_TRIGGER_TRIVIAL:
    case TRIGGER_SPECIES_NPC_AWAKER_MV:
    case TRIGGER_SPECIES_BOSS_AWAKER_MV:
        if (mainCycleFulfilled) {
                if (0 < currTrigger.Quota) {
                    triggerInNextFrame.State = TriggerState.TcoolingDown;
                    triggerInNextFrame.FramesInState = 0;
                    triggerInNextFrame.Quota = 0;
                    triggerInNextFrame.FramesToRecover = MAX_INT;
                    triggerInNextFrame.FramesToFire = MAX_INT;
                    justTriggeredBgmId = triggerConfigFromTiled.BgmId;
                    fulfilledTriggerSetMask |= (1UL << (triggerInNextFrame.TriggerLocalId - 1));
                    _notifySubscriberTriggers(currRenderFrame.Id, triggerInNextFrame, nextRenderFrameTriggers, false, false, triggerEditorIdToTiledConfig, logger);
                    //logger.LogInfo(String.Format("@rdfId={0}, one-off trigger editor id = {1}, local id = {2} is fulfilled", currRenderFrame.Id, triggerInNextFrame.EditorId ,triggerInNextFrame.TriggerLocalId));
                    if (0 != triggerConfigFromTiled.NewRevivalX || 0 != triggerConfigFromTiled.NewRevivalY) {
                        if (0 < currTrigger.OffenderJoinIndex && currTrigger.OffenderJoinIndex <= roomCapacity) {
                            var thatCharacterInNextFrame = nextRenderFrame.PlayersArr[currTrigger.OffenderJoinIndex - 1];
                            thatCharacterInNextFrame.RevivalVirtualGridX = triggerConfigFromTiled.NewRevivalX;
                            thatCharacterInNextFrame.RevivalVirtualGridY = triggerConfigFromTiled.NewRevivalY;
                        } else if (1 == roomCapacity) {
                            var thatCharacterInNextFrame = nextRenderFrame.PlayersArr[0];
                            thatCharacterInNextFrame.RevivalVirtualGridX = triggerConfigFromTiled.NewRevivalX;
                            thatCharacterInNextFrame.RevivalVirtualGridY = triggerConfigFromTiled.NewRevivalY;
                        }
                    }   
                }
        }
    break;
    case TRIGGER_SPECIES_TRAP_ATK_TRIGGER_MV:
    case TRIGGER_SPECIES_CTRL_PROMPT_MV:
        mainCycleFulfilled &= (TriggerState.Tready == currTrigger.State);
        if (!isSubcycleConfigured) {
            if (mainCycleFulfilled) {
                if (0 < currTrigger.Quota) {
                    triggerInNextFrame.State = TriggerState.TcoolingDown;
                    triggerInNextFrame.FramesInState = 0;
                    triggerInNextFrame.Quota = currTrigger.Quota - 1;
                    triggerInNextFrame.FramesToRecover = triggerConfigFromTiled.RecoveryFrames;
                    fulfilledTriggerSetMask |= (1UL << (triggerInNextFrame.TriggerLocalId - 1));
                    _notifySubscriberTriggers(currRenderFrame.Id, triggerInNextFrame, nextRenderFrameTriggers, false, false, triggerEditorIdToTiledConfig, logger);
                } else if (0 == currTrigger.Quota) {
                    // Set to exhausted
                    triggerInNextFrame.FulfilledEvtMask = EVTSUB_NO_DEMAND_MASK;
                    triggerInNextFrame.DemandedEvtMask = EVTSUB_NO_DEMAND_MASK;
                }
            } else if (0 == currTrigger.FramesToRecover) {
                // replenish upon mainCycle ends, but "false == mainCycleFulfilled"
                if (0 < currTrigger.Quota) {
                    triggerInNextFrame.State = TriggerState.Tready;
                    triggerInNextFrame.FramesInState = 0;
                }
            }
        } else {
            if (subCycleFulfilled) {
                fireSubscribingTraps(currTrigger, triggerConfigFromTiled, triggerInNextFrame, ref fulfilledTriggerSetMask, currRenderFrame, nextRenderFrameTriggers, triggerEditorIdToTiledConfig, logger);
            } else if (mainCycleFulfilled) {
                if (0 < currTrigger.Quota) {
                    triggerInNextFrame.State = TriggerState.TcoolingDown;
                    triggerInNextFrame.FramesInState = 0;
                    triggerInNextFrame.Quota = currTrigger.Quota - 1;
                    triggerInNextFrame.FramesToRecover = triggerConfigFromTiled.RecoveryFrames;
                    triggerInNextFrame.FramesToFire = triggerConfigFromTiled.DelayedFrames;
                    triggerInNextFrame.SubCycleQuotaLeft = triggerConfigFromTiled.SubCycleQuota;
                } else if (0 == currTrigger.Quota) {
                    // Set to exhausted
                    triggerInNextFrame.FulfilledEvtMask = EVTSUB_NO_DEMAND_MASK;
                    triggerInNextFrame.DemandedEvtMask = EVTSUB_NO_DEMAND_MASK;
                }
            } else if (0 == currTrigger.FramesToRecover) {
                // replenish upon mainCycle ends, but "false == mainCycleFulfilled"
                if (0 < currTrigger.Quota) {
                    triggerInNextFrame.State = TriggerState.Tready;
                    triggerInNextFrame.FramesInState = 0;
                }
            }
        }
    break; 
    case TRIGGER_SPECIES_BOSS_SAVEPOINT:
        if (mainCycleFulfilled) {
                if (0 < currTrigger.Quota) {
                    triggerInNextFrame.FramesToRecover = DEFAULT_FRAMES_DELAYED_OF_BOSS_SAVEPOINT;
                    triggerInNextFrame.FramesToFire = MAX_INT;
                    if (0 != triggerConfigFromTiled.NewRevivalX || 0 != triggerConfigFromTiled.NewRevivalY) {
                        if (0 < currTrigger.OffenderJoinIndex && currTrigger.OffenderJoinIndex <= roomCapacity) {
                            var thatCharacterInNextFrame = nextRenderFrame.PlayersArr[currTrigger.OffenderJoinIndex - 1];
                            thatCharacterInNextFrame.RevivalVirtualGridX = triggerConfigFromTiled.NewRevivalX;
                            thatCharacterInNextFrame.RevivalVirtualGridY = triggerConfigFromTiled.NewRevivalY;
                        } else if (1 == roomCapacity) {
                            var thatCharacterInNextFrame = nextRenderFrame.PlayersArr[0];
                            thatCharacterInNextFrame.RevivalVirtualGridX = triggerConfigFromTiled.NewRevivalX;
                            thatCharacterInNextFrame.RevivalVirtualGridY = triggerConfigFromTiled.NewRevivalY;
                        }
                    }   
                }
        } else if (0 == currTrigger.FramesToRecover) {
            if (0 < currTrigger.Quota) {
                triggerInNextFrame.FramesToRecover = MAX_INT;
                triggerInNextFrame.Quota = 0;
                fulfilledTriggerSetMask |= (1UL << (triggerInNextFrame.TriggerLocalId - 1));
            }
        }
    break;
    case TRIGGER_SPECIES_NSWITCH:
    case TRIGGER_SPECIES_PSWITCH:
        if (mainCycleFulfilled) {
            if (0 < currTrigger.Quota) {
                triggerInNextFrame.State = TriggerState.TcoolingDown;
                triggerInNextFrame.FramesInState = 0;
                triggerInNextFrame.FulfilledEvtMask = EVTSUB_NO_DEMAND_MASK;
                triggerInNextFrame.Quota = currTrigger.Quota - 1;
                triggerInNextFrame.FramesToRecover = triggerConfigFromTiled.RecoveryFrames;
                triggerInNextFrame.FramesToFire = triggerConfigFromTiled.DelayedFrames;
                fulfilledTriggerSetMask |= (1UL << (triggerInNextFrame.TriggerLocalId - 1));
                _notifySubscriberTriggers(currRenderFrame.Id, triggerInNextFrame, nextRenderFrameTriggers, false, false, triggerEditorIdToTiledConfig, logger);
                //logger.LogInfo(String.Format("@rdfId={0}, switch trigger editor id = {1}, local id = {2} is fulfilled", currRenderFrame.Id, triggerInNextFrame.EditorId ,triggerInNextFrame.TriggerLocalId));
            }
        } else if (0 == currTrigger.FramesToRecover) {
            // replenish upon mainCycle ends, but "false == mainCycleFulfilled"
            if (0 < currTrigger.Quota) {
                triggerInNextFrame.State = TriggerState.Tready;
                triggerInNextFrame.FramesInState = 0;
            }
        }
    break;
    case TRIGGER_SPECIES_STORYPOINT_TRIVIAL:
    case TRIGGER_SPECIES_STORYPOINT_MV:
        if (mainCycleFulfilled) {
            if (0 < currTrigger.Quota) {
                triggerInNextFrame.Quota = currTrigger.Quota - 1;
                triggerInNextFrame.FramesToRecover = triggerConfigFromTiled.RecoveryFrames;
                triggerInNextFrame.FramesToFire = triggerConfigFromTiled.DelayedFrames;
                justTriggeredStoryPointId = triggerConfigFromTiled.StoryPointId;
                justTriggeredBgmId = triggerConfigFromTiled.BgmId;
                fulfilledTriggerSetMask |= (1UL << (triggerInNextFrame.TriggerLocalId - 1));
                //logger.LogInfo(String.Format("@rdfId={0}, story point trigger editor id = {1}, local id = {2} is fulfilled", currRenderFrame.Id, triggerInNextFrame.EditorId ,triggerInNextFrame.TriggerLocalId));
            }
        }
    break;
    case TRIGGER_SPECIES_TIMED_WAVE_PICKABLE_DROPPER:
        if (subCycleFulfilled) {
            int pickableSpawnerConfigIdx = triggerConfigFromTiled.QuotaCap - currTrigger.Quota;
            var pickableSpawnerConfig = lowerBoundForPickableSpawnerConfig(pickableSpawnerConfigIdx, triggerConfigFromTiled.PickableSpawnerTimeSeq, logger);
            fireTriggerPickableSpawning(currRenderFrame, currTrigger, triggerInNextFrame, ref pickableLocalIdCounter, ref nextRdfPickableCnt, nextRenderFramePickables, decodedInputHolder, pickableSpawnerConfig, triggerEditorIdToTiledConfig, logger);
        } else if (mainCycleFulfilled) {
            if (0 < currTrigger.Quota) {
                triggerInNextFrame.Quota = currTrigger.Quota - 1;
                triggerInNextFrame.FramesToRecover = triggerConfigFromTiled.RecoveryFrames;
                triggerInNextFrame.FramesToFire = triggerConfigFromTiled.DelayedFrames;
                triggerInNextFrame.SubCycleQuotaLeft = triggerConfigFromTiled.SubCycleQuota;
                fulfilledTriggerSetMask |= (1UL << (triggerInNextFrame.TriggerLocalId - 1));
            } else if (0 == currTrigger.Quota) {
                // Set to exhausted
                triggerInNextFrame.FulfilledEvtMask = EVTSUB_NO_DEMAND_MASK;
                triggerInNextFrame.DemandedEvtMask = EVTSUB_NO_DEMAND_MASK;
            }
        } else if (0 == currTrigger.FramesToRecover) {
            // replenish upon mainCycle ends, but "false == mainCycleFulfilled"
            if (0 < currTrigger.Quota) {
                triggerInNextFrame.State = TriggerState.Tready;
                triggerInNextFrame.FramesInState = 0;
            }
        }
    break;
    case TRIGGER_SPECIES_TIMED_WAVE_DOOR_1:
    case TRIGGER_SPECIES_INDI_WAVE_DOOR_1:
    case TRIGGER_SPECIES_SYNC_WAVE_DOOR_1:
        var firstSubscribesToTriggerEditorId = (0 >= triggerConfigFromTiled.SubscribesToIdList.Count ? TERMINATING_EVTSUB_ID_INT : triggerConfigFromTiled.SubscribesToIdList[0]);
        var subscribesToTriggerInNextFrame = (TERMINATING_EVTSUB_ID_INT == firstSubscribesToTriggerEditorId ? null : nextRenderFrameTriggers[triggerEditorIdToLocalId[firstSubscribesToTriggerEditorId] - 1]);
        var npcExhaustedReceptionTriggerInNextFrame = ((TRIGGER_SPECIES_INDI_WAVE_DOOR_1 == triggerConfig.SpeciesId || TRIGGER_SPECIES_TIMED_WAVE_DOOR_1 == triggerConfig.SpeciesId) ? triggerInNextFrame : subscribesToTriggerInNextFrame);

        if (subCycleFulfilled) {
            // [WARNING] The information of "justFulfilled" will be lost after then just-fulfilled renderFrame, thus temporarily using "FramesToFire" to keep track of subsequent spawning
            int chSpawnerConfigIdx = triggerConfigFromTiled.QuotaCap - currTrigger.Quota;
            var chSpawnerConfig = lowerBoundForSpawnerConfig(chSpawnerConfigIdx, triggerConfigFromTiled.CharacterSpawnerTimeSeq);
            fireTriggerSpawning(currRenderFrame, roomCapacity, currTrigger, triggerInNextFrame, nextRenderFrameNpcs, ref npcLocalIdCounter, ref npcCnt, npcExhaustedReceptionTriggerInNextFrame, decodedInputHolder, chSpawnerConfig, triggerEditorIdToTiledConfig, logger);
        } else if (mainCycleFulfilled) {
            if (0 < currTrigger.Quota) {
                triggerInNextFrame.Quota = currTrigger.Quota - 1;
                triggerInNextFrame.FramesToRecover = triggerConfigFromTiled.RecoveryFrames;
                triggerInNextFrame.FramesToFire = triggerConfigFromTiled.DelayedFrames;
                int chSpawnerConfigIdx = triggerConfigFromTiled.QuotaCap - triggerInNextFrame.Quota;
                var chSpawnerConfig = lowerBoundForSpawnerConfig(chSpawnerConfigIdx, triggerConfigFromTiled.CharacterSpawnerTimeSeq);
                int nextWaveNpcCnt = (chSpawnerConfig.SpeciesIdList.Count < triggerConfigFromTiled.SubCycleQuota ? chSpawnerConfig.SpeciesIdList.Count : triggerConfigFromTiled.SubCycleQuota);
                triggerInNextFrame.SubCycleQuotaLeft = triggerConfigFromTiled.SubCycleQuota;
                if (TRIGGER_SPECIES_TIMED_WAVE_DOOR_1 == triggerConfigFromTiled.SpeciesId) {
                    // [WARNING] For exhaustion of a TimedWaveDoor, we required all spawned NPC-exhausted
                    if (currTrigger.Quota == triggerConfigFromTiled.QuotaCap) {
                        triggerInNextFrame.WaveNpcExhaustedEvtMaskCounter = 1UL;
                        triggerInNextFrame.DemandedEvtMask = (1UL << nextWaveNpcCnt) - 1; 
                        //logger.LogInfo(String.Format("@rdfId={0}, {10} editor id = {1} INITIAL mainCycleFulfilled, local id = {2}, of next frame:: demandedEvtMask = {3}, fulfilledEvtMask = {4}, waveNpcExhaustedEvtMaskCounter = {5}, quota = {6}, subCycleQuota = {7}, framesToRecover = {8}, framesToFire = {9}", currRenderFrame.Id, triggerInNextFrame.EditorId, triggerInNextFrame.TriggerLocalId, triggerInNextFrame.DemandedEvtMask, triggerInNextFrame.FulfilledEvtMask, triggerInNextFrame.WaveNpcExhaustedEvtMaskCounter, triggerInNextFrame.Quota, triggerInNextFrame.SubCycleQuotaLeft, triggerInNextFrame.FramesToRecover, triggerInNextFrame.FramesToFire, currTrigger.Config.SpeciesName));
                    } else {
                        triggerInNextFrame.DemandedEvtMask <<= nextWaveNpcCnt; 
                        triggerInNextFrame.DemandedEvtMask |= (1UL << nextWaveNpcCnt) - 1;
                        //logger.LogInfo(String.Format("@rdfId={0}, {10} editor id = {1} SUBSEQ mainCycleFulfilled, local id = {2}, of next frame: demandedEvtMask = {3}, fulfilledEvtMask = {4}, waveNpcExhaustedEvtMaskCounter = {5}, quota = {6}, subCycleQuota = {7}, framesToRecover = {8}, framesToFire = {9}", currRenderFrame.Id, triggerInNextFrame.EditorId, triggerInNextFrame.TriggerLocalId, triggerInNextFrame.DemandedEvtMask, triggerInNextFrame.FulfilledEvtMask, triggerInNextFrame.WaveNpcExhaustedEvtMaskCounter, triggerInNextFrame.Quota, triggerInNextFrame.SubCycleQuotaLeft, triggerInNextFrame.FramesToRecover, triggerInNextFrame.FramesToFire, currTrigger.Config.SpeciesName));
                    }     
                } else {
                    // [WARNING] For SyncWaveDoor, its main cycles are not triggered by NPC-exhausted, thus assigning this value uniformly is not an issue!
                    triggerInNextFrame.WaveNpcExhaustedEvtMaskCounter = 1UL;
                    triggerInNextFrame.DemandedEvtMask = (1UL << nextWaveNpcCnt) - 1;
                    triggerInNextFrame.FulfilledEvtMask = EVTSUB_NO_DEMAND_MASK;
                    //logger.LogInfo(String.Format("@rdfId={0}, {1} editor id = {2} mainCycleFulfilled, local id = {3} of next frame: demandedEvtMask = {4}, fulfilledEvtMask = {5}, waveNpcExhaustedEvtMaskCounter = {6}, quota = {7}, subCycleQuota = {8}, framesToRecover = {9}, framesToFire = {10}", currRenderFrame.Id, currTrigger.Config.SpeciesName, triggerInNextFrame.EditorId, triggerInNextFrame.TriggerLocalId, triggerInNextFrame.DemandedEvtMask, triggerInNextFrame.FulfilledEvtMask, triggerInNextFrame.WaveNpcExhaustedEvtMaskCounter, triggerInNextFrame.Quota, triggerInNextFrame.SubCycleQuotaLeft, triggerInNextFrame.FramesToRecover, triggerInNextFrame.FramesToFire));
                }

                fulfilledTriggerSetMask |= (1UL << (triggerInNextFrame.TriggerLocalId - 1));
            } else if (0 == currTrigger.Quota) {
                // Set to exhausted
                // [WARNING] Exclude MAGIC_QUOTA_INFINITE and MAGIC_QUOTA_EXHAUSTED here!
                //logger.LogInfo(String.Format("@rdfId={0}, {6} editor id = {1} exhausted, local id = {2}, of next frame:: demandedEvtMask = {3}, fulfilledEvtMask = {4}, waveNpcExhaustedEvtMaskCounter = {5}", currRenderFrame.Id, triggerInNextFrame.EditorId, triggerInNextFrame.TriggerLocalId, triggerInNextFrame.DemandedEvtMask, triggerInNextFrame.FulfilledEvtMask, triggerInNextFrame.WaveNpcExhaustedEvtMaskCounter, currTrigger.Config.SpeciesName));
                triggerInNextFrame.FulfilledEvtMask = EVTSUB_NO_DEMAND_MASK;
                triggerInNextFrame.DemandedEvtMask = EVTSUB_NO_DEMAND_MASK;
                
                if (null != subscribesToTriggerInNextFrame) {
                    // [WARNING] Whenever a single NPC wave door is subscribing to any group trigger, ignore its exhaust subscribers, i.e. exhaust subscribers should respect the group trigger instead!
                    subscribesToTriggerInNextFrame.FulfilledEvtMask |= (1UL << (currTrigger.TriggerLocalId-1));
                } else {
                    _notifySubscriberTriggers(currRenderFrame.Id, triggerInNextFrame, nextRenderFrameTriggers, false, true, triggerEditorIdToTiledConfig, logger);
                }
            }
        } else if (0 == currTrigger.FramesToRecover) {
            // replenish upon mainCycle ends, but "false == mainCycleFulfilled"
            if (0 < currTrigger.Quota) {
                triggerInNextFrame.State = TriggerState.Tready;
                triggerInNextFrame.FramesInState = 0;
            }
        }
    break;
    case TRIGGER_SPECIES_INDI_WAVE_GROUP_TRIGGER_TRIVIAL:
    case TRIGGER_SPECIES_INDI_WAVE_GROUP_TRIGGER_MV:
        if (mainCycleFulfilled) {
            if (0 < currTrigger.Quota) {
                triggerInNextFrame.Quota = currTrigger.Quota - 1;
                triggerInNextFrame.FramesToRecover = triggerConfigFromTiled.RecoveryFrames;
                _notifySubscriberTriggers(currRenderFrame.Id, triggerInNextFrame, nextRenderFrameTriggers, false, false, triggerEditorIdToTiledConfig, logger); 
                // Special handling, reverse subscription and repurpose evt mask fields. 
                triggerInNextFrame.FulfilledEvtMask = EVTSUB_NO_DEMAND_MASK;
                triggerInNextFrame.DemandedEvtMask = triggerInNextFrame.SubscriberLocalIdsMask;
                triggerInNextFrame.SubscriberLocalIdsMask = EVTSUB_NO_DEMAND_MASK; // There's no long any subscriber to this group trigger 
                fulfilledTriggerSetMask |= (1UL << (triggerInNextFrame.TriggerLocalId - 1));
                //logger.LogInfo(String.Format("@rdfId={0}, {3} editor id = {1}, local id = {2} is fulfilled for the first time and re-purposed", currRenderFrame.Id, triggerInNextFrame.EditorId ,triggerInNextFrame.TriggerLocalId, currTrigger.Config.SpeciesName));
            } else {
                // Set to exhausted
                //logger.LogInfo(String.Format("@rdfId={0}, {3} editor id = {1}, local id = {2} is exhausted", currRenderFrame.Id, triggerInNextFrame.EditorId ,triggerInNextFrame.TriggerLocalId, currTrigger.Config.SpeciesName));
                triggerInNextFrame.FulfilledEvtMask = EVTSUB_NO_DEMAND_MASK;
                triggerInNextFrame.DemandedEvtMask = EVTSUB_NO_DEMAND_MASK;
                _notifySubscriberTriggers(currRenderFrame.Id, triggerInNextFrame, nextRenderFrameTriggers, false, true, triggerEditorIdToTiledConfig, logger);
            }
        }
    break;
    case TRIGGER_SPECIES_SYNC_WAVE_GROUP_TRIGGER_TRIVIAL:
    case TRIGGER_SPECIES_SYNC_WAVE_GROUP_TRIGGER_MV:
            if (mainCycleFulfilled) {
                if (0 < currTrigger.Quota) {
                    triggerInNextFrame.Quota = currTrigger.Quota - 1;
                    triggerInNextFrame.FramesToRecover = triggerConfigFromTiled.RecoveryFrames;
                    triggerInNextFrame.WaveNpcExhaustedEvtMaskCounter = 1UL;
                    int nextWaveNpcCnt = _notifySubscriberTriggers(currRenderFrame.Id, triggerInNextFrame, nextRenderFrameTriggers, true, false, triggerEditorIdToTiledConfig, logger); 
                    triggerInNextFrame.DemandedEvtMask = (1UL << nextWaveNpcCnt) - 1;
                    // Special handling, reverse subscription and repurpose evt mask fields. 
                    triggerInNextFrame.FulfilledEvtMask = EVTSUB_NO_DEMAND_MASK;
                    fulfilledTriggerSetMask |= (1UL << (triggerInNextFrame.TriggerLocalId - 1));
                    //logger.LogInfo(String.Format("@rdfId={0}, {1} editor id = {2}, local id = {3} is initiated and re-purposed. DemandedEvtMask={4} from nextWaveNpcCnt={5}", currRenderFrame.Id, currTrigger.Config.SpeciesName, triggerInNextFrame.EditorId, triggerInNextFrame.TriggerLocalId, triggerInNextFrame.DemandedEvtMask, nextWaveNpcCnt));
                } else {
                    // Set to exhausted
                    //logger.LogInfo(String.Format("@rdfId={0}, {1} editor id = {2}, local id = {3} is exhausted", currRenderFrame.Id, currTrigger.Config.SpeciesName, triggerInNextFrame.EditorId, triggerInNextFrame.TriggerLocalId));
                    triggerInNextFrame.FulfilledEvtMask = EVTSUB_NO_DEMAND_MASK;
                    triggerInNextFrame.DemandedEvtMask = EVTSUB_NO_DEMAND_MASK;
                    _notifySubscriberTriggers(currRenderFrame.Id, triggerInNextFrame, nextRenderFrameTriggers, false, true, triggerEditorIdToTiledConfig, logger);
                }
            }
            break;
    }
    */
}

void BaseBattle::handleLhsCharacterCollision(
    const int currRdfId, 
    RenderFrame* nextRdf,
    const uint64_t udLhs, const uint64_t udtLhs, const CharacterDownsync* currChd, CharacterDownsync* nextChd,
    const uint64_t udRhs, const uint64_t udtRhs, 
    const CollideShapeResult& inResult,
    uint32_t& outNewEffDebuffSpeciesId, int& outNewDamage, bool& outNewEffBlownUp, int& outNewEffFramesToRecover, float& outNewEffPushbackVelX, float& outNewEffPushbackVelY) {

    switch (udtRhs) {
    case UDT_BL:
        if (!transientUdToCurrBl.count(udRhs)) {
#ifndef NDEBUG
            std::ostringstream oss;
            auto bulletId = getUDPayload(udRhs);
            oss << "handleLhsCharacterCollision/nextBl with currRdfId=" << currRdfId << ", id=" << bulletId << " is NOT FOUND";
            Debug::Log(oss.str(), DColor::Orange);
#endif
            return;
        }

        if (0 < currChd->frames_invinsible()) {
            break;
        }
        const Bullet* rhsCurrBl = transientUdToCurrBl[udRhs];
        if (!isBulletActive(rhsCurrBl)) {
            break;
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
                    break;
                }
                immuneRcdI++;
            }

            if (shouldBeImmune) {
                break;
            }
            
            if (!(successfulDef1 && rhsBlConfig->takes_def1_as_hard_pushback())) {
                int nextImmuneRcdI = immuneRcdI;
                int terminatingImmuneRcdI = nextImmuneRcdI + 1;
                if (nextImmuneRcdI == nextChd->bullet_immune_records_size()) {
                    nextImmuneRcdI = 0;
                    terminatingImmuneRcdI = nextChd->bullet_immune_records_size(); // [WARNING] DON'T update termination in this case! 
                }
                auto nextImmuneRcd = nextChd->mutable_bullet_immune_records(nextImmuneRcdI);
                nextImmuneRcd->set_bullet_id(rhsCurrBl->id());
                nextImmuneRcd->set_remaining_lifetime_rdf_count((INT_MAX <= rhsBlConfig->hit_stun_frames()) ? INT_MAX : (rhsBlConfig->hit_stun_frames() << 3));

                if (terminatingImmuneRcdI < nextChd->bullet_immune_records_size()) {
                    auto terminatingImmuneRcd = nextChd->mutable_bullet_immune_records(terminatingImmuneRcdI);
                    terminatingImmuneRcd->set_bullet_id(globalPrimitiveConsts->terminating_bullet_id());
                    terminatingImmuneRcd->set_remaining_lifetime_rdf_count(0);
                }
            }
        }

        const CharacterConfig* cc = getCc(currChd->species_id());
        if (!(successfulDef1 && 0 >= cc->def1_damage_yield())) {
            int effDamage = successfulDef1 ? (int)std::ceil(cc->def1_damage_yield()*rhsBlConfig->damage()) : rhsBlConfig->damage(); 
            outNewDamage += effDamage; 
            if (rhsBlConfig->blow_up()) {
                outNewEffBlownUp = true;
            }
            if (rhsBlConfig->remains_upon_hit()) {
                addNewExplosionToNextFrame(currRdfId, nextRdf, rhsCurrBl, inResult);
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

        break;
    }
}

void BaseBattle::handleLhsBulletCollision(
    const int currRdfId, 
    RenderFrame* nextRdf,
    const uint64_t udLhs, const uint64_t udtLhs, const Bullet* currBl, Bullet* nextBl,
    const uint64_t udRhs, const uint64_t udtRhs, const JPH::CollideShapeResult& inResult) {
    auto bulletId = currBl->id();
    const Skill* lhsSkill = nullptr;
    const BulletConfig* lhsBlConfig = nullptr;
    FindBulletConfig(currBl->skill_id(), currBl->active_skill_hit(), lhsSkill, lhsBlConfig);

    switch (udtRhs) {
    case UDT_PLAYER:
    case UDT_NPC:
    case UDT_TRAP:
    case UDT_OBSTACLE:
        switch (lhsBlConfig->b_type()) {
        case BulletType::MechanicalCartridge: {
            auto explosionPos = inResult.mContactPointOn1 + Vec3(currBl->x(), currBl->y(), 0);
            nextBl->set_bl_state(BulletState::Exploding);
            nextBl->set_frames_in_bl_state(0);
            nextBl->set_x(explosionPos.GetX());
            nextBl->set_y(explosionPos.GetY());
            nextBl->set_vel_x(0);
            nextBl->set_vel_y(0);
/*
#ifndef NDEBUG
            std::ostringstream oss;
            oss << "handleLhsBulletCollision/nextBl with currRdfId=" << currRdfId << ", id=" << bulletId << ", ud=" << udLhs << " explodes on udtRhs=" << udtRhs << " at position=(" << currBl->x() << ", " << currBl->y() << ")";
            Debug::Log(oss.str(), DColor::Orange);
#endif // !NDEBUG
*/
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
