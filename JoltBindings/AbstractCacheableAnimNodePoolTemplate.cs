using System.Collections.Generic;

public abstract class AbstractCacheableAnimNodePoolTemplate<T, S, C, G, A> where A : AbstractCacheableAnimNodeTemplate<T, S, C, G> {
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
            G? cacheGroupId = g.GetCacheGroupId();
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

    public (A?, ulong) GetOrCreateAnimNode(in ulong ud, in G cachedGroupId, in C insConfig) {
        A? a = null;
        ulong oldUd = terminatingUd;
        if (activeAnimNodes.ContainsKey(ud)) {
            a = activeAnimNodes[ud];
        } else if (!cachedAnimNodes.ContainsKey(cachedGroupId)) {
            a = CreateAnimNode(cachedGroupId, insConfig);
        } else {
            var cachedQ = cachedAnimNodes[cachedGroupId];
            if (0 >= cachedQ.Count) {
                a = CreateAnimNode(cachedGroupId, insConfig);
            } else {
                a = cachedQ.Last.Value;
                cachedQ.RemoveLast();
            }
        }

        if (null != a) {
            oldUd = a.GetUd();
            a.SetUd(ud);
            activeAnimNodes[ud] = a;
        } 
        
        return (a, oldUd);
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
