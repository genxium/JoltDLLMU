using System;
using System.Collections.Generic;
using UnityEngine;

public abstract class AbstractCacheableAnimNode<T, S, C, G> : MonoBehaviour where G : IComparable {

    protected Animator animator;
    protected SpriteRenderer spr;
    protected Material material;

    private ulong ud = 0;
    public ulong GetUd() {
        return ud;
    }
    public void SetUd(ulong val) {
        ud = val;
    }

    private G cacheGroupId;
    public G GetCacheGroupId() {
        return cacheGroupId;
    }

    public void SetCacheGroupId(G val) {
        cacheGroupId = val;
    }

    private int lastActiveRdfId = 0;
    public int GetLastActiveRdfId() {
        return lastActiveRdfId;
    }

    protected Vector3 scaleHolder = new Vector3();
    protected Vector3 positionHolder = new Vector3();
    protected Quaternion facingQ = new Quaternion();

    protected abstract bool lazyInit();

    void Start() {
        lazyInit();
    }

    public bool updateAnim(in int currRdfId, in ulong newUd, in T ins, in S newState, in C insConfig, in int framesInNewState) {
        this.ud = newUd;
        if (!lazyInit()) return false;
        if (updateAnimUnderlying(currRdfId, ins, newState, insConfig, framesInNewState)) {
            lastActiveRdfId = currRdfId;
            return true;
        } else {
            return false;
        }
    }

    protected abstract bool updateAnimUnderlying(in int currRdfId, in T ins, in S newState, in C insConfig, in int framesInNewState);

    protected Animator getMainAnimator() {
        var animator = gameObject.GetComponent<Animator>();
        if (null == animator) {
            Debug.LogError($"{identifier()} has no Animator component, is it an Aseprite file with only 1 frame?");
        }
        return animator;
    }

    protected Dictionary<S, AnimationClip> lookUpTable;
    public void resetLookupTable() {
        if (null != lookUpTable) {
            lookUpTable.Clear();
        }
    }

    public void pause(bool toPause) {
        var mainAnimator = getMainAnimator();
        if (toPause) {
            mainAnimator.speed = 0f;
        } else {
            mainAnimator.speed = 1f;
        }
    }

    protected Material sprDefaultMaterial;

    public void setSprDefaultMaterial(Material theSprDefaultMaterial) {
        sprDefaultMaterial = theSprDefaultMaterial;
    }

    public string identifier() {
        return $"(class={this.GetType().Name}, ud={ud}, cachedGroupId={cacheGroupId})";
    }
}
