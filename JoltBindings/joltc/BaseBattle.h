#pragma once

#include "joltc_export.h"
#include <Jolt/Jolt.h>
#include <Jolt/Core/Reference.h>
#include <Jolt/Core/UnorderedMap.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include "FrameRingBuffer.h"
#include "BattleConsts.h"
#include "serializable_data.pb.h"

#include "CollisionLayers.h"
#include "CollisionCallbacks.h"
#include <map>
#include <deque>

// All Jolt symbols are in the JPH namespace
using namespace JPH;
using namespace jtshared;

class JOLTC_EXPORT BaseBattle {
    public:
        BaseBattle(char* inBytes, int inBytesCnt, int renderBufferSize, int inputBufferSize); 
        virtual ~BaseBattle();

    public:
        bool frameLogEnabled = false;
        int playersCnt;
        uint64_t allConfirmedMask; 
        uint64_t inactiveJoinMask; // realtime information
        int timerRdfId;
        int battleDurationFrames;
        FrameRingBuffer<RenderFrame> rdfBuffer;
        FrameRingBuffer<InputFrameDownsync> ifdBuffer;

        std::vector<uint64_t> prefabbedInputList;
        std::vector<int> playerInputFrontIds;
        std::vector<uint64_t> playerInputFronts;

        int lastConsecutivelyAllConfirmedIfdId = -1;

        BPLayerInterfaceImpl bpLayerInterface;
        ObjectVsBroadPhaseLayerFilterImpl ovbLayerFilter;
        ObjectLayerPairFilterImpl ovoLayerFilter;
        MyBodyActivationListener bodyActivationListener;
        MyContactListener contactListener;
        PhysicsSystem* phySys;
        JobSystemThreadPool* jobSys;

        StaticArray<BodyID, DEFAULT_PREALLOC_DYNAMIC_COLLIDER_CAPACITY> dynamicBodyIDs;
        UnorderedMap<Vec4, BoxShapeSettings*> cachedBoxShapeSettings; // Key is "{density, halfExtent}", where "convexRadius" is determined by "halfExtent"

        // It's by design that "JPH::CharacterVirtual" instead of "JPH::Character" or even "JPH::Body" is used here, see "https://jrouwe.github.io/JoltPhysics/index.html#character-controllers" for their differences.
        std::deque<Ref<CharacterVirtual>> activeChColliders;
        UnorderedMap<Vec3, std::deque<Ref<CharacterVirtual>>> cachedChColliders; // Key is "{density, radius, halfHeight}", kindly note that position and orientation of "CharacterVirtual" are mutable during reuse, thus not using "RefConst<>".

    protected:
        inline int ConvertToDynamicallyGeneratedDelayInputFrameId(int renderFrameId, int localExtraInputDelayFrames) {
            return ((renderFrameId+localExtraInputDelayFrames) >> INPUT_SCALE_FRAMES);
        }

        inline int ConvertToDelayedInputFrameId(int renderFrameId) {
            if (renderFrameId < INPUT_DELAY_FRAMES) {
                return 0;
            }
            return ((renderFrameId - INPUT_DELAY_FRAMES) >> INPUT_SCALE_FRAMES);
        }

        inline int ConvertToFirstUsedRenderFrameId(int inputFrameId) {
            return ((inputFrameId << INPUT_SCALE_FRAMES) + INPUT_DELAY_FRAMES);
        }

        inline int ConvertToLastUsedRenderFrameId(int inputFrameId) {
            return ((inputFrameId << INPUT_SCALE_FRAMES) + INPUT_DELAY_FRAMES + (1 << INPUT_SCALE_FRAMES) - 1);
        }

        inline uint64_t calcJoinIndexMask(int joinIndex) {
            return (U64_1 << (joinIndex - 1));
        }

    public:
        inline void SetPlayerActive(int joinIndex) {
            inactiveJoinMask &= (allConfirmedMask ^ calcJoinIndexMask(joinIndex));
        }

        inline void SetPlayerInactive(int joinIndex) {
            inactiveJoinMask |= calcJoinIndexMask(joinIndex);
        }

        void Step(int fromRdfId, int toRdfId, TempAllocator* tempAllocator);

        void Clear();

protected:
        BodyIDVector bodyIDsToClear;

        // Backend & Frontend shared functions
        void elapse1RdfForChd(CharacterDownsync* chd);

        int moveForwardlastConsecutivelyAllConfirmedIfdId(int proposedIfdEdFrameId, uint64_t skippableJoinMask, uint64_t& unconfirmedMask);

        CharacterVirtual* getOrCreateCachedCharacterCollider(CharacterDownsync* inCd, CharacterConfig* inCc);

        std::unordered_map<int, CharacterDownsync*> transientCurrJoinIndexToChdMap; // Mainly for "Bullet.offender_join_index" referencing characters, and avoiding the unnecessary [joinIndex change in `_leftShiftDeadNpcs` of DLLMU-v2.3.4](https://github.com/genxium/DelayNoMoreUnity/blob/v2.3.4/shared/Battle_builders.cs#L580)

private:
        CharacterConfig dummyCc;
};

