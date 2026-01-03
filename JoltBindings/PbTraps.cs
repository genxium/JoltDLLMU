using Google.Protobuf.Collections;
using jtshared;
using static JoltCSharp.PbPrimitives;

namespace JoltCSharp {
    public class PbTraps {

        private static TrapConfig SlidingPlatformTrap = new TrapConfig {
            Tpt = PbPrimitives.underlying.TptSlidingPlatform
        };

        public static MapField<uint, TrapConfig> underlying = new MapField<uint, TrapConfig>() { };

        static PbTraps() {
            underlying.Add(PbPrimitives.underlying.TptSlidingPlatform, SlidingPlatformTrap);
        }
    }
}
