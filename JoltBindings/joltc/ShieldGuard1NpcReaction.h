#ifndef SHIELDGUARD1_NPC_REACTION_H_
#define SHIELDGUARD1_NPC_REACTION_H_ 1

#include "BaseNpcReaction.h"

using namespace jtshared;
using namespace JPH;

class ShieldGuard1NpcReaction : public BaseNpcReaction {
public:
    ShieldGuard1NpcReaction() : BaseNpcReaction() {

    }

    virtual int deriveNpcVisionReactionAgainstOppoChUd(int rdfId, std::unordered_map<uint64_t, const PlayerCharacterDownsync*>& currPlayersMap, std::unordered_map<uint64_t, const NpcCharacterDownsync*>& currNpcsMap, const CH_COLLIDER_T* selfNpcCollider, const BodyID& selfNpcBodyID, const uint64_t selfNpcUd, const CharacterDownsync& currChd, const MassProperties& massProps, const Vec3& currChdFacing, const CharacterConfig* cc, CharacterDownsync* nextChd, const bool cvSupported, const bool cvInAir, const bool cvOnWall, const bool currNotDashing, const bool currEffInAir, const bool currIsFlying, const bool oldNextNotDashing, const bool oldNextEffInAir, const bool inJumpStartupOrJustEnded, CharacterBase::EGroundState cvGroundState, const bool canJumpWithinInertia, const Vec3& visionDirection, const uint64_t toHandleOppoChUd, const Vec3& selfNpcPositionDiffForOppoChUd, bool& outOpponentBehindMe, bool& outOpponentAboveMe, bool& outOpponentIsAttacking, bool& outOpponentIsFacingMe);
};

#endif
