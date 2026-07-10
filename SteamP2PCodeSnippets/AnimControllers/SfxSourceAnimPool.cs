using jtshared;
using UnityEngine;

public class SfxSourceAnimPool : AbstractCacheableAnimNodePool<CharacterDownsync, CharacterState, CharacterConfig, uint, SfxSourceAnimController> {

    public SfxSourceAnimPool(in AbstractJoltMapController joltMap) : base(joltMap, 0, 0) {
    }

    protected override GameObject loadPrefab(CharacterConfig insConfig) {
        return joltMap.loadSfxSourcePrefab();
    }
}
