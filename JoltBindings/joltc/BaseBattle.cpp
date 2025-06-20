#include "BaseBattle.h"
#include "CollisionLayers.h"
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <string>

using namespace jtshared;

// Backend & Frontend shared functions
int BaseBattle::moveForwardlastConsecutivelyAllConfirmedIfdId(int proposedIfdEdFrameId, uint64_t skippableJoinMask, uint64_t& unconfirmedMask) {
    // [WARNING/BACKEND] This function MUST BE called while "inputBufferLock" is locked!

    int incCnt = 0;
    int proposedIfdStFrameId = lastConsecutivelyAllConfirmedIfdId + 1;
    if (proposedIfdStFrameId >= proposedIfdEdFrameId) {
        return incCnt;
    }
    
    unconfirmedMask |= inactiveJoinMask;
    for (int inputFrameId = proposedIfdStFrameId; inputFrameId < proposedIfdEdFrameId; inputFrameId++) {
        // See comments for the traversal in "markConfirmationIfApplicable".
        if (inputFrameId < ifdBuffer.StFrameId) {
            continue;
        }
        InputFrameDownsync* ifd = ifdBuffer.GetByFrameId(inputFrameId);
        if (nullptr == ifd) {
            throw std::runtime_error("[_moveForwardlastConsecutivelyAllConfirmedIfdId] inputFrameId=" + std::to_string(inputFrameId) + " doesn't exist for lastConsecutivelyAllConfirmedIfdId=" + std::to_string(lastConsecutivelyAllConfirmedIfdId) + ", proposedIfdStFrameId = " + std::to_string(proposedIfdStFrameId) + ", proposedIfdEdFrameId = " + std::to_string(proposedIfdEdFrameId) + ", inputBuffer = " + ifdBuffer.toSimpleStat());
        }

        if (allConfirmedMask != (ifd->confirmed_list() | inactiveJoinMask | skippableJoinMask)) {
            break;
        }

        incCnt += 1;
        ifd->set_confirmed_list(allConfirmedMask);
        if (lastConsecutivelyAllConfirmedIfdId < inputFrameId) {
            // Such that "lastConsecutivelyAllConfirmedIfdId" is monotonic.
            lastConsecutivelyAllConfirmedIfdId = inputFrameId;
        }
    }

    return incCnt;
}

static inline EBackFaceMode sBackFaceMode = EBackFaceMode::CollideWithBackFaces;
static inline float		sUpRotationX = 0;
static inline float		sUpRotationZ = 0;
static inline float		sMaxSlopeAngle = DegreesToRadians(45.0f);
static inline float		sMaxStrength = 100.0f;
static inline float		sCharacterPadding = 0.02f;
static inline float		sPenetrationRecoverySpeed = 1.0f;
static inline float		sPredictiveContactDistance = 0.1f;
static inline bool		sEnableWalkStairs = true;
static inline bool		sEnableStickToFloor = true;
static inline bool		sEnhancedInternalEdgeRemoval = false;
static inline bool		sCreateInnerBody = false;
static inline bool		sPlayerCanPushOtherCharacters = true;
static inline bool		sOtherCharactersCanPushPlayer = true;
CharacterVirtual* BaseBattle::getOrCreateCachedCharacterCollider(CharacterDownsync* cd, CharacterConfig* cc) {
    auto capsuleKey = Vec3(1.0, cc->capsule_radius(), cc->capsule_half_height());
    RVec3Arg initialPos = RVec3Arg(cd->x(), cd->y(), 0);
    CharacterVirtual* chCollider = nullptr;
    auto itChCollider = cachedChColliders.find(capsuleKey);
    if (itChCollider == cachedChColliders.end()) {
        CapsuleShape* chShapeCenterAnchor = new CapsuleShape(dummyCc.capsule_half_height(), dummyCc.capsule_radius()); // transient, only created on stack for immediately transform
        RefConst<Shape> chShape = RotatedTranslatedShapeSettings(Vec3(0, dummyCc.capsule_half_height() + dummyCc.capsule_radius(), 0), Quat::sIdentity(), chShapeCenterAnchor).Create().Get();
        Ref<CharacterVirtualSettings> settings = new CharacterVirtualSettings();
        settings->mMaxSlopeAngle = sMaxSlopeAngle;
        settings->mMaxStrength = sMaxStrength;
        settings->mShape = chShape;
        settings->mBackFaceMode = sBackFaceMode;
        settings->mCharacterPadding = sCharacterPadding;
        settings->mPenetrationRecoverySpeed = sPenetrationRecoverySpeed;
        settings->mPredictiveContactDistance = sPredictiveContactDistance;
        settings->mSupportingVolume = Plane(Vec3::sAxisY(), -dummyCc.capsule_radius()); // Accept contacts that touch the lower sphere of the capsule
        settings->mEnhancedInternalEdgeRemoval = sEnhancedInternalEdgeRemoval;
        settings->mInnerBodyShape = chShape;
        settings->mInnerBodyLayer = MonInsObjectLayers::MOVING;

        // Create character
        chCollider = new CharacterVirtual(settings, initialPos, Quat::sIdentity(), 0, phySys);
        auto [it2, res2] = cachedChColliders.try_emplace(capsuleKey, chCollider);
        if (!res2) {
            throw std::runtime_error("[_getOrCreateCachedCharacterCollider] failed to insert into preallocatedChColliders");
        }
    } else {
        chCollider = itChCollider->second;
        chCollider->SetPosition(initialPos);
        auto innerBodyID = chCollider->GetInnerBodyID();
        phySys->GetBodyInterface().AddBody(innerBodyID, EActivation::Activate);
    }
    return chCollider;
}

void BaseBattle::Step(int fromRdfId, int toRdfId, TempAllocator* tempAllocator) {
    for (int rdfId = fromRdfId; rdfId < toRdfId; rdfId++) {
        auto rdf = rdfBuffer.GetByFrameId(rdfId);
        for (int i = 0; i < playersCnt; i++) {
            auto player = rdf->players_arr(i);
            auto chCollider = getOrCreateCachedCharacterCollider(&player, &dummyCc);
            dynamicBodyIDs.push_back(chCollider->GetInnerBodyID());
        }
        phySys->Update(ESTIMATED_SECONDS_PER_FRAME, 1, tempAllocator, jobSys);
        // TODO: Update nextRdf by (rdf, phySys current body states of "dynamicBodyIDs"), then set back into "rdfBuffer" for rendering.
        phySys->GetBodyInterface().RemoveBodies(dynamicBodyIDs.data(), dynamicBodyIDs.size());
        dynamicBodyIDs.clear();
    }
}

void BaseBattle::elapse1RdfForChd(CharacterDownsync* chd) {
    
}
