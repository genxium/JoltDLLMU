using JoltCSharp;
using System.Threading;
using TMPro;

public class SteamRejoinPrompt : AbstractSingleSelectGroup.AbstractSingleSelectPanel {
    public SysBtnsHintController sysBtnsHint;
    public ConfirmOrCancelSelectGroup yesOrNoSelectGroup;
    public TMP_Text hint;

    public void SetCallbacks(in SteamOnlineMapController theMap, in UISoundSource theUiSoundSource, in PostCancelledCallbackT thePostScopeCancelledCb) {
        base.setUiSoundSource(theUiSoundSource);
        base.setPanelScopeCallbacks(thePostScopeCancelledCb);
        map = theMap;
        yesOrNoSelectGroup.SetCallbacks(thePostConfirmedCb: () => {
            var selectedCell = yesOrNoSelectGroup.GetActiveCell();
            if (null == selectedCell) {
                return;
            }
            var selectedIdx = selectedCell.GetSelectedIdx();
            switch (selectedIdx) {
                case 0:
                    ToggleUIInteractability(false);
                    TogglePlayerInput(false);
                    map.AttemptToRejoinBattle();
                    triggerRejoinTimer();
                    break;
                case 1:
                    map.OnBattleStopped("Exit from rejoinPrompt");
                    OnCancel(null);
                    break;
                default:
                    break;
            }
        }, thePostCursorMovedCb: () => {

        });
    }

    public override void ToggleUIInteractability(bool val) {
        base.ToggleUIInteractability(val);
        if (val) {
            hint.text = "You're disconnected, please choose an action";
            yesOrNoSelectGroup.gameObject.SetActive(true);
        } else {
            hint.text = "Rejoining, please wait...";
            yesOrNoSelectGroup.gameObject.SetActive(false);
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////////
    protected SteamOnlineMapController map;
    private Timer rejoinTimer = null;

    protected override void Start() {
        base.Start();
        if (null != sysBtnsHint) {
            sysBtnsHint.setForConfirmOrCancelSelect();
        }
    }

    protected override void OnEnable() {
        base.OnEnable();
        if (null != sysBtnsHint) {
            sysBtnsHint.setForConfirmOrCancelSelect();
        }
    }

    protected override void OnDisable() {
        base.OnDisable();
        disposeRejoinTimer("SteamRejoinPrompt.OnDisable");
    }

    protected void disposeRejoinTimer(in string motivation) {
        Debug.Log($"{motivation}: disposeRejoinTimer");
        if (null != rejoinTimer) {
            rejoinTimer.Dispose();
            rejoinTimer = null;
        }
    }

    protected void triggerRejoinTimer() {
        disposeRejoinTimer("SteamRejoinPrompt.triggerRejoinTimer");
        long timeoutMillis = 3000;
        rejoinTimer = new Timer(new TimerCallback((object s) => {
            long battleState = map.GetBattleState();
            if (PbPrimitivesOverride.ROOM_STATE_IN_BATTLE == battleState) {
                Debug.Log($"rejoinTimer ticked with battleState=ROOM_STATE_IN_BATTLE");
                OnCancel(null);
            } else {
                map.OnRejoinFailed(battleState);
            }
        }), this, timeoutMillis, Timeout.Infinite);
    }
}
