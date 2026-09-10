using Google.Protobuf;
using JoltCSharp;
using jtshared;
using System;
using UnityEngine;
using static JoltDebugColliderAnimPool;

public class JoltDebugColliderAnimPool : AbstractCacheableAnimNodePool<IMessage, Enum, IMessage, MeshType, JoltDebugColliderAnimController> {

    public JoltDebugColliderAnimPool(in AbstractJoltMapController joltMap) : base(joltMap, Bindings.APP_CalcNpcUserData(PbPrimitivesOverride.Instance.getUnderlying().TerminatingCharacterId), MeshType.None) {
    }

    public enum MeshType {
        None,
        Circle,
        Box,
        Capsule,
    }

    public MeshType CalcMeshType(in IMessage target, in IMessage newTargetConfig) {
        switch (target) {
            case Bullet bullet:
                if (newTargetConfig is BulletConfig bulletConfig) {
                    if (BulletType.MechanicalBouncerSpherical == bulletConfig.BType) {
                        return MeshType.Circle;
                    } else {
                        return MeshType.Box;
                    }
                }
                return MeshType.None;
            case CharacterDownsync chd:
                if (newTargetConfig is CharacterConfig chConfig) {
                    return MeshType.Capsule;
                }
                return MeshType.None;
            default:
                return MeshType.None;
        }
    }

    protected override GameObject loadPrefab(IMessage targetConfig) {
        return joltMap.loadDebugColliderPrefab();
    }
}
