#ifndef BASE_BATTLE_COLLISION_FILTER_H_
#define BASE_BATTLE_COLLISION_FILTER_H_ 1

#include "serializable_data.pb.h"

#include <atomic>
#include <utility> // for "std::pair"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Character/Character.h>
#include <Jolt/Physics/Body/Body.h>
#include "CppOnlyConsts.h"
#include "PbConsts.h"

#define BL_COLLIDER_T JPH::Body
#define BL_CACHE_KEY_T std::vector<float>
#define BL_COLLIDER_Q std::vector<BL_COLLIDER_T*>

#define CH_CACHE_KEY_T std::vector<float>
#define CH_COLLIDER_T JPH::Character
#define CH_COLLIDER_Q std::vector<CH_COLLIDER_T*>

#define NON_CONTACT_CONSTRAINT_T JPH::Constraint

const JPH::Quat  cTurnbackAroundYAxis = JPH::Quat(0, 1, 0, 0);
const JPH::Quat  cTurn90DegsAroundZAxis = JPH::Quat::sRotation(JPH::Vec3::sAxisZ(), 0.5f*3.1415926);
const JPH::Mat44 cTurn90DegsAroundZAxisMat = JPH::Mat44::sRotation(cTurn90DegsAroundZAxis);

class BaseBattleCollisionFilter {
public:
    std::atomic<uint32_t>       mNextRdfBulletIdCounter = 0;
    std::atomic<uint32_t>       mNextRdfBulletCount = 0;

    virtual JPH::ValidateResult validateLhsCharacterContact(const CharacterDownsync* lhsCurrChd, const CharacterDownsync* rhsCurrChd) const = 0;

    virtual JPH::ValidateResult validateLhsCharacterContact(const CharacterDownsync* lhsCurrChd, const Bullet* rhsCurrBl) const = 0;

    virtual JPH::ValidateResult validateLhsCharacterContact(const CharacterDownsync* lhsCurrChd, const uint64_t udRhs, const uint64_t udtRhs) const = 0;

    virtual JPH::ValidateResult validateLhsCharacterContact(const uint64_t udLhs, const uint64_t udtLhs,
        const JPH::Body& lhs, // the "Character"
        const uint64_t udRhs, const uint64_t udtRhs, const JPH::Body& rhs) const = 0;

    virtual JPH::ValidateResult validateLhsBulletContact(const Bullet* lhsCurrBl, const uint64_t udRhs, const uint64_t udtRhs) const = 0;

    virtual JPH::ValidateResult validateLhsBulletContact(const uint64_t udLhs,
        const JPH::Body& lhs, // the "Bullet"
        const uint64_t udRhs, const uint64_t udtRhs, const JPH::Body& rhs) const = 0;
    
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

    inline const uint64_t calcPublishingToTriggerUd(const NpcCharacterDownsync& npcChd) {
        return calcTriggerUserData(npcChd.publishing_to_trigger_id_upon_exhausted());
    }

    inline const uint64_t calcPublishingToTriggerUd(const Trigger& tr) {
        return calcTriggerUserData(tr.publishing_to_trigger_id_upon_exhausted());
    }

    inline const uint64_t calcUserData(const PlayerCharacterDownsync& playerChd) const {
        return calcPlayerUserData(playerChd.join_index());
    }

    inline const uint64_t calcUserData(const NpcCharacterDownsync& npcChd) const {
        return calcNpcUserData(npcChd.id());
    }

    inline const uint64_t calcUserData(const Bullet& bl) const {
        return calcBulletUserData(bl.id());
    }

    inline const uint64_t calcUserData(const Trigger& trigger) const {
        return calcTriggerUserData(trigger.id());
    }

    inline const uint64_t calcUserData(const Trap& trap) const {
        return calcTrapUserData(trap.id());
    }

    inline const uint64_t calcUserData(const Pickable& pk) const {
        return calcPickableUserData(pk.id());
    }

    // The following static member functions are to be used by frontend too 
    inline static const uint64_t getUDT(const uint64_t& ud) {
        return (ud & UDT_STRIPPER);
    }

    inline static const uint32_t getUDPayload(const uint64_t& ud) {
        return (ud & UD_PAYLOAD_STRIPPER);
    }

    inline static const uint64_t calcStaticColliderUserData(const uint32_t staticColliderId) {
        return UDT_OBSTACLE + staticColliderId;
    }

    inline static const uint64_t calcPlayerUserData(const uint32_t joinIndex) {
        return UDT_PLAYER + joinIndex;
    }

    inline static const uint64_t calcNpcUserData(const uint32_t npcId) {
        return UDT_NPC + npcId;
    }

    inline static const uint64_t calcBulletUserData(const uint32_t bulletId) {
        return UDT_BL + bulletId;
    }

    inline static const uint64_t calcTriggerUserData(const uint32_t triggerId) {
        return UDT_TRIGGER + triggerId;
    }

    inline static const uint64_t calcTrapUserData(const uint32_t trapId) {
        return UDT_TRAP + trapId;
    }

    inline static const uint64_t calcPickableUserData(const uint32_t pickableId) {
        return UDT_PICKABLE + pickableId;
    }

    inline static uint32_t encodeDir(const int dx, const int dy) {
        if (0 == dx && 0 == dy) return 0;
        if (0 == dx) {
            if (0 < dy) return 1; // up
            else return 2; // down
        }
        if (0 < dx) {
            if (0 == dy) return 3; // right
            if (0 < dy) return 5;
            else return 7;
        }
        // 0 > dx
        if (0 == dy) return 4; // left
        if (0 < dy) return 8;
        else return 6;
    }

    inline static uint64_t encodeInput(const InputFrameDecoded& ifDecoded) {
        return encodeInput(ifDecoded.dx(), ifDecoded.dy(), ifDecoded.btn_a_level(), ifDecoded.btn_b_level(), ifDecoded.btn_c_level(), ifDecoded.btn_d_level(), ifDecoded.btn_e_level());
    }

    inline static uint64_t encodeInput(const int dx, const int dy, const uint64_t btnALevel, const uint64_t btnBLevel, const uint64_t btnCLevel, const uint64_t btnDLevel, const uint64_t btnELevel) {
        uint64_t encodedBtnALevel = (btnALevel << 4);
        uint64_t encodedBtnBLevel = (btnBLevel << 5);
        uint64_t encodedBtnCLevel = (btnCLevel << 6);
        uint64_t encodedBtnDLevel = (btnDLevel << 7);
        uint64_t encodedBtnELevel = (btnELevel << 8);
        uint64_t discretizedDir = encodeDir(dx, dy);
        return (discretizedDir + encodedBtnALevel + encodedBtnBLevel + encodedBtnCLevel + encodedBtnDLevel + encodedBtnELevel);
    }

    inline static bool decodeInput(uint64_t encodedInput, InputFrameDecoded* holder) {
        holder->Clear();
        int encodedDirection = (int)(encodedInput & 15);
        uint64_t btnALevel = ((encodedInput >> 4) & 1);
        uint64_t btnBLevel = ((encodedInput >> 5) & 1);
        uint64_t btnCLevel = ((encodedInput >> 6) & 1);
        uint64_t btnDLevel = ((encodedInput >> 7) & 1);
        uint64_t btnELevel = ((encodedInput >> 8) & 1);

        holder->set_dx(DIRECTION_DECODER[encodedDirection][0]);
        holder->set_dy(DIRECTION_DECODER[encodedDirection][1]);
        holder->set_btn_a_level(btnALevel);
        holder->set_btn_b_level(btnBLevel);
        holder->set_btn_c_level(btnCLevel);
        holder->set_btn_d_level(btnDLevel);
        holder->set_btn_e_level(btnELevel);
        return true;
    }


    inline static bool hasCriticalBtnLevel(const InputFrameDecoded& decodedInputHolder) {
        return 0 < decodedInputHolder.btn_a_level() || 0 < decodedInputHolder.btn_b_level() || 0 < decodedInputHolder.btn_c_level() || 0 < decodedInputHolder.btn_d_level() || 0 < decodedInputHolder.btn_e_level();
    }

    inline static uint64_t sanitizeCachedCueCmd(uint64_t origCmd) {
        return (origCmd & 31u); // i.e. Only reserve directions and BtnALevel
    }
}; 

#endif
