using JoltCSharp;
using jtshared;
using System.Collections.Generic;
using UnityEngine;

public class JoltAimingRayAnimPool : AbstractCacheableAnimNodePool<AimingRay, CharacterState, AbstractJoltMapController, uint, JoltAimingRayAnimController> {

    public JoltAimingRayAnimPool(in AbstractJoltMapController joltMap) : base(joltMap, Bindings.APP_CalcNpcUserData(PbPrimitivesOverride.Instance.getUnderlying().TerminatingCharacterId), PbPrimitivesOverride.Instance.getUnderlying().ChSpecies.None) {
    }

    protected override GameObject loadPrefab(AbstractJoltMapController theMap) {
        return joltMap.loadAimingRayPrefab();
    }

    public new void HideInactiveAnimNodes(int atTheEndOfRdfId) {
        transientActiveAnimNodeKeysToRemove.Clear();
        transientActiveAnimNodeKeysToRemove.AddRange(activeAnimNodes.Keys);
        foreach (var ud in transientActiveAnimNodeKeysToRemove) {
            JoltAimingRayAnimController g = activeAnimNodes[ud];
            bool wasUsedInLastRdf = (g.GetLastActiveRdfId() == atTheEndOfRdfId);
            if (wasUsedInLastRdf) continue;
            activeAnimNodes.Remove(ud);
            var cacheGroupId = g.GetCacheGroupId();
            positionHolder.Set(effectivelyInfinitelyFar, effectivelyInfinitelyFar, g.gameObject.transform.position.z);
            g.gameObject.transform.position = positionHolder;
            if (!cachedAnimNodes.ContainsKey(cacheGroupId)) {
                cachedAnimNodes[cacheGroupId] = new LinkedList<JoltAimingRayAnimController>();
            }
            var cachedQ = cachedAnimNodes[cacheGroupId];
            cachedQ.AddLast(g);
            g.lineRenderer.SetPosition(0, Vector3.zero);
            g.lineRenderer.SetPosition(1, Vector3.zero);
        }
        transientActiveAnimNodeKeysToRemove.Clear();
    }
}
