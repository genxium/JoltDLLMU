using UnityEngine;
using UnityEngine.UI;
using UnityEngine.InputSystem;
using UnityEngine.InputSystem.Controls;
using UnityEngine.EventSystems;

public abstract partial class AbstractSingleSelectGroup {

    /*
    [REMINDER] Using nested-class to access protected member variables & methods of "AbstractSingleSelectGroup".
    */

    public abstract class AbstractSingleSelectPanel : MonoBehaviour, ICancelHandler {
    
        /////////////////////////////////////////////////////////////////////////////////////////
        public InputActionAsset inputActionAsset;   
    
        public delegate void PostCancelledCallbackT();
        public Image cancelBtn;

        protected virtual void Start() {
            initPlayerInput();
            initCancelBtnPointerClickHandler();
            initPostCursorMovedCallbacksForSubGroups();
            TogglePlayerInput(currentSelectPanelEnabled);
        }

        protected virtual void OnEnable() {
            TogglePlayerInput(currentSelectPanelEnabled);
        }

        protected virtual void OnDisable() {
            ToggleUIInteractability(false);
            TogglePlayerInput(false);
        }

        public AbstractSingleSelectGroup[] subGroups = null;
        public AbstractSingleSelectGroup GetActiveSubGroup() {
            return subGroups[selectedSubGroupIdx];
        }

        public AbstractSingleSelectCell GetActiveCell() {
            var activeSubGroup = GetActiveSubGroup();
            return activeSubGroup.GetActiveCell();
        }
        
        public virtual void OnBtnConfirmed(InputAction.CallbackContext context) {
            var activeSubGroup = GetActiveSubGroup();
            activeSubGroup.onBtnConfirmed(context);
        }

        public virtual void OnBtnCancel(InputAction.CallbackContext context) {
            if (!currentSelectPanelEnabled) return;
            bool rising = context.ReadValueAsButton();
            if (rising && InputActionPhase.Performed == context.phase) {
                OnCancel(null);
            }
        }

        public virtual void OnCancel(BaseEventData eventData) {
            if (null != uiSoundSource) {
                uiSoundSource.PlayCancel();
            }
            gameObject.SetActive(false);
            if (null != panelScopeCancelledCb) {
                panelScopeCancelledCb();
            }
        }

        public virtual void OnMoveByKeyboard(InputAction.CallbackContext context) {
            if (!currentSelectPanelEnabled) return;
            var kctrl = (KeyControl)context.control;
            if (InputActionPhase.Performed != context.phase) return;
            switch (kctrl.keyCode) {
                case Key.W:
                case Key.UpArrow: {
                    moveSelection(-1);
                    break;
                }
                case Key.S:
                case Key.DownArrow: {
                    moveSelection(+1);
                    break;
                }
                default: {
                    break;
                }
            }
        }

        public virtual void ToggleUIInteractability(bool val) {
            currentSelectPanelEnabled = val;
            foreach (var subGroup in subGroups) {
                subGroup.toggleUIInteractability(val);
            }
            if (null != cancelBtn) {
                if (val) {
                    cancelBtn.transform.localScale = Vector3.one;
                } else {
                    cancelBtn.transform.localScale = Vector3.zero;
                }
            }
        }

        public void TogglePlayerInput(bool val) {
            if (null != playerInput) {
                //Debug.Log($"{this.GetType().Name}.togglePlayerInput triggered with val={val}");
                if (val) {
                    playerInput.ActivateInput();
                } else {
                    playerInput.DeactivateInput();
                }
            }
        }

        /////////////////////////////////////////////////////////////////////////////////////////
        protected PlayerInput playerInput;
        protected int selectedSubGroupIdx = 0;
        protected bool currentSelectPanelEnabled = false;

        protected UISoundSource? uiSoundSource;
        protected virtual void setUiSoundSource(in UISoundSource theUiSoundSource) {
            uiSoundSource = theUiSoundSource;
            foreach (var subGroup in subGroups) {
                subGroup.setUiSoundSource(theUiSoundSource);
            }
        }

        protected virtual void clearUiSoundSource() {
            uiSoundSource = null;
            foreach (var subGroup in subGroups) {
                subGroup.clearUiSoundSource();
            }
        }

        protected virtual void initPlayerInput() {
            playerInput = GetComponent<PlayerInput>();
            if (null == playerInput) {
                playerInput = gameObject.AddComponent<PlayerInput>();
                //Debug.Log($"PlayerInput instance created for {this.GetType().Name}");
                playerInput.actions = inputActionAsset;
                playerInput.notificationBehavior = PlayerNotifications.InvokeCSharpEvents;
                var defaultActionMap = inputActionAsset.actionMaps[0];
                playerInput.defaultActionMap = defaultActionMap.name;
            } else {
                // Otherwise it's already initialized in Unity Editor
            }

            if (null != playerInput && PlayerNotifications.InvokeCSharpEvents == playerInput.notificationBehavior) {
                playerInput.onActionTriggered -= defaultActionDispatcher;
                playerInput.onActionTriggered += defaultActionDispatcher;
            }
        }

        protected class CancelBtnTrigger : EventTrigger {
            private AbstractSingleSelectPanel selectPanel;
            public void setSelectPanel(AbstractSingleSelectPanel theSelectPanel) {
                selectPanel = theSelectPanel;
            }

            public override void OnPointerClick(PointerEventData eventData) {
                selectPanel.OnCancel(eventData);
            }
        }
        protected virtual void initCancelBtnPointerClickHandler() {
            if (null == cancelBtn) return;
            var existingEventTrigger = cancelBtn.GetComponent<CancelBtnTrigger>();
            if (null == existingEventTrigger) {
                existingEventTrigger = cancelBtn.gameObject.AddComponent<CancelBtnTrigger>();
                existingEventTrigger.setSelectPanel(this);
            }
        }

        protected virtual void initPostCursorMovedCallbacksForSubGroups() {
            for (int subGroupIdx = 0; subGroupIdx < subGroups.Length; subGroupIdx++) {
                var subGroup = subGroups[subGroupIdx];
                if (null == subGroup) continue;
                subGroup.setSubGroupIdx(subGroupIdx);
                subGroup.initSelectedIndicesForCells();
                subGroup.setPostCursorMovedCallback(() => {
                    // For dynamic cursor movement
                    int oldSelectedSubGroupIdx = selectedSubGroupIdx;
                    if (subGroup.subGroupIdx != oldSelectedSubGroupIdx) {
                        var oldSelectedSubGroup = subGroups[oldSelectedSubGroupIdx];
                        oldSelectedSubGroup.onCellSelected(-1, false);
                        selectedSubGroupIdx = subGroup.subGroupIdx;
                    }
                    subGroup.onCellSelected(subGroup.selectedIdx, false);
                });
                // For initialization
                if (subGroupIdx == selectedSubGroupIdx) {
                    subGroup.onCellSelected(0, false);
                } else {
                    subGroup.onCellSelected(-1, false);
                }
            }
        }

        protected PostCancelledCallbackT panelScopeCancelledCb;
        protected virtual void setPanelScopeCallbacks(PostCancelledCallbackT thePanelScopeCancelledCb) {
            panelScopeCancelledCb = thePanelScopeCancelledCb;
        }

        protected virtual void moveSelection(int delta) {
            var oldActiveSelectGroup = GetActiveSubGroup();
            int newSelectedIdx = selectedSubGroupIdx;
            int newSelectedIdxWithinActiveGroup = oldActiveSelectGroup.moveSelection(delta);
            if (AbstractSingleSelectGroup.OVERFLOWN_MIN == newSelectedIdxWithinActiveGroup) {
                newSelectedIdx = selectedSubGroupIdx - 1;
                if (0 > newSelectedIdx) {
                    newSelectedIdx = 0;
                    var newActiveSubGroup = subGroups[newSelectedIdx];
                    newActiveSubGroup.onCellSelected(0, false);
                } else {
                    oldActiveSelectGroup.onCellSelected(-1, false);
                    var newActiveSubGroup = subGroups[newSelectedIdx];
                    newActiveSubGroup.onCellSelected(newActiveSubGroup.cells.Length - 1, false);
                }
            } else if (AbstractSingleSelectGroup.OVERFLOWN_MAX == newSelectedIdxWithinActiveGroup) {
                newSelectedIdx = selectedSubGroupIdx + 1;
                if (subGroups.Length <= newSelectedIdx) {
                    oldActiveSelectGroup.onCellSelected(-1, false);
                    newSelectedIdx = subGroups.Length - 1;
                    var newActiveSubGroup = subGroups[newSelectedIdx];
                    newActiveSubGroup.onCellSelected(newActiveSubGroup.cells.Length - 1, false);
                } else {
                    var newActiveSubGroup = subGroups[newSelectedIdx];
                    newActiveSubGroup.onCellSelected(0, false);
                }
            }

            selectedSubGroupIdx = newSelectedIdx;
        }

        protected virtual void defaultActionDispatcher(InputAction.CallbackContext context) {
            if (InputActionPhase.Performed != context.phase) return;
            //Debug.Log($"{this.GetType().Name}.defaultActionDispatcher triggered with context.action.name={context.action.name}");
            switch (context.action.name) {
                case "MoveByKeyboard":
                    OnMoveByKeyboard(context);
                    break;
                case "BtnConfirm":
                    OnBtnConfirmed(context);
                    break;
                case "BtnCancel":
                    OnBtnCancel(context);
                    break;
            }
        }
    }
}
