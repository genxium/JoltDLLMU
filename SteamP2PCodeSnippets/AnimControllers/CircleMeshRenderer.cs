using UnityEngine;

[RequireComponent(typeof(MeshFilter), typeof(MeshRenderer))]
public class CircleMeshRenderer : MonoBehaviour {

    public static class Circle2DGeometry {

        public static void Generate(
            float radius,
            int segments,
            ref Vector3[] verts,
            ref int[] tris,
            ref int vertOffset) {

            Vector3 center = Vector3.zero;

            float step = Mathf.PI / segments;
            int v = 0;

            // --- Vertices ---
            verts[v++] = center;
            for (int i = 1; i < vertCount; i++) {
                float a = i * step;
                verts[v++] = (center + new Vector3(Mathf.Cos(a) * radius, Mathf.Sin(a) * radius));
            }

            // --- Triangles ---
            int t = 0;

            for (int i = 1; i < vertCount; i++) {
                tris[t++] = 0;
                var start = i;
                var end = i + 1;
                if (end >= vertCount) {
                    end = 1;
                }
                tris[t++] = start;
                tris[t++] = end;
            }

            vertOffset += vertCount;
        }
    }

    private ulong ud = 0;
    private float radius = 0f;

    private const int segments = 24;
    private const int vertCount = segments * 1;
    private const int triCount = segments;
    private Vector3[] vertices = new Vector3[vertCount];
    private int[] triangles = new int[triCount];

    public bool SetUd(in ulong theUd) {
        if (ud != theUd) {
            ud = theUd;
            return true;
        }
        return false;
    }

    public bool SetRadius(in float theRadius) {
        if (radius != theRadius) {
            radius = theRadius;
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

        Circle2DGeometry.Generate(radius,
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
