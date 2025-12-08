#ifndef CH_VIS_COLLIDE_SHAPE_COLLECTOR_H_
#define CH_VIS_COLLIDE_SHAPE_COLLECTOR_H_ 1

#include "BaseBattleCollisionFilter.h"
#include <Jolt/Physics/Collision/CollideShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <limits> // Required for std::numeric_limits

class CharacterVisionCollideShapeCollector : public JPH::CollideShapeCollector {

public:
    explicit CharacterVisionCollideShapeCollector(const int currRdfId, const JPH::BodyInterface* bi, const uint64_t ud, const uint64_t udt, const NpcCharacterDownsync* currNpc, NpcCharacterDownsync* nextNpc, BaseBattleCollisionFilter* filter, CH_COLLIDER_T* npcSelfCollider) : mCurrRdfId(currRdfId), mBi(bi), mUd(ud), mUdt(udt), mCurrSelfNpc(currNpc), mNextSelfNpc(nextNpc), mFilter(filter),mNpcSelfCollider(npcSelfCollider) {
        mNpcSelfShape = static_cast<const RotatedTranslatedShape*>(mNpcSelfCollider->GetShape());
        mNpcSelfInnerShape = static_cast<const CapsuleShape*>(mNpcSelfShape->GetInnerShape());
        auto& currSelfChd = mCurrSelfNpc->chd(); 
        mNpcSelfColliderLeft = currSelfChd.x() - mNpcSelfInnerShape->GetRadius();
        mNpcSelfColliderRight = currSelfChd.x() + mNpcSelfInnerShape->GetRadius();
        mNpcSelfColliderTop = currSelfChd.y() + mNpcSelfInnerShape->GetHalfHeightOfCylinder();
        mNpcSelfColliderBottom = currSelfChd.y();
    }

    int                     mCurrRdfId;
    float                   mNpcSelfColliderLeft;
    float                   mNpcSelfColliderRight; 
    float                   mNpcSelfColliderTop; 
    float                   mNpcSelfColliderBottom;
    uint64_t                oppoChUd;
    uint64_t                oppoBlUd;
    uint64_t                allyChUd;
    uint64_t                mvBlockerUd;

    virtual void AddHit(const JPH::CollideShapeResult& inResult) override {
        const uint64_t udRhs = mBi->GetUserData(inResult.mBodyID2);
        const uint64_t udtRhs = mFilter->getUDT(udRhs);

        float minAbsColliderDx = std::numeric_limits<float>::max();
        float minAbsColliderDy = std::numeric_limits<float>::max();

        float minAbsColliderDxForAlly = std::numeric_limits<float>::max();
        float minAbsColliderDyForAlly = std::numeric_limits<float>::max();

        float minAbsColliderDxForMvBlocker = std::numeric_limits<float>::max();
        float minAbsColliderDyForMvBlocker = std::numeric_limits<float>::max();

        float rhsColliderLeft, rhsColliderRight, rhsColliderTop, rhsColliderBottom;

        mFilter->handleLhsCharacterVisionCollision(mCurrRdfId, mUd, mUdt, mCurrSelfNpc, mNextSelfNpc, udRhs, udtRhs, inResult,
        minAbsColliderDx,
        minAbsColliderDy,
        minAbsColliderDxForAlly,
        minAbsColliderDyForAlly,
        minAbsColliderDxForMvBlocker,
        minAbsColliderDyForMvBlocker,
        rhsColliderLeft, 
        rhsColliderRight, 
        rhsColliderTop, 
        rhsColliderBottom,
        oppoChUd,
        oppoBlUd,
        allyChUd,
        mvBlockerUd
        );
    }

private:
    const JPH::BodyInterface*     mBi;
    const uint64_t                mUd;
    const uint64_t                mUdt;
    const NpcCharacterDownsync*   mCurrSelfNpc;
    NpcCharacterDownsync*         mNextSelfNpc;
    BaseBattleCollisionFilter*    mFilter;
    CH_COLLIDER_T*                mNpcSelfCollider;                        
    const RotatedTranslatedShape* mNpcSelfShape;                        
    const CapsuleShape*           mNpcSelfInnerShape;
};

#endif
