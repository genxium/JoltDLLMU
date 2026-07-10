using JoltCSharp;
using jtshared;
using UnityEngine;

public class GameplayBtnsHintAnimController : AbstractCacheableAnimNode<Trigger, TriggerState, TriggerConfigFromTiled, uint> {

    public GameObject patternFGroup;
    public GameObject slidingGroup;
    public GameObject slipJumpGroup;

    public void SetForPatternF() {
        patternFGroup.gameObject.SetActive(true);
        slidingGroup.gameObject.SetActive(false);
        slipJumpGroup.gameObject.SetActive(false);
    }

    public void SetForSliding() {
        patternFGroup.gameObject.SetActive(true);
        slidingGroup.gameObject.SetActive(false);
        slipJumpGroup.gameObject.SetActive(false);
    }

    public void SetForSlipJump() {
        patternFGroup.gameObject.SetActive(false);
        slidingGroup.gameObject.SetActive(true);
        slipJumpGroup.gameObject.SetActive(false);
    }

    public GameplayBtnsHintAnimController() {
        SetUd(PbPrimitivesOverride.Instance.getUnderlying().TerminatingTriggerId);
        SetCacheGroupId(PbPrimitivesOverride.Instance.getUnderlying().Trts.None);
    }

    protected new Animator getMainAnimator() {
        return null;
    }

    protected override bool lazyInit() {
        return true;
    }

    protected override bool updateAnimUnderlying(in int currRdfId, in Trigger trigger, in TriggerState newState, in TriggerConfigFromTiled insConfig, in int frameIdxInAnim) {
        SetCacheGroupId(insConfig.Trt);

        if (PbPrimitivesOverride.Instance.getUnderlying().Trts.ByPatternF == insConfig.Trt) {
            SetForPatternF();
        } else {
            // [TODO] Other types
        }

        return true;
    }
}
