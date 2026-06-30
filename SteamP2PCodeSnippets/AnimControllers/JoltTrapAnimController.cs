using System.Collections.Generic;
using UnityEngine;
using jtshared;
using System;
using JoltCSharp;

public class JoltTrapAnimController : AbstractCacheableAnimNode<Trap, TrapState, TrapConfig, uint> {

    public JoltTrapAnimController() {
        SetUd(PbPrimitivesOverride.Instance.getUnderlying().TerminatingTrapId);
        SetCacheGroupId(PbPrimitivesOverride.Instance.getUnderlying().Tpts.None);
    }

    protected override bool lazyInit() {
        if (null != lookUpTable && 0 < lookUpTable.Count) return true;
        lookUpTable = new Dictionary<TrapState, AnimationClip>();
        animator = getMainAnimator();
        if (null == animator) return false;
        spr = gameObject.GetComponent<SpriteRenderer>();
        if (null != sprDefaultMaterial) {
            spr.material = sprDefaultMaterial; // [WARNING] This assignment creates a copy of the material for the current SpriteRenderer, thus independent from other SpriteRenderers when being updated.
        }
        material = spr.material;
        spr.sortingLayerName = "Trap";
        if (null == spr) return false;
        foreach (AnimationClip clip in animator.runtimeAnimatorController.animationClips) {
            TrapState trapState;
            Enum.TryParse(clip.name, out trapState);
            lookUpTable[trapState] = clip;
        }
        return true;
    }

    protected override bool updateAnimUnderlying(in int currRdfId, in Trap currTrap, in TrapState newState, in TrapConfig insConfig, in int frameIdxInAnim) {
        if (!lookUpTable.ContainsKey(newState)) {
            return false;
        }
        SetCacheGroupId(currTrap.Tpt);
        facingQ.Set(currTrap.QX, currTrap.QY, currTrap.QZ, currTrap.QW);
        Vector3 trapFacing = facingQ * Vector3.right;

        if (PbPrimitivesOverride.TPT_ROTATING_PLATFORM == currTrap.Tpt) {
            this.gameObject.transform.localRotation = facingQ;
        } else {
            if (0 > trapFacing.x) {
                scaleHolder.Set(-1.0f, 1.0f, this.gameObject.transform.localScale.z);
                this.gameObject.transform.localScale = scaleHolder;
            } else if (0 < trapFacing.x) {
                scaleHolder.Set(+1.0f, 1.0f, this.gameObject.transform.localScale.z);
                this.gameObject.transform.localScale = scaleHolder;
            }
        }

        int targetLayer = 0; // We have only 1 layer, i.e. the baseLayer, playing at any time
        int targetClipIdx = 0; // We have only 1 frame anim playing at any time
        var curClip = animator.GetCurrentAnimatorClipInfo(targetLayer)[targetClipIdx].clip;
        var targetClip = lookUpTable[newState];
        float normalizedFromTime = (frameIdxInAnim / (targetClip.frameRate * targetClip.length)); // TODO: Anyway to avoid using division here?
        animator.Play(targetClip.name, targetLayer, normalizedFromTime);

        return true;
    }
}
