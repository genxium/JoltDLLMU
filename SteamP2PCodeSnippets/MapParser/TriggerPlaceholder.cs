using JoltCSharp;
using jtshared;
using System;
using UnityEngine;

namespace Assets.Scripts.MapParser {

    public class TriggerPlaceholder : MonoBehaviour {
        [Serializable]
        public struct ChAndOp {
            public uint speciesId;
            public ulong initOp;
        }

        [Serializable]
        public struct CharacterSpawner {
            public int cutoffRdfId;
            public ChAndOp[] chAndOps;
        }

        [Serializable]
        public struct PkAndOp {
            public uint speciesId;
            public ulong initOp;
        }

        [Serializable]
        public struct PickableSpawner {
            public int cutoffRdfId;
            public PkAndOp[] pkAndOps;
        }

        public uint id;
        public uint trt;
        public int delayedFrames;  
        public int recoveryFrames;
        public int bulletTeamId; 

        public int subCycleTriggerFrames;
        public OptionalInt subCycleQuota;

        public OptionalInt quota;

        public OptionalUint publishingToTriggerIdUponExhausted; // [WARNING] Points to a "topo_lv" smaller than itself, and "A.topo_lv < B.topo_lv" means "A" is an ancestor of "B".

        public bool shouldRender;

        public CharacterSpawner[] characterSpawnerTimeSeq; 
        public PickableSpawner[] pickableSpawnerTimeSeq;

        // Use this for initialization
        void Start() {

        }

        // Update is called once per frame
        void Update() {

        }

        public TriggerConfigFromTiled buildTriggerConfigFromTiled() {
            var ret = new TriggerConfigFromTiled {
                Name = name,
                Id = id,
                Trt = trt,
                BulletTeamId = bulletTeamId,
                DelayedFrames = delayedFrames,
                RecoveryFrames = recoveryFrames,

                ShouldRender = shouldRender,
                SubCycleTriggerFrames = subCycleTriggerFrames,

                InitQX = gameObject.transform.rotation.x,
                InitQY = gameObject.transform.rotation.y,
                InitQZ = gameObject.transform.rotation.z,
                InitQW = gameObject.transform.rotation.w,
            };

            if (quota.configured) {
                ret.Quota = quota.value;
            } else {
                ret.Quota = 0;
            }

            if (subCycleQuota.configured) {
                ret.SubCycleQuota = subCycleQuota.value;
            } else {
                ret.SubCycleQuota = 0;
            }

            if (publishingToTriggerIdUponExhausted.configured) {
                ret.PublishingToTriggerIdUponExhausted = publishingToTriggerIdUponExhausted.value;
            } else {
                ret.PublishingToTriggerIdUponExhausted = PbPrimitivesOverride.Instance.getUnderlying().TerminatingTriggerId;
            }

            foreach (var s in characterSpawnerTimeSeq) {
                var casted = new CharacterSpawnerConfig {
                    CutoffRdfId = s.cutoffRdfId, 
                };
                foreach (var e in s.chAndOps) {
                    casted.SpeciesIdList.Add(e.speciesId);
                    casted.InitOpList.Add(e.initOp);
                }
                ret.CharacterSpawnerTimeSeq.Add(casted);
            }

            foreach (var s in pickableSpawnerTimeSeq) {
                var casted = new PickableSpawnerConfig {
                    CutoffRdfId = s.cutoffRdfId,
                };
                foreach (var e in s.pkAndOps) {
                    casted.PickupTypeList.Add(e.speciesId);
                    casted.InitOpList.Add(e.initOp);
                }
                ret.PickableSpawnerTimeSeq.Add(casted);
            }

            ret.BoxHalfSizeX = 0;
            ret.BoxHalfSizeY = 0;
            ret.RenderBoxHalfSizeX = 0;
            ret.RenderBoxHalfSizeY = 0;

            var animatorChild = gameObject.GetComponentInChildren<Animator>();
            if (null != animatorChild) {
                Renderer renderer = animatorChild.GetComponent<Renderer>();
                Bounds worldBounds = renderer.bounds;
                Vector3 center = worldBounds.center;
                Vector3 scaledSize = worldBounds.size;
                ret.BoxHalfSizeX = scaledSize.x * 0.5f;
                ret.BoxHalfSizeY = scaledSize.y * 0.5f;
                ret.RenderBoxHalfSizeX = scaledSize.x * 0.5f;
                ret.RenderBoxHalfSizeY = scaledSize.y * 0.5f;
            }

            var collidingSectionOverride = MapLayer.parseSerializableConvexPolygon(gameObject.transform.Find("CollidingSectionOverride"));
            if (null != collidingSectionOverride) {
                ret.CollidingSectionOverride = collidingSectionOverride;
            }

            return ret;
        }
    }
}
