using jtshared;
using UnityEngine;

public class SfxSourceAnimController : AbstractCacheableAnimNode<CharacterDownsync, CharacterState, CharacterConfig, uint> {

    public SfxSourceAnimController() {
        SetUd(0);
        SetCacheGroupId(0);
    }

    protected new Animator getMainAnimator() {
        return null;
    }

    protected override bool lazyInit() {
        return true;
    }

    protected override bool updateAnimUnderlying(in int currRdfId, in CharacterDownsync chd, in CharacterState newState, in CharacterConfig insConfig, in int frameIdxInAnim) {
        SetCacheGroupId(insConfig.SpeciesId);
        return true;
    }
}
