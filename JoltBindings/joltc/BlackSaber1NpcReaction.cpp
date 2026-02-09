#include "BlackSaber1NpcReaction.h"

int BlackSaber1NpcReaction::deriveNpcVisionReactionAgainstOppoChUd(int rdfId, std::unordered_map<uint64_t, const PlayerCharacterDownsync*>& currPlayersMap, std::unordered_map<uint64_t, const NpcCharacterDownsync*>& currNpcsMap, const CH_COLLIDER_T* selfNpcCollider, const BodyID& selfNpcBodyID, const uint64_t selfNpcUd, const CharacterDownsync& currChd, const MassProperties& massProps, const Vec3& currChdFacing, const CharacterConfig* cc, CharacterDownsync* nextChd, bool cvSupported, bool cvInAir, bool cvOnWall, bool currNotDashing, bool currEffInAir, bool oldNextNotDashing, bool oldNextEffInAir, bool inJumpStartupOrJustEnded, CharacterBase::EGroundState cvGroundState, bool canJumpWithinInertia, const Vec3& visionDirection, const uint64_t toHandleOppoChUd, const Vec3& selfNpcPositionDiffForOppoChUd, bool& outOpponentBehindMe, bool& outOpponentAboveMe, bool& outOpponentIsAttacking, bool& outOpponentIsFacingMe) {

    int newVisionReaction = BaseNpcReaction::deriveNpcVisionReactionAgainstOppoChUd(rdfId, currPlayersMap, currNpcsMap, selfNpcCollider, selfNpcBodyID, selfNpcUd, currChd, massProps, currChdFacing, cc, nextChd, cvSupported, cvInAir, cvOnWall, currNotDashing, currEffInAir, oldNextNotDashing, oldNextEffInAir, inJumpStartupOrJustEnded, cvGroundState, canJumpWithinInertia, visionDirection, toHandleOppoChUd, selfNpcPositionDiffForOppoChUd, outOpponentBehindMe, outOpponentAboveMe, outOpponentIsAttacking, outOpponentIsFacingMe);

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
