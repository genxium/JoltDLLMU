using UnityEngine;
using UnityEngine.Rendering.Universal;

namespace Assets.Scripts.AnimControllers {
    internal class SpriteSequenceLight2D : MonoBehaviour {
        public Animator animator;
        public Light2D light2D;
        private SpriteRenderer spriteRenderer;

        private void Start() {
        }

        private void LateUpdate() {
            if (light2D == null) {
                return;
            }

            if (null == spriteRenderer) {
                if (null != animator) {
                    spriteRenderer = animator.GetComponent<SpriteRenderer>();
                }
            }

            if (light2D.lightCookieSprite != spriteRenderer.sprite) {
                light2D.lightCookieSprite = spriteRenderer.sprite;
            }
        }
    }
}
