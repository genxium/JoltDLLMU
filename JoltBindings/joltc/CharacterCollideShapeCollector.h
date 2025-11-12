#ifndef CHARACTER_COLLIDE_SHAPE_COLLECTOR_H_
#define CHARACTER_COLLIDE_SHAPE_COLLECTOR_H_ 1

#include <Jolt/Physics/Collision/CollideShape.h>
#include "PbConsts.h"
#include "BaseBattleCollisionFilter.h"

class CharacterCollideShapeCollector : public JPH::CollideShapeCollector {
public:
    explicit CharacterCollideShapeCollector(const int currRdfId, RenderFrame* nextRdf, const JPH::BodyInterface* bi, const uint64_t ud, const uint64_t udt, const CharacterDownsync* currChd, CharacterDownsync* nextChd, JPH::Vec3Arg inUp, JPH::Vec3Arg baseOffset, BaseBattleCollisionFilter* filter) : mCurrRdfId(currRdfId), mNextRdf(nextRdf), mBi(bi), mUd(ud), mUdt(udt), mCurrChd(currChd), mNextChd(nextChd), mBaseOffset(baseOffset), mUp(inUp), mFilter(filter) {}

    int                     mCurrRdfId;
    RenderFrame*            mNextRdf;

    JPH::BodyID				mGroundBodyID;
    JPH::SubShapeID			mGroundBodySubShapeID;
    JPH::RVec3				mGroundPosition = JPH::RVec3::sZero();
    JPH::Vec3				mGroundNormal = JPH::Vec3::sZero();

    uint32_t                newEffDebuffSpeciesId = globalPrimitiveConsts->terminating_debuff_species_id();
    int                     newEffDamage = 0;
    bool                    newEffBlownUp = false;
    int                     newEffFramesToRecover = 0;
    float                   newEffPushbackVelX = 0;
    float                   newEffPushbackVelY = 0;

    virtual void		AddHit(const JPH::CollideShapeResult& inResult) override {
        const uint64_t udRhs = mBi->GetUserData(inResult.mBodyID2);
        const uint64_t udtRhs = mFilter->getUDT(udRhs);
        auto res = mFilter->validateLhsCharacterContact(mCurrChd, udRhs, udtRhs);
        if (JPH::ValidateResult::AcceptContact != res && JPH::ValidateResult::AcceptAllContactsForThisBodyPair != res) {
            return;
        }
        auto normal = -inResult.mPenetrationAxis.Normalized();
        float dot = normal.Dot(mUp);
        if (dot > mBestDot) // Find the hit that is most aligned with the up vector
        {
            mGroundBodyID = inResult.mBodyID2;
            mGroundBodySubShapeID = inResult.mSubShapeID2;
            mGroundPosition = mBaseOffset + inResult.mContactPointOn2;
            mGroundNormal = normal;
            mBestDot = dot;
        }

        mFilter->handleLhsCharacterCollision(mCurrRdfId, mNextRdf, mUd, mUdt, mCurrChd, mNextChd, udRhs, udtRhs, inResult, newEffDebuffSpeciesId, newEffDamage, newEffBlownUp, newEffFramesToRecover, newEffPushbackVelX, newEffPushbackVelY);
    }

private:
    const JPH::BodyInterface*     mBi;
    const uint64_t                mUd;
    const uint64_t                mUdt;
    const CharacterDownsync*      mCurrChd;
    CharacterDownsync*            mNextChd;
    JPH::Vec3				      mUp;
    JPH::Vec3                     mBaseOffset;
    BaseBattleCollisionFilter*    mFilter;
    float				          mBestDot = -FLT_MAX;
};

#endif

