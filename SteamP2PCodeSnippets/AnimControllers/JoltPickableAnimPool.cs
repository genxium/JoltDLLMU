using UnityEngine;
using jtshared;
using JoltCSharp;

public class JoltPickableAnimPool : AbstractCacheableAnimNodePool<Pickable, PickableState, PickableConfig, uint, JoltPickableAnimController> {

    protected Material sprDefaultMaterial;

    public JoltPickableAnimPool(in AbstractJoltMapController joltMap, in Material theSprDefaultMaterial) : base(joltMap, Bindings.APP_CalcPickableUserData(PbPrimitivesOverride.Instance.getUnderlying().TerminatingPickableId), PbPrimitivesOverride.Instance.getUnderlying().Pkts.None) {
        this.sprDefaultMaterial = theSprDefaultMaterial;
    }

    protected override GameObject loadPrefab(PickableConfig insConfig) {
        return joltMap.loadPickablePrefab(insConfig);
    }

    protected override JoltPickableAnimController CreateAnimNode(in uint cacheGroupId, in PickableConfig insConfig, in Transform parent, in int specifiedLayer = -1) {
        var g = base.CreateAnimNode(cacheGroupId, insConfig, parent, specifiedLayer);
        g.SetSprDefaultMaterial(sprDefaultMaterial);
        return g;
    }
}
