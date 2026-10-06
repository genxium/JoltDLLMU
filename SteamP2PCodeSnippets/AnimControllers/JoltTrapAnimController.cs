using System.Collections.Generic;
using UnityEngine;
using jtshared;
using System;
using JoltCSharp;

public class JoltTrapAnimController : AbstractCacheableAnimNode<Trap, TrapState, TrapConfigFromTiled, uint> {

    public JoltTrapAnimController() {
        SetUd(PbPrimitivesOverride.Instance.getUnderlying().TerminatingTrapId);
        SetCacheGroupId(PbPrimitivesOverride.Instance.getUnderlying().Tpts.None);
    }

    protected float absScaleX = 1.0f;
    protected float absScaleY = 1.0f;

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

    protected override bool updateAnimUnderlying(in int currRdfId, in Trap currTrap, in TrapState newState, in TrapConfigFromTiled insConfig, in int frameIdxInAnim) {
        if (!lookUpTable.ContainsKey(newState)) {
            return false;
        }

        if (TrapState.TpIdle == newState) {
            // [REMINDER] The "scale" MUST be calculated w.r.t. the reference frame used in map editor (e.g. Tiled).
            absScaleX = 2.0f * insConfig.RenderBoxHalfSizeX / spr.size.x;
            absScaleY = 2.0f * insConfig.RenderBoxHalfSizeY / spr.size.y;
        }

        facingQ.Set(currTrap.QX, currTrap.QY, currTrap.QZ, currTrap.QW);
        Vector3 trapFacing = facingQ * Vector3.right;

        if (insConfig.AllowsRotationFromPhySys) {
            this.gameObject.transform.localRotation = facingQ;
            scaleHolder.Set(+absScaleX, absScaleY, this.gameObject.transform.localScale.z);
            this.gameObject.transform.localScale = scaleHolder;
        } else {
            if (0 > trapFacing.x) {
                scaleHolder.Set(-absScaleX, absScaleY, this.gameObject.transform.localScale.z);
                this.gameObject.transform.localScale = scaleHolder;
            } else if (0 < trapFacing.x) {
                scaleHolder.Set(+absScaleX, absScaleY, this.gameObject.transform.localScale.z);
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
