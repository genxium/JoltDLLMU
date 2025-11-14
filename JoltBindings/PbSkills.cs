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
            CooldownFrames = 11,
            Damage = 10,
            PushbackVelX = PbPrimitives.underlying.NoLockVel,
            PushbackVelY = PbPrimitives.underlying.NoLockVel,
            SelfLockVelX = PbPrimitives.underlying.NoLockVel,
            SelfLockVelY = PbPrimitives.underlying.NoLockVel,
            SelfLockVelYWhenFlying = PbPrimitives.underlying.NoLockVel,
            HitboxOffsetX = 11f,
            HitboxOffsetY = 20f,
            HitboxHalfSizeX = 2,
            HitboxHalfSizeY = 2,
            SpeciesId = 1,
            ExplosionSpeciesId = 1,
            Speed = 6.0f * BATTLE_DYNAMICS_FPS,
            Hardness = 4,
            ExplosionFrames = 25,
            // [WARNING] For "MechanicalCartridge", don't set "PushbackVelX" or "PushbackVelY", instead set density of the bullet and rely on the Physics engine to calculate impulse.
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
                                                                .SetHitboxOffsets(10f, 24f)
                                                                .SetAllowsWalking(true)
                                                                .SetAllowsCrouching(true);

        private static BulletConfig BasicPistolBulletCrouch = new BulletConfig(BasicPistolBulletAir)
                                                                .SetHitboxOffsets(11f, 20f)
                                                                .SetSelfLockVel(0, 0, 0);

        private static BulletConfig BasicPistolBulletWalking = new BulletConfig(BasicPistolBulletAir)
                                                                .SetHitboxOffsets(14f, 24f)
                                                                .SetAllowsWalking(true)
                                                                .SetAllowsCrouching(true);

        public static Skill HunterPistolWall = new Skill {
            Id = HunterPistolWallId,
            RecoveryFrames = BasicPistolBulletAir.StartupFrames+BasicPistolBulletAir.CooldownFrames,
            RecoveryFramesOnBlock = BasicPistolBulletAir.StartupFrames+BasicPistolBulletAir.CooldownFrames,
            RecoveryFramesOnHit = BasicPistolBulletAir.StartupFrames+BasicPistolBulletAir.CooldownFrames,
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
        .AddHit(new BulletConfig(BasicPistolBulletWalking)
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

        public static BulletConfig BasicBladeHit1 = new BulletConfig {
            StartupFrames = 5,
            StartupInvinsibleFrames = 3,
            ActiveFrames = 12,
            HitStunFrames = 18,
            BlockStunFrames = 8,
            CooldownFrames = 5,
            Damage = 10,
            PushbackVelX = 0.3f,
            PushbackVelY = PbPrimitives.underlying.NoLockVel,
            SelfLockVelX = PbPrimitives.underlying.NoLockVel,
            SelfLockVelY = PbPrimitives.underlying.NoLockVel,
            SelfLockVelYWhenFlying = PbPrimitives.underlying.NoLockVel,
            HitboxOffsetX = 10f,
            HitboxOffsetY = 17f,
            HitboxHalfSizeX = 18f,
            HitboxHalfSizeY = 18f,
            CancellableStFrame = 10,
            CancellableEdFrame = 21,
            SpeciesId = 2,
            ExplosionSpeciesId = 2,
            ExplosionFrames = 25,
            BType = BulletType.Melee,
            Hardness = 5,
            GuardBreakerExtraHitCnt = 1,
            ReflectFireballXIfNotHarder = true,
            CharacterEmitSfxName = "SlashEmitSpd1",
            ExplosionSfxName="Melee_Explosion2",
            RemainsUponHit = true,
            ActiveVfxSpeciesId = 0, // TODO
            CollisionTypeMask = 0, // TODO
        };

        public static BulletConfig BasicBladeHit2 = new BulletConfig {
            StartupFrames = 6,
            StartupInvinsibleFrames = 3,
            ActiveFrames = 12,
            HitStunFrames = 20,
            BlockStunFrames = 8,
            CooldownFrames = 6,
            Damage = 8,
            PushbackVelX = 0.5f,
            PushbackVelY = PbPrimitives.underlying.NoLockVel,
            SelfLockVelX = PbPrimitives.underlying.NoLockVel,
            SelfLockVelY = PbPrimitives.underlying.NoLockVel,
            SelfLockVelYWhenFlying = PbPrimitives.underlying.NoLockVel,
            HitboxOffsetX = 12f,
            HitboxOffsetY = 17f,
            HitboxHalfSizeX = 18f,
            HitboxHalfSizeY = 18f,
            CancellableStFrame = 11,
            CancellableEdFrame = 24,
            SpeciesId = 2,
            ExplosionSpeciesId = 2,
            ExplosionFrames = 25,
            BType = BulletType.Melee,
            Hardness = 6,
            GuardBreakerExtraHitCnt = 1,
            ReflectFireballXIfNotHarder = true,
            CharacterEmitSfxName = "SlashEmitSpd2",
            ExplosionSfxName="Melee_Explosion2",
            RemainsUponHit = true,
            ActiveVfxSpeciesId = 0, // TODO
            CollisionTypeMask = 0, // TODO
        };

        public static BulletConfig BasicBladeHit3 = new BulletConfig {
            StartupFrames = 15,
            StartupInvinsibleFrames = 10,
            ActiveFrames = 12,
            HitStunFrames = 40,
            BlockStunFrames = 8,
            CooldownFrames = 25,
            Damage = 15,
            PushbackVelX = 0.5f,
            PushbackVelY = -0.5f,
            SelfLockVelX = PbPrimitives.underlying.NoLockVel,
            SelfLockVelY = PbPrimitives.underlying.NoLockVel,
            SelfLockVelYWhenFlying = PbPrimitives.underlying.NoLockVel,
            HitboxOffsetX = 24f,
            HitboxOffsetY = 17f,
            HitboxHalfSizeX = 22f,
            HitboxHalfSizeY = 18f,
            SpeciesId = 2,
            ExplosionSpeciesId = 2,
            ExplosionFrames = 25,
            BType = BulletType.Melee,
            Hardness = 7,
            GuardBreakerExtraHitCnt = 1,
            ReflectFireballXIfNotHarder = true,
            CharacterEmitSfxName = "SlashEmitSpd3",
            ExplosionSfxName="Melee_Explosion2",
            RemainsUponHit = true,
            ActiveVfxSpeciesId = 0, // TODO
            CollisionTypeMask = 0, // TODO
        };

        public static BulletConfig BasicBladeCrouchHit1 = new BulletConfig {
            StartupFrames = 5,
            StartupInvinsibleFrames = 2,
            ActiveFrames = 12,
            HitStunFrames = 18,
            BlockStunFrames = 8,
            CooldownFrames = 4,
            Damage = 5,
            PushbackVelX = 0.3f,
            PushbackVelY = PbPrimitives.underlying.NoLockVel,
            SelfLockVelX = PbPrimitives.underlying.NoLockVel,
            SelfLockVelY = PbPrimitives.underlying.NoLockVel,
            SelfLockVelYWhenFlying = PbPrimitives.underlying.NoLockVel,
            HitboxOffsetX = 10f,
            HitboxOffsetY = 13f,
            HitboxHalfSizeX = 18f,
            HitboxHalfSizeY = 6f,
            CancellableStFrame = 10,
            CancellableEdFrame = 24,
            SpeciesId = 2,
            ExplosionSpeciesId = 2,
            ExplosionFrames = 25,
            BType = BulletType.Melee,
            Hardness = 4,
            GuardBreakerExtraHitCnt = 1,
            ReflectFireballXIfNotHarder = true,
            CharacterEmitSfxName = "SlashEmitSpd1",
            ExplosionSfxName="Melee_Explosion2",
            RemainsUponHit = true,
            ActiveVfxSpeciesId = 0, // TODO
            CollisionTypeMask = 0, // TODO
        };

        public static BulletConfig BasicBladeAirHit1 = new BulletConfig(BasicBladeHit1);

        public static Skill BladeGirlGroundSlash1 = new Skill {
            Id = BladeGirlGroundSlash1Id,
            RecoveryFrames = BasicBladeHit1.StartupFrames+BasicBladeHit1.ActiveFrames+BasicBladeHit1.CooldownFrames,
            RecoveryFramesOnBlock = BasicBladeHit1.StartupFrames+BasicBladeHit1.ActiveFrames+BasicBladeHit1.CooldownFrames,
            RecoveryFramesOnHit = BasicBladeHit1.StartupFrames+BasicBladeHit1.ActiveFrames+BasicBladeHit1.CooldownFrames,
            TriggerType = SkillTriggerType.RisingEdge,
            BoundChState = CharacterState.Atk1
        }.AddHit(
            new BulletConfig(BasicBladeHit1)
            .UpsertCancelTransit(EncodePatternForCancelTransit(PbPrimitives.underlying.PatternB, false, false, false, false, false), BladeGirlGroundSlash2Id)
            .UpsertCancelTransit(EncodePatternForCancelTransit(PbPrimitives.underlying.PatternDownB, false, false, false, false, false), BladeGirlCrouchSlashId)
        );

        public static Skill BladeGirlGroundSlash2 = new Skill {
            Id = BladeGirlGroundSlash2Id,
            RecoveryFrames = BasicBladeHit2.StartupFrames+BasicBladeHit2.ActiveFrames+BasicBladeHit2.CooldownFrames,
            RecoveryFramesOnBlock = BasicBladeHit2.StartupFrames+BasicBladeHit2.ActiveFrames+BasicBladeHit2.CooldownFrames,
            RecoveryFramesOnHit = BasicBladeHit2.StartupFrames+BasicBladeHit2.ActiveFrames+BasicBladeHit2.CooldownFrames,
            TriggerType = SkillTriggerType.RisingEdge,
            BoundChState = CharacterState.Atk2
        }.AddHit(
            new BulletConfig(BasicBladeHit2)
            .UpsertCancelTransit(EncodePatternForCancelTransit(PbPrimitives.underlying.PatternB, false, false, false, false, false), BladeGirlGroundSlash3Id)
            .UpsertCancelTransit(EncodePatternForCancelTransit(PbPrimitives.underlying.PatternDownB, false, false, false, false, false), BladeGirlCrouchSlashId)
        );

        public static Skill BladeGirlGroundSlash3 = new Skill {
            Id = BladeGirlGroundSlash3Id,
            RecoveryFrames = BasicBladeHit3.StartupFrames+BasicBladeHit3.ActiveFrames+BasicBladeHit3.CooldownFrames,
            RecoveryFramesOnBlock = BasicBladeHit3.StartupFrames+BasicBladeHit3.ActiveFrames+BasicBladeHit3.CooldownFrames,
            RecoveryFramesOnHit = BasicBladeHit3.StartupFrames+BasicBladeHit3.ActiveFrames+BasicBladeHit3.CooldownFrames,
            TriggerType = SkillTriggerType.RisingEdge,
            BoundChState = CharacterState.Atk3
        }.AddHit(BasicBladeHit3);

        public static Skill BladeGirlCrouchSlash1 = new Skill {
            Id = BladeGirlCrouchSlashId,
            RecoveryFrames = BasicBladeCrouchHit1.StartupFrames+BasicBladeCrouchHit1.ActiveFrames+BasicBladeCrouchHit1.CooldownFrames,
            RecoveryFramesOnBlock = BasicBladeCrouchHit1.StartupFrames+BasicBladeCrouchHit1.ActiveFrames+BasicBladeCrouchHit1.CooldownFrames,
            RecoveryFramesOnHit = BasicBladeCrouchHit1.StartupFrames+BasicBladeCrouchHit1.ActiveFrames+BasicBladeCrouchHit1.CooldownFrames,
            TriggerType = SkillTriggerType.RisingEdge,
            BoundChState = CharacterState.CrouchAtk1
        }.AddHit(BasicBladeCrouchHit1);

        public static Skill BladeGirlAirSlash1 = new Skill {
            Id = BladeGirlAirSlash1Id,
            RecoveryFrames = BasicBladeAirHit1.StartupFrames+BasicBladeAirHit1.ActiveFrames+BasicBladeAirHit1.CooldownFrames,
            RecoveryFramesOnBlock = BasicBladeAirHit1.StartupFrames+BasicBladeAirHit1.ActiveFrames+BasicBladeAirHit1.CooldownFrames,
            RecoveryFramesOnHit = BasicBladeAirHit1.StartupFrames+BasicBladeAirHit1.ActiveFrames+BasicBladeAirHit1.CooldownFrames,
            TriggerType = SkillTriggerType.RisingEdge,
            BoundChState = CharacterState.InAirAtk1
        }.AddHit(BasicBladeAirHit1);

        public static MapField<uint, Skill> underlying = new MapField<uint, Skill>() { };

        static PbSkills() {
            underlying.Add(BladeGirlAirSlash1Id, BladeGirlAirSlash1);
            underlying.Add(BladeGirlGroundSlash1Id, BladeGirlGroundSlash1);
            underlying.Add(BladeGirlGroundSlash2Id, BladeGirlGroundSlash2);
            underlying.Add(BladeGirlGroundSlash3Id, BladeGirlGroundSlash3);
            underlying.Add(BladeGirlCrouchSlashId, BladeGirlCrouchSlash1);

            underlying.Add(HunterPistolAirId, HunterPistolAir);
            underlying.Add(HunterPistolId, HunterPistol);
            underlying.Add(HunterPistolWallId, HunterPistolWall);
            underlying.Add(HunterPistolWalkingId, HunterPistolWalking);
            underlying.Add(HunterPistolCrouchId, HunterPistolCrouch);
        }
    }
}
