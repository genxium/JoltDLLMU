using UnityEngine;
using jtshared;
using JoltCSharp;

public class JoltTriggerAnimPool : AbstractCacheableAnimNodePool<Trigger, TriggerState, TriggerConfigFromTiled, uint, JoltTriggerAnimController> {

    public JoltTriggerAnimPool(in AbstractJoltMapController joltMap) : base(joltMap, Bindings.APP_CalcTriggerUserData(PbPrimitivesOverride.Instance.getUnderlying().TerminatingTriggerId), PbPrimitivesOverride.Instance.getUnderlying().Tpts.None) {
    }

    protected override GameObject loadPrefab(TriggerConfigFromTiled insConfig) {
        return joltMap.loadTriggerPrefab(insConfig);
    }
}
