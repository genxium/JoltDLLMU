using jtshared;
using static FrontendOnlyGeometry;

namespace JoltCSharp {
    public class PbTriggersOverride : PbTriggers {
        ////////////////////////////////////////////////////////////////////////////////////////
        public PbTriggersOverride(in PrimitiveConsts primitiveConsts) : base(primitiveConsts) {
        }
        
        public static PbTriggersOverride Instance {
            get {
                if (null != _instance) {
                    return _instance;
                }
                lock (_padLock) {
                    if (null == _instance) {
                        _instance = new PbTriggersOverride(PbPrimitivesOverride.Instance.getUnderlying());
                    }
                }
                return _instance;
            }
        }

        ////////////////////////////////////////////////////////////////////////////////////////
        private static PbTriggersOverride _instance = null;
        private static readonly object _padLock = new object();
        private bool _initialized = false;
        protected override bool lazyInit() {
            if (_initialized) {
                return true;
            }
            base.lazyInit();
            underlying[PbPrimitivesOverride.TRT_INDI_WAVE_NPC_SPAWNER].Name = "IndiWaveDoor1";
            underlying[PbPrimitivesOverride.TRT_INDI_WAVE_PICKABLE_SPAWNER].Name = "IndiWavePickableChest1";

           
            if (underlying.ContainsKey(PbPrimitivesOverride.TRT_BY_ATTACK)) {
                underlying[PbPrimitivesOverride.TRT_BY_ATTACK].Name = "ByAttack";
            } else {
                underlying[PbPrimitivesOverride.TRT_BY_ATTACK] = new TriggerConfig {
                    Trt = PbPrimitivesOverride.TRT_BY_ATTACK,
                    Name = "ByAttack"
                };
            }

            rectifyByPPU();

            _initialized = true;
            return _initialized;
        }

        private void rectifyByPPU() {
            foreach (var trigger in underlying.Values) {
                // By now triggers have no size properties that need to be adjusted by PPU
            }
        }
    }
}
