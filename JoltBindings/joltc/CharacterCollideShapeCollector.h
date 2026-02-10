#ifndef CHARACTER_COLLIDE_SHAPE_COLLECTOR_H_
#define CHARACTER_COLLIDE_SHAPE_COLLECTOR_H_ 1

#include "BaseBattleCollisionFilter.h"
#include "PbConsts.h"
#include <Jolt/Physics/Body/BodyFilter.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Collision/CollideShape.h>

using namespace jtshared;
using namespace JPH;

class VisionBodyFilter : public BodyFilter {
public:
    const CharacterDownsync* mSelfNpcChd;
    const CharacterDownsync* mSelfNpcNextChd;
    BodyID   mSelfNpcBodyID;
    uint64_t mSelfNpcUd;
    const BaseBattleCollisionFilter* mBaseBattleFilter;

    VisionBodyFilter(const CharacterDownsync* inSelfNpcChd, const CharacterDownsync* inSelfNpcNextChd, const BodyID& inSelfNpcBodyID, const uint64_t inSelfNpcUd, const BaseBattleCollisionFilter* baseBattleFilter) : mSelfNpcChd(inSelfNpcChd), mSelfNpcNextChd(inSelfNpcNextChd), mSelfNpcBodyID(inSelfNpcBodyID), mSelfNpcUd(inSelfNpcUd), mBaseBattleFilter(baseBattleFilter) {

    }

    virtual bool			ShouldCollide([[maybe_unused]] const BodyID &inBodyID) const {
        return inBodyID != mSelfNpcBodyID;
    }

    virtual bool			ShouldCollideLocked([[maybe_unused]] const Body &inBody) const {
        if (nullptr == mBaseBattleFilter) {
            return true;
        }
        const uint64_t udRhs = inBody.GetUserData();
        const uint64_t udtRhs = mBaseBattleFilter->getUDT(udRhs);
        auto res = mBaseBattleFilter->validateLhsCharacterContact(mSelfNpcChd, mSelfNpcNextChd, udRhs, udtRhs, inBody);
        if (ValidateResult::AcceptContact != res && ValidateResult::AcceptAllContactsForThisBodyPair != res) {
            return false;
        }
        return true;
    }
};

class ChdPostPhysicsNarrowPhaseBodyFilter : public BodyFilter {
public:
    const CharacterDownsync* mSelfChd;
    const CharacterDownsync* mSelfNextChd;
    BodyID   mSelfBodyID;
    uint64_t mSelfUd;
    const BaseBattleCollisionFilter* mBaseBattleFilter;

    ChdPostPhysicsNarrowPhaseBodyFilter(const CharacterDownsync* inSelfChd, const CharacterDownsync* inSelfNextChd, const BodyID& inSelfBodyID, const uint64_t inSelfUd, const BaseBattleCollisionFilter* baseBattleFilter) : mSelfChd(inSelfChd), mSelfNextChd(inSelfNextChd), mSelfBodyID(inSelfBodyID), mSelfUd(inSelfUd), mBaseBattleFilter(baseBattleFilter) {

    }

    virtual bool			ShouldCollide([[maybe_unused]] const BodyID &inBodyID) const {
        return inBodyID != mSelfBodyID;
    }

    virtual bool			ShouldCollideLocked([[maybe_unused]] const Body &inBody) const {
        if (nullptr == mBaseBattleFilter) {
            return true;
        }
        const uint64_t udRhs = inBody.GetUserData();
        const uint64_t udtRhs = mBaseBattleFilter->getUDT(udRhs);
        auto res = mBaseBattleFilter->validateLhsCharacterContact(mSelfChd, mSelfNextChd, udRhs, udtRhs, inBody);
        if (ValidateResult::AcceptContact != res && ValidateResult::AcceptAllContactsForThisBodyPair != res) {
            return false;
        }
        return true;
    }
};

class CharacterCollideShapeCollector : public JPH::CollideShapeCollector {
public:
    explicit CharacterCollideShapeCollector(const int currRdfId, RenderFrame* nextRdf, const JPH::BodyInterface* bi, const uint64_t ud, const uint64_t udt, const CharacterDownsync* currChd, CharacterDownsync* nextChd, JPH::Vec3Arg inUp, JPH::Vec3Arg baseOffset, BaseBattleCollisionFilter* filter) : mCurrRdfId(currRdfId), mNextRdf(nextRdf), mBi(bi), mUd(ud), mUdt(udt), mCurrChd(currChd), mNextChd(nextChd), mBaseOffset(baseOffset), mUp(inUp), mBaseBattleFilter(filter) {}

    int                     mCurrRdfId;
    RenderFrame*            mNextRdf;

    JPH::BodyID				mGroundBodyID;
    JPH::SubShapeID			mGroundBodySubShapeID;
    JPH::RVec3				mGroundPosition = JPH::RVec3::sZero();
    JPH::Vec3				mGroundNormal = JPH::Vec3::sZero();
    
    virtual void		AddHit(const JPH::CollideShapeResult& inResult) override {
        const uint64_t udRhs = mBi->GetUserData(inResult.mBodyID2);
        const uint64_t udtRhs = mBaseBattleFilter->getUDT(udRhs);
        auto normal = -inResult.mPenetrationAxis.Normalized();
        float dot = normal.Dot(mUp);
        if (dot > mBestDot) {
            // Find the hit that is most aligned with the up vector
            mGroundBodyID = inResult.mBodyID2;
            mGroundBodySubShapeID = inResult.mSubShapeID2;
            mGroundPosition = mBaseOffset + inResult.mContactPointOn2;
            mGroundNormal = normal;
            mBestDot = dot;
        }
    }

private:
    const JPH::BodyInterface*     mBi;
    const uint64_t                mUd;
    const uint64_t                mUdt;
    const CharacterDownsync*      mCurrChd;
    CharacterDownsync*            mNextChd;
    Vec3				      mUp;
    Vec3                     mBaseOffset;
    BaseBattleCollisionFilter*    mBaseBattleFilter;
    float				          mBestDot = -FLT_MAX;
};

#endif

