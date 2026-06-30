using UnityEngine;
using jtshared;
using JoltCSharp;

public class JoltTrapAnimPool : AbstractCacheableAnimNodePool<Trap, TrapState, TrapConfig, uint, JoltTrapAnimController> {

    public JoltTrapAnimPool(in AbstractJoltMapController joltMap) : base(joltMap, Bindings.APP_CalcTrapUserData(PbPrimitivesOverride.Instance.getUnderlying().TerminatingTrapId), PbPrimitivesOverride.Instance.getUnderlying().Tpts.None) {
    }

    protected override GameObject loadPrefab(TrapConfig insConfig) {
        return joltMap.loadTrapPrefab(insConfig);
    }
}