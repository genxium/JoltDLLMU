using UnityEngine;
using jtshared;
using JoltCSharp;

public class JoltBulletAnimPool : AbstractCacheableAnimNodePool<Bullet, BulletState, BulletConfig, string, JoltBulletAnimController> {

    public JoltBulletAnimPool(in AbstractJoltMapController joltMap) : base(joltMap, Bindings.APP_CalcBulletUserData(PbPrimitivesOverride.Instance.getUnderlying().TerminatingBulletId), "") {
    }

    protected override GameObject loadPrefab(BulletConfig insConfig) {
        return joltMap.loadBulletPrefab(insConfig);
    }
}
