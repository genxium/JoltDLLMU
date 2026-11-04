using Assets.Scripts.MapParser;
using JoltCSharp;
using jtshared;
using UnityEngine;

public class TrapPlaceholder : MonoBehaviour {

    public uint id;
    public uint tpt;
    public OptionalInt quota;

    public Vector3 initVel;

    public Vector3 initAngVel;
    public OptionalInt cooldownRdfCount;
    public Vector3 sliderAxis;

    public OptionalFloat pseudoKinematicFactor;

    public OptionalUint subscribesToTriggerId;

    public OptionalInt hp; 
    public bool destructible;
    public bool allowsRotationFromPhySys;

    public OptionalFloat friction;
    public OptionalFloat restitution;
    public OptionalFloat colliderDensity;
    public OptionalFloat gravityFactor;

    // Used for constraint limit assignment
    public OptionalFloat limit1;
    public OptionalFloat limit2;
    public OptionalFloat limit3;
    public OptionalFloat limit4;

    public bool initNotMoving; // thus only moves or rotates after triggered

    public MapColliderOptions colliderOptions;

    // Start is called once before the first execution of Update after the MonoBehaviour is created
    void Start() {
        
    }

    // Update is called once per frame
    void Update() {
        
    }

    public TrapConfigFromTiled buildTrapConfigFromTiled() {

        var colliderAttr = new BarrierColliderAttr {
            ProvidesSlipJump = colliderOptions.providesSlipJump,
            ProvidesStairsP = colliderOptions.providesStairsP,
            ProvidesStairsN = colliderOptions.providesStairsN,
            ProhibitsWallGrabbing = !colliderOptions.providesWallGrabbing,
        };

        var ret = new TrapConfigFromTiled {
            Id = id,
            Name = name,
            Tpt = tpt,

            InitX = gameObject.transform.position.x,
            InitY = gameObject.transform.position.y,
            InitZ = gameObject.transform.position.z,

            InitQX = gameObject.transform.rotation.x,
            InitQY = gameObject.transform.rotation.y,
            InitQZ = gameObject.transform.rotation.z,
            InitQW = gameObject.transform.rotation.w,

            InitVelX = initVel.x,
            InitVelY = initVel.y,
            InitVelZ = initVel.z,

            InitAngVelX = initAngVel.x,
            InitAngVelY = initAngVel.y,
            InitAngVelZ = initAngVel.z,

            SliderAxisX = sliderAxis.x,
            SliderAxisY = sliderAxis.y,
            SliderAxisZ = sliderAxis.z,

            Destructible = destructible,
            AllowsRotationFromPhySys = allowsRotationFromPhySys,
            InitNotMoving = initNotMoving,

            BarrierAttr = colliderAttr,
        };

        if (pseudoKinematicFactor.configured) {
            ret.PseudoKinematicFactor = pseudoKinematicFactor.value;
        }

        if (quota.configured) {
            ret.Quota = quota.value;
        } else {
            ret.Quota = PbPrimitivesOverride.Instance.getUnderlying().MagicQuotaInfinite;
        }

        if (subscribesToTriggerId.configured) {
            ret.SubscribesToTriggerId = subscribesToTriggerId.value;
        } else {
            ret.SubscribesToTriggerId = PbPrimitivesOverride.Instance.getUnderlying().TerminatingTriggerId;
        }

        if (cooldownRdfCount.configured) {
            ret.CooldownRdfCount = cooldownRdfCount.value;
        } else {
            ret.CooldownRdfCount = PbPrimitivesOverride.Instance.getUnderlying().MagicRdfCntInfinite;
        }

        if (hp.configured) {
            ret.Hp = hp.value;
        }

        if (friction.configured) {
            ret.Friction = friction.value;
        }

        if (restitution.configured) {
            ret.Restitution = restitution.value;
        }

        if (colliderDensity.configured) {
            ret.ColliderDensity = colliderDensity.value;
        }

        if (gravityFactor.configured) {
            ret.GravityFactor = gravityFactor.value;
        }

        if (limit1.configured) {
            ret.Limit1 = limit1.value;
        }

        if (limit2.configured) {
            ret.Limit2 = limit2.value;
        }

        if (limit3.configured) {
            ret.Limit3 = limit3.value;
        }

        if (limit4.configured) {
            ret.Limit4 = limit4.value;
        }

        var animatorChild = gameObject.GetComponentInChildren<Animator>();
        Renderer renderer = animatorChild.GetComponent<Renderer>();
        
        Bounds worldBounds = renderer.bounds;
        Vector3 center = worldBounds.center;
        Vector3 scaledSize = worldBounds.size;
        ret.BoxHalfSizeX = scaledSize.x * 0.5f;
        ret.BoxHalfSizeY = scaledSize.y * 0.5f;
        ret.RenderBoxHalfSizeX = scaledSize.x * 0.5f;
        ret.RenderBoxHalfSizeY = scaledSize.y * 0.5f;

        var collidingSectionOverride = MapLayer.parseSerializableConvexPolygon(gameObject.transform.Find("CollidingSectionOverride"));
        if (null != collidingSectionOverride) {
            ret.CollidingSectionOverride = collidingSectionOverride;
        }

        return ret;
    }
}
