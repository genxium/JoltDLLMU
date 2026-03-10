using Google.Protobuf.Collections;
using jtshared;
using static JoltCSharp.PbPrimitives;

namespace JoltCSharp {
    public class PbDebuffs {

        public static DebuffConfig ShortFrozen = new DebuffConfig {
            SpeciesId = 1,
            StockType = BuffStockType.Timed,
            Stock = 2*PbPrimitives.BATTLE_DYNAMICS_FPS,
            Type = DebuffType.FrozenPositionLocked,
            ArrIdx = PbPrimitives.DEBUFF_ARR_IDX_ELEMENTAL
        };

        public static DebuffConfig ShortParalyzed = new DebuffConfig {
            SpeciesId = 2,
            StockType = BuffStockType.Timed,
            Stock = 3*PbPrimitives.BATTLE_DYNAMICS_FPS,
            Type = DebuffType.PositionLockedOnly,
            ArrIdx = PbPrimitives.DEBUFF_ARR_IDX_ELEMENTAL
        };

        public static DebuffConfig LongFrozen = new DebuffConfig {
            SpeciesId = 3,
            StockType = BuffStockType.Timed,
            Stock = (int)(3.5f*PbPrimitives.BATTLE_DYNAMICS_FPS),
            Type = DebuffType.FrozenPositionLocked,
            ArrIdx = PbPrimitives.DEBUFF_ARR_IDX_ELEMENTAL
        };

        public static DebuffConfig LongParalyzed = new DebuffConfig {
            SpeciesId = 4,
            StockType = BuffStockType.Timed,
            Stock = 5*PbPrimitives.BATTLE_DYNAMICS_FPS,
            Type = DebuffType.PositionLockedOnly,
            ArrIdx = PbPrimitives.DEBUFF_ARR_IDX_ELEMENTAL
        };

        public static MapField<uint, DebuffConfig> underlying = new MapField<uint, DebuffConfig>() { };

        static PbDebuffs() {
            underlying.Add(ShortFrozen.SpeciesId, ShortFrozen);
            underlying.Add(ShortParalyzed.SpeciesId, ShortParalyzed);
            underlying.Add(LongFrozen.SpeciesId, LongFrozen);
            underlying.Add(LongParalyzed.SpeciesId, LongParalyzed);
        }
    }
}
