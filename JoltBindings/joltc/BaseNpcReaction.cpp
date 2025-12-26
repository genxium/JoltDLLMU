#include "BaseNpcReaction.h"
#include "CharacterCollideShapeCollector.h"
#include "CollisionLayers.h"
#include "CollisionCallbacks.h"

#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/TaperedCylinderShape.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>

#include <climits>

void BaseNpcReaction::postStepDeriveNpcVisionReaction(int currRdfId, std::unordered_map<uint64_t, const PlayerCharacterDownsync*>& currPlayersMap, std::unordered_map<uint64_t, const NpcCharacterDownsync*>& currNpcsMap, std::unordered_map<uint64_t, const Bullet*>& currBulletsMap, const BodyInterface* biNoLock, const NarrowPhaseQuery* narrowPhaseQuery, const BaseBattleCollisionFilter* baseBattleFilter, const DefaultBroadPhaseLayerFilter& bplf, const DefaultObjectLayerFilter& olf, const CH_COLLIDER_T* selfNpcCollider, const BodyID& selfNpcBodyID, const uint64_t selfNpcUd, const NpcGoal currNpcGoal, const uint64_t currNpcCachedCueCmd, const CharacterDownsync& currChd, const CharacterConfig* cc, CharacterDownsync* nextChd, bool cvSupported, bool cvInAir, bool cvOnWall, bool currNotDashing, bool currEffInAir, bool oldNextNotDashing, bool oldNextEffInAir, bool inJumpStartupOrJustEnded, CharacterBase::EGroundState cvGroundState, NpcGoal& outNextNpcGoal, uint64_t& outCmd) {

    Vec3 initVisionOffset(cc->vision_offset_x(), cc->vision_offset_y(), 0);
    auto visionInitTransform = cTurn90DegsAroundZAxisMat.PostTranslated(initVisionOffset); // Rotate, and then translate
    Vec3 selfNpcPosition;
    Quat selfNpcRotation;
    selfNpcCollider->GetPositionAndRotation(selfNpcPosition, selfNpcRotation, false);
    auto visionCOMTransform = (JPH::Mat44::sRotation(selfNpcRotation)*visionInitTransform).PostTranslated(selfNpcPosition); //and then rotate again by the NPC's orientation (affecting "initVisionOffset" too), and finally apply the NPC's position as translation
    
    float visionHalfHeight = cc->vision_half_height(), visionTopRadius = cc->vision_top_radius(), visionBottomRadius = cc->vision_bottom_radius();
    float visionConvexRadius = (visionTopRadius < visionBottomRadius ? visionTopRadius : visionBottomRadius)*0.9f; // Must be smaller than the min of these two 
    TaperedCylinderShapeSettings initVisionShapeSettings(visionHalfHeight, visionTopRadius, visionBottomRadius, visionConvexRadius);
    TaperedCylinderShapeSettings::ShapeResult shapeResult;
    TaperedCylinderShape initVisionShape(initVisionShapeSettings, shapeResult); // [WARNING] A transient, on-stack shape only bound to lifecycle of the current function & current thread. 

    // Moreover, the center-of-mass DOESN'T have a local y-coordinate "0" within "initVisionShape" when (visionTopRadius != visionBottomRadius).

    initVisionShape.SetEmbedded(); // To allow deallocation on-stack, i.e. the "mRefCount" will equal 1 when it deallocates on-stack with the current function closure.
    
    const TaperedCylinderShape* effVisionShape = &initVisionShape; 

    const Vec3 visionNarrowPhaseInBaseOffset = selfNpcPosition + selfNpcRotation * initVisionOffset;

    const Vec3 visionDirection = Quat(nextChd->q_x(), nextChd->q_y(), nextChd->q_z(), nextChd->q_w())*Vec3::sAxisX(); // [WARNING] Don't use "selfNpcCollider->GetRotation(false)" which DIDN'T respect the input from "npc-pre-physics-update" job!
  
    const BodyID& selfNpcGroundBodyID = cvSupported ? selfNpcCollider->GetGroundBodyID() : BodyID();
    VisionBodyFilter visionBodyFilter(((const CharacterDownsync*)&currChd), selfNpcBodyID, selfNpcGroundBodyID, selfNpcUd, baseBattleFilter);

    VISION_HIT_COLLECTOR_T visionHitCollector;
    auto scaling = Vec3::sOne();
    
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
    Vec3 selfNpcPositionDiffForAllyUd, selfNpcPositionDiffForOppoChUd, selfNpcPositionDiffForOppoBlUd, outVisionPenetrationAgainstMvBlocker;

    BodyID toHandleMvBlockerBodyID;

    extractKeyEntitiesInVision(currRdfId, currPlayersMap, currNpcsMap, currBulletsMap, biNoLock, narrowPhaseQuery, selfNpcCollider, selfNpcBodyID, selfNpcUd, currChd, cc, nextChd, cvSupported, cvInAir, cvOnWall, currNotDashing, currEffInAir, oldNextNotDashing, oldNextEffInAir, inJumpStartupOrJustEnded, cvGroundState, visionAABB, visionNarrowPhaseInBaseOffset, visionDirection, visionHitCollector, toHandleAllyUd, selfNpcPositionDiffForAllyUd, toHandleOppoChUd, selfNpcPositionDiffForOppoChUd, toHandleOppoBlUd, selfNpcPositionDiffForOppoBlUd, toHandleMvBlockerUd, toHandleMvBlockerBodyID, outVisionPenetrationAgainstMvBlocker);

    bool notDashing = BaseBattleCollisionFilter::chIsNotDashing(*nextChd);
    bool canJumpWithinInertia = BaseBattleCollisionFilter::chCanJumpWithInertia(currChd, cc, notDashing);

    int newVisionReaction = TARGET_CH_REACTION_UNCHANGED;
    if (0 != toHandleOppoChUd) {
        newVisionReaction = deriveNpcVisionReactionAgainstOppoChUd(currRdfId, currPlayersMap, currNpcsMap, selfNpcCollider, selfNpcBodyID, selfNpcUd, currChd, cc, nextChd, cvSupported, cvInAir, cvOnWall, currNotDashing, currEffInAir, oldNextNotDashing, oldNextEffInAir, inJumpStartupOrJustEnded, cvGroundState, canJumpWithinInertia, visionDirection, toHandleOppoChUd, selfNpcPositionDiffForOppoChUd);

        switch (currNpcGoal) {
        case NpcGoal::NIdle:
            outNextNpcGoal = NpcGoal::NHuntThenIdle;
            break;
        case NpcGoal::NIdleIfGoHuntingThenPatrol:
        case NpcGoal::NPatrol:
            outNextNpcGoal = NpcGoal::NHuntThenPatrol;
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
           int groundAndMvBlockerReaction = deriveReactionAgainstGroundAndMvBlocker(currRdfId, biNoLock, selfNpcCollider, selfNpcBodyID, selfNpcUd, outNextNpcGoal, currChd, cc, nextChd, cvSupported, cvInAir, cvOnWall, currNotDashing, currEffInAir, oldNextNotDashing, oldNextEffInAir, inJumpStartupOrJustEnded, cvGroundState, canJumpWithinInertia, visionAABB, visionNarrowPhaseInBaseOffset, visionDirection, outVisionPenetrationAgainstMvBlocker, toHandleMvBlockerBodyID, toHandleMvBlockerUd, newVisionReaction);

           switch (groundAndMvBlockerReaction) {
               case TARGET_CH_REACTION_STOP_BY_MV_BLOCKER:
               case TARGET_CH_REACTION_JUMP_TOWARDS_MV_BLOCKER:
                   newVisionReaction = groundAndMvBlockerReaction;
                   break;
           }
           break;
       }
       default: {
           int groundAndMvBlockerReaction = deriveReactionAgainstGroundAndMvBlocker(currRdfId, biNoLock, selfNpcCollider, selfNpcBodyID, selfNpcUd, outNextNpcGoal, currChd, cc, nextChd, cvSupported, cvInAir, cvOnWall, currNotDashing, currEffInAir, oldNextNotDashing, oldNextEffInAir, inJumpStartupOrJustEnded, cvGroundState, canJumpWithinInertia, visionAABB, visionNarrowPhaseInBaseOffset, visionDirection, outVisionPenetrationAgainstMvBlocker, toHandleMvBlockerBodyID, toHandleMvBlockerUd, newVisionReaction);
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

void BaseNpcReaction::extractKeyEntitiesInVision(int currRdfId, std::unordered_map<uint64_t, const PlayerCharacterDownsync*>& currPlayersMap, std::unordered_map<uint64_t, const NpcCharacterDownsync*>& currNpcsMap, std::unordered_map<uint64_t, const Bullet*>& currBulletsMap, const BodyInterface* biNoLock, const NarrowPhaseQuery* narrowPhaseQuery, const CH_COLLIDER_T* selfNpcCollider, const BodyID& selfNpcBodyID, const uint64_t selfNpcUd, const CharacterDownsync& currChd, const CharacterConfig* cc, CharacterDownsync* nextChd, bool cvSupported, bool cvInAir, bool cvOnWall, bool currNotDashing, bool currEffInAir, bool oldNextNotDashing, bool oldNextEffInAir, bool inJumpStartupOrJustEnded, CharacterBase::EGroundState cvGroundState, const AABox& visionAABB, const Vec3Arg& visionNarrowPhaseInBaseOffset, const Vec3Arg& visionDirection, const VISION_HIT_COLLECTOR_T& visionCastResultCollector, uint64_t& outToHandleAllyUd, Vec3& outSelfNpcPositionDiffForAllyUd, uint64_t& outToHandleOppoChUd, Vec3& outSelfNpcPositionDiffForOppoChUd, uint64_t& outToHandleOppoBlUd, Vec3& outSelfNpcPositionDiffForOppoBlUd, uint64_t& outToHandleMvBlockerUd, BodyID& outToHandleMvBlockerBodyID, Vec3& outVisionPenetrationAgainstMvBlocker) {
    if (!visionCastResultCollector.HadHit()) return;
    bool isCharacterFlying = (currChd.omit_gravity() || cc->omit_gravity());

    const TransformedShape& selfNpcTransformedShape = selfNpcCollider->GetTransformedShape(false);
	const AABox& selfNpcWWorldSpaceBounds = selfNpcTransformedShape.GetWorldSpaceBounds();

    float bestVisionAlignmentForOppo = FLT_MAX;
    float bestVisionAlignmentForAlly = FLT_MAX;
    float bestVisionAlignmentForMvBlocker = FLT_MAX;

    const Vec3 lhsPos = selfNpcCollider->GetPosition(false); 
    int hitsCnt = visionCastResultCollector.mHits.size();

    VisionBodyFilter visionRayCastBodyFilter(((const CharacterDownsync*)&currChd), selfNpcBodyID, BodyID(), selfNpcUd, nullptr);
    for (int i = 0; i < hitsCnt; i++) {
        const CollideShapeCollector::ResultType hit = visionCastResultCollector.mHits.at(i);
        const BodyID rhsBodyID = hit.mBodyID2;
        RRayCast ray(visionNarrowPhaseInBaseOffset, hit.mContactPointOn1);
        bool rayTestPassed = false;
        RayCastResult rcResult;
        narrowPhaseQuery->CastRay(ray, rcResult, {}, {}, visionRayCastBodyFilter); // [REMINDER] "RayCast direction" MUST come with a magnitude, i.e. DON'T just use a normalized vector!
        if (!rcResult.mBodyID.IsInvalid() && rcResult.mBodyID != rhsBodyID) {
            /*
            [REMINDER] If "selfNpcGroundBodyID" has too much overlapping volume with "rhsBodyID" (which ideally it shouldn't have any), then "rhsBodyID" might fail this RayCast test.  
            */
/*
#ifndef NDEBUG
            if (selfNpcUd == 8589934593) {
                std::ostringstream oss;
                oss << "@currRdfId=" << currRdfId << ", selfNpcUd=" << selfNpcUd << " has visionAABB=(minX=" << visionAABB.mMin.GetX() << ", maxX=" << visionAABB.mMax.GetX() << ", minY=" << visionAABB.mMin.GetY() << ", maxY=" << visionAABB.mMax.GetY() << "), curr_ch_state=" << currChd.ch_state() << ", curr_frames_in_ch_state=" << currChd.frames_in_ch_state() << ", cvSupported=" << cvSupported << ", vision hit rhsBodyID=" << rhsBodyID.GetIndexAndSequenceNumber() << " is invalid due to being blocked by rcResult.mBodyID=" << rcResult.mBodyID.GetIndexAndSequenceNumber();
                Debug::Log(oss.str(), DColor::Orange);
            }
#endif
*/
            continue; 
        }
        const TransformedShape& rhsTransformedShape = biNoLock->GetTransformedShape(rhsBodyID);
        const AABox& rhsAABB = rhsTransformedShape.GetWorldSpaceBounds();
        const uint64_t udRhs = biNoLock->GetUserData(rhsBodyID);
        const uint64_t udtRhs = BaseBattleCollisionFilter::getUDT(udRhs);
        const float visionAlignment = visionDirection.Dot(hit.mContactPointOn1);
        switch (udtRhs) {
        case UDT_PLAYER: 
        case UDT_NPC: {
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
                if (visionAlignment >= bestVisionAlignmentForOppo) {
                    continue;
                }
                bestVisionAlignmentForOppo = visionAlignment;
                 
                outToHandleOppoChUd = udRhs;
                outToHandleOppoBlUd = 0;
                outSelfNpcPositionDiffForOppoChUd = selfNpcPositionDiff;
                outSelfNpcPositionDiffForOppoBlUd = Vec3::sZero();
            } else {
                if (visionAlignment >= bestVisionAlignmentForAlly) {
                    continue;
                }

                bestVisionAlignmentForAlly = visionAlignment;
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

                if (visionAlignment > bestVisionAlignmentForOppo) {
                    continue;
                }

                bestVisionAlignmentForOppo = visionAlignment;
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
            const Vec3 rhsPos = 0 < visionDirection.GetX() ? rhsAABB.mMax : rhsAABB.mMin;
            const Vec3 selfNpcPositionDiff = rhsPos - lhsPos;
            bool isAlongForwardMv = (0 < selfNpcPositionDiff.Dot(visionDirection));
            if (!isAlongForwardMv) {
                // Not a "movement blocker candidate" 
                continue;
            }
            if (!isCharacterFlying) {
                bool strictlyUp = (rhsAABB.mMin.GetY() >= selfNpcWWorldSpaceBounds.mMax.GetY());
                bool holdableForRight = (rhsAABB.mMax.GetX() > selfNpcWWorldSpaceBounds.mMax.GetX());
                bool holdableForLeft = (rhsAABB.mMin.GetX() < selfNpcWWorldSpaceBounds.mMin.GetX());
                bool strictlyUpAndHoldableOnBothSides = (strictlyUp && holdableForLeft && holdableForRight);
                if (strictlyUpAndHoldableOnBothSides) {
                    // Not a "movement blocker candidate" 
                    continue;
                }
            }
            if (visionAlignment > bestVisionAlignmentForMvBlocker) {
                continue;
            }
            bestVisionAlignmentForMvBlocker = visionAlignment;
            outToHandleMvBlockerUd = udRhs;
            outToHandleMvBlockerBodyID = rhsBodyID;
            outVisionPenetrationAgainstMvBlocker = hit.mPenetrationDepth*hit.mPenetrationAxis.Normalized();
            break;
        }
        default:
            break;
        }
    }
}

int BaseNpcReaction::deriveNpcVisionReactionAgainstOppoChUd(int currRdfId, std::unordered_map<uint64_t, const PlayerCharacterDownsync*>& currPlayersMap, std::unordered_map<uint64_t, const NpcCharacterDownsync*>& currNpcsMap, const CH_COLLIDER_T* selfNpcCollider, const BodyID& selfNpcBodyID, const uint64_t selfNpcUd, const CharacterDownsync& currChd, const CharacterConfig* cc, CharacterDownsync* nextChd, bool cvSupported, bool cvInAir, bool cvOnWall, bool currNotDashing, bool currEffInAir, bool oldNextNotDashing, bool oldNextEffInAir, bool inJumpStartupOrJustEnded, CharacterBase::EGroundState cvGroundState, bool canJumpWithinInertia, const Vec3& visionDirection, const uint64_t toHandleOppoChUd, const Vec3& selfNpcPositionDiffForOppoChUd) {
    int ret = TARGET_CH_REACTION_UNCHANGED;
    const TransformedShape& selfNpcTransformedShape = selfNpcCollider->GetTransformedShape(false);
	const AABox& selfNpcWWorldSpaceBounds = selfNpcTransformedShape.GetWorldSpaceBounds();

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
            Vec3 oppoFacing = Quat(rhsCurrChd->q_x(), rhsCurrChd->q_y(), rhsCurrChd->q_z(), rhsCurrChd->q_w())*Vec3::sAxisX();
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

int BaseNpcReaction::deriveReactionAgainstGroundAndMvBlocker(int currRdfId, const BodyInterface* biNoLock, const CH_COLLIDER_T* selfNpcCollider, const BodyID& selfNpcBodyID, const uint64_t selfNpcUd, const NpcGoal inNpcGoal, const CharacterDownsync& currChd, const CharacterConfig* cc, CharacterDownsync* nextChd, bool cvSupported, bool cvInAir, bool cvOnWall, bool currNotDashing, bool currEffInAir, bool oldNextNotDashing, bool oldNextEffInAir, bool inJumpStartupOrJustEnded, CharacterBase::EGroundState cvGroundState, bool canJumpWithinInertia, const AABox& visionAABB, const Vec3Arg& visionNarrowPhaseInBaseOffset, const Vec3& visionDirection, const Vec3& visionPenetrationAgainstRhs, const BodyID& toHandleMvBlockerBodyID, const uint64_t toHandleMvBlockerUd, const int visionReactionByFar) {
    
    if (NpcGoal::NIdle == inNpcGoal) {
        return visionReactionByFar;
    }

    int newVisionReaction = visionReactionByFar;

    const TransformedShape& selfNpcTransformedShape = selfNpcCollider->GetTransformedShape(false);
	const AABox& selfNpcWorldSpaceBounds = selfNpcTransformedShape.GetWorldSpaceBounds();
    bool isCharacterFlying = (currChd.omit_gravity() || cc->omit_gravity());
    
    bool temptingToMove = (temptingToMoveNpcGoalSet.count(inNpcGoal)) && (canJumpWithinInertia || isCharacterFlying);

    /*
    [WARNING] DON'T use "selfNpcCollider->GetLinearVelocity()" to evaluate "potentialMvDx" (and thus "currGroundCanHoldMeIfWalkOn"). 

    When a jumping character touches the vertical-side-edge of a higher platform, its velocity might be calculated by the ContactManager to an opposite direction than the vision direction.  
    */
    float potentialMvSpeed = cc->speed();
    switch (inNpcGoal) {
        case NIdle: {
            potentialMvSpeed = 0;
            break;
        }
        case NIdleIfGoHuntingThenPatrol: {
            if (globalPrimitiveConsts->npc_flee_grace_period_rdf_cnt() >= currChd.frames_in_ch_state()) {
                potentialMvSpeed = 0;
            }
            break;
        }
        default:
            break;
    }
    const Vec3 potentialMvVel = potentialMvSpeed*visionDirection;
     
    float potentialMvDx = potentialMvVel.GetX()*globalPrimitiveConsts->estimated_seconds_per_rdf();

    bool currGroundCanHoldMeIfWalkOn = false, currGapCanHoldMeIfWalkOn = false;
    const AABox* selfNpcGroundAABB = nullptr;

    float effGroundMaxX = FLT_MIN, effGroundMinX = FLT_MAX;
    if (cvSupported) {
        const TransformedShape& selfNpcGroundTransformedShape = biNoLock->GetTransformedShape(selfNpcCollider->GetGroundBodyID());
        selfNpcGroundAABB = &(selfNpcGroundTransformedShape.GetWorldSpaceBounds());
        effGroundMaxX = selfNpcGroundAABB->mMax.GetX(); 
        effGroundMinX = selfNpcGroundAABB->mMin.GetX();

        if (!isCharacterFlying) {
            bool holdableForRight = 0 < visionDirection.GetX() ? (effGroundMaxX > selfNpcWorldSpaceBounds.mMax.GetX()+potentialMvDx ) : true;
            bool holdableForLeft = 0 > visionDirection.GetX() ? (effGroundMinX < selfNpcWorldSpaceBounds.mMin.GetX()+potentialMvDx ) : true;;
            
            currGroundCanHoldMeIfWalkOn = (holdableForLeft && holdableForRight);
        }
    }

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
                oss << "@currRdfId=" << currRdfId << ", selfNpcUd=" << selfNpcUd << " has visionAABB=(minX=" << visionAABB.mMin.GetX() << ", maxX=" << visionAABB.mMax.GetX() << ", minY=" << visionAABB.mMin.GetY() << ", maxY=" << visionAABB.mMax.GetY() << "), curr_ch_state=" << currChd.ch_state() << ", curr_frames_in_ch_state=" << currChd.frames_in_ch_state() << ", cvSupported=" << cvSupported << ", selfNpcGroundBodyID=" << selfNpcCollider->GetGroundBodyID().GetIndexAndSequenceNumber() << ", there's no toHandleMvBlockerBodyID, canJumpWithinInertia=" << canJumpWithinInertia << "; potentialMvVel=(" << potentialMvVel.GetX() << ", " << potentialMvVel.GetY()<< "), hasEffectiveMvBlocker=" << hasEffectiveMvBlocker << ", returning newVisionReaction=" << newVisionReaction;
                Debug::Log(oss.str(), DColor::Orange);
            } 
        }
#endif
*/
        return newVisionReaction;
    }

    bool hasBlockerInXForward = false;

    float mvBlockerColliderLeft = FLT_MAX;
    float mvBlockerColliderRight = -FLT_MAX;
    float mvBlockerColliderBottom = FLT_MAX;
    float mvBlockerColliderTop = -FLT_MAX;

    bool mvBlockerHoldableForRight = false;
    bool mvBlockerHoldableForLeft = false;
    bool mvBlockerStrictlyToTheRight = false;
    bool mvBlockerStrictlyToTheLeft = false;

    bool mvBlockerStrictlyDown = false;
    bool mvBlockerStrictlyUp = false;

    const TransformedShape& mvBlockerTransformedShape = biNoLock->GetTransformedShape(toHandleMvBlockerBodyID);
    const AABox& mvBlockerAABB = mvBlockerTransformedShape.GetWorldSpaceBounds();
   
    mvBlockerHoldableForRight = 0 < visionDirection.GetX() ? (mvBlockerAABB.mMax.GetX() > selfNpcWorldSpaceBounds.mMax.GetX()+potentialMvDx ) : true;
    mvBlockerHoldableForLeft  = 0 > visionDirection.GetX() ? (mvBlockerAABB.mMin.GetX() < selfNpcWorldSpaceBounds.mMin.GetX()+potentialMvDx ) : true;

    float selfNpcCentralXApprox = 0.5f*(selfNpcWorldSpaceBounds.mMax.GetX() + selfNpcWorldSpaceBounds.mMin.GetX());

    mvBlockerStrictlyToTheRight = mvBlockerAABB.mMin.GetX() > selfNpcCentralXApprox + potentialMvDx;
    mvBlockerStrictlyToTheLeft = mvBlockerAABB.mMax.GetX() <= selfNpcCentralXApprox + potentialMvDx;

    mvBlockerStrictlyDown = (mvBlockerAABB.mMax.GetY() <= selfNpcWorldSpaceBounds.mMin.GetY());
    mvBlockerStrictlyUp = (mvBlockerAABB.mMin.GetY() >= selfNpcWorldSpaceBounds.mMax.GetY()); 
    
    hasBlockerInXForward = (0 < visionDirection.GetX() && (mvBlockerStrictlyToTheRight || (mvBlockerHoldableForRight && mvBlockerStrictlyDown))) || (0 > visionDirection.GetX() && (mvBlockerStrictlyToTheLeft || (mvBlockerHoldableForLeft && mvBlockerStrictlyDown)));
    hasBlockerInXForward &= (mvBlockerAABB.mMin.GetY() <= selfNpcWorldSpaceBounds.mMax.GetY() || mvBlockerAABB.mMax.GetY() >= selfNpcWorldSpaceBounds.mMin.GetY());

    bool isMinGapJumpable = false, isCurrGapJumpable = false;
    float currGapToJumpDxAbs = FLT_MAX, currGapToJumpDy = FLT_MAX, currGapEstimatedSpeedX = BaseBattleCollisionFilter::IsLengthNearZero(currChd.vel_x()) ? 0.5f * cc->speed() : 0.6f * std::abs(currChd.vel_x());
    float minGapToJumpDxAbs = FLT_MAX, minGapToJumpDy = FLT_MAX, minGapEstimatedSpeedX = 0.8f * cc->speed();
    float currDxAbsToGroundEdge = FLT_MAX;

    if (hasBlockerInXForward && cvSupported) {
        /*
        [TODO] Handle the following 2 cases.
        - "selfNpcGroundBodyID" being a slope that I can just walk along.
        - "toHandleMvBlockerBodyID" being a slope that I can just walk onto.
        */
        if (!mvBlockerStrictlyDown && !mvBlockerStrictlyUp && hasBlockerInXForward) {
            // Handle "overlapping cutoff"
            effGroundMaxX = 0 < visionDirection.GetX() ? std::min(selfNpcGroundAABB->mMax.GetX(), mvBlockerAABB.mMin.GetX()) : selfNpcGroundAABB->mMax.GetX();
            effGroundMinX = 0 > visionDirection.GetX() ? std::max(selfNpcGroundAABB->mMin.GetX(), mvBlockerAABB.mMax.GetX()) : selfNpcGroundAABB->mMin.GetX();
            bool holdableForRight = 0 < visionDirection.GetX() ? (effGroundMaxX > selfNpcWorldSpaceBounds.mMax.GetX() + potentialMvDx) : true;
            bool holdableForLeft = 0 > visionDirection.GetX() ? (effGroundMinX < selfNpcWorldSpaceBounds.mMin.GetX() + potentialMvDx) : true;;

            currGroundCanHoldMeIfWalkOn = (holdableForLeft && holdableForRight);
        }

        minGapToJumpDy = (mvBlockerAABB.mMax.GetY() - selfNpcGroundAABB->mMax.GetY());
        currGapToJumpDy = (mvBlockerAABB.mMax.GetY() - selfNpcWorldSpaceBounds.mMin.GetY());

        if (0 < visionDirection.GetX()) {
            currGapToJumpDxAbs = (mvBlockerAABB.mMin.GetX() - selfNpcCentralXApprox);
            currDxAbsToGroundEdge = effGroundMaxX - selfNpcWorldSpaceBounds.mMax.GetX();
            minGapToJumpDxAbs = (mvBlockerAABB.mMin.GetX() - effGroundMaxX);
        } else {
            currGapToJumpDxAbs = (selfNpcCentralXApprox - mvBlockerAABB.mMax.GetX());
            currDxAbsToGroundEdge = selfNpcWorldSpaceBounds.mMin.GetX() - effGroundMinX;
            minGapToJumpDxAbs = (effGroundMinX - mvBlockerAABB.mMax.GetX());
        }
        isMinGapJumpable = isGapJumpable(minGapToJumpDxAbs, minGapToJumpDy, minGapEstimatedSpeedX, cc->jumping_init_vel_y());
        isCurrGapJumpable = isGapJumpable(currGapToJumpDxAbs, currGapToJumpDy, currGapEstimatedSpeedX, cc->jumping_init_vel_y());
        currGapCanHoldMeIfWalkOn = isCurrGapJumpable && (0 >= currGapToJumpDxAbs && 0 >= currGapToJumpDy);
    }

    newVisionReaction = deriveReactionAgainstMvBlockerAfterApproximation(currRdfId, selfNpcUd, currChd, cvSupported, canJumpWithinInertia, isMinGapJumpable, isCurrGapJumpable, currGroundCanHoldMeIfWalkOn, currGapCanHoldMeIfWalkOn, currDxAbsToGroundEdge, temptingToMove, newVisionReaction);
/*
#ifndef NDEBUG
    if (selfNpcUd == 8589934593 && TARGET_CH_REACTION_UNCHANGED != newVisionReaction) {
        if (TARGET_CH_REACTION_WALK_ALONG != newVisionReaction || (currRdfId % 16 == 0)) {
            std::ostringstream oss;
            oss << "@currRdfId=" << currRdfId << ", selfNpcUd=" << selfNpcUd << " has visionDir=(" << visionDirection.GetX() << ", " << visionDirection.GetY() << "), visionAABB=(minX=" << visionAABB.mMin.GetX() << ", maxX=" << visionAABB.mMax.GetX() << ", minY=" << visionAABB.mMin.GetY() << ", maxY=" << visionAABB.mMax.GetY() << "), curr_ch_state=" << currChd.ch_state() << ", curr_frames_in_ch_state=" << currChd.frames_in_ch_state() << ", cvSupported=" << cvSupported << ", selfNpcGroundBodyID=" << selfNpcCollider->GetGroundBodyID().GetIndexAndSequenceNumber() << ", toHandleMvBlockerBodyID=" << toHandleMvBlockerBodyID.GetIndexAndSequenceNumber() << ", canJumpWithinInertia=" << canJumpWithinInertia << ", isMinGapJumpable=" << isMinGapJumpable << ", isCurrGapJumpable=" << isCurrGapJumpable << ", currGroundCanHoldMeIfWalkOn=" << currGroundCanHoldMeIfWalkOn << ", currGapCanHoldMeIfWalkOn=" << currGapCanHoldMeIfWalkOn << ", currDxAbsToGroundEdge=" << currDxAbsToGroundEdge << "(effGroundMinX=" << effGroundMinX << ", effGroundMaxX=" << effGroundMaxX << "), temptingToMove=" << temptingToMove << "; minGapToJumpDxAbs=" << minGapToJumpDxAbs << ", minGapToJumpDy=" << minGapToJumpDy << ", currGapToJumpDxAbs=" << currGapToJumpDxAbs << ", currGapToJumpDy=" << currGapToJumpDy << "; returning newVisionReaction=" << newVisionReaction;
            Debug::Log(oss.str(), DColor::Orange);
        } 
    }
#endif
*/
    return newVisionReaction;
}

int BaseNpcReaction::deriveReactionAgainstMvBlockerAfterApproximation(int currRdfId, const uint64_t selfNpcUd, const CharacterDownsync& currChd, const bool cvSupported, const bool canJumpWithinInertia, const bool isMinGapJumpable, const bool isCurrGapJumpable, const bool currGroundCanHoldMeIfWalkOn, const bool currGapCanHoldMeIfWalkOn, const float currDxAbsToGroundEdge, const bool temptingToMove, const int visionReactionByFar) {
    int newVisionReaction = visionReactionByFar;
    if (!cvSupported) {
        if (temptingToMove) {
            newVisionReaction = TARGET_CH_REACTION_WALK_ALONG;
        } else {
            newVisionReaction = TARGET_CH_REACTION_UNCHANGED;
        }
    } else if (canJumpWithinInertia && isCurrGapJumpable) {
        if (temptingToMove) {
            if (currGapCanHoldMeIfWalkOn) {
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
                if (TURNAROUND_FROM_MV_BLOCKER_DX_THRESHOLD < currDxAbsToGroundEdge) {
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

bool BaseNpcReaction::isGapJumpable(const float toJumpDxAbs, const float toJumpDy, const float avgVelXAbs, const float chJumpInitVelY) {
    if (0 >= avgVelXAbs) return false;
    if (0 >= toJumpDxAbs) {
        // Only need evaluate if we can jump vertically first and then slowly move over onto the new platform.
        float airingTimeSingleTrip = chJumpInitVelY / (-globalPrimitiveConsts->gravity_y());
        float estimatedYHighestInTrajectory = 0.5f*chJumpInitVelY*airingTimeSingleTrip;
        return  estimatedYHighestInTrajectory > toJumpDy;
    }
    /*
    When a character jumps at (x=0, y=0) with "avgVelXAbs" and "chJumpInitVelY" (in ISO units), the trajectory is 
    - x(t) = avgVelXAbs*t
    - y(t) = chJumpInitVelY*t + 0.5*g*t where "g < 0"
    */
    float estimatedTSeconds = toJumpDxAbs / avgVelXAbs;
    float estimatedYInTrajectory = chJumpInitVelY * estimatedTSeconds + 0.5f * globalPrimitiveConsts->gravity_y() * estimatedTSeconds * estimatedTSeconds;
    return estimatedYInTrajectory > toJumpDy;
}
