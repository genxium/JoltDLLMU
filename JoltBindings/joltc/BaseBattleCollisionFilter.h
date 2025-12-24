#ifndef BASE_BATTLE_COLLISION_FILTER_H_
#define BASE_BATTLE_COLLISION_FILTER_H_ 1

#include "serializable_data.pb.h"

#include <atomic>
#include <utility> // for "std::pair"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Character/Character.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Math/Vec3.h>

#include "CppOnlyConsts.h"
#include "PbConsts.h"

using namespace JPH;

#define BL_COLLIDER_T JPH::Body
#define BL_CACHE_KEY_T std::vector<float>
#define BL_COLLIDER_Q std::vector<BL_COLLIDER_T*>

#define CH_CACHE_KEY_T std::vector<float>
#define CH_COLLIDER_T JPH::Character
#define CH_COLLIDER_Q std::vector<CH_COLLIDER_T*>

#define NON_CONTACT_CONSTRAINT_T JPH::Constraint

const Quat  cTurnbackAroundYAxis = Quat(0, 1, 0, 0);
const Quat  cTurn90DegsAroundZAxis = Quat::sRotation(Vec3::sAxisZ(), 0.5f*3.1415926);
const Mat44 cTurn90DegsAroundZAxisMat = Mat44::sRotation(cTurn90DegsAroundZAxis);


class BaseBattleCollisionFilter {
public:
    std::atomic<uint32_t>       mNextRdfBulletIdCounter = 0;
    std::atomic<uint32_t>       mNextRdfBulletCount = 0;

    virtual ValidateResult validateLhsCharacterContact(const CharacterDownsync* lhsCurrChd, const CharacterDownsync* rhsCurrChd) const = 0;

    virtual ValidateResult validateLhsCharacterContact(const CharacterDownsync* lhsCurrChd, const Bullet* rhsCurrBl) const = 0;

    virtual ValidateResult validateLhsCharacterContact(const CharacterDownsync* lhsCurrChd, const uint64_t udRhs, const uint64_t udtRhs) const = 0;

    virtual ValidateResult validateLhsCharacterContact(const uint64_t udLhs, const uint64_t udtLhs,
        const Body& lhs, // the "Character"
        const uint64_t udRhs, const uint64_t udtRhs, const Body& rhs) const = 0;

    virtual ValidateResult validateLhsBulletContact(const Bullet* lhsCurrBl, const uint64_t udRhs, const uint64_t udtRhs) const = 0;

    virtual ValidateResult validateLhsBulletContact(const uint64_t udLhs,
        const Body& lhs, // the "Bullet"
        const uint64_t udRhs, const uint64_t udtRhs, const Body& rhs) const = 0;
    
    /*
    [WARNING] Per thread-safety concerns, for any collision pair in "handleLhsXxxCollision", only the impact to the lhs instance in "nextRdf" should be calculated and updated.

    For example, for a same "character-bullet" collision, in "handleLhsCharacterCollision" we calculate damage and update hp, in "handleLhsBulletCollision" we calculate and update explosion status.
    */
    virtual void handleLhsCharacterCollision(
        const int currRdfId,
        RenderFrame* nextRdf,
        const uint64_t udLhs, const uint64_t udtLhs, const CharacterDownsync* currChd, CharacterDownsync* nextChd,
        const uint64_t udRhs, const uint64_t udtRhs,
        const CollideShapeResult& inResult,
        uint32_t& outNewEffDebuffSpeciesId, int& outNewDamage, bool& outNewEffBlownUp, int& outNewEffFramesToRecover, int& outNewEffDef1QuotaReduction, float& outNewEffPushbackVelX, float& outNewEffPushbackVelY) = 0;

    virtual void handleLhsBulletCollision(
        const int currRdfId,
        RenderFrame* nextRdf,
        const uint64_t udLhs, const uint64_t udtLhs, const Bullet* currBl, Bullet* nextBl,
        const uint64_t udRhs, const uint64_t udtRhs, 
        const CollideShapeResult& inResult) = 0;

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

    inline static bool chIsNotDashing(const CharacterDownsync& chd) {
        return (Dashing != chd.ch_state() && Sliding != chd.ch_state() && BackDashing != chd.ch_state() && InAirDashing != chd.ch_state());
    }

    inline static bool chCanJumpWithInertia(const CharacterDownsync& currChd, const CharacterConfig* cc, bool notDashing) {
        if (0 >= cc->jumping_init_vel_y()) return false;
        if (0 >= currChd.frames_to_recover()) return true;
        if (!notDashing && cc->proactive_jump_startup_frames() <= currChd.frames_in_ch_state()) return true;
        if (walkingAtkSet.count(currChd.ch_state()) && cc->proactive_jump_startup_frames() <= currChd.frames_in_ch_state()) return true;
        return false;
    }

    inline static float InvSqrt32(float number) {
        long i;
        float x2, y;
        const float threehalfs = 1.5F;

        x2 = number * 0.5F;
        y = number;
        i = *(long*)&y;
        i = 0x5f3759df - (i >> 1);
        y = *(float*)&i;
        y = y * (threehalfs - (x2 * y * y));

        return y;
    }

    inline static bool IsLengthNearZero(float length) {
        return -cLengthEps < length && length < cLengthEps;
    }

    inline static bool IsLengthSquaredNearZero(float lengthSquared) {
        return cLengthEpsSquared > lengthSquared;
    }

    inline static bool IsLengthDiffNearlySame(float lengthDiff) {
        return -cLengthNearlySameEps < lengthDiff && lengthDiff < cLengthNearlySameEps;
    }

    inline static bool IsLengthDiffSquaredNearlySame(float lengthDiffSquared) {
        return cLengthNearlySameEpsSquared > lengthDiffSquared;
    }

    inline static bool isNearlySame(float lhs, float rhs) {
        /*
        Floating point calculations are NOT associative, and Jolt uses "warm-start solvers" as well as "multi-threading" extensively, it's reasonable to set some tolerance for "nearly the same".
        */
        return IsLengthDiffNearlySame(rhs - lhs);
    }
    
    inline static bool isNearlySame(const float lhsX, const float lhsY, const float lhsZ, const float rhsX, const float rhsY, const float rhsZ) {
        float dx = rhsX - lhsX;
        float dy = rhsY - lhsY;
        float dz = rhsZ - lhsZ;
        return IsLengthDiffSquaredNearlySame(dx * dx + dy * dy + dz * dz);
    }

    inline static bool isNearlySame(Vec3& lhs, Vec3& rhs) {
        return isNearlySame(lhs.GetX(), lhs.GetY(), lhs.GetZ(), rhs.GetX(), rhs.GetY(), rhs.GetZ());
    }

    inline static uint64_t CalcJoinIndexMask(uint32_t joinIndex) {
        if (0 == joinIndex) return 0;
        return (U64_1 << (joinIndex - 1));
    }
}; 

#endif
