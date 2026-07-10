using UnityEngine;
using jtshared;
using JoltCSharp;

public class JoltBulletAnimPool : AbstractCacheableAnimNodePool<Bullet, BulletState, BulletConfig, string, JoltBulletAnimController> {

    protected Material sprDefaultMaterial;

    public JoltBulletAnimPool(in AbstractJoltMapController joltMap, in Material theSprDefaultMaterial) : base(joltMap, Bindings.APP_CalcBulletUserData(PbPrimitivesOverride.Instance.getUnderlying().TerminatingBulletId), "") {
        this.sprDefaultMaterial = theSprDefaultMaterial;
    }

    protected override GameObject loadPrefab(BulletConfig insConfig) {
        return joltMap.loadBulletPrefab(insConfig);
    }

    protected override JoltBulletAnimController CreateAnimNode(in string cacheGroupId, in BulletConfig insConfig, in Transform parent, in int specifiedLayer = -1) {
        var g = base.CreateAnimNode(cacheGroupId, insConfig, parent, specifiedLayer);
        g.SetSprDefaultMaterial(sprDefaultMaterial);
        return g;
    }
}
