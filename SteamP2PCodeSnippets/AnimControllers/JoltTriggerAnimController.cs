using System.Collections.Generic;
using UnityEngine;
using jtshared;
using System;
using JoltCSharp;

public class JoltTriggerAnimController : AbstractCacheableAnimNode<Trigger, TriggerState, TriggerConfigFromTiled, uint> {

    public JoltTriggerAnimController() {
        SetUd(PbPrimitivesOverride.Instance.getUnderlying().TerminatingTriggerId);
        SetCacheGroupId(PbPrimitivesOverride.Instance.getUnderlying().Tpts.None);
    }

    protected override bool lazyInit() {
        if (null != lookUpTable && 0 < lookUpTable.Count) return true;
        lookUpTable = new Dictionary<TriggerState, AnimationClip>();
        animator = getMainAnimator();
        if (null == animator) return false;
        spr = gameObject.GetComponent<SpriteRenderer>();
        if (null != sprDefaultMaterial) {
            spr.material = sprDefaultMaterial; // [WARNING] This assignment creates a copy of the material for the current SpriteRenderer, thus independent from other SpriteRenderers when being updated.
        }
        material = spr.material;
        spr.sortingLayerName = "Trigger";
        if (null == spr) return false;
        foreach (AnimationClip clip in animator.runtimeAnimatorController.animationClips) {
            TriggerState triggerState;
            Enum.TryParse(clip.name, out triggerState);
            lookUpTable[triggerState] = clip;
        }
        return true;
    }

    protected override bool updateAnimUnderlying(in int currRdfId, in Trigger currTrigger, in TriggerState newState, in TriggerConfigFromTiled insConfig, in int frameIdxInAnim) {
        var effNewState = newState;
        if (!lookUpTable.ContainsKey(newState)) {
            effNewState = TriggerState.TrCoolingDown;
        }
        SetCacheGroupId(currTrigger.Trt);
        facingQ.Set(insConfig.InitQX, insConfig.InitQY, insConfig.InitQZ, insConfig.InitQW);
        Vector3 facing = facingQ * Vector3.right;
        if (0 > facing.x) {
            scaleHolder.Set(-1.0f, 1.0f, this.gameObject.transform.localScale.z);
            this.gameObject.transform.localScale = scaleHolder;
        } else if (0 < facing.x) {
            scaleHolder.Set(+1.0f, 1.0f, this.gameObject.transform.localScale.z);
            this.gameObject.transform.localScale = scaleHolder;
        }

        int targetLayer = 0; // We have only 1 layer, i.e. the baseLayer, playing at any time
        int targetClipIdx = 0; // We have only 1 frame anim playing at any time
        var curClip = animator.GetCurrentAnimatorClipInfo(targetLayer)[targetClipIdx].clip;
        var targetClip = lookUpTable[effNewState];
        float normalizedFromTime = (frameIdxInAnim / (targetClip.frameRate * targetClip.length)); // TODO: Anyway to avoid using division here?
        animator.Play(targetClip.name, targetLayer, normalizedFromTime);

        return true;
    }
}
