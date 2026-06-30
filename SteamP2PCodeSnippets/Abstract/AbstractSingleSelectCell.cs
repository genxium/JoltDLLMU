using UnityEngine;
using UnityEngine.EventSystems;

public abstract partial class AbstractSingleSelectGroup {
    /*
    [REMINDER] Using nested-class to access protected member variables & methods of "AbstractSingleSelectGroup".
    */
    public abstract class AbstractSingleSelectCell : MonoBehaviour, IPointerClickHandler {
        /////////////////////////////////////////////////////////////////////////////////////////
        
        public int GetSelectedIdx() {
            return selectedIdx;
        }

        public virtual void SetSelectedIdx(AbstractSingleSelectGroup theSg, int val) {
            selectGroup = theSg;
            selectedIdx = val;
        }

        public virtual void OnPointerClick(PointerEventData evt) {
            if (!selectGroup.currentSelectGroupEnabled) {
                // [REMINDER] When "AbstractDraggableSingleSelectPanel.isDragging == true", this method shouldn't be able to select a cell.
                //Debug.Log($"{GetType().Name} cell OnPointerClick stopped due to false == {selectGroup.GetType().Name}.currentSelectGroupEnabled");
                return;
            }
            /*
             [WARNING] 

             If this is not called, then go make sure that there's a "Physics2DRaycaster" attached to "GameplayCamera" with a larger than 0 interaction quota!

             Why using physics raycast to detect click on a Sprite? Reference https://stackoverflow.com/questions/41391708/how-to-detect-click-touch-events-on-ui-and-gameobjects
             */
            //Debug.Log($"{GetType().Name} cell OnPointerClick processed due to true == {selectGroup.GetType().Name}.currentSelectGroupEnabled");
            selectGroup.onCellSelected(selectedIdx, true);
        }
        
        public void SetSelected(bool val) {
            if (val) {
                highlightAsSelected();
            } else {
                dimAsUnselected();
            }
        }
        
        /////////////////////////////////////////////////////////////////////////////////////////
        protected abstract void highlightAsSelected();
        protected abstract void dimAsUnselected();

        // Start is called before the first frame update
        protected int selectedIdx;
        protected AbstractSingleSelectGroup selectGroup;
    }
}
