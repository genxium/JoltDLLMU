using System;
using System.Collections.Generic;
using UnityEngine;

public abstract class AbstractCacheableAnimNodePool<T, S, C, G, A> where A : AbstractCacheableAnimNode<T, S, C, G> where G : IComparable {
    protected ulong terminatingUd;
    protected G terminatingCacheGroupId;
    protected float effectivelyInfinitelyFar;
    protected float defaultZ;
    protected AbstractJoltMapController joltMap;

    protected Vector3 positionHolder = new Vector3();

    public void SetGeometricConsts(in float theEffectivelyInfinitelyFar, in float theDefaultZ) {
        this.effectivelyInfinitelyFar = theEffectivelyInfinitelyFar;
        this.defaultZ = theDefaultZ;
    }

    public AbstractCacheableAnimNodePool(in AbstractJoltMapController joltMap, in ulong terminatingUd, in G terminatingGroupId) {
        this.joltMap = joltMap;
        this.terminatingUd = terminatingUd;
        this.terminatingCacheGroupId = terminatingGroupId;
    }

    protected List<ulong> transientActiveAnimNodeKeysToRemove;
    protected Dictionary<ulong, A> activeAnimNodes;
    protected Dictionary<G, LinkedList<A>> cachedAnimNodes;

    public void HideInactiveAnimNodes(int atTheEndOfRdfId) {
        transientActiveAnimNodeKeysToRemove.Clear();
        transientActiveAnimNodeKeysToRemove.AddRange(activeAnimNodes.Keys);
        foreach (var ud in transientActiveAnimNodeKeysToRemove) {
            var g = activeAnimNodes[ud];
            bool wasUsedInLastRdf = (g.GetLastActiveRdfId() == atTheEndOfRdfId);
            if (wasUsedInLastRdf) continue;
            activeAnimNodes.Remove(ud);
            G cacheGroupId = g.GetCacheGroupId();
            positionHolder.Set(effectivelyInfinitelyFar, effectivelyInfinitelyFar, g.gameObject.transform.position.z);
            g.gameObject.transform.position = positionHolder;
            if (!cachedAnimNodes.ContainsKey(cacheGroupId)) {
                cachedAnimNodes[cacheGroupId] = new LinkedList<A>();
            }
            var cachedQ = cachedAnimNodes[cacheGroupId];
            cachedQ.AddLast(g);
        }
        transientActiveAnimNodeKeysToRemove.Clear();
    }

    public void ResetUponBattlePreparation(int specifiedLayer = -1) {
        if (null == cachedAnimNodes) {
            cachedAnimNodes = new Dictionary<G, LinkedList<A>>();
        }

        if (null == activeAnimNodes) {
            activeAnimNodes = new Dictionary<ulong, A>();
        }

        if (null == transientActiveAnimNodeKeysToRemove) {
            transientActiveAnimNodeKeysToRemove = new List<ulong>();
        }

        HideInactiveAnimNodes(-1);

        foreach (var (_, v) in cachedAnimNodes) {
            while (0 < v.Count) {
                var g = v.Last.Value;
                v.RemoveLast();
                if (null != g) {
                    UnityEngine.Object.Destroy(g.gameObject);
                }
            }
        }

    }

    protected abstract GameObject loadPrefab(C insConfig);

    protected virtual A CreateAnimNode(in G cacheGroupId, in C insConfig, in Transform parent, in int specifiedLayer = -1) {
        var thePrefab = loadPrefab(insConfig);
        positionHolder.Set(effectivelyInfinitelyFar, effectivelyInfinitelyFar, defaultZ);
        GameObject newNode = UnityEngine.Object.Instantiate(thePrefab, positionHolder, Quaternion.identity, parent);
        if (0 <= specifiedLayer) {
            newNode.layer = specifiedLayer;
        }
        var g = newNode.GetComponent<A>();
        if (null == g) {
            g = newNode.AddComponent<A>();
        }
        g.SetUd(terminatingUd);
        g.SetCacheGroupId(cacheGroupId);
        return g;
    }

    public (A, ulong) GetOrCreateAnimNode(in ulong ud, in G cacheGroupId, in C insConfig, in Transform parent, in int specifiedLayer = -1) {
        A g = null;
        ulong oldUd = terminatingUd;
        if (activeAnimNodes.ContainsKey(ud)) {
            if (0 == cacheGroupId.CompareTo(activeAnimNodes[ud].GetCacheGroupId())) {
                g = activeAnimNodes[ud];
            } else {
                var obsolete = activeAnimNodes[ud];
                activeAnimNodes.Remove(ud);
                G obsoleteCacheGroupId = obsolete.GetCacheGroupId();
                positionHolder.Set(effectivelyInfinitelyFar, effectivelyInfinitelyFar, obsolete.gameObject.transform.position.z);
                obsolete.gameObject.transform.position = positionHolder;
                if (!cachedAnimNodes.ContainsKey(obsoleteCacheGroupId)) {
                    cachedAnimNodes[obsoleteCacheGroupId] = new LinkedList<A>();
                }
                var cachedQ = cachedAnimNodes[obsoleteCacheGroupId];
                cachedQ.AddLast(obsolete);
            }
        }

        if (null == g) {
            if (!cachedAnimNodes.ContainsKey(cacheGroupId)) {
                g = CreateAnimNode(cacheGroupId, insConfig, parent, specifiedLayer);
            } else {
                var cachedQ = cachedAnimNodes[cacheGroupId];
                if (0 >= cachedQ.Count) {
                    g = CreateAnimNode(cacheGroupId, insConfig, parent, specifiedLayer);
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
}
