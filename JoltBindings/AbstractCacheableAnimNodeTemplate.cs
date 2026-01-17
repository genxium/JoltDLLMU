using System.Collections.Generic;

public abstract class AbstractCacheableAnimNodeTemplate<T, S, C, G> {

    private ulong ud = 0;
    public ulong GetUd() {
        return ud;
    }
    public void SetUd(ulong val) {
        ud = val;
    }

    private G? cacheGroupId;
    public G? GetCacheGroupId() {
        return cacheGroupId;
    }

    public void SetCacheGroupId(G val) {
        cacheGroupId = val;
    }

    private int lastActiveRdfId = 0;
    public int GetLastActiveRdfId() {
        return lastActiveRdfId;
    }

    public bool updateAnim(in int currRdfId, in ulong newUd, in T ins, in S newState, in C insConfig, in int framesInNewState) {
        this.ud = newUd;
        if (!lazyInit()) return false;
        updateAnimUnderlying(currRdfId, ins, newState, insConfig, framesInNewState);
        lastActiveRdfId = currRdfId;
        return true;
    }

    protected abstract void updateAnimUnderlying(in int currRdfId, in T ins, in S newState, in C insConfig, in int framesInNewState);

    public string identifier() {
        return $"(class={this.GetType().Name}, ud={ud}, cachedGroupId={cacheGroupId})";
    }

    protected abstract bool lazyInit();
    public abstract void guiDestroy();
    public abstract void hideSelf(in float effectivelyInfinitelyFar, in float defaultZ);
    /*
    // A reference for Unity implementation.
    public override void hideSelf(in float effectivelyInfinitelyFar, in float defaultZ) {
        positionHolder.Set(effectivelyInfinitelyFar, effectivelyInfinitelyFar, defaultZ);
        g.gameObject.transform.position = positionHolder;
    }
    */
}
