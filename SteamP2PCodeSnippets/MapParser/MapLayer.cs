using Google.Protobuf.Collections;
using jtshared;
using System;
using System.Linq;
using UnityEngine;

namespace Assets.Scripts.MapParser {
    public enum MapLayerType {
        None,
        Background,
        Foreground,
        Barrier,
        Trap,
        Trigger,
        Player,
        Npc,
       
        GrandCameraBound,
        CameraBound,
    }

    [Serializable]
    public struct OptionalBool {
        public bool configured;
        public bool value;

        public bool? Value => configured ? value : null;
    }

    [Serializable]
    public struct OptionalInt {
        public bool configured;
        public int value;

        public int? Value => configured ? value : null;
    }

    [Serializable]
    public struct OptionalUint {
        public bool configured;
        public uint value;

        public uint? Value => configured ? value : null;
    }

    [Serializable]
    public struct OptionalUlong {
        public bool configured;
        public ulong value;

        public ulong? Value => configured ? value : null;
    }

    [Serializable]
    public struct OptionalFloat {
        public bool configured;
        public float value;

        public float? Value => configured ? value : null;
    }

    [Serializable]
    public class MapColliderOptions {
        public bool providesSlipJump = false;
        public bool providesStairsP = false;
        public bool providesStairsN = false;
        public bool providesWallGrabbing = false;
    }

    [Serializable]
    public class CharacterOptions {
        public int teamId;
    }

    public class MapLayer : MonoBehaviour {
        public MapLayerType primaryLayerType = MapLayerType.Barrier;
        public float xParallaxFactor = 1f;
        public float yParallaxFactor = 1f;
        
        public static SerializableConvexPolygon parseSerializableConvexPolygon(in Transform shapeChild) {
            if (null == shapeChild) {
                return null;
            }
            var boxCollider2D = shapeChild.GetComponent<BoxCollider2D>();
            if (null != boxCollider2D) {
                // [REMINDER] Intentionally NOT using "boxCollider2D.bounds.min & boxCollider2D.bounds.max" here because we want to return a "serializedConvexPolygon.points" relative to "shapeChild.parent"
                float effOffsetXRelativeToParent = shapeChild.localPosition.x + (shapeChild.lossyScale.x * boxCollider2D.offset.x);
                float effOffsetYRelativeToParent = shapeChild.localPosition.y + (shapeChild.lossyScale.y * boxCollider2D.offset.y);

                float halfW = .5f * boxCollider2D.bounds.size.x, halfH = .5f * boxCollider2D.bounds.size.y;

                float minXRelativeToParent = effOffsetXRelativeToParent - halfW, maxXRelativeToParent = effOffsetXRelativeToParent + halfW;
                float minYRelativeToParent = effOffsetYRelativeToParent - halfH, maxYRelativeToParent = effOffsetYRelativeToParent + halfH;

                RepeatedField<PbVec2> points = new RepeatedField<PbVec2> {
                    new PbVec2 {
                        X = minXRelativeToParent,
                        Y = minYRelativeToParent,
                    },
                    new PbVec2 {
                        X = minXRelativeToParent,
                        Y = maxYRelativeToParent,
                    },
                    new PbVec2 {
                        X = maxXRelativeToParent,
                        Y = maxYRelativeToParent,
                    },
                    new PbVec2 {
                        X = maxXRelativeToParent,
                        Y = minYRelativeToParent,
                    }
                };
                return new SerializableConvexPolygon { }.AddPoints(points).SetBox(true, shapeChild.rotation.x, shapeChild.rotation.y, shapeChild.rotation.z, shapeChild.rotation.w);
            } else {
                var polygonCollider2D = shapeChild.GetComponent<PolygonCollider2D>();
                if (null != polygonCollider2D) {
                    return new SerializableConvexPolygon { }.AddPoints(polygonCollider2D.points.Select(point => new PbVec2 { X = point.x, Y = point.y }).ToList());
                } else {
                    return null;
                }
            }
        }
    }
}
