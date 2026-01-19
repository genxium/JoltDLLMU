#include "BaseNpcReaction.h"
#include "CharacterCollideShapeCollector.h"
#include "CollisionLayers.h"
#include "CollisionCallbacks.h"

#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/TaperedCylinderShape.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>

#include <climits>

void BaseNpcReaction::postStepDeriveNpcVisionReaction(int currRdfId, const Vec3& antiGravityNorm, const float gravityMagnitude, std::unordered_map<uint64_t, const PlayerCharacterDownsync*>& currPlayersMap, std::unordered_map<uint64_t, const NpcCharacterDownsync*>& currNpcsMap, std::unordered_map<uint64_t, const Bullet*>& currBulletsMap, const BodyInterface* biNoLock, const NarrowPhaseQuery* narrowPhaseQuery, const BaseBattleCollisionFilter* baseBattleFilter, const DefaultBroadPhaseLayerFilter& bplf, const DefaultObjectLayerFilter& olf, const CH_COLLIDER_T* selfNpcCollider, const BodyID& selfNpcBodyID, const uint64_t selfNpcUd, const NpcGoal currNpcGoal, const uint64_t currNpcCachedCueCmd, const CharacterDownsync& currChd, const MassProperties& massProps, const Vec3& currChdFacing, const CharacterConfig* cc, CharacterDownsync* nextChd, bool cvSupported, bool cvInAir, bool cvOnWall, bool currNotDashing, bool currEffInAir, bool oldNextNotDashing, bool oldNextEffInAir, bool inJumpStartupOrJustEnded, CharacterBase::EGroundState cvGroundState, NpcGoal& outNextNpcGoal, uint64_t& outCmd) {

    Vec3 initVisionOffset(cc->vision_offset_x(), cc->vision_offset_y(), 0);
    auto visionInitTransform = cTurn90DegsAroundZAxisMat.PostTranslated(initVisionOffset); // Rotate, and then translate
    Vec3 selfNpcPosition(currChd.x(), currChd.y(), currChd.z());
    JPH::Quat offenderEffQ = 0 < currChdFacing.GetX() ? cIdentityQ : cTurnbackAroundYAxis;
    auto visionCOMTransform = (JPH::Mat44::sRotation(offenderEffQ)*visionInitTransform).PostTranslated(selfNpcPosition); //and then rotate again by the NPC's orientation (affecting "initVisionOffset" too), and finally apply the NPC's position as translation
    
    float visionHalfHeight = cc->vision_half_height(), visionTopRadius = cc->vision_top_radius(), visionBottomRadius = cc->vision_bottom_radius();
    float visionConvexRadius = (visionTopRadius < visionBottomRadius ? visionTopRadius : visionBottomRadius)*0.9f; // Must be smaller than the min of these two 
    TaperedCylinderShapeSettings initVisionShapeSettings(visionHalfHeight, visionTopRadius, visionBottomRadius, visionConvexRadius);
    TaperedCylinderShapeSettings::ShapeResult shapeResult;
    TaperedCylinderShape initVisionShape(initVisionShapeSettings, shapeResult); // [WARNING] A transient, on-stack shape only bound to lifecycle of the current function & current thread. 

    // Moreover, the center-of-mass DOESN'T have a local y-coordinate "0" within "initVisionShape" when (visionTopRadius != visionBottomRadius).

    initVisionShape.SetEmbedded(); // To allow deallocation on-stack, i.e. the "mRefCount" will equal 1 when it deallocates on-stack with the current function closure.
    
    const TaperedCylinderShape* effVisionShape = &initVisionShape; 

    const Vec3 effVisionOffsetFromNpcChd = offenderEffQ * initVisionOffset;
    const Vec3 visionNarrowPhaseInBaseOffset = selfNpcPosition + effVisionOffsetFromNpcChd;

    const Vec3 visionDirection = currChdFacing;
  
    VisionBodyFilter visionBodyFilter(((const CharacterDownsync*)&currChd), selfNpcBodyID, selfNpcUd, baseBattleFilter);

    VISION_HIT_COLLECTOR_T visionHitCollector;
    const Vec3 scaling = Vec3::sOne();
    
    CollideShapeSettings settings;
    settings.mMaxSeparationDistance = cCollisionTolerance;
    settings.mActiveEdgeMode = EActiveEdgeMode::CollideOnlyWithActive;
    settings.mActiveEdgeMovementDirection = visionDirection;
    settings.mBackFaceMode = EBackFaceMode::IgnoreBackFaces;
    
	const AABox visionAABB = effVisionShape->GetWorldSpaceBounds(visionCOMTransform, scaling);
    narrowPhaseQuery->CollideShape(effVisionShape, scaling, visionCOMTransform, settings, visionNarrowPhaseInBaseOffset, visionHitCollector, bplf, olf, visionBodyFilter);
    
    bool hasVisionHit = visionHitCollector.HadHit();
    initVisionShape.Release();

    /*
    Now that we've got all entities in vision, will start handling each.
    */
    uint64_t toHandleAllyUd = 0, toHandleOppoChUd = 0, toHandleOppoBlUd = 0, toHandleMvBlockerUd = 0;
    Vec3 selfNpcPositionDiffForAllyUd, selfNpcPositionDiffForOppoChUd, selfNpcPositionDiffForOppoBlUd;
    GapToJump currGapToJump; currGapToJump.set_vision_alignment(FLT_MAX); currGapToJump.set_anti_gravity_alignment(FLT_MAX);
    GapToJump minGapToJump;  minGapToJump.set_vision_alignment(FLT_MAX); currGapToJump.set_anti_gravity_alignment(FLT_MAX);
    GapToJump currGroundMvTolerance; currGroundMvTolerance.set_vision_alignment(0); currGroundMvTolerance.set_anti_gravity_alignment(0);

    BodyID toHandleMvBlockerBodyID;

    extractKeyEntitiesInVision(currRdfId, antiGravityNorm, currPlayersMap, currNpcsMap, currBulletsMap, biNoLock, narrowPhaseQuery, selfNpcCollider, selfNpcBodyID, selfNpcUd, currChd, cc, nextChd, cvSupported, cvInAir, cvOnWall, currNotDashing, currEffInAir, oldNextNotDashing, oldNextEffInAir, inJumpStartupOrJustEnded, cvGroundState, visionAABB, effVisionOffsetFromNpcChd, visionNarrowPhaseInBaseOffset, visionDirection, visionHitCollector, toHandleAllyUd, selfNpcPositionDiffForAllyUd, toHandleOppoChUd, selfNpcPositionDiffForOppoChUd, toHandleOppoBlUd, selfNpcPositionDiffForOppoBlUd, toHandleMvBlockerUd, toHandleMvBlockerBodyID, currGapToJump, minGapToJump, currGroundMvTolerance);

    bool notDashing = BaseBattleCollisionFilter::chIsNotDashing(*nextChd);
    bool canJumpWithinInertia = BaseBattleCollisionFilter::chCanJumpWithInertia(currChd, cc, notDashing, inJumpStartupOrJustEnded);

    int newVisionReaction = TARGET_CH_REACTION_UNCHANGED;
    if (0 != toHandleOppoChUd) {
        newVisionReaction = deriveNpcVisionReactionAgainstOppoChUd(currRdfId, currPlayersMap, currNpcsMap, selfNpcCollider, selfNpcBodyID, selfNpcUd, currChd, massProps, currChdFacing, cc, nextChd, cvSupported, cvInAir, cvOnWall, currNotDashing, currEffInAir, oldNextNotDashing, oldNextEffInAir, inJumpStartupOrJustEnded, cvGroundState, canJumpWithinInertia, visionDirection, toHandleOppoChUd, selfNpcPositionDiffForOppoChUd);

        switch (currNpcGoal) {
        case NpcGoal::NIdle:
            outNextNpcGoal = NpcGoal::NHuntThenIdle;
            break;
        case NpcGoal::NIdleIfGoHuntingThenPatrol:
        case NpcGoal::NPatrol:
            outNextNpcGoal = NpcGoal::NHuntThenPatrol;
            break;
        case NpcGoal::NIdleIfGoHuntingThenPathPatrol:
        case NpcGoal::NPathPatrol:
            outNextNpcGoal = NpcGoal::NHuntThenPathPatrol;
            break;
        case NpcGoal::NFollowAlly:
            outNextNpcGoal = NpcGoal::NHuntThenFollowAlly;
            break;
        default:
            break;
        }
    } else {
        switch (currNpcGoal) {
        case NpcGoal::NHuntThenIdle:
            outNextNpcGoal = NpcGoal::NIdle;
            newVisionReaction = TARGET_CH_REACTION_HUNTING_LOSS;
            break;
        case NpcGoal::NHuntThenPatrol:
            outNextNpcGoal = NpcGoal::NPatrol;
            newVisionReaction = TARGET_CH_REACTION_HUNTING_LOSS;
            break;
        case NpcGoal::NHuntThenPathPatrol:
            outNextNpcGoal = NpcGoal::NPathPatrol;
            newVisionReaction = TARGET_CH_REACTION_HUNTING_LOSS;
            break;
        case NpcGoal::NHuntThenFollowAlly:
            outNextNpcGoal = NpcGoal::NFollowAlly;
            newVisionReaction = TARGET_CH_REACTION_HUNTING_LOSS;
            break;
        default:
            break;
        }
    }

    switch (newVisionReaction) {
        case TARGET_CH_REACTION_DEF1:
        case TARGET_CH_REACTION_USE_DRAGONPUNCH:
        case TARGET_CH_REACTION_USE_MELEE:
        case TARGET_CH_REACTION_USE_SLOT_C:
            break;
        case TARGET_CH_REACTION_FOLLOW:
        case TARGET_CH_REACTION_FLEE_OPPO: {
           int groundAndMvBlockerReaction = deriveReactionAgainstGroundAndMvBlocker(currRdfId, antiGravityNorm, gravityMagnitude, biNoLock, selfNpcCollider, selfNpcBodyID, selfNpcUd, outNextNpcGoal, currChd, massProps, currChdFacing, cc, nextChd, cvSupported, cvInAir, cvOnWall, currNotDashing, currEffInAir, oldNextNotDashing, oldNextEffInAir, inJumpStartupOrJustEnded, cvGroundState, canJumpWithinInertia, visionAABB, visionNarrowPhaseInBaseOffset, visionDirection, toHandleMvBlockerBodyID, toHandleMvBlockerUd, currGapToJump, minGapToJump, currGroundMvTolerance, newVisionReaction);

           switch (groundAndMvBlockerReaction) {
               case TARGET_CH_REACTION_STOP_BY_MV_BLOCKER:
               case TARGET_CH_REACTION_JUMP_TOWARDS_MV_BLOCKER:
                   newVisionReaction = groundAndMvBlockerReaction;
                   break;
           }
           break;
       }
       default: {
           int groundAndMvBlockerReaction = deriveReactionAgainstGroundAndMvBlocker(currRdfId, antiGravityNorm, gravityMagnitude, biNoLock, selfNpcCollider, selfNpcBodyID, selfNpcUd, outNextNpcGoal, currChd, massProps, currChdFacing, cc, nextChd, cvSupported, cvInAir, cvOnWall, currNotDashing, currEffInAir, oldNextNotDashing, oldNextEffInAir, inJumpStartupOrJustEnded, cvGroundState, canJumpWithinInertia, visionAABB, visionNarrowPhaseInBaseOffset, visionDirection, toHandleMvBlockerBodyID, toHandleMvBlockerUd, currGapToJump, minGapToJump, currGroundMvTolerance, newVisionReaction);
           newVisionReaction = groundAndMvBlockerReaction;
           break;
        }
    }
    
    InputFrameDecoded ifDecodedHolder;
    uint64_t inheritedCachedCueCmd = BaseBattleCollisionFilter::sanitizeCachedCueCmd(currNpcCachedCueCmd);
    BaseBattleCollisionFilter::decodeInput(inheritedCachedCueCmd, &ifDecodedHolder);
    if (TARGET_CH_REACTION_UNCHANGED == newVisionReaction) {
        // Intentionally left blank
    } else if (TARGET_CH_REACTION_SLIP_JUMP_TOWARDS_CH == newVisionReaction) {
        ifDecodedHolder.set_dx(0);
        ifDecodedHolder.set_dy(-2);
        ifDecodedHolder.set_btn_a_level(1);
    } else if (TARGET_CH_REACTION_TURNAROUND_MV_BLOCKER == newVisionReaction) {
        int toMoveDirX = 0 < visionDirection.GetX() ? -2 : +2;
        ifDecodedHolder.set_dx(toMoveDirX);
        ifDecodedHolder.set_dy(0);
        ifDecodedHolder.set_btn_a_level(0);
    } else if (TARGET_CH_REACTION_JUMP_TOWARDS_CH == newVisionReaction || TARGET_CH_REACTION_JUMP_TOWARDS_MV_BLOCKER == newVisionReaction) {
        int toMoveDirX = 0 < visionDirection.GetX() ? +2 : -2;
        ifDecodedHolder.set_dx(toMoveDirX);
        ifDecodedHolder.set_dy(0);
        ifDecodedHolder.set_btn_a_level(1);
    } else if (TARGET_CH_REACTION_HUNTING_LOSS == newVisionReaction) {
        int inheritedDirX = 0 < visionDirection.GetX() ? +2 : -2;
        switch (outNextNpcGoal) {
        case NpcGoal::NIdle:
            ifDecodedHolder.set_dx(0);
            ifDecodedHolder.set_dy(0);
            ifDecodedHolder.set_btn_a_level(0);
            break;
        default:
            ifDecodedHolder.set_dx(inheritedDirX);
            ifDecodedHolder.set_dy(0);
            ifDecodedHolder.set_btn_a_level(0);
            break;
        }
    } else {
        int toMoveDirX = 0;
        if (NpcGoal::NIdle == currNpcGoal) {
            toMoveDirX = 0;
        } else if (0 != toHandleOppoChUd) {
            toMoveDirX = 0 < selfNpcPositionDiffForOppoChUd.GetX() ? +2 : -2;
        } else if (0 != toHandleAllyUd) {
            toMoveDirX = 0 < selfNpcPositionDiffForAllyUd.GetX() ? +2 : -2;
        } else {
            toMoveDirX = 0 < visionDirection.GetX() ? +2 : -2;
        }
       
        // It's important to unset "BtnALevel" if no proactive jump is implied by vision reaction, otherwise its value will remain even after execution and sanitization
        ifDecodedHolder.set_dx(toMoveDirX);
        ifDecodedHolder.set_dy(0);
        ifDecodedHolder.set_btn_a_level(0);
    }

    uint64_t newCachedCueCmd = BaseBattleCollisionFilter::encodeInput(ifDecodedHolder);
    outCmd = newCachedCueCmd;
}

void BaseNpcReaction::extractKeyEntitiesInVision(int currRdfId, const Vec3& antiGravityNorm, std::unordered_map<uint64_t, const PlayerCharacterDownsync*>& currPlayersMap, std::unordered_map<uint64_t, const NpcCharacterDownsync*>& currNpcsMap, std::unordered_map<uint64_t, const Bullet*>& currBulletsMap, const BodyInterface* biNoLock, const NarrowPhaseQuery* narrowPhaseQuery, const CH_COLLIDER_T* selfNpcCollider, const BodyID& selfNpcBodyID, const uint64_t selfNpcUd, const CharacterDownsync& currChd, const CharacterConfig* cc, CharacterDownsync* nextChd, bool cvSupported, bool cvInAir, bool cvOnWall, bool currNotDashing, bool currEffInAir, bool oldNextNotDashing, bool oldNextEffInAir, bool inJumpStartupOrJustEnded, CharacterBase::EGroundState cvGroundState, const AABox& visionAABB, const Vec3Arg& effVisionOffsetFromNpcChd, const Vec3Arg& visionNarrowPhaseInBaseOffset, const Vec3Arg& visionDirection, const VISION_HIT_COLLECTOR_T& visionCastResultCollector, uint64_t& outToHandleAllyUd, Vec3& outSelfNpcPositionDiffForAllyUd, uint64_t& outToHandleOppoChUd, Vec3& outSelfNpcPositionDiffForOppoChUd, uint64_t& outToHandleOppoBlUd, Vec3& outSelfNpcPositionDiffForOppoBlUd, uint64_t& outToHandleMvBlockerUd, BodyID& outToHandleMvBlockerBodyID, GapToJump& outCurrGapToJump, GapToJump& outMinGapToJump, GapToJump& outCurrGroundMvTolerance) {
    if (!visionCastResultCollector.HadHit()) return;
    bool isCharacterFlying = (currChd.omit_gravity() || cc->omit_gravity());

    const TransformedShape& selfNpcTransformedShape = selfNpcCollider->GetTransformedShape(false);
	const AABox& selfNpcAABB = selfNpcTransformedShape.GetWorldSpaceBounds();
    float selfNpcAABBJumpingAxisAlignment1 = selfNpcAABB.mMax.Dot(antiGravityNorm);
    float selfNpcAABBJumpingAxisAlignment2 = selfNpcAABB.mMin.Dot(antiGravityNorm);
    float selfNpcAABBVisionAlignment1 = selfNpcAABB.mMax.Dot(visionDirection);
    float selfNpcAABBVisionAlignment2 = selfNpcAABB.mMin.Dot(visionDirection);

    float bestVisionAlignmentForOppo = FLT_MAX;
    float bestVisionAlignmentForAlly = FLT_MAX;
    float bestVisionAlignmentForMvBlocker = FLT_MAX;

    const Vec3 lhsPos = selfNpcCollider->GetPosition(false); 
    int hitsCnt = visionCastResultCollector.mHits.size();

    const BodyID& selfNpcGroundBodyID = cvSupported ? selfNpcCollider->GetGroundBodyID() : BodyID();
    float selfNpcGroundAABBJumpingAxisAlignment1 = 0;
    float selfNpcGroundAABBJumpingAxisAlignment2 = 0;
    float selfNpcGroundAABBVisionAlignment1 = 0;
    float selfNpcGroundAABBVisionAlignment2 = 0;
    if (!selfNpcGroundBodyID.IsInvalid()) {
        const TransformedShape& selfNpcGroundTransformedShape = biNoLock->GetTransformedShape(selfNpcGroundBodyID);
        const AABox& selfNpcGroundAABB = selfNpcGroundTransformedShape.GetWorldSpaceBounds();
        selfNpcGroundAABBJumpingAxisAlignment1 = selfNpcGroundAABB.mMax.Dot(antiGravityNorm);
        selfNpcGroundAABBJumpingAxisAlignment2 = selfNpcGroundAABB.mMin.Dot(antiGravityNorm);
        selfNpcGroundAABBVisionAlignment1 = selfNpcGroundAABB.mMax.Dot(visionDirection);
        selfNpcGroundAABBVisionAlignment2 = selfNpcGroundAABB.mMin.Dot(visionDirection);
    }
    for (int i = 0; i < hitsCnt; i++) {
        const CollideShapeCollector::ResultType hit = visionCastResultCollector.mHits.at(i);
        const BodyID rhsBodyID = hit.mBodyID2;
        const float rhsVisionAlignmentFromNpcChdPosition = visionDirection.Dot(hit.mContactPointOn1 + effVisionOffsetFromNpcChd);
        if (!rhsBodyID.IsInvalid() && rhsBodyID == selfNpcGroundBodyID) {
            // [WARNING] When "selfNpcGroundBody" is of complicated shape, it's too inefficient to traverse all its vertices and find the largest projected value on "visionDirection", instead we can just allow "selfNpcGroundBody" to collide with "effVisionShape" and use the immediately visible distance as "currGroundMvTolerance" to roughly decide whether or not we can move on.
            outCurrGroundMvTolerance.set_vision_alignment(rhsVisionAlignmentFromNpcChdPosition);
            continue;
            // [WARNING] Intentionally NOT proceeding from here even if the "rhsBodyID" refers to an opponent character or bullet.
        }
        
        const TransformedShape& rhsTransformedShape = biNoLock->GetTransformedShape(rhsBodyID);
        const AABox& rhsAABB = rhsTransformedShape.GetWorldSpaceBounds();
        const uint64_t udRhs = biNoLock->GetUserData(rhsBodyID);
        const uint64_t udtRhs = BaseBattleCollisionFilter::getUDT(udRhs);
        switch (udtRhs) {
        case UDT_PLAYER: 
        case UDT_NPC: {
            VisionBodyFilter visionRayCastBodyFilter(((const CharacterDownsync*)&currChd), selfNpcBodyID, selfNpcUd, nullptr);
            RRayCast ray(visionNarrowPhaseInBaseOffset, hit.mContactPointOn1);
            bool rayTestPassed = false;
            RayCastResult rcResult;
            narrowPhaseQuery->CastRay(ray, rcResult, {}, {}, visionRayCastBodyFilter); // [REMINDER] "RayCast direction" MUST come with a magnitude, i.e. DON'T just use a normalized vector!
            if (!rcResult.mBodyID.IsInvalid() && rcResult.mBodyID != rhsBodyID) {
                continue;
            }
            const CharacterDownsync* rhsCurrChd = nullptr;
            if (UDT_PLAYER == udtRhs) {
                auto rhsCurrPlayer = currPlayersMap.at(udRhs);
                rhsCurrChd = &(rhsCurrPlayer->chd());
            } else {
                auto rhsCurrNpc = currNpcsMap.at(udRhs);
                rhsCurrChd = &(rhsCurrNpc->chd());
            }
            const Vec3 rhsPos = biNoLock->GetPosition(rhsBodyID);
            const Vec3 selfNpcPositionDiff = rhsPos - lhsPos;
            if (rhsCurrChd->bullet_team_id() != currChd.bullet_team_id()) {
                if (rhsVisionAlignmentFromNpcChdPosition >= bestVisionAlignmentForOppo) {
                    continue;
                }
                bestVisionAlignmentForOppo = rhsVisionAlignmentFromNpcChdPosition;
                 
                outToHandleOppoChUd = udRhs;
                outToHandleOppoBlUd = 0;
                outSelfNpcPositionDiffForOppoChUd = selfNpcPositionDiff;
                outSelfNpcPositionDiffForOppoBlUd = Vec3::sZero();
            } else {
                if (rhsVisionAlignmentFromNpcChdPosition >= bestVisionAlignmentForAlly) {
                    continue;
                }

                bestVisionAlignmentForAlly = rhsVisionAlignmentFromNpcChdPosition;
                outToHandleAllyUd = udRhs;
                outSelfNpcPositionDiffForAllyUd = selfNpcPositionDiff;
            }
            break;
        }
        case UDT_BL: {
            const Bullet* rhsCurrBl = currBulletsMap.at(udRhs);
            const Vec3 rhsPos = biNoLock->GetPosition(rhsBodyID);
            const Vec3 selfNpcPositionDiff = rhsPos - lhsPos;
            if (rhsCurrBl->team_id() != currChd.bullet_team_id()) {
                Vec3 rhsCurrBlFacing = Quat(rhsCurrBl->q_x(), rhsCurrBl->q_y(), rhsCurrBl->q_z(), rhsCurrBl->q_w())*Vec3::sAxisX();
                if (0 <= selfNpcPositionDiff.Dot(rhsCurrBlFacing)) {
                    continue; // seemingly not offensive
                }

                if (rhsVisionAlignmentFromNpcChdPosition > bestVisionAlignmentForOppo) {
                    continue;
                }

                bestVisionAlignmentForOppo = rhsVisionAlignmentFromNpcChdPosition;
                outToHandleOppoBlUd = udRhs;
                outToHandleOppoChUd = 0;
                outSelfNpcPositionDiffForOppoChUd = Vec3::sZero();
                outSelfNpcPositionDiffForOppoBlUd = selfNpcPositionDiff;
            } else {
                // [TODO] Handling of "for ally bullets" 
            }
            break;
        }
        case UDT_OBSTACLE: {
            bool isAlongForwardMv = (0 < rhsVisionAlignmentFromNpcChdPosition);

            if (!isAlongForwardMv) {
                // Not a "movement blocker candidate" 
                continue;
            }
            if (rhsVisionAlignmentFromNpcChdPosition > bestVisionAlignmentForMvBlocker) {
                continue;
            }
            float rhsAABBJumpingAxisAlignment1 = rhsAABB.mMax.Dot(antiGravityNorm);
            float rhsAABBJumpingAxisAlignment2 = rhsAABB.mMin.Dot(antiGravityNorm);
            float rhsAABBVisionAlignment1 = rhsAABB.mMax.Dot(visionDirection);
            float rhsAABBVisionAlignment2 = rhsAABB.mMin.Dot(visionDirection);

            if (!isCharacterFlying) {
                bool strictlyUp = false;

                if (rhsAABBJumpingAxisAlignment1 > rhsAABBJumpingAxisAlignment2) {
                    strictlyUp = (rhsAABBJumpingAxisAlignment2 >= selfNpcAABBJumpingAxisAlignment1);
                } else {    
                    strictlyUp = (rhsAABBJumpingAxisAlignment1 >= selfNpcAABBJumpingAxisAlignment2);
                }

                bool holdableBothForwardAndBackward = false;
                if (rhsAABBVisionAlignment1 > rhsAABBVisionAlignment2) {
                    holdableBothForwardAndBackward = (rhsAABBVisionAlignment1 >= selfNpcAABBVisionAlignment1 && rhsAABBVisionAlignment2 <= selfNpcAABBVisionAlignment2); 
                } else {
                    holdableBothForwardAndBackward = (rhsAABBVisionAlignment1 <= selfNpcAABBVisionAlignment1 && rhsAABBVisionAlignment2 >= selfNpcAABBVisionAlignment2);
                }
                if (strictlyUp && holdableBothForwardAndBackward) {
                    // Not a "movement blocker candidate" 
                    continue;
                }
            }
            bool rayTestPassed = false, rayTestCanIgnoreSelfNpcGround = false;

            if (rhsAABBVisionAlignment1 > rhsAABBVisionAlignment2 && rhsAABBVisionAlignment1 > selfNpcGroundAABBVisionAlignment1) {
                if (rhsAABBJumpingAxisAlignment1 > rhsAABBJumpingAxisAlignment2 ) {
                    rayTestCanIgnoreSelfNpcGround = (rhsAABBJumpingAxisAlignment1 <= selfNpcGroundAABBJumpingAxisAlignment2);
                } else {
                    rayTestCanIgnoreSelfNpcGround = (rhsAABBJumpingAxisAlignment2 <= selfNpcGroundAABBJumpingAxisAlignment1);
                }
            } else if (rhsAABBVisionAlignment2 > rhsAABBVisionAlignment1 && rhsAABBVisionAlignment2 > selfNpcGroundAABBVisionAlignment2) {
                if (rhsAABBJumpingAxisAlignment1 > rhsAABBJumpingAxisAlignment2) {
                    rayTestCanIgnoreSelfNpcGround = (rhsAABBJumpingAxisAlignment1 <= selfNpcGroundAABBJumpingAxisAlignment2);
                } else {
                    rayTestCanIgnoreSelfNpcGround = (rhsAABBJumpingAxisAlignment2 <= selfNpcGroundAABBJumpingAxisAlignment1);
                }
            }

            VisionBodyFilter visionRayCastBodyFilter(((const CharacterDownsync*)&currChd), selfNpcBodyID, selfNpcUd, nullptr);
            RRayCast ray(visionNarrowPhaseInBaseOffset, hit.mContactPointOn1);
            RayCastResult rcResult;
            narrowPhaseQuery->CastRay(ray, rcResult, {}, {}, visionRayCastBodyFilter); // [REMINDER] "RayCast direction" MUST come with a magnitude, i.e. DON'T just use a normalized vector!
            if (!rcResult.mBodyID.IsInvalid() && rcResult.mBodyID != rhsBodyID) {

                if (!rayTestCanIgnoreSelfNpcGround && rcResult.mBodyID == selfNpcGroundBodyID) {
                    /* 
                    // [REMINDER] If "selfNpcGroundBodyID" has too much overlapping volume with "rhsBodyID" (which ideally it shouldn't have any), then "rhsBodyID" might fail this RayCast test.
#ifndef NDEBUG
                    std::ostringstream oss;
                    oss << "@currRdfId=" << currRdfId << ", selfNpcUd=" << selfNpcUd << " has visionAABB=(minX=" << visionAABB.mMin.GetX() << ", maxX=" << visionAABB.mMax.GetX() << ", minY=" << visionAABB.mMin.GetY() << ", maxY=" << visionAABB.mMax.GetY() << "), curr_ch_state=" << currChd.ch_state() << ", curr_frames_in_ch_state=" << currChd.frames_in_ch_state() << ", cvSupported=" << cvSupported << ", vision hit rhsBodyID=" << rhsBodyID.GetIndexAndSequenceNumber() << " is invalid due to being blocked by rcResult.mBodyID=" << rcResult.mBodyID.GetIndexAndSequenceNumber();
                    Debug::Log(oss.str(), DColor::Orange);
#endif
                */
                    continue;
                }
            }
            
            bestVisionAlignmentForMvBlocker = rhsVisionAlignmentFromNpcChdPosition;
            outToHandleMvBlockerUd = udRhs;
            outToHandleMvBlockerBodyID = rhsBodyID;

            // Calc "outCurrGapToJump"
            if (rhsAABBVisionAlignment1 > rhsAABBVisionAlignment2) {
                outCurrGapToJump.set_vision_alignment(rhsAABBVisionAlignment2 - selfNpcAABBVisionAlignment1);
            } else {    
                outCurrGapToJump.set_vision_alignment(rhsAABBVisionAlignment1 - selfNpcAABBVisionAlignment2);
            }

            if (rhsAABBJumpingAxisAlignment1 > rhsAABBJumpingAxisAlignment2) {
                outCurrGapToJump.set_anti_gravity_alignment(rhsAABBJumpingAxisAlignment1 - selfNpcAABBJumpingAxisAlignment2);
            } else {    
                outCurrGapToJump.set_anti_gravity_alignment(rhsAABBJumpingAxisAlignment2 - selfNpcAABBJumpingAxisAlignment1);
            }

            if (!selfNpcGroundBodyID.IsInvalid()) {
                // Calc "minGapToJump"
                if (rhsAABBVisionAlignment1 > rhsAABBVisionAlignment2) {
                    outMinGapToJump.set_vision_alignment(rhsAABBVisionAlignment2 - selfNpcGroundAABBVisionAlignment1);
                } else {    
                    outMinGapToJump.set_vision_alignment(rhsAABBVisionAlignment1 - selfNpcGroundAABBVisionAlignment2);
                }

                if (rhsAABBJumpingAxisAlignment1 > rhsAABBJumpingAxisAlignment2) {
                    outMinGapToJump.set_anti_gravity_alignment(rhsAABBJumpingAxisAlignment1 - selfNpcGroundAABBJumpingAxisAlignment1);
                } else {    
                    outMinGapToJump.set_anti_gravity_alignment(rhsAABBJumpingAxisAlignment2 - selfNpcGroundAABBJumpingAxisAlignment2);
                }
            }
            break;
        }
        default:
            break;
        }
    }

    // In case there's some unintentional drawing overlap.
    if (FLT_MAX != outCurrGroundMvTolerance.vision_alignment() && outCurrGroundMvTolerance.vision_alignment() > outCurrGapToJump.vision_alignment()) {
        outCurrGroundMvTolerance.set_vision_alignment(outCurrGapToJump.vision_alignment());
    }
}

int BaseNpcReaction::deriveNpcVisionReactionAgainstOppoChUd(int currRdfId, std::unordered_map<uint64_t, const PlayerCharacterDownsync*>& currPlayersMap, std::unordered_map<uint64_t, const NpcCharacterDownsync*>& currNpcsMap, const CH_COLLIDER_T* selfNpcCollider, const BodyID& selfNpcBodyID, const uint64_t selfNpcUd, const CharacterDownsync& currChd, const MassProperties& massProps, const Vec3& currChdFacing, const CharacterConfig* cc, CharacterDownsync* nextChd, bool cvSupported, bool cvInAir, bool cvOnWall, bool currNotDashing, bool currEffInAir, bool oldNextNotDashing, bool oldNextEffInAir, bool inJumpStartupOrJustEnded, CharacterBase::EGroundState cvGroundState, bool canJumpWithinInertia, const Vec3& visionDirection, const uint64_t toHandleOppoChUd, const Vec3& selfNpcPositionDiffForOppoChUd) {
    int ret = TARGET_CH_REACTION_UNCHANGED;
    bool opponentBehindMe = (0 > (selfNpcPositionDiffForOppoChUd.GetX() * visionDirection.GetX()));
    bool opponentAboveMe = cc->capsule_half_height() < selfNpcPositionDiffForOppoChUd.GetY();
    const CharacterDownsync* rhsCurrChd = nullptr;
    const uint64_t udtRhs = BaseBattleCollisionFilter::getUDT(toHandleOppoChUd);
    if (UDT_PLAYER == udtRhs) {
        auto rhsCurrPlayer = currPlayersMap.at(toHandleOppoChUd);
        rhsCurrChd = &(rhsCurrPlayer->chd());
    } else {
        auto rhsCurrNpc = currNpcsMap.at(toHandleOppoChUd);
        rhsCurrChd = &(rhsCurrNpc->chd());
    }

    if (!opponentBehindMe) {
        // Opponent is in front of me
        ret = TARGET_CH_REACTION_FOLLOW;
        // TODO: Use skill?
    } else {
        // Opponent is behind me
        if (0 >= cc->speed()) {
            // e.g. Tower
        } else {
            bool opponentIsAttacking = !nonAttackingSet.count(rhsCurrChd->ch_state());
            Quat oppoChdQ ;
            Vec3 oppoFacing; 
            BaseBattleCollisionFilter::calcChdFacing(*rhsCurrChd, oppoChdQ, oppoFacing);
            bool oppoIsFacingMe = (0 > selfNpcPositionDiffForOppoChUd.GetX() * oppoFacing.GetX());
            if ((opponentIsAttacking && oppoIsFacingMe)) {
                ret = TARGET_CH_REACTION_FOLLOW;
            } else {
                ret = TARGET_CH_REACTION_FLEE_OPPO;
            }
        }
    }

    return ret;
}

int BaseNpcReaction::deriveReactionAgainstGroundAndMvBlocker(int currRdfId, const Vec3& antiGravityNorm, const float gravityMagnitude, const BodyInterface* biNoLock, const CH_COLLIDER_T* selfNpcCollider, const BodyID& selfNpcBodyID, const uint64_t selfNpcUd, const NpcGoal inNpcGoal, const CharacterDownsync& currChd, const MassProperties& massProps, const Vec3& currChdFacing, const CharacterConfig* cc, CharacterDownsync* nextChd, bool cvSupported, bool cvInAir, bool cvOnWall, bool currNotDashing, bool currEffInAir, bool oldNextNotDashing, bool oldNextEffInAir, bool inJumpStartupOrJustEnded, CharacterBase::EGroundState cvGroundState, bool canJumpWithinInertia, const AABox& visionAABB, const Vec3Arg& visionNarrowPhaseInBaseOffset, const Vec3& visionDirection, const BodyID& toHandleMvBlockerBodyID, const uint64_t toHandleMvBlockerUd, const GapToJump& currGapToJump, const GapToJump& minGapToJump, const GapToJump& currGroundMvTolerance, const int visionReactionByFar) {
    
    if (NpcGoal::NIdle == inNpcGoal || NpcGoal::NIdleIfGoHuntingThenPatrol == inNpcGoal || NpcGoal::NIdleIfGoHuntingThenPathPatrol == inNpcGoal) {
        return visionReactionByFar;
    }

    int newVisionReaction = visionReactionByFar;

    const TransformedShape& selfNpcTransformedShape = selfNpcCollider->GetTransformedShape(false);
	const AABox& selfNpcAABB = selfNpcTransformedShape.GetWorldSpaceBounds();
    bool isCharacterFlying = (currChd.omit_gravity() || cc->omit_gravity());
    
    bool temptingToMove = (temptingToMoveNpcGoalSet.count(inNpcGoal)) && (canJumpWithinInertia || isCharacterFlying);

    /*
    [WARNING] DON'T use "selfNpcCollider->GetLinearVelocity()" to evaluate "currGroundCanHoldMeIfWalkOn". 

    When a jumping character touches the vertical-side-edge of a higher platform, its velocity might be calculated by the ContactManager to an opposite direction than the vision direction.  
    */
    const float potentialMv = cc->speed()*globalPrimitiveConsts->estimated_seconds_per_rdf();

    bool currGroundCanHoldMeIfWalkOn = (currGroundMvTolerance.vision_alignment() >= potentialMv);
    bool toHandleMvBlockerCanHoldMeIfWalkOn = false;
    const Vec3& chColliderVel = selfNpcCollider->GetLinearVelocity(false);
    const float constraintVelXDiff = chColliderVel.GetX() - nextChd->vel_x();
    bool hasEffectiveMvBlocker = (0 > constraintVelXDiff*nextChd->vel_x()) && !BaseBattleCollisionFilter::IsLengthNearZero(constraintVelXDiff) && walkingSet.count(currChd.ch_state());

    if (cvSupported) {
        if (!currGroundCanHoldMeIfWalkOn || hasEffectiveMvBlocker) {
            /*
            [WARNING] Don't IMMEDIATELY return the "newVisionReaction" if "0 != toHandleMvBlockerUd", there might be still chance to jump onto a horizontally forward holding platform.
            */
            if (temptingToMove) {
                newVisionReaction = TARGET_CH_REACTION_TURNAROUND_MV_BLOCKER;
            } else {
                newVisionReaction = TARGET_CH_REACTION_STOP_BY_MV_BLOCKER;
            }
        } else {
            if (temptingToMove) {
                newVisionReaction = TARGET_CH_REACTION_WALK_ALONG;
            } else {
                newVisionReaction = TARGET_CH_REACTION_STOP_BY_MV_BLOCKER;
            }
        }
    } else {
        if (temptingToMove) {
            newVisionReaction = TARGET_CH_REACTION_WALK_ALONG;
        } else {
            newVisionReaction = TARGET_CH_REACTION_STOP_BY_MV_BLOCKER;
        }
    }

    if (0 == toHandleMvBlockerUd) {
/*
#ifndef NDEBUG
        if (selfNpcUd == 8589934593 && TARGET_CH_REACTION_UNCHANGED != newVisionReaction) {
            if (TARGET_CH_REACTION_WALK_ALONG != newVisionReaction || (currRdfId % 16 == 0)) {
                std::ostringstream oss;
                oss << "@currRdfId=" << currRdfId << ", selfNpcUd=" << selfNpcUd << " has visionAABB=(minX=" << visionAABB.mMin.GetX() << ", maxX=" << visionAABB.mMax.GetX() << ", minY=" << visionAABB.mMin.GetY() << ", maxY=" << visionAABB.mMax.GetY() << "), curr_ch_state=" << currChd.ch_state() << ", curr_frames_in_ch_state=" << currChd.frames_in_ch_state() << ", cvSupported=" << cvSupported << ", selfNpcGroundBodyID=" << selfNpcCollider->GetGroundBodyID().GetIndexAndSequenceNumber() << ", there's no toHandleMvBlockerBodyID, canJumpWithinInertia=" << canJumpWithinInertia << ", hasEffectiveMvBlocker=" << hasEffectiveMvBlocker << ", currGroundMvTolerance=" << currGroundMvTolerance.vision_alignment() << ", returning newVisionReaction = " << newVisionReaction;
                Debug::Log(oss.str(), DColor::Orange);
            } 
        }
#endif
*/
        return newVisionReaction;
    }

    bool isMinGapJumpable = false, isCurrGapJumpable = false;
    float currGapEstimatedSpeedX = BaseBattleCollisionFilter::IsLengthNearZero(currChd.vel_x()) ? 0.7f * cc->speed() : 0.8f * std::abs(currChd.vel_x());
    float minGapEstimatedSpeedX = 0.8f * cc->speed();
    float currGapToJumpVisionAlignment = currGapToJump.vision_alignment(), currGapToJumpAntiGravityAlignment = currGapToJump.anti_gravity_alignment();

    if (FLT_MAX != currGapToJumpVisionAlignment && cvSupported) {
        /*
        [TODO] Handle the following 2 cases.
        - "selfNpcGroundBodyID" being a slope that I can just walk along.
        - "toHandleMvBlockerBodyID" being a slope that I can just walk onto.
        */

        /*
        When a character jumps at (x=0, y=0) with "forwardSpeed (in the re-aligned x-axis)" and "chJumpInitSpeed" (in the re-aligned y-axis), the trajectory (in ISO units) is
        - x(t) = forwardSpeed*t
        - y(t) = chJumpInitSpeed*t - 0.5*gravityMagnitude*t where "gravityMagnitude > 0"
        */
        const float jumpAccMagY = cc->jump_acc_mag_y();
        const int jumpStartupFrames = cc->jump_startup_frames();
        const float chJumpAccSeconds = ((jumpStartupFrames + 1) * globalPrimitiveConsts->estimated_seconds_per_rdf());
        const float chJumpInitSpeed = jumpAccMagY * chJumpAccSeconds;
        const float extraAccendingY = ((chJumpInitSpeed * 0.5f) * chJumpAccSeconds);
        isMinGapJumpable = isGapJumpable(gravityMagnitude, minGapToJump.vision_alignment() + cc->capsule_radius(), minGapToJump.anti_gravity_alignment(), minGapEstimatedSpeedX, chJumpAccSeconds, chJumpInitSpeed, extraAccendingY);
        isCurrGapJumpable = isGapJumpable(gravityMagnitude, currGapToJumpVisionAlignment + cc->capsule_radius(), currGapToJumpAntiGravityAlignment, currGapEstimatedSpeedX, chJumpAccSeconds, chJumpInitSpeed, extraAccendingY);
        toHandleMvBlockerCanHoldMeIfWalkOn = isCurrGapJumpable && (0 >= currGapToJumpVisionAlignment && 0 >= currGapToJumpAntiGravityAlignment);
    }

    newVisionReaction = deriveReactionAgainstMvBlockerAfterApproximation(currRdfId, antiGravityNorm, selfNpcUd, currChd, massProps, currChdFacing, cvSupported, canJumpWithinInertia, isMinGapJumpable, isCurrGapJumpable, currGroundCanHoldMeIfWalkOn, toHandleMvBlockerCanHoldMeIfWalkOn, currGapToJumpVisionAlignment, temptingToMove, newVisionReaction);
/*
#ifndef NDEBUG
    if (selfNpcUd == 8589934593 && TARGET_CH_REACTION_UNCHANGED != newVisionReaction) {
        if (TARGET_CH_REACTION_WALK_ALONG != newVisionReaction || (currRdfId % 16 == 0)) {
            std::ostringstream oss;
            oss << "@currRdfId=" << currRdfId << ", selfNpcUd=" << selfNpcUd << " has visionDir=(" << visionDirection.GetX() << ", " << visionDirection.GetY() << "), visionAABB=(minX=" << visionAABB.mMin.GetX() << ", maxX=" << visionAABB.mMax.GetX() << ", minY=" << visionAABB.mMin.GetY() << ", maxY=" << visionAABB.mMax.GetY() << "), curr_ch_state=" << currChd.ch_state() << ", curr_frames_in_ch_state=" << currChd.frames_in_ch_state() << ", cvSupported=" << cvSupported << ", selfNpcGroundBodyID=" << selfNpcCollider->GetGroundBodyID().GetIndexAndSequenceNumber() << ", toHandleMvBlockerBodyID=" << toHandleMvBlockerBodyID.GetIndexAndSequenceNumber() << ", canJumpWithinInertia=" << canJumpWithinInertia << ", isMinGapJumpable=" << isMinGapJumpable << ", isCurrGapJumpable=" << isCurrGapJumpable << ", currGroundCanHoldMeIfWalkOn=" << currGroundCanHoldMeIfWalkOn << ", toHandleMvBlockerCanHoldMeIfWalkOn=" << toHandleMvBlockerCanHoldMeIfWalkOn << ", currGapToJumpVisionAlignment=" << currGapToJumpVisionAlignment << ", temptingToMove=" << temptingToMove << "; currGroundMvTolerance=" << currGroundMvTolerance.vision_alignment() << ", minGapToJump=(" << minGapToJump.vision_alignment() << ", " << minGapToJump.anti_gravity_alignment() << "), currGapToJump=(" <<  currGapToJump.vision_alignment() << ", " << currGapToJump.anti_gravity_alignment() << "); returning newVisionReaction = " << newVisionReaction;
            Debug::Log(oss.str(), DColor::Orange);
        } 
    }
#endif
*/
    return newVisionReaction;
}

int BaseNpcReaction::deriveReactionAgainstMvBlockerAfterApproximation(int currRdfId, const Vec3& antiGravityNorm, const uint64_t selfNpcUd, const CharacterDownsync& currChd, const MassProperties& massProps, const Vec3& currChdFacing, const bool cvSupported, const bool canJumpWithinInertia, const bool isMinGapJumpable, const bool isCurrGapJumpable, const bool currGroundCanHoldMeIfWalkOn, const bool toHandleMvBlockerCanHoldMeIfWalkOn, const float currGapToJumpVisionAlignment, const bool temptingToMove, const int visionReactionByFar) {
    int newVisionReaction = visionReactionByFar;
    if (!cvSupported) {
        if (temptingToMove) {
            newVisionReaction = TARGET_CH_REACTION_WALK_ALONG;
        } else {
            newVisionReaction = TARGET_CH_REACTION_UNCHANGED;
        }
    } else if (canJumpWithinInertia && isCurrGapJumpable) {
        if (temptingToMove) {
            if (toHandleMvBlockerCanHoldMeIfWalkOn) {
                newVisionReaction = TARGET_CH_REACTION_WALK_ALONG;
            } else {
                newVisionReaction = TARGET_CH_REACTION_JUMP_TOWARDS_MV_BLOCKER;
            }
        } else {
            newVisionReaction = TARGET_CH_REACTION_STOP_BY_MV_BLOCKER;
        }
    } else {
        if (temptingToMove) {
            if (currGroundCanHoldMeIfWalkOn) {
                if (TURNAROUND_FROM_MV_BLOCKER_DX_THRESHOLD < currGapToJumpVisionAlignment) {
                    newVisionReaction = TARGET_CH_REACTION_WALK_ALONG;
                } else {
                    if (isMinGapJumpable) {
                        newVisionReaction = TARGET_CH_REACTION_WALK_ALONG;
                    } else {
                        newVisionReaction = TARGET_CH_REACTION_TURNAROUND_MV_BLOCKER;
                    }
                }
            } else {
                // There's no need to go any further for jumping
                newVisionReaction = TARGET_CH_REACTION_TURNAROUND_MV_BLOCKER;
            }
        } else {
            newVisionReaction = TARGET_CH_REACTION_STOP_BY_MV_BLOCKER;
        }
    }

    return newVisionReaction;
}

bool BaseNpcReaction::isGapJumpable(const float gravityMagnitude, const float forwardDistanceAbs, const float jumpingAxisDistance, const float forwardSpeed, const float chJumpAccSeconds, const float chJumpInitSpeed, const float extraAccendingY) {
    
    if (0 >= forwardSpeed) return false;
    if (0 >= forwardDistanceAbs) {
        // Only need evaluate if we can jump vertically first and then slowly move over onto the new platform.
        float airingTimeSingleTrip = (chJumpInitSpeed / gravityMagnitude) - chJumpAccSeconds;
        float estimatedYHighestInTrajectory = extraAccendingY + 0.5f*chJumpInitSpeed*airingTimeSingleTrip;
        return estimatedYHighestInTrajectory > jumpingAxisDistance;
    }
    float estimatedTSeconds = forwardDistanceAbs / forwardSpeed;
    float estimatedTSecondsExcludingAccending = (estimatedTSeconds - chJumpAccSeconds);

    float estimatedYInTrajectory = extraAccendingY + chJumpInitSpeed * estimatedTSecondsExcludingAccending - 0.5f * gravityMagnitude * estimatedTSecondsExcludingAccending * estimatedTSecondsExcludingAccending;
    return estimatedYInTrajectory > jumpingAxisDistance;
}
