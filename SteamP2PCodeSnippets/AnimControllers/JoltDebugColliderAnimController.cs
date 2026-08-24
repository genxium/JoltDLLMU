using Google.Protobuf;
using JoltCSharp;
using jtshared;
using System;
using UnityEngine;

public class JoltDebugColliderAnimController : AbstractCacheableAnimNode<IMessage, Enum, IMessage, uint> {
    public Material hitboxMaterial;
    public Material hurtboxMateria;

    private BoxMeshRenderer box2DRenderer;
    public BoxMeshRenderer GetBox2DRenderer() {
        return box2DRenderer;
    }

    private CapsuleMeshRenderer capsule2DRenderer;
    public CapsuleMeshRenderer GetCapsule2DRenderer() {
        return capsule2DRenderer;
    }

    public JoltDebugColliderAnimController() {
        SetUd(PbPrimitivesOverride.Instance.getUnderlying().TerminatingCharacterId);
        SetCacheGroupId(PbPrimitivesOverride.Instance.getUnderlying().ChSpecies.None);
    }

    public Material GetMaterial() {
        if (!lazyInit()) return null;
        return material;
    }

    public float GetCapsuleHalfHeight() {
        if (null == capsule2DRenderer) {
            return 0.0f;
        }
        return capsule2DRenderer.GetHalfHeight();
    }

    protected new Animator getMainAnimator() {
        return null;
    }

    private MeshFilter meshFilter = null;
    private MeshRenderer meshRenderer = null;
    protected bool initialized = false;

    protected override bool lazyInit() {
        if (initialized) return true;
        meshFilter = gameObject.AddComponent<MeshFilter>();
        meshFilter.mesh = new Mesh { };
        meshRenderer = gameObject.AddComponent<MeshRenderer>();
        meshRenderer.sortingLayerName = "EmittingBullet";
        initialized = true;
        return true;
    }
    
    protected override bool updateAnimUnderlying(in int rdfId, in IMessage target, in Enum newTargetState, in IMessage newTargetConfig, in int framesInNewState) {
        SetCacheGroupId(0);
        switch (target) {
            case Bullet bullet:
                if (newTargetConfig is BulletConfig bulletConfig) {
                    if (null == box2DRenderer) {                       
                        box2DRenderer = gameObject.AddComponent<BoxMeshRenderer>();
                    } else {
                        box2DRenderer = gameObject.GetComponent<BoxMeshRenderer>();
                    }
                    meshRenderer.sharedMaterial = hitboxMaterial;
                    box2DRenderer.SetHalfExtent(bulletConfig.HitboxHalfSizeX, bulletConfig.HitboxHalfSizeY);
                }
                break;
            case CharacterDownsync chd:
                if (newTargetConfig is CharacterConfig chConfig) {
                    if (null == capsule2DRenderer) {
                        capsule2DRenderer = gameObject.AddComponent<CapsuleMeshRenderer>();
                    } else {
                        capsule2DRenderer = gameObject.GetComponent<CapsuleMeshRenderer>();
                    }
                    meshRenderer.sharedMaterial = hurtboxMateria;
                    float capsuleRadius = 0, capsuleHalfHeight = 0;
                    calcChdShape(chd.ChState, chConfig, out capsuleRadius, out capsuleHalfHeight);
                    capsule2DRenderer.SetRadiusAndHalfHeight(capsuleRadius, capsuleHalfHeight);
                }
                break;
            default:
                break;
        }

        return true;
    }

    void calcChdShape(in CharacterState chState, in CharacterConfig cc, out float outCapsuleRadius, out float outCapsuleHalfHeight) {
        switch (chState) {
            case CharacterState.LayDown1:
            case CharacterState.GetUp1:
                outCapsuleRadius = cc.LayDownCapsuleRadius;
                outCapsuleHalfHeight = cc.LayDownCapsuleHalfHeight;
                break;
            case CharacterState.Dying:
                outCapsuleRadius = cc.DyingCapsuleRadius;
                outCapsuleHalfHeight = cc.DyingCapsuleHalfHeight;
                break;
            case CharacterState.BlownUp1:
            case CharacterState.InAirIdle1NoJump:
            case CharacterState.InAirIdle1ByJump:
            case CharacterState.InAirIdle2ByJump:
            case CharacterState.InAirIdle1ByWallJump:
            case CharacterState.InAirWalking:
            case CharacterState.InAirAtk1:
            case CharacterState.InAirAtked1:
            case CharacterState.OnWallIdle1:
            case CharacterState.OnWallAtk1:
            case CharacterState.Sliding:
            case CharacterState.GroundDodged:
            case CharacterState.CrouchIdle1:
            case CharacterState.CrouchAtk1:
            case CharacterState.CrouchAtked1:
            case CharacterState.Dashing:
                outCapsuleRadius = cc.ShrinkedCapsuleRadius;
                outCapsuleHalfHeight = cc.ShrinkedCapsuleHalfHeight;
                break;
            case CharacterState.Dimmed:
                if (0 != cc.DimmedCapsuleRadius && 0 != cc.DimmedCapsuleHalfHeight) {
                    outCapsuleRadius = cc.DimmedCapsuleRadius;
                    outCapsuleHalfHeight = cc.DimmedCapsuleHalfHeight;
                } else {
                    outCapsuleRadius = cc.CapsuleRadius;
                    outCapsuleHalfHeight = cc.CapsuleHalfHeight;
                }
                break;
            default:
                outCapsuleRadius = cc.CapsuleRadius;
                outCapsuleHalfHeight = cc.CapsuleHalfHeight;
                break;
        }
    }

}
