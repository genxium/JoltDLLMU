#ifndef BULLET_COLLIDE_SHAPE_COLLECTOR_H_
#define BULLET_COLLIDE_SHAPE_COLLECTOR_H_ 1

#include "BaseBattleCollisionFilter.h"
#include <Jolt/Physics/Collision/CollideShape.h>
#include <Jolt/Physics/Body/BodyFilter.h>

class BulletBodyFilter : public IgnoreSingleBodyFilter {
public:
    using IgnoreSingleBodyFilter::IgnoreSingleBodyFilter;

    virtual bool	ShouldCollideLocked(const Body& inBody) const override
    {
        return !inBody.IsSensor();
    }
};

class BulletCollideShapeCollector : public JPH::CollideShapeCollector {

public:
    explicit BulletCollideShapeCollector(const int currRdfId, RenderFrame* nextRdf, const JPH::BodyInterface* bi, const uint64_t ud, const uint64_t udt, const Bullet* currBl, Bullet* nextBl, BaseBattleCollisionFilter* filter) : mCurrRdfId(currRdfId), mNextRdf(nextRdf), mBi(bi), mUd(ud), mUdt(udt), mCurrBl(currBl), mNextBl(nextBl), mFilter(filter) {}

    int                     mCurrRdfId;
    RenderFrame*            mNextRdf;

    virtual void		AddHit(const JPH::CollideShapeResult& inResult) override {
        const uint64_t udRhs = mBi->GetUserData(inResult.mBodyID2);
        const uint64_t udtRhs = mFilter->getUDT(udRhs);

        auto res = mFilter->validateLhsBulletContact(mCurrBl, udRhs, udtRhs);
        if (JPH::ValidateResult::AcceptContact != res && JPH::ValidateResult::AcceptAllContactsForThisBodyPair != res) {
            return;
        }

        mFilter->handleLhsBulletCollision(mCurrRdfId, mNextRdf, mUd, mUdt, mCurrBl, mNextBl, udRhs, udtRhs, inResult);
    }

private:
    const JPH::BodyInterface*     mBi;
    const uint64_t                mUd;
    const uint64_t                mUdt;
    const Bullet*                 mCurrBl;
    Bullet*                       mNextBl;
    BaseBattleCollisionFilter*    mFilter;
};

#endif
