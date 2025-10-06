#ifndef BASE_BATTLE_H_
#define BASE_BATTLE_H_ 1

#include "joltc_export.h"
#include <Jolt/Jolt.h>
#include <Jolt/Core/Reference.h>
#include <Jolt/Math/Float2.h>

#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/Character/Character.h>
#include <Jolt/Physics/Body/Body.h>
#include "FrameRingBuffer.h"
#include "CppOnlyConsts.h"
#include "PbConsts.h"
#include <Jolt/Physics/StateRecorderImpl.h>

#include "serializable_data.pb.h"

#include "CollisionLayers.h"
#include "CollisionCallbacks.h"
#include <vector>
#include <map>
#include <set>
#include <deque>
#include <google/protobuf/arena.h>

#define BL_COLLIDER_T JPH::Body
#define BL_CACHE_KEY_T std::vector<float>
#define BL_COLLIDER_Q std::vector<BL_COLLIDER_T*>
#define CH_CACHE_KEY_T std::vector<float>

/*
[REMINDER] 

It's by design that "JPH::Character" instead of "JPH::CharacterVirtual" is used here while [their differences]("https://jrouwe.github.io/JoltPhysics/index.html#character-controllers) are understood.

The lack of use of broadphase makes "JPH::CharacterVirtual" very inefficient in "Character v.s. Character" collision handling. See [CharacterVsCharacterCollisionSimple](https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Physics/Character/CharacterVirtual.cpp#L34) for its official default implementation.
*/
#define CH_COLLIDER_T JPH::Character
#define CH_COLLIDER_Q std::vector<CH_COLLIDER_T*>
typedef struct VectorFloatHasher {
    std::size_t operator()(const std::vector<float>& v) const {
        std::size_t seed = v.size(); // Start with the size of the vector
        for (float i : v) {
            seed ^= std::hash<float>()(i) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }
} COLLIDER_HASH_KEY_T;

// All Jolt symbols are in the JPH namespace
using namespace JPH;
using namespace jtshared;

class JOLTC_EXPORT BaseBattle : public JPH::ContactListener {
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

    BL_COLLIDER_Q activeBlColliders;
    BL_COLLIDER_Q* blStockCache;
    std::unordered_map< BL_CACHE_KEY_T, BL_COLLIDER_Q, COLLIDER_HASH_KEY_T > cachedBlColliders; // Key is "{(default state) halfExtent}", where "convexRadius" is determined by "halfExtent"

    CH_COLLIDER_Q activeChColliders;

    /*
     [TODO]

     Make "cachedChColliders" keyed by "CharacterConfig.species_id()" to fit the need of multi-shape character (at different ch_state).

    It's by design that "ScaledShape" is NOT used here, because when mixed with translation and rotation, the order of affine transforms matters but is difficult to keep in mind.

    Moreover, by using this approach to manage multi-shape character I dropped the "shared shapes across bodies" feature of Jolt.
    */
    std::unordered_map< CH_CACHE_KEY_T, CH_COLLIDER_Q, COLLIDER_HASH_KEY_T > cachedChColliders; // Key is "{(default state) radius, halfHeight}", kindly note that position and orientation of "Character" are mutable during reuse, thus not using "RefConst<>".

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
        auto oldVal = inactiveJoinMask.fetch_and(allConfirmedMask ^ calcJoinIndexMask(joinIndex));
        return inactiveJoinMask;
    }

    inline uint64_t SetPlayerInactive(uint32_t joinIndex) {
        auto oldVal = inactiveJoinMask.fetch_or(calcJoinIndexMask(joinIndex));
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
    virtual bool Step(int fromRdfId, int toRdfId, DownsyncSnapshot* virtualIfds = nullptr);

    virtual void Clear();

    virtual bool ResetStartRdf(char* inBytes, int inBytesCnt);
    virtual bool ResetStartRdf(const WsReq* initializerMapData);

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
        return IsLengthDiffNearlySame(rhs - lhs);
    }

    inline static void AssertNearlySame(const PlayerCharacterDownsync& lhs, const PlayerCharacterDownsync& rhs) {
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
        JPH_ASSERT(lhs.frames_captured_by_inertia() == rhs.frames_captured_by_inertia());
        JPH_ASSERT(lhs.frames_invinsible() == rhs.frames_invinsible());
        JPH_ASSERT(lhs.frames_to_recover() == rhs.frames_to_recover());
        JPH_ASSERT(lhs.frames_to_start_jump() == rhs.frames_to_start_jump());
        JPH_ASSERT(lhs.remaining_air_dash_quota() == rhs.remaining_air_dash_quota());
        JPH_ASSERT(lhs.remaining_air_jump_quota() == rhs.remaining_air_jump_quota());
        JPH_ASSERT(lhs.remaining_def1_quota() == rhs.remaining_def1_quota());
        JPH_ASSERT(lhs.slip_jump_triggered() == rhs.slip_jump_triggered());
        JPH_ASSERT(lhs.jump_triggered() == rhs.jump_triggered());
        JPH_ASSERT(lhs.jump_started() == rhs.jump_started());
        JPH_ASSERT(isNearlySame(lhs.x(), lhs.y(), lhs.z(), rhs.x(), rhs.y(), rhs.z()));
        JPH_ASSERT(lhs.dir_x() == rhs.dir_x());
        JPH_ASSERT(lhs.dir_y() == rhs.dir_y());
        JPH_ASSERT(isNearlySame(lhs.vel_x(), lhs.vel_y(), lhs.vel_z(), rhs.vel_x(), rhs.vel_y(), rhs.vel_z()));
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

protected:
    BodyIDVector staticColliderBodyIDs;
    BodyIDVector bodyIDsToClear;
    BodyIDVector bodyIDsToAdd;
    BodyIDVector bodyIDsToActivate;

    // Backend & Frontend shared functions
    inline void elapse1RdfForRdf(int currRdfId, RenderFrame* nextRdf);
    inline void elapse1RdfForBl(int currRdfId, Bullet* bl, const Skill* skill, const BulletConfig* bc);
    inline void elapse1RdfForPlayerChd(PlayerCharacterDownsync* playerChd, const CharacterConfig* cc);
    inline void elapse1RdfForNpcChd(NpcCharacterDownsync* npcChd, const CharacterConfig* cc);
    inline void elapse1RdfForChd(CharacterDownsync* cd, const CharacterConfig* cc);
    inline void elapse1RdfForPickable(Pickable* pk);

    int moveForwardLastConsecutivelyAllConfirmedIfdId(int proposedIfdEdFrameId, uint64_t skippableJoinMask = 0);

    inline uint64_t calcUserData(const PlayerCharacterDownsync& playerChd) {
        return UDT_PLAYER + playerChd.join_index();
    }

    inline uint64_t calcUserData(const NpcCharacterDownsync& npcChd) {
        return UDT_NPC + npcChd.id();
    }

    inline uint64_t calcUserData(const Bullet& bl) {
        return UDT_BL + bl.id();
    }

    inline uint64_t calcStaticColliderUserData(const int staticColliderId) {
        return UDT_OBSTACLE + staticColliderId;
    }

    inline uint64_t getUDT(const uint64_t& ud) {
        return (ud & UDT_STRIPPER);
    }

    inline uint32_t getUDPayload(const uint64_t& ud) {
        return (ud & UD_PAYLOAD_STRIPPER);
    }

    CH_COLLIDER_T* getOrCreateCachedPlayerCollider(const uint64_t ud, const PlayerCharacterDownsync& currPlayer, const CharacterConfig* cc, PlayerCharacterDownsync* nextPlayer = nullptr);
    // Unlike DLLMU-v2.3.4, even if "preallocateNpcDict" is empty, "getOrCreateCachedNpcCollider" still works
    CH_COLLIDER_T* getOrCreateCachedNpcCollider(const uint64_t ud, const NpcCharacterDownsync& currNpc, const CharacterConfig* cc, NpcCharacterDownsync* nextNpc = nullptr);
    CH_COLLIDER_T* getOrCreateCachedCharacterCollider(const uint64_t ud, const CharacterConfig* inCc, float newRadius, float newHalfHeight);

    Body*             getOrCreateCachedBulletCollider(const uint64_t ud, const float immediateBoxHalfSizeX, const float immediateBoxHalfSizeY, const BulletType blType);

    std::unordered_map<uint64_t, CH_COLLIDER_T*> transientUdToChCollider;
    std::unordered_map<uint64_t, const BodyID*> transientUdToBodyID;

    std::unordered_map<uint64_t, const PlayerCharacterDownsync*> transientUdToCurrPlayer;
    std::unordered_map<uint64_t, PlayerCharacterDownsync*> transientUdToNextPlayer;

    std::unordered_map<uint64_t, const NpcCharacterDownsync*> transientUdToCurrNpc; // Mainly for "Bullet.offender_ud" referencing characters, and avoiding the unnecessary [joinIndex change in `leftShiftDeadNpcs` of DLLMU-v2.3.4](https://github.com/genxium/DelayNoMoreUnity/blob/v2.3.4/shared/Battle_builders.cs#L580)
    std::unordered_map<uint64_t, NpcCharacterDownsync*> transientUdToNextNpc;

    std::unordered_map<uint64_t, const Bullet*> transientUdToCurrBl;
    std::unordered_map<uint64_t, Bullet*> transientUdToNextBl;

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

    inline uint64_t calcJoinIndexMask(uint32_t joinIndex) {
        if (0 == joinIndex) return 0;
        return (U64_1 << (joinIndex - 1));
    }

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
        return (Dashing != chd.ch_state() && Sliding != chd.ch_state() && BackDashing != chd.ch_state());
    }

    inline bool isEffInAir(const CharacterDownsync& chd, bool notDashing);

    void updateBtnHoldingByInput(const CharacterDownsync& currChd, const InputFrameDecoded& decodedInputHolder, CharacterDownsync* nextChd);

    void derivePlayerOpPattern(int rdfId, const CharacterDownsync& currChd, const CharacterConfig* cc, CharacterDownsync* nextChd, bool currEffInAir, bool notDashing, const InputFrameDecoded& ioIfDecoded, int& outPatternId, bool& outJumpedOrNot, bool& outSlipJumpedOrNot, int& outEffDx, int& outEffDy);

    void deriveNpcOpPattern(int rdfId, const CharacterDownsync& currChd, const CharacterConfig* cc, bool currEffInAir, bool notDashing, const InputFrameDecoded& ifDecoded, int& outPatternId, bool& outJumpedOrNot, bool& outSlipJumpedOrNot, int& outEffDx, int& outEffDy);

    void processPlayerInputs(const int rdfId, const RenderFrame* currRdf, float dt, RenderFrame* nextRdf, const int delayedIfdId, const InputFrameDownsync* delayedInputFrameDownsync);
    void processNpcInputs(const int rdfId, const RenderFrame* currRdf, float dt, RenderFrame* nextRdf, const InputFrameDownsync* delayedIfd);

    void processSingleCharacterInput(int rdfId, float dt, int patternId, bool jumpedOrNot, bool slipJumpedOrNot, int effDx, int effDy, bool slowDownToAvoidOverlap, const CharacterDownsync& currChd, uint64_t ud, bool currEffInAir, bool currCrouching, bool currOnWall, bool currDashing, bool currWalking, bool currInBlockStun, bool currAtked, bool currParalyzed, const CharacterConfig* cc, CharacterDownsync* nextChd, RenderFrame* nextRdf);

    void transitToGroundDodgedChState(CharacterDownsync* nextChd, const CharacterConfig* cc, bool currParalyzed);
    void calcFallenDeath(RenderFrame* nextRdf);
    void resetJumpStartup(CharacterDownsync* nextChd, bool putBtnHoldingJammed = false);
    bool isInJumpStartup(const CharacterDownsync& cd, const CharacterConfig* cc);
    bool isJumpStartupJustEnded(const CharacterDownsync& currCd, const CharacterDownsync* nextCd, const CharacterConfig* cc);
    void jamBtnHolding(CharacterDownsync* nextChd);
    void processInertiaWalkingHandleZeroEffDx(int currRdfId, float dt, const CharacterDownsync& currChd, CharacterDownsync* nextChd, int effDy, const CharacterConfig* cc, bool effInAir, bool currParalyzed);
    void processInertiaWalking(int rdfId, float dt, const CharacterDownsync& currChd, CharacterDownsync* nextChd, bool currEffInAir, int effDx, int effDy, const CharacterConfig* cc, bool currParalyzed, bool currInBlockStun);
    void processInertiaFlyingHandleZeroEffDxAndDy(int rdfId, float dt, const CharacterDownsync& currChd, CharacterDownsync* nextChd, const CharacterConfig* cc, bool currParalyzed);
    void processInertiaFlying(int rdfId, float dt, const CharacterDownsync& currChd, CharacterDownsync* nextChd, int effDx, int effDy, const CharacterConfig* cc, bool currParalyzed, bool currInBlockStun);
    bool addNewBulletToNextFrame(int rdfId, const CharacterDownsync& currChd, CharacterDownsync* nextChd, const CharacterConfig* cc, bool currParalyzed, bool currEffInAir, int xfac, int yfac, const Skill* skillConfig, int activeSkillHit, uint32_t activeSkillId, RenderFrame* nextRdf, const Bullet* referenceBullet, const BulletConfig* referenceBulletConfig, uint64_t offenderUd, int bulletTeamId);

    void prepareJumpStartup(int currRdfId, const CharacterDownsync& currChd, CharacterDownsync* nextChd, bool currEffInAir, const CharacterConfig* cc, bool currParalyzed);
    void processJumpStarted(int currRdfId, const CharacterDownsync& currChd, CharacterDownsync* nextChd, bool currEffInAir, const CharacterConfig* cc);

    void processWallGrabbingPostPhysicsUpdate(int currRdfId, const CharacterDownsync& currChd, CharacterDownsync* nextChd, const CharacterConfig* cc, const CH_COLLIDER_T* cv, bool inJumpStartupOrJustEnded);

    void processDelayedBulletSelfVel(int rdfId, const CharacterDownsync& currChd, CharacterDownsync* nextChd, const CharacterConfig* cc, bool currParalyzed, bool nextEffInAir);

    virtual void postStepSingleChdStateCorrection(const int currRdfId, const uint64_t udt, const uint64_t ud, const CH_COLLIDER_T* chCollider, const CharacterDownsync& currChd, CharacterDownsync* nextChd, const CharacterConfig* cc, bool cvSupported, bool cvInAir, bool cvOnWall, bool currNotDashing, bool currEffInAir, bool oldNextNotDashing, bool oldNextEffInAir, bool inJumpStartupOrJustEnded, CharacterBase::EGroundState cvGroundState);
    void leftShiftDeadNpcs(int currRdfId, RenderFrame* nextRdf);
    void leftShiftDeadBullets(int currRdfId, RenderFrame* nextRdf);
    void leftShiftDeadPickables(int currRdfId, RenderFrame* nextRdf);

    bool useSkill(int rdfId, RenderFrame* nextRdf, const CharacterDownsync& currChd, uint64_t ud, const CharacterConfig* cc, CharacterDownsync* nextChd, int effDx, int effDy, int patternId, bool currEffInAir, bool currCrouching, bool currOnWall, bool currDashing, bool currWalking, bool currInBlockStun, bool currAtked, bool currParalyzed, int& outSkillId, const Skill*& outSkill, const BulletConfig*& outPivotBc);

    void useInventorySlot(int rdfId, int patternId, const CharacterDownsync& currChd, const CharacterConfig* cc, CharacterDownsync* nextChd, bool& outSlotUsed, bool& outDodgedInBlockStun);
    static bool IsChargingAtkChState(CharacterState chState) {
        return (CharacterState::Atk7_Charging == chState);
    }

    inline const CharacterConfig* getCc(uint32_t speciesId) {
        auto& ccs = globalConfigConsts->character_configs();
        if (!ccs.contains(speciesId)) {
            return nullptr;
        }
        auto& v = ccs.at(speciesId);
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

    Body*             createDefaultBulletCollider(const float immediateBoxHalfSizeX, const float immediateBoxHalfSizeY, float& outConvexRadius, const EMotionType motionType = EMotionType::Static);

    void preallocateBodies(const RenderFrame* startRdf, const ::google::protobuf::Map<::google::protobuf::int32, ::google::protobuf::int32 >& preallocateNpcSpeciesDict);

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

    inline bool isBulletActive(const Bullet* bullet, const BulletConfig* bc, int currRdfId) {
        if (BulletState::Exploding == bullet->bl_state() || BulletState::Vanishing == bullet->bl_state()) {
            return false;
        }
        return (bullet->originated_render_frame_id() + bc->startup_frames() < currRdfId) && (currRdfId < bullet->originated_render_frame_id() + bc->startup_frames() + bc->active_frames());
    }

    inline bool isBulletJustActive(const Bullet* bullet, const BulletConfig* bc, int currRdfId) {
        if (BulletState::Exploding == bullet->bl_state() || BulletState::Vanishing == bullet->bl_state()) {
            return false;
        }
        // [WARNING] Practically a bullet might propagate for a few render frames before hitting its visually "VertMovingTrapLocalIdUponActive"!
        int visualBufferRdfCnt = 3;
        if (BulletState::Active == bullet->bl_state()) {
            return visualBufferRdfCnt >= bullet->frames_in_bl_state();
        }
        return (bullet->originated_render_frame_id() + bc->startup_frames() < currRdfId && currRdfId <= bullet->originated_render_frame_id() + bc->startup_frames() + visualBufferRdfCnt);
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
        return (currRdfId < bullet->originated_render_frame_id() + bc->startup_frames() + bc->active_frames());
    }

    inline bool isNpcJustDead(const CharacterDownsync* chd) {
        return (0 >= chd->hp() && globalPrimitiveConsts->dying_frames_to_recover() == chd->frames_to_recover());
    }

    inline bool isNpcDeadToDisappear(const CharacterDownsync* chd) {
        return (0 >= chd->hp() && 0 >= chd->frames_to_recover());
    }

    inline bool publishNpcKilledEvt(int rdfId, uint64_t publishingEvtMask, uint64_t offenderUd, int offenderBulletTeamId, Trigger* nextRdfTrigger) {
        if (globalPrimitiveConsts->terminating_evtsub_id() == nextRdfTrigger->demanded_evt_mask()) return false;
        nextRdfTrigger->set_fulfilled_evt_mask(nextRdfTrigger->fulfilled_evt_mask() | publishingEvtMask);
        nextRdfTrigger->set_offender_ud(offenderUd);
        nextRdfTrigger->set_offender_bullet_team_id(offenderBulletTeamId);
#ifndef NDEBUG
        //Debug::Log(String.Format("@rdfId={0}, published evtmask = {1} to trigger editor id = {2}, local id = {3}, fulfilledEvtMask = {4}, demandedEvtMask = {5} by npc killed", rdfId, publishingEvtMask, nextRdfTrigger.EditorId, nextRdfTrigger.TriggerLocalId, nextRdfTrigger.FulfilledEvtMask, nextRdfTrigger.DemandedEvtMask));
#endif // !NDEBUG
        return true;
    }
    
    virtual bool allocPhySys();
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
    // #JPH::ContactListener
    /*
    [WARNING] For rollback-netcode implementation, only "OnContactAdded" and "OnContactPersisted" are to be concerned with, MOREOVER there's NO NEED to clear "contact cache" upon each "Step".

    In a Jolt PhysicsSystem, all EMotionType pairs will trigger "OnContactAdded" or "OnContactPersisted" when they come into contact geometrically (e.g. Kinematic v.s. Kinematic, Kinematic v.s. Static), but NOT ALL EMotionType pairs will create "ContactConstraint" instances (e.g. Kinematic v.s. Kinematic, Kinematic v.s. Static, see https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Physics/Constraints/ContactConstraintManager.cpp#L1108).
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

protected:
    virtual JPH::ValidateResult validateLhsCharacterContact(const uint64_t udLhs, const uint64_t udtLhs,
        const JPH::Body& lhs, // the "Character"
        const uint64_t udRhs, const uint64_t udtRhs, const JPH::Body& rhs);

    virtual JPH::ValidateResult validateLhsBulletContact(const uint64_t udLhs,
        const JPH::Body& lhs, // the "Bullet"
        const uint64_t udRhs, const uint64_t udtRhs, const JPH::Body& rhs);

    virtual void handleLhsCharacterCollision(
        const uint64_t udLhs, const uint64_t udtLhs,
        const JPH::Body& lhs, // the "Character"
        const uint64_t udRhs, const uint64_t udtRhs, const JPH::Body& rhs,
        const JPH::ContactManifold& inManifold,
        const JPH::ContactSettings& ioSettings);

    virtual void handleLhsBulletCollision(
        const uint64_t udLhs,
        const JPH::Body& lhs, // the "Bullet"
        const uint64_t udRhs, const uint64_t udtRhs, const JPH::Body& rhs,
        const JPH::ContactManifold& inManifold,
        const JPH::ContactSettings& ioSettings);
};

#endif
