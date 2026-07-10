using JoltCSharp;
using jtshared;
using UnityEngine;

public class GameplayBtnsHintAnimPool : AbstractCacheableAnimNodePool<Trigger, TriggerState, TriggerConfigFromTiled, uint, GameplayBtnsHintAnimController> {

    public GameplayBtnsHintAnimPool(in AbstractJoltMapController joltMap) : base(joltMap, Bindings.APP_CalcTriggerUserData(PbPrimitivesOverride.Instance.getUnderlying().TerminatingTriggerId), PbPrimitivesOverride.Instance.getUnderlying().Trts.None) {
    }

    protected override GameObject loadPrefab(TriggerConfigFromTiled insConfig) {
        return joltMap.loadGameplayBtnsHintPrefab();
    }
}
