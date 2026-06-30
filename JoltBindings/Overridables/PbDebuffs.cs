using Google.Protobuf.Collections;
using jtshared;
using System;

namespace JoltCSharp {
    public class PbDebuffs {
        private PrimitiveConsts primitiveConsts;
        public PbDebuffs(PrimitiveConsts primitiveConsts) {
            this.primitiveConsts = primitiveConsts;
        }

        protected MapField<uint, DebuffConfig>? underlying;

        protected virtual bool lazyInit() {
            if (null != underlying) return true;
            DebuffConfig ShortFrozen = new DebuffConfig {
                SpeciesId = primitiveConsts.DebuffSpecies.ShortFrozen,
                StockType = BuffStockType.Timed,
                Stock = 2 * primitiveConsts.BattleDynamicsFps,
                Type = DebuffType.FrozenPositionLocked,
                ArrIdx = primitiveConsts.DebuffArrayIdxElemental
            };

            DebuffConfig ShortParalyzed = new DebuffConfig {
                SpeciesId = primitiveConsts.DebuffSpecies.ShortParalyzed,
                StockType = BuffStockType.Timed,
                Stock = 3 * primitiveConsts.BattleDynamicsFps,
                Type = DebuffType.PositionLockedOnly,
                ArrIdx = primitiveConsts.DebuffArrayIdxElemental
            };

            DebuffConfig LongFrozen = new DebuffConfig {
                SpeciesId = primitiveConsts.DebuffSpecies.LongFrozen,
                StockType = BuffStockType.Timed,
                Stock = (int)(3.5f * primitiveConsts.BattleDynamicsFps),
                Type = DebuffType.FrozenPositionLocked,
                ArrIdx = primitiveConsts.DebuffArrayIdxElemental
            };

            DebuffConfig LongParalyzed = new DebuffConfig {
                SpeciesId = primitiveConsts.DebuffSpecies.LongParalyzed,
                StockType = BuffStockType.Timed,
                Stock = 5 * primitiveConsts.BattleDynamicsFps,
                Type = DebuffType.PositionLockedOnly,
                ArrIdx = primitiveConsts.DebuffArrayIdxElemental
            };

            underlying = new MapField<uint, DebuffConfig> {
            { ShortFrozen.SpeciesId, ShortFrozen },
            { ShortParalyzed.SpeciesId, ShortParalyzed },
            { LongFrozen.SpeciesId, LongFrozen },
            { LongParalyzed.SpeciesId, LongParalyzed }
        };

            return true;
        }

        public MapField<uint, DebuffConfig> getUnderlying() {
            if (!lazyInit() || null == underlying) {
                throw new ArgumentNullException("Failed to initialize the underlying of PbDebuffs");
            }
            return underlying;
        }
    }
}
