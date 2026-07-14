using Google.Protobuf.Collections;
using jtshared;
using System;

namespace JoltCSharp {
    public class PbPickables {

        private PrimitiveConsts primitiveConsts;
        public PbPickables(PrimitiveConsts primitiveConsts) {
            this.primitiveConsts = primitiveConsts;
        }

        protected MapField<uint, PickableConfig>? underlying;

        protected virtual bool lazyInit() {
            if (null != underlying) return true;

             PickableConfig HpSmall = new PickableConfig {
                 PickupType = primitiveConsts.Pkts.HpSmall,
                 TakesGravity = true,
                 ActiveAnimName = "HpSmall",
                 Amount1 = 10
             };

            PickableConfig MpSmall = new PickableConfig {
                PickupType = primitiveConsts.Pkts.MpSmall,
                TakesGravity = true,
                ActiveAnimName = "MpSmall",
                Amount1 = 10
            };

            PickableConfig InvCRefillSmall = new PickableConfig {
                PickupType = primitiveConsts.Pkts.InvCRefillSmall,
                TakesGravity = true,
                ActiveAnimName = "InvCRefillSmall",
                Amount1 = 1
            };

            PickableConfig InvDRefillSmall = new PickableConfig {
                PickupType = primitiveConsts.Pkts.InvDRefillSmall,
                TakesGravity = true,
                ActiveAnimName = "InvDRefillSmall",
                Amount1 = 1
            };

             PickableConfig Coin = new PickableConfig {
                 PickupType = primitiveConsts.Pkts.Coin,
                 TakesGravity = true,
                 ActiveAnimName = "Coin",
                 Amount1 = 1
             };

             PickableConfig DragonCrystal = new PickableConfig {
                 PickupType = primitiveConsts.Pkts.DragonCrystal,
                 TakesGravity = true,
                 ActiveAnimName = "DragonCrystal",
                 Amount1 = 1
             };

            underlying = new MapField<uint, PickableConfig> {
                { HpSmall.PickupType, HpSmall },
                { MpSmall.PickupType, MpSmall },
                { InvCRefillSmall.PickupType, InvCRefillSmall },
                { InvDRefillSmall.PickupType, InvDRefillSmall },
                { Coin.PickupType, Coin },
                { DragonCrystal.PickupType, DragonCrystal },
            };

            return true;
        }

        public MapField<uint, PickableConfig> getUnderlying() {
            if (!lazyInit() || null == underlying) {
                throw new ArgumentNullException("Failed to initialize the underlying of PbPickables");
            }
            return underlying;
        }
    }
}

