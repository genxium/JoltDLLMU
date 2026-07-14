#include "Bat1NpcReaction.h"

void Bat1NpcReaction::postStepDeriveNpcVisionReaction(int currRdfId, const Vec3& antiGravityNorm, const float gravityMagnitude, std::unordered_map<uint64_t, const PlayerCharacterDownsync*>& currPlayersMap, std::unordered_map<uint64_t, const NpcCharacterDownsync*>& currNpcsMap, std::unordered_map<uint64_t, const Bullet*>& currBulletsMap, const BodyInterface* biNoLock, const NarrowPhaseQuery* narrowPhaseQuery, const BaseBattleCollisionFilter* baseBattleFilter, const DefaultBroadPhaseLayerFilter& bplf, const DefaultObjectLayerFilter& olf, const CH_COLLIDER_T* selfNpcCollider, const BodyID& selfNpcBodyID, const uint64_t selfNpcUd, const NpcGoal currNpcGoal, const uint64_t currNpcCachedCueCmd, const CharacterDownsync& currChd, const MassProperties& massProps, const Vec3& currChdFacing, const CharacterConfig* cc, CharacterDownsync* nextChd, const bool cvSupported, const bool cvInAir, const bool cvOnWall, const bool currNotDashing, const bool currEffInAir, const bool currIsFlying, const bool oldNextNotDashing, const bool oldNextEffInAir, const bool inJumpStartupOrJustEnded, CharacterBase::EGroundState cvGroundState, const uint64_t toRevengeOppoUdt, const uint64_t toRevengeOppoUd, const Vec3& positionDiffForToRevengeOppoUd, NpcGoal& outNextNpcGoal, uint64_t& outCmd, int& outLastFledRdfId) {
    if (cc->omit_gravity() && cc->anti_gravity_when_idle() && InAirIdle1NoJump == nextChd->ch_state() && globalPrimitiveConsts->default_fleeing_grace_period_rdf_cnt() >= nextChd->frames_in_ch_state()) {
        outCmd = 0;
    } else {
        BaseNpcReaction::postStepDeriveNpcVisionReaction(currRdfId, antiGravityNorm, gravityMagnitude, currPlayersMap, currNpcsMap, currBulletsMap, biNoLock, narrowPhaseQuery, baseBattleFilter, bplf, olf, selfNpcCollider, selfNpcBodyID, selfNpcUd, currNpcGoal, currNpcCachedCueCmd, currChd, massProps, currChdFacing, cc, nextChd, cvSupported, cvInAir, cvOnWall, currNotDashing, currEffInAir, currIsFlying, oldNextNotDashing, oldNextEffInAir, inJumpStartupOrJustEnded, cvGroundState, toRevengeOppoUdt, toRevengeOppoUd, positionDiffForToRevengeOppoUd, outNextNpcGoal, outCmd, outLastFledRdfId);
    }
}

int Bat1NpcReaction::deriveNpcVisionReactionAgainstOppoChUd(int rdfId, std::unordered_map<uint64_t, const PlayerCharacterDownsync*>& currPlayersMap, std::unordered_map<uint64_t, const NpcCharacterDownsync*>& currNpcsMap, const CH_COLLIDER_T* selfNpcCollider, const BodyID& selfNpcBodyID, const uint64_t selfNpcUd, const CharacterDownsync& currChd, const MassProperties& massProps, const Vec3& currChdFacing, const CharacterConfig* cc, CharacterDownsync* nextChd, const bool cvSupported, const bool cvInAir, const bool cvOnWall, const bool currNotDashing, const bool currEffInAir, const bool currIsFlying, const bool oldNextNotDashing, const bool oldNextEffInAir, const bool inJumpStartupOrJustEnded, CharacterBase::EGroundState cvGroundState, const bool canJumpWithinInertia, const Vec3& visionDirection, const uint64_t toHandleOppoChUd, const Vec3& selfNpcPositionDiffForOppoChUd, bool& outOpponentBehindMe, bool& outOpponentAboveMe, bool& outOpponentIsAttacking, bool& outOpponentIsFacingMe) {

    int newVisionReaction = BaseNpcReaction::deriveNpcVisionReactionAgainstOppoChUd(rdfId, currPlayersMap, currNpcsMap, selfNpcCollider, selfNpcBodyID, selfNpcUd, currChd, massProps, currChdFacing, cc, nextChd, cvSupported, cvInAir, cvOnWall, currNotDashing, currEffInAir, currIsFlying, oldNextNotDashing, oldNextEffInAir, inJumpStartupOrJustEnded, cvGroundState, canJumpWithinInertia, visionDirection, toHandleOppoChUd, selfNpcPositionDiffForOppoChUd, outOpponentBehindMe, outOpponentAboveMe, outOpponentIsAttacking, outOpponentIsFacingMe);

    if (0 >= currChd.frames_to_recover()) {
        // Check melee reachable or not.
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
    
    return newVisionReaction;
}

int Bat1NpcReaction::deriveReactionAgainstGroundAndMvBlocker(int currRdfId, const Vec3& antiGravityNorm, const float gravityMagnitude, const BodyInterface* biNoLock, const CH_COLLIDER_T* selfNpcCollider, const BodyID& selfNpcBodyID, const uint64_t selfNpcUd, const NpcGoal inNpcGoal, const CharacterDownsync& currChd, const MassProperties& massProps, const Vec3& currChdFacing, const CharacterConfig* cc, CharacterDownsync* nextChd, const bool cvSupported, const bool cvInAir, const bool cvOnWall, const bool currNotDashing, const bool currEffInAir, const bool currIsFlying, const bool oldNextNotDashing, const bool oldNextEffInAir, const bool inJumpStartupOrJustEnded, CharacterBase::EGroundState cvGroundState, const bool canJumpWithinInertia, const AABox& visionAABB, const Vec3Arg& visionNarrowPhaseInBaseOffset, const Vec3& visionDirection, const BodyID& toHandleMvBlockerBodyID, const uint64_t toHandleMvBlockerUd, const GapToJump& currGapToJump, const GapToJump& minGapToJump, const GapToJump& currGroundMvTolerance, const int visionReactionByFar, const uint64_t toHandleOppoChUd, const Vec3& selfNpcPositionDiffForOppoChUd, const bool opponentBehindMe, const bool opponentAboveMe, const bool opponentIsAttacking, const bool opponentIsFacingMe, const int lastFledRdfId) {
    if (cc->omit_gravity() && cc->anti_gravity_when_idle() && InAirIdle1NoJump == nextChd->ch_state()) {
        return TARGET_CH_REACTION_UNCHANGED;
    } else {
        return BaseNpcReaction::deriveReactionAgainstGroundAndMvBlocker(currRdfId, antiGravityNorm, gravityMagnitude, biNoLock, selfNpcCollider, selfNpcBodyID, selfNpcUd, inNpcGoal, currChd, massProps, currChdFacing, cc, nextChd, cvSupported, cvInAir, cvOnWall, currNotDashing, currEffInAir, currIsFlying, oldNextNotDashing, oldNextEffInAir, inJumpStartupOrJustEnded, cvGroundState, canJumpWithinInertia, visionAABB, visionNarrowPhaseInBaseOffset, visionDirection, toHandleMvBlockerBodyID, toHandleMvBlockerUd, currGapToJump, minGapToJump, currGroundMvTolerance, visionReactionByFar, toHandleOppoChUd, selfNpcPositionDiffForOppoChUd, opponentBehindMe, opponentAboveMe, opponentIsAttacking, opponentIsFacingMe, lastFledRdfId);
    }
}
