using Google.Protobuf.Collections;
using jtshared;
using static JoltCSharp.PbPrimitives;

namespace JoltCSharp {
    public class PbSkills {
        public static int EncodePatternForCancelTransit(int patternId, bool currEffInAir, bool currCrouching, bool currOnWall, bool currDashing, bool currWalking) {
            /*
            For simplicity,
            - "currSliding" = "currCrouching" + "currDashing"
            */
            int encodedPatternId = patternId;
            if (currEffInAir) {
                encodedPatternId += (1 << 16);
            }
            if (currCrouching) {
                encodedPatternId += (1 << 17);
            }
            if (currOnWall) {
                encodedPatternId += (1 << 18);
            }
            if (currDashing) {
                encodedPatternId += (1 << 19);
            }
            if (currWalking) {
                encodedPatternId += (1 << 20);
            }
            return encodedPatternId;
        }

        public static int EncodePatternForInitSkill(int patternId, bool currEffInAir, bool currCrouching, bool currOnWall, bool currDashing, bool currWalking, bool currInBlockStun, bool currAtked, bool currParalyzed) {
            int encodedPatternId = EncodePatternForCancelTransit(patternId, currEffInAir, currCrouching, currOnWall, currDashing, currWalking);
            if (currInBlockStun) {
                encodedPatternId += (1 << 21);
            }
            if (currAtked) {
                encodedPatternId += (1 << 22);
            }
            if (currParalyzed) {
                encodedPatternId += (1 << 23);
            }
            return encodedPatternId;
        }

        private static BulletConfig BasicPistolBulletAir = new BulletConfig {
            StartupFrames = 7,
            ActiveFrames = 360,
            HitStunFrames = 7,
            BlockStunFrames = 7,
            Damage = 10,
            PushbackVelX = 0.8f,
            PushbackVelY = PbPrimitives.underlying.NoLockVel,
            SelfLockVelX = PbPrimitives.underlying.NoLockVel,
            SelfLockVelY = PbPrimitives.underlying.NoLockVel,
            SelfLockVelYWhenFlying = PbPrimitives.underlying.NoLockVel,
            HitboxOffsetX = 12f,
            HitboxOffsetY = 4f,
            HitboxHalfSizeX = 2,
            HitboxHalfSizeY = 2,
            SpeciesId = 1,
            Speed = 7.0f * BATTLE_DYNAMICS_FPS,
            Hardness = 4,
            ExplosionFrames = 25,
            BType = BulletType.MechanicalCartridge,
            CharacterEmitSfxName = "PistolEmit",
            ActiveVfxSpeciesId = 0, // TODO
            IsPixelatedActiveVfx = true,
            ExplosionSfxName = "Piercing",
            ExplosionOnRockSfxName = "Explosion8",
            CancellableStFrame = 11,
            CancellableEdFrame = 19,
            CollisionTypeMask = 0, // TODO
            AnimLoopingRdfOffset = 10,
        };

        private static BulletConfig BasicPistolBulletGround = new BulletConfig(BasicPistolBulletAir)
                                                                .SetHitboxOffsets(14f, 10f)
                                                                .SetAllowsWalking(true)
                                                                .SetAllowsCrouching(true);

        private static BulletConfig BasicPistolBulletCrouch = new BulletConfig(BasicPistolBulletAir)
                                                                .SetHitboxOffsets(12f, 10f)
                                                                .SetSelfLockVel(0, 0, 0);

        private static BulletConfig BasicPistolBulletWalking = new BulletConfig(BasicPistolBulletAir)
                                                                .SetHitboxOffsets(14f, 10f)
                                                                .SetAllowsWalking(true)
                                                                .SetAllowsCrouching(true);

        public static Skill HunterPistolWall = new Skill {
            Id = HunterPistolWallId,
            RecoveryFrames = 18,
            RecoveryFramesOnBlock = 18,
            RecoveryFramesOnHit = 18,
            TriggerType = SkillTriggerType.RisingEdge,
            BoundChState = CharacterState.OnWallAtk1
        }.AddHit(new BulletConfig(BasicPistolBulletAir)
            .UpsertCancelTransit(EncodePatternForCancelTransit(PbPrimitives.underlying.PatternUpB, true, false, true, false, false), HunterAirSlashId)
        );

        public static Skill HunterPistol = new Skill {
            Id = HunterPistolId,
            RecoveryFrames = HunterPistolWall.RecoveryFrames,
            RecoveryFramesOnBlock = HunterPistolWall.RecoveryFramesOnBlock,
            RecoveryFramesOnHit = HunterPistolWall.RecoveryFramesOnHit,
            TriggerType = SkillTriggerType.RisingEdge,
            BoundChState = CharacterState.Atk1
        }
        .AddHit(new BulletConfig(BasicPistolBulletGround)
            .UpsertCancelTransit(EncodePatternForCancelTransit(PbPrimitives.underlying.PatternUpB, false, false, false, false, false), HunterDragonPunchId)
            .UpsertCancelTransit(EncodePatternForCancelTransit(PbPrimitives.underlying.PatternUpB, false, false, false, false, true), HunterDragonPunchId)
            .UpsertCancelTransit(EncodePatternForCancelTransit(PbPrimitives.underlying.PatternDownB, false, false, false, false, false), HunterPistolCrouchId)
            .UpsertCancelTransit(EncodePatternForCancelTransit(PbPrimitives.underlying.PatternDownB, false, false, false, false, true), HunterPistolCrouchId)
        );

        public static Skill HunterPistolAir = new Skill {
            Id = HunterPistolAirId,
            RecoveryFrames = HunterPistolWall.RecoveryFrames,
            RecoveryFramesOnBlock = HunterPistolWall.RecoveryFramesOnBlock,
            RecoveryFramesOnHit = HunterPistolWall.RecoveryFramesOnHit,
            TriggerType = SkillTriggerType.RisingEdge,
            BoundChState = CharacterState.InAirAtk1
        }
        .AddHit(new BulletConfig(BasicPistolBulletAir)
            .UpsertCancelTransit(EncodePatternForCancelTransit(PbPrimitives.underlying.PatternUpB, true, false, false, false, false), HunterAirSlashId)
        );

        public static Skill HunterPistolWalking = new Skill {
            Id = HunterPistolWalkingId,
            RecoveryFrames = HunterPistolWall.RecoveryFrames,
            RecoveryFramesOnBlock = HunterPistolWall.RecoveryFramesOnBlock,
            RecoveryFramesOnHit = HunterPistolWall.RecoveryFramesOnHit,
            TriggerType = SkillTriggerType.RisingEdge,
            BoundChState = CharacterState.WalkingAtk1
        }
        .AddHit(new BulletConfig(BasicPistolBulletGround)
            .UpsertCancelTransit(EncodePatternForCancelTransit(PbPrimitives.underlying.PatternUpB, false, false, false, false, false), HunterDragonPunchId)
            .UpsertCancelTransit(EncodePatternForCancelTransit(PbPrimitives.underlying.PatternUpB, false, false, false, false, true), HunterDragonPunchId)
            .UpsertCancelTransit(EncodePatternForCancelTransit(PbPrimitives.underlying.PatternDownB, false, false, false, false, false), HunterPistolCrouchId)
            .UpsertCancelTransit(EncodePatternForCancelTransit(PbPrimitives.underlying.PatternDownB, false, false, false, false, true), HunterPistolCrouchId)
        );

        public static Skill HunterPistolCrouch = new Skill {
            Id = HunterPistolCrouchId,
            RecoveryFrames = HunterPistolWall.RecoveryFrames,
            RecoveryFramesOnBlock = HunterPistolWall.RecoveryFramesOnBlock,
            RecoveryFramesOnHit = HunterPistolWall.RecoveryFramesOnHit,
            TriggerType = SkillTriggerType.RisingEdge,
            BoundChState = CharacterState.CrouchAtk1
        }
        .AddHit(new BulletConfig(BasicPistolBulletCrouch)
            .UpsertCancelTransit(EncodePatternForCancelTransit(PbPrimitives.underlying.PatternUpB, false, true, false, false, false), HunterDragonPunchId)
        );


        public static MapField<uint, Skill> underlying = new MapField<uint, Skill>() { };

        static PbSkills() {
            underlying.Add(HunterPistolWallId, HunterPistolWall);
            underlying.Add(HunterPistolId, HunterPistol);
            underlying.Add(HunterPistolAirId, HunterPistolAir);
            underlying.Add(HunterPistolWalkingId, HunterPistolWalking);
            underlying.Add(HunterPistolCrouchId, HunterPistolCrouch);
        }
    }
}
