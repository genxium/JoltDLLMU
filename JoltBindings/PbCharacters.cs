using Google.Protobuf.Collections;
using jtshared;
using static JoltCSharp.PbPrimitives;

namespace JoltCSharp {
    public class PbCharacters {

        /**
         [WARNING] 

         The "CapsuleHalfHeight" component of "BlownUp/LayDown/Dying" MUST be smaller or equal to that of "Shrinked", such that when a character is blown up and falled onto a "slip-jump provider", it wouldn't trigger an unexpected slip-jump.

         Reference value for "ProactiveJumpStartupFrames" -- Ibuki in Street Figher IV/V has a pre-jump frame count of 4, according to https://streetfighter.fandom.com/wiki/Jump. I also counted that of Ken in Street Fighter VI by 60fps recording and got the same result.
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
            JumpingInitVelY = 8f * BATTLE_DYNAMICS_FPS, 
            AccMag = 7.8f * BATTLE_DYNAMICS_FPS,
            InertiaFramesToRecover = 4,
            DashingEnabled = true,
            SlidingEnabled = true,
            OnWallEnabled = true,
            CrouchingEnabled = true,
            CrouchingAtkEnabled = true, // It's actually weird to have "false == CrouchingAtkEnabled && true == CrouchingEnabled", but flexibility provided anyway
            WallJumpingFramesToRecover = 8,
            WallJumpingInitVelX = (2.5f) * BATTLE_DYNAMICS_FPS,
            WallJumpingInitVelY =  (8f) * BATTLE_DYNAMICS_FPS,
            WallSlidingVelY = (-1f) * BATTLE_DYNAMICS_FPS,
            VisionOffsetX = (8.0f),
            VisionOffsetY = (5.0f),
            VisionSizeX = (220.0f),
            VisionSizeY = (100.0f),
            VisionSearchIntervalPow2Minus1U = VISION_SEARCH_INTERVAL_IMMEDIATE_U-1,
            VisionSearchIntervalPow2Minus1 = VISION_SEARCH_INTERVAL_IMMEDIATE-1,
            CapsuleRadius = (8.0f), // [WARNING] Being too "wide" can make "CrouchIdle1" bouncing on slopes!
            CapsuleHalfHeight = (16.0f),
            ShrinkedCapsuleRadius = (8.0f),
            ShrinkedCapsuleHalfHeight = (9.0f),
            LayDownCapsuleRadius = (16.0f),
            LayDownCapsuleHalfHeight = (8.0f),
            DyingCapsuleRadius = (16.0f),
            DyingCapsuleHalfHeight = (6.0f),
            HasTurnAroundAnim = false,
            ProactiveJumpStartupFrames = 4,
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
            GaugeIncWhenKilled = 80,
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
            JumpingInitVelY = 8.9f * BATTLE_DYNAMICS_FPS,
            AccMag = 7.8f * BATTLE_DYNAMICS_FPS,
            InertiaFramesToRecover = 4,
            VisionOffsetX = (8.0f),
            VisionOffsetY = (16.0f),
            VisionSizeX = (160.0f),
            VisionSizeY = (100.0f),
            VisionSearchIntervalPow2Minus1U = VISION_SEARCH_INTERVAL_IMMEDIATE_U - 1,
            VisionSearchIntervalPow2Minus1 = VISION_SEARCH_INTERVAL_IMMEDIATE - 1,
            CapsuleRadius = (8.0f),
            CapsuleHalfHeight = (16.0f),
            ShrinkedCapsuleRadius = (8.0f),
            ShrinkedCapsuleHalfHeight = (9.0f),
            LayDownCapsuleRadius = (16.0f),
            LayDownCapsuleHalfHeight = (8.0f),
            DyingCapsuleRadius = (16.0f),
            DyingCapsuleHalfHeight = (6.0f),
            HasTurnAroundAnim = false,
            CrouchingEnabled = true,
            CrouchingAtkEnabled = true,
            OnWallEnabled = true,
            WallJumpingFramesToRecover = 8,
            WallJumpingInitVelX = (2.5f) * BATTLE_DYNAMICS_FPS,
            WallJumpingInitVelY = (8f) * BATTLE_DYNAMICS_FPS,
            WallSlidingVelY = (-1) * BATTLE_DYNAMICS_FPS,
            DashingEnabled = true,
            SlidingEnabled = true,
            ProactiveJumpStartupFrames = 4,
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
            GaugeIncWhenKilled = 80,
            HasBtnBCharging = true,
            Ifc = IfaceCat.Flesh,
            TransformIntoSpeciesIdUponDeath = SPECIES_NONE_CH        
        };

        public static MapField<uint, CharacterConfig> underlying = new MapField<uint, CharacterConfig>() { };

        static PbCharacters() {
            underlying.Add(BLADEGIRL.SpeciesId, BLADEGIRL);

            BOUNTY_HUNTER.LoopingChStates.Add(((int)CharacterState.InAirIdle1ByJump), 23);
            BOUNTY_HUNTER.LoopingChStates.Add(((int)CharacterState.InAirIdle1ByWallJump), 25);
            BOUNTY_HUNTER.LoopingChStates.Add(((int)CharacterState.InAirIdle2ByJump), 31);
            underlying.Add(BOUNTY_HUNTER.SpeciesId, BOUNTY_HUNTER);
        }
    }
}
