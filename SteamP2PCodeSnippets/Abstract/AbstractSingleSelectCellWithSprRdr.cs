using UnityEngine;

public abstract class AbstractSingleSelectCellWithSprRdr : AbstractSingleSelectGroup.AbstractSingleSelectCell {
    // Start is called before the first frame update
    public SpriteRenderer background;
    public Sprite selectedSpr, unselectedSpr;
    public bool hideUponNullBkg = true;

    protected override void highlightAsSelected() {
        background.sprite = selectedSpr;
        if (hideUponNullBkg) {
            background.transform.localScale = Vector3.one;
        }
    }

    protected override void dimAsUnselected() {
        if (null != unselectedSpr) {
            background.sprite = unselectedSpr;
            if (hideUponNullBkg) {
                background.transform.localScale = Vector3.one;
            }
        } else {
            if (hideUponNullBkg) {
                background.transform.localScale = Vector3.zero;
                // Don't set "background.sprite = null" in this case, or it might accidentally reset the sprite draw mode
            } else {
                background.sprite = null;
            }
        }
    }
}
