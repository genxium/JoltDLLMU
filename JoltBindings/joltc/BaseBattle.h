#ifndef BASE_BATTLE_H_
#define BASE_BATTLE_H_ 1

#include "BaseBattleCollisionFilter.h"
#include "FrameRingBuffer.h"
#include "CollisionLayers.h"
#include "CollisionCallbacks.h"
#include <Jolt//Physics/Body/BodyLockInterface.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Core/JobSystemThreadPool.h>

#include <vector>
#include <map>
#include <set>
#include <deque>
#include <google/protobuf/arena.h>

/*
[REMINDER] 

It's by design that "JPH::Character" instead of "JPH::CharacterVirtual" is used here while [their differences]("https://jrouwe.github.io/JoltPhysics/index.html#character-controllers) are understood.

The lack of use of broadphase makes "JPH::CharacterVirtual" very inefficient in "Character v.s. Character" collision handling. See [CharacterVsCharacterCollisionSimple](https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Physics/Character/CharacterVirtual.cpp#L34) for its official default implementation.

My current choice is to
- use "JPH::Character" for efficient "(Regular)Constraint & ContactConstraint" calculation in "JPH::PhysicsSystem" along with other "JPH:Body"s, then 
- use "NarrowPhase" collision detection to handle application specific state transitions.
*/

typedef struct VectorFloatHasher {
    std::size_t operator()(const std::vector<float>& v) const {
        std::size_t seed = v.size(); // Start with the size of the vector
        for (float i : v) {
            seed ^= std::hash<float>()(i) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }
} VectorFloatHasher;

using ContactPoints = StaticArray<Vec3, 64>;
class CollisionUdHolder_ThreadSafe {
private:
    std::vector<uint64_t> uds; 
    std::vector<ContactPoints> contactPointsPerUd;
    atomic<int> cnt;
    int size;

    std::unordered_set<uint64_t> seenUdsDuringReading_NotThreadSafe;

public:
    CollisionUdHolder_ThreadSafe(const int inSize) {
        size = inSize;
        uds.reserve(inSize);
        contactPointsPerUd.reserve(inSize);
        for (int i = 0; i < inSize; ++i) {
            uds.push_back(0);
            contactPointsPerUd.push_back(ContactPoints());
        }
        cnt = 0;
    }

    // [WARNING] All fields, including "contactPointsPerUd", will be destructed implicitly when "delete holder" is called in "~CollisionUdHolderStockCache_ThreadSafe". See https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Core/StaticArray.h#L44 for more information.

    bool Add_ThreadSafe(const uint64_t inUd, const ContactPoints& inContactPoints) {
        int idx = cnt.fetch_add(1);
        if (idx >= size) {
            --cnt;
            return false;
        }
        uds[idx] = inUd;
        contactPointsPerUd[idx] = inContactPoints; // It does work this way, see https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Core/StaticArray.h#L225.
        return true;
    }


    int GetCnt_Realtime() const {
        return cnt;
    }

    bool GetUd_NotThreadSafe(const int idx, uint64_t& outUd, ContactPoints& outContactPoints) {
        uint64_t cand = uds[idx];
        if (seenUdsDuringReading_NotThreadSafe.count(cand)) return false;
        outUd = cand;
        outContactPoints = contactPointsPerUd[idx];
        seenUdsDuringReading_NotThreadSafe.insert(cand);
        return true;
    }

    void Clear_ThreadSafe() {
        cnt = 0;
        seenUdsDuringReading_NotThreadSafe.clear();
    }
};

class CollisionUdHolderStockCache_ThreadSafe {
private:
    std::vector<CollisionUdHolder_ThreadSafe*> holders; 
    atomic<int> cnt;
    int size;

public:
    CollisionUdHolderStockCache_ThreadSafe(const int inSize, const int holderSize) {
        size = inSize;
        holders.reserve(inSize);
        for (int i = 0; i < inSize; ++i) {
            holders.push_back(new CollisionUdHolder_ThreadSafe(holderSize));
        }
        cnt = 0;
    }

    CollisionUdHolder_ThreadSafe* Take_ThreadSafe() {
        int idx = cnt.fetch_add(1);
        if (idx >= size) {
            --cnt;
            return nullptr;
        }
        CollisionUdHolder_ThreadSafe* holder = holders[idx];   
        holder->Clear_ThreadSafe();
        return holder; 
    }

    void Clear_ThreadSafe() {
        cnt = 0;
    }

    ~CollisionUdHolderStockCache_ThreadSafe() {
        cnt = 0;
        while (!holders.empty()) {
            CollisionUdHolder_ThreadSafe* holder = holders.back();
            holders.pop_back();
            delete holder;
        }
    }
};

typedef struct InputInducedMotion {
    // "COM" refers to "Center Of Mass"
    Vec3 forceCOM;  
    Vec3 torqueCOM;
    Vec3 velCOM; // Only inherited vel of CharacterDownsync, use of skills or impact from opponent bullets will write into "velCOM"
    Vec3 angVelCOM; // Only proactive input (including NPC AI) will write into "angVelCOM" 
    bool jumpTriggered;
    bool slipJumpTriggered;

    InputInducedMotion() : forceCOM(Vec3::sZero()), torqueCOM(Vec3::sZero()), velCOM(Vec3::sZero()), angVelCOM(Vec3::sZero()), jumpTriggered(false), slipJumpTriggered(false) {
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

// All Jolt symbols are in the JPH namespace
using namespace JPH;
using namespace jtshared;

class JOLTC_EXPORT BaseBattle : public JPH::ContactListener, public BaseBattleCollisionFilter {
public:
    BaseBattle(int renderBufferSize, int inputBufferSize, TempAllocator* inGlobalTempAllocator);
    virtual ~BaseBattle();

public:
    google::protobuf::Arena pbTempAllocator;
    TempAllocator* globalTempAllocator;
    bool frameLogEnabled = false;
    int playersCnt;
    uint64_t allConfirmedMask;
    atomic<uint64_t> inactiveJoinMask; // realtime information
    int battleDurationFrames;
    FrameRingBuffer<RenderFrame> rdfBuffer;
    FrameRingBuffer<InputFrameDownsync> ifdBuffer;
    FrameRingBuffer<FrameLog> frameLogBuffer;

    std::vector<uint64_t> prefabbedInputList;
    std::vector<int> playerInputFrontIds;
    std::multiset<int> playerInputFrontIdsSorted;
    std::vector<uint64_t> playerInputFronts;

    /*
    [WARNING/BACKEND] At any point of time it's maintained that "lcacIfdId + 1 >= ifdBuffer.StFrameId", i.e. if "StFrameId eviction upon DryPut() of ifdBuffer" occurs, then "lcacIfdId" should also be incremented (along with "currDynamicsRdfId").

    A known impact of this design is that we MUST de-couple the use of "DownsyncSnapshot.ref_rdf" from "DownsyncSnapshot.ifd_batch" for frontend (e.g. upon player re-join or regular refRdf broadcast) because "ifdBuffer.GetByFrameId(lcacIfdId)" might be null.

    Moreover, it's INTENTIONALLY DESIGNED NOT to maintain "lcacIfdId >= ifdBuffer.StFrameId" because in the extreme case of "StFrameId eviction upon DryPut()" we COULDN'T PREDICT when "postEvictionStFrameId" becomes all-confirmed. See codes of "BackendBattle::OnUpsyncSnapshotReceived(...)" for details.
    */
    /*
       [WARNING/FRONTEND] At any point of time it's maintained that "lcacIfdId + 1 >= ifdBuffer.StFrameId", i.e. if "StFrameId eviction upon DryPut() of ifdBuffer" occurs, then "lcacIfdId" should also be incremented, but NOT along with anything else -- it's much simpler than the backend counterpart because we don't produce "DownsyncSnapshot" and the handling of "DownsyncSnapshot.ref_rdf & DownsyncSnapshot.ifd_batch" are now de-coupled on frontend.
    */
    int lcacIfdId = -1; // short for "last consecutively all confirmed IfdId"

    BPLayerInterfaceImpl bpLayerInterface;
    ObjectVsBroadPhaseLayerFilterImpl ovbLayerFilter;
    ObjectLayerPairFilterImpl ovoLayerFilter;
    MyBodyActivationListener bodyActivationListener;
    MyContactListener contactListener;
    PhysicsSystem* phySys;
    Vec3 antiGravityNorm;
    float gravityMagnitude;
    BodyInterface* bi;
    BodyInterface* biNoLock;
    const NarrowPhaseQuery* narrowPhaseQueryNoLock;
    JobSystemThreadPool* jobSys;
    DefaultBroadPhaseLayerFilter defaultBplf; 
    DefaultObjectLayerFilter defaultOlf;

    /*
    [WARNING] Unlike "AbstractCacheableAnimNodeTemplate", the "activeXxx & cachedXxx" have little to none shared features compared to their GUI counterparts (e.g. handling of "cachedChColliders" is significantly different from that of "cachedTpColliders", and handling of "activeNonContactConstraints & cachedNonContactConstraints" is totally different from all the others), hence NOT suitable for abstracting a shared interface or wrapper class -- at least by the time of writing. 
    */

    /////////////////////////////////////////////////////Bullet Collider Cache/////////////////////////////////////////////////////
    BL_COLLIDER_Q activeBlColliders;
    BL_COLLIDER_Q* blStockCache;
    std::unordered_map< BL_CACHE_KEY_T, BL_COLLIDER_Q, VectorFloatHasher > cachedBlColliders; // Key is "{(default state) halfExtent}", where "convexRadius" is determined by "halfExtent"

    /////////////////////////////////////////////////////Character Collider Cache/////////////////////////////////////////////////////
    /*
    [WARNING] The use of "activeChColliders" in "BaseBattle::batchRemoveFromPhySysAndCache" is order-sensitive, i.e. the removal order of "activeChColliders" MUST BE "reverse-order w.r.t. BaseBattle::batchPutIntoPhySysFromCache" (including Player and Npc), any other removal order there results in failure of "FrontendTest/runTestCase11" when checking rollback-chasing alignment of characters. 

     While "reverse-order w.r.t. BaseBattle::batchPutIntoPhySysFromCache" being correct might be a coincidence of matching some order-sensitive mechanism in JoltPhysics/PhysicsSystem-Character setters, by the time of writing the cause of misalignment when using other removal orders is yet UNKNOWN. 
    */
    CH_COLLIDER_Q activeChColliders;
    /*
     [TODO]

     Make "cachedChColliders" keyed by "CharacterConfig.species_id()" to fit the need of multi-shape character (at different ch_state).

    It's by design that "ScaledShape" is NOT used here, because when mixed with translation and rotation, the order of affine transforms matters but is difficult to keep in mind.

    Moreover, by using this approach to manage multi-shape character I dropped the "shared shapes across bodies" feature of Jolt.
    */
    std::unordered_map< CH_CACHE_KEY_T, CH_COLLIDER_Q, VectorFloatHasher > cachedChColliders; // Key is "{(default state) radius, halfHeight}", kindly note that position and orientation of "Character" are mutable during reuse, thus not using "RefConst<>".

    /////////////////////////////////////////////////////Trap Collider Cache/////////////////////////////////////////////////////
    TP_COLLIDER_Q activeTpColliders;
    TP_COLLIDER_Q* tpDynamicStockCache;    // (Dynamic,   MyObjectLayers::MOVING)
    TP_COLLIDER_Q* tpKinematicStockCache;  // (Kinematic, MyObjectLayers::MOVING)
    TP_COLLIDER_Q* tpObsIfaceStockCache;   // (Dynamic,   MyObjectLayers::TRAP_OBSTACLE_INTERFACE)
    TP_COLLIDER_Q* tpHelperStockCache;     // (Static,    MyObjectLayers::TRAP_HELPER)
    std::unordered_map< TP_CACHE_KEY_T, TP_COLLIDER_Q, TrapCacheKeyHasher > cachedTpColliders;

    /////////////////////////////////////////////////////Trigger Collider Cache/////////////////////////////////////////////////////
    /////////////////////////////////////////////////////Pickable Collider Cache/////////////////////////////////////////////////////
    /*
    [WARNING] By the time of writing, it's by design that there's no "activeTrColliders/cachedTrColliders" or "activePkColliders/cachedPkColliders" with reasons.
    - Not all triggers have geometric shapes, and for those who do, we use "multithreaded-NarrowPhaseQuery + custom collector for min/max recognition" to handle their collisions (just like "vision handling"), because "who best hits the trigger" is a critical information that must be deterministic in rollback netcode.  
    - Similarly "who best picks the pickable" is a critical information that must be deterministic in rollback netcode too.  
    */

    /////////////////////////////////////////////////////NonContactConstraint Cache/////////////////////////////////////////////////////
    NON_CONTACT_CONSTRAINT_Q activeNonContactConstraints;
    std::unordered_map< NON_CONTACT_CONSTRAINT_CACHE_KEY_T, NON_CONTACT_CONSTRAINT_Q, NonContactConstraintCacheKeyHasher > cachedNonContactConstraints;

public:
    static void FindTrapConfig(const uint32_t trapSpeciesId, const uint32_t trapId, const std::unordered_map<uint32_t, const TrapConfigFromTiled*> inTrapConfigFromTileDict, const TrapConfig*& outTpConfig, const TrapConfigFromTiled*& outTpConfigFromTiled);

    static void FindBulletConfig(const uint32_t skillId, const uint32_t skillHit, const Skill*& outSkill, const BulletConfig*& outBulletConfig);

    inline static int ConvertToIfdId(int rdfId, int delayRdfCnt) {
        if (rdfId < delayRdfCnt) {
            return 0;
        }
        return ((rdfId - delayRdfCnt) >> globalPrimitiveConsts->input_scale_frames());
    }

    inline static int ConvertToGeneratingIfdId(int renderFrameId, int localExtraInputDelayFrames = 0) {
        return ConvertToIfdId(renderFrameId, -localExtraInputDelayFrames);
    }

    inline static int ConvertToDelayedInputFrameId(int renderFrameId) {
        return ConvertToIfdId(renderFrameId, globalPrimitiveConsts->input_delay_frames());
    }

    inline static int ConvertToFirstUsedRenderFrameId(int inputFrameId) {
        return ((inputFrameId << globalPrimitiveConsts->input_scale_frames()) + globalPrimitiveConsts->input_delay_frames());
    }

    inline static int ConvertToLastUsedRenderFrameId(int inputFrameId) {
        return ((inputFrameId << globalPrimitiveConsts->input_scale_frames()) + globalPrimitiveConsts->input_delay_frames() + (1 << globalPrimitiveConsts->input_scale_frames()) - 1);
    }

    inline uint64_t SetPlayerActive(uint32_t joinIndex) {
        auto oldVal = inactiveJoinMask.fetch_and(allConfirmedMask ^ CalcJoinIndexMask(joinIndex));
        return inactiveJoinMask;
    }

    inline uint64_t SetPlayerInactive(uint32_t joinIndex) {
        auto oldVal = inactiveJoinMask.fetch_or(CalcJoinIndexMask(joinIndex));
        return inactiveJoinMask;
    }

    inline uint64_t GetInactiveJoinMask() {
        return inactiveJoinMask;
    }

    inline uint64_t SetInactiveJoinMask(uint64_t value) {
        auto oldVal = inactiveJoinMask.exchange(value);
        return oldVal;
    }

    inline bool SetFrameLogEnabled(bool val) {
        bool oldVal = frameLogEnabled;
        frameLogEnabled = val;
        return oldVal;
    }

    void updateChColliderBeforePhysicsUpdate_ThreadSafe(uint64_t ud, CH_COLLIDER_T* chCollider, const float dt, const CharacterDownsync& currChd, const InputInducedMotion* inInputInducedMotion, const bool inGravityDirty, const bool inFrictionDirty);

    virtual RenderFrame* CalcSingleStep(const int currRdfId, int delayedIfdId, InputFrameDownsync* delayedIfd);
    
    virtual void Clear();

    virtual bool ResetStartRdf(char* inBytes, int inBytesCnt);
    virtual bool ResetStartRdf(WsReq* initializerMapData);

    virtual bool initializeTriggerDemandedEvtMask(RenderFrame* startRdf);

    static void NewPreallocatedBullet(Bullet* single, int bulletLocalId, int originatedRenderFrameId, int teamId, BulletState blState, int framesInBlState);
    static void NewPreallocatedCharacterDownsync(CharacterDownsync* single, int buffCapacity, int debuffCapacity, int inventoryCapacity, int bulletImmuneRecordCapacity);
    static void NewPreallocatedPlayerCharacterDownsync(PlayerCharacterDownsync* single, int buffCapacity, int debuffCapacity, int inventoryCapacity, int bulletImmuneRecordCapacity);
    static void NewPreallocatedNpcCharacterDownsync(NpcCharacterDownsync* single, int buffCapacity, int debuffCapacity, int inventoryCapacity, int bulletImmuneRecordCapacity);
    static RenderFrame* NewPreallocatedRdf(int roomCapacity, int preallocNpcCount, int preallocBulletCount);

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

    inline static void AssertNearlySame(const RenderFrame* lhs, const RenderFrame* rhs) {
        JPH_ASSERT(lhs->players_size() == rhs->players_size());
        for (int i = 0; i < lhs->players_size(); i++) {
            auto lhsCh = lhs->players(i);
            auto rhsCh = rhs->players(i);
            BaseBattle::AssertNearlySame(lhsCh, rhsCh);
        }
        JPH_ASSERT(lhs->npcs_size() == rhs->npcs_size());
        JPH_ASSERT(lhs->npc_count() == rhs->npc_count());
        JPH_ASSERT(lhs->npc_id_counter() == rhs->npc_id_counter());
        for (int i = 0; i < lhs->npcs_size(); i++) {
            auto lhsCh = lhs->npcs(i);
            if (globalPrimitiveConsts->terminating_character_id() == lhsCh.id()) break;
            auto rhsCh = rhs->npcs(i);
            BaseBattle::AssertNearlySame(lhsCh, rhsCh);
        }
        JPH_ASSERT(lhs->bullets_size() == rhs->bullets_size());
        JPH_ASSERT(lhs->bullet_id_counter() == rhs->bullet_id_counter());
        JPH_ASSERT(lhs->bullet_count() == rhs->bullet_count());
        for (int i = 0; i < lhs->bullets_size(); i++) {
            auto lhsB = lhs->bullets(i);
            if (globalPrimitiveConsts->terminating_bullet_id() == lhsB.id()) break;
            auto rhsB = rhs->bullets(i);
            BaseBattle::AssertNearlySame(lhsB, rhsB);
        }

        JPH_ASSERT(lhs->dynamic_traps_size() == rhs->dynamic_traps_size());
        JPH_ASSERT(lhs->dynamic_trap_count() == rhs->dynamic_trap_count());
        for (int i = 0; i < lhs->dynamic_traps_size(); i++) {
            auto lhsTp = lhs->dynamic_traps(i);
            if (globalPrimitiveConsts->terminating_trap_id() == lhsTp.id()) break;
            auto rhsTp = rhs->dynamic_traps(i);
            BaseBattle::AssertNearlySame(lhsTp, rhsTp);
        }

    }

    inline static void AssertNearlySame(const PlayerCharacterDownsync& lhs, const PlayerCharacterDownsync& rhs) {
        auto lhsChd = lhs.chd();
        auto rhsChd = rhs.chd();
        AssertNearlySame(lhsChd, rhsChd);
    }

    inline static void AssertNearlySame(const NpcCharacterDownsync& lhs, const NpcCharacterDownsync& rhs) {
        JPH_ASSERT(lhs.id() == rhs.id());
        auto lhsChd = lhs.chd();
        auto rhsChd = rhs.chd();
        AssertNearlySame(lhsChd, rhsChd);
    }

    inline static void AssertNearlySame(const CharacterDownsync& lhs, const CharacterDownsync& rhs) {
        JPH_ASSERT(lhs.ch_state() == rhs.ch_state());
        JPH_ASSERT(lhs.frames_in_ch_state() == rhs.frames_in_ch_state());
        JPH_ASSERT(lhs.ground_ud() == rhs.ground_ud());
        JPH_ASSERT(lhs.btn_a_holding_rdf_cnt() == rhs.btn_a_holding_rdf_cnt());
        JPH_ASSERT(lhs.btn_b_holding_rdf_cnt() == rhs.btn_b_holding_rdf_cnt());
        JPH_ASSERT(lhs.btn_c_holding_rdf_cnt() == rhs.btn_c_holding_rdf_cnt());
        JPH_ASSERT(lhs.btn_d_holding_rdf_cnt() == rhs.btn_d_holding_rdf_cnt());
        JPH_ASSERT(lhs.btn_e_holding_rdf_cnt() == rhs.btn_e_holding_rdf_cnt());
        JPH_ASSERT(lhs.frames_invinsible() == rhs.frames_invinsible());
        JPH_ASSERT(lhs.frames_to_recover() == rhs.frames_to_recover());
        JPH_ASSERT(lhs.lower_part_rdf_cnt() == rhs.lower_part_rdf_cnt());
        JPH_ASSERT(lhs.walkstopping_rdf_countdown() == rhs.walkstopping_rdf_countdown());
        JPH_ASSERT(lhs.fallstopping_rdf_countdown() == rhs.fallstopping_rdf_countdown());
        JPH_ASSERT(lhs.remaining_air_dash_quota() == rhs.remaining_air_dash_quota());
        JPH_ASSERT(lhs.remaining_air_jump_quota() == rhs.remaining_air_jump_quota());
        JPH_ASSERT(lhs.remaining_def1_quota() == rhs.remaining_def1_quota());
        JPH_ASSERT(isNearlySame(lhs.vel_x(), lhs.vel_y(), lhs.vel_z(), rhs.vel_x(), rhs.vel_y(), rhs.vel_z()));
        JPH_ASSERT(isNearlySame(lhs.ground_vel_x(), lhs.ground_vel_y(), lhs.ground_vel_z(), rhs.ground_vel_x(), rhs.ground_vel_y(), rhs.ground_vel_z()));
        JPH_ASSERT(isNearlySame(lhs.x(), lhs.y(), lhs.z(), rhs.x(), rhs.y(), rhs.z()));
        JPH::Quat lhsQ(lhs.q_x(), lhs.q_y(), lhs.q_z(), lhs.q_w()), rhsQ(rhs.q_x(), rhs.q_y(), rhs.q_z(), rhs.q_w());
        JPH_ASSERT(lhsQ.IsClose(rhsQ));
    }

    inline static void AssertNearlySame(const Bullet& lhs, const Bullet& rhs) {
        JPH_ASSERT(lhs.bl_state() == rhs.bl_state());
        JPH_ASSERT(lhs.frames_in_bl_state() == rhs.frames_in_bl_state());
        JPH_ASSERT(lhs.ud() == rhs.ud());
        JPH_ASSERT(lhs.originated_render_frame_id() == rhs.originated_render_frame_id());
        JPH_ASSERT(lhs.offender_ud() == rhs.offender_ud());
        JPH_ASSERT(isNearlySame(lhs.x(), lhs.y(), lhs.z(), rhs.x(), rhs.y(), rhs.z()));
        JPH_ASSERT(isNearlySame(lhs.originated_x(), lhs.originated_y(), lhs.originated_z(), rhs.originated_x(), rhs.originated_y(), rhs.originated_z()));
        JPH_ASSERT(isNearlySame(lhs.ground_vel_x(), lhs.ground_vel_y(), lhs.ground_vel_z(), rhs.ground_vel_x(), rhs.ground_vel_y(), rhs.ground_vel_z()));
        JPH_ASSERT(isNearlySame(lhs.vel_x(), lhs.vel_y(), lhs.vel_z(), rhs.vel_x(), rhs.vel_y(), rhs.vel_z()));
        JPH::Quat lhsQ(lhs.q_x(), lhs.q_y(), lhs.q_z(), lhs.q_w()), rhsQ(rhs.q_x(), rhs.q_y(), rhs.q_z(), rhs.q_w());
        JPH_ASSERT(lhsQ.IsClose(rhsQ));
        JPH_ASSERT(lhs.repeat_quota_left() == rhs.repeat_quota_left());
        JPH_ASSERT(lhs.remaining_hard_pushback_bounce_quota() == rhs.remaining_hard_pushback_bounce_quota());
        JPH_ASSERT(lhs.target_ud() == rhs.target_ud());
        JPH_ASSERT(lhs.damage_dealed() == rhs.damage_dealed());

        JPH_ASSERT(lhs.hit_on_ifc() == rhs.hit_on_ifc());
        JPH_ASSERT(lhs.active_skill_hit() == rhs.active_skill_hit());
        JPH_ASSERT(lhs.skill_id() == rhs.skill_id());
        JPH_ASSERT(lhs.id() == rhs.id());
        JPH_ASSERT(lhs.team_id() == rhs.team_id());
    }

    inline static void AssertNearlySame(const Trap& lhs, const Trap& rhs) {
        JPH_ASSERT(lhs.trap_state() == rhs.trap_state());
        JPH_ASSERT(lhs.frames_in_trap_state() == rhs.frames_in_trap_state());
        JPH_ASSERT(isNearlySame(lhs.x(), lhs.y(), lhs.z(), rhs.x(), rhs.y(), rhs.z()));
        JPH_ASSERT(isNearlySame(lhs.vel_x(), lhs.vel_y(), lhs.vel_z(), rhs.vel_x(), rhs.vel_y(), rhs.vel_z()));
        JPH::Quat lhsQ(lhs.q_x(), lhs.q_y(), lhs.q_z(), lhs.q_w()), rhsQ(rhs.q_x(), rhs.q_y(), rhs.q_z(), rhs.q_w());
        JPH_ASSERT(lhsQ.IsClose(rhsQ));
        JPH_ASSERT(lhs.id() == rhs.id());
    }
    
protected:
    float fallenDeathHeight = 0;

    BodyIDVector staticColliderBodyIDs;
    BodyIDVector bodyIDsToClear;
    BodyIDVector bodyIDsToAdd;
    BodyIDVector bodyIDsToActivate;

    // Backend & Frontend shared functions
    inline void elapse1RdfForRdf(const int currRdfId, RenderFrame* nextRdf);
    inline void elapse1RdfForBl(const int currRdfId, Bullet* bl, const Skill* skill, const BulletConfig* bc);
    inline void elapse1RdfForPlayerChd(const int currRdfId, PlayerCharacterDownsync* playerChd, const CharacterConfig* cc);
    inline void elapse1RdfForNpcChd(const int currRdfId, NpcCharacterDownsync* npcChd, const CharacterConfig* cc);
    inline void elapse1RdfForChd(const int currRdfId, CharacterDownsync* cd, const CharacterConfig* cc);
    inline void elapse1RdfForTrap(Trap* tp);
    inline void elapse1RdfForTrigger(Trigger* tr);
    inline void elapse1RdfForPickable(Pickable* pk);
    inline void elapse1RdfForIvSlot(InventorySlot* ivs, const InventorySlotConfig* ivsConfig);

    int moveForwardLastConsecutivelyAllConfirmedIfdId(int proposedIfdEdFrameId, uint64_t skippableJoinMask = 0);

    CH_COLLIDER_T* getOrCreateCachedPlayerCollider_NotThreadSafe(const uint64_t ud, const PlayerCharacterDownsync& currPlayer, const CharacterConfig* cc, PlayerCharacterDownsync* nextPlayer = nullptr);
    // Unlike DLLMU-v2.3.4, even if "preallocateNpcDict" is empty, "getOrCreateCachedNpcCollider_NotThreadSafe" still works
    CH_COLLIDER_T* getOrCreateCachedNpcCollider_NotThreadSafe(const uint64_t ud, const NpcCharacterDownsync& currNpc, const CharacterConfig* cc, NpcCharacterDownsync* nextNpc = nullptr);
    CH_COLLIDER_T* getOrCreateCachedCharacterCollider_NotThreadSafe(const uint64_t ud, const CharacterConfig* inCc, const float newRadius, const float newHalfHeight, const Vec3Arg& newPos, const QuatArg& newRot);

    BL_COLLIDER_T* getOrCreateCachedBulletCollider_NotThreadSafe(const uint64_t ud, const float immediateBoxHalfSizeX, const float immediateBoxHalfSizeY, const BulletType blType, const Vec3Arg& newPos, const QuatArg& newRot);
    TP_COLLIDER_T* getOrCreateCachedTrapCollider_NotThreadSafe(const uint64_t ud, const float immediateBoxHalfSizeX, const float immediateBoxHalfSizeY, const TrapConfig* tpConfig, const TrapConfigFromTiled* tpConfigFromTile, const bool forConstraintHelperBody, const bool forConstraintObsIfaceBody, const Vec3Arg& newPos, const QuatArg& newRot);

    NON_CONTACT_CONSTRAINT_T* getOrCreateCachedNonContactConstraint_NotThreadSafe(const EConstraintType nonContactConstraintType, const EConstraintSubType nonContactConstraintSubType, Body* body1, Body* body2, JPH::ConstraintSettings* inConstraintSettings);

    std::unordered_map<uint32_t, const TrapConfigFromTiled*> trapConfigFromTileDict;
    std::unordered_map<uint32_t, TriggerConfigFromTiled*> triggerConfigFromTileDict;

    InputInducedMotionStockCache inputInducedMotionStockCache;
    CollisionUdHolderStockCache_ThreadSafe collisionUdHolderStockCache;
    
    const BattleSpecificConfig* battleSpecificConfig = nullptr;

    std::unordered_set<uint64_t> transientSlipJumpableUds;
    std::unordered_map<uint64_t, CH_COLLIDER_T*> transientUdToChCollider;
    std::unordered_map<uint64_t, const BodyID*> transientUdToBodyID;
    std::unordered_map<uint64_t, TP_COLLIDER_T*> transientUdToTpCollider;
    std::unordered_map<uint64_t, const BodyID*> transientUdToConstraintHelperBodyID;
    std::unordered_map<uint64_t, Body*> transientUdToConstraintHelperBody;
    std::unordered_map<uint64_t, const BodyID*> transientUdToConstraintObsIfaceBodyID;
    std::unordered_map<uint64_t, Body*> transientUdToConstraintObsIfaceBody;

    std::unordered_map<uint64_t, CollisionUdHolder_ThreadSafe*> transientUdToCollisionUdHolder;
    std::unordered_map<uint64_t, InputInducedMotion*> transientUdToInputInducedMotion;

    std::unordered_map<uint64_t, const PlayerCharacterDownsync*> transientUdToCurrPlayer;
    std::unordered_map<uint64_t, PlayerCharacterDownsync*> transientUdToNextPlayer;

    std::unordered_map<uint64_t, const NpcCharacterDownsync*> transientUdToCurrNpc; // Mainly for "Bullet.offender_ud" referencing characters, and avoiding the unnecessary [joinIndex change in `leftShiftDeadNpcs` of DLLMU-v2.3.4](https://github.com/genxium/DelayNoMoreUnity/blob/v2.3.4/shared/Battle_builders.cs#L580)
    std::unordered_map<uint64_t, NpcCharacterDownsync*> transientUdToNextNpc;

    std::unordered_map<uint64_t, const Bullet*> transientUdToCurrBl;
    std::unordered_map<uint64_t, Bullet*> transientUdToNextBl;

    std::unordered_map<uint64_t, const Trap*> transientUdToCurrTrap;
    std::unordered_map<uint64_t, Trap*> transientUdToNextTrap;

    std::unordered_map<uint64_t, const Trigger*> transientUdToCurrTrigger;
    std::unordered_map<uint64_t, Trigger*> transientUdToNextTrigger;

    std::unordered_map<uint64_t, const Pickable*> transientUdToCurrPickable;
    std::unordered_map<uint64_t, Pickable*> transientUdToNextPickable;

    inline const CharacterDownsync& immutableCurrChdFromUd(uint64_t ud) {
        uint64_t udt = getUDT(ud);
        return immutableCurrChdFromUd(udt, ud);
    }

    inline CharacterDownsync* mutableNextChdFromUd(uint64_t ud) {
        uint64_t udt = getUDT(ud);
        return mutableNextChdFromUd(udt, ud);
    }

    inline const CharacterDownsync& immutableCurrChdFromUd(uint64_t udt, uint64_t ud) {
        return (UDT_PLAYER == udt ? transientUdToCurrPlayer[ud]->chd() : transientUdToCurrNpc[ud]->chd());
    }

    inline CharacterDownsync* mutableNextChdFromUd(uint64_t udt, uint64_t ud) {
        return (UDT_PLAYER == udt ? transientUdToNextPlayer[ud]->mutable_chd() : transientUdToNextNpc[ud]->mutable_chd());
    }

    inline void calcChCacheKey(const CharacterConfig* cc, CH_CACHE_KEY_T& ioCacheKey) {
        ioCacheKey[0] = cc->capsule_radius();
        ioCacheKey[1] = cc->capsule_half_height();
    }

    inline void calcBlCacheKey(const float immediateBoxHalfSizeX, const float immediateBoxHalfSizeY, BL_CACHE_KEY_T& ioCacheKey) {
        ioCacheKey[0] = immediateBoxHalfSizeX;
        ioCacheKey[1] = immediateBoxHalfSizeY;
    }

    inline void calcTpCacheKey(const float immediateBoxHalfSizeX, const float immediateBoxHalfSizeY, const EMotionType immediateMotionType, const bool immediateIsSensor, const ObjectLayer immediateObjLayer, TP_CACHE_KEY_T& ioCacheKey) {
        ioCacheKey.boxHalfExtentX = immediateBoxHalfSizeX;
        ioCacheKey.boxHalfExtentY = immediateBoxHalfSizeY;
        ioCacheKey.motionType = immediateMotionType;
        ioCacheKey.isSensor = immediateIsSensor;
        ioCacheKey.objLayer = immediateObjLayer;
    }

    InputFrameDownsync* getOrPrefabInputFrameDownsync(int inIfdId, uint32_t inSingleJoinIndex, uint64_t inSingleInput, bool fromUdp, bool fromTcp, bool& outExistingInputMutated);

    /* [WARNING] The following functions "batchPutIntoPhySysFromCache & batchNonContactConstraintsSetupFromCache & batchRemoveFromPhySysAndCache" are all single-threaded, using "getOrCreateCachedXxx_NotThreadSafe" extensively. Why are they NOT made multi-threaded? Here're a few concerns.
    
    - C++11 (or any later version) "new" used in "createDefaultXxx" is thread-safe, but thread-safe "new/delete" will have "lock contention" for the non-thread-safe "sbrk" anyway regardless of thread-local-cache optimization, see https://app.yinxiang.com/fx/b5affa04-b7d0-412f-9c74-6cf5f2bc6def for more information 
    - Operation "activeXxxColliders.push_back(...)" is not thread-safe, and we can use preallocated vector with an atomic counter to solve this
    - Operation "transientUdToXxx[]" is not thread-safe either, espectially the setter "transientUdToXxx[ud] = Xxx", we have to choose a map class which is built thread-safe instead (like ConcurrentHashMap in Java)
    - Failure of "FrontendTest/runTestCase11" might occurr due to multi-threaded randomness of vector traversal of "activeXxxColliders" even if all the above succeeded
    - Single-threaded traversals in "batchXxx" for just setting positions, activating/deactivating "BodyID"s in batch is already very efficient -- same big-O time comlexity as just dispatching the jobs -- while multi-threaded overhead and the use of "bi" instead of "biNoLock" might be slower when there's no heavy workload like "BroadPhase/NarrowPhaseQuery"
    */
    void batchPutIntoPhySysFromCache(const int currRdfId, const RenderFrame* currRdf, RenderFrame* nextRdf);
    void batchNonContactConstraintsSetupFromCache(const int currRdfId, const RenderFrame* currRdf, RenderFrame* nextRdf);
    void batchRemoveFromPhySysAndCache(const int currRdfId, const RenderFrame* currRdf);

    inline bool isInBlockStun(const CharacterDownsync& currChd) {
        return (Def1 == currChd.ch_state() && 0 < currChd.frames_to_recover());
    }

    inline bool isStaticCrouching(CharacterState state) {
        return (CrouchIdle1 == state || CrouchAtk1 == state || CrouchAtked1 == state);
    }

    inline bool isCrouching(CharacterState state, const CharacterConfig* cc) {
        return (CrouchIdle1 == state || CrouchAtk1 == state || CrouchAtked1 == state || (Sliding == state && cc->crouching_enabled()));
    }

    inline bool isEffInAir(const CharacterDownsync& chd, bool notDashing);

    void updateBtnHoldingByInput(const CharacterDownsync& currChd, const InputFrameDecoded& decodedInputHolder, CharacterDownsync* nextChd);

    void deriveCharacterOpPattern(const int currRdfId, const CharacterDownsync& currChd, const Vec3& currChdFacing, const CharacterConfig* cc, CharacterDownsync* nextChd, bool currEffInAir, bool notDashing, const InputFrameDecoded& ifDecoded, int& outPatternId, bool& outJumpedOrNot, bool& outSlipJumpedOrNot, int& outEffDx, int& outEffDy);

    void processSingleCharacterInput(const int currRdfId, float dt, int patternId, bool jumpedOrNot, bool slipJumpedOrNot, int effDx, int effDy, bool slowDownToAvoidOverlap, const CharacterDownsync& currChd, const MassProperties& massProps, const Vec3& currChdFacing, uint64_t ud, bool currEffInAir, bool currCrouching, bool currOnWall, bool currDashing, bool currWalking, bool currInBlockStun, bool currAtked, bool currParalyzed, const CharacterConfig* cc, CharacterDownsync* nextChd, RenderFrame* nextRdf, bool& usedSkill, const CH_COLLIDER_T* chCollider, InputInducedMotion* ioInputInducedMotion, bool& ioGravityDirty, bool& ioFrictionDirty);

    void transitToGroundDodgedChState(const CharacterDownsync& currChd, const Vec3 currChdFacing, CharacterDownsync* nextChd, const CharacterConfig* cc, bool currParalyzed, InputInducedMotion* ioInputInducedMotion);
    void calcFallenDeath(const RenderFrame* currRdf, RenderFrame* nextRdf);
    bool isInJumpStartup(const CharacterDownsync& cd, const CharacterConfig* cc);
    bool isJumpStartupJustEnded(const CharacterDownsync& currCd, const CharacterDownsync* nextCd, const CharacterConfig* cc);
    void jamBtnHolding(CharacterDownsync* nextChd);
    inline static void resetInputInducedMotion(InputInducedMotion* ioInputInducedMotion, const bool resetVelCOM=false) {
        ioInputInducedMotion->forceCOM.Set(0, 0, 0);
        ioInputInducedMotion->torqueCOM.Set(0, 0, 0);
        ioInputInducedMotion->jumpTriggered = false;
        ioInputInducedMotion->slipJumpTriggered = false;
        if (resetVelCOM) {
            ioInputInducedMotion->velCOM.Set(0, 0, 0);
            ioInputInducedMotion->angVelCOM.Set(0, 0, 0);
        }
    }


    void prepareJumpStartup(const int currRdfId, const CharacterDownsync& currChd, const uint64_t currChdUd, const MassProperties& massProps, const Vec3& currChdFacing, const bool jumpTriggered, const bool slipJumpTriggered, CharacterDownsync* nextChd, const bool currEffInAir, const CharacterConfig* cc, const bool currParalyzed, const CH_COLLIDER_T* chCollider, const bool currInJumpStartUp, const bool currDashing, InputInducedMotion* ioInputInducedMotion);

    void processInertiaWalkingHandleZeroEffDx(const int currRdfId, float dt, const CharacterDownsync& currChd, const MassProperties& massProps, const Vec3& currChdFacing, CharacterDownsync* nextChd, int effDy, const CharacterConfig* cc, bool effInAir, bool currParalyzed, const bool isInWalkingAtkAndNotRecovered, const uint64_t ud, const CH_COLLIDER_T* chCollider, const bool currDashing, InputInducedMotion* ioInputInducedMotion, bool& ioGravityDirty, bool& ioFrictionDirty);
    void processInertiaWalking(const int currRdfId, float dt, const CharacterDownsync& currChd, const MassProperties& massProps, const Vec3& currChdFacing, CharacterDownsync* nextChd, bool currEffInAir, int effDx, int effDy, const CharacterConfig* cc, bool currParalyzed, bool currInBlockStun, const uint64_t ud, const CH_COLLIDER_T* chCollider, const bool currInJumpStartUp, const bool nextInJumpStartUp, const bool currDashing, InputInducedMotion* ioInputInducedMotion, bool& ioGravityDirty, bool& ioFrictionDirty);

    void processInertiaFlyingHandleZeroEffDxAndDy(const int currRdfId, float dt, const CharacterDownsync& currChd, const MassProperties& massProps, const Vec3& currChdFacing, CharacterDownsync* nextChd, const CharacterConfig* cc, bool currParalyzed, const uint64_t ud, const CH_COLLIDER_T* chCollider, const bool currDashing, InputInducedMotion* ioInputInducedMotion, bool& ioGravityDirty, bool& ioFrictionDirty);
    void processInertiaFlying(const int currRdfId, float dt, const CharacterDownsync& currChd, const MassProperties& massProps, const Vec3& currChdFacing, CharacterDownsync* nextChd, int effDx, int effDy, const CharacterConfig* cc, bool currParalyzed, bool currInBlockStun, const uint64_t ud, const CH_COLLIDER_T* chCollider, const bool currInJumpStartup, const bool nextInJumpStartup, const bool currDashing, InputInducedMotion* ioInputInducedMotion, bool& ioGravityDirty, bool& ioFrictionDirty);


    void handleLhsCharacterCollisionWithRhsBullet(
        const int currRdfId,
        RenderFrame* nextRdf,
        const uint64_t udLhs, const uint64_t udtLhs, const CharacterDownsync* currChd, CharacterDownsync* nextChd,
        const uint64_t udRhs, const uint64_t udtRhs,
        const ContactPoints& inContactPoints,
        uint32_t& outNewEffDebuffSpeciesId, int& outNewDamage, bool& outNewEffBlownUp, int& outNewEffFramesToRecover, int& outEffDef1QuotaReduction, float& outNewEffPushbackVelX, float& outNewEffPushbackVelY);
    bool addBlHitToNextFrame(const int currRdfId, RenderFrame* nextRdf, const Bullet* referenceBullet, const Vec3& newPos);
    bool addNewBulletToNextFrame(const int currRdfId, const CharacterDownsync& currChd, const Vec3& currChdFacing, const CharacterConfig* cc, bool currParalyzed, bool currEffInAir, const Skill* skillConfig, int activeSkillHit, uint32_t activeSkillId, RenderFrame* nextRdf, const Bullet* referenceBullet, const BulletConfig* referenceBulletConfig, uint64_t offenderUd, int bulletTeamId);


    void processWallGrabbingPostPhysicsUpdate(const int currRdfId, const CharacterDownsync& currChd, CharacterDownsync* nextChd, const CharacterConfig* cc, const CH_COLLIDER_T* cv, bool inJumpStartupOrJustEnded);

    bool transitToDying(const int currRdfId, const CharacterDownsync& currChd, const bool cvInAir, CharacterDownsync* nextChd);
    bool transitToDying(const int currRdfId, const PlayerCharacterDownsync& currPlayer, const bool cvInAir, PlayerCharacterDownsync* nextPlayer);
    bool transitToDying(const int currRdfId, const NpcCharacterDownsync& currNpc, const bool cvInAir, NpcCharacterDownsync* nextNpc);

    void processDelayedBulletSelfVel(const int currRdfId, const CharacterDownsync& currChd, const MassProperties& massProps, const Vec3& currChdFacing, CharacterDownsync* nextChd, const CharacterConfig* cc, const bool currParalyzed, const bool nextEffInAir, InputInducedMotion* ioInputInducedMotion);

    virtual void stepSingleChdState(const int currRdfId, const RenderFrame* currRdf, RenderFrame* nextRdf, const float dt, const uint64_t ud, const uint64_t udt, const CharacterConfig* cc, CH_COLLIDER_T* single, const CharacterDownsync& currChd, CharacterDownsync* nextChd, bool& groundBodyIsChCollider, bool& isDead, bool& cvOnWall, bool& cvSupported, bool& cvInAir, bool& inJumpStartupOrJustEnded, CharacterBase::EGroundState& cvGroundState);
    virtual void postStepSingleChdStateCorrection(const int currRdfId, const uint64_t udt, const uint64_t ud, const CH_COLLIDER_T* chCollider, const CharacterDownsync& currChd, CharacterDownsync* nextChd, const CharacterConfig* cc, bool cvSupported, bool cvInAir, bool cvOnWall, bool currNotDashing, bool currEffInAir, bool oldNextNotDashing, bool oldNextEffInAir, bool inJumpStartupOrJustEnded, CharacterBase::EGroundState cvGroundState, const InputInducedMotion* inputInducedMotion);

    virtual void topoSortTriggerConfigFromTiledList(WsReq* initializerMapData);
    virtual void stepSingleTrivialTriggerState(const int currRdfId, const Trigger& currTrigger, Trigger* nextTrigger, StepResult* stepResult);
    virtual void stepSingleTimedTriggerState(const int currRdfId, const Trigger& currTrigger, Trigger* nextTrigger, StepResult* stepResult);
    virtual void stepSingleIndiWaveNpcSpawner(const int currRdfId, const Trigger& currTrigger, Trigger* nextTrigger);

    void leftShiftDeadNpcs(const int currRdfId, RenderFrame* nextRdf);
    void leftShiftDeadBullets(const int currRdfId, RenderFrame* nextRdf);
    void leftShiftDeadDynamicTraps(const int currRdfId, RenderFrame* nextRdf);
    void leftShiftDeadTriggers(const int currRdfId, RenderFrame* nextRdf);
    void leftShiftDeadPickables(const int currRdfId, RenderFrame* nextRdf);

    void leftShiftDeadBlImmuneRecords(const int currRdfId, CharacterDownsync* nextChd);
    void leftShiftDeadIvSlots(const int currRdfId, CharacterDownsync* nextChd);
    void leftShiftDeadBuffs(const int currRdfId, CharacterDownsync* nextChd);
    void leftShiftDeadDebuffs(const int currRdfId, CharacterDownsync* nextChd);

    bool useSkill(const int currRdfId, RenderFrame* nextRdf, const CharacterDownsync& currChd, const Vec3& currChdFacing, uint64_t ud, const CharacterConfig* cc, CharacterDownsync* nextChd, int effDx, int effDy, int patternId, bool currEffInAir, bool currCrouching, bool currOnWall, bool currDashing, bool currWalking, bool currInBlockStun, bool currAtked, bool currParalyzed, int& outSkillId, const Skill*& outSkill, const BulletConfig*& outPivotBc);

    void useInventorySlot(const int currRdfId, int patternId, const CharacterDownsync& currChd, const CharacterConfig* cc, CharacterDownsync* nextChd, bool& outSlotUsed, bool& outDodgedInBlockStun);
    static bool IsChargingAtkChState(CharacterState chState) {
        return (CharacterState::Atk7_Charging == chState);
    }

    inline const CharacterConfig* getCc(uint32_t speciesId) {
        auto& ccs = globalConfigConsts->character_configs();
        JPH_ASSERT(ccs.contains(speciesId));
        auto& v = ccs.at(speciesId);
        return &v;
    }

    inline const TriggerConfig* getTriggerConfig(uint32_t tt) {
        auto& tcs = globalConfigConsts->trigger_configs();
        JPH_ASSERT(tcs.contains(tt));
        auto& v = tcs.at(tt);
        return &v;
    }

    CH_COLLIDER_T* createDefaultCharacterCollider(const CharacterConfig* cc, const Vec3Arg& newPos, const QuatArg& newRot, const uint64_t newUd, BodyInterface* inBodyInterface);

    inline EMotionType       calcBlMotionType(const BulletType blType) {
        /*
         Kindly note that in Jolt, "NonDynamics v.s. NonDynamics" (e.g. "Kinematic v.s. Static" or even "Kinematic v.s. Kinematic") WOULDN'T be automatically handled by "ContactConstraintManager" (https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Physics/Constraints/ContactConstraintManager.cpp#L1108). 
         
         It's very deep in the collision handling callstack (https://github.com/genxium/JoltDLLMU/blob/main/JoltBindings/RuleOfThumb.md) and there's no simple hook to override this behavior while integrating well with the existing solver.
         
         By the time of writing, I think it's both difficult and unnecessary to find a way of adding "Kinematic-Static ContactConstraint" or "Kinematic-Kinematic ContactConstraint" into "ContactConstraintManager".
        */
        switch (blType) {
            case BulletType::MagicalFireball:
            case BulletType::Melee:
                return EMotionType::Kinematic;
            case BulletType::MechanicalCartridge:
            case BulletType::GroundWave:
                return EMotionType::Dynamic;
            default:
                return EMotionType::Static;
        }
    }

    inline bool               calcBlIsSensor(const BulletType blType) {
        switch (blType) {
            case BulletType::MechanicalCartridge:
                return false;
            default:
                return true;
        }
    }

    BL_COLLIDER_T*             createDefaultBulletCollider(const float immediateBoxHalfSizeX, const float immediateBoxHalfSizeY, float& outConvexRadius, const EMotionType motionType, const bool isSensor, const Vec3Arg& newPos, const QuatArg& newRot, BodyInterface* inBodyInterface);

    TP_COLLIDER_T* createDefaultTrapCollider(const Vec3Arg& newHalfExtent, const Vec3Arg& newPos, const QuatArg& newRot, float& outConvexRadius, const EMotionType motionType, const bool isSensor, const ObjectLayer objLayer, BodyInterface* inBodyInterface);

    NON_CONTACT_CONSTRAINT_T*  createDefaultNonContactConstraint(const EConstraintType nonContactConstraintType, const EConstraintSubType nonContactConstraintSubType, Body* inBody1, Body* inBody2, JPH::ConstraintSettings* inConstraintSettings);

    void preallocateBodies(const RenderFrame* startRdf, const ::google::protobuf::Map< uint32_t, uint32_t >& preallocateNpcSpeciesDict);

    inline void calcChdShape(const CharacterState chState, const CharacterConfig* cc, float& outCapsuleRadius, float& outCapsuleHalfHeight);

    inline float lerp(float from, float to, float step) {
        float diff = (to - from);
        if (IsLengthSquaredNearZero(diff * diff)) {
            return to;
        }
        float proposed = from + step;
        if (0 < step && proposed > to) {
            return to;
        }

        if (0 > step && proposed < to) {
            return to;
        }

        return proposed;
    }

    inline bool updatePlayerInputFronts(int inIfdId, int inSingleJoinIndexArrIdx, int inSingleInput) {
        if (inIfdId <= playerInputFrontIds[inSingleJoinIndexArrIdx]) {
            return false;
        }
        auto it = playerInputFrontIdsSorted.find(playerInputFrontIds[inSingleJoinIndexArrIdx]);
        if (it != playerInputFrontIdsSorted.end()) {
            playerInputFrontIdsSorted.erase(it);
        }
        playerInputFrontIds[inSingleJoinIndexArrIdx] = inIfdId;
        playerInputFronts[inSingleJoinIndexArrIdx] = inSingleInput;
        playerInputFrontIdsSorted.insert(inIfdId);
        return true;
    }

    inline bool isBulletJustActive(const Bullet* bullet, const BulletConfig* bc, int currRdfId) const {
        // [WARNING] Practically a bullet might propagate for a few render frames before hitting its visually "VertMovingTrapLocalIdUponActive"!
        if (BulletState::Active != bullet->bl_state()) {
            return false;
        }
        int visualBufferRdfCnt = 3;
        return visualBufferRdfCnt >= bullet->frames_in_bl_state();
    }

    inline bool isBulletAlive(const Bullet* bullet, const BulletConfig* bc, const int currRdfId) const {
        if (BulletState::Vanishing == bullet->bl_state()) {
            return bullet->frames_in_bl_state() < bc->vanishing_anim_rdf_cnt();
        }
        if (BulletState::Hit == bullet->bl_state()) {
            return bullet->frames_in_bl_state() < bc->hit_anim_rdf_cnt();
        }
        if (BulletState::StartUp == bullet->bl_state() && BulletType::Melee == bc->b_type()) {
            uint64_t offenderUd = bullet->offender_ud();
            uint64_t offenderUdt = getUDT(offenderUd);
            switch (offenderUdt) {
                case UDT_PLAYER: {
                    if (!transientUdToNextPlayer.count(offenderUd)) {
                        return false; // The offender might've been dead
                    }
                    auto nextOffenderPlayer = transientUdToNextPlayer.at(offenderUd); 
                    auto nextOffenderChd = nextOffenderPlayer->chd(); 
                    if (atkedSet.count(nextOffenderChd.ch_state()) || noOpSet.count(nextOffenderChd.ch_state())) {
                        return false; // The offender is hit
                    }
                    break;
                }
                case UDT_NPC: {
                    if (!transientUdToNextNpc.count(offenderUd)) {
#ifndef NDEBUG
                        std::ostringstream oss;
                        auto bulletId = bullet->id();
                        oss << "isBulletAlive returning false#1 because bulletId=" << bulletId << ", offenderUd=" << offenderUd << " doesn't exist in transientUdToNextNpc";
                        Debug::Log(oss.str(), DColor::Orange);
#endif
                        return false; // The offender might've been dead
                    }
                    auto nextOffenderNpc = transientUdToNextNpc.at(offenderUd); 
                    auto nextOffenderChd = nextOffenderNpc->chd(); 
                    if (atkedSet.count(nextOffenderChd.ch_state()) || noOpSet.count(nextOffenderChd.ch_state())) {
#ifndef NDEBUG
                        std::ostringstream oss;
                        auto bulletId = bullet->id();
                        oss << "isBulletAlive returning false#2 because bulletId=" << bulletId << ", offenderUd=" << offenderUd << " has next_ch_state=" << (int)nextOffenderChd.ch_state();
                        Debug::Log(oss.str(), DColor::Orange);
#endif
                        return false; // The offender is hit
                    }
                    break;
                }
                default: {
                    break;
                }
            }
        }
        return (currRdfId < bullet->originated_render_frame_id() + bc->startup_frames() + bc->active_frames() + bc->cooldown_frames());
    }

    inline bool isTrapAlive(const Trap* trap, int currRdfId) const {
        // TODO
        return true;
    }

    inline bool isTriggerAlive(const Trigger* trigger, int currRdfId) const {
        return 0 < trigger->demanded_evt_mask() || 0 < trigger->quota();
    }

    inline bool isPickableAlive(const Pickable* pickable, int currRdfId) const {
        return 0 < pickable->remaining_lifetime_rdf_count();
    }

    inline bool isNpcJustDead(const CharacterDownsync* chd) const {
        return (0 >= chd->hp() && globalPrimitiveConsts->dying_frames_to_recover() == chd->frames_to_recover());
    }

    inline bool isNpcDeadToDisappear(const CharacterDownsync* chd) const {
        return (0 >= chd->hp() && 0 >= chd->frames_to_recover());
    }

    inline bool publishNpcExhaustedEvt(const int currRdfId, uint64_t publishingEvtMask, uint64_t offenderUd, int offenderBulletTeamId, Trigger* nextRdfTrigger) {
        if (0 == nextRdfTrigger->demanded_evt_mask()) return false;
        nextRdfTrigger->set_fulfilled_evt_mask(nextRdfTrigger->fulfilled_evt_mask() | publishingEvtMask);
        nextRdfTrigger->set_offender_ud(offenderUd);
        nextRdfTrigger->set_offender_bullet_team_id(offenderBulletTeamId);
        return true;
    }

    inline bool isBattleSettled(const RenderFrame* rdf) const {
        if (rdf->has_prev_rdf_step_result()) {
            auto& prevRdfStepResult = rdf->prev_rdf_step_result();
            for (int i = 0; i < prevRdfStepResult.fulfilled_triggers_size(); i++) {
                auto& fulfilledTrigger = prevRdfStepResult.fulfilled_triggers(i);
                if (globalPrimitiveConsts->trt_victory() == fulfilledTrigger.trt()) {
                    return true;
                }
            }
        }
        return false;
    }
    
    virtual bool allocPhySys() = 0;
    virtual bool deallocPhySys();

    void stringifyPlayerInputsInIfdBuffer(std::ostringstream& oss, int joinIndexArrIdx) {
        if (0 >= ifdBuffer.Cnt) return;
        bool nonEmpty = false;
        for (int ifdId = ifdBuffer.StFrameId; ifdId < ifdBuffer.EdFrameId; ifdId++) {
            auto ifd = ifdBuffer.GetByFrameId(ifdId);
            if (nullptr == ifd) break;
            uint64_t cmd = ifd->input_list(joinIndexArrIdx);
            if (nonEmpty) {
                oss << "; ";
            } else {
                oss << "[";
                nonEmpty = true;
            }
            oss << "{j:" << ifdId << ", si:" << cmd << ", c:" << ifd->confirmed_list() << ", uc:" << ifd->udp_confirmed_list() << "}"; // "si" == "self input"
        }
        if (nonEmpty) {
            oss << "]";
        }
    }

private:
    Vec3 safeDeactiviatedPosition;
    InputFrameDecoded ifDecodedHolder;

public:
    // BaseBattleCollisionFilter
    virtual JPH::ValidateResult validateLhsCharacterContact(const CharacterDownsync* lhsCurrChd, const CharacterDownsync* rhsCurrChd) const {
        if (lhsCurrChd->bullet_team_id() == rhsCurrChd->bullet_team_id()) {
            return JPH::ValidateResult::RejectContact;
        }
        if (invinsibleSet.count(lhsCurrChd->ch_state()) || invinsibleSet.count(rhsCurrChd->ch_state())) {
            return JPH::ValidateResult::RejectContact;
        }
        return JPH::ValidateResult::AcceptContact;
    }

    virtual JPH::ValidateResult validateLhsCharacterContact(const CharacterDownsync* lhsCurrChd, const Bullet* rhsCurrBl) const {
        if (BulletState::Active != rhsCurrBl->bl_state()) {
            return JPH::ValidateResult::RejectContact;
        }

        if (lhsCurrChd->bullet_team_id() == rhsCurrBl->team_id()) {
            if (!rhsCurrBl->for_ally()) {
                return JPH::ValidateResult::RejectContact;
            }
        }

        if (invinsibleSet.count(lhsCurrChd->ch_state())) {
            if (!rhsCurrBl->for_ally()) {
                return JPH::ValidateResult::RejectContact;
            }
        }
        return JPH::ValidateResult::AcceptContact;
    }

    virtual JPH::ValidateResult validateLhsCharacterContact(const CharacterDownsync* lhsCurrChd, const Trap* rhsCurrTrap) const {
        return JPH::ValidateResult::AcceptContact;
    }

    virtual JPH::ValidateResult validateLhsCharacterContact(const CharacterDownsync* lhsCurrChd, const CharacterDownsync* lhsNextChd, const uint64_t udRhs, const uint64_t udtRhs, const Body& rhs) const {
        if (transientSlipJumpableUds.count(udRhs)) {
            // Check early returns
            if (CharacterState::InAirIdle1BySlipJump == lhsCurrChd->ch_state()) {
                return JPH::ValidateResult::RejectContact;
            } else if (CharacterState::InAirIdle1BySlipJump == lhsNextChd->ch_state()) {
                return JPH::ValidateResult::RejectContact;
            } else if (0 < lhsCurrChd->vel_y()) {
                return JPH::ValidateResult::RejectContact;
            }  else {
                const AABox& rhsAABB = rhs.GetWorldSpaceBounds();
                if (lhsCurrChd->y() < rhsAABB.mMin.GetY()) {
                    return JPH::ValidateResult::RejectContact;
                }
            }
        }

        switch (udtRhs) {
        case UDT_PLAYER: {
            auto rhsCurrPlayer = transientUdToCurrPlayer.at(udRhs);
            auto& rhsCurrChd = rhsCurrPlayer->chd();
            return validateLhsCharacterContact(lhsCurrChd, &rhsCurrChd);
        }
        case UDT_NPC: {
            auto rhsCurrNpc = transientUdToCurrNpc.at(udRhs);
            auto& rhsCurrChd = rhsCurrNpc->chd();
            return validateLhsCharacterContact(lhsCurrChd, &rhsCurrChd);
        }
        case UDT_BL: {
            if (!transientUdToCurrBl.count(udRhs)) {
/*
#ifndef NDEBUG
                std::ostringstream oss;
                auto bulletId = getUDPayload(udRhs);
                oss << "validateLhsCharacterContact/currBl with udRhs=" << udRhs << ", id=" << bulletId << " is NOT FOUND";
                Debug::Log(oss.str(), DColor::Orange);
#endif
*/
                return JPH::ValidateResult::RejectContact;
            }
            auto rhsCurrBl = transientUdToCurrBl.at(udRhs);
            return validateLhsCharacterContact(lhsCurrChd, rhsCurrBl);
        }
        case UDT_TRAP: {
            auto rhsCurrTp = transientUdToCurrTrap.at(udRhs);
            return validateLhsCharacterContact(lhsCurrChd, rhsCurrTp);
        }
        default:
            return JPH::ValidateResult::AcceptContact;
        }
    }

    virtual JPH::ValidateResult validateLhsCharacterContact(const uint64_t udLhs, const uint64_t udtLhs,
        const JPH::Body& lhs, // the "Character"
        const uint64_t udRhs, const uint64_t udtRhs, const JPH::Body& rhs) const {
        switch (udtLhs) {
            case UDT_PLAYER: {
                auto lhsCurrPlayer = transientUdToCurrPlayer.at(udLhs);
                auto& lhsCurrChd = lhsCurrPlayer->chd();
                auto lhsNextPlayer = transientUdToNextPlayer.at(udLhs);
                auto& lhsNextChd = lhsNextPlayer->chd();
                return validateLhsCharacterContact(&lhsCurrChd, &lhsNextChd, udRhs, udtRhs, rhs);
                break;
            }
            case UDT_NPC: {
                auto lhsCurrNpc = transientUdToCurrNpc.at(udLhs);
                auto& lhsCurrChd = lhsCurrNpc->chd();
                auto lhsNextNpc = transientUdToNextNpc.at(udLhs);
                auto& lhsNextChd = lhsNextNpc->chd();
                return validateLhsCharacterContact(&lhsCurrChd, &lhsNextChd, udRhs, udtRhs, rhs);
                break;
            }
            default:
                return JPH::ValidateResult::AcceptContact;
        }
    }

    virtual JPH::ValidateResult validateLhsBulletContact(const Bullet* lhsCurrBl, const uint64_t udRhs, const uint64_t udtRhs, const Body& rhs) const {
        switch (udtRhs) {
        case UDT_PLAYER: {
            auto rhsCurrPlayer = transientUdToCurrPlayer.at(udRhs);
            auto& rhsCurrChd = rhsCurrPlayer->chd();
            if (lhsCurrBl->team_id() == rhsCurrChd.bullet_team_id()) {
                return JPH::ValidateResult::RejectContact;
            }
            if (invinsibleSet.count(rhsCurrChd.ch_state())) {
                return JPH::ValidateResult::RejectContact;
            }
            return JPH::ValidateResult::AcceptContact;
        }
        case UDT_NPC: {
            auto rhsCurrNpc = transientUdToCurrNpc.at(udRhs);
            auto& rhsCurrChd = rhsCurrNpc->chd();
            if (lhsCurrBl->team_id() == rhsCurrChd.bullet_team_id()) {
                return JPH::ValidateResult::RejectContact;
            }
            if (invinsibleSet.count(rhsCurrChd.ch_state())) {
                return JPH::ValidateResult::RejectContact;
            }
            return JPH::ValidateResult::AcceptContact;

        }
        case UDT_BL: {
            auto rhsCurrBl = transientUdToCurrBl.at(udRhs);
            if (lhsCurrBl->team_id() == rhsCurrBl->team_id()) {
                return JPH::ValidateResult::RejectContact;
            }
            if (BulletState::Active != lhsCurrBl->bl_state()) {
                return JPH::ValidateResult::RejectContact;
            }

            if (BulletState::Active != rhsCurrBl->bl_state()) {
                return JPH::ValidateResult::RejectContact;
            }
            return JPH::ValidateResult::AcceptContact;
        }
        default:
            return JPH::ValidateResult::AcceptContact;
        }
    }

    virtual JPH::ValidateResult validateLhsBulletContact(const uint64_t udLhs,
        const JPH::Body& lhs, // the "Bullet"
        const uint64_t udRhs, const uint64_t udtRhs, const JPH::Body& rhs) const {
        auto lhsCurrBl = transientUdToCurrBl.at(udLhs);
        return validateLhsBulletContact(lhsCurrBl, udRhs, udtRhs, rhs);
    }

public:
    // #JPH::ContactListener
    /*
    [WARNING] For rollback-netcode implementation, only "OnContactAdded" and "OnContactPersisted" are to be concerned with, MOREOVER there's NO NEED to clear "contact cache" upon each "Step".

    In a Jolt PhysicsSystem, all EMotionType pairs will trigger "OnContactAdded" or "OnContactPersisted" when they come into contact geometrically (e.g. Kinematic v.s. Kinematic, Kinematic v.s. Static), but NOT ALL EMotionType pairs will create "ContactConstraint" instances (e.g. Kinematic v.s. Kinematic, Kinematic v.s. Static, see https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Physics/Constraints/ContactConstraintManager.cpp#L1108).

    Moreover, Jolt calls "OnContactAdded" and "OnContactPersisted" in a multi-threaded manner, so the codes of "OnContactCommon" MUST BE thread-safe. How does Jolt keep itself thread-safe for the complicated collision detection and constraint solver? Take "ContactConstraintManager" as an example,
    
    - by calling "PhysicsSystem::JobFindCollisions", it only appends newly discovered constraints, making use of counter "ContactConstraintManager.mNumConstraints"(https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Physics/Constraints/ContactConstraintManager.h#L515) to guarantee thread-safety of the constraint collection "ContactConstraintManager.mConstraints"(https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Physics/Constraints/ContactConstraintManager.cpp#L1126) -- a similar technique like that of "<proj-root>/JoltBindings/joltc/RingBufferMt";
    
    - by calling of "PhysicsSystem::JobSolvePositionConstraints" (https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Physics/PhysicsSystem.cpp#L503, kindly note that "inCollisionSteps" of each "PhysicsSystem::Update" is always 1 in my current implementation of rollback-chasing), it only fetches "next island", making use of counter "PhysicsUpdateContext.mSolvePositionConstraintsNextIsland"(https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Physics/PhysicsUpdateContext.h#L93) to guarantee thread-safety of the island collection "IslandBuilder::mIslandsSorted"(https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Physics/IslandBuilder.cpp#L408).

    However, there's a "determinism caveat" associated with the aforementioned "PhysicsSystem::JobFindCollisions" and "PhysicsSystem::JobSolvePositionConstraints" -- as the appending and consuming of "ContactConstraintManager.mConstraints" and "IslandBuilder::mIslandsSorted" are randomized by multi-threading, you will execute "[AxisConstraintPart::SolvePositionConstraintWithMassOverride](https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Physics/Constraints/ConstraintPart/AxisConstraintPart.h#L605) -> [Body::Add/SubPositionStep](https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Physics/Body/Body.h#L342)" in a random order (i.e. whether "IslandBuilder::mIslandsSorted" is sorted or not doesn't matter, the randomness comes from multi-threading intrinsically) -- hence the solved positions & velocities would NOT be perfectly determinisitc due to the lack of associativity in floating number calculations.     
    */

    /*
    On the other hand, it's INTENTIONALLY AVOIDED to call "CharacterCollideShapeCollector" within "OnContactCommon" -- take "Character" as an example,  
    - during BroadPhase multi-threading, a same "Character" might enter "OnContactCommon" from different threads concurrently, if we'd like to manage "CharacterCollideShapeCollector.mBestDot" in a thread-safe manner, the same "do { ...... compare_exchange_weak(...) ...... } while(0 < retryCnt)" trick in "RingBufferMt::DryPut/Pop/PopTail" can be used; 
    - HOWEVER it's NOT worth such hassle! It's much simpler to handle "same-(character/bullet/trap)-NarrowPhase-in-same-thread" in later traversal (i.e. to call "PostSimulationWithCollector -> CharacterCollideShapeCollector") -- by ONLY making use of BroadPhase multithreading to solve constraints (including "regular NonContactConstraint" and "ContactConstraint") we balance both efficiency and simplicity.  
    */
    virtual void OnContactCommon(const JPH::Body& inBody1,
        const JPH::Body& inBody2,
        const JPH::ContactManifold& inManifold,
        const JPH::ContactSettings& ioSettings) {

        uint64_t ud1 = inBody1.GetUserData();
        uint64_t ud2 = inBody2.GetUserData();

        if (transientUdToCollisionUdHolder.count(ud1)) {
            CollisionUdHolder_ThreadSafe* udHolder = transientUdToCollisionUdHolder[ud1];
            udHolder->Add_ThreadSafe(ud2, inManifold.mRelativeContactPointsOn1);
        }
        if (transientUdToCollisionUdHolder.count(ud2)) {
            CollisionUdHolder_ThreadSafe* udHolder = transientUdToCollisionUdHolder[ud2];
            udHolder->Add_ThreadSafe(ud1, inManifold.mRelativeContactPointsOn2);
        }
        // Others are intentionally left blank by the time of writing.
    }

    virtual void OnContactAdded(
        const JPH::Body& inBody1,
        const JPH::Body& inBody2,
        const JPH::ContactManifold& inManifold,
        JPH::ContactSettings& ioSettings) override {
        OnContactCommon(inBody1, inBody2, inManifold, ioSettings);
    }

    // Called whenever a contact point is being persisted (i.e., it already existed in the previous frame).
    virtual void OnContactPersisted(
        const JPH::Body& inBody1,
        const JPH::Body& inBody2,
        const JPH::ContactManifold& inManifold,
        JPH::ContactSettings& ioSettings) override {
        OnContactCommon(inBody1, inBody2, inManifold, ioSettings);
    }

    // Called to validate if a contact should be allowed.
    virtual JPH::ValidateResult OnContactValidate(
        const JPH::Body& inBody1,
        const JPH::Body& inBody2,
        JPH::RVec3Arg inBaseOffset,
        const JPH::CollideShapeResult& inCollisionResult) override {
        // Just for simplicity, we'll handle validation elsewhere by following DLLMU-v2.3.4.
        
        /*
        [TODO]

        Better override "JPH::PhysicsSystem" to take "BaseBattle" instance as a member for earlier access to "transientUdToXxx" in "JPH::PhysicsSystem.mSimCollideBodyVsBody" (i.e. before any ShapeVsShape calculation).
        */
        uint64_t ud1 = inBody1.GetUserData();
        uint64_t ud2 = inBody2.GetUserData();

        uint64_t udt1 = getUDT(ud1);
        uint64_t udt2 = getUDT(ud2);

        if (transientUdToConstraintHelperBodyID.count(ud2) && inBody2.IsSensor()) return JPH::ValidateResult::RejectContact;
        if (transientUdToConstraintHelperBodyID.count(ud1) && inBody1.IsSensor()) return JPH::ValidateResult::RejectContact;

        if (transientSlipJumpableUds.count(ud2)) {
            // Check early returns
            const AABox& lhsAABB = inBody1.GetWorldSpaceBounds(); 
            const AABox& rhsAABB = inBody2.GetWorldSpaceBounds(); 
            if (lhsAABB.mMin.GetY() < rhsAABB.mMin.GetY()) {    
                return JPH::ValidateResult::RejectContact;    
            }
        } else if (transientSlipJumpableUds.count(ud1)) {
            // Check early returns
            const AABox& lhsAABB = inBody1.GetWorldSpaceBounds(); 
            const AABox& rhsAABB = inBody2.GetWorldSpaceBounds(); 
            if (rhsAABB.mMin.GetY() < lhsAABB.mMin.GetY()) {    
                return JPH::ValidateResult::RejectContact;    
            }
        }

        switch (udt1)
        {
        case UDT_PLAYER:
        case UDT_NPC:
            return validateLhsCharacterContact(ud1, udt1, inBody1, ud2, udt2, inBody2);
        case UDT_BL:
            return validateLhsBulletContact(ud1, inBody1, ud2, udt2, inBody2);
        default:
            switch (udt2) {
            case UDT_PLAYER:
            case UDT_NPC:
                return validateLhsCharacterContact(ud2, udt2, inBody2, ud1, udt1, inBody1);
            case UDT_BL:
                return validateLhsBulletContact(ud2, inBody2, ud1, udt1, inBody1);
            default:
                return JPH::ValidateResult::AcceptContact;
            }
        }

    }
};

#endif
