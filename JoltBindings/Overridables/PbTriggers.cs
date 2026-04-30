using Google.Protobuf.Collections;
using jtshared;
using static JoltCSharp.PbPrimitives;

namespace JoltCSharp {
    public partial class PbTriggers {

        public static TriggerConfig PatternFTrigger = new TriggerConfig {
            Trt = PbPrimitives.underlying.TrtByPatternF,
            Name = "PatternF"
        };

        public static TriggerConfig VictoryTrigger = new TriggerConfig {
            Trt = PbPrimitives.underlying.TrtVictory
        };

        public static TriggerConfig IndiWaveNpcSpawner = new TriggerConfig {
            Trt = PbPrimitives.underlying.TrtIndiWaveNpcSpawner,
            Name = "IndiWaveDoor1"
        };

        public static TriggerConfig IndiWavePickableSpawner = new TriggerConfig {
            Trt = PbPrimitives.underlying.TrtIndiWavePickableSpawner,
        };

        public static MapField<uint, TriggerConfig> underlying = new MapField<uint, TriggerConfig>() { };

        static PbTriggers() {
            underlying.Add(PbPrimitives.underlying.TrtByPatternF, PatternFTrigger);
            underlying.Add(PbPrimitives.underlying.TrtVictory, VictoryTrigger);
            underlying.Add(PbPrimitives.underlying.TrtIndiWaveNpcSpawner, IndiWaveNpcSpawner);
            underlying.Add(PbPrimitives.underlying.TrtIndiWavePickableSpawner, IndiWavePickableSpawner);
        }
    }
}
