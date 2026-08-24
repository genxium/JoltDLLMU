using UnityEngine;

[RequireComponent(typeof(MeshFilter), typeof(MeshRenderer))]
public class BoxMeshRenderer : MonoBehaviour {

    public static class Box2DGeometry {

        public static void Generate(
            float halfX,
            float halfY,
            ref Vector3[] verts,
            ref int[] tris,
            ref int vertOffset) {

            // --- Vertices ---
            verts[0] = Vector3.up * halfY + Vector3.left * halfX;
            verts[1] = Vector3.up * halfY + Vector3.right * halfX;
            verts[2] = Vector3.down * halfY + Vector3.right * halfX;
            verts[3] = Vector3.down * halfY + Vector3.left * halfX;

            // --- Triangles ---           
            tris[0] = 0;
            tris[1] = 1;
            tris[2] = 2;

            tris[3] = 2;
            tris[4] = 3;
            tris[5] = 0;

            vertOffset += vertCount;
        }
    }

    private ulong ud = 0;
    private float halfX = 0f;
    private float halfY= 0f;
    
    private const int vertCount = 4;
    private const int triCount = 2*3;
    private Vector3[] vertices = new Vector3[vertCount];
    private int[] triangles = new int[triCount];

    public bool SetUd(in ulong theUd) {
        if (ud != theUd) {
            ud = theUd;
            return true;
        }
        return false;
    }

    public bool SetHalfExtent(in float theHalfX, in float theHalfY) {
        if (halfX != theHalfX || halfY != theHalfY) {
            halfX = theHalfX;
            halfY = theHalfY;
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

        Box2DGeometry.Generate(halfX,
            halfY,
            ref vertices,
            ref triangles,
            ref vertOffset);

        mesh.vertices = vertices;
        mesh.triangles = triangles;
        mesh.RecalculateNormals();
        mesh.RecalculateBounds();
    }
}
