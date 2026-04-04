using System;
using System.Collections.Generic;

public abstract class AbstractCacheableAnimNodePoolTemplate<T, S, C, G, A> where A : AbstractCacheableAnimNodeTemplate<T, S, C, G> where G : IComparable {
    protected ulong terminatingUd;
    protected G terminatingCacheGroupId;
    protected float effectivelyInfinitelyFar;
    protected float defaultZ;

    public void SetGeometricConsts(in float effectivelyInfinitelyFar, in float defaultZ) {
        this.effectivelyInfinitelyFar = effectivelyInfinitelyFar;
        this.defaultZ = defaultZ;
    }

    protected List<ulong> transientActiveAnimNodeKeysToRemove;
    protected Dictionary<ulong, A> activeAnimNodes;
    protected Dictionary<G, LinkedList<A>> cachedAnimNodes;

    public AbstractCacheableAnimNodePoolTemplate(in ulong terminatingUd, in G terminatingGroupId) {
        this.terminatingUd = terminatingUd;
        this.terminatingCacheGroupId = terminatingGroupId;
        this.cachedAnimNodes = new Dictionary<G, LinkedList<A>>();
        this.activeAnimNodes = new Dictionary<ulong, A>();
        this.transientActiveAnimNodeKeysToRemove = new List<ulong>();
    }

    public void HideInactiveAnimNodes(int atTheEndOfRdfId) {
        transientActiveAnimNodeKeysToRemove.Clear();
        transientActiveAnimNodeKeysToRemove.AddRange(activeAnimNodes.Keys);
        foreach (var ud in transientActiveAnimNodeKeysToRemove) {
            var g = activeAnimNodes[ud];
            bool wasUsedInLastRdf = (g.GetLastActiveRdfId() == atTheEndOfRdfId);
            if (wasUsedInLastRdf) continue;
            activeAnimNodes.Remove(ud);
            G cacheGroupId = g.GetCacheGroupId();
            g.hideSelf(effectivelyInfinitelyFar, defaultZ);
            if (null != cacheGroupId) {
                if (!cachedAnimNodes.ContainsKey(cacheGroupId)) {
                    cachedAnimNodes[cacheGroupId] = new LinkedList<A>();
                }
                var cachedQ = cachedAnimNodes[cacheGroupId];
                cachedQ.AddLast(g);
            }
        }
        transientActiveAnimNodeKeysToRemove.Clear();
    }

    public void ResetCacheUponBattlePreparation(int specifiedLayer = -1) {
        HideInactiveAnimNodes(-1);

        foreach (var (_, v) in cachedAnimNodes) {
            while (0 < v.Count) {
                var g = v.Last.Value;
                v.RemoveLast();
                g.guiDestroy();
            }
        }
    }

    public (A?, ulong) GetOrCreateAnimNode(in ulong ud, in G cacheGroupId, in C insConfig) {
        A? g = null;
        ulong oldUd = terminatingUd;
        if (activeAnimNodes.ContainsKey(ud)) {
            var existingG = activeAnimNodes[ud];
            G existingCacheGroupId = existingG.GetCacheGroupId();
            if (0 == cacheGroupId.CompareTo(existingG.GetCacheGroupId())) {
                g = existingG;
            } else {
                activeAnimNodes.Remove(ud);
                existingG.hideSelf(effectivelyInfinitelyFar, defaultZ);
                if (!cachedAnimNodes.ContainsKey(existingCacheGroupId)) {
                    cachedAnimNodes[existingCacheGroupId] = new LinkedList<A>();
                }
                var cachedQ = cachedAnimNodes[existingCacheGroupId];
                cachedQ.AddLast(existingG);
            }
        }

        if (null == g) {
            if (!cachedAnimNodes.ContainsKey(cacheGroupId)) {
                g = CreateAnimNode(cacheGroupId, insConfig);
            } else {
                var cachedQ = cachedAnimNodes[cacheGroupId];
                if (0 >= cachedQ.Count) {
                    g = CreateAnimNode(cacheGroupId, insConfig);
                } else {
                    g = cachedQ.Last.Value;
                    cachedQ.RemoveLast();
                }
            }
        }

        if (null != g) {
            oldUd = g.GetUd();
            g.SetUd(ud);
            activeAnimNodes[ud] = g;
        } 
        
        return (g, oldUd);
    }

    public abstract A CreateAnimNode(in G cachedGroupId, in C insConfig); 
    
    /*
    // A reference for Unity implementation.
    public override A CreateAnimNode(in G cachedGroupId, in C insConfig) {
        var thePrefab = loadPrefab(insConfig);
        positionHolder.Set(effectivelyInfinitelyFar, effectivelyInfinitelyFar, defaultZ);
        GameObject newNode = Object.Instantiate(thePrefab, positionHolder, Quaternion.identity, defaultParent.transform);
        var g = newNode.AddComponent<A>();
        g.SetUd(terminatingUd);
        g.SetCacheGroupId(cachedGroupId);
        if (null != defaultMaterial) {
            g.setSprDefaultMaterial(mat);
        }
        if (0 <= defaultLayer) {
            newNode.layer = defaultLayer;
        }
        return g;
    } 
    */
}
