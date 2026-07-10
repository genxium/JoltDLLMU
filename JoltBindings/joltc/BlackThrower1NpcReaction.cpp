#include "BlackThrower1NpcReaction.h"

int BlackThrower1NpcReaction::deriveNpcVisionReactionAgainstOppoChUd(int rdfId, std::unordered_map<uint64_t, const PlayerCharacterDownsync*>& currPlayersMap, std::unordered_map<uint64_t, const NpcCharacterDownsync*>& currNpcsMap, const CH_COLLIDER_T* selfNpcCollider, const BodyID& selfNpcBodyID, const uint64_t selfNpcUd, const CharacterDownsync& currChd, const MassProperties& massProps, const Vec3& currChdFacing, const CharacterConfig* cc, CharacterDownsync* nextChd, const bool cvSupported, const bool cvInAir, const bool cvOnWall, const bool currNotDashing, const bool currEffInAir, const bool currIsFlying, const bool oldNextNotDashing, const bool oldNextEffInAir, const bool inJumpStartupOrJustEnded, CharacterBase::EGroundState cvGroundState, const bool canJumpWithinInertia, const Vec3& visionDirection, const uint64_t toHandleOppoChUd, const Vec3& selfNpcPositionDiffForOppoChUd, bool& outOpponentBehindMe, bool& outOpponentAboveMe, bool& outOpponentIsAttacking, bool& outOpponentIsFacingMe) {

    int newVisionReaction = BaseNpcReaction::deriveNpcVisionReactionAgainstOppoChUd(rdfId, currPlayersMap, currNpcsMap, selfNpcCollider, selfNpcBodyID, selfNpcUd, currChd, massProps, currChdFacing, cc, nextChd, cvSupported, cvInAir, cvOnWall, currNotDashing, currEffInAir, currIsFlying, oldNextNotDashing, oldNextEffInAir, inJumpStartupOrJustEnded, cvGroundState, canJumpWithinInertia, visionDirection, toHandleOppoChUd, selfNpcPositionDiffForOppoChUd, outOpponentBehindMe, outOpponentAboveMe, outOpponentIsAttacking, outOpponentIsFacingMe);

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
    bool closeEnoughForMelee = candAbsX <= (10*cc->capsule_radius()+refAbsDx);

    auto* initSkillDict = &(cc->init_skill_transit());
    const google::protobuf::Map<uint32, Skill>& skillConfigs = globalConfigConsts->skill_configs();
    int encodedPattern = BaseBattleCollisionFilter::EncodePatternForInitSkill(globalPrimitiveConsts->pattern_b(), currEffInAir, false, false, false, false, false, false, false);
    uint32 targetSkillId = initSkillDict->at(encodedPattern);
    const Skill& targetSkillConfig = skillConfigs.at(targetSkillId);

    if (0 >= currChd.frames_to_recover() && targetSkillConfig.mp_delta() <= currChd.mp()) {
        if (outOpponentBehindMe) {
            // Intentionally left blank, just use existing "newVisionReaction"
            newVisionReaction = TARGET_CH_REACTION_FOLLOW;
        } else {
            if (closeEnoughForMelee) {
                newVisionReaction = TARGET_CH_REACTION_USE_MELEE;
            } else {
                newVisionReaction = TARGET_CH_REACTION_FOLLOW;
            }
        }
    } else {
        newVisionReaction = TARGET_CH_REACTION_NOT_ENOUGH_MP;
    }
    return newVisionReaction;
}
