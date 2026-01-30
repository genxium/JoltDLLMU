#include "joltc_export.h" // imports the "JOLTC_EXPORT" macro for "serializable_data.pb.h"
#include <Jolt/Jolt.h> // imports the "JPH_EXPORT" macro for classes under namespace JPH
#include "serializable_data.pb.h"
#include "joltc_api.h" 
#include "PbConsts.h"
#include "CppOnlyConsts.h"
#include "DebugLog.h"

#include "FrontendBattle.h"
#include <chrono>
#include <fstream>
#include <filesystem>
#include <google/protobuf/arena.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/util/message_differencer.h>   
google::protobuf::util::MessageDifferencer differ;
google::protobuf::Arena pbTestCaseDataAllocator;

using namespace jtshared;
using namespace std::filesystem;

const int pbBufferSizeLimit = (1 << 14);
char pbByteBuffer[pbBufferSizeLimit];
char rdfFetchBuffer[pbBufferSizeLimit];
char upsyncSnapshotBuffer[pbBufferSizeLimit];
const char* const selfPlayerId = "foobar";
int selfCmdAuthKey = 123456;

RenderFrame* mockStartRdf() {
    auto chSpecies = globalPrimitiveConsts->ch_species();
    const int roomCapacity = 2;
    auto startRdf = BaseBattle::NewPreallocatedRdf(roomCapacity, 8, 128);
    startRdf->set_id(globalPrimitiveConsts->starting_render_frame_id());
    uint32_t pickableIdCounter = 1;
    uint32_t npcIdCounter = 1;
    uint32_t bulletIdCounter = 1;

    auto characterConfigs = globalConfigConsts->character_configs();

    auto player1 = startRdf->mutable_players(0);
    auto playerCh1 = player1->mutable_chd();
    auto playerCh1Species = chSpecies.bountyhunter();
    auto cc1 = characterConfigs[playerCh1Species];
    playerCh1->set_x(-85);
    playerCh1->set_y(200);
    playerCh1->set_speed(cc1.speed());
    playerCh1->set_ch_state(CharacterState::InAirIdle1NoJump);
    playerCh1->set_frames_to_recover(0);
    playerCh1->set_q_x(0);
    playerCh1->set_q_y(0);
    playerCh1->set_q_z(0);
    playerCh1->set_q_w(1);
    playerCh1->set_aiming_q_x(0);
    playerCh1->set_aiming_q_y(0);
    playerCh1->set_aiming_q_z(0);
    playerCh1->set_aiming_q_w(1);
    playerCh1->set_vel_x(0);
    playerCh1->set_vel_y(0);
    playerCh1->set_hp(cc1.hp());
    playerCh1->set_species_id(playerCh1Species);
    playerCh1->set_bullet_team_id(1);
    player1->set_join_index(1);
    player1->set_revival_x(playerCh1->x());
    player1->set_revival_y(playerCh1->y());
    player1->set_revival_q_x(0);
    player1->set_revival_q_y(0);
    player1->set_revival_q_z(0);
    player1->set_revival_q_w(1);

    auto player2 = startRdf->mutable_players(1);
    auto playerCh2 = player2->mutable_chd();
    auto playerCh2Species = chSpecies.bladegirl();
    auto cc2 = characterConfigs[playerCh2Species];
    playerCh2->set_x(+90);
    playerCh2->set_y(300);
    playerCh2->set_speed(cc2.speed());
    playerCh2->set_ch_state(CharacterState::InAirIdle1NoJump);
    playerCh2->set_frames_to_recover(0);
    playerCh2->set_q_x(cTurnbackAroundYAxis.GetX());
    playerCh2->set_q_y(cTurnbackAroundYAxis.GetY());
    playerCh2->set_q_z(cTurnbackAroundYAxis.GetZ());
    playerCh2->set_q_w(cTurnbackAroundYAxis.GetW());
    playerCh2->set_aiming_q_x(0);
    playerCh2->set_aiming_q_y(0);
    playerCh2->set_aiming_q_z(0);
    playerCh2->set_aiming_q_w(1);
    playerCh2->set_vel_x(0);
    playerCh2->set_vel_y(0);
    playerCh2->set_hp(cc2.hp());
    playerCh2->set_species_id(playerCh2Species);
    playerCh2->set_bullet_team_id(2);
    player2->set_join_index(2);
    player2->set_revival_x(playerCh2->x());
    player2->set_revival_y(playerCh2->y());
    player2->set_revival_q_x(cTurnbackAroundYAxis.GetX());
    player2->set_revival_q_y(cTurnbackAroundYAxis.GetY());
    player2->set_revival_q_z(cTurnbackAroundYAxis.GetZ());
    player2->set_revival_q_w(cTurnbackAroundYAxis.GetW());

    startRdf->set_npc_id_counter(npcIdCounter);
    startRdf->set_npc_count(npcIdCounter-1);

    startRdf->set_bullet_id_counter(bulletIdCounter);
    startRdf->set_pickable_id_counter(pickableIdCounter);

    return startRdf;
}

RenderFrame* mockBlacksaber1VisionTestStartRdf() {
    auto chSpecies = globalPrimitiveConsts->ch_species();
    const int roomCapacity = 1;
    auto startRdf = BaseBattle::NewPreallocatedRdf(roomCapacity, 8, 128);
    startRdf->set_id(globalPrimitiveConsts->starting_render_frame_id());
    uint32_t pickableIdCounter = 1;
    uint32_t npcIdCounter = 1;
    uint32_t bulletIdCounter = 1;

    auto characterConfigs = globalConfigConsts->character_configs();

    auto player1 = startRdf->mutable_players(0);
    auto playerCh1 = player1->mutable_chd();
    auto playerCh1Species = chSpecies.bountyhunter();
    auto cc1 = characterConfigs[playerCh1Species];
    playerCh1->set_x(-85);
    playerCh1->set_y(100);
    playerCh1->set_speed(cc1.speed());
    playerCh1->set_ch_state(CharacterState::InAirIdle1NoJump);
    playerCh1->set_frames_to_recover(0);
    playerCh1->set_q_x(0);
    playerCh1->set_q_y(0);
    playerCh1->set_q_z(0);
    playerCh1->set_q_w(1);
    playerCh1->set_aiming_q_x(0);
    playerCh1->set_aiming_q_y(0);
    playerCh1->set_aiming_q_z(0);
    playerCh1->set_aiming_q_w(1);
    playerCh1->set_vel_x(0);
    playerCh1->set_vel_y(0);
    playerCh1->set_hp(cc1.hp());
    playerCh1->set_species_id(playerCh1Species);
    playerCh1->set_bullet_team_id(1);
    player1->set_join_index(1);
    player1->set_revival_x(playerCh1->x());
    player1->set_revival_y(playerCh1->y());
    player1->set_revival_q_x(0);
    player1->set_revival_q_y(0);
    player1->set_revival_q_z(0);
    player1->set_revival_q_w(1);

    auto npc1 = startRdf->mutable_npcs(0);
    npc1->set_id(npcIdCounter++);
    npc1->set_goal_as_npc(NpcGoal::NIdle);
    auto npcCh1 = npc1->mutable_chd();
    auto npcCh1Species = chSpecies.blacksaber_test_with_vision();
    auto npcCc1 = characterConfigs[npcCh1Species];
    npcCh1->set_x(+80);
    npcCh1->set_y(100);
    npcCh1->set_speed(npcCc1.speed());
    npcCh1->set_ch_state(CharacterState::InAirIdle1NoJump);
    npcCh1->set_frames_to_recover(0);
    npcCh1->set_q_x(cTurnbackAroundYAxis.GetX());
    npcCh1->set_q_y(cTurnbackAroundYAxis.GetY());
    npcCh1->set_q_z(cTurnbackAroundYAxis.GetZ());
    npcCh1->set_q_w(cTurnbackAroundYAxis.GetW());
    npcCh1->set_aiming_q_x(0);
    npcCh1->set_aiming_q_y(0);
    npcCh1->set_aiming_q_z(0);
    npcCh1->set_aiming_q_w(1);
    npcCh1->set_vel_x(0);
    npcCh1->set_vel_y(0);
    npcCh1->set_hp(npcCc1.hp());
    npcCh1->set_species_id(npcCh1Species);
    npcCh1->set_bullet_team_id(3);

    auto npc2 = startRdf->mutable_npcs(1);
    npc2->set_id(npcIdCounter++);
    npc2->set_goal_as_npc(NpcGoal::NPatrol);
    auto npcCh2 = npc2->mutable_chd();
    auto npcCh2Species = chSpecies.blacksaber_test_with_vision();
    auto npcCc2 = characterConfigs[npcCh2Species];
    npcCh2->set_x(+350);
    npcCh2->set_y(580);
    npcCh2->set_speed(npcCc2.speed());
    npcCh2->set_ch_state(CharacterState::InAirIdle1NoJump);
    npcCh2->set_frames_to_recover(0);
    npcCh2->set_q_x(0);
    npcCh2->set_q_y(0);
    npcCh2->set_q_z(0);
    npcCh2->set_q_w(1);
    npcCh2->set_aiming_q_x(0);
    npcCh2->set_aiming_q_y(0);
    npcCh2->set_aiming_q_z(0);
    npcCh2->set_aiming_q_w(1);
    npcCh2->set_vel_x(0);
    npcCh2->set_vel_y(0);
    npcCh2->set_hp(10); // For easier death
    npcCh2->set_species_id(npcCh2Species);
    npcCh2->set_bullet_team_id(3);
    
    startRdf->set_npc_id_counter(npcIdCounter);
    startRdf->set_npc_count(npcIdCounter-1);

    startRdf->set_bullet_id_counter(bulletIdCounter);
    startRdf->set_pickable_id_counter(pickableIdCounter);

    return startRdf;
}

RenderFrame* mockSliderTrapTestStartRdf1() {
    auto chSpecies = globalPrimitiveConsts->ch_species();
    const int roomCapacity = 2;
    auto startRdf = BaseBattle::NewPreallocatedRdf(roomCapacity, 8, 128);
    startRdf->set_id(globalPrimitiveConsts->starting_render_frame_id());
    uint32_t pickableIdCounter = 1;
    uint32_t npcIdCounter = 1;
    uint32_t bulletIdCounter = 1;
    uint32_t dynamicTrapCount = 0;

    auto characterConfigs = globalConfigConsts->character_configs();

    auto player1 = startRdf->mutable_players(0);
    auto playerCh1 = player1->mutable_chd();
    auto playerCh1Species = chSpecies.bountyhunter();
    auto cc1 = characterConfigs[playerCh1Species];
    playerCh1->set_x(-85);
    playerCh1->set_y(200);
    playerCh1->set_speed(cc1.speed());
    playerCh1->set_ch_state(CharacterState::InAirIdle1NoJump);
    playerCh1->set_frames_to_recover(0);
    playerCh1->set_q_x(0);
    playerCh1->set_q_y(0);
    playerCh1->set_q_z(0);
    playerCh1->set_q_w(1);
    playerCh1->set_aiming_q_x(0);
    playerCh1->set_aiming_q_y(0);
    playerCh1->set_aiming_q_z(0);
    playerCh1->set_aiming_q_w(1);
    playerCh1->set_vel_x(0);
    playerCh1->set_vel_y(0);
    playerCh1->set_hp(cc1.hp());
    playerCh1->set_species_id(playerCh1Species);
    playerCh1->set_bullet_team_id(1);
    player1->set_join_index(1);
    player1->set_revival_x(playerCh1->x());
    player1->set_revival_y(playerCh1->y());
    player1->set_revival_q_x(0);
    player1->set_revival_q_y(0);
    player1->set_revival_q_z(0);
    player1->set_revival_q_w(1);

    auto player2 = startRdf->mutable_players(1);
    auto playerCh2 = player2->mutable_chd();
    auto playerCh2Species = chSpecies.bladegirl();
    auto cc2 = characterConfigs[playerCh2Species];
    playerCh2->set_x(+90);
    playerCh2->set_y(300);
    playerCh2->set_speed(cc2.speed());
    playerCh2->set_ch_state(CharacterState::InAirIdle1NoJump);
    playerCh2->set_frames_to_recover(0);
    playerCh2->set_q_x(cTurnbackAroundYAxis.GetX());
    playerCh2->set_q_y(cTurnbackAroundYAxis.GetY());
    playerCh2->set_q_z(cTurnbackAroundYAxis.GetZ());
    playerCh2->set_q_w(cTurnbackAroundYAxis.GetW());
    playerCh2->set_aiming_q_x(0);
    playerCh2->set_aiming_q_y(0);
    playerCh2->set_aiming_q_z(0);
    playerCh2->set_aiming_q_w(1);
    playerCh2->set_vel_x(0);
    playerCh2->set_vel_y(0);
    playerCh2->set_hp(cc2.hp());
    playerCh2->set_species_id(playerCh2Species);
    playerCh2->set_bullet_team_id(2);
    player2->set_join_index(2);
    player2->set_revival_x(playerCh2->x());
    player2->set_revival_y(playerCh2->y());
    player2->set_revival_q_x(cTurnbackAroundYAxis.GetX());
    player2->set_revival_q_y(cTurnbackAroundYAxis.GetY());
    player2->set_revival_q_z(cTurnbackAroundYAxis.GetZ());
    player2->set_revival_q_w(cTurnbackAroundYAxis.GetW());

    auto dynamicTrap1 = startRdf->add_dynamic_traps();
    dynamicTrap1->set_id(42);
    dynamicTrap1->set_tpt(globalPrimitiveConsts->tpt_sliding_platform());
    ++dynamicTrapCount;
    
    startRdf->set_npc_id_counter(npcIdCounter);
    startRdf->set_npc_count(npcIdCounter-1);

    startRdf->set_bullet_id_counter(bulletIdCounter);
    startRdf->set_pickable_id_counter(pickableIdCounter);

    startRdf->set_dynamic_trap_count(dynamicTrapCount);

    return startRdf;
}

RenderFrame* mockSliderTrapTestStartRdf2() {
    auto chSpecies = globalPrimitiveConsts->ch_species();
    const int roomCapacity = 2;
    auto startRdf = BaseBattle::NewPreallocatedRdf(roomCapacity, 8, 128);
    startRdf->set_id(globalPrimitiveConsts->starting_render_frame_id());
    uint32_t pickableIdCounter = 1;
    uint32_t npcIdCounter = 1;
    uint32_t bulletIdCounter = 1;
    uint32_t dynamicTrapCount = 0;

    auto characterConfigs = globalConfigConsts->character_configs();

    auto player1 = startRdf->mutable_players(0);
    auto playerCh1 = player1->mutable_chd();
    auto playerCh1Species = chSpecies.bountyhunter();
    auto cc1 = characterConfigs[playerCh1Species];
    playerCh1->set_x(-85);
    playerCh1->set_y(200);
    playerCh1->set_speed(cc1.speed());
    playerCh1->set_ch_state(CharacterState::InAirIdle1NoJump);
    playerCh1->set_frames_to_recover(0);
    playerCh1->set_q_x(0);
    playerCh1->set_q_y(0);
    playerCh1->set_q_z(0);
    playerCh1->set_q_w(1);
    playerCh1->set_aiming_q_x(0);
    playerCh1->set_aiming_q_y(0);
    playerCh1->set_aiming_q_z(0);
    playerCh1->set_aiming_q_w(1);
    playerCh1->set_vel_x(0);
    playerCh1->set_vel_y(0);
    playerCh1->set_hp(cc1.hp());
    playerCh1->set_species_id(playerCh1Species);
    playerCh1->set_bullet_team_id(1);
    player1->set_join_index(1);
    player1->set_revival_x(playerCh1->x());
    player1->set_revival_y(playerCh1->y());
    player1->set_revival_q_x(0);
    player1->set_revival_q_y(0);
    player1->set_revival_q_z(0);
    player1->set_revival_q_w(1);

    auto player2 = startRdf->mutable_players(1);
    auto playerCh2 = player2->mutable_chd();
    auto playerCh2Species = chSpecies.bladegirl();
    auto cc2 = characterConfigs[playerCh2Species];
    playerCh2->set_x(+90);
    playerCh2->set_y(300);
    playerCh2->set_speed(cc2.speed());
    playerCh2->set_ch_state(CharacterState::InAirIdle1NoJump);
    playerCh2->set_frames_to_recover(0);
    playerCh2->set_q_x(cTurnbackAroundYAxis.GetX());
    playerCh2->set_q_y(cTurnbackAroundYAxis.GetY());
    playerCh2->set_q_z(cTurnbackAroundYAxis.GetZ());
    playerCh2->set_q_w(cTurnbackAroundYAxis.GetW());
    playerCh2->set_aiming_q_x(0);
    playerCh2->set_aiming_q_y(0);
    playerCh2->set_aiming_q_z(0);
    playerCh2->set_aiming_q_w(1);
    playerCh2->set_vel_x(0);
    playerCh2->set_vel_y(0);
    playerCh2->set_hp(cc2.hp());
    playerCh2->set_species_id(playerCh2Species);
    playerCh2->set_bullet_team_id(2);
    player2->set_join_index(2);
    player2->set_revival_x(playerCh2->x());
    player2->set_revival_y(playerCh2->y());
    player2->set_revival_q_x(cTurnbackAroundYAxis.GetX());
    player2->set_revival_q_y(cTurnbackAroundYAxis.GetY());
    player2->set_revival_q_z(cTurnbackAroundYAxis.GetZ());
    player2->set_revival_q_w(cTurnbackAroundYAxis.GetW());

    auto dynamicTrap1 = startRdf->add_dynamic_traps();
    dynamicTrap1->set_id(42);
    dynamicTrap1->set_tpt(globalPrimitiveConsts->tpt_sliding_platform());
    ++dynamicTrapCount;

    startRdf->set_npc_id_counter(npcIdCounter);
    startRdf->set_npc_count(npcIdCounter - 1);

    startRdf->set_bullet_id_counter(bulletIdCounter);
    startRdf->set_pickable_id_counter(pickableIdCounter);

    startRdf->set_dynamic_trap_count(dynamicTrapCount);

    return startRdf;
}

RenderFrame* mockSliderTrapTestStartRdf3() {
    auto chSpecies = globalPrimitiveConsts->ch_species();
    const int roomCapacity = 2;
    auto startRdf = BaseBattle::NewPreallocatedRdf(roomCapacity, 8, 128);
    startRdf->set_id(globalPrimitiveConsts->starting_render_frame_id());
    uint32_t pickableIdCounter = 1;
    uint32_t npcIdCounter = 1;
    uint32_t bulletIdCounter = 1;
    uint32_t dynamicTrapCount = 0;

    auto characterConfigs = globalConfigConsts->character_configs();

    auto player1 = startRdf->mutable_players(0);
    auto playerCh1 = player1->mutable_chd();
    auto playerCh1Species = chSpecies.bountyhunter();
    auto cc1 = characterConfigs[playerCh1Species];
    playerCh1->set_x(-400);
    playerCh1->set_y(200);
    playerCh1->set_speed(cc1.speed());
    playerCh1->set_ch_state(CharacterState::InAirIdle1NoJump);
    playerCh1->set_frames_to_recover(0);
    playerCh1->set_q_x(0);
    playerCh1->set_q_y(0);
    playerCh1->set_q_z(0);
    playerCh1->set_q_w(1);
    playerCh1->set_aiming_q_x(0);
    playerCh1->set_aiming_q_y(0);
    playerCh1->set_aiming_q_z(0);
    playerCh1->set_aiming_q_w(1);
    playerCh1->set_vel_x(0);
    playerCh1->set_vel_y(0);
    playerCh1->set_hp(cc1.hp());
    playerCh1->set_species_id(playerCh1Species);
    playerCh1->set_bullet_team_id(1);
    player1->set_join_index(1);
    player1->set_revival_x(playerCh1->x());
    player1->set_revival_y(playerCh1->y());
    player1->set_revival_q_x(0);
    player1->set_revival_q_y(0);
    player1->set_revival_q_z(0);
    player1->set_revival_q_w(1);

    auto player2 = startRdf->mutable_players(1);
    auto playerCh2 = player2->mutable_chd();
    auto playerCh2Species = chSpecies.bladegirl();
    auto cc2 = characterConfigs[playerCh2Species];
    playerCh2->set_x(+100);
    playerCh2->set_y(200);
    playerCh2->set_speed(cc2.speed());
    playerCh2->set_ch_state(CharacterState::InAirIdle1NoJump);
    playerCh2->set_frames_to_recover(0);
    playerCh2->set_q_x(cTurnbackAroundYAxis.GetX());
    playerCh2->set_q_y(cTurnbackAroundYAxis.GetY());
    playerCh2->set_q_z(cTurnbackAroundYAxis.GetZ());
    playerCh2->set_q_w(cTurnbackAroundYAxis.GetW());
    playerCh2->set_aiming_q_x(0);
    playerCh2->set_aiming_q_y(0);
    playerCh2->set_aiming_q_z(0);
    playerCh2->set_aiming_q_w(1);
    playerCh2->set_vel_x(0);
    playerCh2->set_vel_y(0);
    playerCh2->set_hp(cc2.hp());
    playerCh2->set_species_id(playerCh2Species);
    playerCh2->set_bullet_team_id(2);
    player2->set_join_index(2);
    player2->set_revival_x(playerCh2->x());
    player2->set_revival_y(playerCh2->y());
    player2->set_revival_q_x(cTurnbackAroundYAxis.GetX());
    player2->set_revival_q_y(cTurnbackAroundYAxis.GetY());
    player2->set_revival_q_z(cTurnbackAroundYAxis.GetZ());
    player2->set_revival_q_w(cTurnbackAroundYAxis.GetW());

    auto dynamicTrap1 = startRdf->add_dynamic_traps();
    dynamicTrap1->set_id(42);
    dynamicTrap1->set_tpt(globalPrimitiveConsts->tpt_sliding_platform());
    ++dynamicTrapCount;
    
    startRdf->set_npc_id_counter(npcIdCounter);
    startRdf->set_npc_count(npcIdCounter-1);

    startRdf->set_bullet_id_counter(bulletIdCounter);
    startRdf->set_pickable_id_counter(pickableIdCounter);

    startRdf->set_dynamic_trap_count(dynamicTrapCount);

    return startRdf;
}

RenderFrame* mockRollbackChasingAlignTestStartRdf() {
    auto chSpecies = globalPrimitiveConsts->ch_species();
    const int roomCapacity = 2;
    auto startRdf = BaseBattle::NewPreallocatedRdf(roomCapacity, 8, 128);
    startRdf->set_id(globalPrimitiveConsts->starting_render_frame_id());
    uint32_t pickableIdCounter = 1;
    uint32_t npcIdCounter = 1;
    uint32_t bulletIdCounter = 1;

    auto characterConfigs = globalConfigConsts->character_configs();

    auto player1 = startRdf->mutable_players(0);
    auto playerCh1 = player1->mutable_chd();
    auto playerCh1Species = chSpecies.bountyhunter();
    auto cc1 = characterConfigs[playerCh1Species];
    playerCh1->set_x(-85);
    playerCh1->set_y(200);
    playerCh1->set_speed(cc1.speed());
    playerCh1->set_ch_state(CharacterState::InAirIdle1NoJump);
    playerCh1->set_frames_to_recover(0);
    playerCh1->set_q_x(0);
    playerCh1->set_q_y(0);
    playerCh1->set_q_z(0);
    playerCh1->set_q_w(1);
    playerCh1->set_aiming_q_x(0);
    playerCh1->set_aiming_q_y(0);
    playerCh1->set_aiming_q_z(0);
    playerCh1->set_aiming_q_w(1);
    playerCh1->set_vel_x(0);
    playerCh1->set_vel_y(0);
    playerCh1->set_hp(cc1.hp());
    playerCh1->set_species_id(playerCh1Species);
    playerCh1->set_bullet_team_id(1);
    player1->set_join_index(1);
    player1->set_revival_x(playerCh1->x());
    player1->set_revival_y(playerCh1->y());
    player1->set_revival_q_x(0);
    player1->set_revival_q_y(0);
    player1->set_revival_q_z(0);
    player1->set_revival_q_w(1);

    auto player2 = startRdf->mutable_players(1);
    auto playerCh2 = player2->mutable_chd();
    auto playerCh2Species = chSpecies.bladegirl();
    auto cc2 = characterConfigs[playerCh2Species];
    playerCh2->set_x(+90);
    playerCh2->set_y(300);
    playerCh2->set_speed(cc2.speed());
    playerCh2->set_ch_state(CharacterState::InAirIdle1NoJump);
    playerCh2->set_frames_to_recover(0);
    playerCh2->set_q_x(cTurnbackAroundYAxis.GetX());
    playerCh2->set_q_y(cTurnbackAroundYAxis.GetY());
    playerCh2->set_q_z(cTurnbackAroundYAxis.GetZ());
    playerCh2->set_q_w(cTurnbackAroundYAxis.GetW());
    playerCh2->set_aiming_q_x(0);
    playerCh2->set_aiming_q_y(0);
    playerCh2->set_aiming_q_z(0);
    playerCh2->set_aiming_q_w(1);
    playerCh2->set_vel_x(0);
    playerCh2->set_vel_y(0);
    playerCh2->set_hp(cc2.hp());
    playerCh2->set_species_id(playerCh2Species);
    playerCh2->set_bullet_team_id(2);
    player2->set_join_index(2);
    player2->set_revival_x(playerCh2->x());
    player2->set_revival_y(playerCh2->y());
    player2->set_revival_q_x(cTurnbackAroundYAxis.GetX());
    player2->set_revival_q_y(cTurnbackAroundYAxis.GetY());
    player2->set_revival_q_z(cTurnbackAroundYAxis.GetZ());
    player2->set_revival_q_w(cTurnbackAroundYAxis.GetW());

    auto npc1 = startRdf->mutable_npcs(0);
    npc1->set_id(npcIdCounter++);
    auto npcCh1 = npc1->mutable_chd();
    auto npcCh1Species = chSpecies.bladegirl();
    auto npcCc1 = characterConfigs[npcCh1Species];
    npcCh1->set_x(+70);
    npcCh1->set_y(200);
    npcCh1->set_speed(npcCc1.speed());
    npcCh1->set_ch_state(CharacterState::InAirIdle1NoJump);
    npcCh1->set_frames_to_recover(0);
    npcCh1->set_q_x(cTurnbackAroundYAxis.GetX());
    npcCh1->set_q_y(cTurnbackAroundYAxis.GetY());
    npcCh1->set_q_z(cTurnbackAroundYAxis.GetZ());
    npcCh1->set_q_w(cTurnbackAroundYAxis.GetW());
    npcCh1->set_aiming_q_x(0);
    npcCh1->set_aiming_q_y(0);
    npcCh1->set_aiming_q_z(0);
    npcCh1->set_aiming_q_w(1);
    npcCh1->set_vel_x(0);
    npcCh1->set_vel_y(0);
    npcCh1->set_hp(npcCc1.hp());
    npcCh1->set_species_id(npcCh1Species);
    npcCh1->set_bullet_team_id(3);
    
    startRdf->set_npc_id_counter(npcIdCounter);
    startRdf->set_npc_count(npcIdCounter-1);

    startRdf->set_bullet_id_counter(bulletIdCounter);
    startRdf->set_pickable_id_counter(pickableIdCounter);

    return startRdf;
}

RenderFrame* mockRollbackChasingAlignTestStartRdf2() {
    auto chSpecies = globalPrimitiveConsts->ch_species();
    const int roomCapacity = 2;
    auto startRdf = BaseBattle::NewPreallocatedRdf(roomCapacity, 8, 128);
    startRdf->set_id(globalPrimitiveConsts->starting_render_frame_id());
    uint32_t pickableIdCounter = 1;
    uint32_t npcIdCounter = 1;
    uint32_t bulletIdCounter = 1;

    auto characterConfigs = globalConfigConsts->character_configs();

    auto player1 = startRdf->mutable_players(0);
    auto playerCh1 = player1->mutable_chd();
    auto playerCh1Species = chSpecies.bountyhunter();
    auto cc1 = characterConfigs[playerCh1Species];
    playerCh1->set_x(-85);
    playerCh1->set_y(200);
    playerCh1->set_speed(cc1.speed());
    playerCh1->set_ch_state(CharacterState::InAirIdle1NoJump);
    playerCh1->set_frames_to_recover(0);
    playerCh1->set_q_x(0);
    playerCh1->set_q_y(0);
    playerCh1->set_q_z(0);
    playerCh1->set_q_w(1);
    playerCh1->set_aiming_q_x(0);
    playerCh1->set_aiming_q_y(0);
    playerCh1->set_aiming_q_z(0);
    playerCh1->set_aiming_q_w(1);
    playerCh1->set_vel_x(0);
    playerCh1->set_vel_y(0);
    playerCh1->set_hp(cc1.hp());
    playerCh1->set_species_id(playerCh1Species);
    playerCh1->set_bullet_team_id(1);
    player1->set_join_index(1);
    player1->set_revival_x(playerCh1->x());
    player1->set_revival_y(playerCh1->y());
    player1->set_revival_q_x(0);
    player1->set_revival_q_y(0);
    player1->set_revival_q_z(0);
    player1->set_revival_q_w(1);

    auto player2 = startRdf->mutable_players(1);
    auto playerCh2 = player2->mutable_chd();
    auto playerCh2Species = chSpecies.bladegirl();
    auto cc2 = characterConfigs[playerCh2Species];
    playerCh2->set_x(+90);
    playerCh2->set_y(300);
    playerCh2->set_speed(cc2.speed());
    playerCh2->set_ch_state(CharacterState::InAirIdle1NoJump);
    playerCh2->set_frames_to_recover(0);
    playerCh2->set_q_x(cTurnbackAroundYAxis.GetX());
    playerCh2->set_q_y(cTurnbackAroundYAxis.GetY());
    playerCh2->set_q_z(cTurnbackAroundYAxis.GetZ());
    playerCh2->set_q_w(cTurnbackAroundYAxis.GetW());
    playerCh2->set_aiming_q_x(0);
    playerCh2->set_aiming_q_y(0);
    playerCh2->set_aiming_q_z(0);
    playerCh2->set_aiming_q_w(1);
    playerCh2->set_vel_x(0);
    playerCh2->set_vel_y(0);
    playerCh2->set_hp(cc2.hp());
    playerCh2->set_species_id(playerCh2Species);
    playerCh2->set_bullet_team_id(2);
    player2->set_join_index(2);
    player2->set_revival_x(playerCh2->x());
    player2->set_revival_y(playerCh2->y());
    player2->set_revival_q_x(cTurnbackAroundYAxis.GetX());
    player2->set_revival_q_y(cTurnbackAroundYAxis.GetY());
    player2->set_revival_q_z(cTurnbackAroundYAxis.GetZ());
    player2->set_revival_q_w(cTurnbackAroundYAxis.GetW());

    auto npc1 = startRdf->mutable_npcs(0);
    npc1->set_id(npcIdCounter++);
    auto npcCh1 = npc1->mutable_chd();
    auto npcCh1Species = chSpecies.bladegirl();
    auto npcCc1 = characterConfigs[npcCh1Species];
    npcCh1->set_x(+0);
    npcCh1->set_y(200);
    npcCh1->set_speed(npcCc1.speed());
    npcCh1->set_ch_state(CharacterState::InAirIdle1NoJump);
    npcCh1->set_frames_to_recover(0);
    npcCh1->set_q_x(cTurnbackAroundYAxis.GetX());
    npcCh1->set_q_y(cTurnbackAroundYAxis.GetY());
    npcCh1->set_q_z(cTurnbackAroundYAxis.GetZ());
    npcCh1->set_q_w(cTurnbackAroundYAxis.GetW());
    npcCh1->set_aiming_q_x(0);
    npcCh1->set_aiming_q_y(0);
    npcCh1->set_aiming_q_z(0);
    npcCh1->set_aiming_q_w(1);
    npcCh1->set_vel_x(0);
    npcCh1->set_vel_y(0);
    npcCh1->set_hp(npcCc1.hp());
    npcCh1->set_species_id(npcCh1Species);
    npcCh1->set_bullet_team_id(3);

    startRdf->set_npc_id_counter(npcIdCounter);
    startRdf->set_npc_count(npcIdCounter - 1);

    startRdf->set_bullet_id_counter(bulletIdCounter);
    startRdf->set_pickable_id_counter(pickableIdCounter);

    startRdf->set_pickable_id_counter(pickableIdCounter);

    int dynamicTrapCount = 0;
    auto dynamicTrap1 = startRdf->add_dynamic_traps();
    dynamicTrap1->set_id(42);
    dynamicTrap1->set_tpt(globalPrimitiveConsts->tpt_sliding_platform());
    ++dynamicTrapCount;

    auto dynamicTrap2 = startRdf->add_dynamic_traps();
    dynamicTrap2->set_id(43);
    dynamicTrap2->set_tpt(globalPrimitiveConsts->tpt_sliding_platform());
    ++dynamicTrapCount;

    startRdf->set_dynamic_trap_count(dynamicTrapCount);

    return startRdf;
}

RenderFrame* mockFallenDeathRdf() {
    auto chSpecies = globalPrimitiveConsts->ch_species();
    const int roomCapacity = 2;
    auto startRdf = BaseBattle::NewPreallocatedRdf(roomCapacity, 8, 128);
    startRdf->set_id(globalPrimitiveConsts->starting_render_frame_id());
    uint32_t pickableIdCounter = 1;
    uint32_t npcIdCounter = 1;
    uint32_t bulletIdCounter = 1;

    auto characterConfigs = globalConfigConsts->character_configs();

    auto player1 = startRdf->mutable_players(0);
    auto playerCh1 = player1->mutable_chd();
    auto playerCh1Species = chSpecies.bountyhunter();
    auto cc1 = characterConfigs[playerCh1Species];
    playerCh1->set_x(-85);
    playerCh1->set_y(200);
    playerCh1->set_speed(cc1.speed());
    playerCh1->set_ch_state(CharacterState::InAirIdle1NoJump);
    playerCh1->set_frames_to_recover(0);
    playerCh1->set_q_x(0);
    playerCh1->set_q_y(0);
    playerCh1->set_q_z(0);
    playerCh1->set_q_w(1);
    playerCh1->set_aiming_q_x(0);
    playerCh1->set_aiming_q_y(0);
    playerCh1->set_aiming_q_z(0);
    playerCh1->set_aiming_q_w(1);
    playerCh1->set_vel_x(0);
    playerCh1->set_vel_y(0);
    playerCh1->set_hp(cc1.hp());
    playerCh1->set_species_id(playerCh1Species);
    playerCh1->set_bullet_team_id(1);
    player1->set_join_index(1);
    player1->set_revival_x(playerCh1->x());
    player1->set_revival_y(playerCh1->y());
    player1->set_revival_q_x(0);
    player1->set_revival_q_y(0);
    player1->set_revival_q_z(0);
    player1->set_revival_q_w(1);

    auto player2 = startRdf->mutable_players(1);
    auto playerCh2 = player2->mutable_chd();
    auto playerCh2Species = chSpecies.bladegirl();
    auto cc2 = characterConfigs[playerCh2Species];
    playerCh2->set_x(+80);
    playerCh2->set_y(200);
    playerCh2->set_speed(cc2.speed());
    playerCh2->set_ch_state(CharacterState::InAirIdle1NoJump);
    playerCh2->set_frames_to_recover(0);
    playerCh2->set_q_x(cTurnbackAroundYAxis.GetX());
    playerCh2->set_q_y(cTurnbackAroundYAxis.GetY());
    playerCh2->set_q_z(cTurnbackAroundYAxis.GetZ());
    playerCh2->set_q_w(cTurnbackAroundYAxis.GetW());
    playerCh2->set_aiming_q_x(0);
    playerCh2->set_aiming_q_y(0);
    playerCh2->set_aiming_q_z(0);
    playerCh2->set_aiming_q_w(1);
    playerCh2->set_vel_x(0);
    playerCh2->set_vel_y(0);
    playerCh2->set_hp(cc2.hp());
    playerCh2->set_species_id(playerCh2Species);
    playerCh2->set_bullet_team_id(2);
    player2->set_join_index(2);
    player2->set_revival_x(playerCh2->x());
    player2->set_revival_y(playerCh2->y());
    player2->set_revival_q_x(cTurnbackAroundYAxis.GetX());
    player2->set_revival_q_y(cTurnbackAroundYAxis.GetY());
    player2->set_revival_q_z(cTurnbackAroundYAxis.GetZ());
    player2->set_revival_q_w(cTurnbackAroundYAxis.GetW());

    startRdf->set_npc_id_counter(npcIdCounter);
    startRdf->set_bullet_id_counter(bulletIdCounter);
    startRdf->set_pickable_id_counter(pickableIdCounter);

    return startRdf;
}

RenderFrame* mockBladeGirlSkillRdf() {
    auto chSpecies = globalPrimitiveConsts->ch_species();
    const int roomCapacity = 2;
    auto startRdf = BaseBattle::NewPreallocatedRdf(roomCapacity, 8, 128);
    startRdf->set_id(globalPrimitiveConsts->starting_render_frame_id());
    uint32_t pickableIdCounter = 1;
    uint32_t npcIdCounter = 1;
    uint32_t bulletIdCounter = 1;

    auto characterConfigs = globalConfigConsts->character_configs();

    auto player1 = startRdf->mutable_players(0);
    auto playerCh1 = player1->mutable_chd();
    auto playerCh1Species = chSpecies.bladegirl();
    auto cc1 = characterConfigs[playerCh1Species];
    playerCh1->set_x(-85);
    playerCh1->set_y(200);
    playerCh1->set_speed(cc1.speed());
    playerCh1->set_ch_state(CharacterState::InAirIdle1NoJump);
    playerCh1->set_frames_to_recover(0);
    playerCh1->set_q_x(0);
    playerCh1->set_q_y(0);
    playerCh1->set_q_z(0);
    playerCh1->set_q_w(1);
    playerCh1->set_aiming_q_x(0);
    playerCh1->set_aiming_q_y(0);
    playerCh1->set_aiming_q_z(0);
    playerCh1->set_aiming_q_w(1);
    playerCh1->set_vel_x(0);
    playerCh1->set_vel_y(0);
    playerCh1->set_hp(cc1.hp());
    playerCh1->set_species_id(playerCh1Species);
    playerCh1->set_bullet_team_id(1);
    player1->set_join_index(1);
    player1->set_revival_x(playerCh1->x());
    player1->set_revival_y(playerCh1->y());
    player1->set_revival_q_x(0);
    player1->set_revival_q_y(0);
    player1->set_revival_q_z(0);
    player1->set_revival_q_w(1);

    auto player2 = startRdf->mutable_players(1);
    auto playerCh2 = player2->mutable_chd();
    auto playerCh2Species = chSpecies.bountyhunter();
    auto cc2 = characterConfigs[playerCh2Species];
    playerCh2->set_x(+90);
    playerCh2->set_y(200);
    playerCh2->set_speed(cc2.speed());
    playerCh2->set_ch_state(CharacterState::InAirIdle1NoJump);
    playerCh2->set_frames_to_recover(0);
    playerCh2->set_q_x(cTurnbackAroundYAxis.GetX());
    playerCh2->set_q_y(cTurnbackAroundYAxis.GetY());
    playerCh2->set_q_z(cTurnbackAroundYAxis.GetZ());
    playerCh2->set_q_w(cTurnbackAroundYAxis.GetW());
    playerCh2->set_aiming_q_x(0);
    playerCh2->set_aiming_q_y(0);
    playerCh2->set_aiming_q_z(0);
    playerCh2->set_aiming_q_w(1);
    playerCh2->set_vel_x(0);
    playerCh2->set_vel_y(0);
    playerCh2->set_hp(cc2.hp());
    playerCh2->set_species_id(playerCh2Species);
    playerCh2->set_bullet_team_id(2);
    player2->set_join_index(2);
    player2->set_revival_x(playerCh2->x());
    player2->set_revival_y(playerCh2->y());
    player2->set_revival_q_x(cTurnbackAroundYAxis.GetX());
    player2->set_revival_q_y(cTurnbackAroundYAxis.GetY());
    player2->set_revival_q_z(cTurnbackAroundYAxis.GetZ());
    player2->set_revival_q_w(cTurnbackAroundYAxis.GetW());

    auto npc1 = startRdf->mutable_npcs(0);
    npc1->set_id(npcIdCounter++);
    auto npcCh1 = npc1->mutable_chd();
    auto npcCh1Species = chSpecies.bladegirl();
    auto npcCc1 = characterConfigs[npcCh1Species];
    npcCh1->set_x(+5);
    npcCh1->set_y(200);
    npcCh1->set_speed(npcCc1.speed());
    npcCh1->set_ch_state(CharacterState::InAirIdle1NoJump);
    npcCh1->set_frames_to_recover(0);
    npcCh1->set_q_x(cTurnbackAroundYAxis.GetX());
    npcCh1->set_q_y(cTurnbackAroundYAxis.GetY());
    npcCh1->set_q_z(cTurnbackAroundYAxis.GetZ());
    npcCh1->set_q_w(cTurnbackAroundYAxis.GetW());
    npcCh1->set_aiming_q_x(0);
    npcCh1->set_aiming_q_y(0);
    npcCh1->set_aiming_q_z(0);
    npcCh1->set_aiming_q_w(1);
    npcCh1->set_vel_x(0);
    npcCh1->set_vel_y(0);
    npcCh1->set_hp(npcCc1.hp());
    npcCh1->set_species_id(npcCh1Species);
    npcCh1->set_bullet_team_id(3);
    
    startRdf->set_npc_id_counter(npcIdCounter);
    startRdf->set_npc_count(npcIdCounter-1);

    startRdf->set_bullet_id_counter(bulletIdCounter);
    startRdf->set_pickable_id_counter(pickableIdCounter);

    return startRdf;
}

RenderFrame* mockBountyHunterSkillRdf() {
    auto chSpecies = globalPrimitiveConsts->ch_species();
    const int roomCapacity = 2;
    auto startRdf = BaseBattle::NewPreallocatedRdf(roomCapacity, 8, 128);
    startRdf->set_id(globalPrimitiveConsts->starting_render_frame_id());
    uint32_t pickableIdCounter = 1;
    uint32_t npcIdCounter = 1;
    uint32_t bulletIdCounter = 1;

    auto characterConfigs = globalConfigConsts->character_configs();

    auto player1 = startRdf->mutable_players(0);
    auto playerCh1 = player1->mutable_chd();
    auto playerCh1Species = chSpecies.bountyhunter();
    auto cc1 = characterConfigs[playerCh1Species];
    playerCh1->set_x(-85);
    playerCh1->set_y(200);
    playerCh1->set_speed(cc1.speed());
    playerCh1->set_ch_state(CharacterState::InAirIdle1NoJump);
    playerCh1->set_frames_to_recover(0);
    playerCh1->set_q_x(0);
    playerCh1->set_q_y(0);
    playerCh1->set_q_z(0);
    playerCh1->set_q_w(1);
    playerCh1->set_aiming_q_x(0);
    playerCh1->set_aiming_q_y(0);
    playerCh1->set_aiming_q_z(0);
    playerCh1->set_aiming_q_w(1);
    playerCh1->set_vel_x(0);
    playerCh1->set_vel_y(0);
    playerCh1->set_hp(cc1.hp());
    playerCh1->set_species_id(playerCh1Species);
    playerCh1->set_bullet_team_id(1);
    player1->set_join_index(1);
    player1->set_revival_x(playerCh1->x());
    player1->set_revival_y(playerCh1->y());
    player1->set_revival_q_x(0);
    player1->set_revival_q_y(0);
    player1->set_revival_q_z(0);
    player1->set_revival_q_w(1);

    auto player2 = startRdf->mutable_players(1);
    auto playerCh2 = player2->mutable_chd();
    auto playerCh2Species = chSpecies.bladegirl();
    auto cc2 = characterConfigs[playerCh2Species];
    playerCh2->set_x(+90);
    playerCh2->set_y(200);
    playerCh2->set_speed(cc2.speed());
    playerCh2->set_ch_state(CharacterState::InAirIdle1NoJump);
    playerCh2->set_frames_to_recover(0);
    playerCh2->set_q_x(cTurnbackAroundYAxis.GetX());
    playerCh2->set_q_y(cTurnbackAroundYAxis.GetY());
    playerCh2->set_q_z(cTurnbackAroundYAxis.GetZ());
    playerCh2->set_q_w(cTurnbackAroundYAxis.GetW());
    playerCh2->set_aiming_q_x(0);
    playerCh2->set_aiming_q_y(0);
    playerCh2->set_aiming_q_z(0);
    playerCh2->set_aiming_q_w(1);
    playerCh2->set_vel_x(0);
    playerCh2->set_vel_y(0);
    playerCh2->set_hp(cc2.hp());
    playerCh2->set_species_id(playerCh2Species);
    playerCh2->set_bullet_team_id(2);
    player2->set_join_index(2);
    player2->set_revival_x(playerCh2->x());
    player2->set_revival_y(playerCh2->y());
    player2->set_revival_q_x(cTurnbackAroundYAxis.GetX());
    player2->set_revival_q_y(cTurnbackAroundYAxis.GetY());
    player2->set_revival_q_z(cTurnbackAroundYAxis.GetZ());
    player2->set_revival_q_w(cTurnbackAroundYAxis.GetW());

    auto npc1 = startRdf->mutable_npcs(0);
    npc1->set_id(npcIdCounter++);
    auto npcCh1 = npc1->mutable_chd();
    auto npcCh1Species = chSpecies.bladegirl();
    auto npcCc1 = characterConfigs[npcCh1Species];
    npcCh1->set_x(+5);
    npcCh1->set_y(200);
    npcCh1->set_speed(npcCc1.speed());
    npcCh1->set_ch_state(CharacterState::InAirIdle1NoJump);
    npcCh1->set_frames_to_recover(0);
    npcCh1->set_q_x(cTurnbackAroundYAxis.GetX());
    npcCh1->set_q_y(cTurnbackAroundYAxis.GetY());
    npcCh1->set_q_z(cTurnbackAroundYAxis.GetZ());
    npcCh1->set_q_w(cTurnbackAroundYAxis.GetW());
    npcCh1->set_aiming_q_x(0);
    npcCh1->set_aiming_q_y(0);
    npcCh1->set_aiming_q_z(0);
    npcCh1->set_aiming_q_w(1);
    npcCh1->set_vel_x(0);
    npcCh1->set_vel_y(0);
    npcCh1->set_hp(npcCc1.hp());
    npcCh1->set_species_id(npcCh1Species);
    npcCh1->set_bullet_team_id(3);
    
    startRdf->set_npc_id_counter(npcIdCounter);
    startRdf->set_npc_count(npcIdCounter-1);

    startRdf->set_bullet_id_counter(bulletIdCounter);
    startRdf->set_pickable_id_counter(pickableIdCounter);

    return startRdf;
}

RenderFrame* mockVictoryRdf() {
    auto chSpecies = globalPrimitiveConsts->ch_species();
    const int roomCapacity = 1;
    auto startRdf = BaseBattle::NewPreallocatedRdf(roomCapacity, 8, 128);
    startRdf->set_id(globalPrimitiveConsts->starting_render_frame_id());
    uint32_t pickableIdCounter = 1;
    uint32_t npcIdCounter = 1;
    uint32_t bulletIdCounter = 1;

    int triggerCount = 0;

    auto characterConfigs = globalConfigConsts->character_configs();

    auto player1 = startRdf->mutable_players(0);
    auto playerCh1 = player1->mutable_chd();
    auto playerCh1Species = chSpecies.bountyhunter();
    auto cc1 = characterConfigs[playerCh1Species];
    playerCh1->set_x(-85);
    playerCh1->set_y(200);
    playerCh1->set_speed(cc1.speed());
    playerCh1->set_ch_state(CharacterState::InAirIdle1NoJump);
    playerCh1->set_frames_to_recover(0);
    playerCh1->set_q_x(0);
    playerCh1->set_q_y(0);
    playerCh1->set_q_z(0);
    playerCh1->set_q_w(1);
    playerCh1->set_aiming_q_x(0);
    playerCh1->set_aiming_q_y(0);
    playerCh1->set_aiming_q_z(0);
    playerCh1->set_aiming_q_w(1);
    playerCh1->set_vel_x(0);
    playerCh1->set_vel_y(0);
    playerCh1->set_hp(cc1.hp());
    playerCh1->set_species_id(playerCh1Species);
    playerCh1->set_bullet_team_id(1);
    player1->set_join_index(1);
    player1->set_revival_x(playerCh1->x());
    player1->set_revival_y(playerCh1->y());
    player1->set_revival_q_x(0);
    player1->set_revival_q_y(0);
    player1->set_revival_q_z(0);
    player1->set_revival_q_w(1);

    uint32_t victoryTriggerId = 42;

    auto npc1 = startRdf->mutable_npcs(0);
    npc1->set_id(npcIdCounter++);
    npc1->set_publishing_to_trigger_id_upon_exhausted(victoryTriggerId);    
    auto npcCh1 = npc1->mutable_chd();
    auto npcCh1Species = chSpecies.bladegirl();
    auto npcCc1 = characterConfigs[npcCh1Species];
    npcCh1->set_x(+5);
    npcCh1->set_y(200);
    npcCh1->set_speed(npcCc1.speed());
    npcCh1->set_ch_state(CharacterState::InAirIdle1NoJump);
    npcCh1->set_frames_to_recover(0);
    npcCh1->set_q_x(cTurnbackAroundYAxis.GetX());
    npcCh1->set_q_y(cTurnbackAroundYAxis.GetY());
    npcCh1->set_q_z(cTurnbackAroundYAxis.GetZ());
    npcCh1->set_q_w(cTurnbackAroundYAxis.GetW());
    npcCh1->set_aiming_q_x(0);
    npcCh1->set_aiming_q_y(0);
    npcCh1->set_aiming_q_z(0);
    npcCh1->set_aiming_q_w(1);
    npcCh1->set_vel_x(0);
    npcCh1->set_vel_y(0);
    npcCh1->set_hp(10); // For easier death
    npcCh1->set_species_id(npcCh1Species);
    npcCh1->set_bullet_team_id(3);

    auto npc2 = startRdf->mutable_npcs(1);
    npc2->set_id(npcIdCounter++);
    npc2->set_publishing_to_trigger_id_upon_exhausted(victoryTriggerId);    
    auto npcCh2 = npc2->mutable_chd();
    auto npcCh2Species = chSpecies.bladegirl();
    auto npcCc2 = characterConfigs[npcCh2Species];
    npcCh2->set_x(+80);
    npcCh2->set_y(200);
    npcCh2->set_speed(npcCc2.speed());
    npcCh2->set_ch_state(CharacterState::InAirIdle1NoJump);
    npcCh2->set_frames_to_recover(0);
    npcCh2->set_q_x(cTurnbackAroundYAxis.GetX());
    npcCh2->set_q_y(cTurnbackAroundYAxis.GetY());
    npcCh2->set_q_z(cTurnbackAroundYAxis.GetZ());
    npcCh2->set_q_w(cTurnbackAroundYAxis.GetW());
    npcCh2->set_aiming_q_x(0);
    npcCh2->set_aiming_q_y(0);
    npcCh2->set_aiming_q_z(0);
    npcCh2->set_aiming_q_w(1);
    npcCh2->set_vel_x(0);
    npcCh2->set_vel_y(0);
    npcCh2->set_hp(10); // For easier death
    npcCh2->set_species_id(npcCh2Species);
    npcCh2->set_bullet_team_id(3);

    auto tr1 = startRdf->add_triggers();
    tr1->set_id(victoryTriggerId);
    tr1->set_trt(globalPrimitiveConsts->trt_victory());
    ++triggerCount;
    
    startRdf->set_npc_id_counter(npcIdCounter);
    startRdf->set_npc_count(npcIdCounter-1);

    startRdf->set_bullet_id_counter(bulletIdCounter);
    startRdf->set_pickable_id_counter(pickableIdCounter);

    startRdf->set_trigger_count(triggerCount);

    return startRdf;
}

RenderFrame* mockRefRdf(int refRdfId) {
    auto chSpecies = globalPrimitiveConsts->ch_species();
    const int roomCapacity = 2;
    auto refRdf = BaseBattle::NewPreallocatedRdf(roomCapacity, 8, 128);
    refRdf->set_id(refRdfId);
    uint32_t pickableIdCounter = 1;
    uint32_t npcIdCounter = 1;
    uint32_t bulletIdCounter = 1;

    auto player1 = refRdf->mutable_players(0);
    auto playerCh1 = player1->mutable_chd();
    playerCh1->set_x(-93);
    playerCh1->set_y(100);
    playerCh1->set_speed(10);
    playerCh1->set_ch_state(CharacterState::InAirIdle1NoJump);
    playerCh1->set_frames_to_recover(0);
    playerCh1->set_q_x(0);
    playerCh1->set_q_y(0);
    playerCh1->set_q_z(0);
    playerCh1->set_q_w(1);
    playerCh1->set_aiming_q_x(0);
    playerCh1->set_aiming_q_y(0);
    playerCh1->set_aiming_q_z(0);
    playerCh1->set_aiming_q_w(1);
    playerCh1->set_vel_x(0);
    playerCh1->set_vel_y(0);
    playerCh1->set_hp(100);
    playerCh1->set_species_id(chSpecies.bladegirl());
    player1->set_join_index(1);
    player1->set_revival_x(playerCh1->x());
    player1->set_revival_y(playerCh1->y());
    player1->set_revival_q_x(0);
    player1->set_revival_q_y(0);
    player1->set_revival_q_z(0);
    player1->set_revival_q_w(1);

    auto player2 = refRdf->mutable_players(1);
    auto playerCh2 = player2->mutable_chd();
    playerCh2->set_x(50);
    playerCh2->set_y(100);
    playerCh2->set_speed(10);
    playerCh2->set_ch_state(CharacterState::InAirIdle1NoJump);
    playerCh2->set_frames_to_recover(0);
    playerCh2->set_q_x(cTurnbackAroundYAxis.GetX());
    playerCh2->set_q_y(cTurnbackAroundYAxis.GetY());
    playerCh2->set_q_z(cTurnbackAroundYAxis.GetZ());
    playerCh2->set_q_w(cTurnbackAroundYAxis.GetW());
    playerCh2->set_aiming_q_x(0);
    playerCh2->set_aiming_q_y(0);
    playerCh2->set_aiming_q_z(0);
    playerCh2->set_aiming_q_w(1);
    playerCh2->set_vel_x(0);
    playerCh2->set_vel_y(0);
    playerCh2->set_hp(100);
    playerCh2->set_species_id(chSpecies.bountyhunter());
    player2->set_join_index(2);
    player2->set_revival_x(playerCh2->x());
    player2->set_revival_y(playerCh2->y());
    player2->set_revival_q_x(cTurnbackAroundYAxis.GetX());
    player2->set_revival_q_y(cTurnbackAroundYAxis.GetY());
    player2->set_revival_q_z(cTurnbackAroundYAxis.GetZ());
    player2->set_revival_q_w(cTurnbackAroundYAxis.GetW());

    refRdf->set_npc_id_counter(npcIdCounter);
    refRdf->set_bullet_id_counter(bulletIdCounter);
    refRdf->set_pickable_id_counter(pickableIdCounter);

    return refRdf;
}

void DebugLogCb(const char* message, int color, int size) {
    std::cout << message << std::endl;
}

std::map<int, uint64_t> testCmds1 = {
    {0, 0},
    {60, 0},
    {64, 32},
    {65, 0},
    {227, 0},
    {228, 16},
    {232, 16},
    {248, 3},
    {252, 16},
    {253, 16},
    {260, 0},
    {699, 0},
    {700, 256},
    {720, 3},
    {750, 3},
    {754, 19},
    {758, 3},
    {766, 3},
    {768, 3},
    {772, 19},
    {776, 3},
    {780, 3},
    {784, 3},
    {788, 3},
    {792, 3},
    {796, 3},
    {800, 20},
    {804, 4},
    {808, 4},
    {812, 4},
    {816, 4},
    {832, 4},
    {834, 20},
    {836, 20},
    {848, 4},
    {852, 4},
    {856, 4},
    {860, 4},
    {864, 4},
    {868, 4},
    {1200, 0}
};

std::map<int, uint64_t> testCmds2 = {
    {0, 3},
    {50, 3},
    {56, 3},
    {60, 0},
    {64, 32},
    {68, 3},
    {72, 0},
    {227, 0},
    {228, 3},
    {232, 3},
    {320, 3},
    {324, 35},
    {328, 35},
    {332, 3},
    {360, 3},
    {364, 0},
    {699, 0},
    {700, 256},
    {720, 3},
    {750, 3},
    {754, 19},
    {758, 3},
    {766, 3},
    {768, 3},
    {772, 20},
    {776, 4},
    {780, 4},
    {784, 4},
    {788, 4},
    {792, 4},
    {796, 20},
    {1200, 0}
};

std::map<int, uint64_t> testCmds4 = {
    {0, 3},
    {227, 0},
    {228, 16},
    {231, 16},
    {251, 0},
    {252, 16},
    {699, 0},
    {700, 20},
    {720, 4},
    {739, 4},
    {740, 20},
    {770, 4},
    {775, 4},
    {790, 20},
    {795, 4},
    {820, 20},
    {821, 4},
    {910, 4},
    {1100, 4},
    {1200, 0}
};

std::map<int, uint64_t> testCmds8 = {
    {0, 0},
    {60, 0},
    {64, 32},
    {65, 0},
    {70, 3},
    {96, 3},
    {512, 3},
    {513, 19},
    {540, 3},
    {580, 3},
    {581, 0}
};

std::map<int, uint64_t> testCmds9 = {
    {0, 3},
    {59, 3},
    {60, 0},
    {64, 4},
    {119, 4},
    {120, 0},
    {180, 0},
    {227, 0},
    {228, 32},
    {229, 0},
    {300, 32},
    {301, 0},
    {420, 32},
    {421, 0},
    {699, 32},
    {700, 0},
    {720, 4},
    {739, 4},
    {740, 20},
    {760, 4},
    {780, 20},
    {781, 4},
    {820, 20},
    {821, 4},
    {910, 4},
    {1100, 4},
    {1200, 0}
};

std::map<int, uint64_t> testCmds10 = {
    {59, 0},
    {60, 4},
    {179, 4},
    {180, 20},
    {181, 4},
    {220, 4},
    {221, 20},
    {222, 4},
    {249, 4},
    {250, 3},
    {480, 3},
    {2048, 0},
    {9999, 0}
};

std::map<int, uint64_t> testCmds11 = {
    {0, 3},
    {32, 3},
    {40, 3},
    {44, 0},
    {160, 4}, // toGenIfdId=40
    {163, 4}, // toGenIfdId=40
    {164, 4}, // toGenIfdId=41
    {166, 4}, // toGenIfdId=41
    {167, 20}, // toGenIfdId=41, [WARNING] Intentionally trigger a self-input-induced rollback
    {191, 0},
    {192, 3},
    {200, 3},
    {220, 4},
    {240, 0},
    {360, 0}
};

std::map<int, uint64_t> testCmds12 = {
    {0, 3},
    {32, 0},
    {39, 0},
    {40, 32},
    {41, 0},
    {59, 0}, 
    {60, 32}, 
    {61, 0}, 
    {69, 0}, 
    {70, 32}, 
    {71, 0} 
};

std::map<int, uint64_t> testCmds13 = {
    {0, 0},
    {8, 32},
    {9, 0},
    {16, 3},
    {58, 3}, 
    {59, 0}, 
    {60, 32}, 
    {61, 0}, 
    {69, 0}, 
    {70, 32}, 
    {71, 0},
    {99, 0},
    {100, 256},
    {104, 3},
    {108, 3},
    {112, 3},
    {116, 0},
};

std::map<int, uint64_t> testCmds14 = {
    {0, 3},
    {50, 3}, 
    {51, 0}, 
    {60, 0}, 
    {61, 0}, 
    {67, 0},
    {68, 32},
    {69, 0}, 
    {70, 0}, 
    {71, 0},
    {100, 0},
    {111, 19},
    {110, 0},
    {119, 19},
    {120, 0},
    {122, 0},
    {179, 0},
    {180, 32},
    {181, 0},
    {182, 3},
    {205, 3},
    {206, 0},
    {208, 19},
    {209, 0},
    {210, 3},
    {226, 3},
    {227, 0},
    {228, 19},
    {229, 0},
    {247, 0},
    {248, 4},
    {251, 4},
    {255, 4},
    {256, 0},
    {283, 0},
    {284, 32},
    {285, 0},
    {320, 0},
    {321, 3},
    {340, 3},
    {360, 0},
    {361, 32},
    {362, 0},
    {1024, 0}
};

std::map<int, uint64_t> testCmds15 = {
    {0, 3},
    {50, 3}, 
    {51, 0}, 
    {60, 0}, 
    {61, 0}, 
    {67, 0},
    {68, 32},
    {69, 0}, 
    {70, 0}, 
    {71, 0},
    {100, 0},
    {111, 0},
    {110, 0},
    {119, 0},
    {120, 0},
    {122, 0},
    {179, 0},
    {180, 32},
    {181, 0},
    {300, 0},
    {301, 32},
    {302, 0},
    {1024, 0}
};

std::map<int, uint64_t> testCmds16 = {
    {0, 3},
    {59, 3},
    {60, 0},
    {64, 4},
    {119, 4},
    {120, 0},
    {180, 0},
    {227, 0},
    {228, 32},
    {229, 0},
    {300, 32},
    {301, 0},
    {420, 32},
    {421, 0},
    {699, 32},
    {700, 0},
    {720, 4},
    {739, 4},
    {740, 20},
    {760, 4},
    {780, 20},
    {781, 4},
    {820, 20},
    {821, 4},
    {910, 4},
    {1100, 4},
    {1200, 0}
};

std::map<int, uint64_t> testCmds17 = {
    {0, 0},
    {23, 0},
    {24, 19},
    {64, 19},
    {65, 3},
    {108, 3},
    {109, 0},
    {1024, 0},
};

std::map<int, uint64_t> testCmds18 = {
    {0, 3},
    {32, 3},
    {40, 35},
    {44, 0},
    {160, 3}, // toGenIfdId=40
    {163, 35}, // toGenIfdId=40, [WARNING] Intentionally trigger a self-input-induced rollback
    {164, 3}, // toGenIfdId=41
    {166, 3}, // toGenIfdId=41
    {167, 19}, // toGenIfdId=41, [WARNING] Intentionally trigger a self-input-induced rollback
    {191, 0},
    {192, 35},
    {200, 3},
    {220, 4},
    {240, 0},
    {360, 0}
};

uint64_t getSelfCmdByRdfId(std::map<int, uint64_t>& testCmds, int rdfId) {
    auto it = testCmds.lower_bound(rdfId);
    if (it == testCmds.end()) {
        --it;
    }
    return it->second;
}

uint64_t getSelfCmdByIfdId(std::map<int, uint64_t>& testCmds, int ifdId) {
    int rdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(ifdId);
    return getSelfCmdByRdfId(testCmds, rdfId);
}

std::unordered_map<int, WsReq*> incomingUpsyncSnapshotReqs1; // key is "timerRdfId"; random order of "UpsyncSnapshot.st_ifd_id", relatively big packet loss 
std::unordered_map<int, DownsyncSnapshot*> incomingDownsyncSnapshots1; // key is "timerRdfId"; non-descending order of "DownsyncSnapshot.st_ifd_id" (see comments on "inputBufferLock" in "joltc_api.h"), relatively small packet loss

std::unordered_map<int, WsReq*> incomingUpsyncSnapshotReqs2;
std::unordered_map<int, DownsyncSnapshot*> incomingDownsyncSnapshots2; 

std::unordered_map<int, WsReq*> incomingUpsyncSnapshotReqs3;
std::unordered_map<int, DownsyncSnapshot*> incomingDownsyncSnapshots3; 

std::unordered_map<int, DownsyncSnapshot*> incomingDownsyncSnapshots4; 

std::unordered_map<int, DownsyncSnapshot*> incomingDownsyncSnapshots5;

std::unordered_map<int, WsReq*> incomingUpsyncSnapshotReqs6Intime;
std::unordered_map<int, WsReq*> incomingUpsyncSnapshotReqs6Rollback;

std::unordered_map<int, WsReq*> incomingUpsyncSnapshotReqs7Intime;
std::unordered_map<int, WsReq*> incomingUpsyncSnapshotReqs7Rollback;

std::unordered_map<int, WsReq*> incomingUpsyncSnapshotReqs10;  
std::unordered_map<int, DownsyncSnapshot*> incomingDownsyncSnapshots10; 

std::unordered_map<int, WsReq*> incomingUpsyncSnapshotReqs11Intime;
std::unordered_map<int, WsReq*> incomingUpsyncSnapshotReqs11Rollback;

std::unordered_map<int, WsReq*> incomingUpsyncSnapshotReqs12Intime;

std::unordered_map<int, WsReq*> incomingUpsyncSnapshotReqs14Intime;

std::unordered_map<int, WsReq*> incomingUpsyncSnapshotReqs18Intime;
std::unordered_map<int, WsReq*> incomingUpsyncSnapshotReqs18Rollback;

void initTest1Data() {
    // incomingUpsyncSnapshotReqs1
    {
        int receivedTimerRdfId = 120;
        int receivedEdIfdId = 26;
        int receivedStIfdId = 13;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(4);
        }
        
        incomingUpsyncSnapshotReqs1[receivedTimerRdfId] = req;
    }
    {
        int receivedTimerRdfId = 155;
        int receivedEdIfdId = 13;
        int receivedStIfdId = 0;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        int mid = 6;
        for (int ifdId = receivedStIfdId; ifdId < mid; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(4);
        }
        for (int ifdId = mid; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(19);
        }
        incomingUpsyncSnapshotReqs1[receivedTimerRdfId] = req;
    }
    {
        int receivedTimerRdfId = 500;
        int receivedEdIfdId = 60;
        int receivedStIfdId = 29;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        int mid = 44;
        for (int ifdId = receivedStIfdId; ifdId < mid; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(4);
        }
        for (int ifdId = mid; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(20);
        }
        incomingUpsyncSnapshotReqs1[receivedTimerRdfId] = req;
    }
    {
        int receivedTimerRdfId = 560;
        int receivedEdIfdId = 29;
        int receivedStIfdId = 26;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(2);
        }
        incomingUpsyncSnapshotReqs1[receivedTimerRdfId] = req;
    }
    // incomingDownsyncSnapshots1
    {
        int receivedTimerRdfId = 180;
        int receivedEdIfdId = 26;
        int receivedStIfdId = 0;
        DownsyncSnapshot* srvDownsyncSnapshot = google::protobuf::Arena::Create<DownsyncSnapshot>(&pbTestCaseDataAllocator);
        srvDownsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            InputFrameDownsync* ifdBatch = srvDownsyncSnapshot->add_ifd_batch();
            ifdBatch->add_input_list(getSelfCmdByIfdId(testCmds1, ifdId));
            ifdBatch->add_input_list(
                6 > ifdId ?
                4
                :
                (13 > ifdId ? 4 : 19)
            );
        }
        incomingDownsyncSnapshots1[receivedTimerRdfId] = srvDownsyncSnapshot;
    }
    {
        int receivedTimerRdfId = 570;
        int receivedEdIfdId = 58;
        int receivedStIfdId = 26;
        DownsyncSnapshot* srvDownsyncSnapshot = google::protobuf::Arena::Create<DownsyncSnapshot>(&pbTestCaseDataAllocator);
        srvDownsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            InputFrameDownsync* ifdBatch = srvDownsyncSnapshot->add_ifd_batch();
            ifdBatch->add_input_list(getSelfCmdByIfdId(testCmds1, ifdId));
            ifdBatch->add_input_list(
                29 > ifdId ?
                2
                :
                (44 > ifdId ? 4 : 20)
            );
        }
        incomingDownsyncSnapshots1[receivedTimerRdfId] = srvDownsyncSnapshot;
    }
}

void initTest2Data() {
    // incomingUpsyncSnapshotReqs2
    {
        int receivedTimerRdfId = 120;
        int receivedEdIfdId = 26;
        int receivedStIfdId = 13;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(4);
        }
        incomingUpsyncSnapshotReqs2[receivedTimerRdfId] = req;
    }
    {
        int receivedTimerRdfId = 155;
        int receivedEdIfdId = 13;
        int receivedStIfdId = 0;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        int mid = 6;
        for (int ifdId = receivedStIfdId; ifdId < mid; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(4);
        }
        for (int ifdId = mid; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(19);
        }
        incomingUpsyncSnapshotReqs2[receivedTimerRdfId] = req;
    }
    {
        int receivedTimerRdfId = 500;
        int receivedEdIfdId = 60;
        int receivedStIfdId = 29;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        int mid = 44;
        for (int ifdId = receivedStIfdId; ifdId < mid; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(4);
        }
        for (int ifdId = mid; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(20);
        }
        incomingUpsyncSnapshotReqs2[receivedTimerRdfId] = req;
    }
    {
        int receivedTimerRdfId = 560;
        int receivedEdIfdId = 29;
        int receivedStIfdId = 26;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(2);
        }
        incomingUpsyncSnapshotReqs2[receivedTimerRdfId] = req;
    }
    // incomingDownsyncSnapshots2
    {
        int receivedTimerRdfId = 180;
        int receivedEdIfdId = 26;
        int receivedStIfdId = 0;
        DownsyncSnapshot* srvDownsyncSnapshot = google::protobuf::Arena::Create<DownsyncSnapshot>(&pbTestCaseDataAllocator);
        srvDownsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            InputFrameDownsync* ifdBatch = srvDownsyncSnapshot->add_ifd_batch();
            ifdBatch->add_input_list(getSelfCmdByIfdId(testCmds1, ifdId));
            ifdBatch->add_input_list(
                6 > ifdId ?
                4
                :
                (13 > ifdId ? 4 : 19)
            );
        }
        incomingDownsyncSnapshots2[receivedTimerRdfId] = srvDownsyncSnapshot;
    }
    {
        int receivedTimerRdfId = 571;
        int refRdfId = 1100;
        int receivedEdIfdId = BaseBattle::ConvertToDelayedInputFrameId(refRdfId)  - 2; // a slightly smaller "receivedEdIfdId" than "ConvertToDelayedInputFrameId(refRdfId)" such that frontend would have to prefab self cmd after reception.
        int receivedStIfdId = receivedEdIfdId - 20;
        int mid = ((receivedStIfdId + receivedEdIfdId) >> 1);
        DownsyncSnapshot* srvDownsyncSnapshot = google::protobuf::Arena::Create<DownsyncSnapshot>(&pbTestCaseDataAllocator);
        srvDownsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            InputFrameDownsync* ifdBatch = srvDownsyncSnapshot->add_ifd_batch();
            ifdBatch->add_input_list(getSelfCmdByIfdId(testCmds1, ifdId));
            ifdBatch->add_input_list(
                mid > ifdId ?
                19
                :
                (mid+3 > ifdId ? 0 : 19)
            );
        }
        incomingDownsyncSnapshots2[receivedTimerRdfId] = srvDownsyncSnapshot;
        RenderFrame* refRdf = mockRefRdf(refRdfId);
        srvDownsyncSnapshot->set_ref_rdf_id(refRdfId);
        srvDownsyncSnapshot->set_allocated_ref_rdf(refRdf);
    }
}

void initTest3Data() {
    // incomingUpsyncSnapshotReqs3
    {
        int receivedTimerRdfId = 120;
        int receivedEdIfdId = 26;
        int receivedStIfdId = 13;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(4);
        }
        incomingUpsyncSnapshotReqs3[receivedTimerRdfId] = req;
    }
    {
        int receivedTimerRdfId = 155;
        int receivedEdIfdId = 13;
        int receivedStIfdId = 0;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        int mid = 6;
        for (int ifdId = receivedStIfdId; ifdId < mid; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(4);
        }
        for (int ifdId = mid; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(19);
        }
        incomingUpsyncSnapshotReqs3[receivedTimerRdfId] = req;
    }
    {
        int receivedTimerRdfId = 500;
        int receivedEdIfdId = 60;
        int receivedStIfdId = 29;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        int mid = 44;
        for (int ifdId = receivedStIfdId; ifdId < mid; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(4);
        }
        for (int ifdId = mid; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(20);
        }
        incomingUpsyncSnapshotReqs3[receivedTimerRdfId] = req;
    }
    {
        int receivedTimerRdfId = 560;
        int receivedEdIfdId = 29;
        int receivedStIfdId = 26;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(2);
        }
        incomingUpsyncSnapshotReqs3[receivedTimerRdfId] = req;
    }
    // incomingDownsyncSnapshots3
    {
        int receivedTimerRdfId = 180;
        int receivedEdIfdId = 26;
        int receivedStIfdId = 0;
        DownsyncSnapshot* srvDownsyncSnapshot = google::protobuf::Arena::Create<DownsyncSnapshot>(&pbTestCaseDataAllocator);
        srvDownsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            InputFrameDownsync* ifdBatch = srvDownsyncSnapshot->add_ifd_batch();
            ifdBatch->add_input_list(getSelfCmdByIfdId(testCmds1, ifdId));
            ifdBatch->add_input_list(
                6 > ifdId ?
                4
                :
                (13 > ifdId ? 4 : 19)
            );
        }
        incomingDownsyncSnapshots3[receivedTimerRdfId] = srvDownsyncSnapshot;
    }
    {
        int receivedTimerRdfId = 800;
        int refRdfId = 780;
        int receivedEdIfdId = BaseBattle::ConvertToDelayedInputFrameId(refRdfId) - 3;
        int receivedStIfdId = 30;
        int mid = ((receivedStIfdId + receivedEdIfdId) >> 1);
        DownsyncSnapshot* srvDownsyncSnapshot = google::protobuf::Arena::Create<DownsyncSnapshot>(&pbTestCaseDataAllocator);
        srvDownsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            InputFrameDownsync* ifdBatch = srvDownsyncSnapshot->add_ifd_batch();
            ifdBatch->add_input_list(getSelfCmdByIfdId(testCmds1, ifdId));
            ifdBatch->add_input_list(
                mid > ifdId ?
                19
                :
                (mid+3 > ifdId ? 0 : 19)
            );
        }
        incomingDownsyncSnapshots3[receivedTimerRdfId] = srvDownsyncSnapshot;
        RenderFrame* refRdf = mockRefRdf(refRdfId);
        srvDownsyncSnapshot->set_ref_rdf_id(refRdfId);
        srvDownsyncSnapshot->set_allocated_ref_rdf(refRdf);
    }
}

void initTest4Data() {
    // incomingDownsyncSnapshots4
    {
        int receivedEdIfdId = 4;
        int receivedStIfdId = 0;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedEdIfdId - 1) - 1; // A few rdfs after the last self-input generation, right before being used
        DownsyncSnapshot* srvDownsyncSnapshot = google::protobuf::Arena::Create<DownsyncSnapshot>(&pbTestCaseDataAllocator);
        srvDownsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            InputFrameDownsync* ifdBatch = srvDownsyncSnapshot->add_ifd_batch();
            ifdBatch->add_input_list(getSelfCmdByIfdId(testCmds4, ifdId));
            ifdBatch->add_input_list(
                6 > ifdId ?
                4
                :
                (13 > ifdId ? 4 : 19)
            );
        }
        int refRdfId = BaseBattle::ConvertToLastUsedRenderFrameId(receivedEdIfdId - 1) + 1;
        srvDownsyncSnapshot->set_ref_rdf_id(refRdfId);
        srvDownsyncSnapshot->set_allocated_ref_rdf(mockRefRdf(refRdfId));
        incomingDownsyncSnapshots4[receivedTimerRdfId] = srvDownsyncSnapshot;
    }
}

void initTest5Data() {
    // incomingDownsyncSnapshots5
    {
        int receivedTimerRdfId = 180;
        int receivedEdIfdId = 26;
        int receivedStIfdId = 0;
        DownsyncSnapshot* srvDownsyncSnapshot = google::protobuf::Arena::Create<DownsyncSnapshot>(&pbTestCaseDataAllocator);
        srvDownsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            InputFrameDownsync* ifdBatch = srvDownsyncSnapshot->add_ifd_batch();
            ifdBatch->add_input_list(getSelfCmdByIfdId(testCmds1, ifdId));
            ifdBatch->add_input_list(
                6 > ifdId ?
                4
                :
                (13 > ifdId ? 4 : 19)
            );
        }
        incomingDownsyncSnapshots5[receivedTimerRdfId] = srvDownsyncSnapshot;
    }
    {
        int receivedTimerRdfId = 570;
        int receivedEdIfdId = 58;
        int receivedStIfdId = 26;
        DownsyncSnapshot* srvDownsyncSnapshot = google::protobuf::Arena::Create<DownsyncSnapshot>(&pbTestCaseDataAllocator);
        srvDownsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            InputFrameDownsync* ifdBatch = srvDownsyncSnapshot->add_ifd_batch();
            ifdBatch->add_input_list(getSelfCmdByIfdId(testCmds1, ifdId));
            ifdBatch->add_input_list(
                29 > ifdId ?
                2
                :
                (44 > ifdId ? 4 : 20)
            );
        }
        incomingDownsyncSnapshots5[receivedTimerRdfId] = srvDownsyncSnapshot;
    }
}

void initTest6Data() {
    // Intime reference inputs
    {
        int receivedEdIfdId = 13;
        int receivedStIfdId = 0;
        int receivedTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        int mid = 6;
        peerUpsyncSnapshot->add_cmd_list(0);
        for (int ifdId = 1; ifdId < mid; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(4);
        }
        for (int ifdId = mid; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(19);
        }
        incomingUpsyncSnapshotReqs6Intime[receivedTimerRdfId] = req;
    }
    {
        int receivedEdIfdId = 26;
        int receivedStIfdId = 13;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId)-1;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(4);
        }
        incomingUpsyncSnapshotReqs6Intime[receivedTimerRdfId] = req;
    }
    {
        int receivedEdIfdId = 29;
        int receivedStIfdId = 26;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId)-1;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(2);
        }
        incomingUpsyncSnapshotReqs6Intime[receivedTimerRdfId] = req;
    }
    {
        int receivedEdIfdId = 60;
        int receivedStIfdId = 29;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId)-1;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        int mid = 44;
        for (int ifdId = receivedStIfdId; ifdId < mid; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(4);
        }
        for (int ifdId = mid; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(20);
        }
        incomingUpsyncSnapshotReqs6Intime[receivedTimerRdfId] = req;
    }
    {
        int receivedEdIfdId = 61;
        int receivedStIfdId = 60;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId)-1;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs6Intime[receivedTimerRdfId] = req;
    }

    // Rollback triggering inputs
    {
        int receivedTimerRdfId = 120;
        int receivedEdIfdId = 26;
        int receivedStIfdId = 13;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(4);
        }
        incomingUpsyncSnapshotReqs6Rollback[receivedTimerRdfId] = req;
    }
    {
        int receivedTimerRdfId = 155;
        int receivedEdIfdId = 13;
        int receivedStIfdId = 0;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        int mid = 6;
        peerUpsyncSnapshot->add_cmd_list(0);
        for (int ifdId = 1; ifdId < mid; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(4);
        }
        for (int ifdId = mid; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(19);
        }
        incomingUpsyncSnapshotReqs6Rollback[receivedTimerRdfId] = req;
    }
    {
        int receivedTimerRdfId = 300;
        int receivedEdIfdId = 60;
        int receivedStIfdId = 29;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        int mid = 44;
        for (int ifdId = receivedStIfdId; ifdId < mid; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(4);
        }
        for (int ifdId = mid; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(20);
        }
        incomingUpsyncSnapshotReqs6Rollback[receivedTimerRdfId] = req;
    }
    {
        int receivedTimerRdfId = 320;
        int receivedEdIfdId = 29;
        int receivedStIfdId = 26;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(2);
        }
        incomingUpsyncSnapshotReqs6Rollback[receivedTimerRdfId] = req;
    }
    {
        int receivedTimerRdfId = 330;
        int receivedEdIfdId = BaseBattle::ConvertToDelayedInputFrameId(380-1)+1;
        int receivedStIfdId = 60;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs6Rollback[receivedTimerRdfId] = req;
    }
}

void initTest7Data() {
    // Intime reference inputs
    {
        int receivedEdIfdId = 16;
        int receivedStIfdId = 0;
        int receivedTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        peerUpsyncSnapshot->add_cmd_list(0);
        for (int ifdId = 1; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs7Intime[receivedTimerRdfId] = req;
    }
    {
        int receivedEdIfdId = 18;
        int receivedStIfdId = 16;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId)-1;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(4);
        }
        incomingUpsyncSnapshotReqs7Intime[receivedTimerRdfId] = req;
    }

    // Rollback triggering inputs
    {
        int receivedEdIfdId = 16;
        int receivedStIfdId = 0;
        int receivedTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        peerUpsyncSnapshot->add_cmd_list(0);
        for (int ifdId = 1; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs7Rollback[receivedTimerRdfId] = req;
    }
    {
        int receivedEdIfdId = 18;
        int receivedStIfdId = 16;
        int receivedTimerRdfId = 68;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(4);
        }
        incomingUpsyncSnapshotReqs7Rollback[receivedTimerRdfId] = req;
    }
}

void initTest8Data(WsReq* blacksaber1VisionTestInitializerMapData, std::vector<std::vector<float>>& hulls) {
    auto blacksaber1VisionTestStartRdf = mockBlacksaber1VisionTestStartRdf();
    for (auto hull : hulls) {
        auto srcBarrier = blacksaber1VisionTestInitializerMapData->add_serialized_barriers();
        auto srcPolygon = srcBarrier->mutable_polygon();
        float anchorX = 0, anchorY = 0;
        for (int i = 0; i < hull.size(); i += 2) {
            PbVec2* newPt = srcPolygon->add_points();
            newPt->set_x(hull[i]);
            newPt->set_y(hull[i + 1]);
            anchorX += hull[i];
            anchorY += hull[i + 1];
        }
        anchorX /= (hull.size() >> 1);
        anchorY /= (hull.size() >> 1);
        auto anchor = srcPolygon->mutable_anchor();
        anchor->set_x(anchorX);
        anchor->set_y(anchorY);
        srcPolygon->set_is_box(true);
    }
    blacksaber1VisionTestInitializerMapData->set_allocated_self_parsed_rdf(blacksaber1VisionTestStartRdf);
}

void initTest9Data(WsReq* sliderTrapTestInitializerMapData, std::vector<std::vector<float>>& hulls) {
    auto sliderTrapTestStartRdf = mockSliderTrapTestStartRdf1();
    for (auto hull : hulls) {
        auto srcBarrier = sliderTrapTestInitializerMapData->add_serialized_barriers();
        auto srcPolygon = srcBarrier->mutable_polygon();
        float anchorX = 0, anchorY = 0;
        for (int i = 0; i < hull.size(); i += 2) {
            PbVec2* newPt = srcPolygon->add_points();
            newPt->set_x(hull[i]);
            newPt->set_y(hull[i + 1]);
            anchorX += hull[i];
            anchorY += hull[i + 1];
        }
        anchorX /= (hull.size() >> 1);
        anchorY /= (hull.size() >> 1);
        auto anchor = srcPolygon->mutable_anchor();
        anchor->set_x(anchorX);
        anchor->set_y(anchorY);
        srcPolygon->set_is_box(true);
    }
    sliderTrapTestInitializerMapData->set_allocated_self_parsed_rdf(sliderTrapTestStartRdf);
    auto trapConfigFromTiled1 = sliderTrapTestInitializerMapData->add_trap_config_from_tile_list();
    Vec3 initVel(3.f * globalPrimitiveConsts->battle_dynamics_fps(), 4.f * globalPrimitiveConsts->battle_dynamics_fps(), 0);
    trapConfigFromTiled1->set_id(sliderTrapTestStartRdf->dynamic_traps(0).id());
    trapConfigFromTiled1->set_tpt(sliderTrapTestStartRdf->dynamic_traps(0).tpt());
    trapConfigFromTiled1->set_linear_speed(initVel.Length());
    trapConfigFromTiled1->set_box_half_size_x(50.f);
    trapConfigFromTiled1->set_box_half_size_y(50.f);
    trapConfigFromTiled1->set_init_q_x(0);
    trapConfigFromTiled1->set_init_q_y(0);
    trapConfigFromTiled1->set_init_q_z(0);
    trapConfigFromTiled1->set_init_q_w(1);

    trapConfigFromTiled1->set_init_vel_x(initVel.GetX());
    trapConfigFromTiled1->set_init_vel_y(initVel.GetY());
    trapConfigFromTiled1->set_init_vel_z(initVel.GetZ());

    trapConfigFromTiled1->set_slider_axis_x(1);
    trapConfigFromTiled1->set_slider_axis_y(0);
    trapConfigFromTiled1->set_slider_axis_z(0);

    trapConfigFromTiled1->set_init_x(0);
    trapConfigFromTiled1->set_init_y(750);
    trapConfigFromTiled1->set_init_z(0);

    trapConfigFromTiled1->set_cooldown_rdf_count(120);
    trapConfigFromTiled1->set_limit_1(-500.0f);
    trapConfigFromTiled1->set_limit_2(+500.0f);
}

void initTest10Data() {
    // incomingUpsyncSnapshotReqs10
    {
        int receivedEdIfdId = 3;
        int receivedStIfdId = 0;
        int receivedTimerRdfId = 8;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }

        incomingUpsyncSnapshotReqs10[receivedTimerRdfId] = req;
    }
    {
        int receivedEdIfdId = 60;
        int receivedStIfdId = 3;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId)+5; //19
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        
        incomingUpsyncSnapshotReqs10[receivedTimerRdfId] = req;
    }
    {
        int receivedEdIfdId = 67;
        int receivedStIfdId = 60;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId)+2; // 244, such that we're half-way in using an obsolete and incorrectly predicted input of peerJoinIndex==2
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(2);
        }
        
        incomingUpsyncSnapshotReqs10[receivedTimerRdfId] = req;
    }
    {
        int receivedEdIfdId = 76;
        int receivedStIfdId = 67;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId)-1; // 269
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(4);
        }
        
        incomingUpsyncSnapshotReqs10[receivedTimerRdfId] = req;
        // Kindly note that ifdId=75 will be first used by rdfId=302, last used by rdfId=305
    }
    {
        int receivedEdIfdId = 77;
        int receivedStIfdId = 76; // first used by rdfId=306, last used by rdfId=309
        int receivedTimerRdfId = BaseBattle::ConvertToLastUsedRenderFrameId(receivedStIfdId)+6; // 315
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(3); // [WARNING] Intentionally contradicting the earlier arrived "20" in "incomingDownsyncSnapshots10"
        }
        
        incomingUpsyncSnapshotReqs10[receivedTimerRdfId] = req;
    }

    // incomingDownsyncSnapshots10
    {
        int receivedEdIfdId = 3;
        int receivedStIfdId = 0;
        int receivedTimerRdfId = 10;
        DownsyncSnapshot* srvDownsyncSnapshot = google::protobuf::Arena::Create<DownsyncSnapshot>(&pbTestCaseDataAllocator);
        srvDownsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            InputFrameDownsync* ifdBatch = srvDownsyncSnapshot->add_ifd_batch();
            ifdBatch->add_input_list(getSelfCmdByIfdId(testCmds10, ifdId));
            ifdBatch->add_input_list(0);
        }
        incomingDownsyncSnapshots10[receivedTimerRdfId] = srvDownsyncSnapshot;
    }
    {
        int receivedEdIfdId = 60;
        int receivedStIfdId = 3;
        int receivedTimerRdfId = 18;
        DownsyncSnapshot* srvDownsyncSnapshot = google::protobuf::Arena::Create<DownsyncSnapshot>(&pbTestCaseDataAllocator);
        srvDownsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            InputFrameDownsync* ifdBatch = srvDownsyncSnapshot->add_ifd_batch();
            ifdBatch->add_input_list(getSelfCmdByIfdId(testCmds10, ifdId));
            ifdBatch->add_input_list(0);
        }
        incomingDownsyncSnapshots10[receivedTimerRdfId] = srvDownsyncSnapshot;
    }
    {
        int receivedEdIfdId = 67;
        int receivedStIfdId = 60;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId)+5; // 247
        DownsyncSnapshot* srvDownsyncSnapshot = google::protobuf::Arena::Create<DownsyncSnapshot>(&pbTestCaseDataAllocator);
        srvDownsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            InputFrameDownsync* ifdBatch = srvDownsyncSnapshot->add_ifd_batch();
            ifdBatch->add_input_list(getSelfCmdByIfdId(testCmds1, ifdId));
            ifdBatch->add_input_list(2);
        }
        incomingDownsyncSnapshots10[receivedTimerRdfId] = srvDownsyncSnapshot;
    }
    {
        int receivedEdIfdId = 76; 
        int receivedStIfdId = 67;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId)+5; // 275
        DownsyncSnapshot* srvDownsyncSnapshot = google::protobuf::Arena::Create<DownsyncSnapshot>(&pbTestCaseDataAllocator);
        srvDownsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            InputFrameDownsync* ifdBatch = srvDownsyncSnapshot->add_ifd_batch();
            ifdBatch->add_input_list(getSelfCmdByIfdId(testCmds1, ifdId));
            ifdBatch->add_input_list(4);
        }
        incomingDownsyncSnapshots10[receivedTimerRdfId] = srvDownsyncSnapshot;
        // Kindly note that ifdId=75 will be first used by rdfId=302, last used by rdfId=305
    }
    {
        int receivedEdIfdId = 77;
        int receivedStIfdId = 76; // first used by rdfId=306, last used by rdfId=309
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId)+2; // 308, such that we're half-way in using an obsolete and incorrectly predicted input of peerJoinIndex==2
        DownsyncSnapshot* srvDownsyncSnapshot = google::protobuf::Arena::Create<DownsyncSnapshot>(&pbTestCaseDataAllocator);
        srvDownsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            InputFrameDownsync* ifdBatch = srvDownsyncSnapshot->add_ifd_batch();
            ifdBatch->add_input_list(getSelfCmdByIfdId(testCmds1, ifdId));
            ifdBatch->add_input_list(20);
        }
        incomingDownsyncSnapshots10[receivedTimerRdfId] = srvDownsyncSnapshot;
    }
}

void initTest11Data(WsReq* rollbackChasingAlignTestInitializerMapData, std::vector<std::vector<float>>& hulls) {
    auto rollbackChasingAlignTestStartRdf = mockRollbackChasingAlignTestStartRdf();
    for (auto hull : hulls) {
        auto srcBarrier = rollbackChasingAlignTestInitializerMapData->add_serialized_barriers();
        auto srcPolygon = srcBarrier->mutable_polygon();
        float anchorX = 0, anchorY = 0;
        for (int i = 0; i < hull.size(); i += 2) {
            PbVec2* newPt = srcPolygon->add_points();
            newPt->set_x(hull[i]);
            newPt->set_y(hull[i + 1]);
            anchorX += hull[i];
            anchorY += hull[i + 1];
        }
        anchorX /= (hull.size() >> 1);
        anchorY /= (hull.size() >> 1);
        auto anchor = srcPolygon->mutable_anchor();
        anchor->set_x(anchorX);
        anchor->set_y(anchorY);
    }
    rollbackChasingAlignTestInitializerMapData->set_allocated_self_parsed_rdf(rollbackChasingAlignTestStartRdf);
    {
        int receivedEdIfdId = 2;
        int receivedStIfdId = 0;
        int receivedTimerRdfId = globalPrimitiveConsts->starting_render_frame_id()+2;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs11Intime[receivedTimerRdfId] = req;
    }
    {
        int receivedEdIfdId = 6;
        int receivedStIfdId = 2;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId)-1;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(4);
        }
        incomingUpsyncSnapshotReqs11Intime[receivedTimerRdfId] = req;
    }
    {
        int receivedEdIfdId = 17;
        int receivedStIfdId = 6;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId)-1;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs11Intime[receivedTimerRdfId] = req;
    }
    {
        int receivedEdIfdId = 33;
        int receivedStIfdId = 17;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId) - 1;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(19);
        }
        incomingUpsyncSnapshotReqs11Intime[receivedTimerRdfId] = req;
    }
    {
        int receivedEdIfdId = 48;
        int receivedStIfdId = 33;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId) - 1;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs11Intime[receivedTimerRdfId] = req;
    }
    {
        int receivedEdIfdId = 60;
        int receivedStIfdId = 48;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId) - 1;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(20);
        }
        incomingUpsyncSnapshotReqs11Intime[receivedTimerRdfId] = req;
    }
    {
        int receivedEdIfdId = 128;
        int receivedStIfdId = 60;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId) - 1;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs11Intime[receivedTimerRdfId] = req;
    }

    {
        int receivedTimerRdfId = 240;
        int receivedEdIfdId = 6;
        int receivedStIfdId = 2;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(4);
        }
        incomingUpsyncSnapshotReqs11Rollback[receivedTimerRdfId] = req;
    }
    {
        int receivedTimerRdfId = 280;
        int receivedEdIfdId = 2;
        int receivedStIfdId = 0;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs11Rollback[receivedTimerRdfId] = req;
    }
    {
        int receivedTimerRdfId = 320;
        int receivedEdIfdId = 17;
        int receivedStIfdId = 6;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs11Rollback[receivedTimerRdfId] = req;
    }
    {
        int receivedTimerRdfId = 360;
        int receivedEdIfdId = 128;
        int receivedStIfdId = 60;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs11Rollback[receivedTimerRdfId] = req;
    }
    {
        int receivedTimerRdfId = 400;
        int receivedEdIfdId = 33;
        int receivedStIfdId = 17;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(19);
        }
        incomingUpsyncSnapshotReqs11Rollback[receivedTimerRdfId] = req;
    }
    {
        int receivedTimerRdfId = 420;
        int receivedEdIfdId = 60;
        int receivedStIfdId = 48;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(20);
        }
        incomingUpsyncSnapshotReqs11Rollback[receivedTimerRdfId] = req;
    }
    {
        int receivedTimerRdfId = 480;
        int receivedEdIfdId = 48;
        int receivedStIfdId = 33;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs11Rollback[receivedTimerRdfId] = req;
    }
    {
        int receivedTimerRdfId = 640;
        int receivedEdIfdId = BaseBattle::ConvertToDelayedInputFrameId(1024-1)+1;
        int receivedStIfdId = 128;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs11Rollback[receivedTimerRdfId] = req;
    }
}

void initTest12Data(WsReq* fallenDeathInitializerMapData, std::vector<std::vector<float>>& hulls) {
    auto fallenDeathStartRdf = mockFallenDeathRdf();
    for (auto hull : hulls) {
        auto srcBarrier = fallenDeathInitializerMapData->add_serialized_barriers();
        auto srcPolygon = srcBarrier->mutable_polygon();
        float anchorX = 0, anchorY = 0;
        for (int i = 0; i < hull.size(); i += 2) {
            PbVec2* newPt = srcPolygon->add_points();
            newPt->set_x(hull[i]);
            newPt->set_y(hull[i + 1]);
            anchorX += hull[i];
            anchorY += hull[i + 1];
        }
        anchorX /= (hull.size() >> 1);
        anchorY /= (hull.size() >> 1);
        auto anchor = srcPolygon->mutable_anchor();
        anchor->set_x(anchorX);
        anchor->set_y(anchorY);
    }
    fallenDeathInitializerMapData->set_allocated_self_parsed_rdf(fallenDeathStartRdf);
    {
        int receivedEdIfdId = 2;
        int receivedStIfdId = 0;
        int receivedTimerRdfId = globalPrimitiveConsts->starting_render_frame_id() + 2;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs12Intime[receivedTimerRdfId] = req;
    }
    {
        int receivedEdIfdId = 6;
        int receivedStIfdId = 2;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId) - 1;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(4);
        }
        incomingUpsyncSnapshotReqs12Intime[receivedTimerRdfId] = req;
    }
    {
        int receivedEdIfdId = 17;
        int receivedStIfdId = 6;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId) - 1;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs12Intime[receivedTimerRdfId] = req;
    }
    {
        int receivedEdIfdId = 33;
        int receivedStIfdId = 17;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId) - 1;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(19);
        }
        incomingUpsyncSnapshotReqs12Intime[receivedTimerRdfId] = req;
    }
    {
        int receivedEdIfdId = 48;
        int receivedStIfdId = 33;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId) - 1;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs12Intime[receivedTimerRdfId] = req;
    }
    {
        int receivedEdIfdId = 60;
        int receivedStIfdId = 48;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId) - 1;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(20);
        }
        incomingUpsyncSnapshotReqs12Intime[receivedTimerRdfId] = req;
    }
    {
        int receivedEdIfdId = 128;
        int receivedStIfdId = 60;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId) - 1;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs12Intime[receivedTimerRdfId] = req;
    }
}

void initTest13Data(WsReq* bladeGirlSkillInitializerMapData, std::vector<std::vector<float>>& hulls) {
    auto bladeGirlSkillStartRdf = mockBladeGirlSkillRdf();
    for (auto hull : hulls) {
        auto srcBarrier = bladeGirlSkillInitializerMapData->add_serialized_barriers();
        auto srcPolygon = srcBarrier->mutable_polygon();
        float anchorX = 0, anchorY = 0;
        for (int i = 0; i < hull.size(); i += 2) {
            PbVec2* newPt = srcPolygon->add_points();
            newPt->set_x(hull[i]);
            newPt->set_y(hull[i + 1]);
            anchorX += hull[i];
            anchorY += hull[i + 1];
        }
        anchorX /= (hull.size() >> 1);
        anchorY /= (hull.size() >> 1);
        auto anchor = srcPolygon->mutable_anchor();
        anchor->set_x(anchorX);
        anchor->set_y(anchorY);
    }
    bladeGirlSkillInitializerMapData->set_allocated_self_parsed_rdf(bladeGirlSkillStartRdf);
}

void initTest14Data(WsReq* bountyHunterSkillInitializerMapData, std::vector<std::vector<float>>& hulls) {
    auto bountyHunterSkillStartRdf = mockBountyHunterSkillRdf();
    for (auto hull : hulls) {
        auto srcBarrier = bountyHunterSkillInitializerMapData->add_serialized_barriers();
        auto srcPolygon = srcBarrier->mutable_polygon();
        float anchorX = 0, anchorY = 0;
        for (int i = 0; i < hull.size(); i += 2) {
            PbVec2* newPt = srcPolygon->add_points();
            newPt->set_x(hull[i]);
            newPt->set_y(hull[i + 1]);
            anchorX += hull[i];
            anchorY += hull[i + 1];
        }
        anchorX /= (hull.size() >> 1);
        anchorY /= (hull.size() >> 1);
        auto anchor = srcPolygon->mutable_anchor();
        anchor->set_x(anchorX);
        anchor->set_y(anchorY);
    }
    bountyHunterSkillInitializerMapData->set_allocated_self_parsed_rdf(bountyHunterSkillStartRdf);
    {
        int receivedEdIfdId = 2;
        int receivedStIfdId = 0;
        int receivedTimerRdfId = globalPrimitiveConsts->starting_render_frame_id() + 2;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs14Intime[receivedTimerRdfId] = req;
    }
    {
        int receivedEdIfdId = 10;
        int receivedStIfdId = 2;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId) - 1;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(4);
        }
        incomingUpsyncSnapshotReqs14Intime[receivedTimerRdfId] = req;
    }
    {
        int receivedEdIfdId = 16;
        int receivedStIfdId = 10;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId) - 1;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(3);
        }
        incomingUpsyncSnapshotReqs14Intime[receivedTimerRdfId] = req;
    }
    {
        int receivedEdIfdId = 17;
        int receivedStIfdId = 16;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId) - 1;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs14Intime[receivedTimerRdfId] = req;
    }
    {
        int receivedEdIfdId = 60;
        int receivedStIfdId = 57;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId) - 1;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(4);
        }
        incomingUpsyncSnapshotReqs14Intime[receivedTimerRdfId] = req;
    }
    {
        int receivedEdIfdId = 66;
        int receivedStIfdId = 60;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId) - 1;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs14Intime[receivedTimerRdfId] = req;
    }
}

void initTest15Data(WsReq* victoryTriggerInitializerMapData, std::vector<std::vector<float>>& hulls) {
    auto victoryTriggerStartRdf = mockVictoryRdf();
    for (auto hull : hulls) {
        auto srcBarrier = victoryTriggerInitializerMapData->add_serialized_barriers();
        auto srcPolygon = srcBarrier->mutable_polygon();
        float anchorX = 0, anchorY = 0;
        for (int i = 0; i < hull.size(); i += 2) {
            PbVec2* newPt = srcPolygon->add_points();
            newPt->set_x(hull[i]);
            newPt->set_y(hull[i + 1]);
            anchorX += hull[i];
            anchorY += hull[i + 1];
        }
        anchorX /= (hull.size() >> 1);
        anchorY /= (hull.size() >> 1);
        auto anchor = srcPolygon->mutable_anchor();
        anchor->set_x(anchorX);
        anchor->set_y(anchorY);
    }
    victoryTriggerInitializerMapData->set_allocated_self_parsed_rdf(victoryTriggerStartRdf);
}

void initTest16Data(WsReq* sliderTrapTestInitializerMapData, std::vector<std::vector<float>>& hulls) {
    auto sliderTrapTestStartRdf = mockSliderTrapTestStartRdf2();
    for (auto hull : hulls) {
        auto srcBarrier = sliderTrapTestInitializerMapData->add_serialized_barriers();
        auto srcPolygon = srcBarrier->mutable_polygon();
        float anchorX = 0, anchorY = 0;
        for (int i = 0; i < hull.size(); i += 2) {
            PbVec2* newPt = srcPolygon->add_points();
            newPt->set_x(hull[i]);
            newPt->set_y(hull[i + 1]);
            anchorX += hull[i];
            anchorY += hull[i + 1];
        }
        anchorX /= (hull.size() >> 1);
        anchorY /= (hull.size() >> 1);
        auto anchor = srcPolygon->mutable_anchor();
        anchor->set_x(anchorX);
        anchor->set_y(anchorY);
        srcPolygon->set_is_box(true);
    }
    sliderTrapTestInitializerMapData->set_allocated_self_parsed_rdf(sliderTrapTestStartRdf);
    auto trapConfigFromTiled2 = sliderTrapTestInitializerMapData->add_trap_config_from_tile_list();
    Vec3 initVel(-0.4f * globalPrimitiveConsts->battle_dynamics_fps(), -0.3f * globalPrimitiveConsts->battle_dynamics_fps(), 0);
    trapConfigFromTiled2->set_id(sliderTrapTestStartRdf->dynamic_traps(0).id());
    trapConfigFromTiled2->set_tpt(sliderTrapTestStartRdf->dynamic_traps(0).tpt());
    trapConfigFromTiled2->set_linear_speed(initVel.Length());
    trapConfigFromTiled2->set_box_half_size_x(100.f);
    trapConfigFromTiled2->set_box_half_size_y(20.f);
    trapConfigFromTiled2->set_init_q_x(0);
    trapConfigFromTiled2->set_init_q_y(0);
    trapConfigFromTiled2->set_init_q_z(0);
    trapConfigFromTiled2->set_init_q_w(1);

    trapConfigFromTiled2->set_init_vel_x(initVel.GetX());
    trapConfigFromTiled2->set_init_vel_y(initVel.GetY());
    trapConfigFromTiled2->set_init_vel_z(initVel.GetZ());

    trapConfigFromTiled2->set_slider_axis_x(4);
    trapConfigFromTiled2->set_slider_axis_y(3);
    trapConfigFromTiled2->set_slider_axis_z(0);

    trapConfigFromTiled2->set_init_x(+0);
    trapConfigFromTiled2->set_init_y(2000);
    trapConfigFromTiled2->set_init_z(0);

    trapConfigFromTiled2->set_cooldown_rdf_count(10);
    trapConfigFromTiled2->set_limit_1(-100.f);
    trapConfigFromTiled2->set_limit_2(+100.0f);
}

void initTest17Data(WsReq* sliderTrapTestInitializerMapData, std::vector<std::vector<float>>& hulls) {
    auto sliderTrapTestStartRdf = mockSliderTrapTestStartRdf3();
    for (auto hull : hulls) {
        auto srcBarrier = sliderTrapTestInitializerMapData->add_serialized_barriers();
        auto srcPolygon = srcBarrier->mutable_polygon();
        float anchorX = 0, anchorY = 0;
        for (int i = 0; i < hull.size(); i += 2) {
            PbVec2* newPt = srcPolygon->add_points();
            newPt->set_x(hull[i]);
            newPt->set_y(hull[i + 1]);
            anchorX += hull[i];
            anchorY += hull[i + 1];
        }
        anchorX /= (hull.size() >> 1);
        anchorY /= (hull.size() >> 1);
        auto anchor = srcPolygon->mutable_anchor();
        anchor->set_x(anchorX);
        anchor->set_y(anchorY);
        srcPolygon->set_is_box(true);
    }

    sliderTrapTestInitializerMapData->set_allocated_self_parsed_rdf(sliderTrapTestStartRdf);
    auto trapConfigFromTiled1 = sliderTrapTestInitializerMapData->add_trap_config_from_tile_list();
    Vec3 initVel(-2.f * globalPrimitiveConsts->battle_dynamics_fps(), 0.f * globalPrimitiveConsts->battle_dynamics_fps(), 0);
    trapConfigFromTiled1->set_id(sliderTrapTestStartRdf->dynamic_traps(0).id());
    trapConfigFromTiled1->set_tpt(sliderTrapTestStartRdf->dynamic_traps(0).tpt());
    trapConfigFromTiled1->set_linear_speed(initVel.Length());
    trapConfigFromTiled1->set_box_half_size_x(50.f);
    trapConfigFromTiled1->set_box_half_size_y(20.f);
    trapConfigFromTiled1->set_init_q_x(0);
    trapConfigFromTiled1->set_init_q_y(0);
    trapConfigFromTiled1->set_init_q_z(0);
    trapConfigFromTiled1->set_init_q_w(1);

    trapConfigFromTiled1->set_init_vel_x(initVel.GetX());
    trapConfigFromTiled1->set_init_vel_y(initVel.GetY());
    trapConfigFromTiled1->set_init_vel_z(initVel.GetZ());

    trapConfigFromTiled1->set_slider_axis_x(1);
    trapConfigFromTiled1->set_slider_axis_y(0);
    trapConfigFromTiled1->set_slider_axis_z(0);

    trapConfigFromTiled1->set_init_x(0);
    trapConfigFromTiled1->set_init_y(105);
    trapConfigFromTiled1->set_init_z(0);

    trapConfigFromTiled1->set_cooldown_rdf_count(120);
    trapConfigFromTiled1->set_limit_1(-300.0f);
    trapConfigFromTiled1->set_limit_2(+300.0f);
}

void initTest18Data(WsReq* rollbackChasingAlignTestInitializerMapData, std::vector<std::vector<float>>& hulls) {
    auto rollbackChasingAlignTestStartRdf = mockRollbackChasingAlignTestStartRdf2();

    auto trapConfigFromTiled1 = rollbackChasingAlignTestInitializerMapData->add_trap_config_from_tile_list();
    Vec3 initVel1(-2.f * globalPrimitiveConsts->battle_dynamics_fps(), 0.f * globalPrimitiveConsts->battle_dynamics_fps(), 0);
    trapConfigFromTiled1->set_id(rollbackChasingAlignTestStartRdf->dynamic_traps(0).id());
    trapConfigFromTiled1->set_tpt(rollbackChasingAlignTestStartRdf->dynamic_traps(0).tpt());
    trapConfigFromTiled1->set_linear_speed(initVel1.Length());
    trapConfigFromTiled1->set_box_half_size_x(50.f);
    trapConfigFromTiled1->set_box_half_size_y(20.f);
    trapConfigFromTiled1->set_init_q_x(0);
    trapConfigFromTiled1->set_init_q_y(0);
    trapConfigFromTiled1->set_init_q_z(0);
    trapConfigFromTiled1->set_init_q_w(1);

    trapConfigFromTiled1->set_init_vel_x(initVel1.GetX());
    trapConfigFromTiled1->set_init_vel_y(initVel1.GetY());
    trapConfigFromTiled1->set_init_vel_z(initVel1.GetZ());

    trapConfigFromTiled1->set_slider_axis_x(1);
    trapConfigFromTiled1->set_slider_axis_y(0);
    trapConfigFromTiled1->set_slider_axis_z(0);

    trapConfigFromTiled1->set_init_x(-40);
    trapConfigFromTiled1->set_init_y(105);
    trapConfigFromTiled1->set_init_z(0);

    trapConfigFromTiled1->set_cooldown_rdf_count(120);
    trapConfigFromTiled1->set_limit_1(-20.0f);
    trapConfigFromTiled1->set_limit_2(+20.0f);

    auto trapConfigFromTiled2 = rollbackChasingAlignTestInitializerMapData->add_trap_config_from_tile_list();
    Vec3 initVel2(+2.f * globalPrimitiveConsts->battle_dynamics_fps(), 0.f * globalPrimitiveConsts->battle_dynamics_fps(), 0);
    trapConfigFromTiled2->set_id(rollbackChasingAlignTestStartRdf->dynamic_traps(1).id());
    trapConfigFromTiled2->set_tpt(rollbackChasingAlignTestStartRdf->dynamic_traps(1).tpt());
    trapConfigFromTiled2->set_linear_speed(initVel2.Length());
    trapConfigFromTiled2->set_box_half_size_x(75.f);
    trapConfigFromTiled2->set_box_half_size_y(20.f);
    trapConfigFromTiled2->set_init_q_x(0);
    trapConfigFromTiled2->set_init_q_y(0);
    trapConfigFromTiled2->set_init_q_z(0);
    trapConfigFromTiled2->set_init_q_w(1);

    trapConfigFromTiled2->set_init_vel_x(initVel2.GetX());
    trapConfigFromTiled2->set_init_vel_y(initVel2.GetY());
    trapConfigFromTiled2->set_init_vel_z(initVel2.GetZ());

    trapConfigFromTiled2->set_slider_axis_x(1);
    trapConfigFromTiled2->set_slider_axis_y(0);
    trapConfigFromTiled2->set_slider_axis_z(0);

    trapConfigFromTiled2->set_init_x(+40);
    trapConfigFromTiled2->set_init_y(105);
    trapConfigFromTiled2->set_init_z(0);

    trapConfigFromTiled2->set_cooldown_rdf_count(120);
    trapConfigFromTiled2->set_limit_1(-20.0f);
    trapConfigFromTiled2->set_limit_2(+20.0f);

    for (auto hull : hulls) {
        auto srcBarrier = rollbackChasingAlignTestInitializerMapData->add_serialized_barriers();
        auto srcPolygon = srcBarrier->mutable_polygon();
        float anchorX = 0, anchorY = 0;
        for (int i = 0; i < hull.size(); i += 2) {
            PbVec2* newPt = srcPolygon->add_points();
            newPt->set_x(hull[i]);
            newPt->set_y(hull[i + 1]);
            anchorX += hull[i];
            anchorY += hull[i + 1];
        }
        anchorX /= (hull.size() >> 1);
        anchorY /= (hull.size() >> 1);
        auto anchor = srcPolygon->mutable_anchor();
        anchor->set_x(anchorX);
        anchor->set_y(anchorY);
    }
    rollbackChasingAlignTestInitializerMapData->set_allocated_self_parsed_rdf(rollbackChasingAlignTestStartRdf);
    {
        int receivedEdIfdId = 2;
        int receivedStIfdId = 0;
        int receivedTimerRdfId = globalPrimitiveConsts->starting_render_frame_id()+2;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs18Intime[receivedTimerRdfId] = req;
    }
    {
        int receivedEdIfdId = 6;
        int receivedStIfdId = 2;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId)-1;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(4);
        }
        incomingUpsyncSnapshotReqs18Intime[receivedTimerRdfId] = req;
    }
    {
        int receivedEdIfdId = 17;
        int receivedStIfdId = 6;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId)-1;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs18Intime[receivedTimerRdfId] = req;
    }
    {
        int receivedEdIfdId = 33;
        int receivedStIfdId = 17;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId) - 1;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(19);
        }
        incomingUpsyncSnapshotReqs18Intime[receivedTimerRdfId] = req;
    }
    {
        int receivedEdIfdId = 48;
        int receivedStIfdId = 33;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId) - 1;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs18Intime[receivedTimerRdfId] = req;
    }
    {
        int receivedEdIfdId = 60;
        int receivedStIfdId = 48;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId) - 1;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(20);
        }
        incomingUpsyncSnapshotReqs18Intime[receivedTimerRdfId] = req;
    }
    {
        int receivedEdIfdId = 128;
        int receivedStIfdId = 60;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId) - 1;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs18Intime[receivedTimerRdfId] = req;
    }

    {
        int receivedTimerRdfId = 240;
        int receivedEdIfdId = 6;
        int receivedStIfdId = 2;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(4);
        }
        incomingUpsyncSnapshotReqs18Rollback[receivedTimerRdfId] = req;
    }
    {
        int receivedTimerRdfId = 280;
        int receivedEdIfdId = 2;
        int receivedStIfdId = 0;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs18Rollback[receivedTimerRdfId] = req;
    }
    {
        int receivedTimerRdfId = 320;
        int receivedEdIfdId = 17;
        int receivedStIfdId = 6;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs18Rollback[receivedTimerRdfId] = req;
    }
    {
        int receivedTimerRdfId = 360;
        int receivedEdIfdId = 128;
        int receivedStIfdId = 60;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs18Rollback[receivedTimerRdfId] = req;
    }
    {
        int receivedTimerRdfId = 400;
        int receivedEdIfdId = 33;
        int receivedStIfdId = 17;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(19);
        }
        incomingUpsyncSnapshotReqs18Rollback[receivedTimerRdfId] = req;
    }
    {
        int receivedTimerRdfId = 420;
        int receivedEdIfdId = 60;
        int receivedStIfdId = 48;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(20);
        }
        incomingUpsyncSnapshotReqs18Rollback[receivedTimerRdfId] = req;
    }
    {
        int receivedTimerRdfId = 480;
        int receivedEdIfdId = 48;
        int receivedStIfdId = 33;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs18Rollback[receivedTimerRdfId] = req;
    }
    {
        int receivedTimerRdfId = 640;
        int receivedEdIfdId = BaseBattle::ConvertToDelayedInputFrameId(1024-1)+1;
        int receivedStIfdId = 128;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs18Rollback[receivedTimerRdfId] = req;
    }
}

std::string outStr;
std::string player1OutStr, player2OutStr;
std::string referencePlayer1OutStr, referencePlayer2OutStr;
bool runTestCase1(FrontendBattle* reusedBattle, WsReq* initializerMapData, int inSingleJoinIndex) {
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);
    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 1024;
    int printIntervalRdfCnt = (1 << 4);
    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    jtshared::RenderFrame* outRdf = google::protobuf::Arena::Create<RenderFrame>(&pbTestCaseDataAllocator);
    int newLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    int newChaserRdfId = 0, newReferenceBattleChaserRdfId = 0;
    while (loopRdfCnt > outerTimerRdfId) {
        bool shouldPrint = false;
        // Handling TCP packets first, and then UDP packets, the same as C# side behavior.
        if (incomingDownsyncSnapshots1.count(outerTimerRdfId)) {
            DownsyncSnapshot* srvDownsyncSnapshot = incomingDownsyncSnapshots1[outerTimerRdfId];
            int outPostTimerRdfEvictedCnt = 0, outPostTimerRdfDelayedIfdEvictedCnt = 0;
            bool applied = reusedBattle->OnDownsyncSnapshotReceived(srvDownsyncSnapshot, &outPostTimerRdfEvictedCnt, &outPostTimerRdfDelayedIfdEvictedCnt, &newChaserRdfId, &newLcacIfdId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
            if (0 == srvDownsyncSnapshot->st_ifd_id()) {
                JPH_ASSERT(25 == newLcacIfdId);
                JPH_ASSERT(25 == minPlayerInputFrontId);
            } else if (26 == srvDownsyncSnapshot->st_ifd_id()) {
                JPH_ASSERT(57 == newLcacIfdId);
            }
        }
        if (incomingUpsyncSnapshotReqs1.count(outerTimerRdfId)) {
            auto req = incomingUpsyncSnapshotReqs1[outerTimerRdfId]; 
            auto peerUpsyncSnapshot = req->upsync_snapshot();
            auto peerJoinIndex = req->join_index();
            bool applied = reusedBattle->OnUpsyncSnapshotReceived(peerJoinIndex, peerUpsyncSnapshot, &newChaserRdfId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
            if (13 == peerUpsyncSnapshot.st_ifd_id()) {
                JPH_ASSERT(BaseBattle::ConvertToFirstUsedRenderFrameId(13) == newChaserRdfId);
                JPH_ASSERT(-1 == reusedBattle->lcacIfdId);
            } else if (0 == peerUpsyncSnapshot.st_ifd_id()) {
                JPH_ASSERT(2 == newChaserRdfId);
                JPH_ASSERT(-1 == reusedBattle->lcacIfdId);
            } else if (29 == peerUpsyncSnapshot.st_ifd_id()) {
                JPH_ASSERT(178 == newChaserRdfId);
                JPH_ASSERT(25 == reusedBattle->lcacIfdId);
            } else if (26 == peerUpsyncSnapshot.st_ifd_id()) {
                JPH_ASSERT(106 == newChaserRdfId);
                JPH_ASSERT(25 == reusedBattle->lcacIfdId);
            }
        }
        uint64_t inSingleInput = getSelfCmdByRdfId(testCmds1, outerTimerRdfId);
        bool cmdInjected = FRONTEND_UpsertSelfCmd(reusedBattle, inSingleInput, &newChaserRdfId);
        if (!cmdInjected) {
            std::cerr << "Failed to inject cmd for outerTimerRdfId=" << outerTimerRdfId << ", inSingleInput=" << inSingleInput << std::endl;
            exit(1);
        }
        int chaserRdfIdEd = newChaserRdfId + globalPrimitiveConsts->max_chasing_render_frames_per_update();
        if (chaserRdfIdEd > outerTimerRdfId) {
            chaserRdfIdEd = outerTimerRdfId;
        }
        FRONTEND_ChaseRolledBackRdfs(reusedBattle, &newChaserRdfId, true);
        FRONTEND_Step(reusedBattle);
        auto outerTimerRdf = reusedBattle->rdfBuffer.GetByFrameId(outerTimerRdfId);
        auto& p1 = outerTimerRdf->players(0);
        auto& p1Chd = p1.chd();
        const uint64_t ud1 = BaseBattleCollisionFilter::calcPlayerUserData(p1.join_index());
        auto& p2 = outerTimerRdf->players(1);
        auto& p2Chd = p2.chd();
        const uint64_t ud2 = BaseBattleCollisionFilter::calcPlayerUserData(p2.join_index());

        
        if (770 <= outerTimerRdfId && outerTimerRdfId < 1000) {
            shouldPrint = true;
        }
        
        if (shouldPrint) {
            std::cout << "TestCase1/outerTimerRdfId=" << outerTimerRdfId << "\n\tp1Chd ud1=" << ud1 << ", cs=" << p1Chd.ch_state() << ", fc=" << p1Chd.frames_in_ch_state() << ", dir=(" << p1Chd.q_x() << ", " << p1Chd.q_y() << ", " << p1Chd.q_z() << ", " << p1Chd.q_w() << "), pos=(" << p1Chd.x() << ", " << p1Chd.y() << "), vel=(" << p1Chd.vel_x() << ", " << p1Chd.vel_y() << "), gud=" << p1Chd.ground_ud() << "\n\tp2Chd ud2=" << ud2 << ", cs=" << p2Chd.ch_state() << ", fc=" << p2Chd.frames_in_ch_state() << ", dir=(" << p2Chd.q_x() << ", " << p2Chd.q_y() << ", " << p2Chd.q_z() << ", " << p2Chd.q_w() << "), pos=(" << p2Chd.x() << ", " << p2Chd.y() << "), vel=(" << p2Chd.vel_x() << ", " << p2Chd.vel_y() << ")" << std::endl;
        }

        if (20 == outerTimerRdfId) {
            JPH_ASSERT(CharacterState::InAirIdle1NoJump == p1Chd.ch_state());
        } else if (21 == outerTimerRdfId) {
            JPH_ASSERT(CharacterState::Idle1 == p1Chd.ch_state());
        } else if (62 == outerTimerRdfId) {
            JPH_ASSERT(CharacterState::Idle1 == p1Chd.ch_state());
        } else if (63 <= outerTimerRdfId && outerTimerRdfId <= 80) {
            int p1ExpectedFramesInChState = outerTimerRdfId - 63;
            const Skill* skill = nullptr;
            const BulletConfig* bulletConfig = nullptr;
            uint32_t p1ChdActiveSkillId = p1Chd.active_skill_id();
            int p1ChdActiveSkillHit = p1Chd.active_skill_hit();
            BaseBattle::FindBulletConfig(p1ChdActiveSkillId, p1ChdActiveSkillHit, skill, bulletConfig);
            JPH_ASSERT(nullptr != skill && nullptr != bulletConfig);
            int p1ExpectedFramesToRecover = skill->recovery_frames() - (p1ExpectedFramesInChState);
            JPH_ASSERT(CharacterState::Atk1 == p1Chd.ch_state());
            JPH_ASSERT(p1ExpectedFramesInChState == p1Chd.frames_in_ch_state());
            JPH_ASSERT(p1ExpectedFramesToRecover == p1Chd.frames_to_recover());
        } else if (81 == outerTimerRdfId) {
            JPH_ASSERT(CharacterState::Idle1 == p1Chd.ch_state());
        } else if (231 <= outerTimerRdfId && outerTimerRdfId <= 250) {
            JPH_ASSERT(CharacterState::InAirIdle1ByJump == p1Chd.ch_state());
        } else if (703 == outerTimerRdfId) {
            JPH_ASSERT(CharacterState::Sliding == p1Chd.ch_state());
        } else if (707 == outerTimerRdfId) {
            JPH_ASSERT(CharacterState::Sliding == p1Chd.ch_state());
            JPH_ASSERT(0 < p1Chd.vel_x());
        } else if (770 == outerTimerRdfId) {
            JPH_ASSERT(CharacterState::OnWallIdle1 == p1Chd.ch_state());
        } else if (775 == outerTimerRdfId) {
            JPH_ASSERT(CharacterState::InAirIdle1ByWallJump == p1Chd.ch_state());
            JPH_ASSERT(0 < p1Chd.vel_y());
        } else if (795 == outerTimerRdfId) {
            JPH_ASSERT(CharacterState::OnWallIdle1 == p1Chd.ch_state());
        } else if (810 == outerTimerRdfId) {
            JPH_ASSERT(CharacterState::InAirIdle1ByWallJump == p1Chd.ch_state());
            JPH_ASSERT(0 > p1Chd.vel_x());
        } else if (848 == outerTimerRdfId) {
            JPH_ASSERT(CharacterState::InAirIdle2ByJump == p1Chd.ch_state());
            JPH_ASSERT(0 > p1Chd.vel_x());
        }
        
        outerTimerRdfId++;
    }

    std::cout << "Passed TestCase1\n" << std::endl;
    reusedBattle->Clear();   
    return true;
}

bool runTestCase2(FrontendBattle* reusedBattle, WsReq* initializerMapData, int inSingleJoinIndex) {
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);
    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 1024;
    int printIntervalRdfCnt = (1 << 30);
    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    int newLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    int newChaserRdfId = 0, newReferenceBattleChaserRdfId = 0;
    jtshared::RenderFrame* outRdf = google::protobuf::Arena::Create<RenderFrame>(&pbTestCaseDataAllocator);
    while (loopRdfCnt > outerTimerRdfId) {
        bool shouldPrint = false;
        // Handling TCP packets first, and then UDP packets, the same as C# side behavior.
        if (incomingDownsyncSnapshots2.count(outerTimerRdfId)) {
            DownsyncSnapshot* srvDownsyncSnapshot = incomingDownsyncSnapshots2[outerTimerRdfId];
            int outPostTimerRdfEvictedCnt = 0, outPostTimerRdfDelayedIfdEvictedCnt = 0;
            bool applied = reusedBattle->OnDownsyncSnapshotReceived(srvDownsyncSnapshot, &outPostTimerRdfEvictedCnt, &outPostTimerRdfDelayedIfdEvictedCnt, &newChaserRdfId, &newLcacIfdId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
            if (0 == srvDownsyncSnapshot->st_ifd_id()) {
                JPH_ASSERT(25 == newLcacIfdId);
            } else if (1100 == srvDownsyncSnapshot->ref_rdf_id()) {
                JPH_ASSERT(271 == newLcacIfdId);
                JPH_ASSERT(18 == outPostTimerRdfEvictedCnt);
                JPH_ASSERT(0 == outPostTimerRdfDelayedIfdEvictedCnt);
                JPH_ASSERT(srvDownsyncSnapshot->ref_rdf_id() == reusedBattle->timerRdfId);
                JPH_ASSERT(srvDownsyncSnapshot->ref_rdf_id() == newChaserRdfId);
                JPH_ASSERT(srvDownsyncSnapshot->ref_rdf_id() == reusedBattle->chaserRdfIdLowerBound);
                outerTimerRdfId = reusedBattle->timerRdfId;
            }
            if (srvDownsyncSnapshot->has_ref_rdf()) {
                auto rdf = srvDownsyncSnapshot->release_ref_rdf();
                delete rdf;
            }
        }
        if (incomingUpsyncSnapshotReqs2.count(outerTimerRdfId)) {
            auto req = incomingUpsyncSnapshotReqs2[outerTimerRdfId];
            auto peerUpsyncSnapshot = req->upsync_snapshot();
            bool applied = reusedBattle->OnUpsyncSnapshotReceived(req->join_index(), peerUpsyncSnapshot, &newChaserRdfId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
            if (13 == peerUpsyncSnapshot.st_ifd_id()) {
                JPH_ASSERT(BaseBattle::ConvertToFirstUsedRenderFrameId(13) == newChaserRdfId);
                JPH_ASSERT(-1 == reusedBattle->lcacIfdId);
            } else if (0 == peerUpsyncSnapshot.st_ifd_id()) {
                JPH_ASSERT(BaseBattle::ConvertToFirstUsedRenderFrameId(0) == newChaserRdfId);
                JPH_ASSERT(-1 == reusedBattle->lcacIfdId);
            } else if (29 == peerUpsyncSnapshot.st_ifd_id()) {
                JPH_ASSERT(178 == newChaserRdfId);
                JPH_ASSERT(25 == reusedBattle->lcacIfdId);
            } else if (26 == peerUpsyncSnapshot.st_ifd_id()) {
                JPH_ASSERT(106 == newChaserRdfId);
                JPH_ASSERT(25 == reusedBattle->lcacIfdId);
            }
        }
        int oldIfdEdFrameId = reusedBattle->ifdBuffer.EdFrameId;
        uint64_t inSingleInput = getSelfCmdByRdfId(testCmds2, outerTimerRdfId);
        bool cmdInjected = FRONTEND_UpsertSelfCmd(reusedBattle, inSingleInput, &newChaserRdfId);
        if (!cmdInjected) {
            std::cerr << "Failed to inject cmd for outerTimerRdfId=" << outerTimerRdfId << ", inSingleInput=" << inSingleInput << std::endl;
            exit(1);
        }
        if (1100 == outerTimerRdfId) {
            // i.e. locally prefabbed to-be-used "delayedIfdId" after resynced (by pumping up "timerRdfId" without downsynced matching cmds to use)
            JPH_ASSERT(outerTimerRdfId == reusedBattle->timerRdfId);
            int newIfdEdFrameId = reusedBattle->ifdBuffer.EdFrameId;
            JPH_ASSERT(oldIfdEdFrameId+4 == newIfdEdFrameId);
        }
        int chaserRdfIdEd = newChaserRdfId + globalPrimitiveConsts->max_chasing_render_frames_per_update();
        if (chaserRdfIdEd > outerTimerRdfId) {
            chaserRdfIdEd = outerTimerRdfId;
        }
        FRONTEND_ChaseRolledBackRdfs(reusedBattle, &newChaserRdfId, true);
        FRONTEND_Step(reusedBattle);
        auto outerTimerRdf = reusedBattle->rdfBuffer.GetByFrameId(outerTimerRdfId);
        auto& p1 = outerTimerRdf->players(0);
        auto& p1Chd = p1.chd();
        auto ud1 = BaseBattleCollisionFilter::calcPlayerUserData(p1.join_index());
        auto& p2 = outerTimerRdf->players(1);
        auto& p2Chd = p2.chd();
        auto ud2 = BaseBattleCollisionFilter::calcPlayerUserData(p2.join_index());

        if (700 <= outerTimerRdfId) {
            shouldPrint = true;
        }
        
        if (shouldPrint) {
            std::cout << "TestCase2/outerTimerRdfId=" << outerTimerRdfId << "\n\tp1Chd ud1=" << ud1 << ", cs=" << p1Chd.ch_state() << ", fc=" << p1Chd.frames_in_ch_state() << ", dir=(" << p1Chd.q_x() << ", " << p1Chd.q_y() << ", " << p1Chd.q_z() << ", " << p1Chd.q_w() << "), pos=(" << p1Chd.x() << ", " << p1Chd.y() << "), vel=(" << p1Chd.vel_x() << ", " << p1Chd.vel_y() << "), gud=" << p1Chd.ground_ud() << "\n\tp2Chd ud2=" << ud2 << ", cs=" << p2Chd.ch_state() << ", fc=" << p2Chd.frames_in_ch_state() << ", dir=(" << p2Chd.q_x() << ", " << p2Chd.q_y() << ", " << p2Chd.q_z() << ", " << p2Chd.q_w() << "), pos=(" << p2Chd.x() << ", " << p2Chd.y() << "), vel=(" << p2Chd.vel_x() << ", " << p2Chd.vel_y() << ")" << std::endl;
        }

        if (20 == outerTimerRdfId) {
            JPH_ASSERT(CharacterState::InAirIdle1NoJump == p1Chd.ch_state());
        } else if (21 == outerTimerRdfId) {
            JPH_ASSERT(CharacterState::Walking == p1Chd.ch_state());
        } else if (62 == outerTimerRdfId) {
            JPH_ASSERT(CharacterState::Walking == p1Chd.ch_state());
        } else if (63 <= outerTimerRdfId && outerTimerRdfId <= 80) {
            JPH_ASSERT(CharacterState::WalkingAtk1 == p1Chd.ch_state());
            int p1ExpectedFramesInChState = outerTimerRdfId - 63;
            const Skill* skill = nullptr;
            const BulletConfig* bulletConfig = nullptr;
            uint32_t p1ChdActiveSkillId = p1Chd.active_skill_id();
            int p1ChdActiveSkillHit = p1Chd.active_skill_hit();
            BaseBattle::FindBulletConfig(p1ChdActiveSkillId, p1ChdActiveSkillHit, skill, bulletConfig);
            JPH_ASSERT(nullptr != skill && nullptr != bulletConfig);
        } else if (81 == outerTimerRdfId) {
            JPH_ASSERT(CharacterState::Idle1 == p1Chd.ch_state());
        } else if (326 == outerTimerRdfId && outerTimerRdfId <= 343) {
            JPH_ASSERT(CharacterState::WalkingAtk1 == p1Chd.ch_state());
        }
        
        outerTimerRdfId++;
    }

    std::cout << "Passed TestCase2\n" << std::endl;
    reusedBattle->Clear();   
    return true;
}

bool runTestCase3(FrontendBattle* reusedBattle, WsReq* initializerMapData, int inSingleJoinIndex) {
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);
    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 1024;
    int printIntervalRdfCnt = (1 << 30);
    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    int lastSentIfdId = -1;
    int timerRdfId = -1, chaserRdfIdLowerBound = -1, oldLcacIfdId = -1, toGenIfdId = -1, localRequiredIfdId = -1;
    int newLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    int newChaserRdfId = 0, newReferenceBattleChaserRdfId = 0;
    jtshared::RenderFrame* outRdf = google::protobuf::Arena::Create<RenderFrame>(&pbTestCaseDataAllocator);
    while (loopRdfCnt > outerTimerRdfId) {
        bool ok1 = reusedBattle->GetRdfAndIfdIds(&timerRdfId, &newChaserRdfId, &chaserRdfIdLowerBound, &oldLcacIfdId, &toGenIfdId, &localRequiredIfdId);
        // Handling TCP packets first, and then UDP packets, the same as C# side behavior.
        if (incomingDownsyncSnapshots3.count(outerTimerRdfId)) {
            DownsyncSnapshot* srvDownsyncSnapshot = incomingDownsyncSnapshots3[outerTimerRdfId];
            int outPostTimerRdfEvictedCnt = 0, outPostTimerRdfDelayedIfdEvictedCnt = 0;
            bool applied = reusedBattle->OnDownsyncSnapshotReceived(srvDownsyncSnapshot, &outPostTimerRdfEvictedCnt, &outPostTimerRdfDelayedIfdEvictedCnt, &newChaserRdfId, &newLcacIfdId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
            if (0 == srvDownsyncSnapshot->st_ifd_id()) {
                JPH_ASSERT(25 == newLcacIfdId);
            } else if (780 == srvDownsyncSnapshot->ref_rdf_id()) {
                JPH_ASSERT(190 == newLcacIfdId); // Drags "lcacIfdId" forward regardless of discontinuity
                JPH_ASSERT(0 == outPostTimerRdfEvictedCnt);
                JPH_ASSERT(0 == outPostTimerRdfDelayedIfdEvictedCnt);
                JPH_ASSERT(outerTimerRdfId == reusedBattle->timerRdfId);
                // no eviction occurred
            }
            if (srvDownsyncSnapshot->has_ref_rdf()) {
                auto rdf = srvDownsyncSnapshot->release_ref_rdf();
                delete rdf;
            }
        }
        if (incomingUpsyncSnapshotReqs3.count(outerTimerRdfId)) {
            auto req = incomingUpsyncSnapshotReqs3[outerTimerRdfId];
            auto peerUpsyncSnapshot = req->upsync_snapshot();
            bool applied = reusedBattle->OnUpsyncSnapshotReceived(req->join_index(), peerUpsyncSnapshot, &newChaserRdfId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
            if (13 == peerUpsyncSnapshot.st_ifd_id()) {
                JPH_ASSERT(54 == newChaserRdfId);
                JPH_ASSERT(-1 == reusedBattle->lcacIfdId);
            } else if (0 == peerUpsyncSnapshot.st_ifd_id()) {
                JPH_ASSERT(2 == newChaserRdfId);
                JPH_ASSERT(-1 == reusedBattle->lcacIfdId);
            } else if (29 == peerUpsyncSnapshot.st_ifd_id()) {
                JPH_ASSERT(178 == newChaserRdfId);
                JPH_ASSERT(25 == reusedBattle->lcacIfdId);
            } else if (26 == peerUpsyncSnapshot.st_ifd_id()) {
                JPH_ASSERT(106 == newChaserRdfId);
                JPH_ASSERT(25 == reusedBattle->lcacIfdId);
            }
        }
        int oldIfdEdFrameId = reusedBattle->ifdBuffer.EdFrameId;
        uint64_t inSingleInput = getSelfCmdByRdfId(testCmds1, outerTimerRdfId);
        bool cmdInjected = FRONTEND_UpsertSelfCmd(reusedBattle, inSingleInput, &newChaserRdfId);
        if (!cmdInjected) {
            std::cerr << "Failed to inject cmd for outerTimerRdfId=" << outerTimerRdfId << ", inSingleInput=" << inSingleInput << std::endl;
            exit(1);
        }
        int chaserRdfIdEd = newChaserRdfId + globalPrimitiveConsts->max_chasing_render_frames_per_update();
        if (chaserRdfIdEd > outerTimerRdfId) {
            chaserRdfIdEd = outerTimerRdfId;
        }
        FRONTEND_ChaseRolledBackRdfs(reusedBattle, &newChaserRdfId, true);
        FRONTEND_Step(reusedBattle);

        if (lastSentIfdId < toGenIfdId) {
            long bytesCnts = pbBufferSizeLimit;
            int proposedBatchIfdIdSt = lastSentIfdId + 1;
            int proposedBatchIfdIdEd = toGenIfdId + 1;
            int lastIfdId = -1;
            reusedBattle->ProduceUpsyncSnapshotRequest(0, proposedBatchIfdIdSt, proposedBatchIfdIdEd, &lastIfdId, upsyncSnapshotBuffer, &bytesCnts);
            if (0 < bytesCnts) {
                JPH_ASSERT(-1 != lastIfdId);
                lastSentIfdId = lastIfdId;
            }
        }
        outerTimerRdfId++;
    }

    std::cout << "Passed TestCase3: lcacIfdId changes\n" << std::endl;
    reusedBattle->Clear();   
    return true;
}

bool runTestCase4(FrontendBattle* reusedBattle, WsReq* initializerMapData, int inSingleJoinIndex) {
    reusedBattle->SetFrameLogEnabled(true);
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);
    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 1024;
    int printIntervalRdfCnt = (1 << 0);
    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    int newChaserRdfId = 0;
    jtshared::RenderFrame* outRdf = google::protobuf::Arena::Create<RenderFrame>(&pbTestCaseDataAllocator);
    int newLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    while (loopRdfCnt > outerTimerRdfId) {
        if (incomingDownsyncSnapshots4.count(outerTimerRdfId)) {
            DownsyncSnapshot* srvDownsyncSnapshot = incomingDownsyncSnapshots4[outerTimerRdfId];
            int outPostTimerRdfEvictedCnt = 0, outPostTimerRdfDelayedIfdEvictedCnt = 0;
            bool applied = reusedBattle->OnDownsyncSnapshotReceived(srvDownsyncSnapshot, &outPostTimerRdfEvictedCnt, &outPostTimerRdfDelayedIfdEvictedCnt, &newChaserRdfId, &newLcacIfdId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
            if (srvDownsyncSnapshot->has_ref_rdf()) {
                auto rdf = srvDownsyncSnapshot->release_ref_rdf();
                delete rdf;
            }
        }
        uint64_t inSingleInput = getSelfCmdByRdfId(testCmds4, outerTimerRdfId);
        bool cmdInjected = FRONTEND_UpsertSelfCmd(reusedBattle, inSingleInput, &newChaserRdfId);
        if (!cmdInjected) {
            std::cerr << "Failed to inject cmd for outerTimerRdfId=" << outerTimerRdfId << ", inSingleInput=" << inSingleInput << std::endl;
            exit(1);
        }
        int chaserRdfIdEd = newChaserRdfId + globalPrimitiveConsts->max_chasing_render_frames_per_update();
        if (chaserRdfIdEd > outerTimerRdfId) {
            chaserRdfIdEd = outerTimerRdfId;
        }
        FRONTEND_ChaseRolledBackRdfs(reusedBattle, &newChaserRdfId, true);
        FRONTEND_Step(reusedBattle);
        if (17 == outerTimerRdfId) {
            JPH_ASSERT(reusedBattle->chaserRdfIdLowerBound == outerTimerRdfId + 1);
            auto frameLog = reusedBattle->frameLogBuffer.GetByFrameId(reusedBattle->chaserRdfIdLowerBound);
            JPH_ASSERT(frameLog->rdf().id() == reusedBattle->chaserRdfIdLowerBound);
            JPH_ASSERT(frameLog->chaser_rdf_id_lower_bound_snatched());
        }
        outerTimerRdfId++;
    }

    std::cout << "Passed TestCase4: Advanced RefRdf in DownsyncSnapshot\n" << std::endl;
    reusedBattle->Clear();   
    return true;
}

bool runTestCase5(FrontendBattle* reusedBattle, WsReq* initializerMapData, int inSingleJoinIndex) {
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);
    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 1024;
    int printIntervalRdfCnt = (1 << 0);
    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    int lastSentIfdId = -1;
    int timerRdfId = -1, chaserRdfIdLowerBound = -1, oldLcacIfdId = -1, toGenIfdId = -1, localRequiredIfdId = -1;
    int newLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    int newChaserRdfId = 0, newReferenceBattleChaserRdfId = 0;
    jtshared::RenderFrame* outRdf = google::protobuf::Arena::Create<RenderFrame>(&pbTestCaseDataAllocator);
    while (loopRdfCnt > outerTimerRdfId) {
        if (incomingDownsyncSnapshots5.count(outerTimerRdfId)) {
            DownsyncSnapshot* srvDownsyncSnapshot = incomingDownsyncSnapshots5[outerTimerRdfId];
            int outPostTimerRdfEvictedCnt = 0, outPostTimerRdfDelayedIfdEvictedCnt = 0;
            bool applied = reusedBattle->OnDownsyncSnapshotReceived(srvDownsyncSnapshot, &outPostTimerRdfEvictedCnt, &outPostTimerRdfDelayedIfdEvictedCnt, &newChaserRdfId, &newLcacIfdId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
            if (0 == srvDownsyncSnapshot->st_ifd_id()) {
                JPH_ASSERT(25 == newLcacIfdId);
                JPH_ASSERT(25 == minPlayerInputFrontId && 25 == maxPlayerInputFrontId);
            } else if (26 == srvDownsyncSnapshot->st_ifd_id()) {
                JPH_ASSERT(57 == newLcacIfdId);
            }
            if (srvDownsyncSnapshot->has_ref_rdf()) {
                auto rdf = srvDownsyncSnapshot->release_ref_rdf();
                delete rdf;
            }
        }
        uint64_t inSingleInput = getSelfCmdByRdfId(testCmds4, outerTimerRdfId);
        bool cmdInjected = FRONTEND_UpsertSelfCmd(reusedBattle, inSingleInput, &newChaserRdfId);
        if (!cmdInjected) {
            std::cerr << "Failed to inject cmd for outerTimerRdfId=" << outerTimerRdfId << ", inSingleInput=" << inSingleInput << std::endl;
            exit(1);
        }
        int chaserRdfIdEd = newChaserRdfId + globalPrimitiveConsts->max_chasing_render_frames_per_update();
        if (chaserRdfIdEd > outerTimerRdfId) {
            chaserRdfIdEd = outerTimerRdfId;
        }
        FRONTEND_ChaseRolledBackRdfs(reusedBattle, &newChaserRdfId, true);
        FRONTEND_Step(reusedBattle);
        outerTimerRdfId++;
    }

    std::cout << "Passed TestCase5: More DownsyncSnapshot tests\n" << std::endl;
    reusedBattle->Clear();   
    return true;
}

bool runTestCase6(FrontendBattle* reusedBattle, WsReq* initializerMapData, int inSingleJoinIndex) {
    
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);

    FrontendBattle* referenceBattle = static_cast<FrontendBattle*>(FRONTEND_CreateBattle(512, true));
    
    referenceBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);

    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 380;
    int printIntervalRdfCnt = (1 << 5);

    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    int timerRdfId = -1, toGenIfdId = -1, localRequiredIfdId = -1; // shared 
    int chaserRdfIdLowerBound = -1, oldLcacIfdId = -1, newLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    int newChaserRdfId = 0, newReferenceBattleChaserRdfId = 0;
    int referenceBattleChaserRdfId = -1, referenceBattleChaserRdfIdLowerBound = -1, referenceBattleOldLcacIfdId = -1, referenceBattleNewLcacIfdId = -1, referenceBattleMaxPlayerInputFrontId = 0, referenceBattleMinPlayerInputFrontId = 0;
    jtshared::RenderFrame* outRdf = google::protobuf::Arena::Create<RenderFrame>(&pbTestCaseDataAllocator);
    jtshared::RenderFrame* referenceBattleOutRdf = google::protobuf::Arena::Create<RenderFrame>(&pbTestCaseDataAllocator);
    while (loopRdfCnt > outerTimerRdfId) {
        bool shouldPrint = false;
        if (incomingUpsyncSnapshotReqs6Intime.count(outerTimerRdfId)) {
            auto req = incomingUpsyncSnapshotReqs6Intime[outerTimerRdfId];
            auto peerUpsyncSnapshot = req->upsync_snapshot();
            referenceBattle->OnUpsyncSnapshotReceived(req->join_index(), peerUpsyncSnapshot, &newReferenceBattleChaserRdfId, &referenceBattleMaxPlayerInputFrontId, &referenceBattleMinPlayerInputFrontId);
            shouldPrint = true;
        }

        if (incomingUpsyncSnapshotReqs6Rollback.count(outerTimerRdfId)) {
            auto req = incomingUpsyncSnapshotReqs6Rollback[outerTimerRdfId];
            auto peerUpsyncSnapshot = req->upsync_snapshot();
            reusedBattle->OnUpsyncSnapshotReceived(req->join_index(), peerUpsyncSnapshot, &newChaserRdfId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
            if (155 == outerTimerRdfId) {
                JPH_ASSERT(BaseBattle::ConvertToFirstUsedRenderFrameId(1) == newChaserRdfId);
            } else if (300 == outerTimerRdfId) {
                JPH_ASSERT(178 == newChaserRdfId);
            } else if (320 == outerTimerRdfId) {
                JPH_ASSERT(BaseBattle::ConvertToFirstUsedRenderFrameId(26) == newChaserRdfId);
            } else if (330 == outerTimerRdfId) {
                JPH_ASSERT(BaseBattle::ConvertToFirstUsedRenderFrameId(60) == newChaserRdfId);
            }
            shouldPrint = true;
        }

        uint64_t inSingleInput = getSelfCmdByRdfId(testCmds4, outerTimerRdfId);
        bool referenceBattleCmdInjected = FRONTEND_UpsertSelfCmd(referenceBattle, inSingleInput, &newReferenceBattleChaserRdfId);
        if (!referenceBattleCmdInjected) {
            std::cerr << "Failed to inject cmd to referenceBattle for outerTimerRdfId=" << outerTimerRdfId << ", inSingleInput=" << inSingleInput << std::endl;
            exit(1);
        }
        FRONTEND_ChaseRolledBackRdfs(referenceBattle, &newReferenceBattleChaserRdfId, true);
        FRONTEND_Step(referenceBattle);

        bool cmdInjected = FRONTEND_UpsertSelfCmd(reusedBattle, inSingleInput, &newChaserRdfId);
        if (!cmdInjected) {
            std::cerr << "Failed to inject cmd for outerTimerRdfId=" << outerTimerRdfId << ", inSingleInput=" << inSingleInput << std::endl;
            exit(1);
        }
       
        FRONTEND_ChaseRolledBackRdfs(reusedBattle, &newChaserRdfId, true);
        FRONTEND_Step(reusedBattle);
        if (155 == outerTimerRdfId) {
            int lastToBeConsistentRdfId = BaseBattle::ConvertToLastUsedRenderFrameId(25) + 1;
            for (int recRdfId = 1; recRdfId <= lastToBeConsistentRdfId; recRdfId++) {
                auto referencedRdf = referenceBattle->rdfBuffer.GetByFrameId(recRdfId);
                auto challengingRdf = reusedBattle->rdfBuffer.GetByFrameId(recRdfId);
                BaseBattle::AssertNearlySame(referencedRdf, challengingRdf);
            }
        } else if (320 == outerTimerRdfId) {
            int lastToBeConsistentRdfId = BaseBattle::ConvertToLastUsedRenderFrameId(59) + 1;  
            auto referencedRdf = referenceBattle->rdfBuffer.GetByFrameId(lastToBeConsistentRdfId);
            auto challengingRdf = reusedBattle->rdfBuffer.GetByFrameId(lastToBeConsistentRdfId);
            BaseBattle::AssertNearlySame(referencedRdf, challengingRdf);
        } else if (330 == outerTimerRdfId) {
            int lastToBeConsistentRdfId = BaseBattle::ConvertToLastUsedRenderFrameId(60) + 1;
            for (int tRdfId = globalPrimitiveConsts->starting_render_frame_id(); tRdfId <= lastToBeConsistentRdfId; tRdfId++) {
                auto referencedRdf = referenceBattle->rdfBuffer.GetByFrameId(tRdfId);
                auto challengingRdf = reusedBattle->rdfBuffer.GetByFrameId(tRdfId);
                BaseBattle::AssertNearlySame(referencedRdf, challengingRdf);
            }
        }
        outerTimerRdfId++;

        shouldPrint |= (outerTimerRdfId >= loopRdfCnt);
        // Printing
        if (0 < outerTimerRdfId && shouldPrint) {
            int delayedIfdId = BaseBattle::ConvertToDelayedInputFrameId(outerTimerRdfId);

            // Fetch reference battle rdf
            InputFrameDownsync* referenceBattleDelayedIfd = referenceBattle->ifdBuffer.GetByFrameId(delayedIfdId);
            memset(rdfFetchBuffer, 0, sizeof(rdfFetchBuffer));
            long outBytesCnt = pbBufferSizeLimit;
            APP_GetRdf(referenceBattle, outerTimerRdfId, rdfFetchBuffer, &outBytesCnt);
            referenceBattleOutRdf->ParseFromArray(rdfFetchBuffer, outBytesCnt);
          
            // Fetch rollback battle rdf
            InputFrameDownsync* delayedIfd = reusedBattle->ifdBuffer.GetByFrameId(delayedIfdId);
            memset(rdfFetchBuffer, 0, sizeof(rdfFetchBuffer));
            outBytesCnt = pbBufferSizeLimit;
            APP_GetRdf(reusedBattle, outerTimerRdfId, rdfFetchBuffer, &outBytesCnt);
            outRdf->ParseFromArray(rdfFetchBuffer, outBytesCnt);
        }
    }

    std::cout << "Passed TestCase6: Basic rollback-chasing\n" << std::endl;
    reusedBattle->Clear();   
    APP_DestroyBattle(referenceBattle);
    return true;
}

bool runTestCase7(FrontendBattle* reusedBattle, WsReq* initializerMapData, int inSingleJoinIndex) {
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);

    FrontendBattle* referenceBattle = static_cast<FrontendBattle*>(FRONTEND_CreateBattle(512, true));
    referenceBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);

    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 69;
    int printIntervalRdfCnt = (1 << 4);

    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    int timerRdfId = -1, toGenIfdId = -1, localRequiredIfdId = -1; // shared 
    int chaserRdfIdLowerBound = -1, oldLcacIfdId = -1, newLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    int newChaserRdfId = 0, newReferenceBattleChaserRdfId = 0;
    int referenceBattleChaserRdfId = -1, referenceBattleChaserRdfIdLowerBound = -1, referenceBattleOldLcacIfdId = -1, referenceBattleNewLcacIfdId = -1, referenceBattleMaxPlayerInputFrontId = 0, referenceBattleMinPlayerInputFrontId = 0;
    jtshared::RenderFrame* outRdf = google::protobuf::Arena::Create<RenderFrame>(&pbTestCaseDataAllocator);
    jtshared::RenderFrame* referenceBattleOutRdf = google::protobuf::Arena::Create<RenderFrame>(&pbTestCaseDataAllocator);
    while (loopRdfCnt > outerTimerRdfId) {
        bool shouldPrint = false;
        if (incomingUpsyncSnapshotReqs7Intime.count(outerTimerRdfId)) {
            auto req = incomingUpsyncSnapshotReqs7Intime[outerTimerRdfId];
            auto peerUpsyncSnapshot = req->upsync_snapshot();
            referenceBattle->OnUpsyncSnapshotReceived(req->join_index(), peerUpsyncSnapshot, &newReferenceBattleChaserRdfId, &referenceBattleMaxPlayerInputFrontId, &referenceBattleMinPlayerInputFrontId);
            shouldPrint = true;
        }

        if (incomingUpsyncSnapshotReqs7Rollback.count(outerTimerRdfId)) {
            auto req = incomingUpsyncSnapshotReqs7Rollback[outerTimerRdfId];
            auto peerUpsyncSnapshot = req->upsync_snapshot();
            reusedBattle->OnUpsyncSnapshotReceived(req->join_index(), peerUpsyncSnapshot, &newChaserRdfId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
            shouldPrint = true;
            if (68 == outerTimerRdfId) {
                JPH_ASSERT(66 == newChaserRdfId);
            }
        }

        uint64_t inSingleInput = 0;
        bool referenceBattleCmdInjected = FRONTEND_UpsertSelfCmd(referenceBattle, inSingleInput, &newReferenceBattleChaserRdfId);
        if (!referenceBattleCmdInjected) {
            std::cerr << "Failed to inject cmd to referenceBattle for outerTimerRdfId=" << outerTimerRdfId << ", inSingleInput=" << inSingleInput << std::endl;
            exit(1);
        }
        int referenceBattleChaserRdfIdEd = outerTimerRdfId;
        FRONTEND_ChaseRolledBackRdfs(referenceBattle, &newReferenceBattleChaserRdfId, true);
        FRONTEND_Step(referenceBattle);

        bool cmdInjected = FRONTEND_UpsertSelfCmd(reusedBattle, inSingleInput, &newChaserRdfId);
        if (!cmdInjected) {
            std::cerr << "Failed to inject cmd for outerTimerRdfId=" << outerTimerRdfId << ", inSingleInput=" << inSingleInput << std::endl;
            exit(1);
        }
        int chaserRdfIdEd = outerTimerRdfId;
       
        FRONTEND_ChaseRolledBackRdfs(reusedBattle, &newChaserRdfId, true);
        FRONTEND_Step(reusedBattle);
        if (68 == outerTimerRdfId) {
            int lastToBeConsistentRdfId = BaseBattle::ConvertToLastUsedRenderFrameId(17) + 1;
            if (lastToBeConsistentRdfId > loopRdfCnt) {
                lastToBeConsistentRdfId = loopRdfCnt;
            }
            for (int tRdfId = globalPrimitiveConsts->starting_render_frame_id(); tRdfId <= lastToBeConsistentRdfId; tRdfId++) {
                auto referencedRdf = referenceBattle->rdfBuffer.GetByFrameId(tRdfId);
                auto challengingRdf = reusedBattle->rdfBuffer.GetByFrameId(tRdfId);
                BaseBattle::AssertNearlySame(referencedRdf, challengingRdf);
            }
        }
        outerTimerRdfId++;

        shouldPrint |= (outerTimerRdfId >= loopRdfCnt);
        // Printing
        if (0 < outerTimerRdfId && shouldPrint) {
            int delayedIfdId = BaseBattle::ConvertToDelayedInputFrameId(outerTimerRdfId);

            // Fetch reference battle rdf
            InputFrameDownsync* referenceBattleDelayedIfd = referenceBattle->ifdBuffer.GetByFrameId(delayedIfdId);
            memset(rdfFetchBuffer, 0, sizeof(rdfFetchBuffer));
            long outBytesCnt = pbBufferSizeLimit;
            APP_GetRdf(referenceBattle, outerTimerRdfId, rdfFetchBuffer, &outBytesCnt);
            referenceBattleOutRdf->ParseFromArray(rdfFetchBuffer, outBytesCnt);
            
            // Fetch rollback battle rdf
            InputFrameDownsync* delayedIfd = reusedBattle->ifdBuffer.GetByFrameId(delayedIfdId);
            memset(rdfFetchBuffer, 0, sizeof(rdfFetchBuffer));
            outBytesCnt = pbBufferSizeLimit;
            APP_GetRdf(reusedBattle, outerTimerRdfId, rdfFetchBuffer, &outBytesCnt);
            outRdf->ParseFromArray(rdfFetchBuffer, outBytesCnt);
            
        }
    }

    std::cout << "Passed TestCase7: Basic rollback-chasing, another\n" << std::endl;
    reusedBattle->Clear();   
    APP_DestroyBattle(referenceBattle);
    return true;
}

bool runTestCase8(FrontendBattle* reusedBattle, WsReq* initializerMapData, int inSingleJoinIndex) {
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);
    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 2048;
    int printIntervalRdfCnt = (1 << 2);
    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    jtshared::RenderFrame* outRdf = google::protobuf::Arena::Create<RenderFrame>(&pbTestCaseDataAllocator);
    int newLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    int newChaserRdfId = 0, newReferenceBattleChaserRdfId = 0;
    while (loopRdfCnt > outerTimerRdfId) {
        // Handling TCP packets first, and then UDP packets, the same as C# side behavior.

        uint64_t inSingleInput = getSelfCmdByRdfId(testCmds8, outerTimerRdfId);
        bool cmdInjected = FRONTEND_UpsertSelfCmd(reusedBattle, inSingleInput, &newChaserRdfId);
        if (!cmdInjected) {
            std::cerr << "Failed to inject cmd for outerTimerRdfId=" << outerTimerRdfId << ", inSingleInput=" << inSingleInput << std::endl;
            exit(1);
        }
        FRONTEND_Step(reusedBattle);
        auto outerTimerRdf = reusedBattle->rdfBuffer.GetByFrameId(outerTimerRdfId);
        auto& p1 = outerTimerRdf->players(0);
        auto& p1Chd = p1.chd();
        auto p1Ud = BaseBattleCollisionFilter::calcPlayerUserData(p1.join_index());

        auto& npc1 = outerTimerRdf->npcs(0);
        auto& npc1Chd = npc1.chd();
        auto npc1Ud = BaseBattleCollisionFilter::calcNpcUserData(npc1.id());

        auto& npc2 = outerTimerRdf->npcs(1);
        auto& npc2Chd = npc2.chd();
        auto npc2Ud = BaseBattleCollisionFilter::calcNpcUserData(npc2.id());

        bool shouldPrint = false;
        
        if (600 > outerTimerRdfId && 440 <= outerTimerRdfId) {
            shouldPrint = true;
        }
        
        if (shouldPrint) {
            std::cout << "TestCase8/outerTimerRdfId=" << outerTimerRdfId << "\n\tp1Chd ud=" << p1Ud << ", hp=" << p1Chd.hp() << ", cs=" << p1Chd.ch_state() << ", fc=" << p1Chd.frames_in_ch_state() << ", q=(" << p1Chd.q_x() << ", " << p1Chd.q_y() << ", " << p1Chd.q_z() << ", " << p1Chd.q_w() << "), pos=(" << p1Chd.x() << ", " << p1Chd.y() << ", " << p1Chd.z() << "), vel=(" << p1Chd.vel_x() << ", " << p1Chd.vel_y() << ")\n\tnpc1Chd ud=" << npc1Ud << ", hp=" << npc1Chd.hp() << ", cs=" << npc1Chd.ch_state() << ", fc=" << npc1Chd.frames_in_ch_state() << ", q=(" << npc1Chd.q_x() << ", " << npc1Chd.q_y() << ", " << npc1Chd.q_z() << ", " << npc1Chd.q_w() << "), pos=(" << npc1Chd.x() << ", " << npc1Chd.y() << ", " << npc1Chd.z() << "), vel=(" << npc1Chd.vel_x() << ", " << npc1Chd.vel_y() << "), ccmd=" << npc1.cached_cue_cmd() << "\n\tnpc2Chd ud=" << npc2Ud << ", hp=" << npc2Chd.hp() << ", cs=" << npc2Chd.ch_state() << ", fc=" << npc2Chd.frames_in_ch_state() << ", q=(" << npc2Chd.q_x() << ", " << npc2Chd.q_y() << ", " << npc2Chd.q_z() << ", " << npc2Chd.q_w() << "), pos=(" << npc2Chd.x() << ", " << npc2Chd.y() << ", " << npc2Chd.z() << "), vel=(" << npc2Chd.vel_x() << ", " << npc2Chd.vel_y() << "), ccmd=" << npc2.cached_cue_cmd() << std::endl;
        }

        if (32 == outerTimerRdfId) {
            // It has landed on "npcVisionHull5" and moving to the right
            JPH_ASSERT(CharacterState::Walking == npc2Chd.ch_state());
            JPH_ASSERT(400 < npc2Chd.y() && 500 >= npc2Chd.y());
            JPH_ASSERT(350 < npc2Chd.x() && 400 > npc2Chd.x());
            JPH_ASSERT(0 < npc2Chd.vel_x());
            shouldPrint = true;
        } else if (220 == outerTimerRdfId) {
            // Same as above, this is a relative long walk
            JPH_ASSERT(CharacterState::Walking == npc2Chd.ch_state());
            JPH_ASSERT(400 < npc2Chd.y() && 500 >= npc2Chd.y());
            JPH_ASSERT(400 < npc2Chd.x() && 500 > npc2Chd.x());
            JPH_ASSERT(0 > npc2Chd.vel_x());
            shouldPrint = true;
        } else if (320 == outerTimerRdfId) {
            // It has turned around on "npcVisionHull5" and moving to the left due to vision reaction of "npcVisionHull3"
            JPH_ASSERT(CharacterState::Walking == npc2Chd.ch_state());
            JPH_ASSERT(400 < npc2Chd.y() && 500 >= npc2Chd.y());
            JPH_ASSERT(350 < npc2Chd.x() && 500 > npc2Chd.x());
            JPH_ASSERT(0 > npc2Chd.vel_x());
            shouldPrint = true;
        } else if (485 == outerTimerRdfId) {
            // It's proactively jumping towards left onto "npcVisionHull6" and moving to the left
            JPH_ASSERT(CharacterState::InAirIdle1ByJump == npc2Chd.ch_state());
            JPH_ASSERT(0 > npc2Chd.vel_x());
            shouldPrint = true;
        } else if (513 == outerTimerRdfId) {
            // It has jumped on "npcVisionHull6" and moving to the left
            JPH_ASSERT(CharacterState::Walking == npc2Chd.ch_state());
            JPH_ASSERT(500 < npc2Chd.y() && 520 >= npc2Chd.y());
            JPH_ASSERT(200 < npc2Chd.x() && 350 > npc2Chd.x());
            JPH_ASSERT(0 > npc2Chd.vel_x());
            shouldPrint = true;
        } else if (900 == outerTimerRdfId) {
            // It's still on "npcVisionHull6" but turned to move to the right due to vision reaction of "npcVisionHull7"
            JPH_ASSERT(CharacterState::Walking == npc2Chd.ch_state());
            JPH_ASSERT(500 < npc2Chd.y() && 520 >= npc2Chd.y());
            JPH_ASSERT(100 < npc2Chd.x() && 300 > npc2Chd.x());
            JPH_ASSERT(0 < npc2Chd.vel_x());
            shouldPrint = true;
        } else if (1300 == outerTimerRdfId) {
            // When standing on the edge of "npcVisionHull6", it should've seen the lower platform  "npcVisionHull5" and decided to move to the right and fall onto "npcVisionHull5"
            JPH_ASSERT(CharacterState::Walking == npc2Chd.ch_state());
            JPH_ASSERT(400 < npc2Chd.y() && 500 >= npc2Chd.y());
            JPH_ASSERT(300 < npc2Chd.x() && 500 > npc2Chd.x());
            JPH_ASSERT(0 < npc2Chd.vel_x());
            shouldPrint = true;
        } else if (1440 == outerTimerRdfId) {
            // Again it has turned around on "npcVisionHull5" and moving to the left due to vision reaction of "npcVisionHull3"
            JPH_ASSERT(CharacterState::Walking == npc2Chd.ch_state());
            JPH_ASSERT(400 < npc2Chd.y() && 500 >= npc2Chd.y());
            JPH_ASSERT(300 < npc2Chd.x() && 500 > npc2Chd.x());
            JPH_ASSERT(0 > npc2Chd.vel_x());
            shouldPrint = true;
        }
        
        outerTimerRdfId++;
    }

    std::cout << "Passed TestCase8: Basic npc vision\n" << std::endl;
    reusedBattle->Clear();
    return true;
}

bool runTestCase9(FrontendBattle* reusedBattle, WsReq* initializerMapData, int inSingleJoinIndex) {
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);
    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 2048;
    int printIntervalRdfCnt = (1 << 4);
    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    jtshared::RenderFrame* outRdf = google::protobuf::Arena::Create<RenderFrame>(&pbTestCaseDataAllocator);
    int newLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    int newChaserRdfId = 0, newReferenceBattleChaserRdfId = 0;
    while (loopRdfCnt > outerTimerRdfId) {
        bool shouldPrint = false;

        // Handling TCP packets first, and then UDP packets, the same as C# side behavior.
        
        uint64_t inSingleInput = getSelfCmdByRdfId(testCmds9, outerTimerRdfId);
        bool cmdInjected = FRONTEND_UpsertSelfCmd(reusedBattle, inSingleInput, &newChaserRdfId);
        if (!cmdInjected) {
            std::cerr << "Failed to inject cmd for outerTimerRdfId=" << outerTimerRdfId << ", inSingleInput=" << inSingleInput << std::endl;
            exit(1);
        }
        FRONTEND_Step(reusedBattle);
        auto outerTimerRdf = reusedBattle->rdfBuffer.GetByFrameId(outerTimerRdfId);
        auto& p1 = outerTimerRdf->players(0);
        auto& p1Chd = p1.chd();
        auto& tp1 = outerTimerRdf->dynamic_traps(0);

        if (50 <= outerTimerRdfId && outerTimerRdfId <= 100) {
            JPH_ASSERT(0 < tp1.x() && tp1.x() < 500);
            JPH_ASSERT(750 == tp1.y());
            //shouldPrint = true;
        } else if (231 <= outerTimerRdfId && outerTimerRdfId <= 248) {
            int p1ExpectedFramesInChState = outerTimerRdfId - 231;
            const Skill* skill = nullptr;
            const BulletConfig* bulletConfig = nullptr;
            uint32_t p1ChdActiveSkillId = p1Chd.active_skill_id();
            int p1ChdActiveSkillHit = p1Chd.active_skill_hit();
            BaseBattle::FindBulletConfig(p1ChdActiveSkillId, p1ChdActiveSkillHit, skill, bulletConfig);
            JPH_ASSERT(nullptr != skill && nullptr != bulletConfig);
            int p1ExpectedFramesToRecover = skill->recovery_frames() - (p1ExpectedFramesInChState);
            JPH_ASSERT(CharacterState::Atk1 == p1Chd.ch_state());
            JPH_ASSERT(p1ExpectedFramesInChState == p1Chd.frames_in_ch_state());
            JPH_ASSERT(p1ExpectedFramesToRecover == p1Chd.frames_to_recover());
            JPH_ASSERT(500 <= tp1.x());
            JPH_ASSERT(750 == tp1.y());
            //shouldPrint = true;
        } else if (249 == outerTimerRdfId) {
            JPH_ASSERT(CharacterState::Idle1 == p1Chd.ch_state());
            //shouldPrint = true;
        } else if (303 < outerTimerRdfId && outerTimerRdfId <= 420) {
            JPH_ASSERT(0 < tp1.x() && tp1.x() < 501);
            JPH_ASSERT(750 == tp1.y());
            //shouldPrint = true;
        } else if (outerTimerRdfId <= 1000) {
            JPH_ASSERT(750 == tp1.y());
            //shouldPrint = true;
        }
        
        if (shouldPrint) {
            std::cout << "TestCase9/outerTimerRdfId=" << outerTimerRdfId << "\n\ttp1 pos=(" << tp1.x() << ", " << tp1.y() << "," << tp1.z() << "), dir=(" << tp1.q_x() << ", " << tp1.q_y() << ", " << tp1.q_z() << ", " << tp1.q_w() << "), vel=(" << tp1.vel_x() << ", " << tp1.vel_y() << ", " << tp1.vel_z() << ")" << std::endl;
        }

        outerTimerRdfId++;
    }

    std::cout << "Passed TestCase9: Slider Trap\n" << std::endl;
    reusedBattle->Clear();
    return true;
}

bool runTestCase10(FrontendBattle* reusedBattle, WsReq* initializerMapData, int inSingleJoinIndex) {
    reusedBattle->SetFrameLogEnabled(true);
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);
    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 1024;
    int printIntervalRdfCnt = (1 << 4);
    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    jtshared::RenderFrame* outRdf = google::protobuf::Arena::Create<RenderFrame>(&pbTestCaseDataAllocator);
    int newLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    int newChaserRdfId = 0, newReferenceBattleChaserRdfId = 0;
    while (loopRdfCnt > outerTimerRdfId) {
        // Handling TCP packets first, and then UDP packets, the same as C# side behavior.
        if (incomingDownsyncSnapshots10.count(outerTimerRdfId)) {
            DownsyncSnapshot* srvDownsyncSnapshot = incomingDownsyncSnapshots10[outerTimerRdfId];
            int outPostTimerRdfEvictedCnt = 0, outPostTimerRdfDelayedIfdEvictedCnt = 0;
            bool applied = reusedBattle->OnDownsyncSnapshotReceived(srvDownsyncSnapshot, &outPostTimerRdfEvictedCnt, &outPostTimerRdfDelayedIfdEvictedCnt, &newChaserRdfId, &newLcacIfdId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
            if (308 == outerTimerRdfId) {
                JPH_ASSERT(306 == reusedBattle->chaserRdfId);
                int toTestIfdId = BaseBattle::ConvertToDelayedInputFrameId(outerTimerRdfId);
                JPH_ASSERT(76 == toTestIfdId);
                InputFrameDownsync* toTestIfd = reusedBattle->ifdBuffer.GetByFrameId(toTestIfdId);
                JPH_ASSERT(nullptr != toTestIfd);
                JPH_ASSERT(3 == toTestIfd->confirmed_list());
            }
            if (srvDownsyncSnapshot->has_ref_rdf()) {
                auto rdf = srvDownsyncSnapshot->release_ref_rdf();
                delete rdf;
            }
        }
        if (incomingUpsyncSnapshotReqs10.count(outerTimerRdfId)) {
            auto req = incomingUpsyncSnapshotReqs10[outerTimerRdfId]; 
            auto peerUpsyncSnapshot = req->upsync_snapshot();
            auto peerJoinIndex = req->join_index();
            bool applied = reusedBattle->OnUpsyncSnapshotReceived(peerJoinIndex, peerUpsyncSnapshot, &newChaserRdfId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
            if (269 == outerTimerRdfId) {
                JPH_ASSERT(269 == reusedBattle->chaserRdfId);
                int toTestIfdId = 75;
                InputFrameDownsync* toTestIfd = reusedBattle->ifdBuffer.GetByFrameId(toTestIfdId);
                JPH_ASSERT(nullptr != toTestIfd);
                uint32_t peerJoinIndex = 2;
                uint64_t peerJoinIndexMask = BaseBattle::CalcJoinIndexMask(peerJoinIndex);
                JPH_ASSERT(0 == toTestIfd->confirmed_list());
                JPH_ASSERT(0 < (toTestIfd->udp_confirmed_list() & peerJoinIndexMask));
            }
        }
        
        uint64_t inSingleInput = getSelfCmdByRdfId(testCmds10, outerTimerRdfId);
        bool cmdInjected = FRONTEND_UpsertSelfCmd(reusedBattle, inSingleInput, &newChaserRdfId);
        if (!cmdInjected) {
            std::cerr << "Failed to inject cmd for outerTimerRdfId=" << outerTimerRdfId << ", inSingleInput=" << inSingleInput << std::endl;
            exit(1);
        }
        FRONTEND_ChaseRolledBackRdfs(reusedBattle, &newChaserRdfId, true);
        FRONTEND_Step(reusedBattle);
        auto outerTimerRdf = reusedBattle->rdfBuffer.GetByFrameId(outerTimerRdfId);
        auto& p1 = outerTimerRdf->players(0);
        auto& p1Chd = p1.chd();

        outerTimerRdfId++;
    }

    FrameRingBuffer<FrameLog>& frameLogBuffer = reusedBattle->frameLogBuffer;
    FrameLog* frameLogToTest1 = frameLogBuffer.GetByFrameId(11);
    JPH_ASSERT(3 == frameLogToTest1->used_ifd_confirmed_list());
    FrameLog* frameLogToTest2 = frameLogBuffer.GetByFrameId(15);
    JPH_ASSERT(0 == frameLogToTest2->used_ifd_confirmed_list());

    std::cout << "Passed TestCase10: Regular DownsyncSnapshot and UpsyncSnapshot mixed handling\n" << std::endl;
    reusedBattle->Clear();   
    return true;
}

bool runTestCase11(FrontendBattle* reusedBattle, WsReq* initializerMapData, int inSingleJoinIndex) {
    bool doCompareWithRollback = true; 

    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);

    FrontendBattle* referenceBattle = static_cast<FrontendBattle*>(FRONTEND_CreateBattle(512, true));
    referenceBattle->SetFrameLogEnabled(true);
    referenceBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);

    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 1024;
    int printIntervalRdfCnt = (1 << 5);

    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    int timerRdfId = -1, toGenIfdId = -1, localRequiredIfdId = -1; // shared 
    int chaserRdfIdLowerBound = -1, oldLcacIfdId = -1, newLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    int newChaserRdfId = 0, newReferenceBattleChaserRdfId = 0;
    int referenceBattleChaserRdfId = -1, referenceBattleChaserRdfIdLowerBound = -1, referenceBattleOldLcacIfdId = -1, referenceBattleNewLcacIfdId = -1, referenceBattleMaxPlayerInputFrontId = 0, referenceBattleMinPlayerInputFrontId = 0;
    jtshared::RenderFrame* outRdf = google::protobuf::Arena::Create<RenderFrame>(&pbTestCaseDataAllocator);
    jtshared::RenderFrame* referenceBattleOutRdf = google::protobuf::Arena::Create<RenderFrame>(&pbTestCaseDataAllocator);
    while (loopRdfCnt > outerTimerRdfId) {
        if (incomingUpsyncSnapshotReqs11Intime.count(outerTimerRdfId)) {
            auto req = incomingUpsyncSnapshotReqs11Intime[outerTimerRdfId];
            auto peerUpsyncSnapshot = req->upsync_snapshot();
            referenceBattle->OnUpsyncSnapshotReceived(req->join_index(), peerUpsyncSnapshot, &newReferenceBattleChaserRdfId, &referenceBattleMaxPlayerInputFrontId, &referenceBattleMinPlayerInputFrontId);
        }

        if (doCompareWithRollback) {
            if (incomingUpsyncSnapshotReqs11Rollback.count(outerTimerRdfId)) {
                auto req = incomingUpsyncSnapshotReqs11Rollback[outerTimerRdfId];
                auto peerUpsyncSnapshot = req->upsync_snapshot();
                reusedBattle->OnUpsyncSnapshotReceived(req->join_index(), peerUpsyncSnapshot, &newChaserRdfId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
            }
        }

        uint64_t inSingleInput = getSelfCmdByRdfId(testCmds11, outerTimerRdfId);
        bool referenceBattleCmdInjected = FRONTEND_UpsertSelfCmd(referenceBattle, inSingleInput, &newReferenceBattleChaserRdfId);
        if (!referenceBattleCmdInjected) {
            std::cerr << "Failed to inject cmd to referenceBattle for outerTimerRdfId=" << outerTimerRdfId << ", inSingleInput=" << inSingleInput << std::endl;
            exit(1);
        }
        int referenceBattleChaserRdfIdEd = outerTimerRdfId;
        FRONTEND_ChaseRolledBackRdfs(referenceBattle, &newReferenceBattleChaserRdfId, true);
        FRONTEND_Step(referenceBattle);
        
        if (doCompareWithRollback) {
            bool cmdInjected = FRONTEND_UpsertSelfCmd(reusedBattle, inSingleInput, &newChaserRdfId);
            if (!cmdInjected) {
                std::cerr << "Failed to inject cmd for outerTimerRdfId=" << outerTimerRdfId << ", inSingleInput=" << inSingleInput << std::endl;
                exit(1);
            }

            bool shouldPrint = false;
            int chaserRdfIdEd = outerTimerRdfId;

            FRONTEND_ChaseRolledBackRdfs(reusedBattle, &newChaserRdfId, true);
            FRONTEND_Step(reusedBattle);

            auto referencedRdf = referenceBattle->rdfBuffer.GetByFrameId(outerTimerRdfId);
            auto challengingRdf = reusedBattle->rdfBuffer.GetByFrameId(outerTimerRdfId);

            const PlayerCharacterDownsync& referencedP1 = referencedRdf->players(0);
            const CharacterDownsync& p1Chd = referencedP1.chd();
            const PlayerCharacterDownsync& referencedP2 = referencedRdf->players(1);
            const CharacterDownsync& p2Chd = referencedP2.chd();
            const NpcCharacterDownsync& referencedNpc1 = referencedRdf->npcs(0);
            const CharacterDownsync& npc1Chd = referencedNpc1.chd();

            if (loopRdfCnt < outerTimerRdfId + 10) {
                
                shouldPrint = true;

                BaseBattle::AssertNearlySame(referencedRdf, challengingRdf);
            } else if (280 == outerTimerRdfId) {
                int firstToBeConsistentRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(0);
                int lastToBeConsistentRdfId = BaseBattle::ConvertToLastUsedRenderFrameId(4) + 1;
                for (int recRdfId = firstToBeConsistentRdfId; recRdfId <= lastToBeConsistentRdfId; recRdfId++) {
                    auto referencedRdf = referenceBattle->rdfBuffer.GetByFrameId(recRdfId);
                    auto challengingRdf = reusedBattle->rdfBuffer.GetByFrameId(recRdfId);
                    BaseBattle::AssertNearlySame(referencedRdf, challengingRdf);
                }
            } else if (400 == outerTimerRdfId) {
                int firstToBeConsistentRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(4);
                int lastToBeConsistentRdfId = BaseBattle::ConvertToLastUsedRenderFrameId(32) + 1;
                for (int recRdfId = firstToBeConsistentRdfId; recRdfId <= lastToBeConsistentRdfId; recRdfId++) {
                    auto referencedRdf = referenceBattle->rdfBuffer.GetByFrameId(recRdfId);
                    auto challengingRdf = reusedBattle->rdfBuffer.GetByFrameId(recRdfId);
                    BaseBattle::AssertNearlySame(referencedRdf, challengingRdf);
                }
            } else if (480 == outerTimerRdfId) {
                int firstToBeConsistentRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(17);
                int lastToBeConsistentRdfId = BaseBattle::ConvertToLastUsedRenderFrameId(60) + 1;
                for (int recRdfId = firstToBeConsistentRdfId; recRdfId <= lastToBeConsistentRdfId; recRdfId++) {
                    auto referencedRdf = referenceBattle->rdfBuffer.GetByFrameId(recRdfId);
                    auto challengingRdf = reusedBattle->rdfBuffer.GetByFrameId(recRdfId);
                    BaseBattle::AssertNearlySame(referencedRdf, challengingRdf);
                }
            }

            if (shouldPrint) {
                std::cout << "TestCase11/outerTimerRdfId=" << outerTimerRdfId << "\n\tp1Chd chState=" << p1Chd.ch_state() << ", framesInChState=" << p1Chd.frames_in_ch_state() << ", dir=(" << p1Chd.q_x() << ", " << p1Chd.q_y() << ", " << p1Chd.q_z() << ", " << p1Chd.q_w() << "), pos=(" << p1Chd.x() << ", " << p1Chd.y() << "), vel=(" << p1Chd.vel_x() << ", " << p1Chd.vel_y() << ")\n\tp2Chd chState=" << p2Chd.ch_state() << ", framesInChState=" << p2Chd.frames_in_ch_state() << ", dir=(" << p2Chd.q_x() << ", " << p2Chd.q_y() << ", " << p2Chd.q_z() << ", " << p2Chd.q_w() << "), pos=(" << p2Chd.x() << ", " << p2Chd.y() << "), vel=(" << p2Chd.vel_x() << ", " << p2Chd.vel_y() << ")\n\tnpc1Chd chState=" << npc1Chd.ch_state() << ", framesInChState=" << npc1Chd.frames_in_ch_state() << ", dir=(" << npc1Chd.q_x() << ", " << npc1Chd.q_y() << ", " << npc1Chd.q_z() << ", " << npc1Chd.q_w() << "), pos=(" << npc1Chd.x() << ", " << npc1Chd.y() << "), vel=(" << npc1Chd.vel_x() << ", " << npc1Chd.vel_y() << ")" << std::endl;
            }
        }
        outerTimerRdfId++;
       
    }

    std::cout << "Passed TestCase11: Rollback-chasing with character soft-pushback\n" << std::endl;
    reusedBattle->Clear();   
    APP_DestroyBattle(referenceBattle);
    return true;
}

bool runTestCase12(FrontendBattle* reusedBattle, WsReq* initializerMapData, int inSingleJoinIndex) {
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);

    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 1024;
    int printIntervalRdfCnt = (1 << 5);

    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    int timerRdfId = -1, toGenIfdId = -1, localRequiredIfdId = -1; // shared 
    int chaserRdfIdLowerBound = -1, oldLcacIfdId = -1, newLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    int newChaserRdfId = 0;
    while (loopRdfCnt > outerTimerRdfId) {
        bool shouldPrint = false;
        if (incomingUpsyncSnapshotReqs12Intime.count(outerTimerRdfId)) {
            auto req = incomingUpsyncSnapshotReqs12Intime[outerTimerRdfId];
            auto peerUpsyncSnapshot = req->upsync_snapshot();
            reusedBattle->OnUpsyncSnapshotReceived(req->join_index(), peerUpsyncSnapshot, &newChaserRdfId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
            shouldPrint = true;
        }

        uint64_t inSingleInput = getSelfCmdByRdfId(testCmds12, outerTimerRdfId);
        bool cmdInjected = FRONTEND_UpsertSelfCmd(reusedBattle, inSingleInput, &newChaserRdfId);
        if (!cmdInjected) {
            std::cerr << "TestCase12/Failed to inject cmd for outerTimerRdfId=" << outerTimerRdfId << ", inSingleInput=" << inSingleInput << std::endl;
            exit(1);
        }
        FRONTEND_ChaseRolledBackRdfs(reusedBattle, &newChaserRdfId, true);
        FRONTEND_Step(reusedBattle);

        RenderFrame* outerTimerRdf = reusedBattle->rdfBuffer.GetByFrameId(outerTimerRdfId);
        auto& p1 = outerTimerRdf->players(0);
        auto& p1Chd = p1.chd();
        auto& p2 = outerTimerRdf->players(1);
        auto& p2Chd = p2.chd();
        
        if (0 <= outerTimerRdfId && outerTimerRdfId <= 300) {    
            // shouldPrint = true;
        }

        if (shouldPrint) {
            std::cout << "TestCase12/outerTimerRdfId=" << outerTimerRdfId << "\n\tp1Chd hp=" << p1Chd.hp() << ", chState = " << p1Chd.ch_state() << ", framesInChState = " << p1Chd.frames_in_ch_state() << ", dir = (" << p1Chd.q_x() << ", " << p1Chd.q_y() << ", " << p1Chd.q_z() << ", " << p1Chd.q_w() << "), pos = (" << p1Chd.x() << ", " << p1Chd.y() << ", " << p1Chd.z() << "), vel = (" << p1Chd.vel_x() << ", " << p1Chd.vel_y() << ", " << p1Chd.vel_z() << ")\n\tp2Chd hp=" << p2Chd.hp() << ", chState = " << p2Chd.ch_state() << ", framesInChState = " << p2Chd.frames_in_ch_state() << ", dir = (" << p2Chd.q_x() << ", " << p2Chd.q_y() << ", " << p2Chd.q_z() << ", " << p2Chd.q_w() << "), pos = (" << p2Chd.x() << ", " << p2Chd.y() << ", " << p2Chd.z() << "), vel = (" << p2Chd.vel_x() << ", " << p2Chd.vel_y() << ", " << p2Chd.vel_z() << ")" << std::endl;
        }

        outerTimerRdfId++;
    }

    std::cout << "Passed TestCase12: Rapid firing and fallen death\n" << std::endl;
    reusedBattle->Clear();   

    return true;
}

bool runTestCase13(FrontendBattle* reusedBattle, WsReq* initializerMapData, int inSingleJoinIndex) {
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);

    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 1024;
    int printIntervalRdfCnt = (1 << 5);

    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    int timerRdfId = -1, toGenIfdId = -1, localRequiredIfdId = -1; // shared 
    int chaserRdfIdLowerBound = -1, oldLcacIfdId = -1, newLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    int newChaserRdfId = 0;
    while (loopRdfCnt > outerTimerRdfId) {
        bool shouldPrint = false;
        uint64_t inSingleInput = getSelfCmdByRdfId(testCmds13, outerTimerRdfId);
        bool cmdInjected = FRONTEND_UpsertSelfCmd(reusedBattle, inSingleInput, &newChaserRdfId);
        if (!cmdInjected) {
            std::cerr << "TestCase13/Failed to inject cmd for outerTimerRdfId=" << outerTimerRdfId << ", inSingleInput=" << inSingleInput << std::endl;
            exit(1);
        }
        FRONTEND_ChaseRolledBackRdfs(reusedBattle, &newChaserRdfId, true);
        FRONTEND_Step(reusedBattle);

        RenderFrame* outerTimerRdf = reusedBattle->rdfBuffer.GetByFrameId(outerTimerRdfId);
        auto& p1 = outerTimerRdf->players(0);
        auto& p1Chd = p1.chd();
        auto& p2 = outerTimerRdf->players(1);
        auto& p2Chd = p2.chd();
        
        auto& npc1 = outerTimerRdf->npcs(0);
        auto& npc1Chd = npc1.chd();

        if (100 <= outerTimerRdfId && outerTimerRdfId <= 160) {
            shouldPrint = true;
        }

        if (shouldPrint) {
            std::cout << "TestCase13/outerTimerRdfId=" << outerTimerRdfId << "\n\tp1Chd hp=" << p1Chd.hp() << ", cs=" << p1Chd.ch_state() << ", fc=" << p1Chd.frames_in_ch_state() << ", q=(" << p1Chd.q_x() << ", " << p1Chd.q_y() << ", " << p1Chd.q_z() << ", " << p1Chd.q_w() << "), pos=(" << p1Chd.x() << ", " << p1Chd.y() << ", " << p1Chd.z() << "), vel=(" << p1Chd.vel_x() << ", " << p1Chd.vel_y() << ", " << p1Chd.vel_z() << ")\n\tp2Chd hp=" << p2Chd.hp() << ", cs=" << p2Chd.ch_state() << ", fc=" << p2Chd.frames_in_ch_state() << ", q=(" << p2Chd.q_x() << ", " << p2Chd.q_y() << ", " << p2Chd.q_z() << ", " << p2Chd.q_w() << "), pos=(" << p2Chd.x() << ", " << p2Chd.y() << ", " << p2Chd.z() << "), vel=(" << p2Chd.vel_x() << ", " << p2Chd.vel_y() << ", " << p2Chd.vel_z() << ")" << std::endl;
        }

        if (63 == outerTimerRdfId) {
            JPH_ASSERT(CharacterState::Atk1 == p1Chd.ch_state());
            JPH_ASSERT(0 == p1Chd.frames_in_ch_state());
        } else if (103 <= outerTimerRdfId && outerTimerRdfId <= 124) {
            JPH_ASSERT(CharacterState::Dashing == p1Chd.ch_state());
            int expectedFramesInChState = outerTimerRdfId-103;
            JPH_ASSERT(p1Chd.frames_in_ch_state() == expectedFramesInChState);
        } else if (125 == outerTimerRdfId) {
            JPH_ASSERT(CharacterState::Idle1 == p1Chd.ch_state());
        }

        outerTimerRdfId++;
    }

    std::cout << "Passed TestCase13: BladeGirl skill\n" << std::endl;
    reusedBattle->Clear();   

    return true;
}

bool runTestCase14(FrontendBattle* reusedBattle, WsReq* initializerMapData, int inSingleJoinIndex) {
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);

    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 1024;
    int printIntervalRdfCnt = (1 << 5);

    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    int timerRdfId = -1, toGenIfdId = -1, localRequiredIfdId = -1; // shared 
    int chaserRdfIdLowerBound = -1, oldLcacIfdId = -1, newLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    int newChaserRdfId = 0;
    while (loopRdfCnt > outerTimerRdfId) {
        bool shouldPrint = false;
        if (incomingUpsyncSnapshotReqs14Intime.count(outerTimerRdfId)) {
            auto req = incomingUpsyncSnapshotReqs14Intime[outerTimerRdfId];
            auto peerUpsyncSnapshot = req->upsync_snapshot();
            reusedBattle->OnUpsyncSnapshotReceived(req->join_index(), peerUpsyncSnapshot, &newChaserRdfId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
            shouldPrint = true;
        }

        uint64_t inSingleInput = getSelfCmdByRdfId(testCmds14, outerTimerRdfId);
        bool cmdInjected = FRONTEND_UpsertSelfCmd(reusedBattle, inSingleInput, &newChaserRdfId);
        if (!cmdInjected) {
            std::cerr << "TestCase14/Failed to inject cmd for outerTimerRdfId=" << outerTimerRdfId << ", inSingleInput=" << inSingleInput << std::endl;
            exit(1);
        }
        FRONTEND_ChaseRolledBackRdfs(reusedBattle, &newChaserRdfId, true);
        FRONTEND_Step(reusedBattle);

        RenderFrame* outerTimerRdf = reusedBattle->rdfBuffer.GetByFrameId(outerTimerRdfId);
        auto& p1 = outerTimerRdf->players(0);
        auto& p1Chd = p1.chd();
        auto& p2 = outerTimerRdf->players(1);
        auto& p2Chd = p2.chd();
        
        auto& npc1 = outerTimerRdf->npcs(0);
        auto& npc1Chd = npc1.chd();

        if (71 == outerTimerRdfId) {
            JPH_ASSERT(CharacterState::Atk1 == p1Chd.ch_state());
            JPH_ASSERT(0 == p1Chd.frames_in_ch_state());
        }

        if (70 <= outerTimerRdfId && outerTimerRdfId <= 90) {
            JPH_ASSERT(150 == p2Chd.hp());
        }

        if (183 == outerTimerRdfId) {
            JPH_ASSERT(CharacterState::Atk1 == p1Chd.ch_state());
            JPH_ASSERT(0 == p1Chd.frames_in_ch_state());
        }

        if (110 < outerTimerRdfId && outerTimerRdfId <= 190) {
            if (111 == outerTimerRdfId) {
                shouldPrint = true;
            }
            JPH_ASSERT(140 == npc1Chd.hp());
        } else if (192 <= outerTimerRdfId && outerTimerRdfId <= 280) {
            JPH_ASSERT(130 == npc1Chd.hp());
        } else if (285 < outerTimerRdfId && outerTimerRdfId <= 400) {
            if (300 == outerTimerRdfId) {
                shouldPrint = true;
                JPH_ASSERT(140 == p2Chd.hp());
            }
        }
        
        if (shouldPrint) {
            std::cout << "TestCase14/outerTimerRdfId=" << outerTimerRdfId << "\n\tp1Chd hp=" << p1Chd.hp() << ", chState = " << p1Chd.ch_state() << ", framesInChState = " << p1Chd.frames_in_ch_state() << ", dir = (" << p1Chd.q_x() << ", " << p1Chd.q_y() << ", " << p1Chd.q_z() << ", " << p1Chd.q_w() << "), pos = (" << p1Chd.x() << ", " << p1Chd.y() << ", " << p1Chd.z() << "), vel = (" << p1Chd.vel_x() << ", " << p1Chd.vel_y() << ", " << p1Chd.vel_z() << ")\n\tp2Chd hp=" << p2Chd.hp() << ", chState = " << p2Chd.ch_state() << ", framesInChState = " << p2Chd.frames_in_ch_state() << ", dir = (" << p2Chd.q_x() << ", " << p2Chd.q_y() << ", " << p2Chd.q_z() << ", " << p2Chd.q_w() << "), pos = (" << p2Chd.x() << ", " << p2Chd.y() << ", " << p2Chd.z() << "), vel = (" << p2Chd.vel_x() << ", " << p2Chd.vel_y() << ", " << p2Chd.vel_z() << ")\n\tnpc1Chd hp=" << npc1Chd.hp() << ", chState = " << npc1Chd.ch_state() << ", framesInChState = " << npc1Chd.frames_in_ch_state() << ", dir = (" << npc1Chd.q_x() << ", " << npc1Chd.q_y() << ", " << npc1Chd.q_z() << ", " << npc1Chd.q_w() << "), pos = (" << npc1Chd.x() << ", " << npc1Chd.y() << ", " << npc1Chd.z() << "), vel = (" << npc1Chd.vel_x() << ", " << npc1Chd.vel_y() << ", " << npc1Chd.vel_z() << ")" << std::endl;
        }

        outerTimerRdfId++;
    }

    std::cout << "Passed TestCase14: BountyHunter skill\n" << std::endl;
    reusedBattle->Clear();   

    return true;
}

bool runTestCase15(FrontendBattle* reusedBattle, WsReq* initializerMapData, int inSingleJoinIndex) {
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);

    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 1024;
    int printIntervalRdfCnt = (1 << 5);

    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    int timerRdfId = -1, toGenIfdId = -1, localRequiredIfdId = -1; // shared 
    int chaserRdfIdLowerBound = -1, oldLcacIfdId = -1, newLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    int newChaserRdfId = 0;
    while (loopRdfCnt > outerTimerRdfId) {
        bool shouldPrint = false;
        uint64_t inSingleInput = getSelfCmdByRdfId(testCmds15, outerTimerRdfId);
        bool cmdInjected = FRONTEND_UpsertSelfCmd(reusedBattle, inSingleInput, &newChaserRdfId);
        if (!cmdInjected) {
            std::cerr << "TestCase15/Failed to inject cmd for outerTimerRdfId=" << outerTimerRdfId << ", inSingleInput=" << inSingleInput << std::endl;
            exit(1);
        }
        FRONTEND_ChaseRolledBackRdfs(reusedBattle, &newChaserRdfId, true);
        FRONTEND_Step(reusedBattle);

        RenderFrame* outerTimerRdf = reusedBattle->rdfBuffer.GetByFrameId(outerTimerRdfId);

        auto& tr1 = outerTimerRdf->triggers(0);

        auto& p1 = outerTimerRdf->players(0);
        auto& p1Chd = p1.chd();
        
        auto& npc1 = outerTimerRdf->npcs(0);
        auto& npc1Chd = npc1.chd();

        auto& npc2 = outerTimerRdf->npcs(1);
        auto& npc2Chd = npc2.chd();

        auto& bl1 = outerTimerRdf->bullets(0);

        if (1 == outerTimerRdfId) {
            JPH_ASSERT(3 == tr1.demanded_evt_mask());
            JPH_ASSERT(0 == tr1.fulfilled_evt_mask());
            JPH_ASSERT(1 == npc1.publishing_evt_mask_upon_exhausted());
            JPH_ASSERT(2 == npc2.publishing_evt_mask_upon_exhausted());
            JPH_ASSERT(42 == npc1.publishing_to_trigger_id_upon_exhausted());
        } 

        if (71 == outerTimerRdfId) {
            JPH_ASSERT(CharacterState::Atk1 == p1Chd.ch_state());
            JPH_ASSERT(0 == p1Chd.frames_in_ch_state());
        }

        if (100 == outerTimerRdfId) {
            JPH_ASSERT(CharacterState::Dying == npc1Chd.ch_state());
        }

        if (182 == outerTimerRdfId) {
            auto& prevRdfStepResult = outerTimerRdf->prev_rdf_step_result();
            JPH_ASSERT(0 == prevRdfStepResult.fulfilled_triggers_size());
        }

        if (303 == outerTimerRdfId) {
            auto& prevRdfStepResult = outerTimerRdf->prev_rdf_step_result();
            JPH_ASSERT(1 == prevRdfStepResult.fulfilled_triggers_size());
            auto& prevRdfFulfilledTr1 = prevRdfStepResult.fulfilled_triggers(0);
            JPH_ASSERT(globalPrimitiveConsts->trt_victory() == prevRdfFulfilledTr1.trt());
        }

        outerTimerRdfId++;
    }

    std::cout << "Passed TestCase15: Victory trigger by killing NPC\n" << std::endl;
    reusedBattle->Clear();   

    return true;
}

bool runTestCase16(FrontendBattle* reusedBattle, WsReq* initializerMapData, int inSingleJoinIndex) {
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);
    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 2048;
    int printIntervalRdfCnt = (1 << 4);
    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    jtshared::RenderFrame* outRdf = google::protobuf::Arena::Create<RenderFrame>(&pbTestCaseDataAllocator);
    int newLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    int newChaserRdfId = 0, newReferenceBattleChaserRdfId = 0;
    while (loopRdfCnt > outerTimerRdfId) {
        bool shouldPrint = false;

        // Handling TCP packets first, and then UDP packets, the same as C# side behavior.

        uint64_t inSingleInput = getSelfCmdByRdfId(testCmds16, outerTimerRdfId);
        bool cmdInjected = FRONTEND_UpsertSelfCmd(reusedBattle, inSingleInput, &newChaserRdfId);
        if (!cmdInjected) {
            std::cerr << "Failed to inject cmd for outerTimerRdfId=" << outerTimerRdfId << ", inSingleInput=" << inSingleInput << std::endl;
            exit(1);
        }
        FRONTEND_Step(reusedBattle);
        auto outerTimerRdf = reusedBattle->rdfBuffer.GetByFrameId(outerTimerRdfId);
        auto& p1 = outerTimerRdf->players(0);
        auto& p1Chd = p1.chd();
        auto& tp1 = outerTimerRdf->dynamic_traps(0);

        if (100 < outerTimerRdfId && outerTimerRdfId < 150) {
            JPH_ASSERT(-70 < tp1.x() && tp1.x() < -40);
            JPH_ASSERT(0 > tp1.vel_x() && 0 > tp1.vel_y() && 0 == tp1.vel_z());
            //shouldPrint = true;
        } else if (205 < outerTimerRdfId && outerTimerRdfId <= 210) {
            JPH_ASSERT(-81 < tp1.x() && tp1.x() < -79);
            JPH_ASSERT(0 == tp1.vel_x() && 0 == tp1.vel_y() && 0 == tp1.vel_z());
            //shouldPrint = true;
        } else if (490 < outerTimerRdfId && outerTimerRdfId < 520) {
            JPH_ASSERT(30 < tp1.x() && tp1.x() < 50);
            JPH_ASSERT(0 < tp1.vel_x() && 0 < tp1.vel_y() && 0 == tp1.vel_z());
            //shouldPrint = true;
        } else if (700 < outerTimerRdfId && outerTimerRdfId < 750) {
            JPH_ASSERT(20 < tp1.x() && tp1.x() < 50);
            JPH_ASSERT(0 > tp1.vel_x() && 0 > tp1.vel_y() && 0 == tp1.vel_z());
            //shouldPrint = true;
        }

        if (shouldPrint) {
            std::cout << "TestCase16/outerTimerRdfId=" << outerTimerRdfId << "\n\ttp1 pos=(" << tp1.x() << ", " << tp1.y() << "," << tp1.z() << "), dir=(" << tp1.q_x() << ", " << tp1.q_y() << ", " << tp1.q_z() << ", " << tp1.q_w() << "), vel=(" << tp1.vel_x() << ", " << tp1.vel_y() << ", " << tp1.vel_z() << ")" << std::endl;
        }

        outerTimerRdfId++;
    }

    std::cout << "Passed TestCase16: Skewed Slider Trap\n" << std::endl;
    reusedBattle->Clear();
    return true;
}

bool runTestCase17(FrontendBattle* reusedBattle, WsReq* initializerMapData, int inSingleJoinIndex) {
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);
    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 2048;
    int printIntervalRdfCnt = (1 << 4);
    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    jtshared::RenderFrame* outRdf = google::protobuf::Arena::Create<RenderFrame>(&pbTestCaseDataAllocator);
    int newLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    int newChaserRdfId = 0, newReferenceBattleChaserRdfId = 0;
    while (loopRdfCnt > outerTimerRdfId) {
        bool shouldPrint = false;

        // Handling TCP packets first, and then UDP packets, the same as C# side behavior.
        
        uint64_t inSingleInput = getSelfCmdByRdfId(testCmds17, outerTimerRdfId);
        bool cmdInjected = FRONTEND_UpsertSelfCmd(reusedBattle, inSingleInput, &newChaserRdfId);
        if (!cmdInjected) {
            std::cerr << "Failed to inject cmd for outerTimerRdfId=" << outerTimerRdfId << ", inSingleInput=" << inSingleInput << std::endl;
            exit(1);
        }
        FRONTEND_Step(reusedBattle);
        auto outerTimerRdf = reusedBattle->rdfBuffer.GetByFrameId(outerTimerRdfId);
        auto& p1 = outerTimerRdf->players(0);
        auto& p1Chd = p1.chd();
        auto& tp1 = outerTimerRdf->dynamic_traps(0);

        if (10 < outerTimerRdfId && outerTimerRdfId < 150) {
            JPH_ASSERT(104.5 < tp1.y() && tp1.y() < 105.5);
            JPH_ASSERT(0 > tp1.x() && -300 < tp1.x());
            //shouldPrint = true;
        } else if (150 <= outerTimerRdfId && outerTimerRdfId <= 248) {
            JPH_ASSERT(104.5 < tp1.y() && tp1.y() < 105.5);
            //shouldPrint = true;
        } else if (249 == outerTimerRdfId) {
            JPH_ASSERT(104.5 < tp1.y() && tp1.y() < 105.5);
            //shouldPrint = true;
        } else if (303 < outerTimerRdfId && outerTimerRdfId <= 420) {
            JPH_ASSERT(104.5 < tp1.y() && tp1.y() < 105.5);
            //shouldPrint = true;
        } else {
            JPH_ASSERT(104.5 < tp1.y() && tp1.y() < 105.5);
            //shouldPrint = true;
        }
        
        if (shouldPrint) {
            std::cout << "TestCase17/outerTimerRdfId=" << outerTimerRdfId << "\n\tp1 ch_state=" << (int)p1Chd.ch_state() << ", frames_in_ch_state=" << p1Chd.frames_in_ch_state() << ", pos = (" << p1Chd.x() << ", " << p1Chd.y() << ", " << p1Chd.z() << "), vel = (" << p1Chd.vel_x() << ", "  << p1Chd.vel_y() << ", " << p1Chd.vel_z() << ")\n\ttp1 pos = (" << tp1.x() << ", " << tp1.y() << ", " << tp1.z() << "), dir = (" << tp1.q_x() << ", " << tp1.q_y() << ", " << tp1.q_z() << ", " << tp1.q_w() << "), vel = (" << tp1.vel_x() << ", " << tp1.vel_y() << ", " << tp1.vel_z() << ")" << std::endl;
        }

        outerTimerRdfId++;
    }

    std::cout << "Passed TestCase17: Slider Trap and character interaction\n" << std::endl;
    reusedBattle->Clear();
    return true;
}

bool runTestCase18(FrontendBattle* reusedBattle, WsReq* initializerMapData, int inSingleJoinIndex) {
    bool doCompareWithRollback = true; 

    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);

    FrontendBattle* referenceBattle = static_cast<FrontendBattle*>(FRONTEND_CreateBattle(512, true));
    referenceBattle->SetFrameLogEnabled(true);
    referenceBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);

    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 1024;
    int printIntervalRdfCnt = (1 << 5);

    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    int timerRdfId = -1, toGenIfdId = -1, localRequiredIfdId = -1; // shared 
    int chaserRdfIdLowerBound = -1, oldLcacIfdId = -1, newLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    int newChaserRdfId = 0, newReferenceBattleChaserRdfId = 0;
    int referenceBattleChaserRdfId = -1, referenceBattleChaserRdfIdLowerBound = -1, referenceBattleOldLcacIfdId = -1, referenceBattleNewLcacIfdId = -1, referenceBattleMaxPlayerInputFrontId = 0, referenceBattleMinPlayerInputFrontId = 0;
    jtshared::RenderFrame* outRdf = google::protobuf::Arena::Create<RenderFrame>(&pbTestCaseDataAllocator);
    jtshared::RenderFrame* referenceBattleOutRdf = google::protobuf::Arena::Create<RenderFrame>(&pbTestCaseDataAllocator);
    while (loopRdfCnt > outerTimerRdfId) {
        if (incomingUpsyncSnapshotReqs18Intime.count(outerTimerRdfId)) {
            auto req = incomingUpsyncSnapshotReqs18Intime[outerTimerRdfId];
            auto peerUpsyncSnapshot = req->upsync_snapshot();
            referenceBattle->OnUpsyncSnapshotReceived(req->join_index(), peerUpsyncSnapshot, &newReferenceBattleChaserRdfId, &referenceBattleMaxPlayerInputFrontId, &referenceBattleMinPlayerInputFrontId);
        }

        if (doCompareWithRollback) {
            if (incomingUpsyncSnapshotReqs18Rollback.count(outerTimerRdfId)) {
                auto req = incomingUpsyncSnapshotReqs18Rollback[outerTimerRdfId];
                auto peerUpsyncSnapshot = req->upsync_snapshot();
                reusedBattle->OnUpsyncSnapshotReceived(req->join_index(), peerUpsyncSnapshot, &newChaserRdfId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
            }
        }

        uint64_t inSingleInput = getSelfCmdByRdfId(testCmds18, outerTimerRdfId);
        bool referenceBattleCmdInjected = FRONTEND_UpsertSelfCmd(referenceBattle, inSingleInput, &newReferenceBattleChaserRdfId);
        if (!referenceBattleCmdInjected) {
            std::cerr << "Failed to inject cmd to referenceBattle for outerTimerRdfId=" << outerTimerRdfId << ", inSingleInput=" << inSingleInput << std::endl;
            exit(1);
        }
        int referenceBattleChaserRdfIdEd = outerTimerRdfId;
        FRONTEND_ChaseRolledBackRdfs(referenceBattle, &newReferenceBattleChaserRdfId, true);
        FRONTEND_Step(referenceBattle);
        
        if (doCompareWithRollback) {
            bool cmdInjected = FRONTEND_UpsertSelfCmd(reusedBattle, inSingleInput, &newChaserRdfId);
            if (!cmdInjected) {
                std::cerr << "Failed to inject cmd for outerTimerRdfId=" << outerTimerRdfId << ", inSingleInput=" << inSingleInput << std::endl;
                exit(1);
            }

            bool shouldPrint = false;
            int chaserRdfIdEd = outerTimerRdfId;

            FRONTEND_ChaseRolledBackRdfs(reusedBattle, &newChaserRdfId, true);
            FRONTEND_Step(reusedBattle);

            auto referencedRdf = referenceBattle->rdfBuffer.GetByFrameId(outerTimerRdfId);
            auto challengingRdf = reusedBattle->rdfBuffer.GetByFrameId(outerTimerRdfId);

            const PlayerCharacterDownsync& referencedP1 = referencedRdf->players(0);
            const CharacterDownsync& p1Chd = referencedP1.chd();
            const PlayerCharacterDownsync& referencedP2 = referencedRdf->players(1);
            const CharacterDownsync& p2Chd = referencedP2.chd();
            const NpcCharacterDownsync& referencedNpc1 = referencedRdf->npcs(0);
            const CharacterDownsync& npc1Chd = referencedNpc1.chd();
            const Trap& referencedTrap1 = referencedRdf->dynamic_traps(0);
            const Trap& referencedTrap2 = referencedRdf->dynamic_traps(1);

            if (loopRdfCnt < outerTimerRdfId + 10) {
                //shouldPrint = true;
                BaseBattle::AssertNearlySame(referencedRdf, challengingRdf);
            } else if (280 == outerTimerRdfId) {
                int firstToBeConsistentRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(0);
                int lastToBeConsistentRdfId = BaseBattle::ConvertToLastUsedRenderFrameId(4) + 1;
                for (int recRdfId = firstToBeConsistentRdfId; recRdfId <= lastToBeConsistentRdfId; recRdfId++) {
                    auto referencedRdf = referenceBattle->rdfBuffer.GetByFrameId(recRdfId);
                    auto challengingRdf = reusedBattle->rdfBuffer.GetByFrameId(recRdfId);
                    BaseBattle::AssertNearlySame(referencedRdf, challengingRdf);
                }
                shouldPrint = true;
            } else if (400 == outerTimerRdfId) {
                int firstToBeConsistentRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(4);
                int lastToBeConsistentRdfId = BaseBattle::ConvertToLastUsedRenderFrameId(32) + 1;
                for (int recRdfId = firstToBeConsistentRdfId; recRdfId <= lastToBeConsistentRdfId; recRdfId++) {
                    auto referencedRdf = referenceBattle->rdfBuffer.GetByFrameId(recRdfId);
                    auto challengingRdf = reusedBattle->rdfBuffer.GetByFrameId(recRdfId);
                    BaseBattle::AssertNearlySame(referencedRdf, challengingRdf);
                }
                shouldPrint = true;
            } else if (480 == outerTimerRdfId) {
                int firstToBeConsistentRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(17);
                int lastToBeConsistentRdfId = BaseBattle::ConvertToLastUsedRenderFrameId(60) + 1;
                for (int recRdfId = firstToBeConsistentRdfId; recRdfId <= lastToBeConsistentRdfId; recRdfId++) {
                    auto referencedRdf = referenceBattle->rdfBuffer.GetByFrameId(recRdfId);
                    auto challengingRdf = reusedBattle->rdfBuffer.GetByFrameId(recRdfId);
                    BaseBattle::AssertNearlySame(referencedRdf, challengingRdf);
                }
                shouldPrint = true;
            }
        
            if (shouldPrint) {
                std::cout << "TestCase18/outerTimerRdfId=" << outerTimerRdfId << "\n\tp1Chd hp=" << p1Chd.hp() << ", chState=" << p1Chd.ch_state() << ", framesInChState=" << p1Chd.frames_in_ch_state() << ", dir=(" << p1Chd.q_x() << ", " << p1Chd.q_y() << ", " << p1Chd.q_z() << ", " << p1Chd.q_w() << "), pos=(" << p1Chd.x() << ", " << p1Chd.y() << "), vel=(" << p1Chd.vel_x() << ", " << p1Chd.vel_y() << ")\n\tp2Chd hp=" << p2Chd.hp() << ", chState=" << p2Chd.ch_state() << ", framesInChState=" << p2Chd.frames_in_ch_state() << ", dir=(" << p2Chd.q_x() << ", " << p2Chd.q_y() << ", " << p2Chd.q_z() << ", " << p2Chd.q_w() << "), pos=(" << p2Chd.x() << ", " << p2Chd.y() << "), vel=(" << p2Chd.vel_x() << ", " << p2Chd.vel_y() << ")\n\tnpc1Chd hp=" << npc1Chd.hp() << ", chState=" << npc1Chd.ch_state() << ", framesInChState=" << npc1Chd.frames_in_ch_state() << ", dir=(" << npc1Chd.q_x() << ", " << npc1Chd.q_y() << ", " << npc1Chd.q_z() << ", " << npc1Chd.q_w() << "), pos=(" << npc1Chd.x() << ", " << npc1Chd.y() << "), vel=(" << npc1Chd.vel_x() << ", " << npc1Chd.vel_y() << ")\n\ttrap1 pos=(" << referencedTrap1.x() << ", " << referencedTrap1.vel_y() << "), vel=(" << referencedTrap1.vel_x() << ", " << referencedTrap1.y() << ")\n\ttrap2 pos=(" << referencedTrap2.x() << ", " << referencedTrap2.y() << "), vel=(" << referencedTrap2.vel_x() << ", " << referencedTrap2.vel_y() << ")" << std::endl;
            }
        }
        outerTimerRdfId++;
    }

    std::cout << "Passed TestCase18: Rollback-chasing with dynamic traps\n" << std::endl;
    reusedBattle->Clear();   
    APP_DestroyBattle(referenceBattle);
    return true;
}

// Program entry point
int main(int argc, char** argv)
{
#ifndef NDEBUG
    std::cout << "Starting in debug mode" << std::endl;
#else
    std::cout << "Starting in release" << std::endl;
#endif
    path exePath(argv[0]);

    // Get the parent path (the directory containing the executable)
    path executableFolder = exePath.parent_path();
    std::ifstream primitiveConstsFin(executableFolder.string() + "/PrimitiveConsts.pb", std::ios::in | std::ios::binary);
    if (!primitiveConstsFin.is_open()) {
        std::cerr << "Failed to open PrimitiveConsts.pb" << std::endl;
        exit(1);
    }
    memset(pbByteBuffer, 0, sizeof(pbByteBuffer));
    primitiveConstsFin.read(pbByteBuffer, pbBufferSizeLimit);
    size_t bytesRead = primitiveConstsFin.gcount(); // Get actual bytes read
    PrimitiveConsts_Init(pbByteBuffer, bytesRead);
    primitiveConstsFin.close();

    std::ifstream configConstsFin(executableFolder.string() + "/ConfigConsts.pb", std::ios::in | std::ios::binary);
    if (!configConstsFin.is_open()) {
        std::cerr << "Failed to open ConfigConsts.pb" << std::endl;
        exit(1);
    }
    memset(pbByteBuffer, 0, sizeof(pbByteBuffer));
    configConstsFin.read(pbByteBuffer, pbBufferSizeLimit);
    bytesRead = configConstsFin.gcount(); // Get actual bytes read
    ConfigConsts_Init(pbByteBuffer, bytesRead);
#ifndef NDEBUG
    auto skillConfigs = globalConfigConsts->skill_configs();
    std::ostringstream oss;
    oss << "skillConfig keys: [ ";
    for (const auto& pair : skillConfigs) {
        oss << pair.first << " ";
    }
    oss << "]";
    std::cout << oss.str() << std::endl;
#endif
    primitiveConstsFin.close();

    // Kindly note that "hull1" goes clockwise, and counter-clockwise DOESN'T WORK to support a character (i.e. OnContactAdded/OnContactPersisted)! However, using "JPH::CharacterVirtual" works and I don't know why yet...
    std::vector<float> hull1 = {
        -100, 0,
        -100, 100,
        100, 100,
        100, 0
    };

    std::vector<float> hull2 = {
        -200, 0,
        -200, 1000,
        -100, 1000,
        -100, 0
    };

    std::vector<float> hull3 = {
        100, 0,
        100, 1000,
        200, 1000,
        200, 0
    };

    std::vector<float> npcVisionHull1 = {
        // Lower floor
        -500, 0,
        -500, 100,
        500, 100,
        500, 0
    };

    std::vector<float> npcVisionHull2 = {
        // Left pillar
        -800, 0,
        -800, 1000,
        -500, 1000,
        -500, 0
    };

    std::vector<float> npcVisionHull3 = {
        // Right pillar
        500, 1000,
        800, 1000,
        800, 0,
        500, 0,
    };

    std::vector<float> npcVisionHull4 = {
        // Lower floor small platform
        -50, 0,
        -50, 140,
        +50, 140,
        +50, 0
    };

    std::vector<float> npcVisionHull5 = {
        // Upper floor, only a right wing is provided to hold npc2, intentionally overlapping with "npcVisionHull3" to test edge cases
        -200, 400,
        -200, 500,
        750, 500,
        750, 400
    };

    std::vector<float> npcVisionHull6 = {
        // Upper floor small platform that's jumpable, intentionally overlapping with "npcVisionHull7" to test edge cases
        -200, 500,
        -200, 520,
        300, 520,
        300, 500
    };

    std::vector<float> npcVisionHull7 = {
        // Upper floor small platform at its left edge that's not jumpable
        -200, 500,
        -200, 600,
        100, 600,
        100, 500
    };

    std::vector<float> wideMapHull1 = {
        // Lower floor
        -500, 0,
        -500, 100,
        500, 100,
        500, 0
    };

    std::vector<float> wideMapHull2 = {
        // Left pillar
        -800, 0,
        -800, 1000,
        -500, 1000,
        -500, 0
    };

    std::vector<float> wideMapHull3 = {
        // Right pillar
        500, 1000,
        800, 1000,
        800, 0,
        500, 0,
    };

    std::vector<float> wideMapHull4 = {
        // Upper-Left  pillar
        -300, 100,
        -300, 300,
        -320, 300,
        -320, 100
    };
    
    std::vector<std::vector<float>> hulls = {hull1, hull2, hull3};
    std::vector<std::vector<float>> fallenDeathHulls = {hull1, hull2};
    std::vector<std::vector<float>> npcVisionHulls = {npcVisionHull1, npcVisionHull2, npcVisionHull3, npcVisionHull4, npcVisionHull5, npcVisionHull6, npcVisionHull7};
    std::vector<std::vector<float>> wideMapHulls = {wideMapHull1, wideMapHull2, wideMapHull3, wideMapHull4};

    JPH_Init(10*1024*1024);
    std::cout << "Initiated" << std::endl;
    
    RegisterDebugCallback(DebugLogCb);

    FrontendBattle* battle = static_cast<FrontendBattle*>(FRONTEND_CreateBattle(512, true));
    std::cout << "Created battle = " << battle << std::endl;

    google::protobuf::Arena pbStarterWsReqAllocator;
    auto startRdf = mockStartRdf();
    WsReq* initializerMapData = google::protobuf::Arena::Create<WsReq>(&pbStarterWsReqAllocator);
    for (auto hull : hulls) {
        auto srcBarrier = initializerMapData->add_serialized_barriers();
        auto srcPolygon = srcBarrier->mutable_polygon();
        float anchorX = 0, anchorY = 0;
        for (int i = 0; i < hull.size(); i += 2) {
            PbVec2* newPt = srcPolygon->add_points();
            newPt->set_x(hull[i]);
            newPt->set_y(hull[i + 1]);
            anchorX += hull[i];
            anchorY += hull[i + 1];
        }
        anchorX /= (hull.size() >> 1);
        anchorY /= (hull.size() >> 1);
        auto anchor = srcPolygon->mutable_anchor();
        anchor->set_x(anchorX);
        anchor->set_y(anchorY);
    }
    initializerMapData->set_allocated_self_parsed_rdf(startRdf); // "initializerMapData" will own "startRdf" and deallocate it implicitly

    int selfJoinIndex = 1;
    
    initTest1Data();
    runTestCase1(battle, initializerMapData, selfJoinIndex);
    pbTestCaseDataAllocator.Reset();
    
    initTest2Data();
    runTestCase2(battle, initializerMapData, selfJoinIndex);
    pbTestCaseDataAllocator.Reset();
     
    initTest3Data();
    runTestCase3(battle, initializerMapData, selfJoinIndex);

    initTest4Data();
    runTestCase4(battle, initializerMapData, selfJoinIndex);
    
    initTest5Data();
    runTestCase5(battle, initializerMapData, selfJoinIndex);

    initTest6Data();
    runTestCase6(battle, initializerMapData, selfJoinIndex);

    initTest7Data();
    runTestCase7(battle, initializerMapData, selfJoinIndex);
    
    WsReq* blacksaber1VisionTestInitializerMapData = google::protobuf::Arena::Create<WsReq>(&pbStarterWsReqAllocator);
    initTest8Data(blacksaber1VisionTestInitializerMapData, npcVisionHulls);
    runTestCase8(battle, blacksaber1VisionTestInitializerMapData, selfJoinIndex);
    pbTestCaseDataAllocator.Reset();
    
    WsReq* sliderTrapTestInitializerMapData = google::protobuf::Arena::Create<WsReq>(&pbStarterWsReqAllocator);    
    initTest9Data(sliderTrapTestInitializerMapData, hulls);
    runTestCase9(battle, sliderTrapTestInitializerMapData, selfJoinIndex);
    pbTestCaseDataAllocator.Reset();
    
    initTest10Data();
    runTestCase10(battle, initializerMapData, selfJoinIndex);
    pbTestCaseDataAllocator.Reset();
    
    WsReq* rollbackChasingAlignTestInitializerMapData = google::protobuf::Arena::Create<WsReq>(&pbStarterWsReqAllocator);
    initTest11Data(rollbackChasingAlignTestInitializerMapData, hulls);
    runTestCase11(battle, rollbackChasingAlignTestInitializerMapData, selfJoinIndex);
    pbTestCaseDataAllocator.Reset();

    WsReq* fallenDeathInitializerMapData = google::protobuf::Arena::Create<WsReq>(&pbStarterWsReqAllocator);
    initTest12Data(fallenDeathInitializerMapData, fallenDeathHulls);
    runTestCase12(battle, fallenDeathInitializerMapData, selfJoinIndex);
    pbTestCaseDataAllocator.Reset();
         
    WsReq* bladeGirlSkillInitializerMapData = google::protobuf::Arena::Create<WsReq>(&pbStarterWsReqAllocator); 
    initTest13Data(bladeGirlSkillInitializerMapData, hulls);
    runTestCase13(battle, bladeGirlSkillInitializerMapData, selfJoinIndex);
    pbTestCaseDataAllocator.Reset();
     
    WsReq* bountyHunterSkillInitializerMapData = google::protobuf::Arena::Create<WsReq>(&pbStarterWsReqAllocator);
    initTest14Data(bountyHunterSkillInitializerMapData, hulls);
    runTestCase14(battle, bountyHunterSkillInitializerMapData, selfJoinIndex);
    pbTestCaseDataAllocator.Reset();
    
    WsReq* victoryTriggerInitializerMapData = google::protobuf::Arena::Create<WsReq>(&pbStarterWsReqAllocator);
    initTest15Data(victoryTriggerInitializerMapData, hulls);
    runTestCase15(battle, victoryTriggerInitializerMapData, selfJoinIndex);
    pbTestCaseDataAllocator.Reset();
    
    WsReq* sliderTrapTestInitializerMapData2 = google::protobuf::Arena::Create<WsReq>(&pbStarterWsReqAllocator);
    initTest16Data(sliderTrapTestInitializerMapData2, hulls);
    runTestCase16(battle, sliderTrapTestInitializerMapData2, selfJoinIndex);
    pbTestCaseDataAllocator.Reset();
     
    WsReq* sliderTrapTestInitializerMapData3 = google::protobuf::Arena::Create<WsReq>(&pbStarterWsReqAllocator);
    initTest17Data(sliderTrapTestInitializerMapData3, wideMapHulls);
    runTestCase17(battle, sliderTrapTestInitializerMapData3, selfJoinIndex);
    pbTestCaseDataAllocator.Reset();
    
    WsReq* sliderTrapTestInitializerMapData4 = google::protobuf::Arena::Create<WsReq>(&pbStarterWsReqAllocator);
    initTest18Data(sliderTrapTestInitializerMapData4, wideMapHulls);
    runTestCase18(battle, sliderTrapTestInitializerMapData4, selfJoinIndex);
    pbTestCaseDataAllocator.Reset();
     
    pbStarterWsReqAllocator.Reset();

    // clean up
    // [REMINDER] "startRdf" and "fallenDeathStartRdf" will be automatically deallocated by the destructor of "wsReq"
    bool destroyRes = APP_DestroyBattle(battle);
    std::cout << "APP_DestroyBattle result=" << destroyRes << std::endl;
    JPH_Shutdown();
	return 0;
}

#ifdef _WIN32
#include <windows.h>

int APIENTRY WinMain(HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow)
{
    return main(__argc, __argv);
}
#endif
