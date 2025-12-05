#ifndef PB_CONSTS_H
#define PB_CONSTS_H 1

#include "serializable_data.pb.h"
#include "joltc_export.h"
#include <set>
#include <utility>

using namespace jtshared;

extern JOLTC_EXPORT const PrimitiveConsts* globalPrimitiveConsts;
extern JOLTC_EXPORT const ConfigConsts* globalConfigConsts;

const std::unordered_set<CharacterState> onWallSet = {
    OnWallIdle1, 
    OnWallAtk1, 
};

const std::unordered_set<CharacterState> proactiveJumpingSet = {
    InAirIdle1ByJump,
    InAirIdle1ByWallJump,
    InAirIdle2ByJump,
};

const std::unordered_set<CharacterState> inAirSet = {
    InAirIdle1NoJump,
    InAirIdle1ByJump,
    InAirIdle1ByWallJump,
    InAirIdle2ByJump,
    InAirAtk1,
    InAirAtk2,
    InAirAtk6,
    InAirAtked1,
    BlownUp1,
    OnWallIdle1,
    OnWallAtk1,
    InAirDashing,
    InAirBackDashing,
    InAirWalking,
    InAirWalkStopping,
    InAirTurnAround,
};

const std::unordered_set<CharacterState> atkedSet = {
    Atked1,
    InAirAtked1,
    CrouchAtked1,
};

const std::unordered_set<CharacterState> noOpSet = {
    BlownUp1,
    LayDown1,
    // [WARNING] During the invinsible frames of GetUp1, the player is allowed to take any action
    Dying,
    Dimmed,
    Awaking,
};

const std::unordered_set<CharacterState> walkingSet = {
    Walking,
    WalkingAtk1,
    WalkingAtk1_Charging,
    BackWalking,
    CrouchWalking,
    InAirWalking,
    // [WARNING] "WalkStopping" doesn't count.
};

const std::unordered_set<CharacterState> invinsibleSet = {
    BlownUp1,
    LayDown1,
    GetUp1,
    Dying,
    TransformingInto
};

const std::unordered_set<CharacterState> nonAttackingSet = {
    Idle1,
    Walking,
    BackWalking, 
    WalkStopping,
    Dashing,
    BackDashing,
    Sliding,
    GroundDodged, 
    TurnAround,
    InAirTurnAround,
    InAirIdle1NoJump,
    InAirIdle1ByJump,
    InAirIdle1ByWallJump,
    InAirIdle2ByJump,
    InAirDashing,
    InAirWalking,
    InAirWalkStopping,
    InAirBackDashing,
    OnWallIdle1,
    CrouchIdle1,
    GetUp1,
    Def1, 
    Def1Atked1,
    Def1Broken,
    Atked1,
    InAirAtked1,
    CrouchAtked1,
    BlownUp1,
    LayDown1,
    Dying,
    Dimmed, 
    TransformingInto,
    Awaking
};

const std::unordered_set<CharacterState> shrinkedSizeSet = {
    BlownUp1,
    LayDown1,
    InAirIdle1NoJump,
    InAirIdle1ByJump,
    InAirIdle2ByJump,
    InAirIdle1ByWallJump,
    InAirDashing,
    InAirAtk1,
    InAirAtk2,
    InAirAtk6,
    InAirAtked1,
    InAirWalking,
    OnWallIdle1,
    Sliding,
    Dashing,
    GroundDodged,
    CrouchIdle1,
    CrouchAtk1,
    CrouchAtked1,
    /*
    // [WARNING] Intentionally NOT added here.
    BackDashing,
    InAirBackDashing,
    */
};


typedef struct ChStatePairHasher {
    std::size_t operator()(const std::pair<CharacterState, CharacterState>& v) const {
        return std::hash<CharacterState>()(v.first) ^ std::hash<CharacterState>()(v.second);
    }
} ChStatePairHasher;

const std::unordered_set<std::pair<CharacterState, CharacterState>, ChStatePairHasher> lowerPartForwardTransitionSet = {
    std::make_pair<CharacterState, CharacterState>(Walking, WalkingAtk1),
    std::make_pair<CharacterState, CharacterState>(Walking, WalkingAtk4),
    std::make_pair<CharacterState, CharacterState>(Walking, WalkingAtk1_Charging),
};

const std::unordered_set<std::pair<CharacterState, CharacterState>, ChStatePairHasher> lowerPartReverseTransitionSet = {
    std::make_pair<CharacterState, CharacterState>(WalkingAtk1, Walking),
    std::make_pair<CharacterState, CharacterState>(WalkingAtk4, Walking),
    std::make_pair<CharacterState, CharacterState>(WalkingAtk1_Charging, Walking),
};

const std::unordered_set<std::pair<CharacterState, CharacterState>, ChStatePairHasher> lowerPartInheritTransitionSet = {
    std::make_pair<CharacterState, CharacterState>(WalkingAtk1_Charging, WalkingAtk1),
    std::make_pair<CharacterState, CharacterState>(WalkingAtk1, WalkingAtk1_Charging),
};

#endif
