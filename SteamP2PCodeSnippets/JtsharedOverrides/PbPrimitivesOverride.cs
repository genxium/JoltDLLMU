

using System;
using static FrontendOnlyGeometry;

namespace JoltCSharp {
    public class PbPrimitivesOverride : PbPrimitives {

        /////////////////////////////////////////////////////////////////////////////////////////

        public static PbPrimitivesOverride Instance {
            get {
                if (null != _instance) {
                    return _instance;
                }
                lock (_padLock) {
                    if (null == _instance) {
                        _instance = new PbPrimitivesOverride();
                    }
                }
                return _instance;
            }
        }

        /////////////////////////////////////////////////////////////////////////////////////////
        private static PbPrimitivesOverride _instance = null;
        private static readonly object _padLock = new object();

        private bool _initialized = false;
        protected override bool lazyInit() {
            if (_initialized) {
                return true;
            }
            bool res = base.lazyInit();
            //underlying.CrouchForcingCeilingDotThreshold = MathF.Cos((float)(Math.PI/8));
            underlying.CrouchForcingCeilingDotThreshold = MathF.Cos((float)(Math.PI / 2 - Math.PI / 720));
            underlying.InputScaleFrames = 2;
            underlying.InputScale = (1 << underlying.InputScaleFrames);
            underlying.DefaultFleeingGracePeriodRdfCnt = 2 * BATTLE_DYNAMICS_FPS;

            underlying.DefaultPickableHurtboxHalfSizeX = 10f;
            underlying.DefaultPickableHurtboxHalfSizeY = 12f;
            underlying.DefaultPickableDisappearingAnimFrames = 45;
            underlying.DefaultPickableConsumedAnimFrames = 45;
            underlying.DefaultPickableRisingVelY = 8f * BATTLE_DYNAMICS_FPS;
            underlying.DefaultPickableStartupFrames = 45;
            underlying.DefaultPickableLifetimeRdfCnt = 60 * BATTLE_DYNAMICS_FPS;

            underlying.DefaultPhySysSubstepCnt = 4; // higher than the default (i.e. 1)
            underlying.DefaultPhySysNumVelSteps = 10u; // Roughly the default (i.e. 10)
            underlying.DefaultPhySysNumPosSteps = 2u; // Roughly the default (i.e. 2)

            //underlying.DefaultPhySysSubstepCnt = 1; // higher than the default (i.e. 1)
            //underlying.DefaultPhySysNumVelSteps = 10u; // Roughly the default (i.e. 10)
            //underlying.DefaultPhySysNumPosSteps = 2u; // Roughly the default (i.e. 2)

            underlying.DefaultChFriction = 0.05f;
            underlying.DefaultChRestitution = 0.0f;
            underlying.GroundDashingChFriction = 0.001f;
            underlying.AntiPushChFriction = 0.5f;
            underlying.WalkstoppingChFriction = 125.0f;
            underlying.FallstoppingChFriction = 90.0f;

            underlying.BaumgarteFactor = 0.5f; // Roughly the default (i.e. 0.2), set higher for faster contact non-penetration constraint restitution
            underlying.PenetrationSlop = 0.02f; // Roughly the default (i.e. 0.02), set higher for tolerating deeper penetrations of falling bricks
            underlying.MaxPenetrationDistance = 0.2f; // Roughly the default (i.e. 0.2)

            underlying.DefaultBarrierFriction = 0.2f;
            underlying.DefaultBarrierRestitution = 0f;

            underlying.DefaultBulletFriction = 0.2f;
            underlying.DefaultBulletRestitution = 0f;

            underlying.DefaultSlipJumpGracePeriodRdfCnt = 15;
            rectifyByPPU();
            _initialized = true;
            return _initialized;
        }

        private void rectifyByPPU() {
            float frictionScaleFactor = 4.0f / DEFAULT_PIXELS_PER_UNIT;
            //underlying.DefaultBarrierFriction *= frictionScaleFactor;
            underlying.DefaultChFriction *= frictionScaleFactor;
            underlying.WalkstoppingChFriction *= (8*frictionScaleFactor);
            underlying.FallstoppingChFriction *= (8*frictionScaleFactor);
            underlying.AntiPushChFriction *= (8*frictionScaleFactor);
            underlying.GroundDashingChFriction *= frictionScaleFactor;
            underlying.DefaultBulletFriction *= frictionScaleFactor;
            underlying.DefaultTrapFriction *= frictionScaleFactor;

            underlying.GravityY /= DEFAULT_PIXELS_PER_UNIT;
            underlying.GravityYJumpHolding /= DEFAULT_PIXELS_PER_UNIT;

            underlying.DefaultPickableHurtboxHalfSizeX /= DEFAULT_PIXELS_PER_UNIT;
            underlying.DefaultPickableHurtboxHalfSizeY /= DEFAULT_PIXELS_PER_UNIT;
            underlying.DefaultPickableRisingVelY /= DEFAULT_PIXELS_PER_UNIT;
            underlying.StickToGroundCorrectionLength /= DEFAULT_PIXELS_PER_UNIT;
            underlying.MaxPenetrationDistance /= DEFAULT_PIXELS_PER_UNIT;
            underlying.SpeculativeContactDistance /= DEFAULT_PIXELS_PER_UNIT;

            underlying.DefaultHalfThickness /= DEFAULT_PIXELS_PER_UNIT;
            underlying.DefaultBarrierHalfThickness /= DEFAULT_PIXELS_PER_UNIT;
            underlying.DefaultBlHalfLength/= DEFAULT_PIXELS_PER_UNIT;
            underlying.DefaultTpHalfLength/= DEFAULT_PIXELS_PER_UNIT;

            underlying.DefaultAimingRayLength /= DEFAULT_PIXELS_PER_UNIT;
            
            /*
            [REMINDER] Intentionally NOT scaling the following parameters because they're already small enough to match the "magic const sweet spot" of Jolt Physics engine.
            
            underlying.CollisionTolerance /= DEFAULT_PIXELS_PER_UNIT;
            underlying.PenetrationSlop /= DEFAULT_PIXELS_PER_UNIT;
            underlying.LengthEps /= DEFAULT_PIXELS_PER_UNIT;
            underlying.LengthEpsSquared /= (DEFAULT_PIXELS_PER_UNIT * DEFAULT_PIXELS_PER_UNIT);
            underlying.LengthNearlySameEps /= DEFAULT_PIXELS_PER_UNIT;
            underlying.LengthNearlySameEpsSquared /= (DEFAULT_PIXELS_PER_UNIT * DEFAULT_PIXELS_PER_UNIT);
            */
        }
    }
}
