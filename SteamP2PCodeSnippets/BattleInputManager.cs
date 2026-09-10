using JoltCSharp;
using System;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.InputSystem;
using UnityEngine.InputSystem.OnScreen;
using UnityEngine.UI;

public class BattleInputManager : MonoBehaviour {
    private float joystickX, joystickY;

    private ulong realtimeBtnALevel = 0;
    private ulong cachedBtnALevel = 0;
    private bool btnAEdgeTriggerLock = false;

    private ulong realtimeBtnBLevel = 0;
    private ulong cachedBtnBLevel = 0;
    private bool btnBEdgeTriggerLock = false;

    private ulong realtimeBtnCLevel = 0;
    private ulong cachedBtnCLevel = 0;
    private bool btnCEdgeTriggerLock = false;

    private ulong realtimeBtnDLevel = 0;
    private ulong cachedBtnDLevel = 0;
    private bool btnDEdgeTriggerLock = false;

    private ulong realtimeBtnELevel = 0;
    private ulong cachedBtnELevel = 0;
    private bool btnEEdgeTriggerLock = false;

    private ulong realtimeBtnFLevel = 0;
    private ulong cachedBtnFLevel = 0;
    private bool btnFEdgeTriggerLock = false;

    private ulong realtimeBtnLLevel = 0;
    private ulong cachedBtnLLevel = 0;
    private bool btnLEdgeTriggerLock = false;

    private ulong realtimeBtnRLevel = 0;
    private ulong cachedBtnRLevel = 0;
    private bool btnREdgeTriggerLock = false;

    private Vector2 joystickInitPos;
    private float joystickKeyboardMoveRadius;
    private float joystickMoveEps;
    public GameObject joystick;
    public GameObject btnA;
    public GameObject btnB;
    public GameObject btnC;
    public GameObject btnD;
    public GameObject btnE;
    public GameObject btnF;
    private bool customEnabled = true;

    public bool enablePlatformSpecificHiding = false;
    public Image joystickImg;
    public Sprite joystickIdle, joystickLeft, joystickRight, joystickUp, joystickDown, joystickDownLeft, joystickDownRight, joystickUpLeft, joystickUpRight;

    private AbstractJoltMapController map;
    public void SetMap(in AbstractJoltMapController val) {
        map = val;
    }
    
    private const float magicLeanLowerBound = 0.1f;
    private const float magicLeanUpperBound = 0.9f;

    public InputActionAsset inputActionAsset;

    void Start() {
        initPlayerInput();
        initCancelBtnPointerClickHandler();
        joystickInitPos = joystick.transform.position;
        joystickKeyboardMoveRadius = 0.5f*joystick.GetComponent<OnScreenStick>().movementRange;
        joystickMoveEps = 0.1f;
        ResetSelf();
    }

    void OnEnable() {
        
    }

    public void OnBtnAInput(InputAction.CallbackContext context) {
        if (!customEnabled) return;
        bool rising = context.ReadValueAsButton();
        _triggerEdgeBtnA(rising);
    }

    public void OnBtnBInput(InputAction.CallbackContext context) {
		if (!customEnabled) return;
        bool rising = context.ReadValueAsButton();
        _triggerEdgeBtnB(rising);
    }

    public void OnBtnCInput(InputAction.CallbackContext context) {
        if (!customEnabled) return;
        bool rising = context.ReadValueAsButton();
        _triggerEdgeBtnC(rising);
    }

    public void OnBtnDInput(InputAction.CallbackContext context) {
        if (!customEnabled) return;
        bool rising = context.ReadValueAsButton();
        _triggerEdgeBtnD(rising);
    }

    public void OnBtnEInput(InputAction.CallbackContext context) {
        if (!customEnabled) return;
        bool rising = context.ReadValueAsButton();
        _triggerEdgeBtnE(rising);
        //Debug.LogFormat("btnELevel is changed to {0}", realtimeBtnELevel);
    }

    public void OnBtnFInput(InputAction.CallbackContext context) {
        if (!customEnabled) return;
        bool rising = context.ReadValueAsButton();
        _triggerEdgeBtnF(rising);
        //Debug.LogFormat("btnFLevel is changed to {0}", realtimeBtnFLevel);
    }

    public void OnBtnLInput(InputAction.CallbackContext context) {
        if (!customEnabled) return;
        bool rising = context.ReadValueAsButton();
        _triggerEdgeBtnL(rising);
        //Debug.LogFormat("btnLLevel is changed to {0}", realtimeBtnLLevel);
    }

    public void OnBtnRInput(InputAction.CallbackContext context) {
        if (!customEnabled) return;
        bool rising = context.ReadValueAsButton();
        _triggerEdgeBtnR(rising);
        //Debug.LogFormat("btnRLevel is changed to {0}", realtimeBtnRLevel);
    }

    public void onBtnCancelInput(InputAction.CallbackContext context) {
        if (!customEnabled) return;
        if (context.performed) {
            OnCancel(null);
        }
    }

    protected void OnCancel(BaseEventData eventData) {
        attemptToCancelBattle();
    }

    public void attemptToCancelBattle() {
        map.OnSettingsClicked();
    }

    public void OnMove(InputAction.CallbackContext context) {
		if (!customEnabled) return;
        joystickX = context.ReadValue<Vector2>().normalized.x;
        joystickY = context.ReadValue<Vector2>().normalized.y;
        //Debug.Log(String.Format("(joystickX,joystickY) is changed to ({0},{1}) by touch", joystickX, joystickY));
    }

    public void OnMoveByKeyboard(InputAction.CallbackContext context) {
		if (!customEnabled) return;
        Vector2 origVal = context.ReadValue<Vector2>();
        joystickY = origVal.y;
        joystickX = 2 * origVal.x;

        //Debug.Log(String.Format("(joystickX,joystickY) is changed to ({0},{1}) by keyboard", joystickX, joystickY));

        joystick.transform.position = new Vector3(
            joystickInitPos.x + joystickKeyboardMoveRadius * joystickX,
            joystickInitPos.y + joystickKeyboardMoveRadius * joystickY,
            joystick.transform.position.z
        );
    }

    public ulong GetEncodedInput() {
        if (!customEnabled) return 0;

        var btnALevel = cachedBtnALevel;
        var btnBLevel = cachedBtnBLevel;
        var btnCLevel = cachedBtnCLevel;
        var btnDLevel = cachedBtnDLevel;
        var btnELevel = cachedBtnELevel;
        var btnFLevel = cachedBtnFLevel;
        var btnLLevel = cachedBtnLLevel;
        var btnRLevel = cachedBtnRLevel;

        cachedBtnALevel = realtimeBtnALevel;
        cachedBtnBLevel = realtimeBtnBLevel;
        cachedBtnCLevel = realtimeBtnCLevel;
        cachedBtnDLevel = realtimeBtnDLevel;
        cachedBtnELevel = realtimeBtnELevel;
        cachedBtnFLevel = realtimeBtnFLevel;
        cachedBtnLLevel = realtimeBtnLLevel;
        cachedBtnRLevel = realtimeBtnRLevel;

        btnAEdgeTriggerLock = false;
        btnBEdgeTriggerLock = false;
        btnCEdgeTriggerLock = false;
        btnDEdgeTriggerLock = false;
        btnEEdgeTriggerLock = false;
        btnFEdgeTriggerLock = false;
        btnLEdgeTriggerLock = false;
        btnREdgeTriggerLock = false;

        float continuousDx = joystickX;
        float continuousDy = joystickY;
        var (dx, dy, discretizedDir) = DiscretizeDirection(continuousDx, continuousDy, joystickMoveEps);
        // "GetEncodedInput" gets called by "AbstractMapController.doUpdate()", thus a proper spot to update UI
        // TODO: Add sprites on skewed directions.
        switch (discretizedDir) {
        case 1:
            joystickImg.sprite = joystickUp;
            break;
        case 2:
            joystickImg.sprite = joystickDown;
            break;
        case 3:
            joystickImg.sprite = joystickRight;
            break;
        case 4:
            joystickImg.sprite = joystickLeft;
            break;
        case 5:
            joystickImg.sprite = joystickUpRight;
            break;
        case 6:
            joystickImg.sprite = joystickDownLeft;
            break;
        case 7:
            joystickImg.sprite = joystickDownRight;
            break;
        case 8:
            joystickImg.sprite = joystickUpLeft;
            break;
        default:
            joystickImg.sprite = joystickIdle;
            break;
        }
        ulong ret = Bindings.APP_EncodeInput(dx, dy, btnALevel, btnBLevel, btnCLevel, btnDLevel, btnELevel, btnFLevel, btnLLevel, btnRLevel);
        return ret;
    }

	public bool enable(bool yesOrNo) {
        if (customEnabled == yesOrNo) return false;
        customEnabled = yesOrNo;
        ResetSelf(); // reset upon any change of this field!
        
        return true;
	}

    public void TogglePlayerInput(bool yesOrNo) {
        if (null != playerInput) {           
            //Debug.Log($"{this.GetType().Name}.togglePlayerInput triggered with val={val}");
            if (yesOrNo) {
                playerInput.ActivateInput();
            } else {
                playerInput.DeactivateInput();
            }
        }
    }
    
    public void ResetSelf() {
        joystickX = 0;
        joystickY = 0;
        joystickImg.sprite = joystickIdle;

        realtimeBtnALevel = 0;
        cachedBtnALevel = 0;
        btnAEdgeTriggerLock = false;

        realtimeBtnBLevel = 0;
        cachedBtnBLevel = 0;
        btnBEdgeTriggerLock = false;

        realtimeBtnCLevel = 0;
        cachedBtnCLevel = 0;
        btnCEdgeTriggerLock = false;

        realtimeBtnDLevel = 0;
        cachedBtnDLevel = 0;
        btnDEdgeTriggerLock = false;

        realtimeBtnELevel = 0;
        cachedBtnELevel = 0;
        btnEEdgeTriggerLock = false;

        if (enablePlatformSpecificHiding && !Application.isMobilePlatform) {
            joystick.gameObject.SetActive(false);
            btnA.gameObject.SetActive(false); // if "chConfig.UseInventoryBtnB", it'll be later enabled in MapController
            btnB.gameObject.SetActive(false);
            btnE.gameObject.SetActive(false);
            if (null != btnF) {
                btnF.gameObject.SetActive(false);
            }
        }
        btnC.gameObject.SetActive(false);
        btnD.gameObject.SetActive(false);
    }

    private void _triggerEdgeBtnA(bool rising) {
        realtimeBtnALevel = (rising ? 1ul : 0ul);
        if (!btnAEdgeTriggerLock && (1 - realtimeBtnALevel) == cachedBtnALevel) {
            cachedBtnALevel = realtimeBtnALevel;
            btnAEdgeTriggerLock = true;
        }

        if (enablePlatformSpecificHiding && !Application.isMobilePlatform) return; // Save some resources on animating
        /*
        if (rising) {
            btnA.transform.DOScale(0.3f * Vector3.one, 0.2f);
        } else {
            btnA.transform.DOScale(1.0f * Vector3.one, 0.8f);
        }
        */
    }

    private void _triggerEdgeBtnB(bool rising) {
        realtimeBtnBLevel = (rising ? 1ul : 0ul);
        if (!btnBEdgeTriggerLock && (1 - realtimeBtnBLevel) == cachedBtnBLevel) {
            cachedBtnBLevel = realtimeBtnBLevel;
            btnBEdgeTriggerLock = true;
        }

        if (enablePlatformSpecificHiding && !Application.isMobilePlatform) return; // Save some resources on animating
        /*
        if (rising) {
            btnB.transform.DOScale(0.3f * Vector3.one, 0.2f);
        } else {
            btnB.transform.DOScale(1.0f * Vector3.one, 0.8f);
        }
        */
    }

    private void _triggerEdgeBtnC(bool rising) {
        realtimeBtnCLevel = (rising ? 1ul : 0ul);
        if (!btnCEdgeTriggerLock && (1 - realtimeBtnCLevel) == cachedBtnCLevel) {
            cachedBtnCLevel = realtimeBtnCLevel;
            btnCEdgeTriggerLock = true;
        }

        /*
        if (rising) {
            btnC.transform.DOScale(0.3f * Vector3.one, 0.2f);
        } else {
            btnC.transform.DOScale(1.0f * Vector3.one, 0.8f);
        }
        */
    }

    private void _triggerEdgeBtnD(bool rising) {
        realtimeBtnDLevel = (rising ? 1ul : 0ul);
        if (!btnDEdgeTriggerLock && (1 - realtimeBtnDLevel) == cachedBtnDLevel) {
            cachedBtnDLevel = realtimeBtnDLevel;
            btnDEdgeTriggerLock = true;
        }

        /*
        if (rising) {
            btnD.transform.DOScale(0.3f * Vector3.one, 0.5f);
        } else {
            btnD.transform.DOScale(1.0f * Vector3.one, 0.8f);
        }
        */
    }

    private void _triggerEdgeBtnE(bool rising) {
        realtimeBtnELevel = (rising ? 1ul : 0ul);
        if (!btnEEdgeTriggerLock && (1 - realtimeBtnELevel) == cachedBtnELevel) {
            cachedBtnELevel = realtimeBtnELevel;
            btnEEdgeTriggerLock = true;
        }

        /*
        if (rising) {
            btnE.transform.DOScale(0.3f * Vector3.one, 0.5f);
        } else {
            btnE.transform.DOScale(1.0f * Vector3.one, 0.8f);
        }
        */
    }

    private void _triggerEdgeBtnF(bool rising) {
        realtimeBtnFLevel = (rising ? 1ul : 0ul);
        if (!btnFEdgeTriggerLock && (1 - realtimeBtnFLevel) == cachedBtnFLevel) {
            cachedBtnFLevel = realtimeBtnFLevel;
            btnFEdgeTriggerLock = true;
        }

        /*
        if (rising) {
            btnF.transform.DOScale(0.3f * Vector3.one, 0.5f);
        } else {
            btnF.transform.DOScale(1.0f * Vector3.one, 0.8f);
        }
        */
    }

    private void _triggerEdgeBtnL(bool rising) {
        realtimeBtnLLevel = (rising ? 1ul : 0ul);
        if (!btnLEdgeTriggerLock && (1 - realtimeBtnLLevel) == cachedBtnLLevel) {
            cachedBtnLLevel = realtimeBtnLLevel;
            btnLEdgeTriggerLock = true;
        }
    }

    private void _triggerEdgeBtnR(bool rising) {
        realtimeBtnRLevel = (rising ? 1ul : 0ul);
        if (!btnREdgeTriggerLock && (1 - realtimeBtnRLevel) == cachedBtnRLevel) {
            cachedBtnRLevel = realtimeBtnRLevel;
            btnREdgeTriggerLock = true;
        }
    }

    public void resumeScales() {
        btnA.transform.localScale = Vector3.one;
        btnB.transform.localScale = Vector3.one;
        btnC.transform.localScale = Vector3.one;
        btnD.transform.localScale = Vector3.one;
        btnE.transform.localScale = Vector3.one;
        if (null != btnF) {
            btnF.transform.localScale = Vector3.one;
        }
    }

    public static (int, int, int) DiscretizeDirection(float continuousDx, float continuousDy, float eps = 0.1f, bool mustHaveNonZeroX = false) {
        int dx = 0, dy = 0, encodedIdx = 0;
        float absContinuousDx = Math.Abs(continuousDx);
        float absContinuousDy = Math.Abs(continuousDy);

        if (absContinuousDx < eps && absContinuousDy < eps) {
            return (dx, dy, encodedIdx);
        }
        float criticalRatio = continuousDy / continuousDx;
        float absCriticalRatio = Math.Abs(criticalRatio);
        float downEps = 5*eps; // dragging down is often more tentative for a player, thus give it a larger threshold!

        if (absCriticalRatio < magicLeanLowerBound && eps < absContinuousDx) {
            dy = 0;
            if (0 < continuousDx) {
                dx = +2; // right 
                encodedIdx = 3;
            } else {
                dx = -2; // left 
                encodedIdx = 4;
            }
        } else if (absCriticalRatio > magicLeanUpperBound && eps < absContinuousDy) {
            dx = 0;
            if (0 < continuousDy) {
                dy = +2; // up
                encodedIdx = 1;
            } else if (downEps < absContinuousDy) {
                dy = -2; // down
                encodedIdx = 2;
            } else {
                // else stays at "encodedIdx == 0" 
            }

            if (mustHaveNonZeroX) {
                if (0 == continuousDx) {
                    if (0 < dy) {
                        dx = +1;
                    } else {
                        dx = -1;
                    }
                } else if (0 < continuousDx) {
                    dx = +1;
                } else {
                    dx = -1;
                }

                if (0 < dx) {
                    if (0 < dy) {
                        dy = +1;
                        encodedIdx = 5;
                    } else {
                        dy = -1;
                        encodedIdx = 7;
                    }
                } else {
                    dx = -1;
                    if (0 < dy) {
                        dy = +1;
                        encodedIdx = 8;
                    } else {
                        dy = -1;
                        encodedIdx = 6;
                    }
                }
            }
        } else if (eps < absContinuousDx && eps < absContinuousDy) {
            if (0 < continuousDx) {
                dx = +1;
                if (0 < continuousDy) {
                    dy = +1;
                    encodedIdx = 5;
                } else {
                    if (downEps < absContinuousDy) {
                        dy = -1;
                        encodedIdx = 7;
                    } else {
                        dx = +2; // right 
                        encodedIdx = 3;
                    }
                } 
            } else {
                // 0 > continuousDx
                dx = -1;
                if (0 < continuousDy) {
                    dy = +1;
                    encodedIdx = 8;
                } else {
                    if (downEps < absContinuousDy) {
                        dy = -1;
                        encodedIdx = 6;
                    } else {
                        dx = -2; // left 
                        encodedIdx = 4;
                    }
                }
            }
        } else {
            // just use encodedIdx = 0
        }

        return (dx, dy, encodedIdx);
    }

    /////////////////////////////////////////////////////////////////////////////////////////
    protected PlayerInput playerInput;
    public Image cancelBtn;

    protected virtual void initPlayerInput() {
        playerInput = GetComponent<PlayerInput>();
        if (null == playerInput) {
            playerInput = gameObject.AddComponent<PlayerInput>();
            //Debug.Log($"PlayerInput instance created for {this.GetType().Name}");
            playerInput.actions = inputActionAsset;
            playerInput.notificationBehavior = PlayerNotifications.InvokeCSharpEvents;
            if (null != playerInput && PlayerNotifications.InvokeCSharpEvents == playerInput.notificationBehavior) {
                playerInput.onActionTriggered -= defaultActionDispatcher;
                playerInput.onActionTriggered += defaultActionDispatcher;
            }
            var defaultActionMap = inputActionAsset.actionMaps[0];
            playerInput.defaultActionMap = defaultActionMap.name;
            playerInput.currentActionMap = defaultActionMap;
        } else {
            // Otherwise it's already initialized in Unity Editor
        }
    }

    protected class CancelBtnTrigger : EventTrigger {
        private BattleInputManager iptMgr;
        public void setBattleInputManager(BattleInputManager theIptMgr) {
            iptMgr = theIptMgr;
        }

        public override void OnPointerClick(PointerEventData eventData) {
            iptMgr.OnCancel(eventData);
        }
    }

    protected virtual void initCancelBtnPointerClickHandler() {
        if (null == cancelBtn) return;
        var existingEventTrigger = cancelBtn.GetComponent<CancelBtnTrigger>();
        if (null == existingEventTrigger) {
            existingEventTrigger = cancelBtn.gameObject.AddComponent<CancelBtnTrigger>();
            existingEventTrigger.setBattleInputManager(this);
        }
    }

    protected void defaultActionDispatcher(InputAction.CallbackContext context) {
        switch (context.action.name) {
            case "Move":
                OnMove(context);
                break;
            case "MoveByKeyboard":
                OnMoveByKeyboard(context);
                break;
            case "BtnA":
                OnBtnAInput(context);
                break;
            case "BtnB":
                OnBtnBInput(context);
                break;
            case "BtnC":
                OnBtnCInput(context);
                break;
            case "BtnD":
                OnBtnDInput(context);
                break;
            case "BtnE":
                OnBtnEInput(context);
                break;
            case "BtnF":
                OnBtnFInput(context);
                break;
            case "BtnL":
                OnBtnLInput(context);
                break;
            case "BtnR":
                OnBtnRInput(context);
                break;
            case "BtnCancel":
                onBtnCancelInput(context);
                break;
        }
    }
}
