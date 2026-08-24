using JoltCSharp;
using jtshared;
using UnityEngine;

public class JoltAimingRayAnimPool : AbstractCacheableAnimNodePool<AimingRay, uint, uint, uint, JoltAimingRayAnimController> {

    public JoltAimingRayAnimPool(in AbstractJoltMapController joltMap) : base(joltMap, Bindings.APP_CalcNpcUserData(PbPrimitivesOverride.Instance.getUnderlying().TerminatingCharacterId), PbPrimitivesOverride.Instance.getUnderlying().ChSpecies.None) {
    }

    protected override GameObject loadPrefab(uint ignored1) {
        return joltMap.loadAimingRayPrefab();
    }
}
