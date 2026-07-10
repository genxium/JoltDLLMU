using JoltCSharp;
using jtshared;
using UnityEngine;
using UnityEngine.Experimental.GlobalIllumination;
using UnityEngine.Rendering.Universal;

public class KeyChLightSourceAnimController : AbstractCacheableAnimNode<CharacterDownsync, CharacterState, CharacterConfig, uint> {
    private bool initialized = false;
    public Light2D spotLight;

    public KeyChLightSourceAnimController() {
        SetUd(0);
        SetCacheGroupId(0);
    }

    protected new Animator getMainAnimator() {
        return null;
    }

    private Quaternion facingXPlus = Quaternion.AngleAxis(-90, new Vector3(0, 0, 1));
    private Quaternion facingXMinus = Quaternion.AngleAxis(+90, new Vector3(0, 0, 1));

    protected override bool lazyInit() {
        if (initialized) return true;
        initialized = true;
        spotLight.pointLightInnerRadius = 0;
        spotLight.pointLightOuterRadius = 0;
        spotLight.pointLightInnerAngle = 360f;
        spotLight.pointLightOuterAngle = 360f;
        spotLight.falloffIntensity = 0.25f;
        return true;
    }

    protected override bool updateAnimUnderlying(in int currRdfId, in CharacterDownsync chd, in CharacterState newState, in CharacterConfig insConfig, in int frameIdxInAnim) {
        SetCacheGroupId(insConfig.SpeciesId);
        facingQ.Set(chd.QX, chd.QY, chd.QZ, chd.QW);
        Vector3 chdFacing = facingQ * Vector3.right;
        // Update directions
        if (0 > chdFacing.x) {
            this.gameObject.transform.localRotation = facingXMinus;
        } else if (0 < chdFacing.x) {
            this.gameObject.transform.localRotation = facingXPlus;
        }
        if (CharacterState.OnWallIdle1 == newState || CharacterState.OnWallAtk1 == newState || CharacterState.TurnAround == newState) {
            if (0 < chdFacing.x) {
                this.gameObject.transform.localRotation = facingXMinus;
            } else {
                this.gameObject.transform.localRotation = facingXPlus;
            }
        }

        spotLight.pointLightInnerRadius = insConfig.CapsuleRadius * 4;
        spotLight.pointLightOuterRadius = insConfig.CapsuleHalfHeight * 10;

        return true;
    }
}
