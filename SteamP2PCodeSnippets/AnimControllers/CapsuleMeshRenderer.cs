using UnityEngine;

[RequireComponent(typeof(MeshFilter), typeof(MeshRenderer))]
public class CapsuleMeshRenderer : MonoBehaviour {

    public static class Capsule2DGeometry {

        public static void Generate(
            float radius,
            float halfHeight,
            int segments,
            ref Vector3[] verts,
            ref int[] tris,
            ref int vertOffset) {

            Vector3 topCenter = Vector3.up * halfHeight;
            Vector3 bottomCenter = Vector3.down * halfHeight;

            float step = Mathf.PI / segments;
            int v = 0;

            // --- Vertices ---
            for (int i = 0; i < arcPoints; i++) {
                float a = i * step;
                verts[v++] = (topCenter + new Vector3(Mathf.Cos(a) * radius, Mathf.Sin(a) * radius));
            }

            for (int i = 0; i < arcPoints; i++) {
                float a = Mathf.PI + i * step;
                verts[v++] = (bottomCenter + new Vector3(Mathf.Cos(a) * radius, Mathf.Sin(a) * radius));
            }

            // --- Triangles ---
            int topStart = vertOffset;
            int topEnd = topStart + segments;
            int bottomStart = topEnd + 1;
            int bottomEnd = bottomStart + segments;
            int t = 0;

            // Top arc fan
            for (int i = 1; i < segments; i++) {
                tris[t++] = topStart;
                tris[t++] = topStart + i;
                tris[t++] = topStart + i + 1;
            }

            // Bottom arc fan
            for (int i = 1; i < segments; i++) {
                tris[t++] = bottomStart;
                tris[t++] = bottomStart + i;
                tris[t++] = bottomStart + i + 1;
            }

            // Rectangle between arcs
            tris[t++] = topStart;
            tris[t++] = topEnd;
            tris[t++] = bottomStart;

            tris[t++] = topStart;
            tris[t++] = bottomStart;
            tris[t++] = bottomEnd;

            vertOffset += (arcPoints * 2);
        }
    }

    private ulong ud = 0;
    private float radius = 0f;
    private float halfHeight = 0f;
    public float GetHalfHeight() {
        return halfHeight;
    }

    private const int segments = 24;
    private const int arcPoints = segments + 1;
    private const int vertCount = arcPoints * 2;
    private const int triCount = 3*(2*(segments - 1) + 2);
    private Vector3[] vertices = new Vector3[vertCount];
    private int[] triangles = new int[triCount];

    public bool SetUd(in ulong theUd) {
        if (ud != theUd) {
            ud = theUd;
            return true;
        }
        return false;
    }

    public bool SetRadiusAndHalfHeight(in float theRadius, in float inHalfHeight) {
        if (radius != theRadius || halfHeight != inHalfHeight) {
            radius = theRadius;
            halfHeight = inHalfHeight;
            rebuild();
            return true;
        }

        return false;
    }

    private void rebuild() {
        var meshFilter = GetComponent<MeshFilter>();
        var mesh = meshFilter.mesh;   
        mesh.Clear();
        
        int vertOffset = 0; // For future batch rendering use

        Capsule2DGeometry.Generate(radius,
            halfHeight,
            segments,
            ref vertices,
            ref triangles,
            ref vertOffset);

        mesh.vertices = vertices;
        mesh.triangles = triangles;
        mesh.RecalculateNormals();
        mesh.RecalculateBounds();
    }
}
