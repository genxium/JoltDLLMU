using Google.Protobuf.Collections;
using jtshared;
using static JoltCSharp.PbPrimitives;

namespace JoltCSharp {
    public class PbBuffs {

        public static BuffConfig ShortFreezer = new BuffConfig {
            SpeciesId = 1,
            StockType = BuffStockType.Timed,
            Stock = 480,
            XformChSpeciesId = PbPrimitives.SPECIES_NONE_CH,
        }.AddAssociatedDebuff(PbDebuffs.ShortFrozen.SpeciesId);

        public static MapField<uint, BuffConfig> underlying = new MapField<uint, BuffConfig>() { };

        static PbBuffs() {
            underlying.Add(ShortFreezer.SpeciesId, ShortFreezer);
        }
    }
}
