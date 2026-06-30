using Google.Protobuf.Collections;
using jtshared;
using System;
using static JoltCSharp.PbPrimitives;

namespace JoltCSharp {
    public class PbBuffs {

        private PrimitiveConsts primitiveConsts;

        public PbBuffs(PrimitiveConsts primitiveConsts) {
            this.primitiveConsts = primitiveConsts;

        }

        protected MapField<uint, BuffConfig>? underlying;

        protected virtual bool lazyInit() {
            if (null != underlying) return true;

            BuffConfig ShortFreezer = new BuffConfig {
                SpeciesId = 1,
                StockType = BuffStockType.Timed,
                Stock = 480,
                XformChSpeciesId = primitiveConsts.ChSpecies.None,
            }.AddAssociatedDebuff(primitiveConsts.DebuffSpecies.ShortFrozen);
            
            underlying = new MapField<uint, BuffConfig>() { };
            underlying.Add(ShortFreezer.SpeciesId, ShortFreezer);
            return true;
        }
    
        public MapField<uint, BuffConfig> getUnderlying() {
            if (!lazyInit() || null == underlying) {
                throw new ArgumentNullException("Failed to initialize the underlying of PbBuffs");
            }

            return underlying;
        }
    }
}
