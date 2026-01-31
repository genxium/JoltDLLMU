using Google.Protobuf.Collections;
using jtshared;
using static JoltCSharp.PbPrimitives;

namespace JoltCSharp {
    public class PbCharacters {

        /**
         [WARNING] 

         The "CapsuleHalfHeight" component of "BlownUp/LayDown/Dying" MUST be smaller or equal to that of "Shrinked", such that when a character is blown up and falled onto a "slip-jump provider", it wouldn't trigger an unexpected slip-jump.

         Reference value for "JumpStartupFrames" -- Ibuki in Street Figher IV/V has a pre-jump frame count of 4, according to https://streetfighter.fandom.com/wiki/Jump. I also counted that of Ken in Street Fighter VI by 60fps recording and got the same result.
         */
        public static CharacterConfig BLADEGIRL = new CharacterConfig {
            SpeciesId = SPECIES_BLADEGIRL,
            SpeciesName = "BladeGirl",
            Hp = 150,
            LayDownFrames = 12,
            LayDownFramesToRecover = 12,
            GetUpInvinsibleFrames = 19,
            GetUpFramesToRecover = 14,
            Speed = 2.3f * BATTLE_DYNAMICS_FPS,
            JumpAccMagY = 120f * BATTLE_DYNAMICS_FPS, 
            JumpStartupFrames = 3,
            AccMagX = 7.8f * BATTLE_DYNAMICS_FPS,
            AngYSpeed = StdYAxisAngularSpeedPerRdf * BATTLE_DYNAMICS_FPS,
            DashingEnabled = true,
            SlidingEnabled = true,
            OnWallEnabled = true,
            CrouchingEnabled = true,
            CrouchingAtkEnabled = true, // It's actually weird to have "false == CrouchingAtkEnabled && true == CrouchingEnabled", but flexibility provided anyway
            WallJumpFramesToRecover = 8,
            WallJumpAccMagX = 16f * BATTLE_DYNAMICS_FPS,
            WallJumpAccMagY = 35f * BATTLE_DYNAMICS_FPS,
            WallSlideVelY = (-1f) * BATTLE_DYNAMICS_FPS,
            WallAngYSpeed = OnWallYAxisAngularSpeedPerRdf * BATTLE_DYNAMICS_FPS,
            WallJumpFreeSpeed = 3.0f * BATTLE_DYNAMICS_FPS,

            CapsuleRadius = (8.0f), // [WARNING] Being too "wide" can make "CrouchIdle1" bouncing on slopes!
            CapsuleHalfHeight = (16.0f),
            ShrinkedCapsuleRadius = (8.0f),
            ShrinkedCapsuleHalfHeight = (12.0f),
            LayDownCapsuleRadius = (16.0f),
            LayDownCapsuleHalfHeight = (8.0f),
            DyingCapsuleRadius = (16.0f),
            DyingCapsuleHalfHeight = (6.0f),
            HasTurnAroundAnim = false,
            Hardness = 5,
            HasDimmedAnim = true,
            MinFallingVelY =  DEFAULT_MIN_FALLING_VEL_Y * BATTLE_DYNAMICS_FPS,
            SlipJumpThresHoldBelowTopFace = DEFAULT_SLIP_JUMP_THRESHOLD_BELOW_TOP_FACE,
            SlipJumpCharacterDropY = DEFAULT_SLIP_JUMP_CHARACTER_DROP,
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
            LayDownFrames = 16,
            LayDownFramesToRecover = 16,
            GetUpInvinsibleFrames = 34,
            GetUpFramesToRecover = 30,
            Speed = 2.3f * BATTLE_DYNAMICS_FPS,
            JumpAccMagY = 120f * BATTLE_DYNAMICS_FPS,
            JumpStartupFrames = 3,
            AccMagX = 7.8f * BATTLE_DYNAMICS_FPS,
            AngYSpeed = StdYAxisAngularSpeedPerRdf * BATTLE_DYNAMICS_FPS,
            CapsuleRadius = (8.0f),
            CapsuleHalfHeight = (16.0f),
            ShrinkedCapsuleRadius = (8.0f),
            ShrinkedCapsuleHalfHeight = (12.0f),
            LayDownCapsuleRadius = (16.0f),
            LayDownCapsuleHalfHeight = (8.0f),
            DyingCapsuleRadius = (16.0f),
            DyingCapsuleHalfHeight = (6.0f),
            HasTurnAroundAnim = false,
            CrouchingEnabled = true,
            CrouchingAtkEnabled = true,
            OnWallEnabled = true,
            WallJumpFramesToRecover = 8,
            WallJumpAccMagX = 16f * BATTLE_DYNAMICS_FPS,
            WallJumpAccMagY = 35f * BATTLE_DYNAMICS_FPS,
            WallSlideVelY = (-1) * BATTLE_DYNAMICS_FPS,
            WallAngYSpeed = OnWallYAxisAngularSpeedPerRdf * BATTLE_DYNAMICS_FPS,
            WallJumpFreeSpeed = 3.0f * BATTLE_DYNAMICS_FPS,
            DashingEnabled = true,
            SlidingEnabled = true,
            Hardness = 5,
            MinFallingVelY = DEFAULT_MIN_FALLING_VEL_Y * BATTLE_DYNAMICS_FPS,
            SlipJumpThresHoldBelowTopFace = DEFAULT_SLIP_JUMP_THRESHOLD_BELOW_TOP_FACE,
            SlipJumpCharacterDropY = DEFAULT_SLIP_JUMP_CHARACTER_DROP,
            DefaultAirDashQuota = 1,
            DefaultAirJumpQuota = 1,
            AirJumpVfxSpeciesId = 0, // TODO
            BtnBChargedVfxSpeciesId = 0, // 0
            GroundDodgeEnabledByRdfCntFromBeginning = 12,
            GroundDodgedFramesInvinsible = 25,
            GroundDodgedFramesToRecover = 30,
            GroundDodgedSpeed = (4.0f) * BATTLE_DYNAMICS_FPS,
            Atk1UsesMagazine = true,
            Atk1Magazine = new InventorySlotConfig {
                StockType = InventorySlotStockType.TimedMagazineIv,
                Quota = 18,
                FramesToRecover = 25,
                BuffSpeciesId = 0,
                SkillId = HunterPistolId,
                SkillIdAir = HunterPistolId,
            },
            GaugeIncWhenExhausted = 80,
            HasBtnBCharging = true,
            Ifc = IfaceCat.Flesh,
            TransformIntoSpeciesIdUponDeath = SPECIES_NONE_CH,
        };

        public static CharacterConfig BLACKSABER1 = new CharacterConfig {
            SpeciesId = SPECIES_BLACKSABER1,
            SpeciesName = "BlackSaber1",
            Hp = 40,
            LayDownFrames = 12,
            LayDownFramesToRecover = 12,
            GetUpInvinsibleFrames = 19,
            GetUpFramesToRecover = 14,
            Speed = 0.6f * BATTLE_DYNAMICS_FPS,
            JumpAccMagY = 120f * BATTLE_DYNAMICS_FPS,
            JumpStartupFrames = 2,
            AccMagX = 3.0f * BATTLE_DYNAMICS_FPS,
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
            CapsuleHalfHeight = (10.0f),
            ShrinkedCapsuleRadius = (8.0f),
            ShrinkedCapsuleHalfHeight = (12.0f),
            LayDownCapsuleRadius = (16.0f),
            LayDownCapsuleHalfHeight = (8.0f),
            DyingCapsuleRadius = (16.0f),
            DyingCapsuleHalfHeight = (6.0f),
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
            Hp = 40,
            LayDownFrames = 12,
            LayDownFramesToRecover = 12,
            GetUpInvinsibleFrames = 19,
            GetUpFramesToRecover = 14,
            Speed = 0.8f * BATTLE_DYNAMICS_FPS,
            JumpAccMagY = 0, 
            AccMagX = 6.8f * BATTLE_DYNAMICS_FPS,
            AngYSpeed = StdYAxisAngularSpeedPerRdf * BATTLE_DYNAMICS_FPS,
            CapsuleRadius = (6.0f), // [WARNING] Being too "wide" can make "CrouchIdle1" bouncing on slopes!
            CapsuleHalfHeight = (10.0f),
            ShrinkedCapsuleRadius = (8.0f),
            ShrinkedCapsuleHalfHeight = (12.0f),
            LayDownCapsuleRadius = (16.0f),
            LayDownCapsuleHalfHeight = (8.0f),
            DyingCapsuleRadius = (16.0f),
            DyingCapsuleHalfHeight = (6.0f),
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
            Hp = 40,
            LayDownFrames = 12,
            LayDownFramesToRecover = 12,
            GetUpInvinsibleFrames = 19,
            GetUpFramesToRecover = 14,
            Speed = 0.6f * BATTLE_DYNAMICS_FPS,
            JumpAccMagY = 120f * BATTLE_DYNAMICS_FPS,
            JumpStartupFrames = 2,
            AccMagX = 3.0f * BATTLE_DYNAMICS_FPS,
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
            CapsuleHalfHeight = (10.0f),
            ShrinkedCapsuleRadius = (8.0f),
            ShrinkedCapsuleHalfHeight = (12.0f),
            LayDownCapsuleRadius = (16.0f),
            LayDownCapsuleHalfHeight = (8.0f),
            DyingCapsuleRadius = (16.0f),
            DyingCapsuleHalfHeight = (6.0f),
            HasTurnAroundAnim = false,
            Hardness = 6,
            HasDimmedAnim = false,
            MinFallingVelY =  DEFAULT_MIN_FALLING_VEL_Y * BATTLE_DYNAMICS_FPS,
            GaugeIncWhenExhausted = 50,
            Ifc = IfaceCat.Metal,
            TransformIntoSpeciesIdUponDeath = SPECIES_NONE_CH,
        };

        public static MapField<uint, CharacterConfig> underlying = new MapField<uint, CharacterConfig>() { };

        static PbCharacters() {
            // BLADEGIRL
            BLADEGIRL.InitSkillTransit.Add(PbSkills.EncodePatternForInitSkill(PbPrimitives.underlying.PatternB, false, false, false, false, false, false, false, false), BladeGirlGroundSlash1Id);
            BLADEGIRL.InitSkillTransit.Add(PbSkills.EncodePatternForInitSkill(PbPrimitives.underlying.PatternB, false, false, false, false, true, false, false, false), BladeGirlGroundSlash1Id);

            BLADEGIRL.InitSkillTransit.Add(PbSkills.EncodePatternForInitSkill(PbPrimitives.underlying.PatternB, true, false, false, false, false, false, false, false), BladeGirlAirSlash1Id);

            BLADEGIRL.InitSkillTransit.Add(PbSkills.EncodePatternForInitSkill(PbPrimitives.underlying.PatternB, false, true, false, false, false, false, false, false), BladeGirlCrouchSlashId);
            BLADEGIRL.InitSkillTransit.Add(PbSkills.EncodePatternForInitSkill(PbPrimitives.underlying.PatternDownB, false, false, false, false, false, false, false, false), BladeGirlCrouchSlashId);
            BLADEGIRL.InitSkillTransit.Add(PbSkills.EncodePatternForInitSkill(PbPrimitives.underlying.PatternDownB, false, true, false, false, false, false, false, false), BladeGirlCrouchSlashId);

            BLADEGIRL.InitSkillTransit.Add(PbSkills.EncodePatternForInitSkill(PbPrimitives.underlying.PatternE, false, false, false, false, false, false, false, false), BladeGirlGroundDashingId);
            BLADEGIRL.InitSkillTransit.Add(PbSkills.EncodePatternForInitSkill(PbPrimitives.underlying.PatternE, false, false, false, false, true, false, false, false), BladeGirlGroundDashingId);
            BLADEGIRL.InitSkillTransit.Add(PbSkills.EncodePatternForInitSkill(PbPrimitives.underlying.PatternFrontE, false, false, false, false, true, false, false, false), BladeGirlGroundDashingId);

            BLADEGIRL.InitSkillTransit.Add(PbSkills.EncodePatternForInitSkill(PbPrimitives.underlying.PatternE, true, false, false, false, false, false, false, false), BladeGirlAirDashingId);
            BLADEGIRL.InitSkillTransit.Add(PbSkills.EncodePatternForInitSkill(PbPrimitives.underlying.PatternFrontE, true, false, false, false, false, false, false, false), BladeGirlAirDashingId);

            underlying.Add(BLADEGIRL.SpeciesId, BLADEGIRL);

            // BOUNTY_HUNTER
            BOUNTY_HUNTER.InitSkillTransit.Add(PbSkills.EncodePatternForInitSkill(PbPrimitives.underlying.PatternB, false, false, false, false, false, false, false, false), HunterPistolId);
            BOUNTY_HUNTER.InitSkillTransit.Add(PbSkills.EncodePatternForInitSkill(PbPrimitives.underlying.PatternB, false, false, false, false, true, false, false, false), HunterPistolWalkingId);

            BOUNTY_HUNTER.InitSkillTransit.Add(PbSkills.EncodePatternForInitSkill(PbPrimitives.underlying.PatternB, true, false, false, false, false, false, false, false), HunterPistolAirId);
            BOUNTY_HUNTER.InitSkillTransit.Add(PbSkills.EncodePatternForInitSkill(PbPrimitives.underlying.PatternB, true, false, true, false, false, false, false, false), HunterPistolWallId);

            BOUNTY_HUNTER.InitSkillTransit.Add(PbSkills.EncodePatternForInitSkill(PbPrimitives.underlying.PatternB, false, true, false, false, false, false, false, false), HunterPistolCrouchId);
            BOUNTY_HUNTER.InitSkillTransit.Add(PbSkills.EncodePatternForInitSkill(PbPrimitives.underlying.PatternDownB, false, false, false, false, false, false, false, false), HunterPistolCrouchId);
            BOUNTY_HUNTER.InitSkillTransit.Add(PbSkills.EncodePatternForInitSkill(PbPrimitives.underlying.PatternDownB, false, true, false, false, false, false, false, false), HunterPistolCrouchId);

            BOUNTY_HUNTER.InitSkillTransit.Add(PbSkills.EncodePatternForInitSkill(PbPrimitives.underlying.PatternE, false, false, false, false, false, false, false, false), HunterSlidingId);
            BOUNTY_HUNTER.InitSkillTransit.Add(PbSkills.EncodePatternForInitSkill(PbPrimitives.underlying.PatternE, false, false, false, false, true, false, false, false), HunterSlidingId);
            BOUNTY_HUNTER.InitSkillTransit.Add(PbSkills.EncodePatternForInitSkill(PbPrimitives.underlying.PatternFrontE, false, false, false, false, true, false, false, false), HunterSlidingId);
            underlying.Add(BOUNTY_HUNTER.SpeciesId, BOUNTY_HUNTER);

            // BLACKSABER1
            underlying.Add(BLACKSABER1.SpeciesId, BLACKSABER1);

            // BLACKSABER_TEST
            underlying.Add(BLACKSABER_TEST_NO_VISION.SpeciesId, BLACKSABER_TEST_NO_VISION);
            underlying.Add(BLACKSABER_TEST_WITH_VISION.SpeciesId, BLACKSABER_TEST_WITH_VISION);
        }
    }
}
