using jtshared;
using UnityEngine;

public class SFXSourceAnimPool : AbstractCacheableAnimNodePool<Bullet, BulletState, BulletConfig, string, SFXSourceAnimController> {

    public SFXSourceAnimPool(in AbstractJoltMapController joltMap) : base(joltMap, 0, "") {
    }

    protected override GameObject loadPrefab(BulletConfig insConfig) {
        return joltMap.loadSfxSourcePrefab();
    }

    public string CalcSfxName(in Bullet bullet, in BulletConfig bulletConfig) {
        switch (bullet.BlState) {
            case BulletState.StartUp:
                switch (bulletConfig.BType) {
                    case BulletType.Melee:
                        return bulletConfig.CharacterEmitSfxName;
                    default:
                        return bulletConfig.FireballEmitSfxName;
                }
            case BulletState.Hit:
                return bulletConfig.HitSfxName;
            default:
                return "";
        }
    }
}
