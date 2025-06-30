#ifndef PB_CONSTS_H
#define PB_CONSTS_H

#pragma once

#include "serializable_data.pb.h"
#include "joltc_export.h"
#include <set>

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
    InAirWalking,
    InAirWalkStopping,
    Dashing // Yes dashing is an InAir state even if you dashed on the ground :)
};

const std::unordered_set<CharacterState> noOpSet = {
    Atked1,
    InAirAtked1,
    CrouchAtked1,
    BlownUp1,
    LayDown1,
    // [WARNING] During the invinsible frames of GET_UP1, the player is allowed to take any action
    Dying,
    Dimmed,
    Awaking,
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
    InAirIdle1NoJump,
    InAirIdle1ByJump,
    InAirIdle1ByWallJump,
    InAirIdle2ByJump,
    InAirWalking,
    InAirWalkStopping,
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
};

#endif
