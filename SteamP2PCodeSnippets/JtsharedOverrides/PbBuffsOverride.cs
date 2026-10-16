using jtshared;

namespace JoltCSharp {
    public class PbBuffsOverride : PbBuffs {
        /////////////////////////////////////////////////////////////////////////////////////////
        public PbBuffsOverride(PrimitiveConsts primitiveConsts) : base(primitiveConsts) {
        }

        public static PbBuffsOverride Instance {
            get {
                if (null != _instance) {
                    return _instance;
                }
                lock (_padLock) {
                    if (null == _instance) {
                        _instance = new PbBuffsOverride(PbPrimitivesOverride.Instance.getUnderlying());
                    }
                }
                return _instance;
            }
        }

        /////////////////////////////////////////////////////////////////////////////////////////
        private static PbBuffsOverride _instance = null;
        private static readonly object _padLock = new object();
    }
}
