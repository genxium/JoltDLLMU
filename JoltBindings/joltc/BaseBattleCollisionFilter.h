#ifndef BASE_BATTLE_COLLISION_FILTER_H_
#define BASE_BATTLE_COLLISION_FILTER_H_ 1

#include "serializable_data.pb.h"

#include <atomic>
#include <utility> // for "std::pair"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Character/Character.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Constraints/Constraint.h>
#include <Jolt/Math/Vec3.h>
#include <Jolt/Math/Quat.h>
#include <Jolt/Math/Mat44.h>

#include "CppOnlyConsts.h"
#include "PbConsts.h"

using namespace JPH;

#define BL_COLLIDER_T JPH::Body
#define BL_CACHE_KEY_T std::vector<float>
#define BL_COLLIDER_Q std::vector<BL_COLLIDER_T*>

#define CH_CACHE_KEY_T std::vector<float>
#define CH_COLLIDER_T JPH::Character
#define CH_COLLIDER_Q std::vector<CH_COLLIDER_T*>

#define TP_COLLIDER_T JPH::Body
typedef struct TrapCacheKey {
    float boxHalfExtentX;
    float boxHalfExtentY;
    EMotionType motionType;
    bool isSensor;
    ObjectLayer objLayer;

    TrapCacheKey(const float inBoxHalfExtentX, const float inBoxHalfExtentY, const EMotionType inMotionType, const bool inIsSensor, const ObjectLayer inObjLayer) : boxHalfExtentX(inBoxHalfExtentX), boxHalfExtentY(inBoxHalfExtentY), motionType(inMotionType), isSensor(inIsSensor), objLayer(inObjLayer) {}

    bool operator==(const TrapCacheKey& other) const {
        return boxHalfExtentX == other.boxHalfExtentX && boxHalfExtentY == other.boxHalfExtentY && motionType == other.motionType && isSensor == other.isSensor && objLayer == other.objLayer;
    }
} TP_CACHE_KEY_T;

typedef struct TrapCacheKeyHasher {
    std::size_t operator()(const TrapCacheKey& v) const {
        std::size_t seed = 4;
        seed ^= std::hash<float>()(v.boxHalfExtentX) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<float>()(v.boxHalfExtentY) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<EMotionType>()(v.motionType) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<bool>()(v.isSensor) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<ObjectLayer>()(v.objLayer) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
} TrapCacheKeyHasher;
#define TP_COLLIDER_Q std::vector<TP_COLLIDER_T*>

typedef struct NonContactConstraint {
    /* 
    [WARNING]
       
    "JPH::Array<Constraint*> ConstraintManager.mConstraints.pop_back()" might call the destructor on "JPH::Constraint" if "RefCount" is not carefully managed.
    */
    JPH::EConstraintType theType = EConstraintType::Constraint;
    JPH::EConstraintSubType theSubType = EConstraintSubType::Fixed;
    Ref<JPH::Constraint> c = nullptr;
    uint64_t ud1 = 0;
    uint64_t ud2 = 0;

    NonContactConstraint(JPH::Constraint* inC, const uint64_t inUd1, const uint64_t inUd2) {
        c = inC;
        theType = inC->GetType();
        theSubType = inC->GetSubType();
        ud1 = inUd1;
        ud2 = inUd2;
    }

    ~NonContactConstraint() {
        ud1 = 0; 
        ud2 = 0;
        theType = EConstraintType::Constraint;
        theSubType = EConstraintSubType::Fixed;
        // [WARNING] "c" will be automatically deleted by the destructor of "Ref<JPH::Constraint>"
    }
} NON_CONTACT_CONSTRAINT_T;

typedef struct NonContactConstraintCacheKey {
    /*
    [WARNING]

    For any "NonContactConstraint", caching isn't trivial because the "BodyID"s of both "Character"s and "Bullet"s can be reused.

    However it's impossible to declare "std::vector<NON_CONTACT_CONSTRAINT_T>  transientNonContactConstraints" to hold them on stack-memory either because such elements are of different sizes, e.g. "SliderConstraint" doesn't have the same size as "PathConstraint" and two different "PathConstraint"s may also have different sizes due to difference in length. Therefore, assignments like "transientNonContactConstraints[idxWithExistingElement] = newElementWithDifferentByteSize" would be quite inefficient -- most importantly, C++ prohibits "vector<AbstractClass>".

    Finally, it's not perfect to use "UserData" of "Character"s, "Bullet"s and "Trap"s either, because some constraints might be reused between different "Pair<UserData1, UserData2>"s, e.g. different instances of a same type of trap with different characters -- however it's still a good choice to include "UserData" in "NonContactConstraintCacheKey" because unlike "BodyID"s, the "UserData"s would NOT be reused (i.e. not re-assignable to new instances), hence matching "UserData"s implies a big part of the constraint can be reused.
    */
    EConstraintType theType;
    EConstraintSubType theSubType;
    uint64_t ud1;
    uint64_t ud2;

    NonContactConstraintCacheKey(const EConstraintType inType, const EConstraintSubType inSubType, const uint64_t inUd1, const uint64_t inUd2) : theType(inType), theSubType(inSubType), ud1(inUd1), ud2(inUd2) {}

    bool operator==(const NonContactConstraintCacheKey& other) const {
        return theType == other.theType && theSubType == other.theSubType && ud1 == other.ud1 && ud2 == other.ud2;
    }
} NON_CONTACT_CONSTRAINT_CACHE_KEY_T;

typedef struct NonContactConstraintCacheKeyHasher {
    std::size_t operator()(const NonContactConstraintCacheKey& v) const {
        std::size_t seed = 4;
        seed ^= std::hash<EConstraintType>()(v.theType) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<EConstraintSubType>()(v.theSubType) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<uint64_t>()(v.ud1) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<uint64_t>()(v.ud2) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
} NonContactConstraintCacheKeyHasher;

#define NON_CONTACT_CONSTRAINT_Q std::vector<NON_CONTACT_CONSTRAINT_T*>

static const float      cHalfPI = 0.5*JPH_PI;
static const JPH::Quat  cIdentityQ = JPH::Quat(0, 0, 0, 1);
static const JPH::Quat  cTurnbackAroundYAxis = JPH::Quat(0, 1, 0, 0);
static const JPH::Quat  cTurnMiniatureAroundYAxis = JPH::Quat::sRotation(Vec3::sAxisY(), JPH_PI/180);
static const JPH::Quat  cTurnNegativeMiniatureAroundYAxis = JPH::Quat::sRotation(Vec3::sAxisY(), -JPH_PI/180);
static const JPH::Quat  cTurn90DegsAroundYAxis = JPH::Quat::sRotation(Vec3::sAxisY(), 0.5f*JPH_PI);
static const JPH::Vec3  cXAxis = JPH::Vec3(1, 0, 0);
static const JPH::Vec3  cYAxis = JPH::Vec3(0, 1, 0);
static const JPH::Vec3  cNegativeZAxis = JPH::Vec3(0, 0, -1);

static const JPH::Quat  cTurn90DegsAroundZAxis = JPH::Quat::sRotation(Vec3::sAxisZ(), 0.5f*JPH_PI);
static const JPH::Mat44 cTurn90DegsAroundZAxisMat = JPH::Mat44::sRotation(cTurn90DegsAroundZAxis);

class BaseBattleCollisionFilter {
public:
    std::atomic<uint32_t>       mNextRdfBulletIdCounter = 0;
    std::atomic<uint32_t>       mNextRdfBulletCount = 0;

    virtual ValidateResult validateLhsCharacterContact(const CharacterDownsync* lhsCurrChd, const CharacterDownsync* rhsCurrChd) const = 0;

    virtual ValidateResult validateLhsCharacterContact(const CharacterDownsync* lhsCurrChd, const Bullet* rhsCurrBl) const = 0;

    virtual ValidateResult validateLhsCharacterContact(const CharacterDownsync* lhsCurrChd, const CharacterDownsync* lhsNextChd, const uint64_t udRhs, const uint64_t udtRhs, const Body& rhs) const = 0;

    virtual ValidateResult validateLhsCharacterContact(const uint64_t udLhs, const uint64_t udtLhs,
        const Body& lhs, // the "Character"
        const uint64_t udRhs, const uint64_t udtRhs, const Body& rhs) const = 0;

    virtual ValidateResult validateLhsBulletContact(const Bullet* lhsCurrBl, const uint64_t udRhs, const uint64_t udtRhs, const Body& rhs) const = 0;

    virtual ValidateResult validateLhsBulletContact(const uint64_t udLhs,
        const Body& lhs, // the "Bullet"
        const uint64_t udRhs, const uint64_t udtRhs, const Body& rhs) const = 0;
    
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

    inline const uint64_t calcUserData(const Trap& trap) const {
        return calcTrapUserData(trap.id());
    }

    inline const uint64_t calcUserData(const Trigger& trigger) const {
        return calcTriggerUserData(trigger.id());
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

    inline static const uint64_t calcTrapUserData(const uint32_t trapId) {
        return UDT_TRAP + trapId;
    }

    inline static const uint64_t calcTriggerUserData(const uint32_t triggerId) {
        return UDT_TRIGGER + triggerId;
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
        return (Dashing != chd.ch_state() && Sliding != chd.ch_state() && BackDashing != chd.ch_state() && InAirDashing != chd.ch_state() && InAirBackDashing != chd.ch_state());
    }

    inline static bool chCanJumpWithInertia(const CharacterDownsync& currChd, const CharacterConfig* cc, const bool notDashing, const bool inJumpStartup) {
        if (0 >= cc->jump_acc_mag_y()) return false;
        if (0 >= currChd.frames_to_recover()) return true;
        if (inJumpStartup) return false;
        if (walkingAtkSet.count(currChd.ch_state()) && cc->jump_startup_frames() <= currChd.frames_in_ch_state()) return true;
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

    inline static void calcChdFacing(const CharacterDownsync& currChd, Quat& outQ, Vec3& outFacing) {
        outQ = Quat(currChd.q_x(), currChd.q_y(), currChd.q_z(), currChd.q_w());
        Vec3 outFacingRaw = outQ.RotateAxisX();
        float outFacingRawProjX = outFacingRaw.Dot(Vec3::sAxisX()); 
        JPH_ASSERT(0 != outFacingRawProjX); // Guaranteed by "lampChdQ"
        float outFacingX = 0 < outFacingRawProjX ? +1 : -1; 
        outFacing.Set(outFacingX, 0, 0); 
    }

    inline static void clampChdQ(Quat& ioChdQ, const int effDx) {
        if (ioChdQ.IsClose(cTurn90DegsAroundYAxis) && 0 != effDx) {
            ioChdQ = (0 > effDx ? cTurnMiniatureAroundYAxis : cTurnNegativeMiniatureAroundYAxis)*ioChdQ; // Turn a little more
        }

        Vec3 qAxis;
        float qAngle;
        ioChdQ.GetAxisAngle(qAxis, qAngle);
        if (0 > qAxis.GetY()) {
            if (0 < effDx || cHalfPI >= qAngle) {
                ioChdQ = cIdentityQ;
            } else {
                ioChdQ = cTurnbackAroundYAxis;
            }
        } else {
            if (0 >= qAngle) {
                ioChdQ = cIdentityQ;
            } else if (JPH_PI <= qAngle) {
                ioChdQ = cTurnbackAroundYAxis;
            }
        }
    }

    inline static void clampChdVel(const CharacterDownsync* nextChd, Vec3& ioVel, const CharacterConfig* cc, const Vec3& groundVel) {
        if (atkedSet.count(nextChd->ch_state()) || noOpSet.count(nextChd->ch_state())) {
            return;
        }

        if (InAirIdle1ByWallJump == nextChd->ch_state()) {
            const float maxVelX = +cc->wall_jump_free_speed();
            const float minVelX = -cc->wall_jump_free_speed();
            if (ioVel.GetX() >= maxVelX) {
                ioVel.SetX(maxVelX);
            } else if (ioVel.GetX() <= minVelX) {
                ioVel.SetX(minVelX);
            }
        } else {
            const float maxVelX = cc->speed() + groundVel.GetX();
            const float minVelX = -cc->speed() + groundVel.GetX();
            if (ioVel.GetX() >= maxVelX) {
                ioVel.SetX(maxVelX);
            } else if (ioVel.GetX() <= minVelX) {
                ioVel.SetX(minVelX);
            }
        }
    }

}; 

#endif
