using jtshared;

namespace JoltCSharp {
    public partial class PbDebuffsOverride : PbDebuffs {
        /////////////////////////////////////////////////////////////////////////////////////////
        public PbDebuffsOverride(PrimitiveConsts primitiveConsts) : base(primitiveConsts) {
        }

        public static PbDebuffsOverride Instance {
            get {
                if (null != _instance) {
                    return _instance;
                }
                lock (_padLock) {
                    if (null == _instance) {
                        _instance = new PbDebuffsOverride(PbPrimitivesOverride.Instance.getUnderlying());
                    }
                }
                return _instance;
            }
        }

        /////////////////////////////////////////////////////////////////////////////////////////
        private static PbDebuffsOverride _instance = null;
        private static readonly object _padLock = new object();
    }
}
