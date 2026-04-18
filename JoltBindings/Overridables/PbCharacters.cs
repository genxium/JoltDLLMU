using Google.Protobuf.Collections;
using jtshared;
using static JoltCSharp.PbPrimitives;
using static jtshared.PbBuilders;

namespace JoltCSharp {
    public class PbCharacters {

        /**
         [WARNING] 

         The "CapsuleHalfHeight" component of "BlownUp/LayDown/Dying" MUST be smaller or equal to that of "Shrinked", such that when a character is blown up and falled onto a "slip-jump provider", it wouldn't trigger an unexpected slip-jump.

         Reference value for "JumpStartupFrames" -- Ibuki in Street Figher IV/V has a pre-jump frame count of 4, according to https://streetfighter.fandom.com/wiki/Jump. I also counted that of Ken in Street Fighter VI by 60fps recording and got the same result.

         In the current jumping implementation, "descending time t_desc" is proportional to "lambda = jump_acc_mag_y/gravity_y_magnitude", i.e. 

         ```
         t_desc = jump_ascending_seconds*sqrt{(lambda)*(1+lambda)}

         where 

         jump_ascending_seconds = (jump_startup_frames+1)*estimated_seconds_per_rdf
         ``` 
         */
        public static CharacterConfig BLADEGIRL = new CharacterConfig {
            SpeciesId = SPECIES_BLADEGIRL,
            SpeciesName = "BladeGirl",
            Hp = 150,
            LayDownFramesToRecover = 12,
            GetUpInvinsibleFrames = 19,
            GetUpFramesToRecover = 14,
            Speed = 2.3f * BATTLE_DYNAMICS_FPS,
            JumpAccMagY = 4.4f * GRAVITY_Y_MAGNITUDE, 
            JumpStartupFrames = 3,
            AccMagX = 0.13f * BATTLE_DYNAMICS_FPS * BATTLE_DYNAMICS_FPS,
            AngYSpeed = StdYAxisAngularSpeedPerRdf * BATTLE_DYNAMICS_FPS,
            DashingEnabled = true,
            SlidingEnabled = true,
            OnWallEnabled = true,
            CrouchingEnabled = true,
            CrouchingAtkEnabled = true, // It's actually weird to have "false == CrouchingAtkEnabled && true == CrouchingEnabled", but flexibility provided anyway
            WallJumpFramesToRecover = 8,
            WallJumpAccMagX = 0.25f * BATTLE_DYNAMICS_FPS * BATTLE_DYNAMICS_FPS,
            WallJumpAccMagY = 1.5f * GRAVITY_Y_MAGNITUDE,
            WallSlideVelY = (-1f) * BATTLE_DYNAMICS_FPS,
            WallAngYSpeed = OnWallYAxisAngularSpeedPerRdf * BATTLE_DYNAMICS_FPS,
            WallJumpFreeSpeed = 3.0f * BATTLE_DYNAMICS_FPS,

            CapsuleRadius = (8.0f), // [WARNING] Being too "wide" can make "CrouchIdle1" bouncing on slopes!
            CapsuleHalfHeight = (16.0f),
            ShrinkedCapsuleRadius = (8.0f),
            ShrinkedCapsuleHalfHeight = (10.0f),
            LayDownCapsuleRadius = (8.0f),
            LayDownCapsuleHalfHeight = (8.0f),
            DyingCapsuleRadius = (8.0f),
            DyingCapsuleHalfHeight = (8.0f),
            HasTurnAroundAnim = false,
            Hardness = 5,
            HasDimmedAnim = true,
            MinFallingVelY =  DEFAULT_MIN_FALLING_VEL_Y * BATTLE_DYNAMICS_FPS,
            DefaultAirDashQuota = 1,
            DefaultAirJumpQuota = 1,
            GroundDodgeEnabledByRdfCntFromBeginning = 12, 
            GroundDodgedFramesInvinsible = 25, 
            GroundDodgedFramesToRecover = 30, 
            GroundDodgedSpeed = (4.0f) * BATTLE_DYNAMICS_FPS, 
            GaugeIncWhenExhausted = 80,
            Ifc = IfaceCat.Flesh,
            TransformIntoSpeciesIdUponDeath = SPECIES_NONE_CH,
        };

        public static CharacterConfig BOUNTY_HUNTER = new CharacterConfig {
            SpeciesId = SPECIES_BOUNTYHUNTER,
            SpeciesName = "BountyHunter",
            Hp = 150,
            LayDownFramesToRecover = 16,
            GetUpInvinsibleFrames = 34,
            GetUpFramesToRecover = 30,
            Speed = 2.3f * BATTLE_DYNAMICS_FPS,
            JumpAccMagY = 4.4f * GRAVITY_Y_MAGNITUDE,
            JumpStartupFrames = 3,
            AccMagX = 0.13f * BATTLE_DYNAMICS_FPS * BATTLE_DYNAMICS_FPS,
            AngYSpeed = StdYAxisAngularSpeedPerRdf * BATTLE_DYNAMICS_FPS,
            DashingEnabled = true,
            SlidingEnabled = true,
            OnWallEnabled = true,
            CrouchingEnabled = true,
            CrouchingAtkEnabled = true,
            WallJumpFramesToRecover = 8,
            WallJumpAccMagX = 0.25f * BATTLE_DYNAMICS_FPS * BATTLE_DYNAMICS_FPS,
            WallJumpAccMagY = 1.5f * GRAVITY_Y_MAGNITUDE,
            WallSlideVelY = (-1) * BATTLE_DYNAMICS_FPS,
            WallAngYSpeed = OnWallYAxisAngularSpeedPerRdf * BATTLE_DYNAMICS_FPS,
            WallJumpFreeSpeed = 3.0f * BATTLE_DYNAMICS_FPS,

            CapsuleRadius = (8.0f),
            CapsuleHalfHeight = (16.0f),
            ShrinkedCapsuleRadius = (8.0f),
            ShrinkedCapsuleHalfHeight = (10.0f),
            LayDownCapsuleRadius = (8.0f),
            LayDownCapsuleHalfHeight = (8.0f),
            DyingCapsuleRadius = (8.0f),
            DyingCapsuleHalfHeight = (8.0f),
            HasTurnAroundAnim = false,
            Hardness = 5,
            MinFallingVelY = DEFAULT_MIN_FALLING_VEL_Y * BATTLE_DYNAMICS_FPS,
            DefaultAirDashQuota = 1,
            DefaultAirJumpQuota = 1,
            GroundDodgeEnabledByRdfCntFromBeginning = 12,
            GroundDodgedFramesInvinsible = 25,
            GroundDodgedFramesToRecover = 30,
            GroundDodgedSpeed = (4.0f) * BATTLE_DYNAMICS_FPS,
            Atk1UsesMagazine = true,
            Atk1Magazine = new InventorySlotConfig {
                StockType = InventorySlotStockType.TimedMagazineIv,
                Quota = 16,
                FramesToRecover = 25,
                BadgeName = "Pistol"
            },
            GaugeIncWhenExhausted = 80,
            HasBtnBCharging = true,
            Ifc = IfaceCat.Flesh,
            TransformIntoSpeciesIdUponDeath = SPECIES_NONE_CH,
            TrailingRdfChargeableChStates = {
                {(int)CharacterState.InAirIdle2ByJump, 25},
                {(int)CharacterState.InAirIdle1ByWallJump, 25},
            } 
        };

        public static CharacterConfig BLACKSABER1 = new CharacterConfig {
            SpeciesId = SPECIES_BLACKSABER1,
            SpeciesName = "BlackSaber1",
            Hp = 40,
            LayDownFramesToRecover = 12,
            GetUpInvinsibleFrames = 19,
            GetUpFramesToRecover = 14,
            Speed = 0.6f * BATTLE_DYNAMICS_FPS,
            JumpAccMagY = 4.6f * GRAVITY_Y_MAGNITUDE,
            JumpStartupFrames = 2,
            AccMagX = 0.05f * BATTLE_DYNAMICS_FPS * BATTLE_DYNAMICS_FPS,
            AngYSpeed = StdYAxisAngularSpeedPerRdf * BATTLE_DYNAMICS_FPS,
            VisionOffsetX = (16.0f),
            VisionOffsetY = (10.0f),
            VisionHalfHeight = (96.0f),
            VisionTopRadius = (32.0f),
            VisionBottomRadius = (48.0f),
            HasVisionReaction = true,
            VisionSearchIntervalPow2Minus1U = VISION_SEARCH_INTERVAL_IMMEDIATE_U-1,
            VisionSearchIntervalPow2Minus1 = VISION_SEARCH_INTERVAL_IMMEDIATE-1,
            CapsuleRadius = (6.0f), // [WARNING] Being too "wide" can make "CrouchIdle1" bouncing on slopes!
            CapsuleHalfHeight = (8.0f),
            ShrinkedCapsuleRadius = (6.0f),
            ShrinkedCapsuleHalfHeight = (8.0f),
            LayDownCapsuleRadius = (8.0f),
            LayDownCapsuleHalfHeight = (4.0f),
            DyingCapsuleRadius = (8.0f),
            DyingCapsuleHalfHeight = (4.0f),
            HasTurnAroundAnim = false,
            Hardness = 6,
            HasDimmedAnim = false,
            MinFallingVelY =  DEFAULT_MIN_FALLING_VEL_Y * BATTLE_DYNAMICS_FPS,
            GaugeIncWhenExhausted = 50,
            Ifc = IfaceCat.Metal,
            TransformIntoSpeciesIdUponDeath = SPECIES_NONE_CH,
        };

        public static CharacterConfig BLACKSABER_TEST_NO_VISION = new CharacterConfig {
            SpeciesId = SPECIES_BLACKSABER_TEST_NO_VISION,
            SpeciesName = "BlackSaber1",
            Hp = 10,
            LayDownFramesToRecover = 12,
            GetUpInvinsibleFrames = 19,
            GetUpFramesToRecover = 14,
            Speed = 0.8f * BATTLE_DYNAMICS_FPS,
            JumpAccMagY = 0, 
            AccMagX = 0.1f * BATTLE_DYNAMICS_FPS * BATTLE_DYNAMICS_FPS,
            AngYSpeed = StdYAxisAngularSpeedPerRdf * BATTLE_DYNAMICS_FPS,
            CapsuleRadius = (6.0f), // [WARNING] Being too "wide" can make "CrouchIdle1" bouncing on slopes!
            CapsuleHalfHeight = (8.0f),
            ShrinkedCapsuleRadius = (6.0f),
            ShrinkedCapsuleHalfHeight = (8.0f),
            LayDownCapsuleRadius = (8.0f),
            LayDownCapsuleHalfHeight = (4.0f),
            DyingCapsuleRadius = (8.0f),
            DyingCapsuleHalfHeight = (4.0f),
            HasTurnAroundAnim = false,
            Hardness = 6,
            HasDimmedAnim = false,
            MinFallingVelY =  DEFAULT_MIN_FALLING_VEL_Y * BATTLE_DYNAMICS_FPS,
            GaugeIncWhenExhausted = 50,
            Ifc = IfaceCat.Metal,
            TransformIntoSpeciesIdUponDeath = SPECIES_NONE_CH,
        };

        public static CharacterConfig BLACKSABER_TEST_WITH_VISION = new CharacterConfig {
            SpeciesId = SPECIES_BLACKSABER_TEST_WITH_VISION,
            SpeciesName = "BlackSaber1",
            Hp = 10,
            LayDownFramesToRecover = 12,
            GetUpInvinsibleFrames = 19,
            GetUpFramesToRecover = 14,
            Speed = 0.6f * BATTLE_DYNAMICS_FPS,
            JumpAccMagY = 4.6f * GRAVITY_Y_MAGNITUDE,
            JumpStartupFrames = 2,
            AccMagX = 0.05f * BATTLE_DYNAMICS_FPS * BATTLE_DYNAMICS_FPS,
            AngYSpeed = StdYAxisAngularSpeedPerRdf * BATTLE_DYNAMICS_FPS,
            VisionOffsetX = (16.0f),
            VisionOffsetY = (10.0f),
            VisionHalfHeight = (30.0f),
            VisionTopRadius = (32.0f),
            VisionBottomRadius = (48.0f),
            HasVisionReaction = true,
            VisionSearchIntervalPow2Minus1U = VISION_SEARCH_INTERVAL_IMMEDIATE_U-1,
            VisionSearchIntervalPow2Minus1 = VISION_SEARCH_INTERVAL_IMMEDIATE-1,
            CapsuleRadius = (6.0f), // [WARNING] Being too "wide" can make "CrouchIdle1" bouncing on slopes!
            CapsuleHalfHeight = (8.0f),
            ShrinkedCapsuleRadius = (6.0f),
            ShrinkedCapsuleHalfHeight = (8.0f),
            LayDownCapsuleRadius = (8.0f),
            LayDownCapsuleHalfHeight = (4.0f),
            DyingCapsuleRadius = (8.0f),
            DyingCapsuleHalfHeight = (4.0f),
            HasTurnAroundAnim = false,
            Hardness = 6,
            HasDimmedAnim = false,
            MinFallingVelY =  DEFAULT_MIN_FALLING_VEL_Y * BATTLE_DYNAMICS_FPS,
            GaugeIncWhenExhausted = 50,
            Ifc = IfaceCat.Metal,
            TransformIntoSpeciesIdUponDeath = SPECIES_NONE_CH,
        };

        public static CharacterConfig BLACKSHOOTER1 = new CharacterConfig {
            SpeciesId = SPECIES_BLACKSHOOTER1,
            SpeciesName = "BlackShooter1",
            Hp = 40,
            Mp = 300,
            MpRegenPerInterval = 60,
            MpRegenInterval = 80,
            LayDownFramesToRecover = 12,
            GetUpInvinsibleFrames = 19,
            GetUpFramesToRecover = 14,
            Speed = 0.6f * BATTLE_DYNAMICS_FPS,
            JumpAccMagY = 4.6f * GRAVITY_Y_MAGNITUDE,
            JumpStartupFrames = 2,
            AccMagX = 0.05f * BATTLE_DYNAMICS_FPS * BATTLE_DYNAMICS_FPS,
            AngYSpeed = StdYAxisAngularSpeedPerRdf * BATTLE_DYNAMICS_FPS,
            VisionOffsetX = (32.0f),
            VisionOffsetY = (10.0f),
            VisionHalfHeight = (96.0f),
            VisionTopRadius = (32.0f),
            VisionBottomRadius = (48.0f),
            HasVisionReaction = true,
            VisionSearchIntervalPow2Minus1U = VISION_SEARCH_INTERVAL_IMMEDIATE_U-1,
            VisionSearchIntervalPow2Minus1 = VISION_SEARCH_INTERVAL_IMMEDIATE-1,
            CapsuleRadius = (6.0f), // [WARNING] Being too "wide" can make "CrouchIdle1" bouncing on slopes!
            CapsuleHalfHeight = (8.0f),
            ShrinkedCapsuleRadius = (6.0f),
            ShrinkedCapsuleHalfHeight = (8.0f),
            LayDownCapsuleRadius = (8.0f),
            LayDownCapsuleHalfHeight = (4.0f),
            DyingCapsuleRadius = (8.0f),
            DyingCapsuleHalfHeight = (4.0f),
            HasTurnAroundAnim = false,
            Hardness = 6,
            HasDimmedAnim = false,
            ColliderDensity = 10.0f,
            MinFallingVelY =  DEFAULT_MIN_FALLING_VEL_Y * BATTLE_DYNAMICS_FPS,
            GaugeIncWhenExhausted = 50,
            Ifc = IfaceCat.Metal,
            TransformIntoSpeciesIdUponDeath = SPECIES_NONE_CH,
        };

        public static CharacterConfig BLACKTHROWER1 = new CharacterConfig {
            SpeciesId = SPECIES_BLACKTHROWER1,
            SpeciesName = "BlackThrower1",
            Hp = 40,
            Mp = 300,
            MpRegenPerInterval = 60,
            MpRegenInterval = 80,
            LayDownFramesToRecover = 12,
            GetUpInvinsibleFrames = 19,
            GetUpFramesToRecover = 14,
            Speed = 0.6f * BATTLE_DYNAMICS_FPS,
            JumpAccMagY = 4.6f * GRAVITY_Y_MAGNITUDE,
            JumpStartupFrames = 2,
            AccMagX = 0.05f * BATTLE_DYNAMICS_FPS * BATTLE_DYNAMICS_FPS,
            AngYSpeed = StdYAxisAngularSpeedPerRdf * BATTLE_DYNAMICS_FPS,
            VisionOffsetX = (32.0f),
            VisionOffsetY = (10.0f),
            VisionHalfHeight = (96.0f),
            VisionTopRadius = (32.0f),
            VisionBottomRadius = (48.0f),
            HasVisionReaction = true,
            VisionSearchIntervalPow2Minus1U = VISION_SEARCH_INTERVAL_IMMEDIATE_U-1,
            VisionSearchIntervalPow2Minus1 = VISION_SEARCH_INTERVAL_IMMEDIATE-1,
            CapsuleRadius = (6.0f), // [WARNING] Being too "wide" can make "CrouchIdle1" bouncing on slopes!
            CapsuleHalfHeight = (8.0f),
            ShrinkedCapsuleRadius = (6.0f),
            ShrinkedCapsuleHalfHeight = (8.0f),
            LayDownCapsuleRadius = (8.0f),
            LayDownCapsuleHalfHeight = (4.0f),
            DyingCapsuleRadius = (8.0f),
            DyingCapsuleHalfHeight = (4.0f),
            HasTurnAroundAnim = false,
            Hardness = 6,
            ColliderDensity = 15.0f,
            HasDimmedAnim = false,
            MinFallingVelY =  DEFAULT_MIN_FALLING_VEL_Y * BATTLE_DYNAMICS_FPS,
            GaugeIncWhenExhausted = 50,
            Ifc = IfaceCat.Metal,
            TransformIntoSpeciesIdUponDeath = SPECIES_NONE_CH,
        };

        public static MapField<uint, CharacterConfig> underlying = new MapField<uint, CharacterConfig>() { };

        static PbCharacters() {
            // BLADEGIRL
            BLADEGIRL.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternB, false, false, false, false, false, false, false, false), BladeGirlGroundSlash1Id);
            BLADEGIRL.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternB, false, false, false, false, true, false, false, false), BladeGirlGroundSlash1Id);

            BLADEGIRL.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternB, true, false, false, false, false, false, false, false), BladeGirlAirSlash1Id);

            BLADEGIRL.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternB, false, true, false, false, false, false, false, false), BladeGirlCrouchSlashId);
            BLADEGIRL.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternDownB, false, false, false, false, false, false, false, false), BladeGirlCrouchSlashId);
            BLADEGIRL.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternDownB, false, true, false, false, false, false, false, false), BladeGirlCrouchSlashId);

            BLADEGIRL.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternE, false, false, false, false, false, false, false, false), BladeGirlSlidingId);
            BLADEGIRL.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternE, false, false, false, false, true, false, false, false), BladeGirlSlidingId);
            BLADEGIRL.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternFrontE, false, false, false, false, false, false, false, false), BladeGirlSlidingId);
            BLADEGIRL.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternFrontE, false, false, false, false, true, false, false, false), BladeGirlSlidingId);
            BLADEGIRL.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternDownE, false, false, false, false, false, false, false, false), BladeGirlSlidingId);
            BLADEGIRL.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternDownE, false, false, false, false, true, false, false, false), BladeGirlSlidingId);

            BLADEGIRL.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternE, false, true, false, false, false, false, false, false), BladeGirlSlidingId);
            BLADEGIRL.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternE, false, true, false, false, true, false, false, false), BladeGirlSlidingId);
            BLADEGIRL.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternFrontE, false, true, false, false, false, false, false, false), BladeGirlSlidingId);
            BLADEGIRL.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternFrontE, false, true, false, false, true, false, false, false), BladeGirlSlidingId);
            BLADEGIRL.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternDownE, false, true, false, false, false, false, false, false), BladeGirlSlidingId);
            BLADEGIRL.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternDownE, false, true, false, false, true, false, false, false), BladeGirlSlidingId);

            BLADEGIRL.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternE, true, false, false, false, false, false, false, false), BladeGirlAirDashingId);
            BLADEGIRL.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternFrontE, true, false, false, false, false, false, false, false), BladeGirlAirDashingId);

            underlying.Add(BLADEGIRL.SpeciesId, BLADEGIRL);

            // BOUNTY_HUNTER
            BOUNTY_HUNTER.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternB, false, false, false, false, false, false, false, false), HunterPistolId);
            BOUNTY_HUNTER.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternB, false, false, false, false, true, false, false, false), HunterPistolWalkingId);

            BOUNTY_HUNTER.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternB, true, false, false, false, false, false, false, false), HunterPistolAirId);
            BOUNTY_HUNTER.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternB, true, false, true, false, false, false, false, false), HunterPistolWallId);

            BOUNTY_HUNTER.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternB, false, true, false, false, false, false, false, false), HunterPistolCrouchId);
            BOUNTY_HUNTER.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternDownB, false, false, false, false, false, false, false, false), HunterPistolCrouchId);
            BOUNTY_HUNTER.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternDownB, false, true, false, false, false, false, false, false), HunterPistolCrouchId);

            BOUNTY_HUNTER.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternReleasedB, false, false, false, false, false, false, false, false), HunterChargedPistolId);
            BOUNTY_HUNTER.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternReleasedB, false, false, false, false, true, false, false, false), HunterChargedPistolWalkingId);

            BOUNTY_HUNTER.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternReleasedB, true, false, false, false, false, false, false, false), HunterChargedPistolAirId);
            BOUNTY_HUNTER.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternReleasedB, true, false, true, false, false, false, false, false), HunterChargedPistolWallId);

            BOUNTY_HUNTER.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternReleasedB, false, true, false, false, false, false, false, false), HunterChargedPistolCrouchId);

            BOUNTY_HUNTER.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternE, false, false, false, false, false, false, false, false), HunterSlidingId);
            BOUNTY_HUNTER.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternE, false, false, false, false, true, false, false, false), HunterSlidingId);
            BOUNTY_HUNTER.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternFrontE, false, false, false, false, false, false, false, false), HunterSlidingId);
            BOUNTY_HUNTER.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternFrontE, false, false, false, false, true, false, false, false), HunterSlidingId);
            BOUNTY_HUNTER.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternFrontEHoldB, false, false, false, false, false, false, false, false), HunterSlidingId);
            BOUNTY_HUNTER.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternFrontEHoldB, false, false, false, false, true, false, false, false), HunterSlidingId);
            BOUNTY_HUNTER.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternDownE, false, false, false, false, false, false, false, false), HunterSlidingId);
            BOUNTY_HUNTER.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternDownE, false, false, false, false, true, false, false, false), HunterSlidingId);
            BOUNTY_HUNTER.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternDownEHoldB, false, false, false, false, false, false, false, false), HunterSlidingId);
            BOUNTY_HUNTER.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternDownEHoldB, false, false, false, false, true, false, false, false), HunterSlidingId);

            BOUNTY_HUNTER.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternE, false, true, false, false, false, false, false, false), HunterSlidingId);
            BOUNTY_HUNTER.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternE, false, true, false, false, true, false, false, false), HunterSlidingId);
            BOUNTY_HUNTER.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternFrontE, false, true, false, false, false, false, false, false), HunterSlidingId);
            BOUNTY_HUNTER.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternFrontE, false, true, false, false, true, false, false, false), HunterSlidingId);
            BOUNTY_HUNTER.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternFrontEHoldB, false, true, false, false, false, false, false, false), HunterSlidingId);
            BOUNTY_HUNTER.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternFrontEHoldB, false, true, false, false, true, false, false, false), HunterSlidingId);
            BOUNTY_HUNTER.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternDownE, false, true, false, false, false, false, false, false), HunterSlidingId);
            BOUNTY_HUNTER.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternDownE, false, true, false, false, true, false, false, false), HunterSlidingId);
            BOUNTY_HUNTER.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternDownEHoldB, false, true, false, false, false, false, false, false), HunterSlidingId);
            BOUNTY_HUNTER.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternDownEHoldB, false, true, false, false, true, false, false, false), HunterSlidingId);
            underlying.Add(BOUNTY_HUNTER.SpeciesId, BOUNTY_HUNTER);

            // BLACKSABER1
            BLACKSABER1.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternB, false, false, false, false, false, false, false, false), BlackSaber1GroundSlash1Id);
            BLACKSABER1.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternB, false, false, false, false, true, false, false, false), BlackSaber1GroundSlash1Id);

            BLACKSABER1.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternB, true, false, false, false, false, false, false, false), BlackSaber1AirSlash1Id);
            underlying.Add(BLACKSABER1.SpeciesId, BLACKSABER1);

            // BLACKSHOOTER1
            BLACKSHOOTER1.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternB, false, false, false, false, false, false, false, false), BlackShooter1RapidFireId);
            BLACKSHOOTER1.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternB, false, false, false, false, true, false, false, false), BlackShooter1RapidFireId);
            BLACKSHOOTER1.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternB, true, false, false, false, false, false, false, false), BlackShooter1RapidFireId);
            underlying.Add(BLACKSHOOTER1.SpeciesId, BLACKSHOOTER1);

            // BLACKTHROWER1
            BLACKTHROWER1.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternB, false, false, false, false, false, false, false, false), BlackThrower1TimedBombId);
            BLACKTHROWER1.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternB, false, false, false, false, true, false, false, false), BlackThrower1TimedBombId);
            BLACKTHROWER1.InitSkillTransit.Add(EncodePatternForInitSkill(PbPrimitives.underlying.PatternB, true, false, false, false, false, false, false, false), BlackThrower1TimedBombId);
            underlying.Add(BLACKTHROWER1.SpeciesId, BLACKTHROWER1);

            // BLACKSABER_TEST
            underlying.Add(BLACKSABER_TEST_NO_VISION.SpeciesId, BLACKSABER_TEST_NO_VISION);
            underlying.Add(BLACKSABER_TEST_WITH_VISION.SpeciesId, BLACKSABER_TEST_WITH_VISION);
        }
    }
}
