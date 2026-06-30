using UnityEngine;
using jtshared;
using JoltCSharp;

public class JoltInplaceHpBarAnimPool : AbstractCacheableAnimNodePool<CharacterDownsync, CharacterState, CharacterConfig, uint, JoltInplaceHpBarAnimController> {

    public JoltInplaceHpBarAnimPool(in AbstractJoltMapController joltMap) : base(joltMap, Bindings.APP_CalcNpcUserData(PbPrimitivesOverride.Instance.getUnderlying().TerminatingCharacterId), PbPrimitivesOverride.Instance.getUnderlying().ChSpecies.None) {
    }

    protected override GameObject loadPrefab(CharacterConfig insConfig) {
        return joltMap.loadInplaceHpBarPrefab(insConfig);
    }
}