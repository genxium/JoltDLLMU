using Google.Protobuf;
using jtshared;
using System;
using UnityEngine;

public class SFXSourceAnimPool : AbstractCacheableAnimNodePool<IMessage, Enum, IMessage, string, SFXSourceAnimController> {

    public SFXSourceAnimPool(in AbstractJoltMapController joltMap) : base(joltMap, 0, "") {
    }

    protected override GameObject loadPrefab(IMessage insConfig) {
        return joltMap.loadSfxSourcePrefab();
    }

    public string CalcSfxName(in Bullet bullet, in BulletConfig bulletConfig) {
        switch (bullet.BlState) {
            case BulletState.StartUp:
                switch (bulletConfig.BType) {
                    case BulletType.Melee:
                        return bulletConfig.CharacterEmitSfxName;
                    default:
                        return bulletConfig.FireballEmitSfxName;
                }
            case BulletState.Hit:
                return bulletConfig.HitSfxName;
            default:
                return "";
        }
    }

    public string CalcSfxName(in CharacterDownsync chd, in CharacterConfig chConfig) {
        switch (chd.ChState) {
            case CharacterState.Dashing:
            case CharacterState.BackDashing:
                return chConfig.DashingSfxName;
            case CharacterState.InAirDashing:
            case CharacterState.InAirBackDashing:
                if (!string.IsNullOrEmpty(chConfig.AirDashingSfxName)) {
                    return chConfig.AirDashingSfxName;
                } else {
                    return chConfig.DashingSfxName;
                }
            case CharacterState.Sliding:
                return chConfig.SlidingSfxName;
            case CharacterState.InAirIdle1ByJump:
                return chConfig.JumpingSfxName;
            case CharacterState.InAirIdle1BySlipJump:
                if (!string.IsNullOrEmpty(chConfig.SlipJumpingSfxName)) {
                    return chConfig.SlipJumpingSfxName;
                } else {
                    return chConfig.JumpingSfxName;
                }
            case CharacterState.InAirIdle2ByJump:
                if (!string.IsNullOrEmpty(chConfig.AirJumpingSfxName)) {
                    return chConfig.AirJumpingSfxName;
                } else {
                    return chConfig.JumpingSfxName;
                }
            case CharacterState.InAirIdle1ByWallJump:
                if (!string.IsNullOrEmpty(chConfig.WallJumpingSfxName)) {
                    return chConfig.WallJumpingSfxName;
                } else {
                    return chConfig.JumpingSfxName;
                }
            case CharacterState.Walking:
                return chConfig.WalkingSfxName;
            case CharacterState.InAirWalking:
                if (!string.IsNullOrEmpty(chConfig.AirWalkingSfxName)) {
                    return chConfig.AirWalkingSfxName;
                } else {
                    return chConfig.WalkingSfxName;
                }
            default:
                if (0 != chd.GroundUd) {
                    if (0 < chd.FallstoppingRdfCountdown) {
                        return chConfig.LandingSfxName;
                    }
                }
                return "";
        }
    }
}
