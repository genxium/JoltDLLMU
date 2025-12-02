#ifndef BASE_BATTLE_H_
#define BASE_BATTLE_H_ 1

#include "joltc_export.h"
#include "CharacterCollideShapeCollector.h"
#include "BulletCollideShapeCollector.h"
#include "FrameRingBuffer.h"

#include "CollisionLayers.h"
#include "CollisionCallbacks.h"
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

typedef struct NonContactConstraintHasher {
    std::size_t operator()(const NON_CONTACT_CONSTRAINT_KEY_T& k) const {
        return std::hash<int>()(static_cast<int>(k.first)) + 0x9e3779b9 + std::hash<int>()(static_cast<int>(k.second));
    }
} NonContactConstraintHasher;

// All Jolt symbols are in the JPH namespace
using namespace JPH;
using namespace jtshared;

const JPH::Quat cTurnbackAroundYAxis = JPH::Quat(0, 1, 0, 0);

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
    BodyInterface* bi;
    JobSystemThreadPool* jobSys;

    BL_COLLIDER_Q activeBlColliderList;
    BL_COLLIDER_Q* blStockCache;
    std::unordered_map< BL_CACHE_KEY_T, BL_COLLIDER_Q, VectorFloatHasher > cachedBlColliders; // Key is "{(default state) halfExtent}", where "convexRadius" is determined by "halfExtent"

    /*
    [WARNING] The use of "activeChColliderList" in "BaseBattle::batchRemoveFromPhySysAndCache" is order-sensitive, i.e. the traversal order of removing "activeChColliderList" MUST BE "BACK-TO-FRONT", any other traversal order there results in failure of "FrontendTest/runTestCase11" when checking rollback-chasing alignment of characters. 

    By the time of writing the cause of misalignment when using removal orders other than back-to-front is unknown, while "back-to-front" being correct might be a coincidence of matching some order-sensitive mechanism in JoltPhysics/PhysicsSystem-Character setters.
    */
    CH_COLLIDER_Q activeChColliderList;
    /*
     [TODO]

     Make "cachedChColliders" keyed by "CharacterConfig.species_id()" to fit the need of multi-shape character (at different ch_state).

    It's by design that "ScaledShape" is NOT used here, because when mixed with translation and rotation, the order of affine transforms matters but is difficult to keep in mind.

    Moreover, by using this approach to manage multi-shape character I dropped the "shared shapes across bodies" feature of Jolt.
    */
    std::unordered_map< CH_CACHE_KEY_T, CH_COLLIDER_Q, VectorFloatHasher > cachedChColliders; // Key is "{(default state) radius, halfHeight}", kindly note that position and orientation of "Character" are mutable during reuse, thus not using "RefConst<>".

    NON_CONTACT_CONSTRAINT_Q activeNonContactConstraints;
    std::unordered_map< NON_CONTACT_CONSTRAINT_KEY_T, NON_CONTACT_CONSTRAINT_Q, NonContactConstraintHasher > cachedNonContactConstraints; // Key is "{non-contact-constraint type}"

public:
    static void FindBulletConfig(uint32_t skillId, uint32_t skillHit, const Skill*& outSkill, const BulletConfig*& outBulletConfig);

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

    inline static float InvSqrt32(float number)
    {
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

    void updateChColliderBeforePhysicsUpdate(uint64_t ud, const CharacterDownsync& currChd, const CharacterDownsync& nextChd);

    virtual RenderFrame* CalcSingleStep(int currRdfId, int delayedIfdId, InputFrameDownsync* delayedIfd);
    
    virtual void Clear();

    virtual bool ResetStartRdf(char* inBytes, int inBytesCnt);
    virtual bool ResetStartRdf(const WsReq* initializerMapData);

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

    inline static void AssertNearlySame(const RenderFrame* lhs, const RenderFrame* rhs) {
        JPH_ASSERT(lhs->players_arr_size() == rhs->players_arr_size());
        for (int i = 0; i < lhs->players_arr_size(); i++) {
            auto lhsCh = lhs->players_arr(i);
            auto rhsCh = rhs->players_arr(i);
            BaseBattle::AssertNearlySame(lhsCh, rhsCh);
        }
        JPH_ASSERT(lhs->npcs_arr_size() == rhs->npcs_arr_size());
        JPH_ASSERT(lhs->npc_count() == rhs->npc_count());
        JPH_ASSERT(lhs->npc_id_counter() == rhs->npc_id_counter());
        for (int i = 0; i < lhs->npcs_arr_size(); i++) {
            auto lhsCh = lhs->npcs_arr(i);
            auto rhsCh = rhs->npcs_arr(i);
            BaseBattle::AssertNearlySame(lhsCh, rhsCh);
            if (globalPrimitiveConsts->terminating_character_id() == lhsCh.id()) break;
        }
        JPH_ASSERT(lhs->bullets_size() == rhs->bullets_size());
        JPH_ASSERT(lhs->bullet_id_counter() == rhs->bullet_id_counter());
        JPH_ASSERT(lhs->bullet_count() == rhs->bullet_count());
        for (int i = 0; i < lhs->bullets_size(); i++) {
            auto lhsB = lhs->bullets(i);
            auto rhsB = rhs->bullets(i);
            BaseBattle::AssertNearlySame(lhsB, rhsB);
            if (globalPrimitiveConsts->terminating_bullet_id() == lhsB.id()) break;
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
        JPH_ASSERT(lhs.btn_a_holding_rdf_cnt() == rhs.btn_a_holding_rdf_cnt());
        JPH_ASSERT(lhs.btn_b_holding_rdf_cnt() == rhs.btn_b_holding_rdf_cnt());
        JPH_ASSERT(lhs.btn_c_holding_rdf_cnt() == rhs.btn_c_holding_rdf_cnt());
        JPH_ASSERT(lhs.btn_d_holding_rdf_cnt() == rhs.btn_d_holding_rdf_cnt());
        JPH_ASSERT(lhs.btn_e_holding_rdf_cnt() == rhs.btn_e_holding_rdf_cnt());
        JPH_ASSERT(lhs.frames_invinsible() == rhs.frames_invinsible());
        JPH_ASSERT(lhs.frames_to_recover() == rhs.frames_to_recover());
        JPH_ASSERT(lhs.frames_to_start_jump() == rhs.frames_to_start_jump());
        JPH_ASSERT(lhs.remaining_air_dash_quota() == rhs.remaining_air_dash_quota());
        JPH_ASSERT(lhs.remaining_air_jump_quota() == rhs.remaining_air_jump_quota());
        JPH_ASSERT(lhs.remaining_def1_quota() == rhs.remaining_def1_quota());
        JPH_ASSERT(lhs.slip_jump_triggered() == rhs.slip_jump_triggered());
        JPH_ASSERT(lhs.jump_triggered() == rhs.jump_triggered());
        JPH_ASSERT(lhs.jump_started() == rhs.jump_started());
        JPH_ASSERT(isNearlySame(lhs.vel_x(), lhs.vel_y(), lhs.vel_z(), rhs.vel_x(), rhs.vel_y(), rhs.vel_z()));
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
        JPH_ASSERT(isNearlySame(lhs.vel_x(), lhs.vel_y(), lhs.vel_z(), rhs.vel_x(), rhs.vel_y(), rhs.vel_z()));
        JPH::Quat lhsQ(lhs.q_x(), lhs.q_y(), lhs.q_z(), lhs.q_w()), rhsQ(rhs.q_x(), rhs.q_y(), rhs.q_z(), rhs.q_w());
        JPH_ASSERT(lhsQ.IsClose(rhsQ));
        JPH_ASSERT(lhs.repeat_quota_left() == rhs.repeat_quota_left());
        JPH_ASSERT(lhs.remaining_hard_pushback_bounce_quota() == rhs.remaining_hard_pushback_bounce_quota());
        JPH_ASSERT(lhs.target_ud() == rhs.target_ud());
        JPH_ASSERT(lhs.damage_dealed() == rhs.damage_dealed());

        JPH_ASSERT(lhs.exploded_on_ifc() == rhs.exploded_on_ifc());
        JPH_ASSERT(lhs.active_skill_hit() == rhs.active_skill_hit());
        JPH_ASSERT(lhs.skill_id() == rhs.skill_id());
        JPH_ASSERT(lhs.id() == rhs.id());
        JPH_ASSERT(lhs.team_id() == rhs.team_id());
    }

    inline static bool isNearlySame(Vec3& lhs, Vec3& rhs) {
        return isNearlySame(lhs.GetX(), lhs.GetY(), lhs.GetZ(), rhs.GetX(), rhs.GetY(), rhs.GetZ());
    }

    inline static bool isNearlySame(const float lhsX, const float lhsY, const float lhsZ, const float rhsX, const float rhsY, const float rhsZ) {
        float dx = rhsX - lhsX;
        float dy = rhsY - lhsY;
        float dz = rhsZ - lhsZ;
        return IsLengthDiffSquaredNearlySame(dx*dx + dy*dy + dz*dz);
    }

    inline static uint64_t CalcJoinIndexMask(uint32_t joinIndex) {
        if (0 == joinIndex) return 0;
        return (U64_1 << (joinIndex - 1));
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
    inline void elapse1RdfForTrigger(Trigger* tr);
    inline void elapse1RdfForPickable(Pickable* pk);

    int moveForwardLastConsecutivelyAllConfirmedIfdId(int proposedIfdEdFrameId, uint64_t skippableJoinMask = 0);

    CH_COLLIDER_T* getOrCreateCachedPlayerCollider(const uint64_t ud, const PlayerCharacterDownsync& currPlayer, const CharacterConfig* cc, PlayerCharacterDownsync* nextPlayer = nullptr);
    // Unlike DLLMU-v2.3.4, even if "preallocateNpcDict" is empty, "getOrCreateCachedNpcCollider" still works
    CH_COLLIDER_T* getOrCreateCachedNpcCollider(const uint64_t ud, const NpcCharacterDownsync& currNpc, const CharacterConfig* cc, NpcCharacterDownsync* nextNpc = nullptr);
    CH_COLLIDER_T* getOrCreateCachedCharacterCollider(const uint64_t ud, const CharacterConfig* inCc, float newRadius, float newHalfHeight);

    Body*          getOrCreateCachedBulletCollider(const uint64_t ud, const float immediateBoxHalfSizeX, const float immediateBoxHalfSizeY, const BulletType blType);

    std::unordered_map<uint32_t, const TriggerConfigFromTiled*> triggerConfigFromTileDict;

    CH_COLLIDER_MAP transientUdToChCollider;
    std::unordered_map<uint64_t, const BodyID*> transientUdToBodyID;

    std::unordered_map<uint64_t, const PlayerCharacterDownsync*> transientUdToCurrPlayer;
    std::unordered_map<uint64_t, PlayerCharacterDownsync*> transientUdToNextPlayer;

    std::unordered_map<uint64_t, const NpcCharacterDownsync*> transientUdToCurrNpc; // Mainly for "Bullet.offender_ud" referencing characters, and avoiding the unnecessary [joinIndex change in `leftShiftDeadNpcs` of DLLMU-v2.3.4](https://github.com/genxium/DelayNoMoreUnity/blob/v2.3.4/shared/Battle_builders.cs#L580)
    std::unordered_map<uint64_t, NpcCharacterDownsync*> transientUdToNextNpc;

    std::unordered_map<uint64_t, const Bullet*> transientUdToCurrBl;
    std::unordered_map<uint64_t, Bullet*> transientUdToNextBl;

    std::unordered_map<uint64_t, const Trigger*> transientUdToCurrTrigger;
    std::unordered_map<uint64_t, Trigger*> transientUdToNextTrigger;

    std::unordered_map<uint64_t, const Trap*> transientUdToCurrTrap;
    std::unordered_map<uint64_t, Trap*> transientUdToNextTrap;

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

    InputFrameDownsync* getOrPrefabInputFrameDownsync(int inIfdId, uint32_t inSingleJoinIndex, uint64_t inSingleInput, bool fromUdp, bool fromTcp, bool& outExistingInputMutated);

    void batchPutIntoPhySysFromCache(const int currRdfId, const RenderFrame* currRdf, RenderFrame* nextRdf);
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

    inline bool isNotDashing(const CharacterDownsync& chd) {
        return (Dashing != chd.ch_state() && Sliding != chd.ch_state() && BackDashing != chd.ch_state() && InAirDashing != chd.ch_state());
    }

    inline bool isEffInAir(const CharacterDownsync& chd, bool notDashing);

    void updateBtnHoldingByInput(const CharacterDownsync& currChd, const InputFrameDecoded& decodedInputHolder, CharacterDownsync* nextChd);

    void derivePlayerOpPattern(int rdfId, const CharacterDownsync& currChd, const CharacterConfig* cc, CharacterDownsync* nextChd, bool currEffInAir, bool notDashing, const InputFrameDecoded& ioIfDecoded, int& outPatternId, bool& outJumpedOrNot, bool& outSlipJumpedOrNot, int& outEffDx, int& outEffDy);

    void deriveNpcOpPattern(int rdfId, const CharacterDownsync& currChd, const CharacterConfig* cc, bool currEffInAir, bool notDashing, const InputFrameDecoded& ifDecoded, int& outPatternId, bool& outJumpedOrNot, bool& outSlipJumpedOrNot, int& outEffDx, int& outEffDy);

    void processSingleCharacterInput(int rdfId, float dt, int patternId, bool jumpedOrNot, bool slipJumpedOrNot, int effDx, int effDy, bool slowDownToAvoidOverlap, const CharacterDownsync& currChd, uint64_t ud, bool currEffInAir, bool currCrouching, bool currOnWall, bool currDashing, bool currWalking, bool currInBlockStun, bool currAtked, bool currParalyzed, const CharacterConfig* cc, CharacterDownsync* nextChd, RenderFrame* nextRdf, bool& usedSkill);

    void transitToGroundDodgedChState(const CharacterDownsync& currChd, CharacterDownsync* nextChd, const CharacterConfig* cc, bool currParalyzed);
    void calcFallenDeath(const RenderFrame* currRdf, RenderFrame* nextRdf);
    void resetJumpStartup(CharacterDownsync* nextChd, bool putBtnHoldingJammed = false);
    bool isInJumpStartup(const CharacterDownsync& cd, const CharacterConfig* cc);
    bool isJumpStartupJustEnded(const CharacterDownsync& currCd, const CharacterDownsync* nextCd, const CharacterConfig* cc);
    void jamBtnHolding(CharacterDownsync* nextChd);
    void processInertiaWalkingHandleZeroEffDx(int currRdfId, float dt, const CharacterDownsync& currChd, CharacterDownsync* nextChd, int effDy, const CharacterConfig* cc, bool effInAir, bool currParalyzed);
    void processInertiaWalking(int rdfId, float dt, const CharacterDownsync& currChd, CharacterDownsync* nextChd, bool currEffInAir, int effDx, int effDy, const CharacterConfig* cc, bool currParalyzed, bool currInBlockStun);
    void processInertiaFlyingHandleZeroEffDxAndDy(int rdfId, float dt, const CharacterDownsync& currChd, CharacterDownsync* nextChd, const CharacterConfig* cc, bool currParalyzed);
    void processInertiaFlying(int rdfId, float dt, const CharacterDownsync& currChd, CharacterDownsync* nextChd, int effDx, int effDy, const CharacterConfig* cc, bool currParalyzed, bool currInBlockStun);
    bool addNewExplosionToNextFrame(int currRdfId, RenderFrame* nextRdf, const Bullet* referenceBullet, const CollideShapeResult& inResult);
    bool addNewBulletToNextFrame(int currRdfId, const CharacterDownsync& currChd, CharacterDownsync* nextChd, const CharacterConfig* cc, bool currParalyzed, bool currEffInAir, const Skill* skillConfig, int activeSkillHit, uint32_t activeSkillId, RenderFrame* nextRdf, const Bullet* referenceBullet, const BulletConfig* referenceBulletConfig, uint64_t offenderUd, int bulletTeamId);

    void prepareJumpStartup(int currRdfId, const CharacterDownsync& currChd, CharacterDownsync* nextChd, bool currEffInAir, const CharacterConfig* cc, bool currParalyzed);
    void processJumpStarted(int currRdfId, const CharacterDownsync& currChd, CharacterDownsync* nextChd, bool currEffInAir, const CharacterConfig* cc);

    void processWallGrabbingPostPhysicsUpdate(int currRdfId, const CharacterDownsync& currChd, CharacterDownsync* nextChd, const CharacterConfig* cc, const CH_COLLIDER_T* cv, bool inJumpStartupOrJustEnded);

    bool transitToDying(const int currRdfId, const CharacterDownsync& currChd, const bool cvInAir, CharacterDownsync* nextChd);
    bool transitToDying(const int currRdfId, const PlayerCharacterDownsync& currPlayer, const bool cvInAir, PlayerCharacterDownsync* nextPlayer);
    bool transitToDying(const int currRdfId, const NpcCharacterDownsync& currNpc, const bool cvInAir, NpcCharacterDownsync* nextNpc);

    void processDelayedBulletSelfVel(int rdfId, const CharacterDownsync& currChd, CharacterDownsync* nextChd, const CharacterConfig* cc, bool currParalyzed, bool nextEffInAir);

    virtual void stepSingleChdState(const int currRdfId, const RenderFrame* currRdf, RenderFrame* nextRdf, const float dt, const uint64_t ud, const uint64_t udt, const CharacterConfig* cc, CH_COLLIDER_T* single, const CharacterDownsync& currChd, CharacterDownsync* nextChd, bool& groundBodyIsChCollider, bool& isDead, bool& cvOnWall, bool& cvSupported, bool& cvInAir, bool& inJumpStartupOrJustEnded, CharacterBase::EGroundState& cvGroundState);
    virtual void postStepSingleChdStateCorrection(const int currRdfId, const uint64_t udt, const uint64_t ud, const CH_COLLIDER_T* chCollider, const CharacterDownsync& currChd, CharacterDownsync* nextChd, const CharacterConfig* cc, bool cvSupported, bool cvInAir, bool cvOnWall, bool currNotDashing, bool currEffInAir, bool oldNextNotDashing, bool oldNextEffInAir, bool inJumpStartupOrJustEnded, CharacterBase::EGroundState cvGroundState);

    virtual void stepSingleTriggerState(int currRdfId, const Trigger& currTrigger, Trigger* nextTrigger);

    void leftShiftDeadNpcs(int currRdfId, RenderFrame* nextRdf);
    void leftShiftDeadBullets(int currRdfId, RenderFrame* nextRdf);
    void leftShiftDeadTriggers(int currRdfId, RenderFrame* nextRdf);
    void leftShiftDeadPickables(int currRdfId, RenderFrame* nextRdf);

    void leftShiftDeadBlImmuneRecords(int currRdfId, CharacterDownsync* nextChd);
    void leftShiftDeadIvSlots(int currRdfId, CharacterDownsync* nextChd);
    void leftShiftDeadBuffs(int currRdfId, CharacterDownsync* nextChd);
    void leftShiftDeadDebuffs(int currRdfId, CharacterDownsync* nextChd);

    bool useSkill(int currRdfId, RenderFrame* nextRdf, const CharacterDownsync& currChd, uint64_t ud, const CharacterConfig* cc, CharacterDownsync* nextChd, int effDx, int effDy, int patternId, bool currEffInAir, bool currCrouching, bool currOnWall, bool currDashing, bool currWalking, bool currInBlockStun, bool currAtked, bool currParalyzed, int& outSkillId, const Skill*& outSkill, const BulletConfig*& outPivotBc);

    void useInventorySlot(int currRdfId, int patternId, const CharacterDownsync& currChd, const CharacterConfig* cc, CharacterDownsync* nextChd, bool& outSlotUsed, bool& outDodgedInBlockStun);
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

    inline bool decodeInput(uint64_t encodedInput, InputFrameDecoded* holder);

    CH_COLLIDER_T* createDefaultCharacterCollider(const CharacterConfig* cc);
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

    Body*             createDefaultBulletCollider(const float immediateBoxHalfSizeX, const float immediateBoxHalfSizeY, float& outConvexRadius, const EMotionType motionType = EMotionType::Static, const bool isSensor = true);

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

    inline bool isBulletVanishing(const Bullet* bullet, const BulletConfig* bc) {
        return BulletState::Vanishing == bullet->bl_state();
    }

    inline bool isBulletExploding(const Bullet* bullet, const BulletConfig* bc) {
        switch (bc->b_type()) {
        case BulletType::Melee:
            return ((BulletState::Exploding == bullet->bl_state() || BulletState::Vanishing == bullet->bl_state()) && bullet->frames_in_bl_state() < bc->explosion_frames());
        case BulletType::MechanicalCartridge:
        case BulletType::MagicalFireball:
        case BulletType::GroundWave:
            return (BulletState::Exploding == bullet->bl_state() || BulletState::Vanishing == bullet->bl_state());
        default:
            return false;
        }
    }

    inline bool isBulletStartingUp(const Bullet* bullet, const BulletConfig* bc, int currRdfId) {
        return BulletState::StartUp == bullet->bl_state();
    }

    inline bool isBulletActive(const Bullet* bullet) {
        return (BulletState::Active == bullet->bl_state());
    }

    inline bool isBulletJustActive(const Bullet* bullet, const BulletConfig* bc, int currRdfId) {
        // [WARNING] Practically a bullet might propagate for a few render frames before hitting its visually "VertMovingTrapLocalIdUponActive"!
        int visualBufferRdfCnt = 3;
        if (BulletState::Active == bullet->bl_state()) {
            return visualBufferRdfCnt >= bullet->frames_in_bl_state();
        } else {
            return false;
        }
    }

    inline bool isTriggerAlive(const Trigger* trigger, int currRdfId) {
        return 0 < trigger->demanded_evt_mask() || 0 < trigger->quota();
    }

    inline bool isPickableAlive(const Pickable* pickable, int currRdfId) {
        return 0 < pickable->remaining_lifetime_rdf_count();
    }

    inline bool isBulletAlive(const Bullet* bullet, const BulletConfig* bc, int currRdfId) {
        if (BulletState::Vanishing == bullet->bl_state()) {
            return bullet->frames_in_bl_state() < bc->active_frames() + bc->explosion_frames();
        }
        if (BulletState::Exploding == bullet->bl_state() && MultiHitType::FromEmission != bc->mh_type()) {
            return bullet->frames_in_bl_state() < bc->active_frames();
        }
        return (currRdfId < bullet->originated_render_frame_id() + bc->startup_frames() + bc->active_frames() + bc->cooldown_frames());
    }

    inline bool isNpcJustDead(const CharacterDownsync* chd) {
        return (0 >= chd->hp() && globalPrimitiveConsts->dying_frames_to_recover() == chd->frames_to_recover());
    }

    inline bool isNpcDeadToDisappear(const CharacterDownsync* chd) {
        return (0 >= chd->hp() && 0 >= chd->frames_to_recover());
    }

    inline bool publishNpcExhaustedEvt(int rdfId, uint64_t publishingEvtMask, uint64_t offenderUd, int offenderBulletTeamId, Trigger* nextRdfTrigger) {
        if (0 == nextRdfTrigger->demanded_evt_mask()) return false;
        nextRdfTrigger->set_fulfilled_evt_mask(nextRdfTrigger->fulfilled_evt_mask() | publishingEvtMask);
        nextRdfTrigger->set_offender_ud(offenderUd);
        nextRdfTrigger->set_offender_bullet_team_id(offenderBulletTeamId);
        return true;
    }

    inline bool isBattleSettled(const RenderFrame* rdf) {
        if (rdf->has_prev_rdf_step_result()) {
            auto& prevRdfStepResult = rdf->prev_rdf_step_result();
            for (int i = 0; i < prevRdfStepResult.fulfilled_triggers_size(); i++) {
                auto& fulfilledTrigger = prevRdfStepResult.fulfilled_triggers(i);
                if (globalPrimitiveConsts->tt_victory() == fulfilledTrigger.trigger_type()) {
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
    virtual JPH::ValidateResult validateLhsCharacterContact(const CharacterDownsync* lhsCurrChd, const CharacterDownsync* rhsCurrChd) {
        if (lhsCurrChd->bullet_team_id() == rhsCurrChd->bullet_team_id()) {
            return JPH::ValidateResult::RejectContact;
        }
        if (invinsibleSet.count(lhsCurrChd->ch_state()) || invinsibleSet.count(rhsCurrChd->ch_state())) {
            return JPH::ValidateResult::RejectContact;
        }
        return JPH::ValidateResult::AcceptContact;
    }

    virtual JPH::ValidateResult validateLhsCharacterContact(const CharacterDownsync* lhsCurrChd, const Bullet* rhsCurrBl) {
        if (lhsCurrChd->bullet_team_id() == rhsCurrBl->team_id()) {
            return JPH::ValidateResult::RejectContact;
        }

        if (!isBulletActive(rhsCurrBl)) {
            return JPH::ValidateResult::RejectContact;
        }

        if (invinsibleSet.count(lhsCurrChd->ch_state())) {
            return JPH::ValidateResult::RejectContact;
        }
        return JPH::ValidateResult::AcceptContact;
    }

    virtual JPH::ValidateResult validateLhsCharacterContact(const CharacterDownsync* lhsCurrChd, const uint64_t udRhs, const uint64_t udtRhs) {
        switch (udtRhs) {
        case UDT_PLAYER: {
            auto rhsCurrPlayer = transientUdToCurrPlayer[udRhs];
            auto& rhsCurrChd = rhsCurrPlayer->chd();
            return validateLhsCharacterContact(lhsCurrChd, &rhsCurrChd);
        }
        case UDT_NPC: {
            auto rhsCurrNpc = transientUdToCurrNpc[udRhs];
            auto& rhsCurrChd = rhsCurrNpc->chd();
            return validateLhsCharacterContact(lhsCurrChd, &rhsCurrChd);
        }
        case UDT_BL: {
            auto& rhsCurrBl = transientUdToCurrBl[udRhs];
            return validateLhsCharacterContact(lhsCurrChd, rhsCurrBl);
        }
        default:
            return JPH::ValidateResult::AcceptContact;
        }
    }

    virtual JPH::ValidateResult validateLhsCharacterContact(const uint64_t udLhs, const uint64_t udtLhs,
        const JPH::Body& lhs, // the "Character"
        const uint64_t udRhs, const uint64_t udtRhs, const JPH::Body& rhs) {
        switch (udtLhs) {
            case UDT_PLAYER: {
                auto& lhsCurrPlayer = transientUdToCurrPlayer[udLhs];
                auto& lhsCurrChd = lhsCurrPlayer->chd();
                return validateLhsCharacterContact(&lhsCurrChd, udRhs, udtRhs);
                break;
            }
            case UDT_NPC: {
                auto& lhsCurrNpc = transientUdToCurrNpc[udLhs];
                auto& lhsCurrChd = lhsCurrNpc->chd();
                return validateLhsCharacterContact(&lhsCurrChd, udRhs, udtRhs);
                break;
            }
            default:
                return JPH::ValidateResult::AcceptContact;
        }
    }

    virtual JPH::ValidateResult validateLhsBulletContact(const Bullet* lhsCurrBl, const uint64_t udRhs, const uint64_t udtRhs) {
        switch (udtRhs) {
        case UDT_PLAYER: {
            auto& rhsCurrPlayer = transientUdToCurrPlayer[udRhs];
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
            auto& rhsCurrNpc = transientUdToCurrNpc[udRhs];
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
            auto& rhsCurrBl = transientUdToCurrBl[udRhs];
            if (lhsCurrBl->team_id() == rhsCurrBl->team_id()) {
                return JPH::ValidateResult::RejectContact;
            }
            if (!isBulletActive(lhsCurrBl)) {
                return JPH::ValidateResult::RejectContact;
            }
            if (!isBulletActive(rhsCurrBl)) {
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
        const uint64_t udRhs, const uint64_t udtRhs, const JPH::Body& rhs) {
        auto& lhsCurrBl = transientUdToCurrBl[udLhs];
        return validateLhsBulletContact(lhsCurrBl, udRhs, udtRhs);
    }

    virtual void handleLhsCharacterCollision(
        const int currRdfId,
        RenderFrame* nextRdf,
        const uint64_t udLhs, const uint64_t udtLhs, const CharacterDownsync* currChd, CharacterDownsync* nextChd,
        const uint64_t udRhs, const uint64_t udtRhs,
        const CollideShapeResult& inResult,
        uint32_t& outNewEffDebuffSpeciesId, int& outNewDamage, bool& outNewEffBlownUp, int& outNewEffFramesToRecover, float& outNewEffPushbackVelX, float& outNewEffPushbackVelY);

    virtual void handleLhsBulletCollision(
        const int currRdfId,
        RenderFrame* nextRdf,
        const uint64_t udLhs, const uint64_t udtLhs, const Bullet* currBl, Bullet* nextBl,
        const uint64_t udRhs, const uint64_t udtRhs, 
        const JPH::CollideShapeResult& inResult);

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
    On the other hand, it's INTENTIONALLY AVOIDED to call "CharacterCollideShapeCollector" or "handleLhsBulletCollision" within "OnContactCommon" -- take "Character" as an example,  
    - during broadphase multi-threading, a same "Character" might enter "OnContactCommon" from different threads concurrently, if we'd like to manage "CharacterCollideShapeCollector.mBestDot" in a thread-safe manner, the same "do { ...... compare_exchange_weak(...) ...... } while(0 < retryCnt)" trick in "RingBufferMt::DryPut/Pop/PopTail" can be used; 
    - HOWEVER it's NOT worth such hassle! It's much simpler to handle "same-(character/bullet/trap)-narrowphase-in-same-thread" in later traversal (i.e. to call "PostSimulationWithCollector -> CharacterCollideShapeCollector") -- by ONLY making use of broadphase multithreading to solve constraints (including regular "Constraint" and "ContactConstraint") we balance both efficiency and simplicity.  
    */
    virtual void OnContactCommon(const JPH::Body& inBody1,
        const JPH::Body& inBody2,
        const JPH::ContactManifold& inManifold,
        const JPH::ContactSettings& ioSettings);

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
