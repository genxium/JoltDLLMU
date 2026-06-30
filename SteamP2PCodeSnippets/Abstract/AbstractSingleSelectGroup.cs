using System;
using UnityEngine;
using UnityEngine.InputSystem;

/*
 [REMINDER] 

 Most setter-methods are intentionally made "protected" to force inheriting classes to implement a more compact public API for exposure.
 */
public abstract partial class AbstractSingleSelectGroup : MonoBehaviour {

    /////////////////////////////////////////////////////////////////////////////////////////
    public bool hideCellUponNullBg = false;
    public Sprite cellBgSelectedSpr, cellBgUnselectedSpr;

    /*
     [REMINDER] Unlike "postConfirmedCallback", a "postCancelledCallback" is ALWAYS a "SelectPanel-scope" concept, see "AbstractSingleSelectGroup.AbstractSettingSelectPanel" for cancel-related-features.
     */
    public delegate void PostCursorMovedCallbackT();
    public delegate void PostConfirmedCallbackT();


    public void SetCallbacks(in PostConfirmedCallbackT thePostConfirmedCb, in PostCursorMovedCallbackT thePostCursorMovedCb) {
        setPostConfirmedCallback(thePostConfirmedCb);
        setPostCursorMovedCallback(thePostCursorMovedCb);
    }

    public const int OVERFLOWN_MAX = Int32.MaxValue;
    public const int OVERFLOWN_MIN = Int32.MinValue;

    public AbstractSingleSelectCell GetActiveCell() {
        if (0 <= selectedIdx && selectedIdx < cells.Length) {
            return cells[selectedIdx];
        }
        return null;
    }

    /////////////////////////////////////////////////////////////////////////////////////////
    protected virtual void Start() {
        //Debug.Log($"Calling {this.GetType().Name}.Start for cells.Length={cells.Length}");
        holderPanel = gameObject.transform.parent.GetComponent<AbstractSingleSelectPanel>();
        initSelectedIndicesForCells();
    }

    protected Transform popupPanelParent;
    protected AbstractSingleSelectPanel holderPanel;

    protected int selectedIdx = -1;
    protected bool currentSelectGroupEnabled = false;
    protected int subGroupIdx = -1;

    protected void setSubGroupIdx(int val) {
        subGroupIdx = val;
    }

    public AbstractSingleSelectCell[] cells = null;

    protected UISoundSource? uiSoundSource;

    protected virtual void setUiSoundSource(in UISoundSource theUiSoundSource) {
        uiSoundSource = theUiSoundSource;
    }

    protected virtual void clearUiSoundSource() {
        uiSoundSource = null;
    }

    protected PostConfirmedCallbackT postConfirmedCallback;
    protected virtual void setPostConfirmedCallback(PostConfirmedCallbackT theCb) {
        postConfirmedCallback = theCb;
    }

    protected PostCursorMovedCallbackT postCursorMovedCallback;
    protected virtual void setPostCursorMovedCallback(PostCursorMovedCallbackT theCb) {
        postCursorMovedCallback = theCb;
    }

    protected virtual void initSelectedIndicesForCells() {
        for (int idx = 0; idx < cells.Length; idx++) {
            var cell = cells[idx];
            cell.SetSelectedIdx(this, idx);
        }
    }

    protected virtual void onBtnConfirmed(InputAction.CallbackContext context) {
        if (!currentSelectGroupEnabled) return;
        if (InputActionPhase.Performed == context.phase) {
            onConfirmed();
        }
    }

    protected virtual void onConfirmed() {
        if (false == currentSelectGroupEnabled) return;
        if (null != uiSoundSource) {
            uiSoundSource.PlayPositive();
        }
        if (null == postConfirmedCallback) return;
        toggleUIInteractability(false);
        postConfirmedCallback();
    }

    protected virtual void onCellSelected(in int newSelectedIdx, in bool fromProactiveConfirmation) {
        bool shouldInvokePostCursorMovedCb = false;
        if (newSelectedIdx == selectedIdx) {
            if (0 <= newSelectedIdx && newSelectedIdx < cells.Length) {
                if (fromProactiveConfirmation) {
                    onConfirmed();
                } else {
                    cells[selectedIdx].SetSelected(true);
                }
            }
        } else {
            if (null != uiSoundSource) {
                uiSoundSource.PlayCursor();
            }
            if (0 <= selectedIdx && selectedIdx < cells.Length) {
                cells[selectedIdx].SetSelected(false);
            }
            if (0 <= newSelectedIdx && newSelectedIdx < cells.Length) {
                cells[newSelectedIdx].SetSelected(true);
                shouldInvokePostCursorMovedCb = true;
            }
        }
        selectedIdx = newSelectedIdx;
        if (shouldInvokePostCursorMovedCb) {
            if (null != postCursorMovedCallback) {
                postCursorMovedCallback();
            }
        }
    }

    protected virtual int moveSelection(int delta) {
        if (!currentSelectGroupEnabled) return selectedIdx;
        int newSelectedIdx = selectedIdx + delta;
        if (0 > newSelectedIdx) {
            cells[selectedIdx].SetSelected(false);
            selectedIdx = OVERFLOWN_MIN;
            return OVERFLOWN_MIN;
        }
        if (cells.Length <= newSelectedIdx) {
            cells[selectedIdx].SetSelected(false);
            selectedIdx = OVERFLOWN_MAX;
            return OVERFLOWN_MAX;
        }
        onCellSelected(newSelectedIdx, false);
        return newSelectedIdx;
    }

    protected virtual void toggleUIInteractability(bool val) {
        currentSelectGroupEnabled = val;
    }

    protected virtual void resetCells(AbstractSingleSelectCell[] newCells, bool deleteComponent) {
        if (null != cells) {
            foreach (var cell in cells) {
                if (deleteComponent) {
                    Destroy(cell);
                } else {
                    Destroy(cell.gameObject);
                }
            }
        }

        cells = newCells;

        int idx = 0;
        foreach (var cell in cells) {
            cell.SetSelectedIdx(this, idx++);
        }
    }
}
