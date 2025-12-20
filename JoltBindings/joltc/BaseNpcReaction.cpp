#include "BaseNpcReaction.h"
#include "CharacterCollideShapeCollector.h"
#include "CollisionLayers.h"
#include "CollisionCallbacks.h"

#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/TaperedCylinderShape.h>
#include <Jolt/Physics/Collision/ShapeCast.h>

void BaseNpcReaction::postStepDeriveNpcVisionReaction(int currRdfId, std::unordered_map<uint64_t, const PlayerCharacterDownsync*>& currPlayersMap, std::unordered_map<uint64_t, const NpcCharacterDownsync*>& currNpcsMap, std::unordered_map<uint64_t, const Bullet*>& currBulletsMap, const BodyInterface* biNoLock, const NarrowPhaseQuery* narrowPhaseQuery, const BaseBattleCollisionFilter* baseBattleFilter, const DefaultBroadPhaseLayerFilter& bplf, const DefaultObjectLayerFilter& dlf, const CH_COLLIDER_T* selfNpcCollider, const BodyID& selfNpcBodyID, const uint64_t selfNpcUd, const NpcCharacterDownsync& currNpc, NpcCharacterDownsync* nextNpc, const CharacterDownsync& currChd, const CharacterConfig* cc, CharacterDownsync* nextChd, bool cvSupported, bool cvInAir, bool cvOnWall, bool currNotDashing, bool currEffInAir, bool oldNextNotDashing, bool oldNextEffInAir, bool inJumpStartupOrJustEnded, CharacterBase::EGroundState cvGroundState) {

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
    Vec3 selfNpcDiffForAllyUd, selfNpcDiffForOppoChUd, selfNpcDiffForOppoBlUd, selfNpcDiffForMvBlockerUd;
    extractKeyEntitiesInVision(currRdfId, currPlayersMap, currNpcsMap, currBulletsMap, biNoLock, selfNpcCollider, selfNpcBodyID, selfNpcUd, currNpc, nextNpc, currChd, cc, nextChd, cvSupported, cvInAir, cvOnWall, currNotDashing, currEffInAir, oldNextNotDashing, oldNextEffInAir, inJumpStartupOrJustEnded, cvGroundState, visionDirection, visionCastResultCollector, toHandleAllyUd, selfNpcDiffForAllyUd, toHandleOppoChUd, selfNpcDiffForOppoChUd, toHandleOppoBlUd, selfNpcDiffForOppoBlUd, toHandleMvBlockerUd, selfNpcDiffForMvBlockerUd);

    int newVisionReaction = TARGET_CH_REACTION_UNCHANGED;
    if (0 != toHandleOppoChUd) {
        newVisionReaction = deriveNpcVisionReactionAgainstOppoChUd(currRdfId, currPlayersMap, currNpcsMap, selfNpcCollider, selfNpcBodyID, selfNpcUd, currNpc, nextNpc, currChd, cc, nextChd, cvSupported, cvInAir, cvOnWall, currNotDashing, currEffInAir, oldNextNotDashing, oldNextEffInAir, inJumpStartupOrJustEnded, cvGroundState, visionDirection, toHandleOppoChUd, selfNpcDiffForOppoChUd);
    } else {
        newVisionReaction = TARGET_CH_REACTION_HUNTING_LOSS;
        if (0 != toHandleMvBlockerUd) {
            // TODO
        }
    }

    InputFrameDecoded ifDecodedHolder;
    uint64_t inheritedCachedCueCmd = BaseBattleCollisionFilter::sanitizeCachedCueCmd(currNpc.cached_cue_cmd());
    BaseBattleCollisionFilter::decodeInput(inheritedCachedCueCmd, &ifDecodedHolder);
    if (TARGET_CH_REACTION_HUNTING_LOSS == newVisionReaction) {
        switch (currNpc.goal_as_npc()) {
        case NpcGoal::NHuntThenIdle:
            nextNpc->set_goal_as_npc(NpcGoal::NIdle);
            break;
        case NpcGoal::NHuntThenPatrol:
            nextNpc->set_goal_as_npc(NpcGoal::NPatrol);
            break;
        case NpcGoal::NHuntThenFollowAlly:
            nextNpc->set_goal_as_npc(NpcGoal::NFollowAlly);
            break;
        default:
            break;
        }
    } else {
        int toMoveDirX = 0;
        if (0 != toHandleOppoChUd) {
            toMoveDirX = 0 < selfNpcDiffForOppoChUd.GetX() ? +2 : -2;
        } else if (0 != toHandleAllyUd) {
            toMoveDirX = 0 < selfNpcDiffForAllyUd.GetX() ? +2 : -2;
        }
        if (TARGET_CH_REACTION_SLIP_JUMP_TOWARDS_CH == newVisionReaction) {
            ifDecodedHolder.set_dx(toMoveDirX);
            ifDecodedHolder.set_dy(-2);
            ifDecodedHolder.set_btn_a_level(1);
        } else if (TARGET_CH_REACTION_JUMP_TOWARDS_CH == newVisionReaction) {
            ifDecodedHolder.set_dx(toMoveDirX);
            ifDecodedHolder.set_dy(0);
            ifDecodedHolder.set_btn_a_level(1);
        } else {
            // It's important to unset "BtnALevel" if no proactive jump is implied by vision reaction, otherwise its value will remain even after execution and sanitization
            ifDecodedHolder.set_dx(toMoveDirX);
            ifDecodedHolder.set_dy(0);
            ifDecodedHolder.set_btn_a_level(0);
        }

        switch (currNpc.goal_as_npc()) {
        case NpcGoal::NIdle:
            nextNpc->set_goal_as_npc(NpcGoal::NHuntThenIdle);
            break;
        case NpcGoal::NPatrol:
            nextNpc->set_goal_as_npc(NpcGoal::NHuntThenPatrol);
            break;
        case NpcGoal::NFollowAlly:
            nextNpc->set_goal_as_npc(NpcGoal::NHuntThenFollowAlly);
            break;
        default:
            break;
        }
    }

    uint64_t newCachedCueCmd = BaseBattleCollisionFilter::encodeInput(ifDecodedHolder);
    nextNpc->set_cached_cue_cmd(newCachedCueCmd);
}

void BaseNpcReaction::extractKeyEntitiesInVision(int rdfId, std::unordered_map<uint64_t, const PlayerCharacterDownsync*>& currPlayersMap, std::unordered_map<uint64_t, const NpcCharacterDownsync*>& currNpcsMap, std::unordered_map<uint64_t, const Bullet*>& currBulletsMap, const BodyInterface* biNoLock, const CH_COLLIDER_T* selfNpcCollider, const BodyID& selfNpcBodyID, const uint64_t selfNpcUd, const NpcCharacterDownsync& currNpc, NpcCharacterDownsync* nextNpc, const CharacterDownsync& currChd, const CharacterConfig* cc, CharacterDownsync* nextChd, bool cvSupported, bool cvInAir, bool cvOnWall, bool currNotDashing, bool currEffInAir, bool oldNextNotDashing, bool oldNextEffInAir, bool inJumpStartupOrJustEnded, CharacterBase::EGroundState cvGroundState, const Vec3& visionDirection, const ClosestHitPerBodyCollisionCollector<CastShapeCollector>& visionCastResultCollector, uint64_t& outToHandleAllyUd, Vec3& outSelfNpcDiffForAllyUd, uint64_t& outToHandleOppoChUd, Vec3& outSelfNpcDiffForOppoChUd, uint64_t& outToHandleOppoBlUd, Vec3& outSelfNpcDiffForOppoBlUd, uint64_t& outToHandleMvBlockerUd, Vec3& outSelfNpcDiffForMvBlockerUd) {
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
        const Vec3 selfNpcDiff = rhsPos - lhsPos;
        float absColliderDx = std::abs(selfNpcDiff.GetX());
        float absColliderDy = std::abs(selfNpcDiff.GetY());
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
                outSelfNpcDiffForOppoChUd = selfNpcDiff;
                outSelfNpcDiffForOppoBlUd = Vec3::sZero();
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
                outSelfNpcDiffForAllyUd = selfNpcDiff;
            }
            break;
        }
        case UDT_BL: {
            const Bullet* rhsCurrBl = currBulletsMap.at(udRhs);
            if (rhsCurrBl->team_id() != currChd.bullet_team_id()) {
                Vec3 rhsCurrBlFacing = Quat(rhsCurrBl->q_x(), rhsCurrBl->q_y(), rhsCurrBl->q_z(), rhsCurrBl->q_w())*Vec3::sAxisX();
                if (0 <= selfNpcDiff.Dot(rhsCurrBlFacing)) {
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
                outSelfNpcDiffForOppoChUd = Vec3::sZero();
                outSelfNpcDiffForOppoBlUd = selfNpcDiff;
            } else {
                // [TODO] Handling of "for ally bullets" 
            }
            break;
        }
        case UDT_OBSTACLE: {
            bool isAlongForwardMv = (0 < selfNpcDiff.Dot(visionDirection));
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
            outSelfNpcDiffForMvBlockerUd = selfNpcDiff;
            break;
        }
        default:
            break;
        }
    }
}

int BaseNpcReaction::deriveNpcVisionReactionAgainstOppoChUd(int rdfId, std::unordered_map<uint64_t, const PlayerCharacterDownsync*>& currPlayersMap, std::unordered_map<uint64_t, const NpcCharacterDownsync*>& currNpcsMap, const CH_COLLIDER_T* selfNpcCollider, const BodyID& selfNpcBodyID, const uint64_t selfNpcUd, const NpcCharacterDownsync& currNpc, NpcCharacterDownsync* nextNpc, const CharacterDownsync& currChd, const CharacterConfig* cc, CharacterDownsync* nextChd, bool cvSupported, bool cvInAir, bool cvOnWall, bool currNotDashing, bool currEffInAir, bool oldNextNotDashing, bool oldNextEffInAir, bool inJumpStartupOrJustEnded, CharacterBase::EGroundState cvGroundState, const Vec3& visionDirection, const uint64_t toHandleOppoUd, const Vec3& selfNpcDiffForOppoUd) {
    int ret = TARGET_CH_REACTION_UNCHANGED;
    const TransformedShape& selfNpcTransformedShape = selfNpcCollider->GetTransformedShape(false);
	const AABox& selfNpcWWorldSpaceBounds = selfNpcTransformedShape.GetWorldSpaceBounds();

    bool opponentBehindMe = (0 > (selfNpcDiffForOppoUd.GetX() * visionDirection.GetX()));
    bool opponentAboveMe = cc->capsule_half_height() < selfNpcDiffForOppoUd.GetY();
    const CharacterDownsync* rhsCurrChd = nullptr;
    const uint64_t udtRhs = BaseBattleCollisionFilter::getUDT(toHandleOppoUd);
    if (UDT_PLAYER == udtRhs) {
        auto rhsCurrPlayer = currPlayersMap.at(toHandleOppoUd);
        rhsCurrChd = &(rhsCurrPlayer->chd());
    } else {
        auto rhsCurrNpc = currNpcsMap.at(toHandleOppoUd);
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
            bool oppoIsFacingMe = (0 > selfNpcDiffForOppoUd.GetX() * oppoFacing.GetX());
            if ((opponentIsAttacking && oppoIsFacingMe)) {
                ret = TARGET_CH_REACTION_FOLLOW;
            } else {
                ret = TARGET_CH_REACTION_FLEE_OPPO;
            }
        }
    }

    return ret;
}
