#ifndef CHARACTER_COLLIDE_SHAPE_COLLECTOR_H_
#define CHARACTER_COLLIDE_SHAPE_COLLECTOR_H_ 1

#include "BaseBattleCollisionFilter.h"
#include <Jolt/Physics/Body/BodyFilter.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Collision/CollideShape.h>

using namespace jtshared;
using namespace JPH;

class AimingRayBodyFilter : public BodyFilter {
public:
    const CharacterDownsync* mSelfChd;
    const CharacterDownsync* mSelfNextChd;
    BodyID   mSelfBodyID;
    uint64_t mSelfUd;
    const BaseBattleCollisionFilter* mBaseBattleFilter;

    AimingRayBodyFilter(const CharacterDownsync* inSelfChd, const CharacterDownsync* inSelfNextChd, const BodyID& inSelfBodyID, const uint64_t inSelfUd, const BaseBattleCollisionFilter* baseBattleFilter) : mSelfChd(inSelfChd), mSelfNextChd(inSelfNextChd), mSelfBodyID(inSelfBodyID), mSelfUd(inSelfUd), mBaseBattleFilter(baseBattleFilter) {

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
        auto res = mBaseBattleFilter->validateLhsCharacterAimingRayContact(mSelfChd, mSelfNextChd, udRhs, udtRhs, inBody);
        if (ValidateResult::AcceptContact != res && ValidateResult::AcceptAllContactsForThisBodyPair != res) {
            return false;
        }
        return true;
    }
};

class VisionBodyFilter : public BodyFilter {
public:
    const CharacterDownsync* mSelfChd;
    const CharacterDownsync* mSelfNextChd;
    BodyID   mSelfBodyID;
    uint64_t mSelfUd;
    const BaseBattleCollisionFilter* mBaseBattleFilter;

    VisionBodyFilter(const CharacterDownsync* inSelfChd, const CharacterDownsync* inSelfNextChd, const BodyID& inSelfBodyID, const uint64_t inSelfUd, const BaseBattleCollisionFilter* baseBattleFilter) : mSelfChd(inSelfChd), mSelfNextChd(inSelfNextChd), mSelfBodyID(inSelfBodyID), mSelfUd(inSelfUd), mBaseBattleFilter(baseBattleFilter) {

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

class ChdPostPhysicsNarrowPhaseBodyFilter : public BodyFilter {
public:
    const CharacterDownsync* mSelfChd;
    const CharacterDownsync* mSelfNextChd;
    BodyID   mSelfBodyID;
    uint64_t mSelfUd;
    const BaseBattleCollisionFilter* mBaseBattleFilter;

    ChdPostPhysicsNarrowPhaseBodyFilter(const CharacterDownsync* inSelfChd, const CharacterDownsync* inSelfNextChd, const BodyID& inSelfBodyID, const uint64_t inSelfUd, const BaseBattleCollisionFilter* baseBattleFilter) : mSelfChd(inSelfChd), mSelfNextChd(inSelfNextChd), mSelfBodyID(inSelfBodyID), mSelfUd(inSelfUd), mBaseBattleFilter(baseBattleFilter) {

    }

    virtual bool			ShouldCollide([[maybe_unused]] const BodyID& inBodyID) const {
        return inBodyID != mSelfBodyID;
    }

    virtual bool			ShouldCollideLocked([[maybe_unused]] const Body& inBody) const {
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

using ContactPoints = StaticArray<Vec3, 64>;
class CollisionUdHolder_ThreadSafe {
private:
    std::vector<uint64_t> uds;
    std::vector<ContactPoints> contactPointsPerUd;
    std::vector<Vec3> worldSpaceNorms;
    std::vector<BodyID> bodyIDs;
    std::vector<SubShapeID> subShapeIDs;
    atomic<int> cnt;
    int size;

    std::unordered_set<uint64_t> seenUdsDuringReading_NotThreadSafe;

public:
    CollisionUdHolder_ThreadSafe(const int inSize) {
        size = inSize;
        uds.reserve(inSize);
        contactPointsPerUd.reserve(inSize);
        for (int i = 0; i < inSize; ++i) {
            uds.push_back(0);
            contactPointsPerUd.push_back(ContactPoints());
            worldSpaceNorms.push_back(Vec3::sZero());
            bodyIDs.push_back(BodyID());
            subShapeIDs.push_back(SubShapeID());
        }
        cnt = 0;
    }

    // [WARNING] All fields, including "contactPointsPerUd", will be destructed implicitly when "delete holder" is called in "~CollisionUdHolderStockCache_ThreadSafe". See https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Core/StaticArray.h#L44 for more information.

    bool Add_ThreadSafe(const uint64_t inUd, const ContactPoints& inContactPoints, const Vec3& inWorldSpaceNorm, const BodyID& inBodyID, const SubShapeID& inSubShapeID) {
        int idx = cnt.fetch_add(1);
        if (idx >= size) {
            --cnt;
            return false;
        }
        uds[idx] = inUd;
        contactPointsPerUd[idx] = inContactPoints; // It does work this way, see https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Core/StaticArray.h#L225.
        worldSpaceNorms[idx] = inWorldSpaceNorm;
        bodyIDs[idx] = inBodyID;
        subShapeIDs[idx] = inSubShapeID;
        return true;
    }


    int GetCnt_Realtime() const {
        return cnt;
    }

    bool GetUd_NotThreadSafe(const int idx, uint64_t& outUd, ContactPoints& outContactPoints, Vec3& outWorldSpaceNorm, BodyID& outBodyID, SubShapeID& outSubShapeID) {
        uint64_t cand = uds[idx];
        if (seenUdsDuringReading_NotThreadSafe.count(cand)) return false;
        outUd = cand;
        outContactPoints = contactPointsPerUd[idx];
        outWorldSpaceNorm = worldSpaceNorms[idx];
        outBodyID = bodyIDs[idx];
        outSubShapeID = subShapeIDs[idx];
        seenUdsDuringReading_NotThreadSafe.insert(cand);
        return true;
    }

    void Clear_ThreadSafe() {
        cnt = 0;
        seenUdsDuringReading_NotThreadSafe.clear();
    }
};

class CollisionUdHolderStockCache_ThreadSafe {
private:
    std::vector<CollisionUdHolder_ThreadSafe*> holders;
    atomic<int> cnt;
    int size;

public:
    CollisionUdHolderStockCache_ThreadSafe(const int inSize, const int holderSize) {
        size = inSize;
        holders.reserve(inSize);
        for (int i = 0; i < inSize; ++i) {
            holders.push_back(new CollisionUdHolder_ThreadSafe(holderSize));
        }
        cnt = 0;
    }

    CollisionUdHolder_ThreadSafe* Take_ThreadSafe() {
        int idx = cnt.fetch_add(1);
        if (idx >= size) {
            --cnt;
            return nullptr;
        }
        CollisionUdHolder_ThreadSafe* holder = holders[idx];
        holder->Clear_ThreadSafe();
        return holder;
    }

    void Clear_ThreadSafe() {
        cnt = 0;
    }

    ~CollisionUdHolderStockCache_ThreadSafe() {
        cnt = 0;
        while (!holders.empty()) {
            CollisionUdHolder_ThreadSafe* holder = holders.back();
            holders.pop_back();
            delete holder;
        }
    }
};

class CharacterContactPushbackCollector : public JPH::CollideShapeCollector {
private:
    const JPH::BodyInterface* mBi;
    const uint64_t                mUd;
    const uint64_t                mUdt;
    const CharacterDownsync* mCurrChd;
    CharacterDownsync* mNextChd;
    Vec3				          mUp;
    Vec3                          mBaseOffset;
    BaseBattleCollisionFilter*    mBaseBattleFilter;
    float				          mGroundBestDot = 0;
    float				          mWallBestDot = FLT_MAX;

public:
    explicit CharacterContactPushbackCollector(const int currRdfId, RenderFrame* nextRdf, const JPH::BodyInterface* bi, const uint64_t ud, const uint64_t udt, const CharacterDownsync* currChd, CharacterDownsync* nextChd, JPH::Vec3Arg inUp, JPH::Vec3Arg baseOffset, BaseBattleCollisionFilter* filter) : mCurrRdfId(currRdfId), mNextRdf(nextRdf), mBi(bi), mUd(ud), mUdt(udt), mCurrChd(currChd), mNextChd(nextChd), mBaseOffset(baseOffset), mUp(inUp), mBaseBattleFilter(filter) {}

    int                     mCurrRdfId;
    RenderFrame*            mNextRdf;

    JPH::BodyID				mWallBodyID;
    JPH::Vec3				mWallNormal = JPH::Vec3::sZero();
    uint64_t                mWallUd = 0;

    JPH::BodyID				mGroundBodyID;
    JPH::SubShapeID         mGroundBodySubShapeID;
    JPH::Vec3               mGroundPosition = JPH::Vec3::sZero();
    JPH::Vec3				mGroundNormal = JPH::Vec3::sZero();
    uint64_t                mGroundUd = 0;

    bool                    mCrouchForced = false;
    
    virtual void		AddHit(const JPH::CollideShapeResult& inResult) override {
        const uint64_t udRhs = mBi->GetUserData(inResult.mBodyID2);
        const uint64_t udtRhs = mBaseBattleFilter->getUDT(udRhs);
        AddHit(udRhs, udtRhs, inResult.mBodyID2, inResult.mSubShapeID2, mBaseOffset + inResult.mContactPointOn2, inResult.mPenetrationAxis.Normalized(), false, false);
    }

    void		AddHit(const uint64_t udRhs, const uint64_t udtRhs, const BodyID rhsBodyID, const SubShapeID rhsSubShapeID, const Vec3& worldContactPosition, const Vec3& worldSpaceNormalIntoBarrier, const bool shouldSkipGroundServing, const bool shouldSkipWallServing) {
        if (UDT_TRIGGER == udtRhs || UDT_PICKABLE == udtRhs) {
            return;
        }
        auto normal = -worldSpaceNormalIntoBarrier;
        float dot = normal.Dot(mUp);
        if (!shouldSkipGroundServing && dot >= cDefaultWallDotThreshold) {
            if (dot > mGroundBestDot || (dot == mGroundBestDot && udRhs < mGroundUd)) {
                // Find the hit that is most aligned with the up vector
                mGroundBodyID = rhsBodyID;
                mGroundBodySubShapeID = rhsSubShapeID;
                mGroundPosition = worldContactPosition;
                mGroundNormal = normal;
                mGroundBestDot = dot;
                mGroundUd = udRhs;
            }
        }

        if (UDT_PLAYER != udtRhs && UDT_NPC != udtRhs) {
            float absDot = abs(dot);
            if (!shouldSkipWallServing && absDot < cDefaultWallDotThreshold) {
                if (absDot < mWallBestDot || (absDot == mWallBestDot && udRhs < mWallUd)) {
                    mWallBodyID = rhsBodyID;
                    mWallNormal = normal;
                    mWallBestDot = dot;
                    mWallUd = udRhs;
                }
            } else if (!shouldSkipGroundServing) {
                float ceilingDot = -dot;
                if (ceilingDot > 0.85f) {
                    mCrouchForced = true;
                }
            }
        }
    }
};

#endif

