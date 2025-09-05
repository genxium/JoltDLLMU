#ifndef BASE_BATTLE_H_
#define BASE_BATTLE_H_ 1

#include "joltc_export.h"
#include <Jolt/Jolt.h>
#include <Jolt/Core/Reference.h>
#include <Jolt/Math/Float2.h>

#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Body/Body.h>
#include "FrameRingBuffer.h"
#include "CppOnlyConsts.h"
#include "PbConsts.h"

#include "serializable_data.pb.h"

#include "CollisionLayers.h"
#include "CollisionCallbacks.h"
#include <vector>
#include <map>
#include <set>
#include <deque>
#include <google/protobuf/arena.h>

#define BL_CACHE_KEY_T std::vector<float>
#define BL_COLLIDER_Q std::vector<JPH::Body*>
#define CH_CACHE_KEY_T std::vector<float>
#define CH_COLLIDER_Q std::vector<JPH::CharacterVirtual*>
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

class JOLTC_EXPORT BaseBattle : public JPH::ContactListener, JPH::CharacterContactListener {
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
    std::unordered_map< BL_CACHE_KEY_T, BL_COLLIDER_Q, COLLIDER_HASH_KEY_T > cachedBlColliders; // Key is "{(default state) halfExtent}", where "convexRadius" is determined by "halfExtent"

    // It's by design that "JPH::CharacterVirtual" instead of "JPH::Character" or even "JPH::Body" is used here, see "https://jrouwe.github.io/JoltPhysics/index.html#character-controllers" for their differences.
    CH_COLLIDER_Q activeChColliders;
    /*
     [TODO]

     Make "cachedChColliders" keyed by "CharacterConfig.species_id()" to fit the need of multi-shape character (at different ch_state).

    It's by design that "ScaledShape" is NOT used here, because when mixed with translation and rotation, the order of affine transforms matters but is difficult to keep in mind.

    Moreover, by using this approach to manage multi-shape character I dropped the "shared shapes across bodies" feature of Jolt.
    */
    std::unordered_map< CH_CACHE_KEY_T, CH_COLLIDER_Q, COLLIDER_HASH_KEY_T > cachedChColliders; // Key is "{(default state) radius, halfHeight}", kindly note that position and orientation of "CharacterVirtual" are mutable during reuse, thus not using "RefConst<>".

public:
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

    virtual bool Step(int fromRdfId, int toRdfId, DownsyncSnapshot* virtualIfds = nullptr);

    virtual void Clear();

    virtual bool ResetStartRdf(char* inBytes, int inBytesCnt);
    virtual bool ResetStartRdf(const WsReq* initializerMapData);

    static void NewPreallocatedBullet(Bullet* single, int bulletLocalId, int originatedRenderFrameId, int teamId, BulletState blState, int framesInBlState);
    static void NewPreallocatedCharacterDownsync(CharacterDownsync* single, int buffCapacity, int debuffCapacity, int inventoryCapacity, int bulletImmuneRecordCapacity);
    static void NewPreallocatedPlayerCharacterDownsync(PlayerCharacterDownsync* single, int buffCapacity, int debuffCapacity, int inventoryCapacity, int bulletImmuneRecordCapacity);
    static void NewPreallocatedNpcCharacterDownsync(NpcCharacterDownsync* single, int buffCapacity, int debuffCapacity, int inventoryCapacity, int bulletImmuneRecordCapacity);
    static RenderFrame* NewPreallocatedRdf(int roomCapacity, int preallocNpcCount, int preallocBulletCount);

    inline static int EncodePatternForCancelTransit(int patternId, bool currEffInAir, bool currCrouching, bool currOnWall, bool currDashing) {
        /*
        For simplicity,
        - "currSliding" = "currCrouching" + "currDashing"
        */
        int encodedPatternId = patternId;
        if (currEffInAir) {
            encodedPatternId += (1L << 16);
        }
        if (currCrouching) {
            encodedPatternId += (1L << 17);
        }
        if (currOnWall) {
            encodedPatternId += (1L << 18);
        }
        if (currDashing) {
            encodedPatternId += (1L << 19);
        }
        return encodedPatternId;
    }

    inline static int EncodePatternForInitSkill(int patternId, bool currEffInAir, bool currCrouching, bool currOnWall, bool currDashing, bool currInBlockStun, bool currAtked, bool currParalyzed) {
        int encodedPatternId = EncodePatternForCancelTransit(patternId, currEffInAir, currCrouching, currOnWall, currDashing);
        if (currInBlockStun) {
            encodedPatternId += (1L << 20);
        }
        if (currAtked) {
            encodedPatternId += (1L << 21);
        }
        if (currParalyzed) {
            encodedPatternId += (1L << 22);
        }
        return encodedPatternId;
    }

protected:
    BodyIDVector staticColliderBodyIDs;
    BodyIDVector bodyIDsToClear;
    BodyIDVector bodyIDsToAdd;

    // Backend & Frontend shared functions
    inline void elapse1RdfForRdf(RenderFrame* rdf);
    inline void elapse1RdfForBl(Bullet* bl, const Skill* skill, const BulletConfig* bc);
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

    inline uint64_t getUDT(const uint64_t& ud) {
        return (ud & UDT_STRIPPER);
    }

    inline uint32_t getUDPayload(const uint64_t& ud) {
        return (ud & UD_PAYLOAD_STRIPPER);
    }

    CharacterVirtual* getOrCreateCachedPlayerCollider(const uint64_t ud, const PlayerCharacterDownsync& currPlayer, const CharacterConfig* cc, PlayerCharacterDownsync* nextPlayer = nullptr);
    // Unlike DLLMU-v2.3.4, even if "preallocateNpcDict" is empty, "getOrCreateCachedNpcCollider" still works
    CharacterVirtual* getOrCreateCachedNpcCollider(const uint64_t ud, const NpcCharacterDownsync& currNpc, const CharacterConfig* cc, NpcCharacterDownsync* nextNpc = nullptr);
    CharacterVirtual* getOrCreateCachedCharacterCollider(const uint64_t ud, const CharacterConfig* inCc, float newRadius, float newHalfHeight);

    Body*             getOrCreateCachedBulletCollider(const uint64_t ud, const BulletConfig* bc, float immediateHalfBoxSizeX, float immediateHalfBoxSizeY);

    std::unordered_map<uint64_t, CharacterVirtual*> transientUdToCv;
    std::unordered_map<uint64_t, const BodyID*> transientUdToBodyID; // other than "CharacterVirtual.mInnerBodyID" 

    std::unordered_map<uint64_t, const PlayerCharacterDownsync*> transientUdToCurrPlayer;
    std::unordered_map<uint64_t, PlayerCharacterDownsync*> transientUdToNextPlayer;

    std::unordered_map<uint64_t, const NpcCharacterDownsync*> transientUdToCurrNpc; // Mainly for "Bullet.offender_ud" referencing characters, and avoiding the unnecessary [joinIndex change in `leftShiftDeadNpcs` of DLLMU-v2.3.4](https://github.com/genxium/DelayNoMoreUnity/blob/v2.3.4/shared/Battle_builders.cs#L580)
    std::unordered_map<uint64_t, NpcCharacterDownsync*> transientUdToNextNpc;

    std::unordered_map<uint64_t, const Bullet*> transientIdToCurrBl;
    std::unordered_map<uint64_t, Bullet*> transientIdToNextBl;

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

    inline void calcBlCacheKey(const BulletConfig* bc, BL_CACHE_KEY_T& ioCacheKey) {
        ioCacheKey[0] = bc->hitbox_size_x();
        ioCacheKey[1] = bc->hitbox_size_y();
    }

    InputFrameDownsync* getOrPrefabInputFrameDownsync(int inIfdId, uint32_t inSingleJoinIndex, uint64_t inSingleInput, bool fromUdp, bool fromTcp, bool& outExistingInputMutated);

    void batchPutIntoPhySysFromCache(const RenderFrame* currRdf, RenderFrame* nextRdf);
    void batchRemoveFromPhySysAndCache(const RenderFrame* currRdf);

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


    void processPlayerInputs(const RenderFrame* currRdf, float dt, RenderFrame* nextRdf, const int delayedIfdId, const InputFrameDownsync* delayedInputFrameDownsync);
    void processNpcInputs(const RenderFrame* currRdf, float dt, RenderFrame* nextRdf, const InputFrameDownsync* delayedIfd);

    void processSingleCharacterInput(int rdfId, float dt, int patternId, bool jumpedOrNot, bool slipJumpedOrNot, int effDx, int effDy, bool slowDownToAvoidOverlap, const CharacterDownsync& currChd, uint64_t ud, bool currEffInAir, bool currCrouching, bool currOnWall, bool currDashing, bool currInBlockStun, bool currAtked, bool currParalyzed, const CharacterConfig* cc, CharacterDownsync* nextChd, RenderFrame* nextRdf);

    void transitToGroundDodgedChState(CharacterDownsync* nextChd, const CharacterConfig* cc, bool currParalyzed);
    void calcFallenDeath(RenderFrame* nextRdf);
    void resetJumpStartup(CharacterDownsync* nextChd, bool putBtnHoldingJammed = false);
    bool isInJumpStartup(const CharacterDownsync& cd, const CharacterConfig* cc);
    bool isJumpStartupJustEnded(const CharacterDownsync& currCd, CharacterDownsync* nextCd, const CharacterConfig* cc);
    void jamBtnHolding(CharacterDownsync* nextChd);
    void processInertiaWalkingHandleZeroEffDx(int currRdfId, float dt, const CharacterDownsync& currChd, CharacterDownsync* nextChd, int effDy, const CharacterConfig* cc, bool effInAir, bool recoveredFromAirAtk, bool currParalyzed);
    void processInertiaWalking(int rdfId, float dt, const CharacterDownsync& currChd, CharacterDownsync* nextChd, bool currEffInAir, int effDx, int effDy, const CharacterConfig* cc, bool usedSkill, const Skill* skillConfig, bool currParalyzed, bool currInBlockStun);
    void processInertiaFlyingHandleZeroEffDxAndDy(int rdfId, float dt, const CharacterDownsync& currChd, CharacterDownsync* nextChd, const CharacterConfig* cc, bool currParalyzed);
    void processInertiaFlying(int rdfId, float dt, const CharacterDownsync& currChd, CharacterDownsync* nextChd, int effDx, int effDy, const CharacterConfig* cc, bool usedSkill, const Skill* skillConfig, bool currParalyzed, bool currInBlockStun);
    bool addNewBulletToNextFrame(int rdfId, const CharacterDownsync& currChd, CharacterDownsync* nextChd, const CharacterConfig* cc, bool currParalyzed, bool currEffInAir, int xfac, int yfac, const Skill* skillConfig, int activeSkillHit, uint32_t activeSkillId, RenderFrame* nextRdf, bool& hasLockVel, const Bullet* referenceBullet, const BulletConfig* referenceBulletConfig, uint64_t offenderUd, int bulletTeamId);

    void prepareJumpStartup(int currRdfId, const CharacterDownsync& currChd, CharacterDownsync* nextChd, bool currEffInAir, const CharacterConfig* cc, bool currParalyzed);
    void processJumpStarted(int currRdfId, const CharacterDownsync& currChd, CharacterDownsync* nextChd, bool currEffInAir, const CharacterConfig* cc);

    void processWallGrabbingPostPhysicsUpdate(int currRdfId, const CharacterDownsync& currChd, CharacterDownsync* nextChd, const CharacterConfig* cc, const CharacterVirtual* cv, bool inJumpStartupOrJustEnded);

    void processDelayedBulletSelfVel(int rdfId, const CharacterDownsync& currChd, CharacterDownsync* nextChd, const CharacterConfig* cc, bool currParalyzed, bool nextEffInAir);

    virtual void postStepSingleChdStateCorrection(const int steppingRdfId, const uint64_t udt, const uint64_t ud, const CharacterDownsync& currChd, CharacterDownsync* nextChd, const CharacterConfig* cc, bool cvSupported, bool cvInAir, bool cvOnWall, bool currNotDashing, bool currEffInAir, bool oldNextNotDashing, bool oldNextEffInAir, bool inJumpStartupOrJustEnded, CharacterVirtual::EGroundState cvGroundState);
    void leftShiftDeadNpcs(int currRdfId, RenderFrame* nextRdf);
    void leftShiftDeadBullets(int currRdfId, RenderFrame* nextRdf);
    void leftShiftDeadPickables(int currRdfId, RenderFrame* nextRdf);

    bool useSkill(int rdfId, RenderFrame* nextRdf, const CharacterDownsync& currChd, uint64_t ud, const CharacterConfig* cc, CharacterDownsync* nextChd, int effDx, int effDy, int patternId, bool currEffInAir, bool currCrouching, bool currOnWall, bool currDashing, bool currInBlockStun, bool currAtked, bool currParalyzed, int& outSkillId, const Skill*& outSkill);

    void useInventorySlot(int rdfId, int patternId, const CharacterDownsync& currChd, const CharacterConfig* cc, CharacterDownsync* nextChd, bool& outSlotUsed, bool& outDodgedInBlockStun);
    void FindBulletConfig(uint32_t skillId, uint32_t skillHit, const Skill*& outSkill, const BulletConfig*& outBulletConfig);
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

    inline bool isLengthNearZero(float length) {
        return 1e-3 > length;
    }

    inline bool isLengthSquaredNearZero(float lengthSquared) {
        return 1e-6 > lengthSquared;
    }

    CharacterVirtual* createDefaultCharacterCollider(const CharacterConfig* cc);
    Body*             createDefaultBulletCollider(const BulletConfig* bc);

    void preallocateBodies(const RenderFrame* startRdf, const ::google::protobuf::Map<::google::protobuf::int32, ::google::protobuf::int32 >& preallocateNpcSpeciesDict);

    inline void calcChdShape(const CharacterState chState, const CharacterConfig* cc, float& outCapsuleRadius, float& outCapsuleHalfHeight);
    inline void calcBlShape(const Bullet& currBl, const BulletConfig* bc, float& outBoxOffsetX, float& outBoxOffsetY, float& outBoxSizeX, float& outBoxSizeY);

    inline float lerp(float from, float to, float step) {
        float diff = (to - from);
        if (isLengthSquaredNearZero(diff * diff)) {
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

    ////////////////////////////////////////////// (to deprecate!)
    BulletConfig dummyBc;

public:
    // #JPH::ContactListener
    /*
    [WARNING] For rollback-netcode implementation, only "OnContactAdded" and "OnContactPersisted" are to be concerned with, MOREOVER there's NO NEED to clear "contact cache" upon each "Step".

    In a Jolt PhysicsSystem, all EMotionType pairs will trigger "OnContactAdded" or "OnContactPersisted" when they come into contact geometrically (e.g. Kinematic v.s. Kinematic, Kinematic v.s. Static), but NOT ALL EMotionType pairs will create "ContactConstraint" instances (e.g. Kinematic v.s. Kinematic, Kinematic v.s. Static, see https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Physics/Constraints/ContactConstraintManager.cpp#L1108).
    */
    virtual void OnContactCommon(const JPH::Body& inBody1,
        const JPH::Body& inBody2,
        const JPH::ContactManifold& inManifold,
        JPH::ContactSettings& ioSettings);

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
        return JPH::ValidateResult::AcceptContact;
    }
public:
    // #JPH::CharacterContactListener
    /* 
    [WARNING] "CharacterVirtual" instances DON'T participate in collision handling in a Jolt PhysicsSystem (unless #useCustomizedInnerBodyHandling, which is not my default), hence a "CharacterVirtual" instance DOESN'T make use of the "BroadPhase/BroadPhaseQuadTree" tools.

    A "CharacterVirtual" instance uses its own "NarrowPhase" collision handling, and seems to create "ContactConstraint" against all EMotionTypes (Static, Kinematic, Dynamic) by default -- hence the challenge is to properly avoid creating unexpected "ContactConstraint"s.  
    */
    virtual void OnContactCommon(const CharacterVirtual* inCharacter, const BodyID& inBodyID2, const SubShapeID& inSubShapeID2, RVec3Arg inContactPosition, Vec3Arg inContactNormal, CharacterContactSettings& ioSettings);

    virtual void OnCharacterContactCommon(const CharacterVirtual* inCharacter, const CharacterVirtual* inOtherCharacter, const SubShapeID& inSubShapeID2, RVec3Arg inContactPosition, Vec3Arg inContactNormal, CharacterContactSettings& ioSettings);

    virtual void						OnContactAdded(const CharacterVirtual* inCharacter, const BodyID& inBodyID2, const SubShapeID& inSubShapeID2, RVec3Arg inContactPosition, Vec3Arg inContactNormal, CharacterContactSettings& ioSettings) override {
        OnContactCommon(inCharacter, inBodyID2, inSubShapeID2, inContactPosition, inContactNormal, ioSettings);
    }

    virtual void						OnContactPersisted(const CharacterVirtual* inCharacter, const BodyID& inBodyID2, const SubShapeID& inSubShapeID2, RVec3Arg inContactPosition, Vec3Arg inContactNormal, CharacterContactSettings& ioSettings) override {
        OnContactCommon(inCharacter, inBodyID2, inSubShapeID2, inContactPosition, inContactNormal, ioSettings);
    }

    virtual void						OnCharacterContactAdded(const CharacterVirtual* inCharacter, const CharacterVirtual* inOtherCharacter, const SubShapeID& inSubShapeID2, RVec3Arg inContactPosition, Vec3Arg inContactNormal, CharacterContactSettings& ioSettings) override {
        OnCharacterContactCommon(inCharacter, inOtherCharacter, inSubShapeID2, inContactPosition, inContactNormal, ioSettings);
    }

    virtual void						OnCharacterContactPersisted(const CharacterVirtual* inCharacter, const CharacterVirtual* inOtherCharacter, const SubShapeID& inSubShapeID2, RVec3Arg inContactPosition, Vec3Arg inContactNormal, CharacterContactSettings& ioSettings) override {
        OnCharacterContactCommon(inCharacter, inOtherCharacter, inSubShapeID2, inContactPosition, inContactNormal, ioSettings);
    }

protected:
    virtual void handleLhsBulletCollision(
        const uint64_t udLhs,
        const JPH::Body& lhs, // the "Bullet"
        uint64_t udtRhs, const JPH::Body& rhs,
        const JPH::ContactManifold& inManifold,
        JPH::ContactSettings& ioSettings);
};

#endif
