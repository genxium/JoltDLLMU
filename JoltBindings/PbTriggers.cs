using Google.Protobuf.Collections;
using jtshared;
using static JoltCSharp.PbPrimitives;

namespace JoltCSharp {
    public class PbTriggers {

        private static TriggerConfig VictoryTrigger = new TriggerConfig {
            Trt = PbPrimitives.underlying.TrtVictory
        };

        private static TriggerConfig IndiWaveNpcSpawner = new TriggerConfig {
            Trt = PbPrimitives.underlying.TrtIndiWaveNpcSpawner
        };

        public static MapField<uint, TriggerConfig> underlying = new MapField<uint, TriggerConfig>() { };

        static PbTriggers() {
            underlying.Add(PbPrimitives.underlying.TrtVictory, VictoryTrigger);
            underlying.Add(PbPrimitives.underlying.TrtIndiWaveNpcSpawner, IndiWaveNpcSpawner);
        }
    }
}
