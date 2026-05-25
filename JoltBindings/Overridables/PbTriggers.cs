using Google.Protobuf.Collections;
using jtshared;
using System;

namespace JoltCSharp {
    public class PbTriggers {
        private PrimitiveConsts primitiveConsts;
        public PbTriggers(in PrimitiveConsts primitiveConsts) {
            this.primitiveConsts = primitiveConsts;
        }

        protected MapField<uint, TriggerConfig>? underlying;
        protected virtual bool lazyInit() {
            if (null != underlying) return true;
            TriggerConfig PatternFTrigger = new TriggerConfig {
                Trt = primitiveConsts.Trts.ByPatternF,
                Name = "PatternF"
            };

            TriggerConfig VictoryTrigger = new TriggerConfig {
                Trt = primitiveConsts.Trts.Victory
            };

            TriggerConfig IndiWaveNpcSpawner = new TriggerConfig {
                Trt = primitiveConsts.Trts.IndiWaveNpcSpawner,
                Name = "IndiWaveDoor1"
            };

            TriggerConfig IndiWavePickableSpawner = new TriggerConfig {
                Trt = primitiveConsts.Trts.IndiWavePickableSpawner,
            };

            underlying  = new MapField<uint, TriggerConfig> {
                { PatternFTrigger.Trt, PatternFTrigger },
                { VictoryTrigger.Trt, VictoryTrigger },
                { IndiWaveNpcSpawner.Trt, IndiWaveNpcSpawner },
                { IndiWavePickableSpawner.Trt, IndiWavePickableSpawner }
            };

            return true;
       
        }

        public MapField<uint, TriggerConfig> getUnderlying() {
            if (!lazyInit() || null == underlying) {
                throw new ArgumentNullException("Failed to initialize the underlying of PbTriggers");
            }
            return underlying;
        }
    }
}
