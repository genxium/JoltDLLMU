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
    bodyIDsToClear.reserve(cMaxBodies);
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

    dummyCc.set_capsule_radius(3.0);
    dummyCc.set_capsule_half_height(10.0);
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
    // TODO: Deallocate cache when resetting battle.
    CH_CACHE_KEY_T capsuleKey = { cc->capsule_radius(), cc->capsule_half_height() };
    CharacterVirtual* chCollider = nullptr;
    auto& it = cachedChColliders.find(capsuleKey);
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
        auto dq = it->second;
        chCollider = dq.front();
        dq.pop_front();
        auto innerBodyID = chCollider->GetInnerBodyID();
        bi->AddBody(innerBodyID, EActivation::Activate);
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
        
        auto rdf = rdfBuffer.GetByFrameId(rdfId);
        RenderFrame* nextRdf = rdfBuffer.GetByFrameId(rdfId+1);
        if (!nextRdf) {
            nextRdf = rdfBuffer.DryPut();
        }
        nextRdf->MergeFrom(*rdf);
        nextRdf->set_id(rdfId+1);
        elapse1RdfForRdf(nextRdf);
        for (int i = 0; i < playersCnt; i++) {
            auto player = rdf->players_arr(i);
            CharacterConfig* cc = &dummyCc;
            auto chCollider = getOrCreateCachedCharacterCollider(&player, cc);
            transientCurrJoinIndexToChdMap[player.join_index()] = &player;
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
            CharacterConfig* cc = &dummyCc;

            auto newPos = single->GetPosition();
            auto newVel = single->IsSupported() 
                ? 
                (single->GetLinearVelocity() + single->GetGroundVelocity())
                : 
                (nextChd->omit_gravity() || cc->omit_gravity() ? single->GetLinearVelocity() : single->GetLinearVelocity() + phySys->GetGravity());

            nextChd->set_in_air(!single->IsSupported());
            nextChd->set_x(newPos.GetX());
            nextChd->set_y(newPos.GetY());
            nextChd->set_vel_x(newVel.GetX());
            nextChd->set_vel_y(newVel.GetY());
        }

        while (!activeChColliders.empty()) {
            CharacterVirtual* front = activeChColliders.front();
            activeChColliders.pop_front(); 
            bodyIDsToClear.push_back(front->GetInnerBodyID());
            const Shape* associatedShape = front->GetShape();
            const CapsuleShape* underlyingShape = static_cast<const CapsuleShape*>(front->GetShape());
            CH_CACHE_KEY_T capsuleKey = { underlyingShape->GetRadius(), underlyingShape->GetHalfHeightOfCylinder() };
            auto& it = cachedChColliders.find(capsuleKey);
            
            if (it == cachedChColliders.end()) {
                // [REMINDER] Lifecycle of this stack-variable "dq" will end after exiting the current closure, thus if "cachedChColliders" is to retain it out of the current closure, some extra space is to be used.
                CH_COLLIDER_Q dq = {front}; 
                cachedChColliders.insert(std::make_pair(capsuleKey, std::move(dq)));
            } else {
                auto& cacheQue = it->second;
                cacheQue.push_back(front);
            }
        }

        bi->RemoveBodies(bodyIDsToClear.data(), bodyIDsToClear.size());
        bi->DestroyBodies(bodyIDsToClear.data(), bodyIDsToClear.size());
        bodyIDsToClear.clear();
        // TODO: Update nextRdf by (rdf, phySys current body states of "dynamicBodyIDs"), then set back into "rdfBuffer" for rendering.
    }
}

void BaseBattle::Clear() {
    activeBlColliders.clear(); // By now there's no active bullet collider

    // In case there's any active character collider left.
    while (!activeChColliders.empty()) {
        auto front = activeChColliders.front();
        activeChColliders.pop_front(); 
        delete front;
    }

    while (!cachedChColliders.empty()) {
        for (auto it = cachedChColliders.cbegin(); it != cachedChColliders.cend(); ) {
            auto dq = std::move(it->second);
            while (!dq.empty()) {
                auto front = dq.front();
                dq.pop_front(); 
                delete front;
            }
            it = cachedChColliders.erase(it++); 
        }
    }

    // Remove other regular bodies, unexpected active bullet colliders will also be cleared here
    phySys->GetBodies(bodyIDsToClear);
    bi->RemoveBodies(bodyIDsToClear.data(), bodyIDsToClear.size());
    bi->DestroyBodies(bodyIDsToClear.data(), bodyIDsToClear.size());
    bodyIDsToClear.clear();
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

CharacterDownsync* BaseBattle::mutableChdFromRdf(int joinIndex, RenderFrame* rdf) {
    if (playersCnt >= joinIndex) {
        return rdf->mutable_players_arr(joinIndex-1);
    } else {
        return rdf->mutable_npcs_arr(joinIndex-playersCnt-1);
    }
}
