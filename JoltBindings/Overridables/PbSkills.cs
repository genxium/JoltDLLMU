using Google.Protobuf.Collections;
using jtshared;
using static JoltCSharp.PbPrimitives;
using static jtshared.PbBuilders;

namespace JoltCSharp {
    public class PbSkills {
        
        public static System.Numerics.Quaternion cTurn45DegsWrtZAxis = System.Numerics.Quaternion.CreateFromAxisAngle(System.Numerics.Vector3.UnitZ, 0.7853981625f);
        public static System.Numerics.Quaternion cTurn60DegsWrtZAxis = System.Numerics.Quaternion.CreateFromAxisAngle(System.Numerics.Vector3.UnitZ, 1.0472f);

        private static BulletConfig BasicPistolBulletAir = new BulletConfig {
            StartupFrames = 6,
            ActiveFrames = 360,
            HitStunFrames = 8,
            BlockStunFrames = 7,
            CooldownFrames = 11,
            Damage = 10,
            PushbackVelX = PbPrimitives.underlying.NoLockVel,
            PushbackVelY = PbPrimitives.underlying.NoLockVel,
            SelfLockVelX = PbPrimitives.underlying.NoLockVel,
            SelfLockVelY = PbPrimitives.underlying.NoLockVel,
            SelfLockVelYWhenFlying = PbPrimitives.underlying.NoLockVel,
            HitboxOffsetX = 16f,
            HitboxOffsetY = HunterPistolOffsetYAir,
            HitboxHalfSizeX = 2,
            HitboxHalfSizeY = 2,
            AnimName = "PistolBullet1",
            Speed = 6.0f * BATTLE_DYNAMICS_FPS,
            Hardness = 4,
            HitAnimRdfCnt = 45,
            VanishingAnimRdfCnt = 25,
            // [WARNING] For "MechanicalCartridge", don't set "PushbackVelX" or "PushbackVelY", instead set density of the bullet and rely on the Physics engine to calculate impulse.
            BType = BulletType.MechanicalCartridge,
            CharacterEmitSfxName = "PistolEmit",
            HitSfxName = "Piercing",
            HitOnRockSfxName = "Vanishing8",
            CancellableStFrame = 11,
            CancellableEdFrame = 19,
            CollisionTypeMask = 0 // TODO
        };

        private static BulletConfig BasicPistolBulletGround = new BulletConfig(BasicPistolBulletAir)
                                                                .SetHitboxOffsets(16f, HunterPistolOffsetY)
                                                                .SetAllowsWalking(true)
                                                                .SetAllowsCrouching(true);

        private static BulletConfig BasicPistolBulletCrouch = new BulletConfig(BasicPistolBulletAir)
                                                                .SetSelfLockVel(0, 0, 0)
                                                                .SetHitboxOffsets(11f, HunterPistolOffsetYCrouch);

        private static BulletConfig BasicPistolBulletWalking = new BulletConfig(BasicPistolBulletAir)
                                                                .SetHitboxOffsets(22f, HunterPistolOffsetY)
                                                                .SetAllowsWalking(true)
                                                                .SetAllowsCrouching(true);

        private static BulletConfig BasicPistolBulletOnWall = new BulletConfig(BasicPistolBulletAir)
                                                                .SetHitboxOffsets(BasicPistolBulletAir.HitboxOffsetX, HunterPistolOffsetYOnWall);

        public static Skill HunterPistolWall = new Skill {
            Id = HunterPistolWallId,
            RecoveryFrames = BasicPistolBulletAir.StartupFrames+BasicPistolBulletAir.CooldownFrames,
            RecoveryFramesOnBlock = BasicPistolBulletAir.StartupFrames+BasicPistolBulletAir.CooldownFrames,
            RecoveryFramesOnHit = BasicPistolBulletAir.StartupFrames+BasicPistolBulletAir.CooldownFrames,
            InvocationType = SkillInvocation.RisingEdge,
            BoundChState = CharacterState.OnWallAtk1,
            Atk1MagazineDelta = 1,
        }.AddHit(new BulletConfig(BasicPistolBulletOnWall)
            .UpsertCancelTransit(EncodePatternForCancelTransit(PbPrimitives.underlying.PatternUpB, true, false, true, false, false), HunterAirSlashId)
        );

        public static Skill HunterPistol = new Skill {
            Id = HunterPistolId,
            RecoveryFrames = HunterPistolWall.RecoveryFrames,
            RecoveryFramesOnBlock = HunterPistolWall.RecoveryFramesOnBlock,
            RecoveryFramesOnHit = HunterPistolWall.RecoveryFramesOnHit,
            InvocationType = SkillInvocation.RisingEdge,
            BoundChState = CharacterState.Atk1,
            Atk1MagazineDelta = 1,
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
            InvocationType = SkillInvocation.RisingEdge,
            BoundChState = CharacterState.InAirAtk1,
            Atk1MagazineDelta = 1,
        }
        .AddHit(new BulletConfig(BasicPistolBulletAir)
            .UpsertCancelTransit(EncodePatternForCancelTransit(PbPrimitives.underlying.PatternUpB, true, false, false, false, false), HunterAirSlashId)
        );

        public static Skill HunterPistolWalking = new Skill {
            Id = HunterPistolWalkingId,
            RecoveryFrames = HunterPistolWall.RecoveryFrames,
            RecoveryFramesOnBlock = HunterPistolWall.RecoveryFramesOnBlock,
            RecoveryFramesOnHit = HunterPistolWall.RecoveryFramesOnHit,
            InvocationType = SkillInvocation.RisingEdge,
            BoundChState = CharacterState.WalkingAtk1,
            Atk1MagazineDelta = 1,
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
            InvocationType = SkillInvocation.RisingEdge,
            BoundChState = CharacterState.CrouchAtk1,
            Atk1MagazineDelta = 1,
        }
        .AddHit(new BulletConfig(BasicPistolBulletCrouch)
            .UpsertCancelTransit(EncodePatternForCancelTransit(PbPrimitives.underlying.PatternUpB, false, true, false, false, false), HunterDragonPunchId)
        );

/////////////////////////////////////////////////////////////////////////////////////////
        private static BulletConfig BasicChargedPistolBulletAir = new BulletConfig {
            StartupFrames = 6,
            ActiveFrames = 360,
            HitStunFrames = DEFAULT_BLOW_UP_RDF_CNT_TO_RECOVER,
            BlowUp = true,
            BlockStunFrames = 18,
            CooldownFrames = 11,
            Damage = 28,
            PushbackVelX = 3.0f * BATTLE_DYNAMICS_FPS,
            PushbackVelY = 4.0f * BATTLE_DYNAMICS_FPS,
            SelfLockVelX = PbPrimitives.underlying.NoLockVel,
            SelfLockVelY = PbPrimitives.underlying.NoLockVel,
            SelfLockVelYWhenFlying = PbPrimitives.underlying.NoLockVel,
            HitboxOffsetX = 16f,
            HitboxOffsetY = HunterPistolOffsetYAir,
            HitboxHalfSizeX = 2,
            HitboxHalfSizeY = 2,
            AnimName = "PistolBullet2",
            Speed = 8.5f * BATTLE_DYNAMICS_FPS,
            Hardness = 7,
            HitAnimRdfCnt = 45,
            VanishingAnimRdfCnt = 25,
            // [WARNING] For "MechanicalCartridge", don't set "PushbackVelX" or "PushbackVelY", instead set density of the bullet and rely on the Physics engine to calculate impulse.
            BType = BulletType.MechanicalCartridge,
            CharacterEmitSfxName = "ChargedPistolEmit",
            HitSfxName = "Piercing",
            HitOnRockSfxName = "Vanishing8",
            CancellableStFrame = 11,
            CancellableEdFrame = 19,
            CollisionTypeMask = 0, // TODO
        };

        private static BulletConfig BasicChargedPistolBulletGround = new BulletConfig(BasicChargedPistolBulletAir)
                                                                .SetHitboxOffsets(16f, HunterPistolOffsetY)
                                                                .SetAllowsWalking(true)
                                                                .SetAllowsCrouching(true);

        private static BulletConfig BasicChargedPistolBulletCrouch = new BulletConfig(BasicChargedPistolBulletAir)
                                                                .SetSelfLockVel(0, 0, 0)
                                                                .SetHitboxOffsets(11f, HunterPistolOffsetYCrouch);

        private static BulletConfig BasicChargedPistolBulletWalking = new BulletConfig(BasicChargedPistolBulletAir)
                                                                .SetHitboxOffsets(22f, HunterPistolOffsetY)
                                                                .SetAllowsWalking(true)
                                                                .SetAllowsCrouching(true);

        private static BulletConfig BasicChargedPistolBulletOnWall = new BulletConfig(BasicChargedPistolBulletAir)
                                                                .SetHitboxOffsets(BasicChargedPistolBulletAir.HitboxOffsetX, HunterPistolOffsetYOnWall);

        public static Skill HunterChargedPistolWall = new Skill {
            Id = HunterChargedPistolWallId,
            RecoveryFrames = BasicChargedPistolBulletAir.StartupFrames+BasicChargedPistolBulletAir.CooldownFrames,
            RecoveryFramesOnBlock = BasicChargedPistolBulletAir.StartupFrames+BasicChargedPistolBulletAir.CooldownFrames,
            RecoveryFramesOnHit = BasicChargedPistolBulletAir.StartupFrames+BasicChargedPistolBulletAir.CooldownFrames,
            InvocationType = SkillInvocation.FallingEdge,
            BoundChState = CharacterState.OnWallAtk1,
            Atk1MagazineDelta = 1,
        }.AddHit(new BulletConfig(BasicChargedPistolBulletOnWall)
            .UpsertCancelTransit(EncodePatternForCancelTransit(PbPrimitives.underlying.PatternUpB, true, false, true, false, false), HunterAirSlashId)
        );

        public static Skill HunterChargedPistol = new Skill {
            Id = HunterChargedPistolId,
            RecoveryFrames = HunterChargedPistolWall.RecoveryFrames,
            RecoveryFramesOnBlock = HunterChargedPistolWall.RecoveryFramesOnBlock,
            RecoveryFramesOnHit = HunterChargedPistolWall.RecoveryFramesOnHit,
            InvocationType = SkillInvocation.FallingEdge,
            BoundChState = CharacterState.Atk1,
            Atk1MagazineDelta = 1,
        }
        .AddHit(new BulletConfig(BasicChargedPistolBulletGround)
            .UpsertCancelTransit(EncodePatternForCancelTransit(PbPrimitives.underlying.PatternUpB, false, false, false, false, false), HunterDragonPunchId)
            .UpsertCancelTransit(EncodePatternForCancelTransit(PbPrimitives.underlying.PatternUpB, false, false, false, false, true), HunterDragonPunchId)
        );

        public static Skill HunterChargedPistolAir = new Skill {
            Id = HunterChargedPistolAirId,
            RecoveryFrames = HunterChargedPistolWall.RecoveryFrames,
            RecoveryFramesOnBlock = HunterChargedPistolWall.RecoveryFramesOnBlock,
            RecoveryFramesOnHit = HunterChargedPistolWall.RecoveryFramesOnHit,
            InvocationType = SkillInvocation.FallingEdge,
            BoundChState = CharacterState.InAirAtk1,
            Atk1MagazineDelta = 1,
        }
        .AddHit(new BulletConfig(BasicChargedPistolBulletAir)
            .UpsertCancelTransit(EncodePatternForCancelTransit(PbPrimitives.underlying.PatternUpB, true, false, false, false, false), HunterAirSlashId)
        );

        public static Skill HunterChargedPistolWalking = new Skill {
            Id = HunterChargedPistolWalkingId,
            RecoveryFrames = HunterChargedPistolWall.RecoveryFrames,
            RecoveryFramesOnBlock = HunterChargedPistolWall.RecoveryFramesOnBlock,
            RecoveryFramesOnHit = HunterChargedPistolWall.RecoveryFramesOnHit,
            InvocationType = SkillInvocation.FallingEdge,
            BoundChState = CharacterState.WalkingAtk1,
            Atk1MagazineDelta = 1,
        }
        .AddHit(new BulletConfig(BasicChargedPistolBulletWalking)
            .UpsertCancelTransit(EncodePatternForCancelTransit(PbPrimitives.underlying.PatternUpB, false, false, false, false, false), HunterDragonPunchId)
            .UpsertCancelTransit(EncodePatternForCancelTransit(PbPrimitives.underlying.PatternUpB, false, false, false, false, true), HunterDragonPunchId)
        );

        public static Skill HunterChargedPistolCrouch = new Skill {
            Id = HunterChargedPistolCrouchId,
            RecoveryFrames = HunterChargedPistolWall.RecoveryFrames,
            RecoveryFramesOnBlock = HunterChargedPistolWall.RecoveryFramesOnBlock,
            RecoveryFramesOnHit = HunterChargedPistolWall.RecoveryFramesOnHit,
            InvocationType = SkillInvocation.FallingEdge,
            BoundChState = CharacterState.CrouchAtk1,
            Atk1MagazineDelta = 1,
        }
        .AddHit(new BulletConfig(BasicChargedPistolBulletCrouch)
            .UpsertCancelTransit(EncodePatternForCancelTransit(PbPrimitives.underlying.PatternUpB, false, true, false, false, false), HunterDragonPunchId)
        );
/////////////////////////////////////////////////////////////////////////////////////////

        public static BulletConfig BasicBladeHit1 = new BulletConfig {
            StartupFrames = 5,
            StartupInvinsibleFrames = 3,
            ActiveFrames = 12,
            HitStunFrames = 18,
            BlockStunFrames = 8,
            CooldownFrames = 5,
            Damage = 10,
            PushbackVelX = 0.3f * BATTLE_DYNAMICS_FPS,
            PushbackVelY = PbPrimitives.underlying.NoLockVel,
            SelfLockVelX = 0,
            SelfLockVelY = PbPrimitives.underlying.NoLockVel,
            SelfLockVelYWhenFlying = PbPrimitives.underlying.NoLockVel,
            HitboxOffsetX = 10f,
            HitboxOffsetY = 17f,
            HitboxHalfSizeX = 18f,
            HitboxHalfSizeY = 18f,
            CancellableStFrame = 10,
            CancellableEdFrame = 21,
            AnimName = "MeleeSlash2",
            HitAnimRdfCnt = 45,
            VanishingAnimRdfCnt = 25,
            BType = BulletType.Melee,
            Hardness = 5,
            GuardBreakerExtraHitCnt = 1,
            ReflectFireballXIfNotHarder = true,
            CharacterEmitSfxName = "SlashEmitSpd1",
            HitSfxName="Melee_Vanishing2",
            RemainsUponHit = true,
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
            PushbackVelX = 0.5f * BATTLE_DYNAMICS_FPS,
            PushbackVelY = PbPrimitives.underlying.NoLockVel,
            SelfLockVelX = 0.1f * BATTLE_DYNAMICS_FPS,
            SelfLockVelY = PbPrimitives.underlying.NoLockVel,
            SelfLockVelYWhenFlying = PbPrimitives.underlying.NoLockVel,
            HitboxOffsetX = 12f,
            HitboxOffsetY = 17f,
            HitboxHalfSizeX = 18f,
            HitboxHalfSizeY = 18f,
            CancellableStFrame = 11,
            CancellableEdFrame = 24,
            AnimName = "MeleeSlash2",
            HitAnimRdfCnt = 45,
            VanishingAnimRdfCnt = 25,
            BType = BulletType.Melee,
            Hardness = 6,
            GuardBreakerExtraHitCnt = 1,
            ReflectFireballXIfNotHarder = true,
            CharacterEmitSfxName = "SlashEmitSpd2",
            HitSfxName="Melee_Vanishing2",
            RemainsUponHit = true,
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
            PushbackVelX = 0.5f * BATTLE_DYNAMICS_FPS,
            PushbackVelY = -0.5f * BATTLE_DYNAMICS_FPS,
            SelfLockVelX = 0.2f * BATTLE_DYNAMICS_FPS,
            SelfLockVelY = PbPrimitives.underlying.NoLockVel,
            SelfLockVelYWhenFlying = PbPrimitives.underlying.NoLockVel,
            HitboxOffsetX = 24f,
            HitboxOffsetY = 17f,
            HitboxHalfSizeX = 22f,
            HitboxHalfSizeY = 18f,
            AnimName = "MeleeSlash2",
            HitAnimRdfCnt = 45,
            VanishingAnimRdfCnt = 25,
            BType = BulletType.Melee,
            Hardness = 7,
            GuardBreakerExtraHitCnt = 1,
            ReflectFireballXIfNotHarder = true,
            CharacterEmitSfxName = "SlashEmitSpd3",
            HitSfxName="Melee_Vanishing2",
            RemainsUponHit = true,
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
            PushbackVelX = 0.3f * BATTLE_DYNAMICS_FPS,
            PushbackVelY = PbPrimitives.underlying.NoLockVel,
            SelfLockVelX = 0,
            SelfLockVelY = PbPrimitives.underlying.NoLockVel,
            SelfLockVelYWhenFlying = PbPrimitives.underlying.NoLockVel,
            HitboxOffsetX = 10f,
            HitboxOffsetY = 13f,
            HitboxHalfSizeX = 18f,
            HitboxHalfSizeY = 6f,
            CancellableStFrame = 10,
            CancellableEdFrame = 24,
            AnimName = "MeleeSlash2",
            HitAnimRdfCnt = 45,
            VanishingAnimRdfCnt = 25,
            BType = BulletType.Melee,
            Hardness = 4,
            GuardBreakerExtraHitCnt = 1,
            ReflectFireballXIfNotHarder = true,
            CharacterEmitSfxName = "SlashEmitSpd1",
            HitSfxName="Melee_Vanishing2",
            RemainsUponHit = false,
            MeleeHitSelfStunFrames = PbPrimitives.DEFAULT_MELEE_HIT_SELF_STUN_FRAMES,
            CollisionTypeMask = 0, // TODO
        };

        public static BulletConfig BasicBladeAirHit1 = new BulletConfig(BasicBladeHit1)
                                                       .SetActiveFrames(16)
                                                       .SetCancellableFrames(0, 0)
                                                       .SetHitboxOffsets(12f, 16f)
                                                       .SetHitboxHalfSizes(20f, 18f)
                                                       .SetRemainsUponHit(false)
                                                       .SetMeleeHitSelfStunFrames(PbPrimitives.DEFAULT_MELEE_HIT_SELF_STUN_FRAMES)
                                                       .SetSelfLockVel(PbPrimitives.underlying.NoLockVel, PbPrimitives.underlying.NoLockVel, PbPrimitives.underlying.NoLockVel);

        public static Skill BladeGirlGroundSlash1 = new Skill {
            Id = BladeGirlGroundSlash1Id,
            RecoveryFrames = BasicBladeHit1.StartupFrames+BasicBladeHit1.ActiveFrames+BasicBladeHit1.CooldownFrames,
            RecoveryFramesOnBlock = BasicBladeHit1.StartupFrames+BasicBladeHit1.ActiveFrames+BasicBladeHit1.CooldownFrames,
            RecoveryFramesOnHit = BasicBladeHit1.StartupFrames+BasicBladeHit1.ActiveFrames+BasicBladeHit1.CooldownFrames,
            InvocationType = SkillInvocation.RisingEdge,
            BoundChState = CharacterState.Atk1
        }.AddHit(
            new BulletConfig(BasicBladeHit1)
            .SetRemainsUponHit(false)
            .SetMeleeHitSelfStunFrames(10)
            .UpsertCancelTransit(EncodePatternForCancelTransit(PbPrimitives.underlying.PatternB, false, false, false, false, false), BladeGirlGroundSlash2Id)
            .UpsertCancelTransit(EncodePatternForCancelTransit(PbPrimitives.underlying.PatternDownB, false, false, false, false, false), BladeGirlCrouchSlashId)
        );

        public static Skill BladeGirlGroundSlash2 = new Skill {
            Id = BladeGirlGroundSlash2Id,
            RecoveryFrames = BasicBladeHit2.StartupFrames+BasicBladeHit2.ActiveFrames+BasicBladeHit2.CooldownFrames,
            RecoveryFramesOnBlock = BasicBladeHit2.StartupFrames+BasicBladeHit2.ActiveFrames+BasicBladeHit2.CooldownFrames,
            RecoveryFramesOnHit = BasicBladeHit2.StartupFrames+BasicBladeHit2.ActiveFrames+BasicBladeHit2.CooldownFrames,
            InvocationType = SkillInvocation.RisingEdge,
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
            InvocationType = SkillInvocation.RisingEdge,
            BoundChState = CharacterState.Atk3
        }.AddHit(BasicBladeHit3);

        public static Skill BladeGirlCrouchSlash1 = new Skill {
            Id = BladeGirlCrouchSlashId,
            RecoveryFrames = BasicBladeCrouchHit1.StartupFrames+BasicBladeCrouchHit1.ActiveFrames+BasicBladeCrouchHit1.CooldownFrames,
            RecoveryFramesOnBlock = BasicBladeCrouchHit1.StartupFrames+BasicBladeCrouchHit1.ActiveFrames+BasicBladeCrouchHit1.CooldownFrames,
            RecoveryFramesOnHit = BasicBladeCrouchHit1.StartupFrames+BasicBladeCrouchHit1.ActiveFrames+BasicBladeCrouchHit1.CooldownFrames,
            InvocationType = SkillInvocation.RisingEdge,
            BoundChState = CharacterState.CrouchAtk1
        }.AddHit(BasicBladeCrouchHit1);

        public static Skill BladeGirlAirSlash1 = new Skill {
            Id = BladeGirlAirSlash1Id,
            RecoveryFrames = BasicBladeAirHit1.StartupFrames+BasicBladeAirHit1.ActiveFrames+BasicBladeAirHit1.CooldownFrames,
            RecoveryFramesOnBlock = BasicBladeAirHit1.StartupFrames+BasicBladeAirHit1.ActiveFrames+BasicBladeAirHit1.CooldownFrames,
            RecoveryFramesOnHit = BasicBladeAirHit1.StartupFrames+BasicBladeAirHit1.ActiveFrames+BasicBladeAirHit1.CooldownFrames,
            InvocationType = SkillInvocation.RisingEdge,
            BoundChState = CharacterState.InAirAtk1
        }.AddHit(BasicBladeAirHit1);

        public static BulletConfig BasicGroundDashingHit1 = new BulletConfig {
            StartupFrames = 4,
            StartupInvinsibleFrames = 2,
            ActiveFrames = 12,
            CooldownFrames = 5,
            PushbackVelX = PbPrimitives.underlying.NoLockVel,
            PushbackVelY = PbPrimitives.underlying.NoLockVel,
            SelfLockVelX = 8.0f * BATTLE_DYNAMICS_FPS,
            SelfLockVelY = PbPrimitives.underlying.NoLockVel,
            SelfLockVelYWhenFlying = PbPrimitives.underlying.NoLockVel,
            BType = BulletType.Melee,
            CharacterEmitSfxName = "SlashEmitSpd1",
            CollisionTypeMask = 0, // TODO
        };

        public static BulletConfig BasicAirDashingHit1 = new BulletConfig {
            StartupFrames = 4,
            StartupInvinsibleFrames = 2,
            ActiveFrames = 12,
            CooldownFrames = 5,
            PushbackVelX = PbPrimitives.underlying.NoLockVel,
            PushbackVelY = PbPrimitives.underlying.NoLockVel,
            SelfLockVelX = 7.8f * BATTLE_DYNAMICS_FPS,
            SelfLockVelY = 0,
            SelfLockVelYWhenFlying = 0,
            BType = BulletType.Melee,
            CharacterEmitSfxName = "SlashEmitSpd1",
            CollisionTypeMask = 0, // TODO
        };

        public static BulletConfig BasicSlidingHit1 = new BulletConfig {
            StartupFrames = 4,
            StartupInvinsibleFrames = 2,
            ActiveFrames = 12,
            CooldownFrames = 5,
            PushbackVelX = PbPrimitives.underlying.NoLockVel,
            PushbackVelY = PbPrimitives.underlying.NoLockVel,
            SelfLockVelX = 7.5f * BATTLE_DYNAMICS_FPS,
            SelfLockVelY = PbPrimitives.underlying.NoLockVel,
            SelfLockVelYWhenFlying = PbPrimitives.underlying.NoLockVel,
            BType = BulletType.Melee,
            CharacterEmitSfxName = "SlashEmitSpd2",
            CollisionTypeMask = 0, // TODO
        };

        public static Skill BladeGirlSliding = new Skill {
            Id = BladeGirlSlidingId,
            RecoveryFrames = BasicSlidingHit1.StartupFrames+BasicSlidingHit1.ActiveFrames+BasicSlidingHit1.CooldownFrames,
            RecoveryFramesOnBlock = BasicSlidingHit1.StartupFrames+BasicSlidingHit1.ActiveFrames+BasicSlidingHit1.CooldownFrames,
            RecoveryFramesOnHit = BasicSlidingHit1.StartupFrames+BasicSlidingHit1.ActiveFrames+BasicSlidingHit1.CooldownFrames,
            InvocationType = SkillInvocation.RisingEdge,
            BoundChState = CharacterState.Sliding
        }.AddHit(
            new BulletConfig(BasicSlidingHit1)
        );

        public static Skill BladeGirlAirDashing = new Skill {
            Id = BladeGirlAirDashingId,
            RecoveryFrames = BasicAirDashingHit1.StartupFrames+BasicAirDashingHit1.ActiveFrames+BasicAirDashingHit1.CooldownFrames,
            RecoveryFramesOnBlock = BasicAirDashingHit1.StartupFrames+BasicAirDashingHit1.ActiveFrames+BasicAirDashingHit1.CooldownFrames,
            RecoveryFramesOnHit = BasicAirDashingHit1.StartupFrames+BasicAirDashingHit1.ActiveFrames+BasicAirDashingHit1.CooldownFrames,
            InvocationType = SkillInvocation.RisingEdge,
            BoundChState = CharacterState.InAirDashing
        }.AddHit(
            new BulletConfig(BasicAirDashingHit1)
        );

        public static Skill BountyhunterSliding = new Skill {
            Id = HunterSlidingId,
            RecoveryFrames = BasicSlidingHit1.StartupFrames+BasicSlidingHit1.ActiveFrames+BasicSlidingHit1.CooldownFrames,
            RecoveryFramesOnBlock = BasicSlidingHit1.StartupFrames+BasicSlidingHit1.ActiveFrames+BasicSlidingHit1.CooldownFrames,
            RecoveryFramesOnHit = BasicSlidingHit1.StartupFrames+BasicSlidingHit1.ActiveFrames+BasicSlidingHit1.CooldownFrames,
            InvocationType = SkillInvocation.RisingEdge,
            BoundChState = CharacterState.Sliding
        }.AddHit(
            new BulletConfig(BasicSlidingHit1)
        );

        public static BulletConfig SlowBladeHit1 = new BulletConfig(BasicBladeHit1)
         .SetStartupFrames(26)
         .SetActiveFrames(4)
         .SetCooldownFrames(6)
        ; 

        public static BulletConfig SlowBladeAirHit1 = new BulletConfig(SlowBladeHit1)
                                                       .SetSelfLockVel(PbPrimitives.underlying.NoLockVel, PbPrimitives.underlying.NoLockVel, PbPrimitives.underlying.NoLockVel);

        public static Skill BlackSaber1AirSlash1 = new Skill {
            Id = BlackSaber1AirSlash1Id,
            RecoveryFrames = SlowBladeAirHit1.StartupFrames+SlowBladeAirHit1.ActiveFrames+SlowBladeAirHit1.CooldownFrames,
            RecoveryFramesOnBlock = SlowBladeAirHit1.StartupFrames+SlowBladeAirHit1.ActiveFrames+SlowBladeAirHit1.CooldownFrames,
            RecoveryFramesOnHit = SlowBladeAirHit1.StartupFrames+SlowBladeAirHit1.ActiveFrames+SlowBladeAirHit1.CooldownFrames,
            InvocationType = SkillInvocation.RisingEdge,
            BoundChState = CharacterState.InAirAtk1
        }.AddHit(SlowBladeAirHit1);

        public static Skill BlackSaber1GroundSlash1 = new Skill {
            Id = BlackSaber1GroundSlash1Id,
            RecoveryFrames = SlowBladeHit1.StartupFrames+SlowBladeHit1.ActiveFrames+SlowBladeHit1.CooldownFrames,
            RecoveryFramesOnBlock = SlowBladeHit1.StartupFrames+SlowBladeHit1.ActiveFrames+SlowBladeHit1.CooldownFrames,
            RecoveryFramesOnHit = SlowBladeHit1.StartupFrames+SlowBladeHit1.ActiveFrames+SlowBladeHit1.CooldownFrames,
            InvocationType = SkillInvocation.RisingEdge,
            BoundChState = CharacterState.Atk1
        }.AddHit(SlowBladeHit1);

        private static BulletConfig BasicRapidFireHit1 = new BulletConfig {
            StartupFrames = 45,
            ActiveFrames = 60,
            HitStunFrames = 8,
            BlockStunFrames = 8,
            CooldownFrames = 12,
            Damage = 2,
            PushbackVelX = PbPrimitives.underlying.NoLockVel,
            PushbackVelY = PbPrimitives.underlying.NoLockVel,
            SelfLockVelX = 0,
            SelfLockVelY = PbPrimitives.underlying.NoLockVel,
            SelfLockVelYWhenFlying = PbPrimitives.underlying.NoLockVel,
            HitboxOffsetX = 16f,
            HitboxOffsetY = BlackShooter1RapidFireOffsetY,
            HitboxHalfSizeX = 2,
            HitboxHalfSizeY = 2,
            AnimName = "MiniBullet1",
            Speed = 6.5f * BATTLE_DYNAMICS_FPS,
            Hardness = 4,
            HitAnimRdfCnt = 45,
            VanishingAnimRdfCnt = 25,
            // [WARNING] For "MechanicalCartridge", don't set "PushbackVelX" or "PushbackVelY", instead set density of the bullet and rely on the Physics engine to calculate impulse.
            BType = BulletType.MechanicalCartridge,
            CharacterEmitSfxName = "PistolEmit",
            HitSfxName = "Piercing",
            HitOnRockSfxName = "Vanishing7",
            TakesGravity = false,
            RenderRotationAlongVelocity = false,
            MhType = MultiHitType.FromEmission,
            SimultaneousMultiHitCnt = 3,
            CollisionTypeMask = 0 // TODO
        };

        private static BulletConfig BasicRapidFireHit2 = new BulletConfig(BasicRapidFireHit1)
                                                                .SetStartupFrames(BasicRapidFireHit1.StartupFrames + 9)
                                                                .SetHitboxOffsets(16f, BlackShooter1RapidFireOffsetY + 4f)
                                                                .SetSimultaneousMultiHitCnt(2);

        private static BulletConfig BasicRapidFireHit3 = new BulletConfig(BasicRapidFireHit2)
                                                                .SetStartupFrames(BasicRapidFireHit2.StartupFrames + 9)
                                                                .SetHitboxOffsets(16f, BlackShooter1RapidFireOffsetY - 4f)
                                                                .SetSimultaneousMultiHitCnt(1);

        private static BulletConfig BasicRapidFireHit4 = new BulletConfig(BasicRapidFireHit3)
                                                                .SetStartupFrames(BasicRapidFireHit3.StartupFrames + 9)
                                                                .SetHitboxOffsets(16f, BlackShooter1RapidFireOffsetY - 2f)
                                                                .SetMhType(MultiHitType.None)
                                                                .SetSimultaneousMultiHitCnt(0);

        public static Skill BlackShooter1RapidFire = new Skill{
            Id = BlackShooter1RapidFireId,
            RecoveryFrames = BasicRapidFireHit1.StartupFrames+BasicRapidFireHit1.CooldownFrames,
            RecoveryFramesOnBlock = BasicRapidFireHit1.StartupFrames+BasicRapidFireHit1.CooldownFrames,
            RecoveryFramesOnHit = BasicRapidFireHit1.StartupFrames+BasicRapidFireHit1.CooldownFrames,
            MpDelta = 220,
            InvocationType = SkillInvocation.RisingEdge,
            BoundChState = CharacterState.Atk1,
        }
        .AddHit(BasicRapidFireHit1) 
        .AddHit(BasicRapidFireHit2) 
        .AddHit(BasicRapidFireHit3) 
        .AddHit(BasicRapidFireHit4); 

        private static BulletConfig BasicTimedBombHit1 = new BulletConfig {
            StartupFrames = 80,
            ActiveFrames = 180,
            HitStunFrames = 4,
            BlockStunFrames = 4,
            CooldownFrames = 40,
            Damage = 0,
            PushbackVelX = PbPrimitives.underlying.NoLockVel,
            PushbackVelY = PbPrimitives.underlying.NoLockVel,
            SelfLockVelX = 0,
            SelfLockVelY = PbPrimitives.underlying.NoLockVel,
            SelfLockVelYWhenFlying = PbPrimitives.underlying.NoLockVel,
            HitboxOffsetX = 16f,
            HitboxOffsetY = 14f,
            HitboxHalfSizeX = 5f,
            HitboxHalfSizeY = 5f,
            AnimName = "TimedBomb1",
            Speed = 3.5f * BATTLE_DYNAMICS_FPS,
            Hardness = 7,
            HitAnimRdfCnt = 45,
            VanishingAnimRdfCnt = 25,
            BType = BulletType.MechanicalBouncerSpherical,
            CharacterEmitSfxName = "PistolEmit",
            HitSfxName = "Piercing",
            HitOnRockSfxName = "Vanishing8",
            TakesGravity = true,
            MhType = MultiHitType.FromPrevHitActualOrActiveTimeUp,
            InitQ = new PbQuat {
                X = cTurn45DegsWrtZAxis.X,
                Y = cTurn45DegsWrtZAxis.Y,
                Z = cTurn45DegsWrtZAxis.Z,
                W = cTurn45DegsWrtZAxis.W,
            },
            NoHitAnim = true,
            Restitution = 0.92f,
            GravityFactor = 0.55f,
            CollisionTypeMask = 0 // TODO
        };
        private static BulletConfig BasicTimedBombHit2 = new BulletConfig {
            StartupFrames = 4,
            ActiveFrames = 8,
            HitStunFrames = DEFAULT_BLOW_UP_RDF_CNT_TO_RECOVER,
            BlowUp = true,
            BlockStunFrames = 60,
            CooldownFrames = 12,
            Damage = 36,
            PushbackVelX = 3.5f * BATTLE_DYNAMICS_FPS,
            PushbackVelY = 5.5f * BATTLE_DYNAMICS_FPS,
            SelfLockVelX = PbPrimitives.underlying.NoLockVel,
            SelfLockVelY = PbPrimitives.underlying.NoLockVel,
            SelfLockVelYWhenFlying = PbPrimitives.underlying.NoLockVel,
            HitboxOffsetX = 0f,
            HitboxOffsetY = 0f,
            HitboxHalfSizeX = 16f,
            HitboxHalfSizeY = 16f,
            Hardness = 8,
            HitAnimRdfCnt = 45,
            VanishingAnimRdfCnt = 25,
            BType = BulletType.MagicalFireball,
            CharacterEmitSfxName = "PistolEmit",
            HitSfxName = "Piercing",
            HitOnRockSfxName = "Vanishing8",
            CollisionTypeMask = 0 // TODO
        };

        public static Skill BlackThrower1TimedBomb = new Skill{
            Id = BlackThrower1TimedBombId,
            RecoveryFrames = BasicTimedBombHit1.StartupFrames+BasicTimedBombHit1.CooldownFrames,
            RecoveryFramesOnBlock = BasicTimedBombHit1.StartupFrames+BasicTimedBombHit1.CooldownFrames,
            RecoveryFramesOnHit = BasicTimedBombHit1.StartupFrames+BasicTimedBombHit1.CooldownFrames,
            MpDelta = 220,
            InvocationType = SkillInvocation.RisingEdge,
            BoundChState = CharacterState.Atk1,
        }
        .AddHit(BasicTimedBombHit1) 
        .AddHit(BasicTimedBombHit2); 

        public static MapField<uint, Skill> underlying = new MapField<uint, Skill>() { };

        static PbSkills() {
            underlying.Add(BladeGirlAirSlash1Id, BladeGirlAirSlash1);
            underlying.Add(BladeGirlGroundSlash1Id, BladeGirlGroundSlash1);
            underlying.Add(BladeGirlGroundSlash2Id, BladeGirlGroundSlash2);
            underlying.Add(BladeGirlGroundSlash3Id, BladeGirlGroundSlash3);
            underlying.Add(BladeGirlCrouchSlashId, BladeGirlCrouchSlash1);
            underlying.Add(BladeGirlSlidingId, BladeGirlSliding);
            underlying.Add(BladeGirlAirDashingId, BladeGirlAirDashing);

            underlying.Add(HunterPistolAirId, HunterPistolAir);
            underlying.Add(HunterPistolId, HunterPistol);
            underlying.Add(HunterPistolWallId, HunterPistolWall);
            underlying.Add(HunterPistolWalkingId, HunterPistolWalking);
            underlying.Add(HunterPistolCrouchId, HunterPistolCrouch);

            underlying.Add(HunterChargedPistolAirId, HunterChargedPistolAir);
            underlying.Add(HunterChargedPistolId, HunterChargedPistol);
            underlying.Add(HunterChargedPistolWallId, HunterChargedPistolWall);
            underlying.Add(HunterChargedPistolWalkingId, HunterChargedPistolWalking);
            underlying.Add(HunterChargedPistolCrouchId, HunterChargedPistolCrouch);

            underlying.Add(HunterSlidingId, BountyhunterSliding);

            underlying.Add(BlackSaber1GroundSlash1Id, BlackSaber1GroundSlash1);
            underlying.Add(BlackSaber1AirSlash1Id, BlackSaber1AirSlash1);

            underlying.Add(BlackShooter1RapidFireId, BlackShooter1RapidFire);

            underlying.Add(BlackThrower1TimedBombId, BlackThrower1TimedBomb);
        }
    }
}
