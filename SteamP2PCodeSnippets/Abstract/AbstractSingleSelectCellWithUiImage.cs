using TMPro;
using UnityEngine;
using UnityEngine.UI;

public abstract class AbstractSingleSelectCellWithUiImage : AbstractSingleSelectGroup.AbstractSingleSelectCell {
    /////////////////////////////////////////////////////////////////////////////////////////
    public TMP_Text title;

    /////////////////////////////////////////////////////////////////////////////////////////
    protected Image background;
    protected virtual bool lazyInit() {
        if (null != background) return true;
        background = GetComponent<Image>();
        if (null == background) {
            background = this.gameObject.AddComponent<Image>();
        }
        return true;
    }
    
    protected override void highlightAsSelected() {
        lazyInit();
        background.sprite = selectGroup.cellBgSelectedSpr;
        if (selectGroup.hideCellUponNullBg) {
            background.transform.localScale = Vector3.one;
        }
    }

    protected override void dimAsUnselected() {
        lazyInit();
        if (null != selectGroup.cellBgUnselectedSpr) {
            background.sprite = selectGroup.cellBgUnselectedSpr;
            if (selectGroup.hideCellUponNullBg) {
                background.transform.localScale = Vector3.one;
            }
        } else {
            if (selectGroup.hideCellUponNullBg) {
                background.transform.localScale = Vector3.zero;
                // Don't set "background.sprite = null" in this case, or it might accidentally reset the sprite draw mode
            } else {
                background.sprite = null;
            }
        }
    }
}

