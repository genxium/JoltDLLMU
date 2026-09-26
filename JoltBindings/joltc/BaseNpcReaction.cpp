#include "BaseNpcReaction.h"
#include "CharacterCollisionCollector.h"
#include "CollisionLayers.h"
#include "CollisionCallbacks.h"

#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/TaperedCylinderShape.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>

#include <climits>

#ifndef NDEBUG
#include "DebugLog.h"
#endif

void BaseNpcReaction::postStepDeriveNpcVisionReaction(int currRdfId, const Vec3& mvIntentionNorm, const Vec3& antiGravityNorm, const float gravityMagnitude, std::unordered_map<uint64_t, const Bullet*>& currBulletsMap, const BodyInterface* biNoLock, const NarrowPhaseQuery* narrowPhaseQuery, const BaseBattleCollisionFilter* baseBattleFilter, const DefaultBroadPhaseLayerFilter& bplf, const DefaultObjectLayerFilter& olf, const NpcCharacterDownsync* nextself, const CH_COLLIDER_T* selfCollider, const BodyID& selfBodyID, const uint64_t selfUd, const NpcGoal currNpcGoal, const uint64_t currNpcCachedCueCmd, const CharacterDownsync& currChd, const MassProperties& massProps, const Vec3& currChdFacing, const CharacterConfig* cc, CharacterDownsync* nextChd, const bool cvSupported, const bool cvInAir, const bool cvOnWall, const bool currNotDashing, const bool currEffInAir, const bool currIsFlying, const bool oldNextNotDashing, const bool oldNextEffInAir, const bool inJumpStartupOrJustEnded, CharacterBase::EGroundState cvGroundState, NpcGoal& outNextNpcGoal, uint64_t& outCmd, int& outLastFledRdfId) {

    Vec3 initVisionOffset(cc->vision_offset_x(), cc->vision_offset_y(), 0);
    auto visionInitTransform = cTurn90DegsAroundZAxisMat.PostTranslated(initVisionOffset); // Rotate, and then translate

    if (currIsFlying && InAirIdle1NoJump == currChd.ch_state() && cc->anti_gravity_when_idle()) {
        initVisionOffset.Set(0, 0, 0);
        visionInitTransform = cTurn180DegsAroundZAxisMat.PostTranslated(initVisionOffset); // Rotate, and then translate
    }

    Vec3 selfPosition(currChd.x(), currChd.y(), currChd.z());
    JPH::Quat offenderEffQ = 0 < currChdFacing.GetX() ? cIdentityQ : cTurnbackAroundYAxis;
    auto visionCOMTransform = (JPH::Mat44::sRotation(offenderEffQ)*visionInitTransform).PostTranslated(selfPosition); //and then rotate again by the NPC's orientation (affecting "initVisionOffset" too), and finally apply the NPC's position as translation
    
    float visionHalfHeight = cc->vision_half_height(), visionTopRadius = cc->vision_top_radius(), visionBottomRadius = cc->vision_bottom_radius();
    float visionConvexRadius = (visionTopRadius < visionBottomRadius ? visionTopRadius : visionBottomRadius)*0.9f; // Must be smaller than the min of these two 
    TaperedCylinderShapeSettings initVisionShapeSettings(visionHalfHeight, visionTopRadius, visionBottomRadius, visionConvexRadius);
    TaperedCylinderShapeSettings::ShapeResult shapeResult;
    TaperedCylinderShape initVisionShape(initVisionShapeSettings, shapeResult); // [WARNING] A transient, on-stack shape only bound to lifecycle of the current function & current thread. 

    // [REMINDER] Moreover, the center-of-mass DOESN'T have a local y-coordinate "0" within "initVisionShape" when (visionTopRadius != visionBottomRadius).

    initVisionShape.SetEmbedded(); // To allow deallocation on-stack, i.e. the "mRefCount" will equal 1 when it deallocates on-stack with the current function closure.
    
    const TaperedCylinderShape* effVisionShape = &initVisionShape; 

    const Vec3 effVisionOffsetFromNpcChd = offenderEffQ * initVisionOffset;
    const Vec3 visionNarrowPhaseInBaseOffset = selfPosition + effVisionOffsetFromNpcChd;

    const Vec3 visionDirection = currChdFacing;
    
    const TransformedShape selfTransformedShape = selfCollider->GetTransformedShape();
    const AABox selfAABB = selfTransformedShape.GetWorldSpaceBounds();

    VisionBodyFilter visionBodyFilter(currRdfId, &selfAABB, ((const CharacterDownsync*)&currChd), (const CharacterDownsync*)nextChd, selfBodyID, selfUd, UDT_NPC, baseBattleFilter);

    VISION_HIT_COLLECTOR_T visionHitCollector;
    const Vec3 scaling = Vec3::sOne();
    
    CollideShapeSettings settings;
    settings.mMaxSeparationDistance = cCollisionTolerance;
    settings.mActiveEdgeMode = EActiveEdgeMode::CollideOnlyWithActive;
    settings.mActiveEdgeMovementDirection = visionDirection;
    settings.mBackFaceMode = EBackFaceMode::IgnoreBackFaces;
    
	const AABox visionAABB = effVisionShape->GetWorldSpaceBounds(visionCOMTransform, scaling);

    /*
    For "narrowPhaseInBaseOffset", in most cases any value will work BUT it's recommended to choose ONLY among {Vec3::sZero(), centerOfMassTranslationOfBody1InWorldSpace}

    - using "narrowPhaseInBaseOffset = Vec3::sZero()" makes "CollideShapeResult.mContactPointOn[1|2]" in world space, and

    - using "narrowPhaseInBaseOffset = centerOfMassTranslationOfBody1InWorldSpace" makes "CollideShapeResult.mContactPointOn[1|2]" in "body1 local space"
    */

    narrowPhaseQuery->CollideShape(effVisionShape, scaling, visionCOMTransform, settings, visionNarrowPhaseInBaseOffset, visionHitCollector, bplf, olf, visionBodyFilter);
    
    bool hasVisionHit = visionHitCollector.HadHit();
    initVisionShape.Release();

#ifndef  NDEBUG
    /*
    if (8589934593UL == selfUd) {
        std::ostringstream oss;
        oss << "@currRdfId=" << currRdfId << ", (selfUd=" << selfUd << ", visionHitCollector.hitsCnt=" << visionHitCollector.mHits.size() << "), pos=(" << currChd.x() << ", " << currChd.y() << "), vel=(" << currChd.vel_x() << ", " << currChd.vel_y() << ")" << std::endl;
        Debug::Log(oss.str(), DColor::Orange);
    }
    */
#endif // ! NDEBUG

    /*
    Now that we've got all entities in vision, will start handling each.
    */
    uint64_t toHandleAllyUd = 0, toHandleOppoChUd = 0, toHandleOppoBlUd = 0, toHandleMvBlockerUd = 0;
    Vec3 selfPositionDiffForAllyUd = Vec3::sZero(), selfPositionDiffForOppoChUd = Vec3::sZero(), selfPositionDiffForOppoBlUd = Vec3::sZero();
    GapToJump currGapToJump; currGapToJump.set_vision_alignment(FLT_MAX); currGapToJump.set_anti_gravity_alignment(FLT_MAX);
    GapToJump minGapToJump;  minGapToJump.set_vision_alignment(FLT_MAX); minGapToJump.set_anti_gravity_alignment(FLT_MAX);
    GapToJump currGroundMvTolerance; currGroundMvTolerance.set_vision_alignment(0); currGroundMvTolerance.set_anti_gravity_alignment(0);

    BodyID toHandleMvBlockerBodyID;
    extractKeyEntitiesInVision(currRdfId, mvIntentionNorm, antiGravityNorm, currBulletsMap, biNoLock, narrowPhaseQuery, baseBattleFilter, selfCollider, &selfAABB, selfBodyID, selfUd, currChd, cc, nextChd, cvSupported, cvInAir, cvOnWall, currNotDashing, currEffInAir, currIsFlying, oldNextNotDashing, oldNextEffInAir, inJumpStartupOrJustEnded, cvGroundState, visionAABB, effVisionOffsetFromNpcChd, visionNarrowPhaseInBaseOffset, visionDirection, visionHitCollector, toHandleAllyUd, selfPositionDiffForAllyUd, toHandleOppoChUd, selfPositionDiffForOppoChUd, toHandleOppoBlUd, selfPositionDiffForOppoBlUd, toHandleMvBlockerUd, toHandleMvBlockerBodyID, currGapToJump, minGapToJump, currGroundMvTolerance);


    int fleeingAnchorRdfId = (outLastFledRdfId + globalPrimitiveConsts->default_fleeing_grace_period_rdf_cnt());
    bool inFleeingGracePeriod = (currRdfId < fleeingAnchorRdfId);

/*
#ifndef NDEBUG
    if (cvSupported) {
        std::ostringstream oss;
        oss << "@currRdfId=" << currRdfId << ", selfUd=" << selfUd << ", groundBodyID=" << selfCollider->GetGroundBodyID().GetIndexAndSequenceNumber() << ", has visionAABB=(minX=" << visionAABB.mMin.GetX() << ", maxX=" << visionAABB.mMax.GetX() << ", minY=" << visionAABB.mMin.GetY() << ", maxY=" << visionAABB.mMax.GetY() << "), currGroundMvTolerance=(" << currGroundMvTolerance.vision_alignment() << "," << currGroundMvTolerance.anti_gravity_alignment() << ")";
        Debug::Log(oss.str(), DColor::Orange);
    }
#endif
*/

    bool notDashing = BaseBattleCollisionFilter::chIsNotDashing(*nextChd);
    bool canJumpWithinInertia = BaseBattleCollisionFilter::chCanJumpWithInertia(currChd, cc, notDashing, inJumpStartupOrJustEnded);
    bool opponentBehindMe = false; 
    bool opponentAboveMe = false;
    bool opponentIsAttacking = false; 
    bool opponentIsFacingMe = false;

    int newVisionReaction = TARGET_CH_REACTION_UNCHANGED;

    bool temptingToMove = (temptingToMoveNpcGoalSet.count(outNextNpcGoal)) && (canJumpWithinInertia || currIsFlying);
    if (currIsFlying && cc->anti_gravity_when_idle() && InAirIdle1NoJump == nextChd->ch_state()) {
        temptingToMove = false;
    }

    uint64_t toRevengeOppoUd = nextself->to_revenge_ud();
    uint64_t toRevengeOppoUdt = BaseBattleCollisionFilter::getUDT(toRevengeOppoUd);
    if (0 == toHandleOppoChUd && 0 != toRevengeOppoUd && (UDT_PLAYER == toRevengeOppoUdt || UDT_NPC == toRevengeOppoUdt)) {
        toHandleOppoChUd = toRevengeOppoUd;
        selfPositionDiffForOppoChUd = cLengthEps*visionDirection;
        RVec3 toRevengeOppoColliderPos = baseBattleFilter->getColliderPositionByUd(toRevengeOppoUd, biNoLock);
        if (!toRevengeOppoColliderPos.IsNaN()) {
            selfPositionDiffForOppoChUd = toRevengeOppoColliderPos - selfPosition;
#ifndef NDEBUG
        /*
            std::ostringstream oss;
            oss << "@currRdfId=" << currRdfId << ", selfUd=" << selfUd << ", found bodyID for toRevengeOppoUd=" << toRevengeOppoUd << ", updated selfPositionDiffForOppoChUd=(" << selfPositionDiffForOppoChUd.GetX() << "," << selfPositionDiffForOppoChUd.GetY() << ") by toRevengeOppoColliderPos=(" << toRevengeOppoColliderPos.GetX() << ", " << toRevengeOppoColliderPos.GetY() << "), selfPosition=(" << selfPosition.GetX() << ", " << selfPosition.GetY() << "), currCachedCueCmd=" << currNpcCachedCueCmd;
            Debug::Log(oss.str(), DColor::Orange);
        } else {
            std::ostringstream oss;
            oss << "@currRdfId=" << currRdfId << ", selfUd=" << selfUd << ", couldn't find bodyID for toRevengeOppoUd=" << toRevengeOppoUd << ", using selfPositionDiffForOppoChUd=(" << selfPositionDiffForOppoChUd.GetX() << "," << selfPositionDiffForOppoChUd.GetY() << "), currCachedCueCmd=" << currNpcCachedCueCmd;
            Debug::Log(oss.str(), DColor::Yellow);
        */
#endif
        }
    }

    /**
    [TODO]

    Another branch to handle "toHandleOppoBlUd".
    */
    if (0 != toHandleOppoChUd && !selfPositionDiffForOppoChUd.IsNaN() && (0 == currChd.locking_on_ud() || toHandleOppoChUd == currChd.locking_on_ud())) {
        newVisionReaction = deriveNpcVisionReactionAgainstOppoChUd(currRdfId, baseBattleFilter, selfCollider, selfBodyID, selfUd, currChd, massProps, currChdFacing, cc, nextChd, cvSupported, cvInAir, cvOnWall, currNotDashing, currEffInAir, currIsFlying, oldNextNotDashing, oldNextEffInAir, inJumpStartupOrJustEnded, cvGroundState, canJumpWithinInertia, visionDirection, toHandleOppoChUd, selfPositionDiffForOppoChUd, opponentBehindMe, opponentAboveMe, opponentIsAttacking, opponentIsFacingMe);

       bool shouldHunt = true;
       bool shouldPause = false;
       bool inOppoBehindMeIgnoringPeriod = (currRdfId < (outLastFledRdfId + (globalPrimitiveConsts->default_fleeing_grace_period_rdf_cnt() >> 1)));
       if (inOppoBehindMeIgnoringPeriod && 0 == toRevengeOppoUd && opponentBehindMe) {
           toHandleOppoChUd = 0;
           selfPositionDiffForOppoChUd = Vec3::sZero();
           // [REMINDER] Might be re-assigned by "toRevengeOppoUd" which is the only way to interrupt a fleeing grace period.
           newVisionReaction = TARGET_CH_REACTION_UNCHANGED;
           shouldHunt = false;
       } else {
           if (TARGET_CH_REACTION_FOLLOW == newVisionReaction && !opponentBehindMe && opponentAboveMe) {
               if (!currIsFlying) {
                   // [WARNING] Update "minGapToJump" and "currGapToJump" in this case to help "selfUd" decide whether or not to follow this opponent, e.g. if the opponent stands on somewhere too high to reach.
                   float candVisionAlignment = std::abs(selfPositionDiffForOppoChUd.GetX());
                   float candAntiGravityAlignment = std::abs(selfPositionDiffForOppoChUd.GetY());
                   GapToJump virtualGapToJump;
                   virtualGapToJump.set_vision_alignment(candVisionAlignment);
                   virtualGapToJump.set_anti_gravity_alignment(candAntiGravityAlignment);

                   const float jumpAccMagY = cc->jump_acc_mag_y();
                   const int jumpStartupFrames = cc->jump_startup_frames();
                   const float chJumpAccSeconds = ((jumpStartupFrames + 1) * globalPrimitiveConsts->estimated_seconds_per_rdf());
                   const float chJumpInitSpeed = jumpAccMagY * chJumpAccSeconds;
                   const float extraAccendingY = ((chJumpInitSpeed * 0.5f) * chJumpAccSeconds);
                   float virtualGapEstimatedSpeedX = 0.8f * cc->speed();
                   bool isVirtualGapJumpable = isGapJumpable(gravityMagnitude, virtualGapToJump.vision_alignment() + cc->capsule_radius(), virtualGapToJump.anti_gravity_alignment(), virtualGapEstimatedSpeedX, chJumpAccSeconds, chJumpInitSpeed, extraAccendingY);

                   if (!isVirtualGapJumpable) {
                       shouldHunt = false;
                   }
               }
           } else if (TARGET_CH_REACTION_NOT_ENOUGH_MP == newVisionReaction) {
               shouldHunt = false;
               shouldPause = true;
           }
       }
        
        if (shouldPause) {
            newVisionReaction = TARGET_CH_REACTION_NOT_ENOUGH_MP;
            outNextNpcGoal = NpcGoal::NIdleIfGoHuntingThenPatrol;
        } else if (shouldHunt) {
            switch (currNpcGoal) {
            case NpcGoal::NIdle:
                outNextNpcGoal = NpcGoal::NHuntThenIdle;
                break;
            case NpcGoal::NIdleIfGoHuntingThenPatrol:
            case NpcGoal::NPatrol:
                outNextNpcGoal = NpcGoal::NHuntThenPatrol;
                break;
            case NpcGoal::NIdleIfGoHuntingThenPathPatrol:
            case NpcGoal::NPathPatrol:
                outNextNpcGoal = NpcGoal::NHuntThenPathPatrol;
                break;
            case NpcGoal::NFollowAlly:
                outNextNpcGoal = NpcGoal::NHuntThenFollowAlly;
                break;
            default:
                break;
            }
            nextChd->set_locking_on_ud(toHandleOppoChUd);
        } else {
            // As if hadn't seen the opponent.
            toHandleOppoChUd = 0;
            selfPositionDiffForOppoChUd = Vec3::sZero();
            newVisionReaction = TARGET_CH_REACTION_UNCHANGED;
            outNextNpcGoal = currNpcGoal;
            nextChd->set_locking_on_ud(0);
        }
    } else {
        switch (currNpcGoal) {
        case NpcGoal::NHuntThenIdle:
            outNextNpcGoal = NpcGoal::NIdle;
            newVisionReaction = TARGET_CH_REACTION_HUNTING_LOSS;
            temptingToMove = false;
            break;
        case NpcGoal::NHuntThenPatrol:
            outNextNpcGoal = NpcGoal::NPatrol;
            newVisionReaction = TARGET_CH_REACTION_HUNTING_LOSS;
            break;
        case NpcGoal::NHuntThenPathPatrol:
            outNextNpcGoal = NpcGoal::NPathPatrol;
            newVisionReaction = TARGET_CH_REACTION_HUNTING_LOSS;
            break;
        case NpcGoal::NHuntThenFollowAlly:
            outNextNpcGoal = NpcGoal::NFollowAlly;
            newVisionReaction = TARGET_CH_REACTION_HUNTING_LOSS;
            break;
        default:
            break;
        }
        toHandleOppoChUd = 0;
        selfPositionDiffForOppoChUd = Vec3::sZero();
        nextChd->set_locking_on_ud(0);
    }

    if (TARGET_CH_REACTION_FLEE_OPPO == newVisionReaction) {
        outLastFledRdfId = currRdfId;
    }

    InputFrameDecoded ifDecodedHolder;
    uint64_t inheritedCachedCueCmd = BaseBattleCollisionFilter::sanitizeCachedCueCmd(currNpcCachedCueCmd);
    BaseBattleCollisionFilter::decodeInput(inheritedCachedCueCmd, &ifDecodedHolder);
    int inheritedDirX = ifDecodedHolder.dx();
    if (0 == inheritedDirX) {
        if (temptingToMove) {
#ifndef NDEBUG
            if (globalPrimitiveConsts->ch_species().wolverine1() == nextChd->species_id()) {
                if (Idle1 == currChd.ch_state() && TARGET_CH_REACTION_HUNTING_LOSS == newVisionReaction) {
                    std::ostringstream oss;
                    oss << "@currRdfId=" << currRdfId << ", Wolverine1 selfUd=" << selfUd << " might begin movement at Idle1 due to HUNTING_LOSS, has inFleeingGracePeriod=" << inFleeingGracePeriod << ", outLastFledRdfId=" << outLastFledRdfId << ", outNextNpcGoal=" << outNextNpcGoal << ", visionDir=(" << visionDirection.GetX() << ", " << visionDirection.GetY() << "), visionAABB=(minX=" << visionAABB.mMin.GetX() << ", maxX=" << visionAABB.mMax.GetX() << ", minY=" << visionAABB.mMin.GetY() << ", maxY=" << visionAABB.mMax.GetY() << "), curr_ch_state=" << currChd.ch_state() << ", curr_frames_in_ch_state=" << currChd.frames_in_ch_state() << ", cvSupported=" << cvSupported << ", groundBodyUd=" << currChd.ground_ud() << ", toHandleOppoChUd=" << toHandleOppoChUd << ", selfPositionDiffForOppoChUd=(" << selfPositionDiffForOppoChUd.GetX() << ", " << selfPositionDiffForOppoChUd.GetY() << "), toHandleMvBlockerUd=" << toHandleMvBlockerUd << ", canJumpWithinInertia=" << canJumpWithinInertia << "; currGroundMvTolerance=" << currGroundMvTolerance.vision_alignment() << ", currGapToJump=(" << currGapToJump.vision_alignment() << ", " << currGapToJump.anti_gravity_alignment() << ")";
                    Debug::Log(oss.str(), DColor::Yellow);
                }
            }
#endif
            if (!inFleeingGracePeriod) {
                inheritedDirX = (0 < visionDirection.GetX() ? +2 : -2);
            }
        }
    }
    int inheritedDirY = 0; // [REMINDER] Intentionally a constant zero even for "currIsFlying" in this case, because when NOT hunting it's more convenient to just stop y-axis flying. 
    
#ifndef NDEBUG
    /*
    if (globalPrimitiveConsts->ch_species().bat1() == nextChd->species_id() && InAirIdle1NoJump != nextChd->ch_state()) {
        std::ostringstream oss;
        oss << "@currRdfId=" << currRdfId << ", flying selfUd=" << selfUd << " called extractKeyEntitiesInVision, pos=(" << currChd.x() << "," << currChd.y() << "), visionDirection=(" << visionDirection.GetX() << ", " << visionDirection.GetY() << "), inheritedDir=(" << inheritedDirX << ", " << inheritedDirY << "), temptingToMove=" << temptingToMove << ", byFarVisionReaction=" << newVisionReaction << ", byFarNpcGoal=" << outNextNpcGoal << ", currChState=" << currChd.ch_state() << ", currFc=" << currChd.frames_in_ch_state() << ", currGapToJump=(" << currGapToJump.vision_alignment() << ", " << currGapToJump.anti_gravity_alignment() << "), nextVel=(" << nextChd->vel_x() << "," << nextChd->vel_y() << "), toHandleMvBlockerUd=" << toHandleMvBlockerUd << ", locking_on_ud=" << currChd.locking_on_ud();
        Debug::Log(oss.str(), DColor::White);
    }
    */
#endif

    switch (newVisionReaction) {
        case TARGET_CH_REACTION_NOT_ENOUGH_MP:
        case TARGET_CH_REACTION_DEF1:
        case TARGET_CH_REACTION_USE_DRAGONPUNCH:
        case TARGET_CH_REACTION_USE_MELEE:
        case TARGET_CH_REACTION_USE_SLOT_C:
        case TARGET_CH_REACTION_HUNTING_LOSS:
            break;
        case TARGET_CH_REACTION_FOLLOW: {
            if (!opponentBehindMe) {
                // If opponent is currently behind me, there's no need to check MvBlocker.
                int groundAndMvBlockerReaction = deriveReactionAgainstGroundAndMvBlocker(currRdfId, mvIntentionNorm, antiGravityNorm, gravityMagnitude, biNoLock, selfCollider, &selfAABB, selfBodyID, selfUd, outNextNpcGoal, currChd, massProps, currChdFacing, cc, nextChd, cvSupported, cvInAir, cvOnWall, currNotDashing, currEffInAir, currIsFlying, oldNextNotDashing, oldNextEffInAir, inJumpStartupOrJustEnded, cvGroundState, canJumpWithinInertia, visionAABB, visionNarrowPhaseInBaseOffset, visionDirection, toHandleMvBlockerBodyID, toHandleMvBlockerUd, currGapToJump, minGapToJump, currGroundMvTolerance, newVisionReaction, toHandleOppoChUd, selfPositionDiffForOppoChUd, opponentBehindMe, opponentAboveMe, opponentIsAttacking, opponentIsFacingMe, temptingToMove, inFleeingGracePeriod);

                switch (groundAndMvBlockerReaction) {
                case TARGET_CH_REACTION_STOP_BY_MV_BLOCKER:
                case TARGET_CH_REACTION_JUMP_TOWARDS_MV_BLOCKER:
                    newVisionReaction = groundAndMvBlockerReaction;
                    break;
                }
            }
            break;
        }
        case TARGET_CH_REACTION_FLEE_OPPO: {
            if (opponentBehindMe) {
                // If opponent is currently in front of me, there's no need to check MvBlocker.
                int groundAndMvBlockerReaction = deriveReactionAgainstGroundAndMvBlocker(currRdfId, mvIntentionNorm, antiGravityNorm, gravityMagnitude, biNoLock, selfCollider, &selfAABB, selfBodyID, selfUd, outNextNpcGoal, currChd, massProps, currChdFacing, cc, nextChd, cvSupported, cvInAir, cvOnWall, currNotDashing, currEffInAir, currIsFlying, oldNextNotDashing, oldNextEffInAir, inJumpStartupOrJustEnded, cvGroundState, canJumpWithinInertia, visionAABB, visionNarrowPhaseInBaseOffset, visionDirection, toHandleMvBlockerBodyID, toHandleMvBlockerUd, currGapToJump, minGapToJump, currGroundMvTolerance, newVisionReaction, toHandleOppoChUd, selfPositionDiffForOppoChUd, opponentBehindMe, opponentAboveMe, opponentIsAttacking, opponentIsFacingMe, temptingToMove, inFleeingGracePeriod);

                switch (groundAndMvBlockerReaction) {
                case TARGET_CH_REACTION_STOP_BY_MV_BLOCKER:
                case TARGET_CH_REACTION_JUMP_TOWARDS_MV_BLOCKER:
                    newVisionReaction = groundAndMvBlockerReaction;
                    break;
                }
            }
            break;
        }
        default: {
            if (0 <= visionDirection.GetX() * inheritedDirX) {
                int groundAndMvBlockerReaction = deriveReactionAgainstGroundAndMvBlocker(currRdfId, mvIntentionNorm, antiGravityNorm, gravityMagnitude, biNoLock, selfCollider, &selfAABB, selfBodyID, selfUd, outNextNpcGoal, currChd, massProps, currChdFacing, cc, nextChd, cvSupported, cvInAir, cvOnWall, currNotDashing, currEffInAir, currIsFlying, oldNextNotDashing, oldNextEffInAir, inJumpStartupOrJustEnded, cvGroundState, canJumpWithinInertia, visionAABB, visionNarrowPhaseInBaseOffset, visionDirection, toHandleMvBlockerBodyID, toHandleMvBlockerUd, currGapToJump, minGapToJump, currGroundMvTolerance, newVisionReaction, toHandleOppoChUd, selfPositionDiffForOppoChUd, opponentBehindMe, opponentAboveMe, opponentIsAttacking, opponentIsFacingMe, temptingToMove, inFleeingGracePeriod);
                newVisionReaction = groundAndMvBlockerReaction;
            } // [REMINDER] If "0 > visionDirection.GetX() * inheritedDirX", the character is potentially turning around for revenge.
            break;
        }
    }

    if (TARGET_CH_REACTION_UNCHANGED == newVisionReaction) {
        // Intentionally left blank
    } else if (TARGET_CH_REACTION_USE_MELEE == newVisionReaction) {
        ifDecodedHolder.set_dx(0);
        ifDecodedHolder.set_dy(0);
        ifDecodedHolder.set_btn_a_level(0);
        ifDecodedHolder.set_btn_b_level(1);
        ifDecodedHolder.set_btn_c_level(0);
        ifDecodedHolder.set_btn_d_level(0);
        ifDecodedHolder.set_btn_e_level(0);
        ifDecodedHolder.set_btn_f_level(0);
        ifDecodedHolder.set_btn_l_level(0);
        ifDecodedHolder.set_btn_r_level(0);
    } else if (TARGET_CH_REACTION_USE_DRAGONPUNCH == newVisionReaction) {
        ifDecodedHolder.set_dx(0);
        ifDecodedHolder.set_dy(+2);
        ifDecodedHolder.set_btn_a_level(0);
        ifDecodedHolder.set_btn_b_level(1);
        ifDecodedHolder.set_btn_c_level(0);
        ifDecodedHolder.set_btn_d_level(0);
        ifDecodedHolder.set_btn_e_level(0);
        ifDecodedHolder.set_btn_f_level(0);
        ifDecodedHolder.set_btn_l_level(0);
        ifDecodedHolder.set_btn_r_level(0);
    } else if (TARGET_CH_REACTION_USE_FIREBALL == newVisionReaction) {
        ifDecodedHolder.set_dx(0);
        ifDecodedHolder.set_dy(-2);
        ifDecodedHolder.set_btn_a_level(0);
        ifDecodedHolder.set_btn_b_level(1);
        ifDecodedHolder.set_btn_c_level(0);
        ifDecodedHolder.set_btn_d_level(0);
        ifDecodedHolder.set_btn_e_level(0);
        ifDecodedHolder.set_btn_f_level(0);
        ifDecodedHolder.set_btn_l_level(0);
        ifDecodedHolder.set_btn_r_level(0);
    } else if (TARGET_CH_REACTION_USE_SLOT_C == newVisionReaction) {
        ifDecodedHolder.set_dx(0);
        ifDecodedHolder.set_dy(0);
        ifDecodedHolder.set_btn_a_level(0);
        ifDecodedHolder.set_btn_b_level(0);
        ifDecodedHolder.set_btn_c_level(1);
        ifDecodedHolder.set_btn_d_level(0);
        ifDecodedHolder.set_btn_e_level(0);
        ifDecodedHolder.set_btn_f_level(0);
        ifDecodedHolder.set_btn_l_level(0);
        ifDecodedHolder.set_btn_r_level(0);
    } else if (TARGET_CH_REACTION_SLIP_JUMP_TOWARDS_CH == newVisionReaction) {
        ifDecodedHolder.set_dx(0);
        ifDecodedHolder.set_dy(-2);
        ifDecodedHolder.set_btn_a_level(1);
        ifDecodedHolder.set_btn_b_level(0);
        ifDecodedHolder.set_btn_c_level(0);
        ifDecodedHolder.set_btn_d_level(0);
        ifDecodedHolder.set_btn_e_level(0);
        ifDecodedHolder.set_btn_f_level(0);
        ifDecodedHolder.set_btn_l_level(0);
        ifDecodedHolder.set_btn_r_level(0);
    } else if (TARGET_CH_REACTION_TURNAROUND_MV_BLOCKER == newVisionReaction) {
        bool toEnterFleeingGracePeriod = (currRdfId > (fleeingAnchorRdfId + 1));
        if (inFleeingGracePeriod) {
            int toMoveDirX = 0;
            int toMoveDirY = 0;
#ifndef NDEBUG
            /*
            if (currIsFlying) {
                std::ostringstream oss;
                oss << "@currRdfId=" << currRdfId << ", flying selfUd=" << selfUd << " has lastFledRdfId=" << outLastFledRdfId << ", newVisionReaction=TURNAROUND_MV_BLOCKER but in grace period, pos=(" << currChd.x() << ", " << currChd.y() << "), visionDirection=(" << visionDirection.GetX() << ", " << visionDirection.GetY() << "), currChS=" << currChd.ch_state() << ", currFc=" << currChd.frames_in_ch_state() << ", currGapToJump=(" << currGapToJump.vision_alignment() << ", " << currGapToJump.anti_gravity_alignment() << "), minGapToJump=(" << minGapToJump.vision_alignment() << ", " << minGapToJump.anti_gravity_alignment() << "), toHandleMvBlockerUd=" << toHandleMvBlockerUd << "\n";
                Debug::Log(oss.str(), DColor::Yellow);
            }
            */
#endif
            ifDecodedHolder.set_dx(toMoveDirX);
            ifDecodedHolder.set_dy(toMoveDirY);
        } else if (toEnterFleeingGracePeriod) {
            int toMoveDirX = 0;
            int toMoveDirY = 0;
#ifndef NDEBUG
            /*
            if (currIsFlying) {
                std::ostringstream oss;
                oss << "@currRdfId=" << currRdfId << ", flying selfUd=" << selfUd << " has lastFledRdfId=" << outLastFledRdfId << ", newVisionReaction=TURNAROUND_MV_BLOCKER but entered grace period, pos=(" << currChd.x() << ", " << currChd.y() << "), visionDirection=(" << visionDirection.GetX() << ", " << visionDirection.GetY() << "), currChS=" << currChd.ch_state() << ", currFc=" << currChd.frames_in_ch_state() << ", currGapToJump=(" << currGapToJump.vision_alignment() << ", " << currGapToJump.anti_gravity_alignment() << "), minGapToJump=(" << minGapToJump.vision_alignment() << ", " << minGapToJump.anti_gravity_alignment() << "), toHandleMvBlockerUd=" << toHandleMvBlockerUd << "\n";
                Debug::Log(oss.str(), DColor::Green);
            }
            */
#endif
            ifDecodedHolder.set_dx(toMoveDirX);
            ifDecodedHolder.set_dy(toMoveDirY);
            outLastFledRdfId = currRdfId;
            /* 
            [REMINDER] 

            After this assignment to "outLastFledRdfId", we'll have "inFleeingGracePeriod=true" in the next "globalPrimitiveConsts->default_fleeing_grace_period_rdf_cnt()" RenderFrames.

            Once the NPC has the next "currRdfId == fleeingAnchorRdfId", it implies "inFleeingGracePeriod=false && inFleeingGracePeriod=false" thus "TARGET_CH_REACTION_TURNAROUND_MV_BLOCKER" will actually occur. 
            */
        } else {
            int toMoveDirX = 0 < visionDirection.GetX() ? -2 : +2;
            int toMoveDirY = 0;
            if (currIsFlying) {
                if (0 == minGapToJump.vision_alignment()) {
                    // "(strictlyUp || strictlyDown) && holdableBothForwardAndBackward"
                    toMoveDirX = visionDirection.GetX();
                }
                float tolerance = 2 * cc->capsule_half_height();
                float anitGravityAlignmentAbs = fabs(currGapToJump.anti_gravity_alignment());
                if (tolerance < anitGravityAlignmentAbs) {
                    // Too far, just keep flying in the current y-offset
                    toMoveDirY = 0;
                } else if (0 > minGapToJump.anti_gravity_alignment()) {
                    toMoveDirY = (+1);
                } else if (0 < minGapToJump.anti_gravity_alignment()) {
                    toMoveDirY = (-1);
                }
#ifndef NDEBUG
                std::ostringstream oss;
                oss << "@currRdfId=" << currRdfId << ", flying selfUd=" << selfUd << " has lockingOnUd=" << nextChd->locking_on_ud() << " and turning around, lastFledRdfId=" << outLastFledRdfId << ", pos=(" << currChd.x() << "," << currChd.y() << "), visionDirection=(" << visionDirection.GetX() << ", " << visionDirection.GetY() << "), currChS=" << currChd.ch_state() << ", currFc=" << currChd.frames_in_ch_state() << ", currGapToJump=(" << currGapToJump.vision_alignment() << ", " << currGapToJump.anti_gravity_alignment() << "), minGapToJump=(" << minGapToJump.vision_alignment() << ", " << minGapToJump.anti_gravity_alignment() << "), toHandleMvBlockerUd=" << toHandleMvBlockerUd << ", toMoveDir=(" << toMoveDirX << ", " << toMoveDirY << ")\n";
                Debug::Log(oss.str(), DColor::Yellow);
            } else {
                if (globalPrimitiveConsts->ch_species().wolverine1() == nextChd->species_id()) {
                    std::ostringstream oss;
                    oss << "@currRdfId=" << currRdfId << ", walking selfUd=" << selfUd << " has lockingOnUd=" << nextChd->locking_on_ud() << " and turning around, lastFledRdfId=" << outLastFledRdfId << ", pos=(" << currChd.x() << "," << currChd.y() << "), visionDirection=(" << visionDirection.GetX() << ", " << visionDirection.GetY() << "), currChS=" << currChd.ch_state() << ", currFc=" << currChd.frames_in_ch_state() << ", currGapToJump=(" << currGapToJump.vision_alignment() << ", " << currGapToJump.anti_gravity_alignment() << "), toHandleMvBlockerUd=" << toHandleMvBlockerUd << ", toMoveDir=(" << toMoveDirX << ", " << toMoveDirY << ")\n";
                    Debug::Log(oss.str(), DColor::Yellow);
                }
#endif
            }
            ifDecodedHolder.set_dx(toMoveDirX);
            ifDecodedHolder.set_dy(toMoveDirY);
            outLastFledRdfId = currRdfId;
        }
        ifDecodedHolder.set_btn_a_level(0);
        ifDecodedHolder.set_btn_b_level(0);
        ifDecodedHolder.set_btn_c_level(0);
        ifDecodedHolder.set_btn_d_level(0);
        ifDecodedHolder.set_btn_e_level(0);
        ifDecodedHolder.set_btn_f_level(0);
        ifDecodedHolder.set_btn_l_level(0);
        ifDecodedHolder.set_btn_r_level(0);
    } else if (TARGET_CH_REACTION_JUMP_TOWARDS_CH == newVisionReaction || TARGET_CH_REACTION_JUMP_TOWARDS_MV_BLOCKER == newVisionReaction) {
        // [REMINDER] Not need to consider "currIsFlying" in this case.
        int toMoveDirX = 0 < visionDirection.GetX() ? +2 : -2;
        ifDecodedHolder.set_dx(toMoveDirX);
        ifDecodedHolder.set_dy(0);
        ifDecodedHolder.set_btn_a_level(1);
        ifDecodedHolder.set_btn_b_level(0);
        ifDecodedHolder.set_btn_c_level(0);
        ifDecodedHolder.set_btn_d_level(0);
        ifDecodedHolder.set_btn_e_level(0);
        ifDecodedHolder.set_btn_f_level(0);
        ifDecodedHolder.set_btn_l_level(0);
        ifDecodedHolder.set_btn_r_level(0);
    } else if (TARGET_CH_REACTION_HUNTING_LOSS == newVisionReaction) {
        ifDecodedHolder.set_dx(inheritedDirX);
        ifDecodedHolder.set_dy(inheritedDirY);
        ifDecodedHolder.set_btn_a_level(0);
        ifDecodedHolder.set_btn_b_level(0);
        ifDecodedHolder.set_btn_c_level(0);
        ifDecodedHolder.set_btn_d_level(0);
        ifDecodedHolder.set_btn_e_level(0);
        ifDecodedHolder.set_btn_f_level(0);
        ifDecodedHolder.set_btn_l_level(0);
        ifDecodedHolder.set_btn_r_level(0);
    } else {
        int toMoveDirX = 0, toMoveDirY = 0;
        if ((TARGET_CH_REACTION_STOP_BY_MV_BLOCKER == newVisionReaction || TARGET_CH_REACTION_NOT_ENOUGH_MP == newVisionReaction) || (NpcGoal::NIdle == outNextNpcGoal || NpcGoal::NIdleIfGoHuntingThenPatrol == outNextNpcGoal || NpcGoal::NIdleIfGoHuntingThenPathPatrol == outNextNpcGoal)) {
            toMoveDirX = 0;
            toMoveDirY = 0;
        } else if (0 != toHandleOppoChUd && !selfPositionDiffForOppoChUd.IsNaN()) {
            if (currIsFlying) {
                if (BaseBattleCollisionFilter::IsLengthNearZero(selfPositionDiffForOppoChUd.GetX())) {
                    toMoveDirX = 0;
                } else {
                    if (TARGET_CH_REACTION_FLEE_OPPO == newVisionReaction) {
                        toMoveDirX = 0 < selfPositionDiffForOppoChUd.GetX() ? -2 : +2;
                    } else {
                        toMoveDirX = 0 < selfPositionDiffForOppoChUd.GetX() ? +2 : -2;
                    }
                }
                if (0 == toMoveDirX) {
                    toMoveDirY = 0 < selfPositionDiffForOppoChUd.GetY() ? +2 : -2;
                } else {
                    // [REMINDER] The y-value of "selfPositionDiffForOppoChUd" might've be set to zero in "extractKeyEntitiesInVision" for "UDT_PLAYER/UDT_NPC". 
                    if (!BaseBattleCollisionFilter::IsLengthNearZero(selfPositionDiffForOppoChUd.GetY())) {
                        toMoveDirY = 0 < selfPositionDiffForOppoChUd.GetY() ? +1 : -1;
                    }
                }
            } else {
                if (TARGET_CH_REACTION_FLEE_OPPO == newVisionReaction) {
                    toMoveDirX = 0 < selfPositionDiffForOppoChUd.GetX() ? -2 : +2;
                } else {
                    toMoveDirX = 0 < selfPositionDiffForOppoChUd.GetX() ? +2 : -2;
                }
            }
#ifndef NDEBUG
            /*
            if (nonAttackingSet.count(currChd.ch_state()) && 0 > toMoveDirX * visionDirection.GetX()) {
                std::ostringstream oss;
                oss << "@currRdfId=" << currRdfId << ", speciesId=" << nextChd->species_id() << ", selfUd=" << selfUd << " turning around due to opponent ud and lockingOnUd=" << nextChd->locking_on_ud() << ", newVisionReaction=" << newVisionReaction << ", has outLastFledRdfId=" << outLastFledRdfId << ", outNextNpcGoal=" << outNextNpcGoal << ", visionDir=(" << visionDirection.GetX() << ", " << visionDirection.GetY() << "), visionAABB=(minX=" << visionAABB.mMin.GetX() << ", maxX=" << visionAABB.mMax.GetX() << ", minY=" << visionAABB.mMin.GetY() << ", maxY=" << visionAABB.mMax.GetY() << "), curr_ch_state=" << currChd.ch_state() << ", curr_frames_in_ch_state=" << currChd.frames_in_ch_state() << ", cvSupported=" << cvSupported << ", groundBodyUd=" << currChd.ground_ud() << ", toHandleOppoChUd=" << toHandleOppoChUd << ", selfPositionDiffForOppoChUd=(" << selfPositionDiffForOppoChUd.GetX() << ", " << selfPositionDiffForOppoChUd.GetY() << "), toHandleMvBlockerUd=" << toHandleMvBlockerUd << ", canJumpWithinInertia=" << canJumpWithinInertia << "; currGroundMvTolerance=" << currGroundMvTolerance.vision_alignment() << ", currGapToJump=(" << currGapToJump.vision_alignment() << ", " << currGapToJump.anti_gravity_alignment() << ")\n";
                Debug::Log(oss.str(), DColor::Green);
            }
            */
#endif
        } else if (0 != toHandleAllyUd && !selfPositionDiffForAllyUd.IsNaN()) {
            if (NFollowAlly == outNextNpcGoal) {
                // [REMINDER] Don't blindly allow ally following.
                if (currIsFlying) {
                    if (BaseBattleCollisionFilter::IsLengthNearZero(selfPositionDiffForAllyUd.GetX())) {
                        toMoveDirX = 0;
                    } else {
                        toMoveDirX = 0 < selfPositionDiffForAllyUd.GetX() ? +2 : -2;
                    }
                    if (0 == toMoveDirX) {
                        toMoveDirY = 0 < selfPositionDiffForAllyUd.GetY() ? +2 : -2;
                    } else {
                        if (!BaseBattleCollisionFilter::IsLengthNearZero(selfPositionDiffForAllyUd.GetY())) {
                            toMoveDirY = 0 < selfPositionDiffForAllyUd.GetY() ? +1 : -1;
                        }
                    }
                } else {
                    toMoveDirX = 0 < selfPositionDiffForAllyUd.GetX() ? +2 : -2;
                }
            } else {
                toMoveDirX = inheritedDirX;
                toMoveDirY = inheritedDirY;
            }
        } else {
            toMoveDirX = inheritedDirX;
            toMoveDirY = inheritedDirY;
        }
       
        // It's important to unset "BtnALevel" if no proactive jump is implied by vision reaction, otherwise its value will remain even after execution and sanitization
        ifDecodedHolder.set_dx(toMoveDirX);
        ifDecodedHolder.set_dy(toMoveDirY);
        ifDecodedHolder.set_btn_a_level(0);
        ifDecodedHolder.set_btn_b_level(0);
        ifDecodedHolder.set_btn_c_level(0);
        ifDecodedHolder.set_btn_d_level(0);
        ifDecodedHolder.set_btn_e_level(0);
        ifDecodedHolder.set_btn_f_level(0);
        ifDecodedHolder.set_btn_l_level(0);
        ifDecodedHolder.set_btn_r_level(0);
    }

#ifndef NDEBUG
    /*
    if (0 < ifDecodedHolder.btn_a_level()) {
        std::ostringstream oss;
        oss << "@currRdfId=" << currRdfId << ", selfUd=" << selfUd << " attempts jumping because newVisionReaction=" << newVisionReaction << ", has visionDir=(" << visionDirection.GetX() << ", " << visionDirection.GetY() << "), visionAABB=(minX=" << visionAABB.mMin.GetX() << ", maxX=" << visionAABB.mMax.GetX() << ", minY=" << visionAABB.mMin.GetY() << ", maxY=" << visionAABB.mMax.GetY() << "), currChS=" << currChd.ch_state() << ", currFc=" << currChd.frames_in_ch_state() << ", cvSupported=" << cvSupported << ", groundBodyUd=" << currChd.ground_ud() << ", toHandleMvBlockerUd=" << toHandleMvBlockerUd << ", canJumpWithinInertia=" << canJumpWithinInertia << "; currGroundMvTolerance=" << currGroundMvTolerance.vision_alignment() << ", currGapToJump=(" <<  currGapToJump.vision_alignment() << ", " << currGapToJump.anti_gravity_alignment() << ")";
        Debug::Log(oss.str(), DColor::Orange);
    }

    if (globalPrimitiveConsts->ch_species().bat1() == nextChd->species_id()) {
        std::ostringstream oss;
        oss << "@currRdfId=" << currRdfId << ", selfUd=" << selfUd << " newVisionReaction=" << newVisionReaction << ", has visionDir=(" << visionDirection.GetX() << ", " << visionDirection.GetY() << "), currChS=" << currChd.ch_state() << ", currFc=" << currChd.frames_in_ch_state() << ", toHandleMvBlockerUd=" << toHandleMvBlockerUd << ", toHandleOppoChUd=" << toHandleOppoChUd << "; vel=(" << nextChd->vel_x() << ", " << nextChd->vel_y() << "), ifDecodedHolder.dir=(" << ifDecodedHolder.dx() << ", " << ifDecodedHolder.dy() << "), currGapToJump=(" <<  currGapToJump.vision_alignment() << ", " << currGapToJump.anti_gravity_alignment() << "), minGapToJump=(" << minGapToJump.vision_alignment() << ", " << minGapToJump.anti_gravity_alignment() << "), selfPositionDiffForOppoChUd=(" << selfPositionDiffForOppoChUd.GetX() << ", " << selfPositionDiffForOppoChUd.GetY() << ")";
        Debug::Log(oss.str(), DColor::Orange);
    }
    */
#endif
    uint64_t newCachedCueCmd = BaseBattleCollisionFilter::encodeInput(ifDecodedHolder);
    outCmd = newCachedCueCmd;
}

void BaseNpcReaction::extractKeyEntitiesInVision(int currRdfId, const Vec3& mvIntentionNorm, const Vec3& antiGravityNorm, std::unordered_map<uint64_t, const Bullet*>& currBulletsMap, const BodyInterface* biNoLock, const NarrowPhaseQuery* narrowPhaseQuery, const BaseBattleCollisionFilter* baseBattleFilter, const CH_COLLIDER_T* selfCollider, const AABox* selfAABB, const BodyID& selfBodyID, const uint64_t selfUd, const CharacterDownsync& currChd, const CharacterConfig* cc, CharacterDownsync* nextChd, const bool cvSupported, const bool cvInAir, const bool cvOnWall, const bool currNotDashing, const bool currEffInAir, const bool currIsFlying, const bool oldNextNotDashing, const bool oldNextEffInAir, const bool inJumpStartupOrJustEnded, CharacterBase::EGroundState cvGroundState, const AABox& visionAABB, const Vec3Arg& effVisionOffsetFromNpcChd, const Vec3Arg& visionNarrowPhaseInBaseOffset, const Vec3Arg& visionDirection, const VISION_HIT_COLLECTOR_T& visionHitCollector, uint64_t& outToHandleAllyUd, Vec3& outselfPositionDiffForAllyUd, uint64_t& outToHandleOppoChUd, Vec3& outSelfPositionDiffForOppoChUd, uint64_t& outToHandleOppoBlUd, Vec3& outSelfPositionDiffForOppoBlUd, uint64_t& outToHandleMvBlockerUd, BodyID& outToHandleMvBlockerBodyID, GapToJump& outCurrGapToJump, GapToJump& outMinGapToJump, GapToJump& outCurrGroundMvTolerance) {
    if (!visionHitCollector.HadHit()) return;

    float selfAABBMvIntentionAlignmentMax = selfAABB->mMax.Dot(mvIntentionNorm);
    float selfAABBMvIntentionAlignmentMin = selfAABB->mMin.Dot(mvIntentionNorm);
    if (selfAABBMvIntentionAlignmentMax < selfAABBMvIntentionAlignmentMin) {
        std::swap(selfAABBMvIntentionAlignmentMax, selfAABBMvIntentionAlignmentMin);
    }

    float selfAABBAntiGAlignmentMax = selfAABB->mMax.Dot(antiGravityNorm);
    float selfAABBAntiGAlignmentMin = selfAABB->mMin.Dot(antiGravityNorm);
    if (selfAABBAntiGAlignmentMax < selfAABBAntiGAlignmentMin) {
        std::swap(selfAABBAntiGAlignmentMax, selfAABBAntiGAlignmentMin);
    }

    float selfAABBVisionAlignmentMax = selfAABB->mMax.Dot(visionDirection);
    float selfAABBVisionAlignmentMin = selfAABB->mMin.Dot(visionDirection);
    if (selfAABBVisionAlignmentMax < selfAABBVisionAlignmentMin) {
        std::swap(selfAABBVisionAlignmentMax, selfAABBVisionAlignmentMin);
    }

    float bestVisionAlignmentForOppo = FLT_MAX;
    float bestVisionAlignmentForAlly = FLT_MAX;
    float bestVisionAlignmentForMvBlocker = FLT_MAX;

    const Vec3 lhsPos = selfCollider->GetPosition(false); 
    const Vec3 lhsCOMPos = selfCollider->GetCenterOfMassPosition(false);
    int hitsCnt = visionHitCollector.mHits.size();
 
    const BodyID& groundBodyID = cvSupported ? selfCollider->GetGroundBodyID() : BodyID();

    float groundAABBMvIntentionAlignmentMax = 0;
    float groundAABBMvIntentionAlignmentMin = 0;
    float groundAABBAntiGAlignmentMax = 0;
    float groundAABBAntiGAlignmentMin = 0;
    float groundAABBVisionAlignmentMax = 0;
    float groundAABBVisionAlignmentMin = 0;
    if (!groundBodyID.IsInvalid()) {
        const TransformedShape& groundTransformedShape = biNoLock->GetTransformedShape(groundBodyID);
        const AABox& groundAABB = groundTransformedShape.GetWorldSpaceBounds();
        groundAABBMvIntentionAlignmentMax = groundAABB.mMax.Dot(mvIntentionNorm);
        groundAABBMvIntentionAlignmentMin = groundAABB.mMin.Dot(mvIntentionNorm);
        if (groundAABBMvIntentionAlignmentMax < groundAABBMvIntentionAlignmentMin) {
            std::swap(groundAABBMvIntentionAlignmentMax, groundAABBMvIntentionAlignmentMin);
        }
        groundAABBAntiGAlignmentMax = groundAABB.mMax.Dot(antiGravityNorm);
        groundAABBAntiGAlignmentMin = groundAABB.mMin.Dot(antiGravityNorm);
        if (groundAABBAntiGAlignmentMax < groundAABBAntiGAlignmentMin) {
            std::swap(groundAABBAntiGAlignmentMax, groundAABBAntiGAlignmentMin);
        }
        groundAABBVisionAlignmentMax = groundAABB.mMax.Dot(visionDirection);
        groundAABBVisionAlignmentMin = groundAABB.mMin.Dot(visionDirection);
        if (groundAABBVisionAlignmentMax < groundAABBVisionAlignmentMin) {
            std::swap(groundAABBVisionAlignmentMax, groundAABBVisionAlignmentMin);
        }
    }
    bool foundSameLockedUd = false;
    for (int i = 0; i < hitsCnt; i++) {
        const CollideShapeCollector::ResultType hit = visionHitCollector.mHits.at(i);
        const BodyID rhsBodyID = hit.mBodyID2;
        float rhsVisionAlignmentFromNpcChdPosition = visionDirection.Dot(hit.mContactPointOn1 + effVisionOffsetFromNpcChd);
        if (!rhsBodyID.IsInvalid() && rhsBodyID == groundBodyID) {
            // [WARNING] When "groundBody" is of complicated shape, it's too inefficient to traverse all its vertices and find the largest projected value on "visionDirection", instead we can just allow "groundBody" to collide with "effVisionShape" and use the immediately visible distance as "currGroundMvTolerance" to roughly decide whether or not we can move on.
            outCurrGroundMvTolerance.set_vision_alignment(rhsVisionAlignmentFromNpcChdPosition);
            continue;
            // [WARNING] Intentionally NOT proceeding from here even if the "rhsBodyID" refers to an opponent character or bullet.
        }
        
        const TransformedShape& rhsTransformedShape = biNoLock->GetTransformedShape(rhsBodyID);
        const AABox& rhsAABB = rhsTransformedShape.GetWorldSpaceBounds();
        const uint64_t udRhs = biNoLock->GetUserData(rhsBodyID);
        const uint64_t udtRhs = BaseBattleCollisionFilter::getUDT(udRhs);

#ifndef NDEBUG
        /*
        if (8589934593UL == selfUd) {
            std::ostringstream oss;
            oss << "@currRdfId=" << currRdfId << ", selfUd=" << selfUd << " checking udRhs=" << udRhs << ", rhsVisionAlignmentFromNpcChdPosition=" << rhsVisionAlignmentFromNpcChdPosition << ", hit.mContactPointOn1=(" << hit.mContactPointOn1.GetX() << ", " << hit.mContactPointOn1.GetY() << ") by far bestVisionAlignmentForMvBlocker=" << bestVisionAlignmentForMvBlocker;
            Debug::Log(oss.str(), DColor::Orange);
        }
        */
#endif

        switch (udtRhs) {
        case UDT_PLAYER: 
        case UDT_NPC: {
            if (foundSameLockedUd) {
                continue;
            }
            VisionBodyFilter visionRayCastBodyFilter(currRdfId, selfAABB, ((const CharacterDownsync*)&currChd), (const CharacterDownsync*)nextChd, selfBodyID, selfUd, UDT_NPC, baseBattleFilter);
            RRayCast ray(visionNarrowPhaseInBaseOffset, hit.mContactPointOn1);
            bool rayTestPassed = false;
            RayCastResult rcResult;
            narrowPhaseQuery->CastRay(ray, rcResult, {}, {}, visionRayCastBodyFilter); // [REMINDER] "RayCast direction" MUST come with a magnitude, i.e. DON'T just use a normalized vector!
            if (!rcResult.mBodyID.IsInvalid() && rcResult.mBodyID != rhsBodyID) {
                // If blocked by others.
                continue;
            }
            const CharacterDownsync* rhsCurrChd = baseBattleFilter->immutableCurrChdPtrFromUd(udtRhs, udRhs);
            if (nullptr == rhsCurrChd) {
                continue;
            }
            auto& rhsCc = globalConfigConsts->character_configs().at(rhsCurrChd->species_id());
            const Vec3 rhsPos = biNoLock->GetPosition(rhsBodyID);
            const Vec3 rhsCOMPos = biNoLock->GetCenterOfMassPosition(rhsBodyID);
            Vec3 selfPositionDiff = (rhsPos - lhsPos);
            if (currIsFlying) {
                selfPositionDiff = (rhsCOMPos - lhsCOMPos);
                if (fabs(selfPositionDiff.GetY()) < 0.3f * rhsCc.capsule_half_height()) {
                    selfPositionDiff.SetY(0.f);
                }
            }
            if (rhsCurrChd->bullet_team_id() != currChd.bullet_team_id()) {
                if (currChd.locking_on_ud() == udRhs) {
                    // [REMINDER] Lock on the same opponent whenever possible.
                    foundSameLockedUd = true;
                } else {
                    if (rhsVisionAlignmentFromNpcChdPosition >= bestVisionAlignmentForOppo) {
                        continue;
                    }
                    bestVisionAlignmentForOppo = rhsVisionAlignmentFromNpcChdPosition;
                }
                 
                outToHandleOppoChUd = udRhs;
                outToHandleOppoBlUd = 0;
                outSelfPositionDiffForOppoChUd = selfPositionDiff;
                outSelfPositionDiffForOppoBlUd = Vec3::sZero();
            } else {
                if (rhsVisionAlignmentFromNpcChdPosition >= bestVisionAlignmentForAlly) {
                    continue;
                }

                bestVisionAlignmentForAlly = rhsVisionAlignmentFromNpcChdPosition;
                outToHandleAllyUd = udRhs;
                outselfPositionDiffForAllyUd = selfPositionDiff;
            }
            break;
        }
        case UDT_BL: {
            if (foundSameLockedUd) {
                continue;
            }
            if (!currBulletsMap.count(udRhs)) {
                continue;
            }
            const Bullet* rhsCurrBl = currBulletsMap.at(udRhs);
            const Vec3 rhsPos = biNoLock->GetPosition(rhsBodyID);
            const Vec3 rhsCOMPos = biNoLock->GetCenterOfMassPosition(rhsBodyID);
            const Vec3 selfPositionDiff = currIsFlying ? (rhsCOMPos - lhsCOMPos) : (rhsPos - lhsPos);
            if (rhsCurrBl->team_id() != currChd.bullet_team_id()) {
                Vec3 rhsCurrBlFacing = Quat(rhsCurrBl->q_x(), rhsCurrBl->q_y(), rhsCurrBl->q_z(), rhsCurrBl->q_w())*Vec3::sAxisX();
                if (0 <= selfPositionDiff.Dot(rhsCurrBlFacing)) {
                    continue; // seemingly not offensive
                }

                if (rhsVisionAlignmentFromNpcChdPosition > bestVisionAlignmentForOppo) {
                    continue;
                }

                bestVisionAlignmentForOppo = rhsVisionAlignmentFromNpcChdPosition;
                outToHandleOppoChUd = 0;
                outToHandleOppoBlUd = udRhs;
                outSelfPositionDiffForOppoChUd = Vec3::sZero();
                outSelfPositionDiffForOppoBlUd = selfPositionDiff;
            } else {
                // [TODO] Handling of "for ally bullets" 
            }
            break;
        }
        case UDT_TRAP: 
        case UDT_OBSTACLE: {
            bool isGround = (udRhs == nextChd->ground_ud());
            if (isGround) {
                continue;
            }

            bool isAlongForwardMv = (0 < rhsVisionAlignmentFromNpcChdPosition);

            if (!isAlongForwardMv) {
                // Not a "movement blocker candidate" 
#ifndef  NDEBUG
                /*
                if (globalPrimitiveConsts->ch_species().bat1() == currChd.species_id() && InAirIdle1NoJump != currChd.ch_state()) {
                    std::ostringstream oss;
                    oss << "@currRdfId=" << currRdfId << ", (selfUd=" << selfUd << ", udRhs=" << udRhs << "), pos=(" << currChd.x() << ", " << currChd.y() << "), vel=(" << currChd.vel_x() << ", " << currChd.vel_y() << "), skipping because NOT isAlongForwardMv, rhsVisionAlignmentFromNpcChdPosition=" << rhsVisionAlignmentFromNpcChdPosition << "." << std::endl;
                    Debug::Log(oss.str(), DColor::Yellow);
                }
                */
#endif // ! NDEBUG
                continue;
            }

            float rhsAABBMvIntentionAlignmentMax = rhsAABB.mMax.Dot(mvIntentionNorm);
            float rhsAABBMvIntentionAlignmentMin = rhsAABB.mMin.Dot(mvIntentionNorm);
            if (rhsAABBMvIntentionAlignmentMax < rhsAABBMvIntentionAlignmentMin) {
                std::swap(rhsAABBMvIntentionAlignmentMax, rhsAABBMvIntentionAlignmentMin);
            }

            float rhsAABBAntiGAlignmentMax = rhsAABB.mMax.Dot(antiGravityNorm);
            float rhsAABBAntiGAlignmentMin = rhsAABB.mMin.Dot(antiGravityNorm);
            if (rhsAABBAntiGAlignmentMax < rhsAABBAntiGAlignmentMin) {
                std::swap(rhsAABBAntiGAlignmentMax, rhsAABBAntiGAlignmentMin);
            }

            float rhsAABBVisionAlignmentMax = rhsAABB.mMax.Dot(visionDirection);
            float rhsAABBVisionAlignmentMin = rhsAABB.mMin.Dot(visionDirection);
            if (rhsAABBVisionAlignmentMax < rhsAABBVisionAlignmentMin) {
                std::swap(rhsAABBVisionAlignmentMax, rhsAABBVisionAlignmentMin);
            }

            bool holdableBothForwardAndBackward = (rhsAABBVisionAlignmentMax >= selfAABBVisionAlignmentMax && rhsAABBVisionAlignmentMin <= selfAABBVisionAlignmentMin);
            bool strictlyUp = (rhsAABBAntiGAlignmentMin + cCollisionTolerance >= selfAABBAntiGAlignmentMax); // the bottom of rhs is higher than self top
            bool strictlyDown = (rhsAABBAntiGAlignmentMax <= selfAABBAntiGAlignmentMin + cCollisionTolerance); // the top of rhs is lower than self bottom
            if (holdableBothForwardAndBackward && (!strictlyUp && !strictlyDown)) {
                strictlyUp = (rhsAABBAntiGAlignmentMax >= selfAABBAntiGAlignmentMax);
                if (!strictlyUp) {
                    strictlyDown = true; 
                }
            }

            bool strictlyRight = (rhsAABBMvIntentionAlignmentMin + cCollisionTolerance >= selfAABBMvIntentionAlignmentMax); 
            bool strictlyLeft = (rhsAABBMvIntentionAlignmentMax <= selfAABBMvIntentionAlignmentMin + cCollisionTolerance); 
            bool holdableBothUpwardAndDownward = (rhsAABBAntiGAlignmentMax >= selfAABBAntiGAlignmentMax && rhsAABBAntiGAlignmentMin <= selfAABBAntiGAlignmentMin);
            if (holdableBothUpwardAndDownward && (!strictlyRight && !strictlyLeft)) {
                strictlyRight = (rhsAABBMvIntentionAlignmentMax >= selfAABBMvIntentionAlignmentMax);
                if (!strictlyRight) {
                    strictlyLeft = true;
                }
            }

            if (0 > mvIntentionNorm.GetX()) {
                std::swap(strictlyLeft, strictlyRight);
            } 
            if (0 > mvIntentionNorm.GetY()) {
                std::swap(strictlyUp, strictlyDown);
            }
            
            if (!currIsFlying) {
                if (strictlyUp && holdableBothForwardAndBackward) {
                    // Not a "movement blocker candidate" 
                    continue;
                }
            } else {
                // For a flying NPC, "strictlyUp" might be a valid "movement blocker candidate"
                if ((strictlyUp || strictlyDown) && holdableBothForwardAndBackward) {
                    rhsVisionAlignmentFromNpcChdPosition = FLT_MAX * 0.5;
                }
            }

            bool compositingGap = false;
            if (currIsFlying) {
                // For composite gap 
                if (0 == outMinGapToJump.vision_alignment() && (strictlyRight || strictlyLeft)) {
                    rhsVisionAlignmentFromNpcChdPosition = bestVisionAlignmentForMvBlocker;
                    compositingGap = true;
                } else if (0 == outMinGapToJump.anti_gravity_alignment() && (strictlyUp || strictlyDown)) {
                    rhsVisionAlignmentFromNpcChdPosition = bestVisionAlignmentForMvBlocker;
                    compositingGap = true;
                }
            }

            if (rhsVisionAlignmentFromNpcChdPosition > bestVisionAlignmentForMvBlocker) {
                if (!compositingGap) {
                    continue;
                }
            }

            bool rayTestPassed = false, rayTestCanIgnoreground = false;

            if (!currIsFlying) {
                rayTestCanIgnoreground = (rhsAABBVisionAlignmentMax > groundAABBVisionAlignmentMax) && (rhsAABBAntiGAlignmentMax <= groundAABBAntiGAlignmentMin);
            } else {
                rayTestCanIgnoreground = true;
            }

            VisionBodyFilter visionRayCastBodyFilter(currRdfId, selfAABB, ((const CharacterDownsync*)&currChd), (const CharacterDownsync*)nextChd, selfBodyID, selfUd, UDT_NPC, nullptr);
            RRayCast ray(visionNarrowPhaseInBaseOffset, hit.mContactPointOn1);
            RayCastResult rcResult;
            narrowPhaseQuery->CastRay(ray, rcResult, {}, {}, visionRayCastBodyFilter); // [REMINDER] "RayCast direction" MUST come with a magnitude, i.e. DON'T just use a normalized vector!
            if (!rcResult.mBodyID.IsInvalid() && rcResult.mBodyID != rhsBodyID) {
                if (!rayTestCanIgnoreground && rcResult.mBodyID == groundBodyID) {
                    /* 
                    // [REMINDER] If "groundBodyID" has too much overlapping volume with "rhsBodyID" (which ideally it shouldn't have any), then "rhsBodyID" might fail this RayCast test.
#ifndef NDEBUG
                    std::ostringstream oss;
                    oss << "@currRdfId=" << currRdfId << ", selfUd=" << selfUd << " has visionAABB=(minX=" << visionAABB.mMin.GetX() << ", maxX=" << visionAABB.mMax.GetX() << ", minY=" << visionAABB.mMin.GetY() << ", maxY=" << visionAABB.mMax.GetY() << "), curr_ch_state=" << currChd.ch_state() << ", curr_frames_in_ch_state=" << currChd.frames_in_ch_state() << ", cvSupported=" << cvSupported << ", vision hit rhsBodyID=" << rhsBodyID.GetIndexAndSequenceNumber() << " is invalid due to being blocked by rcResult.mBodyID=" << rcResult.mBodyID.GetIndexAndSequenceNumber();
                    Debug::Log(oss.str(), DColor::Orange);
#endif
                */
                    continue;
                }
            }

            bestVisionAlignmentForMvBlocker = rhsVisionAlignmentFromNpcChdPosition;
            outToHandleMvBlockerUd = udRhs;
            outToHandleMvBlockerBodyID = rhsBodyID;

            if (currIsFlying) {
                bool rhsOnTop = (rhsAABBAntiGAlignmentMax > selfAABBAntiGAlignmentMax);
                
                if (strictlyUp || strictlyDown) {
                    if (!compositingGap) {
                        outCurrGapToJump.set_vision_alignment(0); // To avoid unexpected "hasEffectiveMvBlocker" 
                    }
                    outCurrGapToJump.set_anti_gravity_alignment(rhsOnTop ? (rhsAABBAntiGAlignmentMin - selfAABBAntiGAlignmentMax) : (selfAABBAntiGAlignmentMin - rhsAABBAntiGAlignmentMax));
                    if (!compositingGap) {
                        outMinGapToJump.set_vision_alignment(outCurrGapToJump.vision_alignment());
                    }
                    outMinGapToJump.set_anti_gravity_alignment(strictlyUp ? +1.0f : -1.0f);
                } else if (strictlyRight || strictlyLeft) {
                    outCurrGapToJump.set_vision_alignment(rhsAABBVisionAlignmentMin - (selfAABBVisionAlignmentMax + cCollisionTolerance));
                    if (!compositingGap) {
                        outCurrGapToJump.set_anti_gravity_alignment(0); // To avoid unexpected "hasEffectiveMvBlocker" 
                    }

                    outMinGapToJump.set_vision_alignment(strictlyRight ? +1.0f : -1.0f);
                    if (!compositingGap) {
                        outMinGapToJump.set_anti_gravity_alignment(outCurrGapToJump.anti_gravity_alignment());
                    }
                } else {
                    outCurrGapToJump.set_vision_alignment(rhsAABBVisionAlignmentMin - (selfAABBVisionAlignmentMax + cCollisionTolerance));
                    outCurrGapToJump.set_anti_gravity_alignment(rhsOnTop ? (rhsAABBAntiGAlignmentMin - selfAABBAntiGAlignmentMax) : (selfAABBAntiGAlignmentMin - rhsAABBAntiGAlignmentMax));

                    outMinGapToJump.set_vision_alignment(outCurrGapToJump.vision_alignment());
                    outMinGapToJump.set_anti_gravity_alignment(outCurrGapToJump.anti_gravity_alignment());
                }

#ifndef  NDEBUG
                /*
                if (globalPrimitiveConsts->ch_species().bat1() == currChd.species_id() && InAirIdle1NoJump != currChd.ch_state()) {
                    std::ostringstream oss;
                    oss << "@currRdfId=" << currRdfId << ", (selfUd=" << selfUd << ", udRhs=" << udRhs << ", compositingGap=" << compositingGap << "), pos=(" << currChd.x() << ", " << currChd.y() << "), vel=(" << currChd.vel_x() << ", " << currChd.vel_y() << "), setting outCurrGapToJump=(" << outCurrGapToJump.vision_alignment() << ", " << outCurrGapToJump.anti_gravity_alignment() << ") by (strictlyUp=" << strictlyUp << ", strictlyDown=" << strictlyDown << ", strictlyRight=" << strictlyRight << ", strictlyLeft=" << strictlyLeft << ", holdableBothForwardAndBackward=" << holdableBothForwardAndBackward << ", holdableBothUpwardAndDownward=" << holdableBothUpwardAndDownward << "), (selfAABBAntiGAlignmentMin=" << selfAABBAntiGAlignmentMin << ", selfAABBAntiGAlignmentMax=" << selfAABBAntiGAlignmentMax << "), (rhsAABBAntiGAlignmentMin=" << rhsAABBAntiGAlignmentMin << ", rhsAABBAntiGAlignmentMax=" << rhsAABBAntiGAlignmentMax << "), (selfAABBMvIntentionAlignmentMin=" << selfAABBMvIntentionAlignmentMin << ", selfAABBMvIntentionAlignmentMax=" << selfAABBMvIntentionAlignmentMax << "), (rhsAABBMvIntentionAlignmentMin=" << rhsAABBMvIntentionAlignmentMin << ", rhsAABBMvIntentionAlignmentMax=" << rhsAABBMvIntentionAlignmentMax << "), mvIntentionNorm=(" << mvIntentionNorm.GetX() << ", " << mvIntentionNorm.GetY() << "), (selfAABBVisionAlignmentMin=" << selfAABBVisionAlignmentMin << ", selfAABBVisionAlignmentMax=" << selfAABBVisionAlignmentMax << "), (rhsAABBVisionAlignmentMin=" << rhsAABBVisionAlignmentMin << ", rhsAABBVisionAlignmentMax=" << rhsAABBVisionAlignmentMax << "), visionDirection=(" << visionDirection.GetX() << ", " << visionDirection.GetY() << "), currChS=" << currChd.ch_state() << ", currFc=" << currChd.frames_in_ch_state() << ", cvSupported=" << cvSupported << ", groundBodyUd=" << currChd.ground_ud() << ", bestVisionAlignmentForMvBlocker=" << bestVisionAlignmentForMvBlocker << "." << std::endl;
                    Debug::Log(oss.str(), DColor::Orange);
                }
                */
#endif // ! NDEBUG
            } else {
                // Calc "outCurrGapToJump", if "true == currIsFlying", these "xxxGapToJump" variables still provide useful information about the gaps.
                outCurrGapToJump.set_vision_alignment(rhsAABBVisionAlignmentMin - selfAABBVisionAlignmentMax);
                outCurrGapToJump.set_anti_gravity_alignment(rhsAABBMvIntentionAlignmentMax - selfAABBMvIntentionAlignmentMin);

                if (!groundBodyID.IsInvalid()) {
                    // Calc "minGapToJump"
                    outMinGapToJump.set_vision_alignment(rhsAABBVisionAlignmentMin - groundAABBVisionAlignmentMax);
                    outMinGapToJump.set_anti_gravity_alignment(rhsAABBMvIntentionAlignmentMax - groundAABBMvIntentionAlignmentMax);
                }
            } 

            break;
        }
        default:
            break;
        }
    }

    // In case there's some unintentional drawing overlap.
    if (FLT_MAX != outCurrGroundMvTolerance.vision_alignment() && outCurrGroundMvTolerance.vision_alignment() > outCurrGapToJump.vision_alignment()) {
        outCurrGroundMvTolerance.set_vision_alignment(outCurrGapToJump.vision_alignment());
    }
}

int BaseNpcReaction::deriveNpcVisionReactionAgainstOppoChUd(int currRdfId, const BaseBattleCollisionFilter* baseBattleFilter, const CH_COLLIDER_T* selfCollider, const BodyID& selfBodyID, const uint64_t selfUd, const CharacterDownsync& currChd, const MassProperties& massProps, const Vec3& currChdFacing, const CharacterConfig* cc, CharacterDownsync* nextChd, const bool cvSupported, const bool cvInAir, const bool cvOnWall, const bool currNotDashing, const bool currEffInAir, const bool currIsFlying, const bool oldNextNotDashing, const bool oldNextEffInAir, const bool inJumpStartupOrJustEnded, CharacterBase::EGroundState cvGroundState, const bool canJumpWithinInertia, const Vec3& visionDirection, const uint64_t toHandleOppoChUd, const Vec3& selfPositionDiffForOppoChUd, bool& outOpponentBehindMe, bool& outOpponentAboveMe, bool& outOpponentIsAttacking, bool& outOpponentIsFacingMe) {
    int ret = TARGET_CH_REACTION_UNCHANGED;
    const CharacterDownsync* rhsCurrChd = baseBattleFilter->immutableCurrChdPtrFromUd(toHandleOppoChUd);
    if (nullptr == rhsCurrChd) {
        return ret;
    }

    outOpponentBehindMe = (0 > (selfPositionDiffForOppoChUd.GetX() * visionDirection.GetX()));
    outOpponentAboveMe = cc->capsule_half_height() < selfPositionDiffForOppoChUd.GetY();
   

    if (!outOpponentBehindMe) {
        // Opponent is in front of me
        ret = TARGET_CH_REACTION_FOLLOW;
        // TODO: Use skill?
    } else {
        // Opponent is behind me
        if (0 >= cc->speed()) {
            // e.g. Tower
        } else {
            outOpponentIsAttacking = !nonAttackingSet.count(rhsCurrChd->ch_state());
            Quat oppoChdQ ;
            Vec3 oppoFacing; 
            BaseBattleCollisionFilter::calcChdFacing(*rhsCurrChd, oppoChdQ, oppoFacing);
            outOpponentIsFacingMe = (0 > selfPositionDiffForOppoChUd.GetX() * oppoFacing.GetX());
            ret = TARGET_CH_REACTION_FOLLOW;
        }
    }

    return ret;
}

int BaseNpcReaction::deriveReactionAgainstGroundAndMvBlocker(int currRdfId, const Vec3& mvIntentionNorm, const Vec3& antiGravityNorm, const float gravityMagnitude, const BodyInterface* biNoLock, const CH_COLLIDER_T* selfCollider, const AABox* selfAABB, const BodyID& selfBodyID, const uint64_t selfUd, const NpcGoal inNpcGoal, const CharacterDownsync& currChd, const MassProperties& massProps, const Vec3& currChdFacing, const CharacterConfig* cc, CharacterDownsync* nextChd, const bool cvSupported, const bool cvInAir, const bool cvOnWall, const bool currNotDashing, const bool currEffInAir, const bool currIsFlying, const bool oldNextNotDashing, const bool oldNextEffInAir, const bool inJumpStartupOrJustEnded, CharacterBase::EGroundState cvGroundState, const bool canJumpWithinInertia, const AABox& visionAABB, const Vec3Arg& visionNarrowPhaseInBaseOffset, const Vec3& visionDirection, const BodyID& toHandleMvBlockerBodyID, const uint64_t toHandleMvBlockerUd, const GapToJump& currGapToJump, const GapToJump& minGapToJump, const GapToJump& currGroundMvTolerance, const int visionReactionByFar, const uint64_t toHandleOppoChUd, const Vec3& selfPositionDiffForOppoChUd, const bool opponentBehindMe, const bool opponentAboveMe, const bool opponentIsAttacking, const bool opponentIsFacingMe, const bool temptingToMove, const bool inFleeingGracePeriod) {
    
    if (NpcGoal::NIdle == inNpcGoal || NpcGoal::NIdleIfGoHuntingThenPatrol == inNpcGoal || NpcGoal::NIdleIfGoHuntingThenPathPatrol == inNpcGoal) {
        return visionReactionByFar;
    }

#ifndef NDEBUG
    /*
    if (globalPrimitiveConsts->ch_species().bat1() == nextChd->species_id() && InAirIdle1NoJump != nextChd->ch_state()) {
        std::ostringstream oss;
        oss << "@currRdfId=" << currRdfId << ", flying selfUd=" << selfUd << " calling deriveReactionAgainstGroundAndMvBlocker, pos=(" << currChd.x() << "," << currChd.y() << "), visionDirection=(" << visionDirection.GetX() << ", " << visionDirection.GetY() << "), temptingToMove=" << temptingToMove << ", inNpcGoal=" << inNpcGoal << ", currChState=" << currChd.ch_state() << ", currFc=" << currChd.frames_in_ch_state() << ", currGapToJump=(" << currGapToJump.vision_alignment() << ", " << currGapToJump.anti_gravity_alignment() << "), nextVel=(" << nextChd->vel_x() << "," << nextChd->vel_y() << "), toHandleMvBlockerUd=" << toHandleMvBlockerUd << ", locking_on_ud=" << currChd.locking_on_ud();
        Debug::Log(oss.str(), DColor::White);
    }
    */
#endif

    int newVisionReaction = visionReactionByFar;

    /*
    [WARNING] DON'T use "selfCollider->GetLinearVelocity()" to evaluate "currGroundCanHoldMeIfWalkOn". 

    When a jumping character touches the vertical-side-edge of a higher platform, its velocity might be calculated by the ContactManager to an opposite direction than the vision direction.  
    */
    const float potentialMv = cc->speed()*globalPrimitiveConsts->estimated_seconds_per_rdf();

    bool currGroundCanHoldMeIfWalkOn = (currGroundMvTolerance.vision_alignment() >= (potentialMv + 0.5*cc->capsule_radius()));
    bool toHandleMvBlockerCanHoldMeIfWalkOn = false;
    const Vec3& chColliderVel = selfCollider->GetLinearVelocity(false);
    const float constraintVelXDiff = chColliderVel.GetX() - nextChd->vel_x();
    const float constraintVelYDiff = chColliderVel.GetY() - nextChd->vel_y();
    bool hasEffectiveMvBlocker = false;

    if (!currIsFlying) {
        hasEffectiveMvBlocker = (walkingSet.count(currChd.ch_state()) || temptingToMove) &&
            (
            (0 > constraintVelXDiff * nextChd->vel_x()) && !BaseBattleCollisionFilter::IsLengthNearZero(constraintVelXDiff * globalPrimitiveConsts->estimated_seconds_per_rdf())
            ||
            (0 > currGapToJump.vision_alignment())
        );
    } else {
        hasEffectiveMvBlocker = (walkingSet.count(currChd.ch_state()) || temptingToMove) &&
            (
                (0 > constraintVelXDiff * nextChd->vel_x()) && !BaseBattleCollisionFilter::IsLengthNearZero(constraintVelXDiff * globalPrimitiveConsts->estimated_seconds_per_rdf())
                ||
                (0 > currGapToJump.vision_alignment())
            )
            || 
            (
                (0 > constraintVelYDiff * nextChd->vel_y()) && !BaseBattleCollisionFilter::IsLengthNearZero(constraintVelYDiff * globalPrimitiveConsts->estimated_seconds_per_rdf())
                ||
                (0 > currGapToJump.anti_gravity_alignment())
            );
#ifndef NDEBUG
            /*
            if (globalPrimitiveConsts->ch_species().bat1() == nextChd->species_id() && InAirIdle1NoJump != nextChd->ch_state()) {
                if (!hasEffectiveMvBlocker && 0 == currChd.locking_on_ud()) {
                    std::ostringstream oss;
                    oss << "@currRdfId=" << currRdfId << ", flying selfUd=" << selfUd << " is not hunting and lost effective movement blocker, pos=(" << currChd.x() << "," << currChd.y() << "), visionDirection=(" << visionDirection.GetX() << ", " << visionDirection.GetY() << "), temptingToMove=" << temptingToMove << ", inNpcGoal=" << inNpcGoal << ", currChState=" << currChd.ch_state() << ", currFc=" << currChd.frames_in_ch_state() << ", currGapToJump=(" << currGapToJump.vision_alignment() << ", " << currGapToJump.anti_gravity_alignment() << "), nextVel=(" << nextChd->vel_x() << "," << nextChd->vel_y() << "), toHandleMvBlockerUd=" << toHandleMvBlockerUd;
                    Debug::Log(oss.str(), DColor::White);
                }
            }
            */
#endif
    }

    if (cvSupported) {
        if (!currGroundCanHoldMeIfWalkOn || hasEffectiveMvBlocker) {
            /*
            [WARNING] Don't IMMEDIATELY return the "newVisionReaction" if "0 != toHandleMvBlockerUd", there might be still chance to jump onto a horizontally forward holding platform.
            */
            if (temptingToMove) {
                newVisionReaction = TARGET_CH_REACTION_TURNAROUND_MV_BLOCKER;
            } else {
                newVisionReaction = TARGET_CH_REACTION_STOP_BY_MV_BLOCKER;
            }
        } else {
            if (temptingToMove) {
                newVisionReaction = TARGET_CH_REACTION_WALK_ALONG;
            } else {
                newVisionReaction = TARGET_CH_REACTION_STOP_BY_MV_BLOCKER;
            }
        }
    } else {
        if (currIsFlying) {
            if (hasEffectiveMvBlocker) {
                if (temptingToMove) {
#ifndef NDEBUG
                    /*
                    if (globalPrimitiveConsts->ch_species().bat1() == nextChd->species_id()) {
                        std::ostringstream oss;
                        oss << "@currRdfId=" << currRdfId << ", flying selfUd=" << selfUd << " got new reaction TURNAROUND_MV_BLOCKER due to hasEffectiveMvBlocker and temptingToMove, currChS=" << currChd.ch_state() << ", currNpcGoal=" << inNpcGoal << ", vel=(" << nextChd->vel_x() << ", " << nextChd->vel_y() << "), constraintVelDiff=(" << constraintVelXDiff << ", " << constraintVelYDiff << ")";
                        Debug::Log(oss.str(), DColor::White);
                    }
                    */
#endif
                    newVisionReaction = TARGET_CH_REACTION_TURNAROUND_MV_BLOCKER;
                } else {
#ifndef NDEBUG
                    /*
                    if (globalPrimitiveConsts->ch_species().bat1() == nextChd->species_id()) {
                        std::ostringstream oss;
                        oss << "@currRdfId=" << currRdfId << ", flying selfUd=" << selfUd << " got new reaction STOP_BY_MV_BLOCKER due to hasEffectiveMvBlocker but not temptingToMove, currChS=" << currChd.ch_state() << ", currNpcGoal=" << inNpcGoal << ", vel=(" << nextChd->vel_x() << ", " << nextChd->vel_y() << "), constraintVelDiff=(" << constraintVelXDiff << ", " << constraintVelYDiff << ")";
                        Debug::Log(oss.str(), DColor::White);
                    }
                    */
#endif
                    newVisionReaction = TARGET_CH_REACTION_STOP_BY_MV_BLOCKER;
                }
            } else {
                if (temptingToMove) {
#ifndef NDEBUG
                    /*
                    if (globalPrimitiveConsts->ch_species().bat1() == nextChd->species_id()) {
                        std::ostringstream oss;
                        oss << "@currRdfId=" << currRdfId << ", flying selfUd=" << selfUd << " got new reaction WALK_ALONG due to no effectiveMvBlocker and temptingToMove, currChS=" << currChd.ch_state() << ", currNpcGoal=" << inNpcGoal << ", vel=(" << nextChd->vel_x() << ", " << nextChd->vel_y() << "), constraintVelDiff=(" << constraintVelXDiff << ", " << constraintVelYDiff << ")";
                        Debug::Log(oss.str(), DColor::White);
                    }
                    */
#endif
                    newVisionReaction = TARGET_CH_REACTION_WALK_ALONG;
                } else {
#ifndef NDEBUG
                    /*
                    if (globalPrimitiveConsts->ch_species().bat1() == nextChd->species_id()) {
                        std::ostringstream oss;
                        oss << "@currRdfId=" << currRdfId << ", flying selfUd=" << selfUd << " got new reaction STOP_BY_MV_BLOCKER due to no effectiveMvBlocker but not temptingToMove, currChS=" << currChd.ch_state() << ", currNpcGoal=" << inNpcGoal;
                        Debug::Log(oss.str(), DColor::White);
                    }
                    */
#endif
                    newVisionReaction = TARGET_CH_REACTION_STOP_BY_MV_BLOCKER;
                }
            }
        } else {
            if (temptingToMove) {
                newVisionReaction = TARGET_CH_REACTION_WALK_ALONG;
            } else {
                newVisionReaction = TARGET_CH_REACTION_STOP_BY_MV_BLOCKER;
            }
        }
    }

    if (0 == toHandleMvBlockerUd) {
/*
#ifndef NDEBUG
        if (TARGET_CH_REACTION_UNCHANGED != newVisionReaction) {
            if (TARGET_CH_REACTION_WALK_ALONG != newVisionReaction || (currRdfId % 16 == 0)) {
                std::ostringstream oss;
                oss << "@currRdfId=" << currRdfId << ", selfUd=" << selfUd << " has visionAABB=(minX=" << visionAABB.mMin.GetX() << ", maxX=" << visionAABB.mMax.GetX() << ", minY=" << visionAABB.mMin.GetY() << ", maxY=" << visionAABB.mMax.GetY() << "), curr_ch_state=" << currChd.ch_state() << ", curr_frames_in_ch_state=" << currChd.frames_in_ch_state() << ", cvSupported=" << cvSupported << ", groundBodyID=" << selfCollider->GetGroundBodyID().GetIndexAndSequenceNumber() << ", there's no toHandleMvBlockerBodyID, canJumpWithinInertia=" << canJumpWithinInertia << ", hasEffectiveMvBlocker=" << hasEffectiveMvBlocker << ", currGroundMvTolerance=" << currGroundMvTolerance.vision_alignment() << ", returning newVisionReaction = " << newVisionReaction;
                Debug::Log(oss.str(), DColor::Orange);
            } 
        }
#endif
*/
        return newVisionReaction;
    }

    if (currIsFlying) {
        return newVisionReaction;
    }

    bool isMinGapJumpable = false, isCurrGapJumpable = false;
    float currGapEstimatedSpeedX = BaseBattleCollisionFilter::IsLengthNearZero(currChd.vel_x()) ? 0.7f * cc->speed() : 0.8f * std::abs(currChd.vel_x());
    float minGapEstimatedSpeedX = 0.8f * cc->speed();
    float currGapToJumpVisionAlignment = currGapToJump.vision_alignment(), currGapToJumpAntiGravityAlignment = currGapToJump.anti_gravity_alignment();

    if (FLT_MAX != currGapToJumpVisionAlignment && cvSupported) {
        /*
        [TODO] Handle the following 2 cases.
        - "groundBodyID" being a slope that I can just walk along.
        - "toHandleMvBlockerBodyID" being a slope that I can just walk onto.
        */

        /*
        When a character jumps at (x=0, y=0) with "forwardSpeed (in the re-aligned x-axis)" and "chJumpInitSpeed" (in the re-aligned y-axis), the trajectory (in ISO units) is
        - x(t) = forwardSpeed*t
        - y(t) = chJumpInitSpeed*t - 0.5*gravityMagnitude*t where "gravityMagnitude > 0"
        */
        const float jumpAccMagY = cc->jump_acc_mag_y();
        const int jumpStartupFrames = cc->jump_startup_frames();
        const float chJumpAccSeconds = ((jumpStartupFrames + 1) * globalPrimitiveConsts->estimated_seconds_per_rdf());
        const float chJumpInitSpeed = jumpAccMagY * chJumpAccSeconds;
        const float extraAccendingY = ((chJumpInitSpeed * 0.5f) * chJumpAccSeconds);
        isMinGapJumpable = isGapJumpable(gravityMagnitude, minGapToJump.vision_alignment() + cc->capsule_radius(), minGapToJump.anti_gravity_alignment(), minGapEstimatedSpeedX, chJumpAccSeconds, chJumpInitSpeed, extraAccendingY);
        isCurrGapJumpable = isGapJumpable(gravityMagnitude, currGapToJumpVisionAlignment + cc->capsule_radius(), currGapToJumpAntiGravityAlignment, currGapEstimatedSpeedX, chJumpAccSeconds, chJumpInitSpeed, extraAccendingY);
        toHandleMvBlockerCanHoldMeIfWalkOn = isCurrGapJumpable && (0 >= currGapToJumpVisionAlignment && 0 >= currGapToJumpAntiGravityAlignment);
    }

    newVisionReaction = deriveReactionAgainstMvBlockerAfterApproximation(currRdfId, mvIntentionNorm, antiGravityNorm, selfUd, currChd, massProps, currChdFacing, cvSupported, canJumpWithinInertia, isMinGapJumpable, isCurrGapJumpable, currGroundCanHoldMeIfWalkOn, toHandleMvBlockerCanHoldMeIfWalkOn, currGapToJumpVisionAlignment, temptingToMove, inFleeingGracePeriod, newVisionReaction);
/*
#ifndef NDEBUG
    if (selfUd == 8589934593 && TARGET_CH_REACTION_UNCHANGED != newVisionReaction) {
        if (TARGET_CH_REACTION_WALK_ALONG != newVisionReaction || (currRdfId % 16 == 0)) {
            std::ostringstream oss;
            oss << "@currRdfId=" << currRdfId << ", selfUd=" << selfUd << " has visionDir=(" << visionDirection.GetX() << ", " << visionDirection.GetY() << "), visionAABB=(minX=" << visionAABB.mMin.GetX() << ", maxX=" << visionAABB.mMax.GetX() << ", minY=" << visionAABB.mMin.GetY() << ", maxY=" << visionAABB.mMax.GetY() << "), curr_ch_state=" << currChd.ch_state() << ", curr_frames_in_ch_state=" << currChd.frames_in_ch_state() << ", cvSupported=" << cvSupported << ", groundBodyID=" << selfCollider->GetGroundBodyID().GetIndexAndSequenceNumber() << ", toHandleMvBlockerBodyID=" << toHandleMvBlockerBodyID.GetIndexAndSequenceNumber() << ", canJumpWithinInertia=" << canJumpWithinInertia << ", isMinGapJumpable=" << isMinGapJumpable << ", isCurrGapJumpable=" << isCurrGapJumpable << ", currGroundCanHoldMeIfWalkOn=" << currGroundCanHoldMeIfWalkOn << ", toHandleMvBlockerCanHoldMeIfWalkOn=" << toHandleMvBlockerCanHoldMeIfWalkOn << ", currGapToJumpVisionAlignment=" << currGapToJumpVisionAlignment << ", temptingToMove=" << temptingToMove << "; currGroundMvTolerance=" << currGroundMvTolerance.vision_alignment() << ", minGapToJump=(" << minGapToJump.vision_alignment() << ", " << minGapToJump.anti_gravity_alignment() << "), currGapToJump=(" <<  currGapToJump.vision_alignment() << ", " << currGapToJump.anti_gravity_alignment() << "); returning newVisionReaction = " << newVisionReaction;
            Debug::Log(oss.str(), DColor::Orange);
        } 
    }
#endif
*/
    return newVisionReaction;
}

int BaseNpcReaction::deriveReactionAgainstMvBlockerAfterApproximation(int currRdfId, const Vec3& mvIntentionNorm, const Vec3& antiGravityNorm, const uint64_t selfUd, const CharacterDownsync& currChd, const MassProperties& massProps, const Vec3& currChdFacing, const bool cvSupported, const bool canJumpWithinInertia, const bool isMinGapJumpable, const bool isCurrGapJumpable, const bool currGroundCanHoldMeIfWalkOn, const bool toHandleMvBlockerCanHoldMeIfWalkOn, const float currGapToJumpVisionAlignment, const bool temptingToMove, const bool inFleeingGracePeriod, const int visionReactionByFar) {
    int newVisionReaction = visionReactionByFar;
    if (!cvSupported) {
        if (temptingToMove) {
            newVisionReaction = TARGET_CH_REACTION_WALK_ALONG;
        } else {
            newVisionReaction = TARGET_CH_REACTION_UNCHANGED;
        }
    } else if (canJumpWithinInertia && isCurrGapJumpable) {
        if (temptingToMove) {
            if (toHandleMvBlockerCanHoldMeIfWalkOn) {
                newVisionReaction = TARGET_CH_REACTION_WALK_ALONG;
            } else {
                newVisionReaction = TARGET_CH_REACTION_JUMP_TOWARDS_MV_BLOCKER;
            }
        } else {
            newVisionReaction = TARGET_CH_REACTION_STOP_BY_MV_BLOCKER;
        }
    } else {
        if (temptingToMove) {
            if (currGroundCanHoldMeIfWalkOn) {
                if (TURNAROUND_FROM_MV_BLOCKER_DX_THRESHOLD < currGapToJumpVisionAlignment) {
                    newVisionReaction = TARGET_CH_REACTION_WALK_ALONG;
                } else {
                    if (isMinGapJumpable) {
                        newVisionReaction = TARGET_CH_REACTION_WALK_ALONG;
                    } else {
                        if (inFleeingGracePeriod) {
                            newVisionReaction = TARGET_CH_REACTION_STOP_BY_MV_BLOCKER;
                        } else {
                            newVisionReaction = TARGET_CH_REACTION_TURNAROUND_MV_BLOCKER;
                        }
                        
                    }
                }
            } else {
                // There's no need to go any further for jumping
                if (inFleeingGracePeriod) {
                    newVisionReaction = TARGET_CH_REACTION_STOP_BY_MV_BLOCKER;
                } else {
                    newVisionReaction = TARGET_CH_REACTION_TURNAROUND_MV_BLOCKER;
                }
            }
        } else {
            newVisionReaction = TARGET_CH_REACTION_STOP_BY_MV_BLOCKER;
        }
    }

    return newVisionReaction;
}

bool BaseNpcReaction::isGapJumpable(const float gravityMagnitude, const float forwardDistanceAbs, const float AntiGDistance, const float forwardSpeed, const float chJumpAccSeconds, const float chJumpInitSpeed, const float extraAccendingY) {
    
    if (0 >= forwardSpeed) return false;
    if (0 >= forwardDistanceAbs) {
        // Only need evaluate if we can jump vertically first and then slowly move over onto the new platform.
        float airingTimeSingleTrip = (chJumpInitSpeed / gravityMagnitude);
        float estimatedYHighestInTrajectory = extraAccendingY + 0.5f*chJumpInitSpeed*airingTimeSingleTrip;
        return estimatedYHighestInTrajectory > AntiGDistance;
    }
    float estimatedTSeconds = forwardDistanceAbs / forwardSpeed;

    float estimatedYInTrajectory = extraAccendingY + chJumpInitSpeed * estimatedTSeconds - 0.5f * gravityMagnitude * estimatedTSeconds * estimatedTSeconds;
    return estimatedYInTrajectory > AntiGDistance;
}
