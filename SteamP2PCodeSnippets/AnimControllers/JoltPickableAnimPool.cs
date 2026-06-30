using UnityEngine;
using jtshared;
using JoltCSharp;

public class JoltPickableAnimPool : AbstractCacheableAnimNodePool<Pickable, PickableState, PickableConfig, uint, JoltPickableAnimController> {

    public JoltPickableAnimPool(in AbstractJoltMapController joltMap) : base(joltMap, Bindings.APP_CalcPickableUserData(PbPrimitivesOverride.Instance.getUnderlying().TerminatingPickableId), PbPrimitivesOverride.Instance.getUnderlying().Pkts.None) {
    }

    protected override GameObject loadPrefab(PickableConfig insConfig) {
        return joltMap.loadPickablePrefab(insConfig);
    }
}
