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
        map.DisposeRejoinTimer("SteamRejoinPrompt.OnDisable");
    }
}
