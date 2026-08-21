using jtshared;
using UnityEngine;
using JoltCSharp;
using Google.Protobuf.Collections;

public class JoltDebugColliderAnimController : AbstractCacheableAnimNode<RepeatedField<PbVec2>, CharacterState, AbstractJoltMapController, uint> {
    public LineRenderer lineRenderer;

    public JoltDebugColliderAnimController() {
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
        lineRenderer.startColor = Color.yellow;
        lineRenderer.endColor = Color.yellow;
        lineRenderer.positionCount = 4;
        lineRenderer.SetPositions(new Vector3[] { Vector3.zero, Vector3.zero, Vector3.zero, Vector3.zero });
        return true;
    }
    
    protected override bool updateAnimUnderlying(in int rdfId, in RepeatedField<PbVec2> points, in CharacterState newCharacterState, in AbstractJoltMapController theMap, in int framesInNewState) {
        SetCacheGroupId(0);
        for (int i = 0; i < points.Count; i++) {
            var fromPoint = points[i];
            int j = (i+1 >= points.Count) ? 0 : (i+1);
            var toPoint = points[j];
            positionHolder.Set(fromPoint.X, fromPoint.Y, 0);
            scaleHolder.Set(toPoint.X, toPoint.Y, 0);
            if (i < lineRenderer.positionCount) {
                lineRenderer.SetPosition(i, positionHolder);
            }
            if (j < lineRenderer.positionCount) {
                lineRenderer.SetPosition(j, scaleHolder);
            }
        }

        return true;
    }
}
