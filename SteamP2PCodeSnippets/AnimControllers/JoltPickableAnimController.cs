using System.Collections.Generic;
using UnityEngine;
using jtshared;
using System;
using JoltCSharp;

public class JoltPickableAnimController : AbstractCacheableAnimNode<Pickable, PickableState, PickableConfig, uint> {

    public JoltPickableAnimController() {
        SetUd(PbPrimitivesOverride.Instance.getUnderlying().TerminatingPickableId);
        SetCacheGroupId(PbPrimitivesOverride.Instance.getUnderlying().Pkts.None);
    }

    protected override bool lazyInit() {
        if (null != lookUpTable && 0 < lookUpTable.Count) return true;
        lookUpTable = new Dictionary<PickableState, AnimationClip>();
        animator = getMainAnimator();
        if (null == animator) return false;
        spr = gameObject.GetComponent<SpriteRenderer>();
        if (null != sprDefaultMaterial) {
            spr.material = sprDefaultMaterial; // [WARNING] This assignment creates a copy of the material for the current SpriteRenderer, thus independent from other SpriteRenderers when being updated.
        }
        material = spr.material;
        spr.sortingLayerName = "Pickable";
        if (null == spr) return false;
        foreach (AnimationClip clip in animator.runtimeAnimatorController.animationClips) {
            PickableState pkState;
            Enum.TryParse(clip.name, out pkState);
            lookUpTable[pkState] = clip;
        }
        return true;
    }

    protected override bool updateAnimUnderlying(in int currRdfId, in Pickable currPickable, in PickableState newState, in PickableConfig insConfig, in int frameIdxInAnim) {
        var effNewState = newState;
        if (!lookUpTable.ContainsKey(newState)) {
            effNewState = PickableState.Pidle;
        }
        SetCacheGroupId(currPickable.PickupType);

        int targetLayer = 0; // We have only 1 layer, i.e. the baseLayer, playing at any time
        int targetClipIdx = 0; // We have only 1 frame anim playing at any time
        var curClip = animator.GetCurrentAnimatorClipInfo(targetLayer)[targetClipIdx].clip;
        var targetClip = lookUpTable[effNewState];
        float normalizedFromTime = (frameIdxInAnim / (targetClip.frameRate * targetClip.length)); // TODO: Anyway to avoid using division here?
        animator.Play(targetClip.name, targetLayer, normalizedFromTime);

        return true;
    }
}
