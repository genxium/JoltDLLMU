using jtshared;
using UnityEngine;
using Google.Protobuf;
using System;

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
            case CharacterState.InAirIdle1ByJump:
            case CharacterState.InAirIdle1BySlipJump:
            case CharacterState.InAirIdle2ByJump:
            case CharacterState.InAirIdle1ByWallJump:
                return "Jump1"; // [TODO] Allow configuration
            case CharacterState.Walking:
                if (0 != chd.GroundUd) {
                    return "FootStep1"; // [TODO] Allow configuration
                }
                return "";
            case CharacterState.Idle1:
                if (0 != chd.GroundUd) {
                    if (0 < chd.FallstoppingRdfCountdown) {
                        return "Landing1"; // [TODO] Allow configuration
                    }
                }
                return "";
            default:
                return "";
        }
    }
}
