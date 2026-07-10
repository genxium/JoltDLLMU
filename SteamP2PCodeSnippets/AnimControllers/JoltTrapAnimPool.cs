using UnityEngine;
using jtshared;
using JoltCSharp;

public class JoltTrapAnimPool : AbstractCacheableAnimNodePool<Trap, TrapState, TrapConfig, uint, JoltTrapAnimController> {

    protected Material sprDefaultMaterial;

    public JoltTrapAnimPool(in AbstractJoltMapController joltMap, in Material theSprDefaultMaterial) : base(joltMap, Bindings.APP_CalcTrapUserData(PbPrimitivesOverride.Instance.getUnderlying().TerminatingTrapId), PbPrimitivesOverride.Instance.getUnderlying().Tpts.None) {
        this.sprDefaultMaterial = theSprDefaultMaterial;
    }

    protected override GameObject loadPrefab(TrapConfig insConfig) {
        return joltMap.loadTrapPrefab(insConfig);
    }

    protected override JoltTrapAnimController CreateAnimNode(in uint cacheGroupId, in TrapConfig insConfig, in Transform parent, in int specifiedLayer = -1) {
        var g = base.CreateAnimNode(cacheGroupId, insConfig, parent, specifiedLayer);
        g.SetSprDefaultMaterial(sprDefaultMaterial);
        return g;
    }
}