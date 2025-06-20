#pragma once
#include "joltc_export.h"
#include <Jolt/Jolt.h>
#include <Jolt/Core/Reference.h>
#include <Jolt/Core/UnorderedMap.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include "FrameRingBuffer.h"
#include "BattleConsts.h"
#include "serializable_data.pb.h"

// All Jolt symbols are in the JPH namespace
using namespace JPH;
using namespace jtshared;

class JOLTC_EXPORT BaseBattle {
    public:
        inline BaseBattle(int aPlayersCnt, const int renderBufferSize, const int inputBufferSize, PhysicsSystem* aPhySys, JobSystemThreadPool* aJobSys) : prefabbedInputList(aPlayersCnt, 0), playerInputFrontIds(aPlayersCnt, 0), playerInputFronts(aPlayersCnt, 0), rdfBuffer(renderBufferSize), ifdBuffer(inputBufferSize) {
            playersCnt = aPlayersCnt;
            allConfirmedMask = (U64_1 << playersCnt) - 1;
            inactiveJoinMask = 0u;
            timerRdfId = 0;
            battleDurationFrames = 0;
            phySys = aPhySys;
            jobSys = aJobSys;

            dummyCc.set_capsule_radius(3.0);
            dummyCc.set_capsule_half_height(10.0);
        }

        virtual ~BaseBattle() {
            playersCnt = 0;
            delete phySys;
            delete jobSys;

            /*
            [WARNING] Unlike "std::vector" and "std::unordered_map", the customized containers "JPH::StaticArray" and "JPH::UnorderedMap" will deallocate their pointer-typed elements in their destructors.
            - https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Core/Array.h#L337 -> https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Core/Array.h#L249
            - https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Core/HashTable.h#L480 -> https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Core/HashTable.h#L497

            However I only use "dynamicBodyIDs" to hold "BodyID" instances which are NOT pointer-typed and effectively freed once "JPH::StaticArray::clear()" is called, i.e. no need for the heavy duty "JPH::Array".
            */
        }

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

        PhysicsSystem* phySys;
        JobSystemThreadPool* jobSys;

        StaticArray<BodyID, DEFAULT_PREALLOC_DYNAMIC_COLLIDER_CAPACITY> dynamicBodyIDs;
        UnorderedMap<Vec4, BoxShapeSettings*> cachedBoxShapeSettings; // Key is "{density, halfExtent}", where "convexRadius" is determined by "halfExtent"
        UnorderedMap<Vec3, CharacterVirtual*> cachedChColliders; // Key is "{density, radius, halfHeight}", kindly note that position and orientation of "CharacterVirtual" are mutable during reuse, thus not using "RefConst<>".

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
        inline void setPlayerActive(int joinIndex) {
            inactiveJoinMask &= (allConfirmedMask ^ calcJoinIndexMask(joinIndex));
        }

        inline void setPlayerInactive(int joinIndex) {
            inactiveJoinMask |= calcJoinIndexMask(joinIndex);
        }

        void Step(int fromRdfId, int toRdfId, TempAllocator* tempAllocator);

protected:
        // Backend & Frontend shared functions
        void elapse1RdfForChd(CharacterDownsync* chd);

        int moveForwardlastConsecutivelyAllConfirmedIfdId(int proposedIfdEdFrameId, uint64_t skippableJoinMask, uint64_t& unconfirmedMask);

        CharacterVirtual* getOrCreateCachedCharacterCollider(CharacterDownsync* inCd, CharacterConfig* inCc);

private:
        CharacterConfig dummyCc;
};

