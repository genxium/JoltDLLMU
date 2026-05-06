using Google.Protobuf.Collections;
using jtshared;
using static JoltCSharp.PbPrimitives;
using static jtshared.PbBuilders;

namespace JoltCSharp {
    public partial class PbPickables {

        public static PickableConfig HpSmall = new PickableConfig {
            PickupType = PbPrimitives.underlying.PktHpSmall,
            TakesGravity = true,
            ActiveAnimName = "HpSmall",
            Amount1 = 10 
        };

        public static PickableConfig MpSmall = new PickableConfig {
            PickupType = PbPrimitives.underlying.PktMpSmall,
            TakesGravity = true,
            ActiveAnimName = "MpSmall", 
            Amount1 = 10 
        };

        public static PickableConfig InvCRefillSmall = new PickableConfig {
            PickupType = PbPrimitives.underlying.PktInvCRefillSmall,
            TakesGravity = true,
            ActiveAnimName = "InvCRefillSmall", 
            Amount1 = 1 
        };

        public static PickableConfig InvDRefillSmall = new PickableConfig {
            PickupType = PbPrimitives.underlying.PktInvDRefillSmall,
            TakesGravity = true,
            ActiveAnimName = "InvDRefillSmall",
            Amount1 = 1 
        };

        public static MapField<uint, PickableConfig> underlying = new MapField<uint, PickableConfig>() { };

        static PbPickables() {
            underlying.Add(HpSmall.PickupType, HpSmall);
            underlying.Add(MpSmall.PickupType, MpSmall);
            underlying.Add(InvCRefillSmall.PickupType, InvCRefillSmall);
            underlying.Add(InvDRefillSmall.PickupType, InvDRefillSmall);
        }
    }
}

