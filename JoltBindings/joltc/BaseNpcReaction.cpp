#include "BaseNpcReaction.h"
#include "CharacterCollideShapeCollector.h"
#include "CollisionLayers.h"
#include "CollisionCallbacks.h"

#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/TaperedCylinderShape.h>
#include <Jolt/Physics/Collision/ShapeCast.h>

#include <climits>

void BaseNpcReaction::postStepDeriveNpcVisionReaction(int currRdfId, std::unordered_map<uint64_t, const PlayerCharacterDownsync*>& currPlayersMap, std::unordered_map<uint64_t, const NpcCharacterDownsync*>& currNpcsMap, std::unordered_map<uint64_t, const Bullet*>& currBulletsMap, const BodyInterface* biNoLock, const NarrowPhaseQuery* narrowPhaseQuery, const BaseBattleCollisionFilter* baseBattleFilter, const DefaultBroadPhaseLayerFilter& bplf, const DefaultObjectLayerFilter& dlf, const CH_COLLIDER_T* selfNpcCollider, const BodyID& selfNpcBodyID, const uint64_t selfNpcUd, const NpcGoal currNpcGoal, const uint64_t currNpcCachedCueCmd, const CharacterDownsync& currChd, const CharacterConfig* cc, CharacterDownsync* nextChd, bool cvSupported, bool cvInAir, bool cvOnWall, bool currNotDashing, bool currEffInAir, bool oldNextNotDashing, bool oldNextEffInAir, bool inJumpStartupOrJustEnded, CharacterBase::EGroundState cvGroundState, NpcGoal& outNextNpcGoal, uint64_t& outCmd) {

    float visionOffsetX = cc->vision_offset_x(), visionOffsetY = cc->vision_offset_y();
    Vec3 initVisionOffset(visionOffsetX, visionOffsetY, 0);    
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

    Vec3 effVisionOffset = cTurn90DegsAroundZAxis*initVisionOffset;    
    auto visionNarrowPhaseInBaseOffset = selfNpcPosition + effVisionOffset;

    Vec3 visionDirection = selfNpcCollider->GetRotation(false)*Vec3::sAxisX();
  
    VisionBodyFilter visionBodyFilter(((const CharacterDownsync*)&currChd), selfNpcCollider->GetBodyID(), selfNpcUd, baseBattleFilter);
    ClosestHitPerBodyCollisionCollector<CastShapeCollector> visionCastResultCollector;
    auto scaling = Vec3::sOne();

    // Settings for collide shape
    ShapeCastSettings settings;
    settings.mActiveEdgeMode = EActiveEdgeMode::CollideOnlyWithActive;
    settings.mActiveEdgeMovementDirection = visionDirection;
    settings.mBackFaceModeConvex = EBackFaceMode::IgnoreBackFaces;
    settings.mBackFaceModeTriangles = EBackFaceMode::IgnoreBackFaces;
    settings.mUseShrunkenShapeAndConvexRadius = true;

    RShapeCast effVisionShapeCast(effVisionShape, scaling, visionCOMTransform, visionDirection);
    narrowPhaseQuery->CastShape(
        effVisionShapeCast,
        settings,
        visionNarrowPhaseInBaseOffset,
        visionCastResultCollector,
        bplf,
        dlf,
        visionBodyFilter,
        { }
    );
    initVisionShape.Release();

    /*
    Now that we've got all entities in vision, will start handling each.
    */
    uint64_t toHandleAllyUd = 0, toHandleOppoChUd = 0, toHandleOppoBlUd = 0, toHandleMvBlockerUd = 0;
    Vec3 selfNpcPositionDiffForAllyUd, selfNpcPositionDiffForOppoChUd, selfNpcPositionDiffForOppoBlUd, selfNpcPositionDiffForMvBlockerUd;

    BodyID toHandleMvBlockerBodyID;
    Vec3 visionOverlapAgainstMvBlocker;

    extractKeyEntitiesInVision(currRdfId, currPlayersMap, currNpcsMap, currBulletsMap, biNoLock, selfNpcCollider, selfNpcBodyID, selfNpcUd, currChd, cc, nextChd, cvSupported, cvInAir, cvOnWall, currNotDashing, currEffInAir, oldNextNotDashing, oldNextEffInAir, inJumpStartupOrJustEnded, cvGroundState, visionDirection, visionCastResultCollector, toHandleAllyUd, selfNpcPositionDiffForAllyUd, toHandleOppoChUd, selfNpcPositionDiffForOppoChUd, toHandleOppoBlUd, selfNpcPositionDiffForOppoBlUd, toHandleMvBlockerUd, selfNpcPositionDiffForMvBlockerUd, toHandleMvBlockerBodyID, visionOverlapAgainstMvBlocker);

    bool notDashing = BaseBattleCollisionFilter::chIsNotDashing(*nextChd);
    bool canJumpWithinInertia = BaseBattleCollisionFilter::chCanJumpWithInertia(currChd, cc, notDashing);

    int newVisionReaction = TARGET_CH_REACTION_UNCHANGED;
    if (0 != toHandleOppoChUd) {
        newVisionReaction = deriveNpcVisionReactionAgainstOppoChUd(currRdfId, currPlayersMap, currNpcsMap, selfNpcCollider, selfNpcBodyID, selfNpcUd, currChd, cc, nextChd, cvSupported, cvInAir, cvOnWall, currNotDashing, currEffInAir, oldNextNotDashing, oldNextEffInAir, inJumpStartupOrJustEnded, cvGroundState, canJumpWithinInertia, visionDirection, toHandleOppoChUd, selfNpcPositionDiffForOppoChUd);

        switch (currNpcGoal) {
        case NpcGoal::NIdle:
            outNextNpcGoal = NpcGoal::NHuntThenIdle;
            break;
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

    if (0 != toHandleMvBlockerUd) {
        const int visionReactionByFar = newVisionReaction; 
        newVisionReaction = deriveReactionAgainstMvBlocker(currRdfId, biNoLock, selfNpcCollider, selfNpcBodyID, selfNpcUd, currNpcGoal, currChd, cc, nextChd, cvSupported, cvInAir, cvOnWall, currNotDashing, currEffInAir, oldNextNotDashing, oldNextEffInAir, inJumpStartupOrJustEnded, cvGroundState, canJumpWithinInertia, visionDirection, visionOverlapAgainstMvBlocker, toHandleMvBlockerBodyID, toHandleMvBlockerUd, selfNpcPositionDiffForMvBlockerUd, visionReactionByFar);
    }

    InputFrameDecoded ifDecodedHolder;
    uint64_t inheritedCachedCueCmd = BaseBattleCollisionFilter::sanitizeCachedCueCmd(currNpcCachedCueCmd);
    BaseBattleCollisionFilter::decodeInput(inheritedCachedCueCmd, &ifDecodedHolder);
    if (TARGET_CH_REACTION_SLIP_JUMP_TOWARDS_CH == newVisionReaction) {
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
        int inheritedDirX = 0;
        if (0 != toHandleOppoChUd) {
            inheritedDirX = 0 < visionDirection.GetX() ? +2 : -2;
        }
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
        if (0 != toHandleOppoChUd) {
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

void BaseNpcReaction::extractKeyEntitiesInVision(int rdfId, std::unordered_map<uint64_t, const PlayerCharacterDownsync*>& currPlayersMap, std::unordered_map<uint64_t, const NpcCharacterDownsync*>& currNpcsMap, std::unordered_map<uint64_t, const Bullet*>& currBulletsMap, const BodyInterface* biNoLock, const CH_COLLIDER_T* selfNpcCollider, const BodyID& selfNpcBodyID, const uint64_t selfNpcUd, const CharacterDownsync& currChd, const CharacterConfig* cc, CharacterDownsync* nextChd, bool cvSupported, bool cvInAir, bool cvOnWall, bool currNotDashing, bool currEffInAir, bool oldNextNotDashing, bool oldNextEffInAir, bool inJumpStartupOrJustEnded, CharacterBase::EGroundState cvGroundState, const Vec3& visionDirection, const ClosestHitPerBodyCollisionCollector<CastShapeCollector>& visionCastResultCollector, uint64_t& outToHandleAllyUd, Vec3& outSelfNpcPositionDiffForAllyUd, uint64_t& outToHandleOppoChUd, Vec3& outSelfNpcPositionDiffForOppoChUd, uint64_t& outToHandleOppoBlUd, Vec3& outSelfNpcPositionDiffForOppoBlUd, uint64_t& outToHandleMvBlockerUd, Vec3& outSelfNpcPositionDiffForMvBlockerUd, BodyID& outToHandleMvBlockerBodyID, Vec3& outVisionOverlapAgainstMvBlocker) {
    if (!visionCastResultCollector.HadHit()) return;
    bool isCharacterFlying = (currChd.omit_gravity() || cc->omit_gravity());

    const TransformedShape& selfNpcTransformedShape = selfNpcCollider->GetTransformedShape(false);
	const AABox& selfNpcWWorldSpaceBounds = selfNpcTransformedShape.GetWorldSpaceBounds();

    float minAbsColliderDxForOppo = FLT_MAX;
    float minAbsColliderDyForOppo = FLT_MAX;

    float minAbsColliderDxForAlly = FLT_MAX;
    float minAbsColliderDyForAlly = FLT_MAX;

    float minAbsColliderDxForMvBlocker = FLT_MAX;
    float minAbsColliderDyForMvBlocker = FLT_MAX;

    const Vec3 lhsPos = selfNpcCollider->GetPosition(false); 
    int hitsCnt = visionCastResultCollector.mHits.size();
    for (int i = 0; i < hitsCnt; i++) {
        const CastShapeCollector::ResultType hit = visionCastResultCollector.mHits.at(i);

        const BodyID rhsBodyID = hit.mBodyID2;
        const Vec3 rhsPos = biNoLock->GetPosition(rhsBodyID);
        const Vec3 selfNpcPositionDiff = rhsPos - lhsPos;
        float absColliderDx = std::abs(selfNpcPositionDiff.GetX());
        float absColliderDy = std::abs(selfNpcPositionDiff.GetY());
        const uint64_t udRhs = biNoLock->GetUserData(rhsBodyID);
        const uint64_t udtRhs = BaseBattleCollisionFilter::getUDT(udRhs);
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
            if (rhsCurrChd->bullet_team_id() != currChd.bullet_team_id()) {
                if (absColliderDx > minAbsColliderDxForOppo) {
                    continue;
                }

                if (absColliderDx == minAbsColliderDxForOppo && absColliderDy > minAbsColliderDyForOppo) {
                    continue;
                }
                minAbsColliderDxForOppo = absColliderDx;
                minAbsColliderDyForOppo = absColliderDy;
                 
                outToHandleOppoChUd = udRhs;
                outToHandleOppoBlUd = 0;
                outSelfNpcPositionDiffForOppoChUd = selfNpcPositionDiff;
                outSelfNpcPositionDiffForOppoBlUd = Vec3::sZero();
            } else {
                if (absColliderDx > minAbsColliderDxForAlly) {
                    continue;
                }

                if (absColliderDx == minAbsColliderDxForAlly && absColliderDy > minAbsColliderDyForAlly) {
                    continue;
                }
                minAbsColliderDxForAlly = absColliderDx;
                minAbsColliderDyForAlly = absColliderDy;
                outToHandleAllyUd = udRhs;
                outSelfNpcPositionDiffForAllyUd = selfNpcPositionDiff;
            }
            break;
        }
        case UDT_BL: {
            const Bullet* rhsCurrBl = currBulletsMap.at(udRhs);
            if (rhsCurrBl->team_id() != currChd.bullet_team_id()) {
                Vec3 rhsCurrBlFacing = Quat(rhsCurrBl->q_x(), rhsCurrBl->q_y(), rhsCurrBl->q_z(), rhsCurrBl->q_w())*Vec3::sAxisX();
                if (0 <= selfNpcPositionDiff.Dot(rhsCurrBlFacing)) {
                    continue; // seemingly not offensive
                }

                if (absColliderDx > minAbsColliderDxForOppo) {
                    continue;
                }

                if (absColliderDx == minAbsColliderDxForOppo && absColliderDy > minAbsColliderDyForOppo) {
                    continue;
                }
                minAbsColliderDxForOppo = absColliderDx;
                minAbsColliderDyForOppo = absColliderDy;
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
            bool isAlongForwardMv = (0 < selfNpcPositionDiff.Dot(visionDirection));
            if (!isAlongForwardMv) {
                // Not a "movement blocker candidate" 
                continue;
            }
            const TransformedShape& rhsTransformedShape = biNoLock->GetTransformedShape(rhsBodyID);
            const AABox& rhsAABB = rhsTransformedShape.GetWorldSpaceBounds();
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
            if (absColliderDx > minAbsColliderDxForMvBlocker) {
                continue;
            }

            if (absColliderDx == minAbsColliderDxForMvBlocker && absColliderDy > minAbsColliderDyForMvBlocker) {
                continue;
            }
            minAbsColliderDxForMvBlocker = absColliderDx;
            minAbsColliderDyForMvBlocker = absColliderDy;
            outToHandleMvBlockerUd = udRhs;
            outToHandleMvBlockerBodyID = rhsBodyID;
            outVisionOverlapAgainstMvBlocker = hit.mContactPointOn1;
            outSelfNpcPositionDiffForMvBlockerUd = selfNpcPositionDiff;
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

int BaseNpcReaction::deriveReactionAgainstMvBlocker(int currRdfId, const BodyInterface* biNoLock, const CH_COLLIDER_T* selfNpcCollider, const BodyID& selfNpcBodyID, const uint64_t selfNpcUd, const NpcGoal currNpcGoal, const CharacterDownsync& currChd, const CharacterConfig* cc, CharacterDownsync* nextChd, bool cvSupported, bool cvInAir, bool cvOnWall, bool currNotDashing, bool currEffInAir, bool oldNextNotDashing, bool oldNextEffInAir, bool inJumpStartupOrJustEnded, CharacterBase::EGroundState cvGroundState, bool canJumpWithinInertia, const Vec3& visionDirection, const Vec3& visionOverlapAgainstRhs, const BodyID& toHandleMvBlockerBodyID, const uint64_t toHandleMvBlockerUd, const Vec3& selfNpcPositionDiffForMvBlockerUd, const int visionReactionByFar) {
    int newVisionReaction = visionReactionByFar;
    const TransformedShape& selfNpcTransformedShape = selfNpcCollider->GetTransformedShape(false);
	const AABox& selfNpcWWorldSpaceBounds = selfNpcTransformedShape.GetWorldSpaceBounds();
    bool isCharacterFlying = (currChd.omit_gravity() || cc->omit_gravity());
    int effDxByFar = 0 < visionDirection.GetX() ? +2 : -2;
    int effDyByFar = 0;
    bool temptingToMove = (NpcGoal::NPatrol == currNpcGoal || NpcGoal::NHuntThenIdle == currNpcGoal || NpcGoal::NHuntThenPatrol == currNpcGoal || NpcGoal::NHuntThenFollowAlly == currNpcGoal) && (canJumpWithinInertia || isCharacterFlying);

    const BodyID selfNpcGroundBodyID = selfNpcCollider->GetGroundBodyID();

    bool canTurnaroundOrStopOrStartWalking = false;
    switch (visionReactionByFar) {
        case TARGET_CH_REACTION_UNCHANGED:
            canTurnaroundOrStopOrStartWalking = (!isCharacterFlying && !currEffInAir && !selfNpcGroundBodyID.IsInvalid());
            break;
        default:
            break;
    }

    Vec3 chVel = selfNpcCollider->GetLinearVelocity(false); 
    float effVelX = chVel.GetX(), effVelY = chVel.GetY();
    switch (currNpcGoal) {
    case NpcGoal::NIdle:
        effVelX = 0;
        effVelY = 0;
        break;
    case NpcGoal::NIdleIfGoHuntingThenPatrol:
        if (globalPrimitiveConsts->npc_flee_grace_period_rdf_cnt() >= currChd.frames_in_ch_state()) {
            /*
                [WARNING] "NIdleIfGoHuntingThenPatrol" is still tempting to patrol, so just give it a grace period before switching reaction to "TARGET_CH_REACTION_WALK_ALONG". Moreover, even given a temptation to resume patroling (i.e. 0 != effVelX), it'll still require passing "currBlockCanStillHoldMe" check to actually allow the transition, e.g. a Boar standing on the edge of a short hovering platform with "NidleIfGoHuntingThenPatrol" is unlikely to transit, but a SwordMan accidentally stopped at the beginning of a long platform is likely to transit.  
            */
            effVelX = 0;
            effVelY = 0;
        }
        break;
    default:
        break;
    }
     
    float potentialMvDx = effVelX*globalPrimitiveConsts->estimated_seconds_per_rdf();

    bool currBlockCanStillHoldMeIfWalkOn = false;
    if (!isCharacterFlying && !selfNpcGroundBodyID.IsInvalid()) {
        const TransformedShape& selfNpcGroundTransformedShape = biNoLock->GetTransformedShape(selfNpcGroundBodyID);
        const AABox& selfNpcGroundAABB = selfNpcGroundTransformedShape.GetWorldSpaceBounds();

        bool holdableForRight = 0 < effVelX ? (selfNpcGroundAABB.mMax.GetX() > selfNpcWWorldSpaceBounds.mMax.GetX()+potentialMvDx ) : true;
        bool holdableForLeft = 0 > effVelX ? (selfNpcGroundAABB.mMin.GetX() < selfNpcWWorldSpaceBounds.mMin.GetX()+potentialMvDx ) : true;;
        currBlockCanStillHoldMeIfWalkOn = (holdableForLeft && holdableForRight);
    } 

    if (!canJumpWithinInertia || toHandleMvBlockerBodyID.IsInvalid()) {
        if (currBlockCanStillHoldMeIfWalkOn) {
            return visionReactionByFar;
        } else {
            if (temptingToMove) {       
                return TARGET_CH_REACTION_TURNAROUND_MV_BLOCKER;
            } else {
                return TARGET_CH_REACTION_STOP_BY_MV_BLOCKER;
            }
        }
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

    mvBlockerHoldableForRight = 0 < effVelX ? (mvBlockerAABB.mMax.GetX() > selfNpcWWorldSpaceBounds.mMax.GetX()+potentialMvDx ) : true;
    mvBlockerHoldableForLeft  = 0 > effVelX ? (mvBlockerAABB.mMin.GetX() < selfNpcWWorldSpaceBounds.mMin.GetX()+potentialMvDx ) : true;;

    mvBlockerStrictlyToTheRight = mvBlockerAABB.mMin.GetX() > selfNpcWWorldSpaceBounds.mMax.GetX()+potentialMvDx;
    mvBlockerStrictlyToTheLeft = mvBlockerAABB.mMax.GetX() <= selfNpcWWorldSpaceBounds.mMin.GetX()+potentialMvDx;

    mvBlockerStrictlyDown = (mvBlockerAABB.mMax.GetY() <= selfNpcWWorldSpaceBounds.mMin.GetY());
    mvBlockerStrictlyUp = (mvBlockerAABB.mMin.GetY() >= selfNpcWWorldSpaceBounds.mMax.GetY()); 

    hasBlockerInXForward = (0 < effVelX && (mvBlockerStrictlyToTheRight || (mvBlockerHoldableForRight && mvBlockerStrictlyDown))) || (0 > effVelX && (mvBlockerStrictlyToTheLeft || (mvBlockerHoldableForLeft && mvBlockerStrictlyDown)));
    hasBlockerInXForward &= (toHandleMvBlockerBodyID != selfNpcGroundBodyID) && (mvBlockerAABB.mMin.GetY() <= selfNpcWWorldSpaceBounds.mMax.GetY() || mvBlockerAABB.mMax.GetY() >= selfNpcWWorldSpaceBounds.mMin.GetY());

    if (hasBlockerInXForward) {
        if (0 < effVelX * visionOverlapAgainstRhs.GetX() && 0 > visionOverlapAgainstRhs.GetY()) {
            // Potentially a slope 
            bool notSteep = (0 < visionOverlapAgainstRhs.GetX() && visionOverlapAgainstRhs.GetX() <= -0.8f * visionOverlapAgainstRhs.GetY())
                            ||
                            (0 > visionOverlapAgainstRhs.GetX() && (-visionOverlapAgainstRhs.GetX()) <= -0.8f * visionOverlapAgainstRhs.GetY());
            if (notSteep) {
                return TARGET_CH_REACTION_WALK_ALONG;
            }
        }
        float diffCxApprox = 0.0f;
        if (0 < effDxByFar) {
            diffCxApprox = (mvBlockerAABB.mMin.GetX() - selfNpcWWorldSpaceBounds.mMax.GetX());
        } else if (0 > effDxByFar) {
            diffCxApprox = (selfNpcWWorldSpaceBounds.mMin.GetX() - mvBlockerAABB.mMax.GetX());
        }

        if (mvBlockerAABB.mMax.GetY() > selfNpcWWorldSpaceBounds.mMin.GetY()) {
            float diffCyApprox = (mvBlockerAABB.mMax.GetY() - selfNpcWWorldSpaceBounds.mMax.GetY());
            float jumpableDiffCyApprox = (cc->jumping_init_vel_y() * cc->jumping_init_vel_y()) / (-globalPrimitiveConsts->gravity_y());
            float jumpableDiffCxApprox = 0 <= diffCyApprox ? (cc->speed() * cc->jumping_init_vel_y()) / (-globalPrimitiveConsts->gravity_y()) : (cc->speed() * (cc->jumping_init_vel_y() + (cc->jumping_init_vel_y() * 0.5f))) / (-globalPrimitiveConsts->gravity_y());
            
            if (mvBlockerStrictlyUp) {
                if (diffCxApprox < TURNAROUND_FROM_MV_BLOCKER_DX_THRESHOLD) {
                    jumpableDiffCxApprox = 0;
                    jumpableDiffCyApprox = 0;
                } else {
                    jumpableDiffCxApprox *= 0.5f;
                    jumpableDiffCyApprox *= 0.8f;
                }
            }
            bool heightDiffJumpableApprox = (jumpableDiffCyApprox >= diffCyApprox);
            bool widthDiffJumpableApprox = (jumpableDiffCxApprox >= diffCxApprox);
            if (heightDiffJumpableApprox && widthDiffJumpableApprox) {
                if (canTurnaroundOrStopOrStartWalking) {
                    newVisionReaction = TARGET_CH_REACTION_JUMP_TOWARDS_MV_BLOCKER;
                } else {
                    newVisionReaction = TARGET_CH_REACTION_JUMP_TOWARDS_CH;
                }
            } else if (canTurnaroundOrStopOrStartWalking) {
                if (0 < diffCyApprox && TURNAROUND_FROM_MV_BLOCKER_DX_THRESHOLD > diffCxApprox) {
                    newVisionReaction = TARGET_CH_REACTION_TURNAROUND_MV_BLOCKER;
                } else if (currBlockCanStillHoldMeIfWalkOn) {
                    newVisionReaction = TARGET_CH_REACTION_WALK_ALONG;
                }
            }
        } else if (canTurnaroundOrStopOrStartWalking && currBlockCanStillHoldMeIfWalkOn) {
            // [WARNING] Implies "0 != effVelX"
            newVisionReaction = TARGET_CH_REACTION_WALK_ALONG;
        } else {
            // Otherwise "mvBlockerCollider" sits lower than "currChd" and "false == currBlockCanStillHoldMeIfWalkOn", a next but lower platform in the direction of movement, might still need jumping but the estimated dx movable by jumping is longer in this case
            float diffCyApprox = (selfNpcWWorldSpaceBounds.mMax.GetY() - mvBlockerAABB.mMax.GetY());
            int jumpableDiffCxBase = (cc->speed() * (cc->jumping_init_vel_y() * 2.0f)) / (-globalPrimitiveConsts->gravity_y());
            int jumpableDiffCxApprox = jumpableDiffCxBase + (jumpableDiffCxBase * 0.25f);
            bool widthDiffJumpableApprox = (jumpableDiffCxApprox >= diffCxApprox);
            if (widthDiffJumpableApprox) {
                newVisionReaction = (canTurnaroundOrStopOrStartWalking ? TARGET_CH_REACTION_JUMP_TOWARDS_MV_BLOCKER : TARGET_CH_REACTION_JUMP_TOWARDS_CH); // "mvBlocker" is found in the direction of hunting even if "false == canTurnaroundOrStopOrStartWalking", can jump anyway
            } else if (canTurnaroundOrStopOrStartWalking) {                            
                newVisionReaction = TARGET_CH_REACTION_STOP_BY_MV_BLOCKER;
            }
        }
    } else if (canTurnaroundOrStopOrStartWalking) {
        // "false == hasBlockerInXForward", in this case blindly moving forward might fall into death
        if (currBlockCanStillHoldMeIfWalkOn) {
            newVisionReaction = TARGET_CH_REACTION_WALK_ALONG;
        } else {
            newVisionReaction = TARGET_CH_REACTION_STOP_BY_MV_BLOCKER;
        }
    }

    return newVisionReaction; 
}
