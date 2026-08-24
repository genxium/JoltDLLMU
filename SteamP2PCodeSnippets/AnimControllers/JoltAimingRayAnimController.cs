using jtshared;
using UnityEngine;
using JoltCSharp;

public class JoltAimingRayAnimController : AbstractCacheableAnimNode<AimingRay, uint, uint, uint> {
    public Material meshMaterial;

    private BoxMeshRenderer lineRenderer;

    public JoltAimingRayAnimController(in AbstractJoltMapController theJoltMap) {
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
    private MeshFilter meshFilter = null;
    private MeshRenderer meshRenderer = null;
    protected override bool lazyInit() {
        if (initialized) return true;
        meshFilter = gameObject.AddComponent<MeshFilter>();
        meshFilter.mesh = new Mesh { };
        meshRenderer = gameObject.AddComponent<MeshRenderer>();
        meshRenderer.sortingLayerName = "EmittingBullet";
        meshRenderer.sharedMaterial = meshMaterial;
        initialized = true;
        return true;
    }
    
    protected override bool updateAnimUnderlying(in int rdfId, in AimingRay aimingRay, in uint ignored1, in uint ignored2, in int framesInNewState) {
        SetCacheGroupId(0);
        if (null == lineRenderer) {
            lineRenderer = gameObject.AddComponent<BoxMeshRenderer>();
        } else {
            lineRenderer = gameObject.GetComponent<BoxMeshRenderer>();
        }

        lineRenderer.SetHalfExtent(0.5f*(aimingRay.EdX - aimingRay.StX), 0.5f);

        return true;
    }
}
