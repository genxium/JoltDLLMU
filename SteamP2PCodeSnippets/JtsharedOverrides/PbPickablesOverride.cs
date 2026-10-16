using jtshared;

namespace JoltCSharp {
    public partial class PbPickablesOverride : PbPickables {
        /////////////////////////////////////////////////////////////////////////////////////////
        public PbPickablesOverride(PrimitiveConsts primitiveConsts) : base(primitiveConsts) {
        }

        public static PbPickablesOverride Instance {
            get {
                if (null != _instance) {
                    return _instance;
                }
                lock (_padLock) {
                    if (null == _instance) {
                        _instance = new PbPickablesOverride(PbPrimitivesOverride.Instance.getUnderlying());
                    }
                }
                return _instance;
            }
        }

        /////////////////////////////////////////////////////////////////////////////////////////
        private static PbPickablesOverride _instance = null;
        private static readonly object _padLock = new object();
    }
}

