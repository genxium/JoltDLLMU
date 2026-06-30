using UnityEngine;
using jtshared;
using JoltCSharp;

public class JoltCharacterAnimPool : AbstractCacheableAnimNodePool<CharacterDownsync, CharacterState, CharacterConfig, uint, JoltCharacterAnimController> {

    public JoltCharacterAnimPool(in AbstractJoltMapController joltMap) : base(joltMap, Bindings.APP_CalcNpcUserData(PbPrimitivesOverride.Instance.getUnderlying().TerminatingCharacterId), PbPrimitivesOverride.Instance.getUnderlying().ChSpecies.None) {
    }

    protected override GameObject loadPrefab(CharacterConfig insConfig) {
        return joltMap.loadCharacterPrefab(insConfig);
    }
}
