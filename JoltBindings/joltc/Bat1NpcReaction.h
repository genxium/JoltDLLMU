#ifndef BAT1_NPC_REACTION_H_
#define BAT1_NPC_REACTION_H_ 1

#include "BaseNpcReaction.h"

using namespace jtshared;
using namespace JPH;

class Bat1NpcReaction : public BaseNpcReaction {
public:
    Bat1NpcReaction() : BaseNpcReaction() {

    }

    virtual void postStepDeriveNpcVisionReaction(int currRdfId, const Vec3& antiGravityNorm, const float gravityMagnitude, std::unordered_map<uint64_t, const PlayerCharacterDownsync*>& currPlayersMap, std::unordered_map<uint64_t, const NpcCharacterDownsync*>& currNpcsMap, std::unordered_map<uint64_t, const Bullet*>& currBulletsMap, const BodyInterface* biNoLock, const NarrowPhaseQuery* narrowPhaseQuery, const BaseBattleCollisionFilter* baseBattleFilter, const DefaultBroadPhaseLayerFilter& bplf, const DefaultObjectLayerFilter& olf, const CH_COLLIDER_T* selfNpcCollider, const BodyID& selfNpcBodyID, const uint64_t selfNpcUd, const NpcGoal currNpcGoal, const uint64_t currNpcCachedCueCmd, const CharacterDownsync& currChd, const MassProperties& massProps, const Vec3& currChdFacing, const CharacterConfig* cc, CharacterDownsync* nextChd, const bool cvSupported, const bool cvInAir, const bool cvOnWall, const bool currNotDashing, const bool currEffInAir, const bool currIsFlying, const bool oldNextNotDashing, const bool oldNextEffInAir, const bool inJumpStartupOrJustEnded, CharacterBase::EGroundState cvGroundState, const uint64_t toRevengeOppoUdt, const uint64_t toRevengeOppoUd, const Vec3& positionDiffForToRevengeOppoUd, NpcGoal& outNextNpcGoal, uint64_t& outCmd, int& outLastFledRdfId);

    virtual int deriveNpcVisionReactionAgainstOppoChUd(int rdfId, std::unordered_map<uint64_t, const PlayerCharacterDownsync*>& currPlayersMap, std::unordered_map<uint64_t, const NpcCharacterDownsync*>& currNpcsMap, const CH_COLLIDER_T* selfNpcCollider, const BodyID& selfNpcBodyID, const uint64_t selfNpcUd, const CharacterDownsync& currChd, const MassProperties& massProps, const Vec3& currChdFacing, const CharacterConfig* cc, CharacterDownsync* nextChd, const bool cvSupported, const bool cvInAir, const bool cvOnWall, const bool currNotDashing, const bool currEffInAir, const bool currIsFlying, const bool oldNextNotDashing, const bool oldNextEffInAir, const bool inJumpStartupOrJustEnded, CharacterBase::EGroundState cvGroundState, const bool canJumpWithinInertia, const Vec3& visionDirection, const uint64_t toHandleOppoChUd, const Vec3& selfNpcPositionDiffForOppoChUd, bool& outOpponentBehindMe, bool& outOpponentAboveMe, bool& outOpponentIsAttacking, bool& outOpponentIsFacingMe);

    virtual int deriveReactionAgainstGroundAndMvBlocker(int currRdfId, const Vec3& antiGravityNorm, const float gravityMagnitude, const BodyInterface* biNoLock, const CH_COLLIDER_T* selfNpcCollider, const BodyID& selfNpcBodyID, const uint64_t selfNpcUd, const NpcGoal inNpcGoal, const CharacterDownsync& currChd, const MassProperties& massProps, const Vec3& currChdFacing, const CharacterConfig* cc, CharacterDownsync* nextChd, const bool cvSupported, const bool cvInAir, const bool cvOnWall, const bool currNotDashing, const bool currEffInAir, const bool currIsFlying, const bool oldNextNotDashing, const bool oldNextEffInAir, const bool inJumpStartupOrJustEnded, CharacterBase::EGroundState cvGroundState, const bool canJumpWithinInertia, const AABox& visionAABB, const Vec3Arg& visionNarrowPhaseInBaseOffset, const Vec3& visionDirection, const BodyID& toHandleMvBlockerBodyID, const uint64_t toHandleMvBlockerUd, const GapToJump& currGapToJump, const GapToJump& minGapToJump, const GapToJump& currGroundMvTolerance, const int visionReactionByFar, const uint64_t toHandleOppoChUd, const Vec3& selfNpcPositionDiffForOppoChUd, const bool opponentBehindMe, const bool opponentAboveMe, const bool opponentIsAttacking, const bool opponentIsFacingMe, const int lastFledRdfId);
};

#endif
