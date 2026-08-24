using JoltCSharp;
using UnityEngine;
using Google.Protobuf;
using System;

public class JoltDebugColliderAnimPool : AbstractCacheableAnimNodePool<IMessage, Enum, IMessage, uint, JoltDebugColliderAnimController> {

    public JoltDebugColliderAnimPool(in AbstractJoltMapController joltMap) : base(joltMap, Bindings.APP_CalcNpcUserData(PbPrimitivesOverride.Instance.getUnderlying().TerminatingCharacterId), PbPrimitivesOverride.Instance.getUnderlying().ChSpecies.None) {
    }

    protected override GameObject loadPrefab(IMessage targetConfig) {
        return joltMap.loadDebugColliderPrefab();
    }
}
