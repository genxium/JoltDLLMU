#ifndef BASE_BATTLE_COLLISION_FILTER_H_
#define BASE_BATTLE_COLLISION_FILTER_H_ 1

#include "CppOnlyConsts.h"

#include <atomic>
#include <utility> // for "std::pair"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/ContactListener.h>
#include <Jolt/Physics/Character/Character.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Constraints/Constraint.h>
#include <Jolt/Math/Vec3.h>
#include <Jolt/Math/Quat.h>
#include <Jolt/Math/Mat44.h>

#ifndef NDEBUG
#include "DebugLog.h"
#endif

using namespace JPH;

typedef struct InputInducedMotion {
    // "COM" refers to "Center Of Mass"
    Vec3 forceCOM;
    Vec3 torqueCOM;
    Vec3 velCOM; // Only inherited vel of CharacterDownsync, use of skills or impact from opponent bullets will write into "velCOM"
    Vec3 angVelCOM; // Only proactive input (including NPC AI) will write into "angVelCOM" 
    bool jumpTriggered;
    bool slipJumpTriggered;
    bool patternFTriggered;
    bool crouchForcedWhileSupported;
    bool stairsIntended;

    InputInducedMotion() : forceCOM(Vec3::sZero()), torqueCOM(Vec3::sZero()), velCOM(Vec3::sZero()), angVelCOM(Vec3::sZero()), jumpTriggered(false), slipJumpTriggered(false), patternFTriggered(false), crouchForcedWhileSupported(false), stairsIntended(false) {
    }
} InputInducedMotion;

class InputInducedMotionStockCache {
private:
    std::vector<InputInducedMotion> holders;
    atomic<int> cnt;
    int size;

public:
    InputInducedMotionStockCache(const int inSize) {
        size = inSize;
        holders.reserve(inSize);
        for (int i = 0; i < inSize; ++i) {
            holders.push_back(InputInducedMotion());
        }
        cnt = 0;
    }

    InputInducedMotion* Take_ThreadSafe() {
        int idx = cnt.fetch_add(1);
        if (idx >= size) {
            --cnt;
            return nullptr;
        }
        InputInducedMotion* holder = &holders[idx];
        holder->forceCOM.Set(0, 0, 0);
        holder->torqueCOM.Set(0, 0, 0);
        holder->velCOM.Set(0, 0, 0);
        holder->angVelCOM.Set(0, 0, 0);
        holder->jumpTriggered = false;
        holder->slipJumpTriggered = false;
        holder->patternFTriggered = false;
        holder->crouchForcedWhileSupported = false;
        holder->stairsIntended = false;
        return holder;
    }

    void Clear_ThreadSafe() {
        cnt = 0;
    }

    ~InputInducedMotionStockCache() {
        cnt = 0;
        holders.clear();
    }
};

#define TP_COLLIDER_T JPH::Body
typedef struct TrapCacheKey {
    /*
     [REMINDER] 

    Traps are very versatile in "size" and "density" while the "shape type (e.g. box, sphere, cylinder)" of any single trap is derivable from "tpt" by design. 

    This "TrapCacheKey" structure matches the usage pattern of "getOrCreateCachedTrapCollider_NotThreadSafe" for a good reuse rate. 

    Moreover, "motionType", "isSensor" and "objLayer" are fields that we want to keep unchanged once a "JPH::Body" is created -- though "motionType" and "isSensor" can be updated in runtime by "BodyInterface", we found it unnecessary most of the time. 
    */
    uint32_t tpt;
    EMotionType motionType;
    bool isSensor;
    ObjectLayer objLayer;

    TrapCacheKey(const uint32_t inTpt, const EMotionType inMotionType, const bool inIsSensor, const ObjectLayer inObjLayer) : tpt(inTpt), motionType(inMotionType), isSensor(inIsSensor), objLayer(inObjLayer) {}

    bool operator==(const TrapCacheKey& other) const {
        return tpt == other.tpt && motionType == other.motionType && isSensor == other.isSensor && objLayer == other.objLayer;
    }
} TP_CACHE_KEY_T;

typedef struct TrapCacheKeyHasher {
    std::size_t operator()(const TrapCacheKey& v) const {
        std::size_t seed = 4;
        seed ^= std::hash<float>()(v.tpt) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<EMotionType>()(v.motionType) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<bool>()(v.isSensor) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<ObjectLayer>()(v.objLayer) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
} TrapCacheKeyHasher;
#define TP_COLLIDER_Q std::vector<TP_COLLIDER_T*>

#define BL_COLLIDER_T JPH::Body
typedef struct BulletCacheKey {
    // See "TrapCacheKey" for design concerns.

    BulletType bType;
    EMotionType motionType;
    bool isSensor;
    ObjectLayer objLayer;

    BulletCacheKey(const BulletType inBType, const EMotionType inMotionType, const bool inIsSensor, const ObjectLayer inObjLayer) : bType(inBType), motionType(inMotionType), isSensor(inIsSensor), objLayer(inObjLayer) {}

    bool operator==(const BulletCacheKey& other) const {
        return bType == other.bType && motionType == other.motionType && isSensor == other.isSensor && objLayer == other.objLayer;
    }
} BL_CACHE_KEY_T;

typedef struct BulletCacheKeyHasher {
    std::size_t operator()(const BulletCacheKey& v) const {
        std::size_t seed = 4;
        seed ^= std::hash<BulletType>()(v.bType) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<EMotionType>()(v.motionType) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<bool>()(v.isSensor) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<ObjectLayer>()(v.objLayer) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
} BulletCacheKeyHasher;
#define BL_COLLIDER_Q std::vector<BL_COLLIDER_T*>

#define PK_COLLIDER_T JPH::Body
typedef struct PickableCacheKey {
    // See "TrapCacheKey" for design concerns.

    uint32_t pType;
    EMotionType motionType;
    bool isSensor;
    ObjectLayer objLayer;

    PickableCacheKey(const uint32_t inPType, const EMotionType inMotionType, const bool inIsSensor, const ObjectLayer inObjLayer) : pType(inPType), motionType(inMotionType), isSensor(inIsSensor), objLayer(inObjLayer) {}

    bool operator==(const PickableCacheKey& other) const {
        return pType == other.pType && motionType == other.motionType && isSensor == other.isSensor && objLayer == other.objLayer;
    }
} PK_CACHE_KEY_T;

typedef struct PickableCacheKeyHasher {
    std::size_t operator()(const PickableCacheKey& v) const {
        std::size_t seed = 3;
        seed ^= std::hash<uint32_t>()(v.pType) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<EMotionType>()(v.motionType) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<bool>()(v.isSensor) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<ObjectLayer>()(v.objLayer) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
} PickableCacheKeyHasher;
#define PK_COLLIDER_Q std::vector<PK_COLLIDER_T*>

#define TR_COLLIDER_T JPH::Body
typedef struct TriggerCacheKey {
    // See "TrapCacheKey" for design concerns.

    uint32_t trt;
    EMotionType motionType;
    bool isSensor;
    ObjectLayer objLayer;

    TriggerCacheKey(const uint32_t inTrt, const EMotionType inMotionType, const bool inIsSensor, const ObjectLayer inObjLayer) : trt(inTrt), motionType(inMotionType), isSensor(inIsSensor), objLayer(inObjLayer) {}

    bool operator==(const TriggerCacheKey& other) const {
        return trt == other.trt && motionType == other.motionType && isSensor == other.isSensor && objLayer == other.objLayer;
    }
} TR_CACHE_KEY_T;

typedef struct TriggerCacheKeyHasher {
    std::size_t operator()(const TriggerCacheKey& v) const {
        std::size_t seed = 4;
        seed ^= std::hash<float>()(v.trt) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<EMotionType>()(v.motionType) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<bool>()(v.isSensor) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<ObjectLayer>()(v.objLayer) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
} TriggerCacheKeyHasher;
#define TR_COLLIDER_Q std::vector<TR_COLLIDER_T*>

#define CH_COLLIDER_T JPH::Character
typedef struct CharacterCacheKey {
    /*
     [REMINDER] 

    Even a same character of a same "speciesId" can be versatile in "size" and "friction" depending on "CharacterState", while the "shape type (e.g. capsule, compound)" of any single character is derivable from "speciesId" by design. 

    See "TrapCacheKey" for more design concerns.
    */
    uint32_t speciesId;

    CharacterCacheKey(const uint32_t inSpeciesId) : speciesId(inSpeciesId) {}

    bool operator==(const CharacterCacheKey& other) const {
        return speciesId == other.speciesId;
    }
} CH_CACHE_KEY_T;

typedef struct CharacterCacheKeyHasher {
    std::size_t operator()(const CharacterCacheKey& v) const {
        std::size_t seed = 1;
        seed ^= std::hash<uint32_t>()(v.speciesId) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
} CharacterCacheKeyHasher;
#define CH_COLLIDER_Q std::vector<CH_COLLIDER_T*>

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

#define HB_SB_COLLIDER_T JPH::Body
typedef struct HurtboxShieldboxCacheKey {
    uint64_t udt;
    uint32_t speciesId;

    HurtboxShieldboxCacheKey(const uint64_t inUdt, const uint32_t inSpeciesId) : udt(inUdt), speciesId(inSpeciesId) {}

    bool operator==(const HurtboxShieldboxCacheKey& other) const {
        return udt == other.udt && speciesId == other.speciesId;
    }
} HB_SB_CACHE_KEY_T;

typedef struct HbSbCacheKeyHasher {
    std::size_t operator()(const HurtboxShieldboxCacheKey& v) const {
        std::size_t seed = 2;
        seed ^= std::hash<uint64_t>()(v.udt) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<uint32_t>()(v.speciesId) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
} HbSbCacheKeyHasher;
#define HB_SB_COLLIDER_Q std::vector<HB_SB_COLLIDER_T*>

static const float      cHalfPI = 0.5*JPH_PI;
static const JPH::Quat  cIdentityQ = JPH::Quat(0, 0, 0, 1);
static const JPH::Quat  cTurnbackAroundYAxis = JPH::Quat(0, 1, 0, 0);
static const JPH::Quat  cTurnMiniatureAroundYAxis = JPH::Quat::sRotation(Vec3::sAxisY(), JPH_PI/180);
static const JPH::Quat  cTurnNegativeMiniatureAroundYAxis = JPH::Quat::sRotation(Vec3::sAxisY(), -JPH_PI/180);
static const JPH::Quat  cTurn90DegsAroundYAxis = JPH::Quat::sRotation(Vec3::sAxisY(), 0.5f*JPH_PI);
static const JPH::Vec3  cXAxis = JPH::Vec3(1, 0, 0);
static const JPH::Vec3  cYAxis = JPH::Vec3(0, 1, 0);
static const JPH::Vec3  cZAxis = JPH::Vec3(0, 0, 1);
static const JPH::Vec3  cNegativeZAxis = JPH::Vec3(0, 0, -1);

static const JPH::Quat  cTurn90DegsAroundZAxis = JPH::Quat::sRotation(Vec3::sAxisZ(), 0.5f*JPH_PI);
static const JPH::Quat  cTurn180DegsAroundZAxis = JPH::Quat::sRotation(Vec3::sAxisZ(), JPH_PI);

static const JPH::Quat  cTurn45DegsAroundZAxis = JPH::Quat::sRotation(Vec3::sAxisZ(), 0.25f*JPH_PI);
static const JPH::Quat  cTurnNegative45DegsAroundZAxis = JPH::Quat::sRotation(Vec3::sAxisZ(), -0.25f*JPH_PI);
static const JPH::Quat  cTurn135DegsAroundZAxis = JPH::Quat::sRotation(Vec3::sAxisZ(), 0.75f*JPH_PI);
static const JPH::Quat  cTurnNegative135DegsAroundZAxis = JPH::Quat::sRotation(Vec3::sAxisZ(), -0.75f*JPH_PI);

static const JPH::Vec3  yTurned45DegsAroundZAxis = cTurn45DegsAroundZAxis*cYAxis;
static const JPH::Vec3  yTurnedNegative45DegsAroundZAxis = cTurnNegative45DegsAroundZAxis*cYAxis;
static const JPH::Vec3  yTurned135DegsAroundZAxis = cTurn135DegsAroundZAxis*cYAxis;
static const JPH::Vec3  yTurnedNegative135DegsAroundZAxis = cTurnNegative135DegsAroundZAxis*cYAxis;

static const JPH::Mat44 cTurn90DegsAroundZAxisMat = JPH::Mat44::sRotation(cTurn90DegsAroundZAxis);
static const JPH::Mat44 cTurn180DegsAroundZAxisMat = JPH::Mat44::sRotation(cTurn180DegsAroundZAxis);

class BaseBattleCollisionFilter {
public:
    std::atomic<uint32_t>       mNextRdfBulletIdCounter = 0;
    std::atomic<uint32_t>       mNextRdfBulletCount = 0;

    std::atomic<uint32_t>       mNextRdfNpcIdCounter = 0;
    std::atomic<uint32_t>       mNextRdfNpcCount = 0;

    std::atomic<uint32_t>       mNextRdfPickableIdCounter = 0;
    std::atomic<uint32_t>       mNextRdfPickableCount = 0;

    std::atomic<uint32_t>       mNextRdfTriggerCount = 0;

    std::atomic<uint32_t>       mNextRdfDynamicTrapCount = 0;

    std::atomic<uint32_t>       mNextRdfAimingRayCount = 0;

    virtual bool shouldCharacterSeeTrap(const uint64_t udLhs, const uint64_t udtLhs, const CharacterDownsync* lhsCurrChd, const uint64_t udRhs, const Body& rhs) const = 0;

    virtual JPH::ValidateResult validateLhsCharacterContact(const CharacterDownsync* lhsCurrChd, const Trigger* rhsCurrTrigger) const = 0;

    virtual JPH::ValidateResult validateLhsCharacterContact(const CharacterDownsync* lhsCurrChd, const Trap* rhsCurrTrap) const = 0;

    virtual ValidateResult validateLhsCharacterContact(const CharacterDownsync* lhsCurrChd, const CharacterDownsync* rhsCurrChd) const = 0;

    virtual ValidateResult validateLhsCharacterContact(const CharacterDownsync* lhsCurrChd, const Bullet* rhsCurrBl) const = 0;

    virtual ValidateResult validateLhsCharacterContact(const uint64_t udLhs, const uint64_t udtLhs,
        const AABox* lhsAABB,
        const CharacterDownsync* lhsCurrChd, const CharacterDownsync* lhsNextChd, const uint64_t udRhs, const uint64_t udtRhs, const Body& rhs) const = 0;

    virtual ValidateResult validateLhsCharacterContact(const uint64_t udLhs, const uint64_t udtLhs,
        const Body& lhs, // the "Character"
        const uint64_t udRhs, const uint64_t udtRhs, const Body& rhs) const = 0;

    virtual ValidateResult validateLhsCharacterAimingRayContact(const uint64_t udtLhs, const CharacterDownsync* lhsCurrChd, const CharacterDownsync* lhsNextChd, const uint64_t udRhs, const uint64_t udtRhs, const Body& rhs) const = 0;

    virtual ValidateResult validateLhsBulletContact(const Bullet* lhsCurrBl, const uint64_t udRhs, const uint64_t udtRhs, const Body& rhs) const = 0;

    virtual ValidateResult validateLhsBulletContact(const uint64_t udLhs,
        const Body& lhs, // the "Bullet"
        const uint64_t udRhs, const uint64_t udtRhs, const Body& rhs) const = 0;

    virtual RVec3 getColliderPositionByUd(const uint64_t ud, const BodyInterface* bi) const = 0;

    virtual const CharacterDownsync* immutableCurrChdPtrFromUd(uint64_t ud) const = 0;
    virtual const CharacterDownsync& immutableCurrChdFromUd(uint64_t ud) const = 0;
    virtual CharacterDownsync* mutableNextChdFromUd(uint64_t ud) const = 0;

    virtual const CharacterDownsync& immutableCurrChdFromUd(uint64_t udt, uint64_t ud) const = 0;
    virtual const CharacterDownsync* immutableCurrChdPtrFromUd(uint64_t udt, uint64_t ud) const = 0;
    virtual CharacterDownsync* mutableNextChdFromUd(uint64_t udt, uint64_t ud) const = 0;
    
    virtual ~BaseBattleCollisionFilter() {

    }

    virtual float calcTerrainPriority(const uint64_t ud) const = 0;

    inline const uint64_t calcPublishingToTriggerUd(const NpcCharacterDownsync& npcChd) {
        return calcTriggerUserData(npcChd.publishing_to_trigger_id_upon_exhausted());
    }

    inline const uint64_t calcPublishingToTriggerUd(const TriggerConfigFromTiled& triggerConfigFromTiled) {
        return calcTriggerUserData(triggerConfigFromTiled.publishing_to_trigger_id_upon_exhausted());
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

    inline static const uint64_t calcPlayerHurtboxUserData(const uint32_t joinIndex, const uint32_t hbIdx) {
        return UDT_PLAYER_HURTBOX + (joinIndex << UD_PAYLOAD_HB_SB_IDX_SHIFT) + hbIdx;
    }

    inline static const uint64_t calcPlayerShieldboxUserData(const uint32_t joinIndex, const uint32_t sbIdx) {
        return UDT_PLAYER_SHIELDBOX + (joinIndex << UD_PAYLOAD_HB_SB_IDX_SHIFT) + sbIdx;
    }

    inline static const uint64_t calcNpcHurtboxUserData(const uint32_t npcId, const uint32_t hbIdx) {
        return UDT_NPC_HURTBOX + (npcId << UD_PAYLOAD_HB_SB_IDX_SHIFT) + hbIdx;
    }

    inline static const uint64_t calcNpcShieldboxUserData(const uint32_t npcId, const uint32_t sbIdx) {
        return UDT_NPC_SHIELDBOX + (npcId << UD_PAYLOAD_HB_SB_IDX_SHIFT) + sbIdx;
    }

    inline static const bool calcCharacterUserDataFromHbSbUd(const uint64_t hbSbUd, uint64_t& outUdt, uint64_t& outUd) {
        uint64_t origUdt = getUDT(hbSbUd);
        uint64_t origUdPayload = getUDPayload(hbSbUd);

        outUdt = 0;
        outUd = 0;

        switch (origUdt) {
        case UDT_PLAYER_HURTBOX:
        case UDT_PLAYER_SHIELDBOX:
            outUdt = UDT_PLAYER;
            outUd = calcPlayerUserData(origUdPayload >> UD_PAYLOAD_HB_SB_IDX_SHIFT); 
            return true;
        case UDT_NPC_HURTBOX:
        case UDT_NPC_SHIELDBOX:
            outUdt = UDT_NPC;
            outUd = calcNpcUserData(origUdPayload >> UD_PAYLOAD_HB_SB_IDX_SHIFT); 
            return true;
        default:
            return false;
        }
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
        return encodeInput(ifDecoded.dx(), ifDecoded.dy(), ifDecoded.btn_a_level(), ifDecoded.btn_b_level(), ifDecoded.btn_c_level(), ifDecoded.btn_d_level(), ifDecoded.btn_e_level(), ifDecoded.btn_f_level(), ifDecoded.btn_l_level(), ifDecoded.btn_r_level());
    }

    inline static uint64_t encodeInput(const int dx, const int dy, const uint64_t btnALevel, const uint64_t btnBLevel, const uint64_t btnCLevel, const uint64_t btnDLevel, const uint64_t btnELevel, const uint64_t btnFLevel, const uint64_t btnLLevel, const uint64_t btnRLevel) {
        uint64_t encodedBtnALevel = (btnALevel << 4);
        uint64_t encodedBtnBLevel = (btnBLevel << 5);
        uint64_t encodedBtnCLevel = (btnCLevel << 6);
        uint64_t encodedBtnDLevel = (btnDLevel << 7);
        uint64_t encodedBtnELevel = (btnELevel << 8);
        uint64_t encodedBtnFLevel = (btnFLevel << 9);
        uint64_t encodedBtnLLevel = (btnFLevel << 10);
        uint64_t encodedBtnRLevel = (btnFLevel << 11);
        uint64_t discretizedDir = encodeDir(dx, dy);
        return (discretizedDir + encodedBtnALevel + encodedBtnBLevel + encodedBtnCLevel + encodedBtnDLevel + encodedBtnELevel + encodedBtnFLevel + encodedBtnLLevel + encodedBtnRLevel);
    }

    inline static bool decodeInput(uint64_t encodedInput, InputFrameDecoded* holder) {
        holder->Clear();
        int encodedDirection = (int)(encodedInput & 15);
        uint64_t btnALevel = ((encodedInput >> 4) & 1);
        uint64_t btnBLevel = ((encodedInput >> 5) & 1);
        uint64_t btnCLevel = ((encodedInput >> 6) & 1);
        uint64_t btnDLevel = ((encodedInput >> 7) & 1);
        uint64_t btnELevel = ((encodedInput >> 8) & 1);
        uint64_t btnFLevel = ((encodedInput >> 9) & 1);
        uint64_t btnLLevel = ((encodedInput >> 10) & 1);
        uint64_t btnRLevel = ((encodedInput >> 11) & 1);

        holder->set_dx(DIRECTION_DECODER[encodedDirection][0]);
        holder->set_dy(DIRECTION_DECODER[encodedDirection][1]);
        holder->set_btn_a_level(btnALevel);
        holder->set_btn_b_level(btnBLevel);
        holder->set_btn_c_level(btnCLevel);
        holder->set_btn_d_level(btnDLevel);
        holder->set_btn_e_level(btnELevel);
        holder->set_btn_f_level(btnFLevel);
        holder->set_btn_l_level(btnLLevel);
        holder->set_btn_r_level(btnRLevel);
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

    inline static bool IsAngleNearZero(float angle) {
        return -cAngleEps < angle && angle < cAngleEps;
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
        Vec3 outFacingRaw = outQ*cXAxis;
        float outFacingRawProjX = outFacingRaw.Dot(cXAxis);
        JPH_ASSERT(0 != outFacingRawProjX); // Guaranteed by "clampChdQ"
        float outFacingX = 0 < outFacingRawProjX ? +1 : -1; 
        outFacing.Set(outFacingX, 0, 0); 
    }

    inline static void calcQFacing(const Bullet& currBl, const Quat& inQ, Vec3& outFacing) {
        Vec3 facingRaw = inQ*cXAxis;
        float facingRawProjX = facingRaw.Dot(cXAxis);
        JPH_ASSERT(0 != facingRawProjX); // Guaranteed by "clampChdQ"
        float outFacingX = 0 < facingRawProjX ? +1 : -1;
        outFacing.Set(outFacingX, 0, 0);
    }

    inline void clampChdQ(Quat& ioChdQ, const int effDx) {
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

    inline void clampRotatedXAxisToXOYPlane(Quat& ioQ) {
        const JPH::Vec3 swungXAxis = ioQ*cXAxis;
        if (0 == swungXAxis.GetZ()) {
            // To save computational cost, most cases should return here.
            return;
        }
        const JPH::Quat qTwist = ioQ.GetTwist(cXAxis);
        const JPH::Quat qSwing = ioQ * qTwist.Conjugated();
        const JPH::Vec3 projectedXAxis(swungXAxis.GetX(), swungXAxis.GetY(), 0.0f);
        const JPH::Quat constrainedSwing = JPH::Quat::sFromTo(cXAxis, projectedXAxis.Normalized()); // [REMINDER] This is relatively expensive.
        ioQ = constrainedSwing * qTwist;
    }

    virtual bool isChdVelClampable(const CharacterDownsync* chd) = 0;
    virtual void clampChdVel(const CharacterDownsync* nextChd, Vec3& ioVel, const CharacterConfig* cc, const Vec3& groundVel) = 0;
    virtual void clampFlyingChdVel(const CharacterDownsync* nextChd, Vec3& ioVel, const CharacterConfig* cc) = 0;

    inline static int EncodePatternForCancelTransit(int patternId, bool currEffInAir, bool currCrouching, bool currOnWall, bool currDashing, bool currWalking) {
        /*
        For simplicity,
        - "currSliding" = "currCrouching" + "currDashing"
        */
        int encodedPatternId = patternId;
        if (currEffInAir) {
            encodedPatternId += (1 << 16);
        }
        if (currCrouching) {
            encodedPatternId += (1 << 17);
        }
        if (currOnWall) {
            encodedPatternId += (1 << 18);
        }
        if (currDashing) {
            encodedPatternId += (1 << 19);
        }
        if (currWalking) {
            encodedPatternId += (1 << 20);
        }
        return encodedPatternId;
    }

    inline static int EncodePatternForInitSkill(int patternId, bool currEffInAir, bool currCrouching, bool currOnWall, bool currDashing, bool currWalking, bool currInBlockStun, bool currAtked, bool currParalyzed) {
        int encodedPatternId = EncodePatternForCancelTransit(patternId, currEffInAir, currCrouching, currOnWall, currDashing, currWalking);
        if (currInBlockStun) {
            encodedPatternId += (1 << 21);
        }
        if (currAtked) {
            encodedPatternId += (1 << 22);
        }
        if (currParalyzed) {
            encodedPatternId += (1 << 23);
        }
        return encodedPatternId;
    }
}; 

#endif
