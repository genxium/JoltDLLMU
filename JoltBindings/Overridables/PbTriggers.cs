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

            TriggerConfig DelayTrigger = new TriggerConfig {
                Trt = primitiveConsts.Trts.ByInitDelay,
            };

            TriggerConfig ByMovementTrigger = new TriggerConfig {
                Trt = primitiveConsts.Trts.ByMovement,
                SubCycleDriven = true,
                Collidable = true,
            };

            TriggerConfig ByAttackTrigger = new TriggerConfig {
                Trt = primitiveConsts.Trts.ByAttack,
                Collidable = true,
            };

            TriggerConfig PatternFTrigger = new TriggerConfig {
                Trt = primitiveConsts.Trts.ByPatternF,
                Name = "PatternF",
                MixedCycle = true,
                Collidable = true,
            };

            TriggerConfig VictoryTrigger = new TriggerConfig {
                Trt = primitiveConsts.Trts.Victory,
                MixedCycle = true,
            };

            TriggerConfig IndiWaveNpcSpawner = new TriggerConfig {
                Trt = primitiveConsts.Trts.IndiWaveNpcSpawner,
                Name = "IndiWaveDoor1"
            };

            TriggerConfig IndiWavePickableSpawner = new TriggerConfig {
                Trt = primitiveConsts.Trts.IndiWavePickableSpawner,
            };

            TriggerConfig FlipFlopTrigger = new TriggerConfig {
                Trt = primitiveConsts.Trts.FlipFlop,
                Name = "FlipFlop",
                MixedCycle = true,
            };

            TriggerConfig SavePointTrigger = new TriggerConfig {
                Trt = primitiveConsts.Trts.SavePoint,
                MixedCycle = true,
            };

            underlying  = new MapField<uint, TriggerConfig> {
                { DelayTrigger.Trt, DelayTrigger },
                { ByMovementTrigger.Trt, ByMovementTrigger },
                { ByAttackTrigger.Trt, ByAttackTrigger },
                { PatternFTrigger.Trt, PatternFTrigger },
                { VictoryTrigger.Trt, VictoryTrigger },
                { IndiWaveNpcSpawner.Trt, IndiWaveNpcSpawner },
                { IndiWavePickableSpawner.Trt, IndiWavePickableSpawner },
                { FlipFlopTrigger.Trt, FlipFlopTrigger },
                { SavePointTrigger.Trt, SavePointTrigger }
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
