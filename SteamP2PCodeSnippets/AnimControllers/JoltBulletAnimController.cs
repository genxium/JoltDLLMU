using UnityEngine;
using jtshared;
using System;
using System.Collections.Generic;
using TMPro;
using JoltCSharp;

public class JoltBulletAnimController : AbstractCacheableAnimNode<Bullet, BulletState, BulletConfig, string> {
    public JoltBulletAnimController() {
        SetUd(PbPrimitivesOverride.Instance.getUnderlying().TerminatingBulletId);
        SetCacheGroupId("");
    }

    private string MATERIAL_REF_THICKNESS = "_Thickness";
    private float ANCHOR_DAMAGE_DEALED_INDICATOR_H = 10f;
    private float MAX_DAMAGE_DEALED_INDICATOR_H = 20f;

    private static Color DAMAGE_FONT_COLOR = new Color(0xDB / 255f, 0x0E / 255f, 0x37 / 255f); 
    private static Color HP_HEAL_FONT_COLOR = new Color(0x00 / 255f, 0xFF / 255f, 0x00 / 255f); 

    private bool hideDamageIndicator = false;
    public void SetHideDamageIndicator(bool val) {
        hideDamageIndicator = val;
    }

    public GameObject damageDealedIndicatorPrefab;
    public TMP_Text damageDealedIndicator;

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
        spr.sortingLayerName = "EmittingBullet";
        lookUpTable = new Dictionary<BulletState, AnimationClip>();
        foreach (AnimationClip clip in animator.runtimeAnimatorController.animationClips) {
            BulletState blState;
            Enum.TryParse(clip.name, out blState);
            lookUpTable[blState] = clip;
        }

        if (null == damageDealedIndicator) {
            GameObject damageDealedObject = Instantiate(damageDealedIndicatorPrefab, Vector3.zero, Quaternion.identity, this.transform);
            damageDealedIndicator = damageDealedObject.GetComponent<TMP_Text>();
        }
        return true;
    }

    protected override bool updateAnimUnderlying(in int currRdfId, in Bullet bullet, in BulletState newState, in BulletConfig bulletConfig, in int frameIdxInAnim) {
        if (!lookUpTable.ContainsKey(newState)) {
            return false;
        }
        SetCacheGroupId(bulletConfig.AnimName);
        if (BulletState.Hit == newState) {
            if (frameIdxInAnim > bulletConfig.HitAnimRdfCnt) {
                return false;
            }
        }
        if (BulletState.Vanishing == newState) {
            if (frameIdxInAnim > bulletConfig.VanishingAnimRdfCnt) {
                return false;
            }
        }
        bool isHit = (BulletState.Hit == newState);
        bool isVanishing = (BulletState.Vanishing == newState);
        spr.transform.localRotation = new Quaternion(bullet.QX, bullet.QY, bullet.QZ, bullet.QW);
        if (null != damageDealedIndicator) {
            if (!isHit || hideDamageIndicator) {
                damageDealedIndicator.gameObject.SetActive(false);
            } else {
                damageDealedIndicator.gameObject.SetActive(true);
                damageDealedIndicator.gameObject.transform.localRotation = spr.transform.localRotation; // To battle rotation induced invisibility.
                positionHolder.Set(0, ANCHOR_DAMAGE_DEALED_INDICATOR_H + (bullet.FramesInBlState < MAX_DAMAGE_DEALED_INDICATOR_H ? bullet.FramesInBlState : MAX_DAMAGE_DEALED_INDICATOR_H), 0);
                damageDealedIndicator.gameObject.transform.localPosition = positionHolder;
                if (0 < bullet.DamageDealed) {
                    damageDealedIndicator.text = ("-" + bullet.DamageDealed.ToString());
                    damageDealedIndicator.color = DAMAGE_FONT_COLOR;
                } else if (0 > bullet.DamageDealed) {
                    damageDealedIndicator.text = ("+" + (-bullet.DamageDealed).ToString());
                    damageDealedIndicator.color = HP_HEAL_FONT_COLOR;
                } else {
                    damageDealedIndicator.text = "";
                }
            }
        }
      
        int targetLayer = 0; // We have only 1 layer, i.e. the baseLayer, playing at any time
        int targetClipIdx = 0; // We have only 1 frame anim playing at any time

        var clipInfoList = animator.GetCurrentAnimatorClipInfo(targetLayer);
        var curClip = (0 >= clipInfoList.Length) ? null : clipInfoList[targetClipIdx].clip;
        var targetClip = lookUpTable[newState];
        bool sameClipName = (null == curClip ? false : targetClip.name.Equals(curClip.name));

        int animLoopingRdfOffset = 0;
        if (BulletState.Active == bullet.BlState) {
            animLoopingRdfOffset = bulletConfig.ActiveAnimLoopingRdfOffset;
        }
        if (BulletState.Vanishing == bullet.BlState) {
            animLoopingRdfOffset = bulletConfig.VanishingAnimLoopingRdfOffset;
        }

        if (0 == animLoopingRdfOffset) {
            if (sameClipName && (null != curClip && curClip.isLooping)) {
                return true;
            }

            var estimatedSecondsInAnim = PbPrimitivesOverride.Instance.getUnderlying().EstimatedSecondsPerRdf * frameIdxInAnim;
            float normalizedTime = (estimatedSecondsInAnim / targetClip.length); // TODO: Anyway to avoid using division here?
            if (1.0f < normalizedTime) {
                normalizedTime = 1.0f;
            } 

            animator.Play(targetClip.name, targetLayer, normalizedTime);
        } else {
            var totRdfCnt = (PbPrimitivesOverride.Instance.getUnderlying().BattleDynamicsFps * targetClip.length);
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
