using UnityEngine;

namespace Assets.Scripts.MapParser {

    public class StoryTriggerPlaceholder : MonoBehaviour {
        public OptionalInt forceCtrlRdfCount;
        public OptionalUlong forceCtrlCmd;
        public uint activatesCameraBoundId;

        public int levelScore;

        public string bgmId;

        public bool isBossSavepoint;
        public uint[] bossSpeciesSet;
    }

}
