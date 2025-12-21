#ifndef NPC_REACTION_H_
#define NPC_REACTION_H_ 1

#include "BaseBattleCollisionFilter.h"

#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Collision/NarrowPhaseQuery.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <map>

using namespace jtshared;
using namespace JPH;

const int TARGET_CH_REACTION_UNCHANGED = 0;
const int TARGET_CH_REACTION_USE_MELEE = 1;
const int TARGET_CH_REACTION_USE_DRAGONPUNCH = 2;
const int TARGET_CH_REACTION_USE_FIREBALL = 3;
const int TARGET_CH_REACTION_FOLLOW = 4;
const int TARGET_CH_REACTION_USE_SLOT_C = 5;
const int TARGET_CH_REACTION_NOT_ENOUGH_MP = 6;
const int TARGET_CH_REACTION_FLEE_OPPO = 7;
const int TARGET_CH_REACTION_DEF1 = 8;
const int TARGET_CH_REACTION_STOP_BY_MV_BLOCKER = 9;

const int TARGET_CH_REACTION_JUMP_TOWARDS_CH = 10;
const int TARGET_CH_REACTION_JUMP_TOWARDS_MV_BLOCKER = 11;
const int TARGET_CH_REACTION_SLIP_JUMP_TOWARDS_CH = 12;
const int TARGET_CH_REACTION_WALK_ALONG = 13;
const int TARGET_CH_REACTION_TURNAROUND_MV_BLOCKER = 14;

const int TARGET_CH_REACTION_STOP_BY_PATROL_CUE_ENTER = 15;
const int TARGET_CH_REACTION_STOP_BY_PATROL_CUE_CAPTURED = 16;

const int TARGET_CH_REACTION_HUNTING_LOSS = 17;

const int NPC_DEF1_MIN_HOLDING_RDF_CNT = 90;

const int VISION_SEARCH_RDF_RANDOMIZE_MASK = (1 << 4) + (1 << 3) + (1 << 1);

const float OPPO_DX_OFFSET = 10.0f; 
const float TURNAROUND_FROM_MV_BLOCKER_DX_THRESHOLD = 32.0f;

/*
[WARNING] The default implementation DOESN'T take into consideration flying/flyable-on-ground NPCs!
*/
class BaseNpcReaction {
public:
    BaseNpcReaction() {
    }

    /*
    [WARNING] Intentionally NOT using "const NpcCharacterDownsync& currNpc" or "NpcCharacterDownsync* nextNpc" in parameters, because I want this class to be reusable by "PlayerCharacterDownsync" for mocking player inputs when online arena match making is difficult.
    */
    void postStepDeriveNpcVisionReaction(int currRdfId, std::unordered_map<uint64_t, const PlayerCharacterDownsync*>& currPlayersMap, std::unordered_map<uint64_t, const NpcCharacterDownsync*>& currNpcsMap, std::unordered_map<uint64_t, const Bullet*>& currBulletsMap, const BodyInterface* biNoLock, const NarrowPhaseQuery* narrowPhaseQuery, const BaseBattleCollisionFilter* baseBattleFilter, const DefaultBroadPhaseLayerFilter& bplf, const DefaultObjectLayerFilter& dlf, const CH_COLLIDER_T* selfNpcCollider, const BodyID& selfNpcBodyID, const uint64_t selfNpcUd, const NpcGoal currNpcGoal, const uint64_t currNpcCachedCueCmd, const CharacterDownsync& currChd, const CharacterConfig* cc, CharacterDownsync* nextChd, bool cvSupported, bool cvInAir, bool cvOnWall, bool currNotDashing, bool currEffInAir, bool oldNextNotDashing, bool oldNextEffInAir, bool inJumpStartupOrJustEnded, CharacterBase::EGroundState cvGroundState, NpcGoal& outNextNpcGoal, uint64_t& outCmd);

    void extractKeyEntitiesInVision(int rdfId, std::unordered_map<uint64_t, const PlayerCharacterDownsync*>& currPlayersMap, std::unordered_map<uint64_t, const NpcCharacterDownsync*>& currNpcsMap, std::unordered_map<uint64_t, const Bullet*>& currBulletsMap, const BodyInterface* biNoLock, const CH_COLLIDER_T* selfNpcCollider, const BodyID& selfNpcBodyID, const uint64_t selfNpcUd, const CharacterDownsync& currChd, const CharacterConfig* cc, CharacterDownsync* nextChd, bool cvSupported, bool cvInAir, bool cvOnWall, bool currNotDashing, bool currEffInAir, bool oldNextNotDashing, bool oldNextEffInAir, bool inJumpStartupOrJustEnded, CharacterBase::EGroundState cvGroundState, const Vec3& visionDirection, const ClosestHitPerBodyCollisionCollector<CastShapeCollector>& visionCastResultCollector, uint64_t& outToHandleAllyUd, Vec3& outSelfNpcPositionDiffForAllyUd, uint64_t& outToHandleOppoChUd, Vec3& outSelfNpcPositionDiffForOppoChUd, uint64_t& outToHandleOppoBlUd, Vec3& outSelfNpcPositionDiffForOppoBlUd, uint64_t& outToHandleMvBlockerUd, Vec3& outSelfNpcPositionDiffForMvBlockerUd, BodyID& outToHandleMvBlockerBodyID, Vec3& outVisionOverlapAgainstMvBlocker);

    int deriveNpcVisionReactionAgainstOppoChUd(int rdfId, std::unordered_map<uint64_t, const PlayerCharacterDownsync*>& currPlayersMap, std::unordered_map<uint64_t, const NpcCharacterDownsync*>& currNpcsMap, const CH_COLLIDER_T* selfNpcCollider, const BodyID& selfNpcBodyID, const uint64_t selfNpcUd, const CharacterDownsync& currChd, const CharacterConfig* cc, CharacterDownsync* nextChd, bool cvSupported, bool cvInAir, bool cvOnWall, bool currNotDashing, bool currEffInAir, bool oldNextNotDashing, bool oldNextEffInAir, bool inJumpStartupOrJustEnded, CharacterBase::EGroundState cvGroundState, bool canJumpWithinInertia, const Vec3& visionDirection, const uint64_t toHandleOppoChUd, const Vec3& selfNpcPositionDiffForOppoChUd);

    int deriveReactionAgainstMvBlocker(int currRdfId, const BodyInterface* biNoLock, const CH_COLLIDER_T* selfNpcCollider, const BodyID& selfNpcBodyID, const uint64_t selfNpcUd, const NpcGoal currNpcGoal, const CharacterDownsync& currChd, const CharacterConfig* cc, CharacterDownsync* nextChd, bool cvSupported, bool cvInAir, bool cvOnWall, bool currNotDashing, bool currEffInAir, bool oldNextNotDashing, bool oldNextEffInAir, bool inJumpStartupOrJustEnded, CharacterBase::EGroundState cvGroundState, bool canJumpWithinInertia, const Vec3& visionDirection, const Vec3& visionOverlapAgainstRhs, const BodyID& toHandleMvBlockerBodyID, const uint64_t toHandleMvBlockerUd, const Vec3& selfNpcPositionDiffForMvBlockerUd, const int visionReactionByFar);
};

#endif
