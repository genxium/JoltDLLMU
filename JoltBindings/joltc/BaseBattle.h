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

class JOLTC_EXPORT BaseBattle {
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
        /*
        [WARNING/FRONTEND] At any point of time it's maintained that "timerRdfId >= rdfBuffer.StFrameId", for obvious reason. 
        */
        int timerRdfId;
        int battleDurationFrames;
        FrameRingBuffer<RenderFrame> rdfBuffer;
        FrameRingBuffer<InputFrameDownsync> ifdBuffer;

        std::vector<uint64_t> prefabbedInputList;
        std::vector<int> playerInputFrontIds;
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
        std::unordered_map< BL_CACHE_KEY_T, BL_COLLIDER_Q, COLLIDER_HASH_KEY_T > cachedBlColliders; // Key is "{halfExtent}", where "convexRadius" is determined by "halfExtent"

        // It's by design that "JPH::CharacterVirtual" instead of "JPH::Character" or even "JPH::Body" is used here, see "https://jrouwe.github.io/JoltPhysics/index.html#character-controllers" for their differences.
        CH_COLLIDER_Q activeChColliders;
        /*
         [TODO] 
    
         Make "cachedChColliders" keyed by "CharacterConfig.species_id()" to fit the need of multi-shape character (at different ch_state). 

        It's by design that "ScaledShape" is NOT used here, because when mixed with translation and rotation, the order of affine transforms matters but is difficult to keep in mind. 

        Moreover, by using this approach to manage multi-shape character I dropped the "shared shapes across bodies" feature of Jolt.
        */
        std::unordered_map< CH_CACHE_KEY_T, CH_COLLIDER_Q, COLLIDER_HASH_KEY_T > cachedChColliders; // Key is "{radius, halfHeight}", kindly note that position and orientation of "CharacterVirtual" are mutable during reuse, thus not using "RefConst<>".

    public:
        inline static int ConvertToIfdId(int rdfId, int delayRdfCnt) {
            if (rdfId < delayRdfCnt) {
                return 0;
            }
            return ((rdfId-delayRdfCnt) >> globalPrimitiveConsts->input_scale_frames());
        }

        inline static int ConvertToGeneratingIfdId(int renderFrameId, int localExtraInputDelayFrames = 0) {
            return ConvertToIfdId(renderFrameId, -localExtraInputDelayFrames);
        }

        inline static int ConvertToDelayedInputFrameId(int renderFrameId) {
            return ConvertToIfdId(renderFrameId, globalPrimitiveConsts->input_delay_frames());
        }

        inline static int ConvertToFirstUsedRenderFrameId(int inputFrameId) {
            return ((inputFrameId << globalPrimitiveConsts->input_scale_frames()) + globalPrimitiveConsts->input_scale_frames());
        }

        inline static int ConvertToLastUsedRenderFrameId(int inputFrameId) {
            return ((inputFrameId << globalPrimitiveConsts->input_scale_frames()) + globalPrimitiveConsts->input_delay_frames() + (1 << globalPrimitiveConsts->input_scale_frames()) - 1);
        }

        inline void SetPlayerActive(uint32_t joinIndex) {
            inactiveJoinMask &= (allConfirmedMask ^ calcJoinIndexMask(joinIndex));
        }

        inline void SetPlayerInactive(uint32_t joinIndex) {
            inactiveJoinMask |= calcJoinIndexMask(joinIndex);
        }

        virtual void Step(int fromRdfId, int toRdfId, DownsyncSnapshot* virtualIfds = nullptr);

        void Clear();

        virtual bool ResetStartRdf(char* inBytes, int inBytesCnt);
        virtual bool ResetStartRdf(const WsReq* initializerMapData);

        static void NewPreallocatedBullet(Bullet* single, int bulletLocalId, int originatedRenderFrameId, int teamId, BulletState blState, int framesInBlState);
        static void NewPreallocatedCharacterDownsync(CharacterDownsync* single, int buffCapacity, int debuffCapacity, int inventoryCapacity, int bulletImmuneRecordCapacity);
        static void NewPreallocatedPlayerCharacterDownsync(PlayerCharacterDownsync* single, int buffCapacity, int debuffCapacity, int inventoryCapacity, int bulletImmuneRecordCapacity);
        static void NewPreallocatedNpcCharacterDownsync(NpcCharacterDownsync* single, int buffCapacity, int debuffCapacity, int inventoryCapacity, int bulletImmuneRecordCapacity);
        static RenderFrame* NewPreallocatedRdf(int roomCapacity, int preallocNpcCount, int preallocBulletCount);

protected:
        BodyIDVector staticColliderBodyIDs;
        BodyIDVector bodyIDsToClear;
        BodyIDVector bodyIDsToAdd;

        // Backend & Frontend shared functions
        inline void elapse1RdfForRdf(RenderFrame* rdf);
        inline void elapse1RdfForBl(Bullet* bl);
        inline void elapse1RdfForPlayerChd(PlayerCharacterDownsync* playerChd, const CharacterConfig* cc);
        inline void elapse1RdfForNpcChd(NpcCharacterDownsync* npcChd, const CharacterConfig* cc);
        inline void elapse1RdfForChd(CharacterDownsync* cd, const CharacterConfig* cc);
             
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

        CharacterVirtual* getOrCreateCachedPlayerCollider(const PlayerCharacterDownsync& currPlayer, const CharacterConfig* cc, PlayerCharacterDownsync* nextPlayer = nullptr);
        CharacterVirtual* getOrCreateCachedNpcCollider(const NpcCharacterDownsync& currNpc, const CharacterConfig* cc, NpcCharacterDownsync* nextNpc = nullptr);
        CharacterVirtual* getOrCreateCachedCharacterCollider(const CharacterConfig* inCc, float newRadius, float newHalfHeight, uint64_t userData);

        std::unordered_map<uint64_t, CharacterVirtual*> transientUdToCv; 
        std::unordered_map<uint64_t, const BodyID*> transientUdToBodyID; // other than "CharacterVirtual.mInnerBodyID" 

        std::unordered_map<uint32_t, const PlayerCharacterDownsync*> transientJoinIndexToCurrPlayer; 
        std::unordered_map<uint32_t, PlayerCharacterDownsync*> transientJoinIndexToNextPlayer;

        std::unordered_map<uint32_t, const NpcCharacterDownsync*> transientIdToCurrNpc; // Mainly for "Bullet.offender_join_index" referencing characters, and avoiding the unnecessary [joinIndex change in `_leftShiftDeadNpcs` of DLLMU-v2.3.4](https://github.com/genxium/DelayNoMoreUnity/blob/v2.3.4/shared/Battle_builders.cs#L580)
        std::unordered_map<uint32_t, NpcCharacterDownsync*> transientIdToNextNpc;

        std::unordered_map<uint32_t, const Bullet*> transientIdToCurrBl;
        std::unordered_map<uint32_t, Bullet*> transientIdToNextBl;

        std::unordered_map<uint32_t, const Trap*> transientIdToCurrTrap;
        std::unordered_map<uint32_t, Trap*> transientIdToNextTrap;

        inline const CharacterDownsync& immutableCurrChdFromUd(uint64_t ud) {
            uint64_t udt = getUDT(ud);
            uint64_t payload = getUDPayload(ud);
            return immutableCurrChdFromUd(udt, payload);
        }

        inline CharacterDownsync* mutableNextChdFromUd(uint64_t ud) {
            uint64_t udt = getUDT(ud);
            uint64_t payload = getUDPayload(ud);
            return mutableNextChdFromUd(udt, payload);
        }

        inline const CharacterDownsync& immutableCurrChdFromUd(uint64_t udt, uint64_t payload) {
            return (UDT_PLAYER == udt ? transientJoinIndexToCurrPlayer[payload]->chd() : transientIdToCurrNpc[payload]->chd());
        }

        inline CharacterDownsync* mutableNextChdFromUd(uint64_t udt, uint64_t payload) {
            return (UDT_PLAYER == udt ? transientJoinIndexToNextPlayer[payload]->mutable_chd() : transientIdToNextNpc[payload]->mutable_chd());
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


        void processPlayerInputs(const RenderFrame* currRdf, RenderFrame* nextRdf, const InputFrameDownsync* delayedInputFrameDownsync);
        void processNpcInputs(const RenderFrame* currRdf, RenderFrame* nextRdf, const InputFrameDownsync* delayedIfd);
        
        void processSingleCharacterInput(int rdfId, int patternId, bool jumpedOrNot, bool slipJumpedOrNot, int effDx, int effDy, bool slowDownToAvoidOverlap, const CharacterDownsync& currChd, bool currEffInAir, const CharacterConfig* cc, CharacterDownsync* nextChd, RenderFrame* nextRdf);

        bool useSkill(int rdfId, int effDx, int effDy, int patternId, const CharacterDownsync& currChd, const CharacterConfig* cc, bool currEffInAir, CharacterDownsync* nextChd, RenderFrame* nextRdf, bool slotUsed, uint32_t slotLockedSkillId, bool isParalyzed);

        void useInventorySlot(int rdfId, int patternId, const CharacterDownsync& currChd, bool currEffInAir, const CharacterConfig* cc, CharacterDownsync* nextChd, bool& outSlotUsed, uint32_t& outSlotLockedSKillId, bool& outDodgedInBlockStun);

        void transitToGroundDodgedChState(CharacterDownsync* nextChd, const CharacterConfig* cc, bool isParalyzed);
        void resetJumpStartup(CharacterDownsync* nextChd, bool putBtnHoldingJammed = false);
        bool isInJumpStartup(const CharacterDownsync& cd, const CharacterConfig* cc);
        bool isJumpStartupJustEnded(const CharacterDownsync& currCd, CharacterDownsync* nextCd, const CharacterConfig* cc);
        void jamBtnHolding(CharacterDownsync* nextChd);
        void processInertiaWalkingHandleZeroEffDx(int currRdfId, const CharacterDownsync& currChd, CharacterDownsync* nextChd, int effDy, const CharacterConfig* cc, bool effInAir, bool recoveredFromAirAtk, bool isParalyzed);
        void processInertiaWalking(int rdfId, const CharacterDownsync& currChd, CharacterDownsync* nextChd, bool currEffInAir, int effDx, int effDy, const CharacterConfig* cc, bool usedSkill, const Skill* skillConfig, bool isParalyzed);
        void processInertiaFlyingHandleZeroEffDxAndDy(int rdfId, const CharacterDownsync& currChd, CharacterDownsync* nextChd, const CharacterConfig* cc, bool isParalyzed);
        void processInertiaFlying(int rdfId, const CharacterDownsync& currChd, CharacterDownsync* nextChd, int effDx, int effDy, const CharacterConfig* cc, bool usedSkill, const Skill* skillConfig, bool isParalyzed);

        void prepareJumpStartup(int currRdfId, const CharacterDownsync& currChd, CharacterDownsync* nextChd, bool currEffInAir, const CharacterConfig* cc, bool isParalyzed);
        void processJumpStarted(int currRdfId, const CharacterDownsync& currChd, CharacterDownsync* nextChd, bool currEffInAir, const CharacterConfig* cc);

        void processWallGrabbingPostPhysicsUpdate(int currRdfId, const CharacterDownsync& currChd, CharacterDownsync* nextChd, const CharacterConfig* cc, const CharacterVirtual* cv, bool inJumpStartupOrJustEnded);

        void processDelayedBulletSelfVel(int rdfId, const CharacterDownsync& currChd, CharacterDownsync* nextChd, const CharacterConfig* cc, bool isParalyzed, bool nextEffInAir);

        void postStepSingleChdStateCorrection(const CharacterDownsync& currChd, CharacterDownsync* nextChd, const CharacterConfig* cc, bool cvSupported, bool cvInAir, bool cvOnWall, bool currNotDashing, bool currEffInAir, bool oldNextNotDashing, bool oldNextEffInAir, bool inJumpStartupOrJustEnded, CharacterVirtual::EGroundState cvGroundState);
        void leftShiftDeadNpcs(bool isChasing);
        void leftShiftDeadBullets(bool isChasing);

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

        void preallocateBodies(const RenderFrame* startRdf);

        inline void calcChdShape(const CharacterDownsync& currChd, const CharacterConfig* cc, float& outCapsuleRadius, float& outCapsuleHalfHeight);

private:
        Vec3 safeDeactiviatedPosition;
        InputFrameDecoded ifDecodedHolder;
        
        ////////////////////////////////////////////// (to deprecate!)
        BulletConfig dummyBc;
};

#endif
