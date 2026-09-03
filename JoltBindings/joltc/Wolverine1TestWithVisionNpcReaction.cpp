#include "Wolverine1TestWithVisionNpcReaction.h"

int Wolverine1TestWithVisionNpcReaction::deriveReactionAgainstGroundAndMvBlocker(int currRdfId, const Vec3& antiGravityNorm, const float gravityMagnitude, const BodyInterface* biNoLock, const CH_COLLIDER_T* selfNpcCollider, const AABox* selfNpcAABB, const BodyID& selfNpcBodyID, const uint64_t selfNpcUd, const NpcGoal inNpcGoal, const CharacterDownsync& currChd, const MassProperties& massProps, const Vec3& currChdFacing, const CharacterConfig* cc, CharacterDownsync* nextChd, const bool cvSupported, const bool cvInAir, const bool cvOnWall, const bool currNotDashing, const bool currEffInAir, const bool currIsFlying, const bool oldNextNotDashing, const bool oldNextEffInAir, const bool inJumpStartupOrJustEnded, CharacterBase::EGroundState cvGroundState, const bool canJumpWithinInertia, const AABox& visionAABB, const Vec3Arg& visionNarrowPhaseInBaseOffset, const Vec3& visionDirection, const BodyID& toHandleMvBlockerBodyID, const uint64_t toHandleMvBlockerUd, const GapToJump& currGapToJump, const GapToJump& minGapToJump, const GapToJump& currGroundMvTolerance, const int visionReactionByFar, const uint64_t toHandleOppoChUd, const Vec3& selfNpcPositionDiffForOppoChUd, const bool opponentBehindMe, const bool opponentAboveMe, const bool opponentIsAttacking, const bool opponentIsFacingMe, const int lastFledRdfId) {
    
    if (NpcGoal::NIdle == inNpcGoal || NpcGoal::NIdleIfGoHuntingThenPatrol == inNpcGoal || NpcGoal::NIdleIfGoHuntingThenPathPatrol == inNpcGoal) {
        return visionReactionByFar;
    }

    int newVisionReaction = visionReactionByFar;
    bool temptingToMove = (temptingToMoveNpcGoalSet.count(inNpcGoal)) && (canJumpWithinInertia || currIsFlying);

    /*
    [WARNING] DON'T use "selfNpcCollider->GetLinearVelocity()" to evaluate "currGroundCanHoldMeIfWalkOn". 

    When a jumping character touches the vertical-side-edge of a higher platform, its velocity might be calculated by the ContactManager to an opposite direction than the vision direction.  
    */
    const float potentialMv = cc->speed()*globalPrimitiveConsts->estimated_seconds_per_rdf();

    bool currGroundCanHoldMeIfWalkOn = (0 < currGroundMvTolerance.vision_alignment());
    bool toHandleMvBlockerCanHoldMeIfWalkOn = false;
    const Vec3& chColliderVel = selfNpcCollider->GetLinearVelocity(false);
    const float constraintVelXDiff = chColliderVel.GetX() - nextChd->vel_x();
    const float constraintVelYDiff = chColliderVel.GetY() - nextChd->vel_y();
    bool hasEffectiveMvBlocker = false;

    bool inFleeingGracePeriod = (currRdfId < lastFledRdfId + globalPrimitiveConsts->default_fleeing_grace_period_rdf_cnt());

    hasEffectiveMvBlocker = (walkingSet.count(currChd.ch_state()) || temptingToMove) &&
        (
        (0 > constraintVelXDiff * nextChd->vel_x()) && !BaseBattleCollisionFilter::IsLengthNearZero(constraintVelXDiff * globalPrimitiveConsts->estimated_seconds_per_rdf())
        ||
        (0 > currGapToJump.vision_alignment())
    );

    if (cvSupported) {
        if (!currGroundCanHoldMeIfWalkOn || hasEffectiveMvBlocker) {
            /*
            [WARNING] Don't IMMEDIATELY return the "newVisionReaction" if "0 != toHandleMvBlockerUd", there might be still chance to jump onto a horizontally forward holding platform.
            */
            if (temptingToMove && !inFleeingGracePeriod) {
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
        return newVisionReaction;
    }

    bool isMinGapJumpable = false, isCurrGapJumpable = false;
    float currGapEstimatedSpeedX = BaseBattleCollisionFilter::IsLengthNearZero(currChd.vel_x()) ? 0.7f * cc->speed() : 0.8f * std::abs(currChd.vel_x());
    float minGapEstimatedSpeedX = 0.8f * cc->speed();
    float currGapToJumpVisionAlignment = currGapToJump.vision_alignment(), currGapToJumpAntiGravityAlignment = currGapToJump.anti_gravity_alignment();

    if (FLT_MAX != currGapToJumpVisionAlignment && cvSupported) {
        const float jumpAccMagY = cc->jump_acc_mag_y();
        const int jumpStartupFrames = cc->jump_startup_frames();
        const float chJumpAccSeconds = ((jumpStartupFrames + 1) * globalPrimitiveConsts->estimated_seconds_per_rdf());
        const float chJumpInitSpeed = jumpAccMagY * chJumpAccSeconds;
        const float extraAccendingY = ((chJumpInitSpeed * 0.5f) * chJumpAccSeconds);
        isMinGapJumpable = isGapJumpable(gravityMagnitude, minGapToJump.vision_alignment() + cc->capsule_radius(), minGapToJump.anti_gravity_alignment(), minGapEstimatedSpeedX, chJumpAccSeconds, chJumpInitSpeed, extraAccendingY);
        isCurrGapJumpable = isGapJumpable(gravityMagnitude, currGapToJumpVisionAlignment + cc->capsule_radius(), currGapToJumpAntiGravityAlignment, currGapEstimatedSpeedX, chJumpAccSeconds, chJumpInitSpeed, extraAccendingY);
        toHandleMvBlockerCanHoldMeIfWalkOn = isCurrGapJumpable && (0 >= currGapToJumpVisionAlignment && 0 >= currGapToJumpAntiGravityAlignment);
    }

    newVisionReaction = deriveReactionAgainstMvBlockerAfterApproximation(currRdfId, antiGravityNorm, selfNpcUd, currChd, massProps, currChdFacing, cvSupported, canJumpWithinInertia, isMinGapJumpable, isCurrGapJumpable, currGroundCanHoldMeIfWalkOn, toHandleMvBlockerCanHoldMeIfWalkOn, currGapToJumpVisionAlignment, temptingToMove, inFleeingGracePeriod, newVisionReaction);

    return newVisionReaction;
}

int Wolverine1TestWithVisionNpcReaction::deriveNpcVisionReactionAgainstOppoChUd(int rdfId, std::unordered_map<uint64_t, const PlayerCharacterDownsync*>& currPlayersMap, std::unordered_map<uint64_t, const NpcCharacterDownsync*>& currNpcsMap, const CH_COLLIDER_T* selfNpcCollider, const BodyID& selfNpcBodyID, const uint64_t selfNpcUd, const CharacterDownsync& currChd, const MassProperties& massProps, const Vec3& currChdFacing, const CharacterConfig* cc, CharacterDownsync* nextChd, const bool cvSupported, const bool cvInAir, const bool cvOnWall, const bool currNotDashing, const bool currEffInAir, const bool currIsFlying, const bool oldNextNotDashing, const bool oldNextEffInAir, const bool inJumpStartupOrJustEnded, CharacterBase::EGroundState cvGroundState, const bool canJumpWithinInertia, const Vec3& visionDirection, const uint64_t toHandleOppoChUd, const Vec3& selfNpcPositionDiffForOppoChUd, bool& outOpponentBehindMe, bool& outOpponentAboveMe, bool& outOpponentIsAttacking, bool& outOpponentIsFacingMe) {

    int newVisionReaction = BaseNpcReaction::deriveNpcVisionReactionAgainstOppoChUd(rdfId, currPlayersMap, currNpcsMap, selfNpcCollider, selfNpcBodyID, selfNpcUd, currChd, massProps, currChdFacing, cc, nextChd, cvSupported, cvInAir, cvOnWall, currNotDashing, currEffInAir, currIsFlying, oldNextNotDashing, oldNextEffInAir, inJumpStartupOrJustEnded, cvGroundState, canJumpWithinInertia, visionDirection, toHandleOppoChUd, selfNpcPositionDiffForOppoChUd, outOpponentBehindMe, outOpponentAboveMe, outOpponentIsAttacking, outOpponentIsFacingMe);

    if (0 >= currChd.frames_to_recover()) {
        // Check melee reachable or not.
        if (!outOpponentBehindMe && !outOpponentAboveMe) {
            float candAbsX = std::abs(selfNpcPositionDiffForOppoChUd.GetX());
            const CharacterDownsync* rhsCurrChd = nullptr;
            const uint64_t udtRhs = BaseBattleCollisionFilter::getUDT(toHandleOppoChUd);
            if (UDT_PLAYER == udtRhs) {
                auto rhsCurrPlayer = currPlayersMap.at(toHandleOppoChUd);
                rhsCurrChd = &(rhsCurrPlayer->chd());
            } else {
                auto rhsCurrNpc = currNpcsMap.at(toHandleOppoChUd);
                rhsCurrChd = &(rhsCurrNpc->chd());
            }
            auto& ccs = globalConfigConsts->character_configs();
            auto& rhsCc = ccs.at(rhsCurrChd->species_id());

            auto refAbsDx = (cc->capsule_radius()+rhsCc.capsule_radius()); 
            auto refAbsDy = (cc->capsule_half_height()+rhsCc.capsule_half_height()); 
            if (candAbsX <= (cc->capsule_radius()+refAbsDx) && -(refAbsDy+cc->capsule_half_height()) <= selfNpcPositionDiffForOppoChUd.GetY() && selfNpcPositionDiffForOppoChUd.GetY() <= (refAbsDy+cc->capsule_half_height())) {
                newVisionReaction = TARGET_CH_REACTION_USE_MELEE;
            }
        }
    }
    
    return newVisionReaction;
}
