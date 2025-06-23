#include "BaseBattle.h"

#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

// STL includes
#include <cstdarg>
#include <thread>
#include <utility>
#include <string>

using namespace jtshared;

const float  cUpRotationX = 0;
const float  cUpRotationZ = 0;
const float  cMaxSlopeAngle = DegreesToRadians(45.0f);
const float  cMaxStrength = 100.0f;
const float  cCharacterPadding = 0.02f;
const float  cPenetrationRecoverySpeed = 1.0f;
const float  cPredictiveContactDistance = 0.1f;
const bool   cEnableWalkStairs = true;
const bool   cEnableStickToFloor = true;
const bool   cEnhancedInternalEdgeRemoval = false;
const bool   cCreateInnerBody = false;
const bool   cPlayerCanPushOtherCharacters = true;
const bool   cOtherCharactersCanPushPlayer = true;

const EBackFaceMode cBackFaceMode = EBackFaceMode::CollideWithBackFaces;
static CH_CACHE_KEY_T chCacheKeyHolder = {0, 0};
static BL_CACHE_KEY_T blCacheKeyHolder = { 0, 0 };

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
    holder->CopyFrom(startRdf);

    prefabbedInputList.assign(playersCnt, 0);   
    playerInputFrontIds.assign(playersCnt, 0); 
    playerInputFronts.assign(playersCnt, 0); 

    ////////////////////////////////////////////// 2
    bodyIDsToClear.reserve(cMaxBodies);
    bodyIDsToAdd.reserve(cMaxBodies);
    phySys = new PhysicsSystem();
    phySys->Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, bpLayerInterface, ovbLayerFilter, ovoLayerFilter);
    phySys->SetBodyActivationListener(&bodyActivationListener);
    phySys->SetContactListener(&contactListener);
    //phySys->SetSimCollideBodyVsBody(&myBodyCollisionPipe); // To omit unwanted body collisions, e.g. same team
    bi = &(phySys->GetBodyInterface());

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

            Float3 v1 = Float3(x1, y1, +cDefaultHalfThickness);
            Float3 v2 = Float3(x1, y1, -cDefaultHalfThickness);
            Float3 v3 = Float3(x2, y2, +cDefaultHalfThickness);
            Float3 v4 = Float3(x2, y2, -cDefaultHalfThickness);

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
        bi->CreateAndAddBody(bodyCreationSettings, EActivation::DontActivate);
    }
    phySys->OptimizeBroadPhase();

    ////////////////////////////////////////////// 3
    jobSys = new JobSystemThreadPool(cMaxPhysicsJobs, cMaxPhysicsBarriers, thread::hardware_concurrency() - 1);

    ////////////////////////////////////////////// 4
    transientCurrJoinIndexToChdMap.reserve(playersCnt + DEFAULT_PREALLOC_NPC_CAPACITY);

    ////////////////////////////////////////////// 5 (to deprecate!)
    dummyCc.set_capsule_radius(3.0);
    dummyCc.set_capsule_half_height(10.0);

    dummyBc.set_hitbox_size_x(12.0);
    dummyBc.set_hitbox_size_y(18.0);
}

BaseBattle::~BaseBattle() {
    Clear();
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
    calcChCacheKey(cc, chCacheKeyHolder);
    CharacterVirtual* chCollider = nullptr;
    auto& it = cachedChColliders.find(chCacheKeyHolder);
    if (it == cachedChColliders.end()) {
        CapsuleShape* chShapeCenterAnchor = new CapsuleShape(cc->capsule_half_height(), cc->capsule_radius()); // transient, only created on stack for immediately transform
        RefConst<Shape> chShape = RotatedTranslatedShapeSettings(Vec3(0, cc->capsule_half_height() + cc->capsule_radius(), 0), Quat::sIdentity(), chShapeCenterAnchor).Create().Get();
        Ref<CharacterVirtualSettings> settings = new CharacterVirtualSettings();
        settings->mMaxSlopeAngle = cMaxSlopeAngle;
        settings->mMaxStrength = cMaxStrength;
        settings->mShape = chShape;
        settings->mBackFaceMode = cBackFaceMode;
        settings->mCharacterPadding = cCharacterPadding;
        settings->mPenetrationRecoverySpeed = cPenetrationRecoverySpeed;
        settings->mPredictiveContactDistance = cPredictiveContactDistance;
        settings->mSupportingVolume = Plane(Vec3::sAxisY(), -cc->capsule_radius()); // Accept contacts that touch the lower sphere of the capsule
        settings->mEnhancedInternalEdgeRemoval = cEnhancedInternalEdgeRemoval;
        settings->mInnerBodyShape = chShape;
        settings->mInnerBodyLayer = MyObjectLayers::MOVING; // A "CharacterVirtual" is by default translational only, no need to set EMotionType::Kinematic here, see https://jrouwe.github.io/JoltPhysics/index.html#character-controllers.
    
        chCollider = new CharacterVirtual(settings, Vec3::sZero(), Quat::sIdentity(), cd->join_index(), phySys);       
    } else {
        auto& q = it->second;
        chCollider = q.back();
        q.pop_back();
        bodyIDsToAdd.push_back(chCollider->GetInnerBodyID());
    }

    chCollider->SetPosition(RVec3Arg(cd->x(), cd->y(), 0));
    chCollider->SetLinearVelocity(RVec3Arg(cd->vel_x(), cd->vel_y(), 0));

    // must be active when created
    activeChColliders.push_back(chCollider);

    return chCollider;
}

void BaseBattle::Step(int fromRdfId, int toRdfId, TempAllocator* tempAllocator) {
    for (int rdfId = fromRdfId; rdfId < toRdfId; rdfId++) {
        transientCurrJoinIndexToChdMap.clear();
        
        auto currRdf = rdfBuffer.GetByFrameId(rdfId);
        RenderFrame* nextRdf = rdfBuffer.GetByFrameId(rdfId+1);
        if (!nextRdf) {
            nextRdf = rdfBuffer.DryPut();
        }
        nextRdf->CopyFrom(*currRdf);
        nextRdf->set_id(rdfId+1);
        elapse1RdfForRdf(nextRdf);

        bodyIDsToAdd.clear();
        for (int i = 0; i < playersCnt; i++) {
            auto player = currRdf->players_arr(i);
            CharacterConfig* cc = &dummyCc;
            auto chCollider = getOrCreateCachedCharacterCollider(&player, cc);
            transientCurrJoinIndexToChdMap[player.join_index()] = &player;
        }
        if (!bodyIDsToAdd.empty()) {
            auto layerState = bi->AddBodiesPrepare(bodyIDsToAdd.data(), bodyIDsToAdd.size());
            bi->AddBodiesFinalize(bodyIDsToAdd.data(), bodyIDsToAdd.size(), layerState, EActivation::Activate);
        }
        phySys->Update(ESTIMATED_SECONDS_PER_FRAME, 1, tempAllocator, jobSys);
        for (auto it = activeChColliders.begin(); it != activeChColliders.end(); it++) {
            CharacterVirtual* single = *it;
            // Settings for our update function
            CharacterVirtual::ExtendedUpdateSettings chColliderExtUpdateSettings;
            chColliderExtUpdateSettings.mStickToFloorStepDown = -single->GetUp() * chColliderExtUpdateSettings.mStickToFloorStepDown.Length();
            chColliderExtUpdateSettings.mWalkStairsStepUp = single->GetUp() * chColliderExtUpdateSettings.mWalkStairsStepUp.Length();

            single->ExtendedUpdate(cDeltaTime, phySys->GetGravity(),
                chColliderExtUpdateSettings,
                phySys->GetDefaultBroadPhaseLayerFilter(MyObjectLayers::MOVING),
                phySys->GetDefaultLayerFilter(MyObjectLayers::MOVING),
                {}, // BodyFilter
                {}, // ShapeFilter
                *tempAllocator);

            int joinIndex = single->GetUserData();
            CharacterDownsync* nextChd = mutableChdFromRdf(joinIndex, nextRdf);
            CharacterConfig* cc = &dummyCc; // TODO: Find by "chd->species_id()"

            auto newPos = single->GetPosition();
            auto newVel = single->IsSupported()
                ?
                (RVec3Arg(single->GetLinearVelocity().GetX(), 0, 0) + single->GetGroundVelocity())
                :
                (nextChd->omit_gravity() || cc->omit_gravity() ? single->GetLinearVelocity() : single->GetLinearVelocity() + phySys->GetGravity());

            nextChd->set_in_air(!single->IsSupported());
            nextChd->set_x(newPos.GetX());
            nextChd->set_y(newPos.GetY());
            nextChd->set_vel_x(newVel.GetX());
            nextChd->set_vel_y(newVel.GetY());
        }

        batchRemoveFromPhySysAndCache(currRdf);
    }
}

void BaseBattle::Clear() {
    activeBlColliders.clear(); // By now there's no active bullet collider

    // In case there's any active character collider left.
    while (!activeChColliders.empty()) {
        auto single = activeChColliders.back();
        activeChColliders.pop_back(); 
        delete single; // Will call "bi->DestroyBody(single->GetInnerBodyID())" if still attached to "phySys"
    }

    while (!cachedChColliders.empty()) {
        for (auto it = cachedChColliders.begin(); it != cachedChColliders.end(); ) {
            auto& q = it->second;
            while (!q.empty()) {
                auto single = q.back();
                q.pop_back(); 
                delete single; // Will call "bi->DestroyBody(single->GetInnerBodyID())" if still attached to "phySys"
            }
            it = cachedChColliders.erase(it); 
        }
    }

    // Remove other regular bodies, unexpected active bullet colliders will also be cleared here
    bodyIDsToClear.clear();
    phySys->GetBodies(bodyIDsToClear);
    bi->RemoveBodies(bodyIDsToClear.data(), bodyIDsToClear.size());
    bi->DestroyBodies(bodyIDsToClear.data(), bodyIDsToClear.size());
}

void BaseBattle::elapse1RdfForRdf(RenderFrame* rdf) {
    for (int i = 0; i < playersCnt; i++) {
        auto cd = rdf->mutable_players_arr(i);
        elapse1RdfForChd(cd, &dummyCc); 
    }
    for (int i = 0; i < rdf->npcs_arr_size(); i++) {
        auto cd = rdf->mutable_npcs_arr(i);
        if (TERMINATING_CHARACTER_ID == cd->id()) break;
        elapse1RdfForChd(cd, &dummyCc);
    }
}

void BaseBattle::elapse1RdfForBl(Bullet* bl) {

}

void BaseBattle::elapse1RdfForChd(CharacterDownsync* cd, CharacterConfig* cc) {
    cd->set_frames_to_recover((0 < cd->frames_to_recover() ? cd->frames_to_recover()-1 : 0));
    cd->set_frames_captured_by_inertia((0 < cd->frames_captured_by_inertia() ? cd->frames_captured_by_inertia() - 1 : 0)); 
    cd->set_frames_in_ch_state(cd->frames_in_ch_state() + 1);
    cd->set_frames_invinsible(0 < cd->frames_invinsible() ? cd->frames_invinsible() - 1 : 0);
    cd->set_frames_in_patrol_cue(0 < cd->frames_in_patrol_cue() ? cd->frames_in_patrol_cue() - 1 : 0);
    cd->set_mp_regen_rdf_countdown(0 < cd->mp_regen_rdf_countdown() ? cd->mp_regen_rdf_countdown()-1 : 0);
    auto mp = cd->mp();
    if (0 >= cd->mp_regen_rdf_countdown()) {
        mp += cc->mp_regen_per_interval();
        if (mp >= cc->mp()) {
            mp = cc->mp();
        }
        cd->set_mp_regen_rdf_countdown(cc->mp_regen_interval());
    }
    cd->set_frames_to_start_jump((0 < cd->frames_to_start_jump() ? cd->frames_to_start_jump() - 1 : 0));
    cd->set_frames_since_last_damaged((0 < cd->frames_since_last_damaged() ? cd->frames_since_last_damaged() - 1 : 0));
    if (0 >= cd->frames_since_last_damaged()) {
        cd->set_damage_elemental_attrs(0);
    }
    cd->set_combo_frames_remained((0 < cd->combo_frames_remained() ? cd->combo_frames_remained() - 1 : 0));
    if (0 >= cd->combo_frames_remained()) {
        cd->set_combo_hit_cnt(0);
    } 

    cd->set_flying_rdf_countdown( (MAX_FLYING_RDF_CNT == cc->flying_quota_rdf_cnt() ? MAX_FLYING_RDF_CNT : (0 < cd->flying_rdf_countdown() ? cd->flying_rdf_countdown() - 1 : 0)));
}

const CharacterDownsync& BaseBattle::immutableChdFromRdf(int joinIndex, RenderFrame* rdf) {
    if (playersCnt >= joinIndex) {
        return rdf->players_arr(joinIndex - 1);
    } else {
        return rdf->players_arr(joinIndex - playersCnt - 1);
    }
}

CharacterDownsync* BaseBattle::mutableChdFromRdf(int joinIndex, RenderFrame* rdf) {
    if (playersCnt >= joinIndex) {
        return rdf->mutable_players_arr(joinIndex-1);
    } else {
        return rdf->mutable_npcs_arr(joinIndex-playersCnt-1);
    }
}

void BaseBattle::calcChCacheKey(CharacterConfig* cc, CH_CACHE_KEY_T& ioCacheKey) {
    ioCacheKey[0] = cc->capsule_radius();
    ioCacheKey[1] = cc->capsule_half_height();
}

void BaseBattle::calcBlCacheKey(BulletConfig* bc, BL_CACHE_KEY_T& ioCacheKey) {
    ioCacheKey[0] = bc->hitbox_size_x();
    ioCacheKey[1] = bc->hitbox_size_y();
}

void BaseBattle::batchRemoveFromPhySysAndCache(RenderFrame* currRdf) {
    bodyIDsToClear.clear();
    while (!activeChColliders.empty()) {
        CharacterVirtual* single = activeChColliders.back();
        activeChColliders.pop_back();
        int joinIndex = single->GetUserData();
        const CharacterDownsync& cd = immutableChdFromRdf(joinIndex, currRdf);
        CharacterConfig* cc = &dummyCc; // TODO: Find by "cd.species_id()"

        calcChCacheKey(cc, chCacheKeyHolder); // [WARNING] Don't use the underlying shape attached to "front" for capsuleKey forming, it's different from the values of corresponding "CharacterConfig*".
        auto& it = cachedChColliders.find(chCacheKeyHolder);

        if (it == cachedChColliders.end()) {
            // [REMINDER] Lifecycle of this stack-variable "q" will end after exiting the current closure, thus if "cachedChColliders" is to retain it out of the current closure, some extra space is to be used.
            CH_COLLIDER_Q q = { single };
            cachedChColliders.emplace(chCacheKeyHolder, q);
        } else {
            auto& cacheQue = it->second;
            cacheQue.push_back(single);
        }

        bodyIDsToClear.push_back(single->GetInnerBodyID());
    }

    while (!activeBlColliders.empty()) {
        Body* single = activeBlColliders.back();
        activeBlColliders.pop_back();
        int blArrIdx = single->GetUserData();
        const Bullet& bl = currRdf->bullets(blArrIdx);
        BulletConfig* bc = &dummyBc; // TODO: Find by "cd.species_id()"

        calcBlCacheKey(bc, blCacheKeyHolder); // [WARNING] Don't use the underlying shape attached to "front" for capsuleKey forming, it's different from the values of corresponding "CharacterConfig*".
        auto& it = cachedBlColliders.find(blCacheKeyHolder);

        if (it == cachedBlColliders.end()) {
            // [REMINDER] Lifecycle of this stack-variable "q" will end after exiting the current closure, thus if "cachedChColliders" is to retain it out of the current closure, some extra space is to be used.
            BL_COLLIDER_Q q = { single };
            cachedBlColliders.emplace(chCacheKeyHolder, q);
        } else {
            auto& cacheQue = it->second;
            cacheQue.push_back(single);
        }

        bodyIDsToClear.push_back(single->GetID());
    }

    bi->RemoveBodies(bodyIDsToClear.data(), bodyIDsToClear.size());
}
