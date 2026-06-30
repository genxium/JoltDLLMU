using jtshared;
using static jtshared.CharacterState;
using System;
using System.Collections.Generic;
using System.Collections.Immutable;
using UnityEngine;
using JoltCSharp;

public class JoltCharacterAnimController : AbstractCacheableAnimNode<CharacterDownsync, CharacterState, CharacterConfig, uint> {
    public JoltCharacterAnimController() {
        SetUd(PbPrimitivesOverride.Instance.getUnderlying().TerminatingCharacterId);
        SetCacheGroupId(PbPrimitivesOverride.Instance.getUnderlying().ChSpecies.None);
    }

    private string MATERIAL_REF_FLASH_INTENSITY = "_FlashIntensity";
    private float DAMAGED_FLASH_INTENSITY = 0.4f;
    private static int DAMAGED_FLASH_SCALE_FRAMES = 2;
    private static int DEFAULT_FRAMES_TO_FLASH_DAMAGED = (PbPrimitivesOverride.Instance.getUnderlying().DefaultFramesToShowDamaged >> DAMAGED_FLASH_SCALE_FRAMES);
    private static int DEFAULT_FRAMES_TO_SHOW_DAMAGED_REDUCTION = (PbPrimitivesOverride.Instance.getUnderlying().DefaultFramesToShowDamaged - DEFAULT_FRAMES_TO_FLASH_DAMAGED);

    public static ImmutableDictionary<uint, Color> ELE_DAMAGED_COLOR = ImmutableDictionary.Create<uint, Color>()
       .Add(PbPrimitivesOverride.Instance.getUnderlying().Elets.None, Color.white)
       .Add(PbPrimitivesOverride.Instance.getUnderlying().Elets.Fire, new Color(191 / 255f, 5 / 255f, 0 / 255f))
       .Add(PbPrimitivesOverride.Instance.getUnderlying().Elets.Thunder, new Color(191 / 255f, 171 / 255f, 43 / 255f))
       ;

    private bool hasIdle1Charging = false;
    private bool hasWalkingCharging = false;
    private bool hasOnWallCharging = false;
    private bool hasInAirIdleCharging = false;
    private bool hasCrouchCharging = false;

    protected static HashSet<CharacterState> INTERRUPT_WAIVE_SET = new HashSet<CharacterState> {
        Idle1,
        Walking,
        InAirWalking,
        InAirIdle1NoJump,
        InAirIdle1ByJump,
        InAirIdle1ByWallJump,
        BlownUp1,
        LayDown1,
        GetUp1,
        OnWallIdle1
    };

    public Material GetMaterial() {
        if (!lazyInit()) return null;
        return material;
    }

   
    protected override bool lazyInit() {
        if (null != lookUpTable && 0 < lookUpTable.Count) return true;
        animator = getMainAnimator();
        if (null == animator) return false;
        spr = GetComponent<SpriteRenderer>();
        if (null == spr) return false;
        if (null != sprDefaultMaterial) {
            spr.material = sprDefaultMaterial; // [WARNING] This assignment creates a copy of the material for the current SpriteRenderer, thus independent from other SpriteRenderers when being updated.
        }
        material = spr.material;
        spr.sortingLayerName = "Character";
        lookUpTable = new Dictionary<CharacterState, AnimationClip>();
        foreach (AnimationClip clip in animator.runtimeAnimatorController.animationClips) {
            CharacterState chState;
            Enum.TryParse(clip.name, out chState);
            switch (chState) {
            case Atk1Charging:
                hasIdle1Charging = true;
            break;
            case WalkingAtk1Charging:
                hasWalkingCharging = true;
            break;
            case OnWallAtk1Charging:
                hasOnWallCharging = true;
            break;
            case InAirAtk1Charging:
                hasInAirIdleCharging = true;
            break;
            case CrouchAtk1Charging:
                hasCrouchCharging = true;
            break;
            default:
            break;
            } 
            lookUpTable[chState] = clip;
        }
        return true;
    }

    protected override bool updateAnimUnderlying(in int rdfId, in CharacterDownsync rdfCharacter, in CharacterState newCharacterState, in CharacterConfig chConfig, in int framesInNewState) {
        // [WARNING] Being frozen might invoke this function with "newCharacterState != rdfCharacter.ChState" 

        // As this function might be called after many frames of a rollback, it's possible that the playing animation was predicted, different from "prevRdfCharacter.CharacterState" but same as "newCharacterState". More granular checks are needed to determine whether we should interrupt the playing animation.
        SetCacheGroupId(chConfig.SpeciesId);
        facingQ.Set(rdfCharacter.QX, rdfCharacter.QY, rdfCharacter.QZ, rdfCharacter.QW);
        Vector3 chdFacing = facingQ * Vector3.right;
        // Update directions
        if (0 > chdFacing.x) {
            scaleHolder.Set(-1.0f, 1.0f, this.gameObject.transform.localScale.z);
            this.gameObject.transform.localScale = scaleHolder;
        } else if (0 < chdFacing.x) {
            scaleHolder.Set(+1.0f, 1.0f, this.gameObject.transform.localScale.z);
            this.gameObject.transform.localScale = scaleHolder;
        }
        if (OnWallIdle1 == newCharacterState || OnWallAtk1 == newCharacterState || TurnAround == newCharacterState) {
            if (0 < chdFacing.x) {
                scaleHolder.Set(-1.0f, 1.0f, this.gameObject.transform.localScale.z);
                this.gameObject.transform.localScale = scaleHolder;
            } else {
                scaleHolder.Set(+1.0f, 1.0f, this.gameObject.transform.localScale.z);
                this.gameObject.transform.localScale = scaleHolder;
            }
        }

        float flashIntensity = 0;
        var remainingFramesToFlash = (rdfCharacter.DamagedHintRdfCountdown > DEFAULT_FRAMES_TO_SHOW_DAMAGED_REDUCTION) ? (rdfCharacter.DamagedHintRdfCountdown - DEFAULT_FRAMES_TO_SHOW_DAMAGED_REDUCTION) : 0; // Such that the rate of remaining frame reduction is the same as BATTLE_DYNAMICS_FPS
        var frameIdxToPlayDef1Atked = (PbPrimitivesOverride.Instance.getUnderlying().DefaultFramesToShowDamaged - rdfCharacter.DamagedHintRdfCountdown);
        if (0 > frameIdxToPlayDef1Atked) frameIdxToPlayDef1Atked = 0;
        var midway = (DEFAULT_FRAMES_TO_FLASH_DAMAGED >> 1);
        if (remainingFramesToFlash >= midway) {
           flashIntensity = DAMAGED_FLASH_INTENSITY*(DEFAULT_FRAMES_TO_FLASH_DAMAGED - remainingFramesToFlash) / midway; 
        } else {
           flashIntensity = DAMAGED_FLASH_INTENSITY*remainingFramesToFlash / midway;
        }
        material.SetFloat(MATERIAL_REF_FLASH_INTENSITY, flashIntensity);
        material.SetInt("_EleDamageFlash", 0);
        material.SetColor("_FlashColor", ELE_DAMAGED_COLOR[PbPrimitivesOverride.Instance.getUnderlying().Elets.None]);
        if (0 < flashIntensity) {
            if (PbPrimitivesOverride.Instance.getUnderlying().Elets.Fire == rdfCharacter.DamagedElementalAttrs) {
                material.SetInt("_EleDamageFlash", 1);
                material.SetColor("_FlashColor", ELE_DAMAGED_COLOR[PbPrimitivesOverride.Instance.getUnderlying().Elets.Fire]);
            } else if (PbPrimitivesOverride.Instance.getUnderlying().Elets.Thunder == rdfCharacter.DamagedElementalAttrs) {
                material.SetInt("_EleDamageFlash", 1);
                material.SetColor("_FlashColor", ELE_DAMAGED_COLOR[PbPrimitivesOverride.Instance.getUnderlying().Elets.Thunder]);
            }
        }
        var effNewChState = newCharacterState;
        if (chConfig.HasBtnBCharging && PbPrimitivesOverride.Instance.getUnderlying().BtnBHoldingRdfCntThreshold2 <= rdfCharacter.BtnBHoldingRdfCnt) {
            switch (newCharacterState) {
            case Idle1:
            if (hasIdle1Charging) {
                effNewChState = Atk1Charging;
            }
            break;
            case Walking:
            if (hasWalkingCharging) {
                effNewChState = WalkingAtk1Charging;
            }
            break;
            case InAirIdle1NoJump:
            case InAirIdle1ByJump:
            case InAirIdle1BySlipJump:
            if (hasInAirIdleCharging) {
                effNewChState = InAirAtk1Charging;
            }
            break;
            case InAirIdle2ByJump:
            case InAirIdle1ByWallJump:
            if (hasInAirIdleCharging && rdfCharacter.FramesInChState >= chConfig.TrailingRdfChargeableChStates[(int)newCharacterState]) {
                effNewChState = InAirAtk1Charging;
            }
            break;
            case OnWallIdle1:
            if (hasOnWallCharging) {
                effNewChState = OnWallAtk1Charging;
            }
            break;
            case CrouchIdle1:
            if (hasCrouchCharging) {
                effNewChState = CrouchAtk1Charging;
            }
            break;
            default:
            break;
            }
        } else {
            if (InAirIdle1BySlipJump == newCharacterState) {
                effNewChState = InAirIdle1ByJump;
            }
        }
        int targetLayer = 0; // We have only 1 layer, i.e. the baseLayer, playing at any time
        int targetClipIdx = 0; // We have only 1 frame anim playing at any time
        var curClip = animator.GetCurrentAnimatorClipInfo(targetLayer)[targetClipIdx].clip;
        var playingAnimName = curClip.name;
        if ((BlownUp1 == effNewChState || LayDown1 == effNewChState || GetUp1 == effNewChState) && !lookUpTable.ContainsKey(effNewChState)) {
            effNewChState = InAirAtked1; // Fallback
        }
        if (InAirAtked1 == effNewChState && !lookUpTable.ContainsKey(effNewChState)) {
            effNewChState = Atked1; // Fallback
        }
        if (!lookUpTable.ContainsKey(effNewChState)) {
            //Debug.LogWarning(chConfig.SpeciesName + " does not have effNewChState = " + effNewChState + "@FramesInChState=" + rdfCharacter.FramesInChState);
            effNewChState = Idle1; // Ultimate Fallback
        }
        var targetClip = lookUpTable[effNewChState];
        if (null == chConfig.LoopingChStates || !chConfig.LoopingChStates.ContainsKey(((int)effNewChState))) {
            if (playingAnimName.Equals(targetClip.name) && INTERRUPT_WAIVE_SET.Contains(effNewChState)) {
                return true;
            }

            if (INTERRUPT_WAIVE_SET.Contains(newCharacterState)) {
                animator.Play(targetClip.name, targetLayer);
                return true;
            }

            var frameIdxInAnim = (Def1Atked1 == newCharacterState ? frameIdxToPlayDef1Atked : rdfCharacter.FramesInChState);    
            var estimatedSecondsInAnim = PbPrimitivesOverride.Instance.getUnderlying().EstimatedSecondsPerRdf*frameIdxInAnim;
            float normalizedTime = (estimatedSecondsInAnim / targetClip.length); // TODO: Anyway to avoid using division here?
            if (1.0f < normalizedTime) {
                normalizedTime = 1.0f;
            } 
            animator.Play(targetClip.name, targetLayer, normalizedTime);
        } else {
            var totRdfCnt = (PbPrimitivesOverride.Instance.getUnderlying().BattleDynamicsFps * targetClip.length);
            int animLoopingRdfOffset = chConfig.LoopingChStates[((int)effNewChState)];
            var frameIdxInAnim = rdfCharacter.FramesInChState;
            if (frameIdxInAnim > animLoopingRdfOffset) {    
                var frameIdxInAnimFloat = animLoopingRdfOffset + ((frameIdxInAnim - animLoopingRdfOffset) % (totRdfCnt - animLoopingRdfOffset));
                float normalizedTime = (frameIdxInAnimFloat / totRdfCnt); // TODO: Anyway to avoid using division here?
                if (1.0f < normalizedTime) {
                    normalizedTime = 1.0f;
                } 
                animator.Play(targetClip.name, targetLayer, normalizedTime);
            } else {
                float normalizedTime = (frameIdxInAnim / totRdfCnt); // TODO: Anyway to avoid using division here?
                if (1.0f < normalizedTime) {
                    normalizedTime = 1.0f;
                } 
                animator.Play(targetClip.name, targetLayer, normalizedTime);
            }
        }

        return true;
    }
}
