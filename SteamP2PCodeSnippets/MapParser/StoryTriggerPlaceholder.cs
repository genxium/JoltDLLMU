using JoltCSharp;
using jtshared;
using System;
using UnityEngine;

namespace Assets.Scripts.MapParser {

    public class StoryTriggerPlaceholder : MonoBehaviour {
        public OptionalInt forceCtrlRdfCount;
        public OptionalUlong forceCtrlCmd;

        public int levelScore;

        public string bgmId;

        public bool isBossSavepoint;
        public uint[] bossSpeciesSet;
    }

}
