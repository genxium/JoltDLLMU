#ifndef BASE_BATTLE_COLLISION_FILTER_H_
#define BASE_BATTLE_COLLISION_FILTER_H_ 1

#include <atomic>
#include <Jolt/Jolt.h>
#include <Jolt/Core/Reference.h>
#include <Jolt/Math/Float2.h>

#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/Character/Character.h>
#include <Jolt/Physics/Body/Body.h>
#include "CppOnlyConsts.h"
#include "PbConsts.h"

#include "serializable_data.pb.h"

#define BL_COLLIDER_T JPH::Body
#define BL_CACHE_KEY_T std::vector<float>
#define BL_COLLIDER_Q std::vector<BL_COLLIDER_T*>
#define CH_CACHE_KEY_T std::vector<float>

#define CH_COLLIDER_T JPH::Character
#define CH_COLLIDER_Q std::vector<CH_COLLIDER_T*>

class BaseBattleCollisionFilter {
public:
    std::atomic<uint32_t>       mNextRdfBulletIdCounter = 0;
    std::atomic<uint32_t>       mNextRdfBulletCount = 0;

    virtual JPH::ValidateResult validateLhsCharacterContact(const CharacterDownsync* lhsCurrChd, const CharacterDownsync* rhsCurrChd) = 0;

    virtual JPH::ValidateResult validateLhsCharacterContact(const CharacterDownsync* lhsCurrChd, const Bullet* rhsCurrBl) = 0;

    virtual JPH::ValidateResult validateLhsCharacterContact(const CharacterDownsync* lhsCurrChd, const uint64_t udRhs, const uint64_t udtRhs) = 0;

    virtual JPH::ValidateResult validateLhsCharacterContact(const uint64_t udLhs, const uint64_t udtLhs,
        const JPH::Body& lhs, // the "Character"
        const uint64_t udRhs, const uint64_t udtRhs, const JPH::Body& rhs) = 0;

    virtual JPH::ValidateResult validateLhsBulletContact(const Bullet* lhsCurrBl, const uint64_t udRhs, const uint64_t udtRhs) = 0;

    virtual JPH::ValidateResult validateLhsBulletContact(const uint64_t udLhs,
        const JPH::Body& lhs, // the "Bullet"
        const uint64_t udRhs, const uint64_t udtRhs, const JPH::Body& rhs) = 0;
    
    /*
    [WARNING] Per thread-safety concerns, for any collision pair in "handleLhsXxxCollision", only the impact to the lhs instance in "nextRdf" should be calculated and updated.

    For example, for a same "character-bullet" collision, in "handleLhsCharacterCollision" we calculate damage and update hp, in "handleLhsBulletCollision" we calculate and update explosion status.
    */
    virtual void handleLhsCharacterCollision(
        const int currRdfId,
        RenderFrame* nextRdf,
        const uint64_t udLhs, const uint64_t udtLhs, const CharacterDownsync* currChd, CharacterDownsync* nextChd,
        const uint64_t udRhs, const uint64_t udtRhs,
        const JPH::CollideShapeResult& inResult,
        uint32_t& outNewEffDebuffSpeciesId, int& outNewDamage, bool& outNewEffBlownUp, int& outNewEffFramesToRecover, float& outNewEffPushbackVelX, float& outNewEffPushbackVelY) = 0;

    virtual void handleLhsBulletCollision(
        const int currRdfId,
        RenderFrame* nextRdf,
        const uint64_t udLhs, const uint64_t udtLhs, const Bullet* currBl, Bullet* nextBl,
        const uint64_t udRhs, const uint64_t udtRhs, 
        const JPH::CollideShapeResult& inResult) = 0;

    virtual ~BaseBattleCollisionFilter() {

    }

    inline const uint64_t calcUserData(const PlayerCharacterDownsync& playerChd) const {
        return UDT_PLAYER + playerChd.join_index();
    }

    inline const uint64_t calcUserData(const NpcCharacterDownsync& npcChd) const {
        return UDT_NPC + npcChd.id();
    }

    inline const uint64_t calcUserData(const Bullet& bl) const {
        return UDT_BL + bl.id();
    }

    inline const uint64_t calcStaticColliderUserData(const int staticColliderId) const {
        return UDT_OBSTACLE + staticColliderId;
    }

    inline const uint64_t getUDT(const uint64_t& ud) const {
        return (ud & UDT_STRIPPER);
    }

    inline const uint32_t getUDPayload(const uint64_t& ud) const {
        return (ud & UD_PAYLOAD_STRIPPER);
    }
}; 

#endif
