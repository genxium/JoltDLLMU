using jtshared;
using static FrontendOnlyGeometry;

namespace JoltCSharp {
    public partial class PbCharactersOverride : PbCharacters {
        /////////////////////////////////////////////////////////////////////////////////////////
        public PbCharactersOverride(PrimitiveConsts primitiveConsts) : base(primitiveConsts) {
        }

        public static PbCharactersOverride Instance {
            get {
                if (null != _instance) {
                    return _instance;
                }
                lock (_padLock) {
                    if (null == _instance) {
                        _instance = new PbCharactersOverride(PbPrimitivesOverride.Instance.getUnderlying());
                    }
                }
                return _instance;
            }
        }

        /////////////////////////////////////////////////////////////////////////////////////////
        private static PbCharactersOverride _instance = null;
        private static readonly object _padLock = new object();
        private bool _initialized = false;
        protected override bool lazyInit() {
            if (_initialized) {
                return true;
            }
            bool res = base.lazyInit();
            var BLADEGIRL = underlying[PbPrimitivesOverride.SPECIES_BLADEGIRL];

            BLADEGIRL.AccMagX = 0.25f * PbPrimitivesOverride.BATTLE_DYNAMICS_FPS * PbPrimitivesOverride.BATTLE_DYNAMICS_FPS;
            BLADEGIRL.AngYSpeed = 1.5f * PbPrimitivesOverride.StdYAxisAngularSpeedPerRdf * PbPrimitivesOverride.BATTLE_DYNAMICS_FPS;
            BLADEGIRL.LoopingChStates.Add((int)CharacterState.Walking, 12);

            var BOUNTYHUNTER = underlying[PbPrimitivesOverride.SPECIES_BOUNTYHUNTER];
            BOUNTYHUNTER.AccMagX = 0.13f * PbPrimitivesOverride.BATTLE_DYNAMICS_FPS * PbPrimitivesOverride.BATTLE_DYNAMICS_FPS;
            BOUNTYHUNTER.AngYSpeed = 1.5f * PbPrimitivesOverride.StdYAxisAngularSpeedPerRdf * PbPrimitivesOverride.BATTLE_DYNAMICS_FPS;

            var BAT1 = underlying[PbPrimitivesOverride.SPECIES_BAT1];
            BAT1.VisionOffsetX = (10.0f);
            BAT1.VisionOffsetY = (3.0f);
            BAT1.VisionHalfHeight = (32.0f);
            BAT1.VisionTopRadius = (12.0f);
            BAT1.VisionBottomRadius = (16.0f);

            var BLACKTHROWER1 = underlying[PbPrimitivesOverride.SPECIES_BLACKTHROWER1];
            BLACKTHROWER1.Hp = 60;
            BLACKTHROWER1.JumpAccMagY = 0;
            BLACKTHROWER1.JumpStartupFrames = 0;

            var BLACKSABER1 = underlying[PbPrimitivesOverride.SPECIES_BLACKSABER1];
            BLACKSABER1.VisionOffsetX = (8.0f);
            BLACKSABER1.VisionOffsetY = (12.0f);
            BLACKSABER1.VisionHalfHeight = (24.0f);
            BLACKSABER1.VisionTopRadius = (16.0f);
            BLACKSABER1.VisionBottomRadius = (32.0f);
            BLACKSABER1.ColliderDensity = (32.0f);

            var WOLVERINE1 = underlying[PbPrimitivesOverride.SPECIES_WOLVERINE1];

            WOLVERINE1.Hardness = 7;
            WOLVERINE1.CapsuleRadius = (12.0f); // [WARNING] Being too "wide" can make "CrouchIdle1" bouncing on slopes!
            WOLVERINE1.CapsuleHalfHeight = (18.0f);
            WOLVERINE1.ShrinkedCapsuleRadius = (12.0f);
            WOLVERINE1.ShrinkedCapsuleHalfHeight = (10.0f);
            WOLVERINE1.LayDownCapsuleRadius = (12.0f);
            WOLVERINE1.LayDownCapsuleHalfHeight = (8.0f);
            WOLVERINE1.DyingCapsuleRadius = (12.0f);
            WOLVERINE1.DyingCapsuleHalfHeight = (8.0f);
            WOLVERINE1.VisionOffsetX = (24.0f);
            WOLVERINE1.VisionOffsetY = (20.0f);
            WOLVERINE1.VisionHalfHeight = (96.0f);
            WOLVERINE1.VisionTopRadius = (16.0f);
            WOLVERINE1.VisionBottomRadius = (64.0f);
            WOLVERINE1.HasAwakingAnim = true;
            //WOLVERINE1.HasTurnAroundAnim = false;
            WOLVERINE1.AwakingFramesToRecover = 135;
            WOLVERINE1.AwakingFramesInvinsible = 135;
            WOLVERINE1.InplacePromptYOffset = 48f;

            rectifyByPPU();

            _initialized = true;
            return _initialized;
        }

        private void rectifyByPPU() {
            foreach (var ch in underlying.Values) {
                ch.CapsuleRadius /= DEFAULT_PIXELS_PER_UNIT;
                ch.CapsuleHalfHeight /= DEFAULT_PIXELS_PER_UNIT;
                ch.ShrinkedCapsuleRadius /= DEFAULT_PIXELS_PER_UNIT;
                ch.ShrinkedCapsuleHalfHeight /= DEFAULT_PIXELS_PER_UNIT;
                ch.LayDownCapsuleRadius /= DEFAULT_PIXELS_PER_UNIT;
                ch.LayDownCapsuleHalfHeight /= DEFAULT_PIXELS_PER_UNIT;
                ch.DyingCapsuleRadius /= DEFAULT_PIXELS_PER_UNIT;
                ch.DyingCapsuleHalfHeight /= DEFAULT_PIXELS_PER_UNIT;
                ch.DimmedCapsuleRadius /= DEFAULT_PIXELS_PER_UNIT;
                ch.DimmedCapsuleHalfHeight /= DEFAULT_PIXELS_PER_UNIT;
                
                ch.VisionOffsetX /= DEFAULT_PIXELS_PER_UNIT;
                ch.VisionOffsetY /= DEFAULT_PIXELS_PER_UNIT;
                ch.VisionHalfHeight /= DEFAULT_PIXELS_PER_UNIT;
                ch.VisionTopRadius /= DEFAULT_PIXELS_PER_UNIT;
                ch.VisionBottomRadius /= DEFAULT_PIXELS_PER_UNIT;
                ch.MaxAscendingVelY /= DEFAULT_PIXELS_PER_UNIT;
                ch.MinFallingVelY /= DEFAULT_PIXELS_PER_UNIT;
                ch.WallSlideVelY /= DEFAULT_PIXELS_PER_UNIT;
                ch.Speed /= DEFAULT_PIXELS_PER_UNIT;
                ch.WallJumpFreeSpeed /= DEFAULT_PIXELS_PER_UNIT;
                ch.AccMagX /= DEFAULT_PIXELS_PER_UNIT;
                ch.JumpAccMagY /= DEFAULT_PIXELS_PER_UNIT;
                ch.WallJumpAccMagX /= DEFAULT_PIXELS_PER_UNIT;
                ch.WallJumpAccMagY /= DEFAULT_PIXELS_PER_UNIT;

                ch.InplacePromptYOffset /= DEFAULT_PIXELS_PER_UNIT;
            }
        }
    }
}
