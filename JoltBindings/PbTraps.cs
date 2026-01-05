using Google.Protobuf.Collections;
using jtshared;
using static JoltCSharp.PbPrimitives;

namespace JoltCSharp {
    public class PbTraps {

        private static TrapConfig SlidingPlatformTrap = new TrapConfig {
            Tpt = PbPrimitives.underlying.TptSlidingPlatform,
            NoXFlipRendering = true,
            DefaultBoxHalfSizeX = 100.0f,
            DefaultBoxHalfSizeY = 100.0f,
            DefaultLinearSpeed = 7.0f*BATTLE_DYNAMICS_FPS, 
            DefaultCooldownRdfCount = 60
        };

        public static MapField<uint, TrapConfig> underlying = new MapField<uint, TrapConfig>() { };

        static PbTraps() {
            underlying.Add(PbPrimitives.underlying.TptSlidingPlatform, SlidingPlatformTrap);
        }
    }
}
