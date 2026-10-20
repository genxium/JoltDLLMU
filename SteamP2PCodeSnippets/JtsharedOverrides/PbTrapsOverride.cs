using jtshared;
using static FrontendOnlyGeometry;

namespace JoltCSharp {
    public class PbTrapsOverride : PbTraps {
        ////////////////////////////////////////////////////////////////////////////////////////

        public PbTrapsOverride(in PrimitiveConsts primitiveConsts) : base(primitiveConsts) {
        }
        public static PbTrapsOverride Instance {
            get {
                if (null != _instance) {
                    return _instance;
                }
                lock (_padLock) {
                    if (null == _instance) {
                        _instance = new PbTrapsOverride(PbPrimitivesOverride.Instance.getUnderlying());
                    }
                }
                return _instance;
            }
        }

        ////////////////////////////////////////////////////////////////////////////////////////
        private static PbTrapsOverride _instance = null;
        private static readonly object _padLock = new object();

        private bool _initialized = false;
        protected override bool lazyInit() {
            if (_initialized) {
                return true;
            }
            bool res = base.lazyInit();
            var SliderPlatformTrap = underlying[PbPrimitivesOverride.Instance.getUnderlying().Tpts.SlidingPlatform];
            SliderPlatformTrap.Friction = 0.5f;

            rectifyByPPU();

            _initialized = true;
            return _initialized;
        }

        private void rectifyByPPU() {
            foreach (var trap in underlying.Values) {
                trap.DefaultBoxHalfSizeX /= DEFAULT_PIXELS_PER_UNIT;
                trap.DefaultBoxHalfSizeY /= DEFAULT_PIXELS_PER_UNIT;


                /*
                [REMINDER] Intentionally NOT scaling the following parameters because they're used in special cases where length scaling is not applicable. 

                [TODO]
                trap.Friction /= DEFAULT_PIXELS_PER_UNIT;
                */
            }
        }
    }
}
