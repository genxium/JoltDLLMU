using System;
using System.Collections;
using UnityEngine;
using UnityEngine.InputSystem;

public abstract partial class AbstractSingleSelectGroup {

    /*
    [REMINDER] Using nested-class to access protected member variables & methods of "AbstractSingleSelectGroup".
    */

    public abstract class AbstractDraggableSingleSelectPanel : AbstractSingleSelectPanel {

        /////////////////////////////////////////////////////////////////////////////////////////
        public float cameraSpeed = 1000;

        public void OnPointerPress(InputAction.CallbackContext context) {
            if (!currentSelectPanelEnabled) return;
            isInCameraAutoTracking = false;
            bool rising = context.ReadValueAsButton();
            if (rising) {
                if (!isDragging) {
                    //Debug.Log("JoltStoryLevelPanel: Pointer press registers");
                    isDragging = true;
                    dragStartingPos = Vector2.zero;
                }
            } else {
                //Debug.Log($"JoltStoryLevelPanel: Pointer press ends by context phase={context.phase}");
                isDragging = false;
                dragStartingPos = Vector2.zero;
                cachedCamDstPos = gameplayCamera.transform.position;
                StartCoroutine(delayToEnableSubGroups());
            }
        }

        public void OnPointerDrag(InputAction.CallbackContext context) {
            if (!currentSelectPanelEnabled) return;
            isInCameraAutoTracking = false;
            if (isDragging && context.performed) {
                var positionInWindowSpace = context.ReadValue<Vector2>();
                var posInMainCamViewport = gameplayCamera.ScreenToViewportPoint(positionInWindowSpace);
                bool isWithinCamera = (0f <= posInMainCamViewport.x && posInMainCamViewport.x <= 1f && 0f <= posInMainCamViewport.y && posInMainCamViewport.y <= 1f);

                camDiffDstHolder.Set(0, 0);
                if (!isWithinCamera) {
                    //Debug.LogFormat("JoltStoryLevelPanel: Pointer press ends by posInMainCamViewport={0}", posInMainCamViewport);
                    isDragging = false;
                    dragStartingPos = Vector2.zero;
                } else {
                    if (Vector2.zero.Equals(dragStartingPos)) {
                        dragStartingPos = positionInWindowSpace;
                        //Debug.Log($"JoltStoryLevelPanel: Pointer press starts at dragStartingPos={dragStartingPos}");
                        disableSubGroups();
                    } else {
                        var deltaInWindowSpace = positionInWindowSpace - dragStartingPos;
                        // Debug.Log($"Pointer Held Down - delta in window space = {deltaInWindowSpace}");
                        camDiffDstHolder = deltaInWindowSpace;
                    }
                }

                var camOldPos = gameplayCamera.transform.position;
                newPosHolder.Set(camOldPos.x - camDiffDstHolder.x, camOldPos.y - camDiffDstHolder.y, camOldPos.z);
                clampToMapBoundary(ref newPosHolder);
                gameplayCamera.transform.position = newPosHolder;
                dragStartingPos = positionInWindowSpace;
            }
        }

        /////////////////////////////////////////////////////////////////////////////////////////
        protected bool draggable = true;
        protected bool zoomable = false;
        protected bool isInCameraAutoTracking = false;
        protected bool isDragging = false;
        protected Vector2 dragStartingPos = Vector2.zero;
        protected Vector3 cachedCamDstPos = Vector3.negativeInfinity;

        protected Camera gameplayCamera;
        protected Camera cutSceneCamera;
        protected Canvas canvas; // Usually but not always, we'd need a popup anchored on Canvas for "AbstractDraggableSingleSelectPanel".

        protected int spaceOffsetX;
        protected int spaceOffsetY;
        protected float cameraCapMinX, cameraCapMaxX, cameraCapMinY, cameraCapMaxY;
        protected float effectivelyInfinitelyFar;

        protected override void OnEnable() {
            ToggleUIInteractability(true);
            base.OnEnable();
        }

        protected override void OnDisable() {
            base.OnDisable();
            if (null != gameplayCamera) {
                cachedCamDstPos = gameplayCamera.transform.position;
            }
        }

        protected virtual void Update() {
            if (!draggable) {
                return;
            }
            if (!currentSelectPanelEnabled) {
                return;
            }
            if (isInCameraAutoTracking) {
                cameraTrack();
            }
        }

        protected Vector3 newPosHolder = new Vector3();
        protected Vector2 camDiffDstHolder = new Vector2();
        protected void calcCameraCaps() {
            int paddingX = (int)(gameplayCamera.orthographicSize * gameplayCamera.aspect);
            int paddingY = (int)(gameplayCamera.orthographicSize);
            cameraCapMinX = 0 + paddingX;
            cameraCapMaxX = (spaceOffsetX << 1) - paddingX;

            cameraCapMinY = -(spaceOffsetY << 1) + paddingY;
            cameraCapMaxY = 0 - paddingY;

            effectivelyInfinitelyFar = 4f * Math.Max(spaceOffsetX, spaceOffsetY);
        }

        protected void clampToMapBoundary(ref Vector3 posHolder) {
            float newX = posHolder.x, newY = posHolder.y, newZ = posHolder.z;
            if (newX > cameraCapMaxX) newX = cameraCapMaxX;
            if (newX < cameraCapMinX) newX = cameraCapMinX;
            if (newY > cameraCapMaxY) newY = cameraCapMaxY;
            if (newY < cameraCapMinY) newY = cameraCapMinY;
            posHolder.Set(newX, newY, newZ);
        }

        protected void cameraTrack() {
            var camOldPos = gameplayCamera.transform.position;
            var activeSelectGroup = GetActiveSubGroup();
            var selectedCell = activeSelectGroup.GetActiveCell();
            var dst = selectedCell.transform.position;
            camDiffDstHolder.Set(dst.x - camOldPos.x, dst.y - camOldPos.y);

            // Immediately teleport
            var stepLength = Time.deltaTime * cameraSpeed;
            if (stepLength > camDiffDstHolder.magnitude) {
                newPosHolder.Set(dst.x, dst.y, camOldPos.z);
                clampToMapBoundary(ref newPosHolder);
                cachedCamDstPos = newPosHolder;
            } else {
                var newMapPosDiff2 = camDiffDstHolder.normalized * stepLength;
                newPosHolder.Set(camOldPos.x + newMapPosDiff2.x, camOldPos.y + newMapPosDiff2.y, camOldPos.z);
                clampToMapBoundary(ref newPosHolder);
            }

            gameplayCamera.transform.position = newPosHolder;
        }

        protected virtual bool cameraTeleport() {
            if (null == gameplayCamera) return false;
            isInCameraAutoTracking = false;
            if (Vector3.negativeInfinity.Equals(cachedCamDstPos)) {
                var camOldPos = gameplayCamera.transform.position;
                var activeSelectGroup = GetActiveSubGroup();
                var selectedCell = activeSelectGroup.GetActiveCell();
                var dst = selectedCell.transform.position;

                // Immediately teleport
                newPosHolder.Set(dst.x, dst.y, camOldPos.z);
                clampToMapBoundary(ref newPosHolder);
                gameplayCamera.transform.position = newPosHolder;
                cachedCamDstPos = newPosHolder;
            } else {
                gameplayCamera.transform.position = cachedCamDstPos;
            }
            
            return true;
        }

        protected override void defaultActionDispatcher(InputAction.CallbackContext context) {
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
                case "PointerDrag":
                    OnPointerDrag(context);
                    break;
                case "PointerPress":
                    OnPointerPress(context);
                    break;
            }
        }

        protected void disableSubGroups() {
            foreach (var subGroup in subGroups) {
                if (null == subGroup) {
                    continue;
                }
                subGroup.toggleUIInteractability(false);
            }
            //Debug.Log($"{this.GetType().Name} subGroups UIInteractability disabled by dragging");
        }

        protected IEnumerator delayToEnableSubGroups() {
            yield return new WaitForEndOfFrame();
            if (currentSelectPanelEnabled) {
                foreach (var subGroup in subGroups) {
                    if (null == subGroup) {
                        continue;
                    }
                    subGroup.toggleUIInteractability(true);
                }
                //Debug.Log($"{this.GetType().Name} subGroups UIInteractability enabled by end of OnPointerPress");
            }
        }
    }
}
