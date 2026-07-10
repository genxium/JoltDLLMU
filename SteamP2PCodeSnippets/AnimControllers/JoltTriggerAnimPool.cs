using UnityEngine;
using jtshared;
using JoltCSharp;

public class JoltTriggerAnimPool : AbstractCacheableAnimNodePool<Trigger, TriggerState, TriggerConfigFromTiled, uint, JoltTriggerAnimController> {

    protected Material sprDefaultMaterial;

    public JoltTriggerAnimPool(in AbstractJoltMapController joltMap, in Material theSprDefaultMaterial) : base(joltMap, Bindings.APP_CalcTriggerUserData(PbPrimitivesOverride.Instance.getUnderlying().TerminatingTriggerId), PbPrimitivesOverride.Instance.getUnderlying().Trts.None) {
        this.sprDefaultMaterial = theSprDefaultMaterial;
    }

    protected override GameObject loadPrefab(TriggerConfigFromTiled insConfig) {
        return joltMap.loadTriggerPrefab(insConfig);
    }

    protected override JoltTriggerAnimController CreateAnimNode(in uint cacheGroupId, in TriggerConfigFromTiled insConfig, in Transform parent, in int specifiedLayer = -1) {
        var g = base.CreateAnimNode(cacheGroupId, insConfig, parent, specifiedLayer);
        g.SetSprDefaultMaterial(sprDefaultMaterial);
        return g;
    }
}
