using Assets.Scripts.MapParser;
using jtshared;
using UnityEngine;

public class NpcPlaceholder : MonoBehaviour
{
    public CharacterOptions characterOptions;
    public NpcGoal initGoal = NpcGoal.Nidle;
    public uint publishingToTriggerIdUponExhausted;
    public uint subscribesToTriggerId;
    public uint exhaustedToDropConsumableSpeciesId;
    public uint exhaustedToDropBuffSpeciesId;
    public uint exhaustedToDropPickupSkillId;
    public bool isMainTowerOfTeam;

    // Start is called once before the first execution of Update after the MonoBehaviour is created
    void Start() {
        
    }

    // Update is called once per frame
    void Update() {
        
    }
}
