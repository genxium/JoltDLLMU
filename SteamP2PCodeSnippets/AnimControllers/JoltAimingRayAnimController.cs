using jtshared;
using UnityEngine;
using JoltCSharp;
using static FrontendOnlyGeometry;

public class JoltAimingRayAnimController : AbstractCacheableAnimNode<AimingRay, CharacterState, AbstractJoltMapController, uint> {
    public LineRenderer lineRenderer;

    public JoltAimingRayAnimController() {
        SetUd(PbPrimitivesOverride.Instance.getUnderlying().TerminatingCharacterId);
        SetCacheGroupId(PbPrimitivesOverride.Instance.getUnderlying().ChSpecies.None);
    }

    public Material GetMaterial() {
        if (!lazyInit()) return null;
        return material;
    }

    protected new Animator getMainAnimator() {
        return null;
    }

    protected bool initialized = false;
    protected override bool lazyInit() {
        if (initialized) return true;
        initialized = true;
        lineRenderer.startWidth = 1.0f;
        lineRenderer.endWidth = 1.0f;
        lineRenderer.startColor = Color.red;
        lineRenderer.endColor = Color.red;
        lineRenderer.SetPositions(new Vector3[] { Vector3.zero, Vector3.zero });
        return true;
    }
    
    protected override bool updateAnimUnderlying(in int rdfId, in AimingRay aimingRay, in CharacterState newCharacterState, in AbstractJoltMapController theMap, in int framesInNewState) {
        SetCacheGroupId(0);
        var (wStX, wStY) = CollisionSpacePositionToWorldPosition(aimingRay.StX, aimingRay.StY, theMap.GetTilemapHalfHeight(), theMap.GetCollisionSpacePaddingLeft(), theMap.GetCollisionSpacePaddingBottom());

        var (wEdX, wEdY) = CollisionSpacePositionToWorldPosition(aimingRay.EdX, aimingRay.EdY, theMap.GetTilemapHalfHeight(), theMap.GetCollisionSpacePaddingLeft(), theMap.GetCollisionSpacePaddingBottom());

        positionHolder.Set(wStX, wStY, 0);
        scaleHolder.Set(wEdX, wEdY, 0);
        lineRenderer.SetPosition(0, positionHolder);
        lineRenderer.SetPosition(1, scaleHolder);
        return true;
    }
}
