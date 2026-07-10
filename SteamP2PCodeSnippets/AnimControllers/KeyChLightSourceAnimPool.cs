using jtshared;
using UnityEngine;

public class KeyChLightSourceAnimPool : AbstractCacheableAnimNodePool<CharacterDownsync, CharacterState, CharacterConfig, uint, KeyChLightSourceAnimController> {

    public KeyChLightSourceAnimPool(in AbstractJoltMapController joltMap) : base(joltMap, 0, 0) {
    }

    protected override GameObject loadPrefab(CharacterConfig insConfig) {
        return joltMap.loadKeyChLightSource2DPrefab();
    }
}
