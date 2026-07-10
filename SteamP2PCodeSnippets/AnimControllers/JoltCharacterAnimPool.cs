using UnityEngine;
using jtshared;
using JoltCSharp;

public class JoltCharacterAnimPool : AbstractCacheableAnimNodePool<CharacterDownsync, CharacterState, CharacterConfig, uint, JoltCharacterAnimController> {

    protected Material sprDefaultMaterial;

    public JoltCharacterAnimPool(in AbstractJoltMapController joltMap, in Material theSprDefaultMaterial) : base(joltMap, Bindings.APP_CalcNpcUserData(PbPrimitivesOverride.Instance.getUnderlying().TerminatingCharacterId), PbPrimitivesOverride.Instance.getUnderlying().ChSpecies.None) {
        this.sprDefaultMaterial = theSprDefaultMaterial;
    }

    protected override GameObject loadPrefab(CharacterConfig insConfig) {
        return joltMap.loadCharacterPrefab(insConfig);
    }

    protected override JoltCharacterAnimController CreateAnimNode(in uint cacheGroupId, in CharacterConfig insConfig, in Transform parent, in int specifiedLayer = -1) {
        var g = base.CreateAnimNode(cacheGroupId, insConfig, parent, specifiedLayer);
        g.SetSprDefaultMaterial(sprDefaultMaterial);
        return g;
    }
}
