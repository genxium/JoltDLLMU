#include "TestHelper.h"
#include "DebugLog.h"

#include "FrontendBattle.h"
#include <chrono>
#include <fstream>
#include <filesystem>

#include <google/protobuf/repeated_field.h>

using namespace jtshared;
using namespace std::filesystem;

const int pbBufferSizeLimit = (1 << 14);
char pbByteBuffer[pbBufferSizeLimit];
char rdfFetchBuffer[pbBufferSizeLimit];
char stepResultFetchBuffer[pbBufferSizeLimit];
char upsyncSnapshotBuffer[pbBufferSizeLimit];
const char* const selfPlayerId = "foobar";
int selfCmdAuthKey = 123456;

RenderFrame* mockStartRdf(google::protobuf::Arena* theAllocator) {
    auto chSpecies = globalPrimitiveConsts->ch_species();
    const int roomCapacity = 2;
    auto* startRdf = TestHelper::NewPreallocatedRdf(roomCapacity, 8, 8, theAllocator);
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

RenderFrame* mockBlacksaber1VisionTestStartRdf(google::protobuf::Arena* theAllocator) {
    auto chSpecies = globalPrimitiveConsts->ch_species();
    const int roomCapacity = 1;
    auto* startRdf = TestHelper::NewPreallocatedRdf(roomCapacity, 8, 8, theAllocator);
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

RenderFrame* mockSliderTrapTestStartRdf1(google::protobuf::Arena* theAllocator) {
    auto chSpecies = globalPrimitiveConsts->ch_species();
    const int roomCapacity = 2;
    auto* startRdf = TestHelper::NewPreallocatedRdf(roomCapacity, 8, 8, theAllocator);
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
    dynamicTrap1->set_tpt(globalPrimitiveConsts->tpts().sliding_platform());
    ++dynamicTrapCount;
    
    startRdf->set_npc_id_counter(npcIdCounter);
    startRdf->set_npc_count(npcIdCounter-1);

    startRdf->set_bullet_id_counter(bulletIdCounter);
    startRdf->set_pickable_id_counter(pickableIdCounter);

    startRdf->set_dynamic_trap_count(dynamicTrapCount);

    return startRdf;
}

RenderFrame* mockSliderTrapTestStartRdf2(google::protobuf::Arena* theAllocator) {
    auto chSpecies = globalPrimitiveConsts->ch_species();
    const int roomCapacity = 2;
    auto* startRdf = TestHelper::NewPreallocatedRdf(roomCapacity, 8, 8, theAllocator);
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
    dynamicTrap1->set_tpt(globalPrimitiveConsts->tpts().sliding_platform());
    ++dynamicTrapCount;

    startRdf->set_npc_id_counter(npcIdCounter);
    startRdf->set_npc_count(npcIdCounter - 1);

    startRdf->set_bullet_id_counter(bulletIdCounter);
    startRdf->set_pickable_id_counter(pickableIdCounter);

    startRdf->set_dynamic_trap_count(dynamicTrapCount);

    return startRdf;
}

RenderFrame* mockSliderTrapTestStartRdf3(google::protobuf::Arena* theAllocator) {
    auto chSpecies = globalPrimitiveConsts->ch_species();
    const int roomCapacity = 2;
    auto* startRdf = TestHelper::NewPreallocatedRdf(roomCapacity, 8, 8, theAllocator);
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
    playerCh1->set_x(0);
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
    dynamicTrap1->set_tpt(globalPrimitiveConsts->tpts().sliding_platform());
    ++dynamicTrapCount;

    auto dynamicTrap2 = startRdf->add_dynamic_traps();
    dynamicTrap2->set_id(43);
    dynamicTrap2->set_tpt(globalPrimitiveConsts->tpts().sliding_platform());
    ++dynamicTrapCount;

    startRdf->set_npc_id_counter(npcIdCounter);
    startRdf->set_npc_count(npcIdCounter-1);

    startRdf->set_bullet_id_counter(bulletIdCounter);
    startRdf->set_pickable_id_counter(pickableIdCounter);

    startRdf->set_dynamic_trap_count(dynamicTrapCount);

    return startRdf;
}

RenderFrame* mockRollbackChasingAlignTestStartRdf(google::protobuf::Arena* theAllocator) {
    auto chSpecies = globalPrimitiveConsts->ch_species();
    const int roomCapacity = 2;
    auto* startRdf = TestHelper::NewPreallocatedRdf(roomCapacity, 8, 8, theAllocator);
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

RenderFrame* mockRollbackChasingAlignTestStartRdf2(google::protobuf::Arena* theAllocator) {
    auto chSpecies = globalPrimitiveConsts->ch_species();
    const int roomCapacity = 2;
    auto* startRdf = TestHelper::NewPreallocatedRdf(roomCapacity, 8, 8, theAllocator);
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
    dynamicTrap1->set_tpt(globalPrimitiveConsts->tpts().sliding_platform());
    ++dynamicTrapCount;

    auto dynamicTrap2 = startRdf->add_dynamic_traps();
    dynamicTrap2->set_id(43);
    dynamicTrap2->set_tpt(globalPrimitiveConsts->tpts().sliding_platform());
    ++dynamicTrapCount;

    startRdf->set_dynamic_trap_count(dynamicTrapCount);

    return startRdf;
}

RenderFrame* mockFallenDeathRdf(google::protobuf::Arena* theAllocator) {
    auto chSpecies = globalPrimitiveConsts->ch_species();
    const int roomCapacity = 2;
    auto* startRdf = TestHelper::NewPreallocatedRdf(roomCapacity, 8, 8, theAllocator);
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

RenderFrame* mockBladeGirlSkillRdf(google::protobuf::Arena* theAllocator) {
    auto chSpecies = globalPrimitiveConsts->ch_species();
    const int roomCapacity = 2;
    auto* startRdf = TestHelper::NewPreallocatedRdf(roomCapacity, 8, 8, theAllocator);
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

RenderFrame* mockBountyHunterSkillRdf(google::protobuf::Arena* theAllocator) {
    auto chSpecies = globalPrimitiveConsts->ch_species();
    const int roomCapacity = 2;
    auto* startRdf = TestHelper::NewPreallocatedRdf(roomCapacity, 8, 8, theAllocator);
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
    npcCh1->set_x(+100);
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

RenderFrame* mockVictoryRdf(google::protobuf::Arena* theAllocator) {
    auto chSpecies = globalPrimitiveConsts->ch_species();
    const int roomCapacity = 1;
    auto* startRdf = TestHelper::NewPreallocatedRdf(roomCapacity, 8, 8, theAllocator);
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
    tr1->set_trt(globalPrimitiveConsts->trts().victory());
    ++triggerCount;
    
    startRdf->set_npc_id_counter(npcIdCounter);
    startRdf->set_npc_count(npcIdCounter-1);

    startRdf->set_bullet_id_counter(bulletIdCounter);
    startRdf->set_pickable_id_counter(pickableIdCounter);

    startRdf->set_trigger_count(triggerCount);

    return startRdf;
}

RenderFrame* mockSlipJumpStartRdf(google::protobuf::Arena* theAllocator) {
    auto chSpecies = globalPrimitiveConsts->ch_species();
    const int roomCapacity = 1;
    auto* startRdf = TestHelper::NewPreallocatedRdf(roomCapacity, 8, 8, theAllocator);
    startRdf->set_id(globalPrimitiveConsts->starting_render_frame_id());
    uint32_t pickableIdCounter = 1;
    uint32_t npcIdCounter = 1;
    uint32_t bulletIdCounter = 1;

    auto characterConfigs = globalConfigConsts->character_configs();

    auto player1 = startRdf->mutable_players(0);
    auto playerCh1 = player1->mutable_chd();
    auto playerCh1Species = chSpecies.bountyhunter();
    auto cc1 = characterConfigs[playerCh1Species];
    playerCh1->set_x(-60);
    playerCh1->set_y(116);
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

    startRdf->set_npc_id_counter(npcIdCounter);
    startRdf->set_npc_count(npcIdCounter-1);

    startRdf->set_bullet_id_counter(bulletIdCounter);
    startRdf->set_pickable_id_counter(pickableIdCounter);

    return startRdf;
}

RenderFrame* mockNpcSpawnerRdf(google::protobuf::Arena* theAllocator) {
    auto chSpecies = globalPrimitiveConsts->ch_species();
    const int roomCapacity = 1;
    auto* startRdf = TestHelper::NewPreallocatedRdf(roomCapacity, 8, 8, theAllocator);
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

    uint32_t movementTriggerId = 42;

    auto tr1 = startRdf->add_triggers();
    tr1->set_id(movementTriggerId);
    tr1->set_trt(globalPrimitiveConsts->trts().by_movement());
    tr1->set_x(+0);
    tr1->set_y(116);
    ++triggerCount;

    uint32_t npcSpawnerTriggerId = 43;

    auto tr2 = startRdf->add_triggers();
    tr2->set_id(npcSpawnerTriggerId);
    tr2->set_trt(globalPrimitiveConsts->trts().indi_wave_npc_spawner());
    tr2->set_x(+90);
    tr2->set_y(116);
    tr2->set_z(0);
    tr2->set_state(TriggerState::TrReady);
    ++triggerCount;

    uint32_t victoryTriggerId = 44;
    auto tr3 = startRdf->add_triggers();
    tr3->set_id(victoryTriggerId);
    tr3->set_trt(globalPrimitiveConsts->trts().victory());
    ++triggerCount;
    
    startRdf->set_npc_id_counter(npcIdCounter);
    startRdf->set_npc_count(npcIdCounter-1);

    startRdf->set_bullet_id_counter(bulletIdCounter);
    startRdf->set_pickable_id_counter(pickableIdCounter);

    startRdf->set_trigger_count(triggerCount);

    return startRdf;
}

RenderFrame* mockAimingRayTestRdf(google::protobuf::Arena* theAllocator) {
    auto chSpecies = globalPrimitiveConsts->ch_species();
    const int roomCapacity = 1;
    auto* startRdf = TestHelper::NewPreallocatedRdf(roomCapacity, 8, 8, theAllocator);
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

    uint32_t movementTriggerId = 42;

    auto tr1 = startRdf->add_triggers();
    tr1->set_id(movementTriggerId);
    tr1->set_trt(globalPrimitiveConsts->trts().by_movement());
    tr1->set_x(+0);
    tr1->set_y(116);
    ++triggerCount;

    startRdf->set_npc_id_counter(npcIdCounter);
    startRdf->set_npc_count(npcIdCounter - 1);

    startRdf->set_bullet_id_counter(bulletIdCounter);
    startRdf->set_pickable_id_counter(pickableIdCounter);

    startRdf->set_trigger_count(triggerCount);

    return startRdf;
}

RenderFrame* mockBulletCollisionTestStartRdf(google::protobuf::Arena* theAllocator) {
    auto chSpecies = globalPrimitiveConsts->ch_species();
    const int roomCapacity = 2;
    auto* startRdf = TestHelper::NewPreallocatedRdf(roomCapacity, 8, 8, theAllocator);
    startRdf->set_id(globalPrimitiveConsts->starting_render_frame_id());
    uint32_t pickableIdCounter = 1;
    uint32_t npcIdCounter = 1;
    uint32_t bulletIdCounter = 1;

    auto characterConfigs = globalConfigConsts->character_configs();

    auto player1 = startRdf->mutable_players(0);
    auto playerCh1 = player1->mutable_chd();
    auto playerCh1Species = chSpecies.bountyhunter();
    auto cc1 = characterConfigs[playerCh1Species];
    playerCh1->set_x(-200);
    playerCh1->set_y(105);
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
    playerCh2->set_x(+200);
    playerCh2->set_y(105);
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

RenderFrame* mockPistolMagazineTestStartRdf(google::protobuf::Arena* theAllocator) {
    auto chSpecies = globalPrimitiveConsts->ch_species();
    const int roomCapacity = 2;
    auto* startRdf = TestHelper::NewPreallocatedRdf(roomCapacity, 8, 8, theAllocator);
    startRdf->set_id(globalPrimitiveConsts->starting_render_frame_id());
    uint32_t pickableIdCounter = 1;
    uint32_t npcIdCounter = 1;
    uint32_t bulletIdCounter = 1;

    auto characterConfigs = globalConfigConsts->character_configs();

    auto player1 = startRdf->mutable_players(0);
    auto playerCh1 = player1->mutable_chd();
    auto playerCh1Species = chSpecies.bountyhunter();
    auto cc1 = characterConfigs[playerCh1Species];
    playerCh1->set_x(-16);
    playerCh1->set_y(105);
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
    auto playerCh2Species = chSpecies.blacksaber1();
    auto cc2 = characterConfigs[playerCh2Species];
    playerCh2->set_x(+16);
    playerCh2->set_y(105);
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

RenderFrame* mockConveyorBeltTestStartRdf(google::protobuf::Arena* theAllocator) {
    auto chSpecies = globalPrimitiveConsts->ch_species();
    const int roomCapacity = 1;
    auto* startRdf = TestHelper::NewPreallocatedRdf(roomCapacity, 8, 8, theAllocator);
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
    playerCh1->set_x(0);
    playerCh1->set_y(640);
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

    auto dynamicTrap1 = startRdf->add_dynamic_traps();
    dynamicTrap1->set_id(42);
    dynamicTrap1->set_tpt(globalPrimitiveConsts->tpts().conveyor_belt());
    ++dynamicTrapCount;

    auto dynamicTrap2 = startRdf->add_dynamic_traps();
    dynamicTrap2->set_id(43);
    dynamicTrap2->set_tpt(globalPrimitiveConsts->tpts().rotating_platform());
    ++dynamicTrapCount;

    startRdf->set_npc_id_counter(npcIdCounter);
    startRdf->set_npc_count(npcIdCounter - 1);

    startRdf->set_bullet_id_counter(bulletIdCounter);
    startRdf->set_pickable_id_counter(pickableIdCounter);

    startRdf->set_dynamic_trap_count(dynamicTrapCount);

    return startRdf;
}

RenderFrame* mockCrouchTestStartRdf(google::protobuf::Arena* theAllocator) {
    auto chSpecies = globalPrimitiveConsts->ch_species();
    const int roomCapacity = 1;
    auto* startRdf = TestHelper::NewPreallocatedRdf(roomCapacity, 8, 8, theAllocator);
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
    playerCh1->set_x(784);
    playerCh1->set_y(160);
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
    npc1->set_goal_as_npc(NpcGoal::NPatrol);
    auto npcCh1 = npc1->mutable_chd();
    auto npcCh1Species = chSpecies.blacksaber_test_with_vision();
    auto npcCc1 = characterConfigs[npcCh1Species];
    npcCh1->set_x(+900);
    npcCh1->set_y(320);
    npcCh1->set_speed(npcCc1.speed());
    npcCh1->set_ch_state(CharacterState::InAirIdle1NoJump);
    npcCh1->set_frames_to_recover(0);
    npcCh1->set_q_x(0);
    npcCh1->set_q_y(1);
    npcCh1->set_q_z(0);
    npcCh1->set_q_w(0);
    npcCh1->set_aiming_q_x(0);
    npcCh1->set_aiming_q_y(0);
    npcCh1->set_aiming_q_z(0);
    npcCh1->set_aiming_q_w(1);
    npcCh1->set_vel_x(0);
    npcCh1->set_vel_y(0);
    npcCh1->set_hp(npcCc1.hp());
    npcCh1->set_species_id(npcCh1Species);
    npcCh1->set_bullet_team_id(3);

    auto dynamicTrap1 = startRdf->add_dynamic_traps();
    dynamicTrap1->set_id(42);
    dynamicTrap1->set_tpt(globalPrimitiveConsts->tpts().sliding_platform());
    ++dynamicTrapCount;

    startRdf->set_npc_id_counter(npcIdCounter);
    startRdf->set_npc_count(npcIdCounter - 1);

    startRdf->set_bullet_id_counter(bulletIdCounter);
    startRdf->set_pickable_id_counter(pickableIdCounter);

    startRdf->set_dynamic_trap_count(dynamicTrapCount);

    return startRdf;
}

RenderFrame* mockBulletHitReactionTestStartRdf(google::protobuf::Arena* theAllocator) {
    auto chSpecies = globalPrimitiveConsts->ch_species();
    const int roomCapacity = 1;
    auto* startRdf = TestHelper::NewPreallocatedRdf(roomCapacity, 8, 8, theAllocator);
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
    npc1->set_goal_as_npc(NpcGoal::NIdleIfGoHuntingThenPatrol);
    auto npcCh1 = npc1->mutable_chd();
    auto npcCh1Species = chSpecies.blacksaber1();
    auto npcCc1 = characterConfigs[npcCh1Species];
    npcCh1->set_x(+80);
    npcCh1->set_y(100);
    npcCh1->set_speed(npcCc1.speed());
    npcCh1->set_ch_state(CharacterState::InAirIdle1NoJump);
    npcCh1->set_frames_to_recover(0);
    npcCh1->set_q_x(0);
    npcCh1->set_q_y(0);
    npcCh1->set_q_z(0);
    npcCh1->set_q_w(1);
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

RenderFrame* mockBlackShooterTestStartRdf(google::protobuf::Arena* theAllocator) {
    auto chSpecies = globalPrimitiveConsts->ch_species();
    const int roomCapacity = 1;
    auto* startRdf = TestHelper::NewPreallocatedRdf(roomCapacity, 8, 8, theAllocator);
    startRdf->set_id(globalPrimitiveConsts->starting_render_frame_id());
    uint32_t pickableIdCounter = 1;
    uint32_t npcIdCounter = 1;
    uint32_t bulletIdCounter = 1;

    auto characterConfigs = globalConfigConsts->character_configs();

    auto player1 = startRdf->mutable_players(0);
    auto playerCh1 = player1->mutable_chd();
    auto playerCh1Species = chSpecies.bountyhunter();
    auto cc1 = characterConfigs[playerCh1Species];
    playerCh1->set_x(320);
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
    npc1->set_goal_as_npc(NpcGoal::NPatrol);
    auto npcCh1 = npc1->mutable_chd();
    auto npcCh1Species = chSpecies.blackshooter1();
    auto npcCc1 = characterConfigs[npcCh1Species];
    npcCh1->set_x(430);
    npcCh1->set_y(100);
    npcCh1->set_speed(npcCc1.speed());
    npcCh1->set_ch_state(CharacterState::InAirIdle1NoJump);
    npcCh1->set_frames_to_recover(0);
    npcCh1->set_q_x(0);
    npcCh1->set_q_y(1);
    npcCh1->set_q_z(0);
    npcCh1->set_q_w(0);
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

RenderFrame* mockBlackThrowerTestStartRdf(google::protobuf::Arena* theAllocator) {
    auto chSpecies = globalPrimitiveConsts->ch_species();
    const int roomCapacity = 1;
    auto* startRdf = TestHelper::NewPreallocatedRdf(roomCapacity, 8, 8, theAllocator);
    startRdf->set_id(globalPrimitiveConsts->starting_render_frame_id());
    uint32_t pickableIdCounter = 1;
    uint32_t npcIdCounter = 1;
    uint32_t bulletIdCounter = 1;

    auto characterConfigs = globalConfigConsts->character_configs();

    auto player1 = startRdf->mutable_players(0);
    auto playerCh1 = player1->mutable_chd();
    auto playerCh1Species = chSpecies.blackthrower1();
    auto cc1 = characterConfigs[playerCh1Species];
    playerCh1->set_x(-120);
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
    npc1->set_goal_as_npc(NpcGoal::NIdleIfGoHuntingThenPatrol);
    auto npcCh1 = npc1->mutable_chd();
    auto npcCh1Species = chSpecies.blackthrower1();
    auto npcCc1 = characterConfigs[npcCh1Species];
    npcCh1->set_x(+80);
    npcCh1->set_y(100);
    npcCh1->set_speed(npcCc1.speed());
    npcCh1->set_ch_state(CharacterState::InAirIdle1NoJump);
    npcCh1->set_frames_to_recover(0);
    npcCh1->set_q_x(0);
    npcCh1->set_q_y(1);
    npcCh1->set_q_z(0);
    npcCh1->set_q_w(0);
    npcCh1->set_aiming_q_x(0);
    npcCh1->set_aiming_q_y(0);
    npcCh1->set_aiming_q_z(0);
    npcCh1->set_aiming_q_w(1);
    npcCh1->set_vel_x(0);
    npcCh1->set_vel_y(0);
    npcCh1->set_hp(npcCc1.hp());
    npcCh1->set_species_id(npcCh1Species);
    npcCh1->set_bullet_team_id(2);

    startRdf->set_npc_id_counter(npcIdCounter);
    startRdf->set_npc_count(npcIdCounter - 1);

    startRdf->set_bullet_id_counter(bulletIdCounter);
    startRdf->set_pickable_id_counter(pickableIdCounter);

    return startRdf;
}

RenderFrame* mockDeadBulletLeftShiftTestStartRdf(google::protobuf::Arena* theAllocator) {
    auto chSpecies = globalPrimitiveConsts->ch_species();
    const int roomCapacity = 1;
    auto* startRdf = TestHelper::NewPreallocatedRdf(roomCapacity, 8, 8, theAllocator);
    startRdf->set_id(754);
    uint32_t pickableIdCounter = 1;
    uint32_t npcIdCounter = 1;
    uint32_t bulletIdCounter = 1;

    auto characterConfigs = globalConfigConsts->character_configs();

    auto player1 = startRdf->mutable_players(0);
    auto playerCh1 = player1->mutable_chd();
    auto playerCh1Species = chSpecies.blackthrower1();
    auto cc1 = characterConfigs[playerCh1Species];
    playerCh1->set_x(-120);
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
    npc1->set_goal_as_npc(NpcGoal::NIdleIfGoHuntingThenPatrol);
    auto npcCh1 = npc1->mutable_chd();
    auto npcCh1Species = chSpecies.blacksaber1();
    auto npcCc1 = characterConfigs[npcCh1Species];
    npcCh1->set_x(+80);
    npcCh1->set_y(100);
    npcCh1->set_speed(npcCc1.speed());
    npcCh1->set_ch_state(CharacterState::InAirIdle1NoJump);
    npcCh1->set_frames_to_recover(0);
    npcCh1->set_q_x(0);
    npcCh1->set_q_y(1);
    npcCh1->set_q_z(0);
    npcCh1->set_q_w(0);
    npcCh1->set_aiming_q_x(0);
    npcCh1->set_aiming_q_y(0);
    npcCh1->set_aiming_q_z(0);
    npcCh1->set_aiming_q_w(1);
    npcCh1->set_vel_x(0);
    npcCh1->set_vel_y(0);
    npcCh1->set_hp(npcCc1.hp());
    npcCh1->set_species_id(npcCh1Species);
    npcCh1->set_bullet_team_id(1);

    auto bl1 = startRdf->mutable_bullets(0);
    bl1->set_id(bulletIdCounter++);
    bl1->set_skill_id(1024);
    bl1->set_active_skill_hit(1);
    bl1->set_bl_state(BulletState::Vanishing);
    bl1->set_frames_in_bl_state(24);
    bl1->set_x(0);
    bl1->set_y(120);
    bl1->set_offender_ud(UDT_NPC + npc1->id());
    bl1->set_originated_render_frame_id(startRdf->id() - 55);

    auto bl2 = startRdf->mutable_bullets(1);
    bl2->set_id(bulletIdCounter++);
    bl2->set_skill_id(1027);
    bl2->set_active_skill_hit(1);
    bl2->set_bl_state(BulletState::StartUp);
    bl2->set_frames_in_bl_state(46);
    bl2->set_x(-50);
    bl2->set_y(120);
    bl2->set_offender_ud(UDT_PLAYER + player1->join_index());
    bl2->set_originated_render_frame_id(startRdf->id() - 46);

    startRdf->set_npc_id_counter(npcIdCounter);
    startRdf->set_npc_count(npcIdCounter - 1);

    startRdf->set_bullet_id_counter(bulletIdCounter);
    startRdf->set_bullet_count(bulletIdCounter - 1);

    startRdf->set_pickable_id_counter(pickableIdCounter);

    return startRdf;
}

RenderFrame* mockNpcSpawnerRdf2(google::protobuf::Arena* theAllocator) {
    auto chSpecies = globalPrimitiveConsts->ch_species();
    const int roomCapacity = 1;
    auto* startRdf = TestHelper::NewPreallocatedRdf(roomCapacity, 8, 8, theAllocator);
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

    uint32_t movementTriggerId = 42;

    auto tr1 = startRdf->add_triggers();
    tr1->set_id(movementTriggerId);
    tr1->set_trt(globalPrimitiveConsts->trts().by_movement());
    tr1->set_x(+0);
    tr1->set_y(116);
    ++triggerCount;

    uint32_t npcSpawnerTriggerId = 43;

    auto tr2 = startRdf->add_triggers();
    tr2->set_id(npcSpawnerTriggerId);
    tr2->set_trt(globalPrimitiveConsts->trts().indi_wave_npc_spawner());
    tr2->set_x(+90);
    tr2->set_y(116);
    tr2->set_z(0);
    tr2->set_state(TriggerState::TrReady);
    ++triggerCount;

    uint32_t victoryTriggerId = 44;
    auto tr3 = startRdf->add_triggers();
    tr3->set_id(victoryTriggerId);
    tr3->set_trt(globalPrimitiveConsts->trts().victory());
    ++triggerCount;
    
    startRdf->set_npc_id_counter(npcIdCounter);
    startRdf->set_npc_count(npcIdCounter-1);

    startRdf->set_bullet_id_counter(bulletIdCounter);
    startRdf->set_pickable_id_counter(pickableIdCounter);

    startRdf->set_trigger_count(triggerCount);

    return startRdf;
}

RenderFrame* mockRotatingPlatformForceCrouchingTestStartRdf(google::protobuf::Arena* theAllocator) {
    auto chSpecies = globalPrimitiveConsts->ch_species();
    const int roomCapacity = 1;
    auto* startRdf = TestHelper::NewPreallocatedRdf(roomCapacity, 8, 8, theAllocator);
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
    playerCh1->set_x(32);
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

    auto dynamicTrap1 = startRdf->add_dynamic_traps();
    dynamicTrap1->set_id(42);
    dynamicTrap1->set_tpt(globalPrimitiveConsts->tpts().rotating_platform());
    ++dynamicTrapCount;

    startRdf->set_npc_id_counter(npcIdCounter);
    startRdf->set_npc_count(npcIdCounter - 1);

    startRdf->set_bullet_id_counter(bulletIdCounter);
    startRdf->set_pickable_id_counter(pickableIdCounter);

    startRdf->set_dynamic_trap_count(dynamicTrapCount);

    return startRdf;
}

RenderFrame* mockRotatingPlatformForceCrouchingTestStartRdf2(google::protobuf::Arena* theAllocator) {
    auto chSpecies = globalPrimitiveConsts->ch_species();
    const int roomCapacity = 1;
    auto* startRdf = TestHelper::NewPreallocatedRdf(roomCapacity, 8, 8, theAllocator);
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
    playerCh1->set_x(-64);
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

    auto dynamicTrap1 = startRdf->add_dynamic_traps();
    dynamicTrap1->set_id(42);
    dynamicTrap1->set_tpt(globalPrimitiveConsts->tpts().rotating_platform());
    ++dynamicTrapCount;

    startRdf->set_npc_id_counter(npcIdCounter);
    startRdf->set_npc_count(npcIdCounter - 1);

    startRdf->set_bullet_id_counter(bulletIdCounter);
    startRdf->set_pickable_id_counter(pickableIdCounter);

    startRdf->set_dynamic_trap_count(dynamicTrapCount);

    return startRdf;
}

RenderFrame* mockBtnFTestStartRdf(google::protobuf::Arena* theAllocator) {
    auto chSpecies = globalPrimitiveConsts->ch_species();
    const int roomCapacity = 1;
    auto* startRdf = TestHelper::NewPreallocatedRdf(roomCapacity, 8, 8, theAllocator);
    startRdf->set_id(globalPrimitiveConsts->starting_render_frame_id());
    uint32_t pickableIdCounter = 1;
    uint32_t npcIdCounter = 1;
    uint32_t bulletIdCounter = 1;

    int pickableCount = 0;
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

    auto npc1 = startRdf->mutable_npcs(0);
    npc1->set_id(npcIdCounter++);
    npc1->set_goal_as_npc(NpcGoal::NIdleIfGoHuntingThenPatrol);
    npc1->set_exhausted_to_drop_pkt(globalPrimitiveConsts->pkts().hp_small());
    auto npcCh1 = npc1->mutable_chd();
    auto npcCh1Species = chSpecies.blacksaber_test_with_vision();
    auto npcCc1 = characterConfigs[npcCh1Species];
    npcCh1->set_x(+200);
    npcCh1->set_y(200);
    npcCh1->set_speed(npcCc1.speed());
    npcCh1->set_ch_state(CharacterState::InAirIdle1NoJump);
    npcCh1->set_frames_to_recover(0);
    npcCh1->set_q_x(0);
    npcCh1->set_q_y(1);
    npcCh1->set_q_z(0);
    npcCh1->set_q_w(0);
    npcCh1->set_aiming_q_x(0);
    npcCh1->set_aiming_q_y(0);
    npcCh1->set_aiming_q_z(0);
    npcCh1->set_aiming_q_w(1);
    npcCh1->set_vel_x(0);
    npcCh1->set_vel_y(0);
    npcCh1->set_hp(npcCc1.hp());
    npcCh1->set_species_id(npcCh1Species);
    npcCh1->set_bullet_team_id(3);

    uint32_t btnFTriggerId = 42;

    auto tr1 = startRdf->add_triggers();
    tr1->set_id(btnFTriggerId);
    tr1->set_trt(globalPrimitiveConsts->trts().by_pattern_f());
    tr1->set_x(+20);
    tr1->set_y(116);
    tr1->set_z(0);
    tr1->set_state(TriggerState::TrReady);
    ++triggerCount;
    
    uint32_t pickupSpawnerTrigger1Id = 43;

    auto tr2 = startRdf->add_triggers();
    tr2->set_id(pickupSpawnerTrigger1Id);
    tr2->set_trt(globalPrimitiveConsts->trts().indi_wave_pickable_spawner());
    tr2->set_x(tr1->x());
    tr2->set_y(tr1->y());
    tr2->set_z(tr1->z());
    tr2->set_state(TriggerState::TrReady);
    ++triggerCount;

    uint32_t pickupSpawnerTrigger2Id = 44;

    auto tr3 = startRdf->add_triggers();
    tr3->set_id(pickupSpawnerTrigger2Id);
    tr3->set_trt(globalPrimitiveConsts->trts().indi_wave_pickable_spawner());
    tr3->set_x(+200);
    tr3->set_y(116);
    tr3->set_z(0);
    tr3->set_state(TriggerState::TrReady);
    ++triggerCount;

    startRdf->set_npc_id_counter(npcIdCounter);
    startRdf->set_npc_count(npcIdCounter-1);

    startRdf->set_bullet_id_counter(bulletIdCounter);
    startRdf->set_pickable_id_counter(pickableIdCounter);
    startRdf->set_pickable_count(pickableIdCounter-1);

    startRdf->set_trigger_count(triggerCount);

    return startRdf;
}

RenderFrame* mockTriggerTrapInteractionTestStartRdf(google::protobuf::Arena* theAllocator) {
    auto chSpecies = globalPrimitiveConsts->ch_species();
    const int roomCapacity = 1;
    auto* startRdf = TestHelper::NewPreallocatedRdf(roomCapacity, 8, 8, theAllocator);
    startRdf->set_id(globalPrimitiveConsts->starting_render_frame_id());
    uint32_t pickableIdCounter = 1;
    uint32_t npcIdCounter = 1;
    uint32_t bulletIdCounter = 1;
    uint32_t dynamicTrapCount = 0;
    int triggerCount = 0;

    auto characterConfigs = globalConfigConsts->character_configs();

    auto player1 = startRdf->mutable_players(0);
    auto playerCh1 = player1->mutable_chd();
    auto playerCh1Species = chSpecies.bountyhunter();
    auto cc1 = characterConfigs[playerCh1Species];
    playerCh1->set_x(0);
    playerCh1->set_y(640);
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

    auto tr1 = startRdf->add_triggers();
    tr1->set_id(101);
    tr1->set_trt(globalPrimitiveConsts->trts().by_attack());
    tr1->set_x(+100);
    tr1->set_y(+116);
    tr1->set_z(0);
    tr1->set_state(TriggerState::TrReady);
    ++triggerCount;

    auto dynamicTrap1 = startRdf->add_dynamic_traps();
    dynamicTrap1->set_id(42);
    dynamicTrap1->set_tpt(globalPrimitiveConsts->tpts().rotating_platform());
    ++dynamicTrapCount;

    startRdf->set_npc_id_counter(npcIdCounter);
    startRdf->set_npc_count(npcIdCounter - 1);

    startRdf->set_bullet_id_counter(bulletIdCounter);
    startRdf->set_pickable_id_counter(pickableIdCounter);

    startRdf->set_trigger_count(triggerCount);
    startRdf->set_dynamic_trap_count(dynamicTrapCount);

    return startRdf;
}

RenderFrame* mockDef1Rdf(google::protobuf::Arena* theAllocator) {
    auto chSpecies = globalPrimitiveConsts->ch_species();
    const int roomCapacity = 2;
    auto* startRdf = TestHelper::NewPreallocatedRdf(roomCapacity, 8, 8, theAllocator);
    startRdf->set_id(globalPrimitiveConsts->starting_render_frame_id());
    uint32_t pickableIdCounter = 1;
    uint32_t npcIdCounter = 1;
    uint32_t bulletIdCounter = 1;

    auto characterConfigs = globalConfigConsts->character_configs();

    auto player1 = startRdf->mutable_players(0);
    auto playerCh1 = player1->mutable_chd();
    auto playerCh1Species = chSpecies.shieldguard1();
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

RenderFrame* mockBat1TestStartRdf(google::protobuf::Arena* theAllocator) {
    auto chSpecies = globalPrimitiveConsts->ch_species();
    const int roomCapacity = 1;
    auto* startRdf = TestHelper::NewPreallocatedRdf(roomCapacity, 8, 8, theAllocator);
    startRdf->set_id(globalPrimitiveConsts->starting_render_frame_id());
    uint32_t pickableIdCounter = 1;
    uint32_t npcIdCounter = 1;
    uint32_t bulletIdCounter = 1;

    auto characterConfigs = globalConfigConsts->character_configs();

    auto player1 = startRdf->mutable_players(0);
    auto playerCh1 = player1->mutable_chd();
    auto playerCh1Species = chSpecies.bladegirl();
    auto cc1 = characterConfigs[playerCh1Species];
    playerCh1->set_x(360);
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
    npc1->set_goal_as_npc(NpcGoal::NIdleIfGoHuntingThenPatrol);
    auto npcCh1 = npc1->mutable_chd();
    auto npcCh1Species = chSpecies.bat1();
    auto npcCc1 = characterConfigs[npcCh1Species];
    npcCh1->set_x(320);
    npcCh1->set_y(100);
    npcCh1->set_speed(npcCc1.speed());
    npcCh1->set_ch_state(CharacterState::Idle1);
    npcCh1->set_frames_to_recover(0);
    // Intentionally NOT facing player at the beginning to test turnaround logic
    npcCh1->set_q_x(0);
    npcCh1->set_q_y(1);
    npcCh1->set_q_z(0);
    npcCh1->set_q_w(0); 
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
    auto npcCh2Species = chSpecies.bat1();
    auto npcCc2 = characterConfigs[npcCh2Species];
    npcCh2->set_x(-400);
    npcCh2->set_y(450);
    npcCh2->set_speed(npcCc2.speed());
    npcCh2->set_ch_state(CharacterState::Walking);
    npcCh2->set_frames_to_recover(0);
    npcCh2->set_q_x(0);
    npcCh2->set_q_y(1);
    npcCh2->set_q_z(0);
    npcCh2->set_q_w(0); 
    npcCh2->set_aiming_q_x(0);
    npcCh2->set_aiming_q_y(0);
    npcCh2->set_aiming_q_z(0);
    npcCh2->set_aiming_q_w(1);
    npcCh2->set_vel_x(-npcCc2.speed());
    npcCh2->set_vel_y(+npcCc2.speed());
    npcCh2->set_hp(npcCc2.hp());
    npcCh2->set_species_id(npcCh2Species);
    npcCh2->set_bullet_team_id(3);
    
    startRdf->set_npc_id_counter(npcIdCounter);
    startRdf->set_npc_count(npcIdCounter-1);

    startRdf->set_bullet_id_counter(bulletIdCounter);
    startRdf->set_pickable_id_counter(pickableIdCounter);

    return startRdf;
}

RenderFrame* mockRefRdf(int refRdfId, google::protobuf::Arena* theAllocator) {
    auto chSpecies = globalPrimitiveConsts->ch_species();
    const int roomCapacity = 2;
    auto refRdf = TestHelper::NewPreallocatedRdf(roomCapacity, 8, 8, theAllocator);
    refRdf->set_id(refRdfId);
    uint32_t pickableIdCounter = 1;
    uint32_t npcIdCounter = 1;
    uint32_t bulletIdCounter = 1;

    auto characterConfigs = globalConfigConsts->character_configs();

    auto player1 = refRdf->mutable_players(0);
    auto playerCh1 = player1->mutable_chd();
    auto playerCh1Species = chSpecies.bladegirl();
    auto cc1 = characterConfigs[playerCh1Species];
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
    playerCh1->set_species_id(playerCh1Species);
    player1->set_join_index(1);
    player1->set_revival_x(playerCh1->x());
    player1->set_revival_y(playerCh1->y());
    player1->set_revival_q_x(0);
    player1->set_revival_q_y(0);
    player1->set_revival_q_z(0);
    player1->set_revival_q_w(1);

    auto player2 = refRdf->mutable_players(1);
    auto playerCh2 = player2->mutable_chd();
    auto playerCh2Species = chSpecies.bountyhunter();
    auto cc2 = characterConfigs[playerCh2Species];
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
    playerCh2->set_species_id(playerCh2Species);
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
    {60, 3},
    {64, 35},
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
    {8, 0},
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
    {199, 0},
    {200, 3},
    {216, 3},
    {220, 19},
    {243, 3},
    {244, 259},
    {248, 3},
    {320, 3},
    {960, 3},
    {964, 3},
    {992, 19},
    {1023, 3},
    {1024, 0},
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
    {399, 0},
    {400, 4},
    {435, 4},
    {436, 20},
    {483, 4},
    {484, 20},
    {499, 4},
    {500, 20},
    {539, 4},
    {540, 20},
    {579, 4},
    {580, 20},
    {600, 4},
    {604, 20},
    {635, 4},
    {636, 3},
    {640, 3},
    {644, 35},
    {800, 35},
    {804, 291},
    {808, 0},
    {820, 0},
    {824, 32},
    {944, 32},
    {948, 0},
    {2048, 0}
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
    {250, 0},
    {254, 3},
    {320, 3},
    {360, 3},
    {400, 19},
    {404, 3},
    {440, 3},
    {444, 19},
    {460, 19},
    {464, 3},
    {480, 3},
    {484, 0},
    {556, 0},
    {560, 32},
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

std::map<int, uint64_t> testCmds19 = {
    {0, 0},
    {63, 0},
    {64, 16},
    {65, 0},
    {83, 0},
    {84, 16},
    {85, 0},
    {143, 0},
    {144, 18},
    {359, 0},
    {360, 16},
    {361, 0},
    {379, 0},
    {380, 16},
    {381, 0},
    {439, 0},
    {440, 3},
    {1023, 3},
    {1024, 0},
};

std::map<int, uint64_t> testCmds20 = {
    {0, 0},
    {67, 0},
    {68, 32},
    {69, 0},
    {70, 0},
    {71, 0},
    {100, 0},
    {111, 0},
    {110, 0},
    {119, 0},
    {120, 32},
    {122, 0},
    {179, 0},
    {180, 32},
    {181, 0},
    {299, 0},
    {300, 32},
    {302, 0},
    {1024, 0}
};

std::map<int, uint64_t> testCmds21 = {
    {0, 0},
};

std::map<int, uint64_t> testCmds22 = {
    {0, 32},
    {240, 32},
    {2048, 0}
};

std::map<int, uint64_t> testCmds23 = {
    {0, 0},
    {32, 0},
    {40, 0},
    {44, 0},
    {67, 0},
    {68, 0}, 
    {72, 32}, 
    {76, 0}, 
    {166, 0}, 
    {167, 0}, 
    {191, 0},
    {192, 0},
    {200, 0},
    {220, 0},
    {240, 0},
    {360, 0}
};

std::map<int, uint64_t> testCmds24 = {
    {0, 0},
    {63, 0},
    {64, 32},
    {83, 0},
    {84, 259},
    {127, 0}, 
    {128, 32}, 
    {147, 0}, 
    {148, 32}, 
    {167, 0}, 
    {168, 32}, 
    {187, 0}, 
    {188, 32}, 
    {207, 0}, 
    {208, 32}, 
    {227, 0}, 
    {228, 32}, 
    {247, 0}, 
    {248, 32}, 
    {267, 0}, 
    {268, 32}, 
    {287, 0}, 
    {288, 32}, 
    {307, 0}, 
    {308, 32}, 
    {327, 0}, 
    {328, 32}, 
    {347, 0}, 
    {348, 32}, 
    {367, 0}, 
    {368, 32}, 
    {387, 0}, 
    {388, 32}, 
    {407, 0},
    {408, 32},
    {427, 0},
    {428, 32},
    {447, 0},
    {448, 32},
    {1023, 0},
    {1024, 32},
    {2048, 0}, 
};

std::map<int, uint64_t> testCmds25 = {
    {0, 0},
    {459, 0},
    {460, 259},
    {447, 0},
    {499, 0},
    {500, 256},
    {2048, 0},
};

std::map<int, uint64_t> testCmds26 = {
    {0, 0},
    {99, 0},
    {100, 3},
    {239, 3},
    {240, 259},
    {241, 0},
    {2048, 0},
};

std::map<int, uint64_t> testCmds27 = {
    {0, 0},
    {99, 0},
    {100, 0},
    {239, 0},
    {240, 32},
    {241, 0},
    {2048, 0},
};

std::map<int, uint64_t> testCmds28 = {
    {0, 0},
    {199, 0},
    {200, 3}, // Follow the fled npc1
    {360, 3},
    {361, 0},
    {2048, 0},
};

std::map<int, uint64_t> testCmds29 = {
    {0, 0},
    {99, 0},
    {100, 0},
    {239, 0},
    {240, 32},
    {241, 0},
    {248, 3}, // Move over to trigger NPC reaction
    {750, 3}, // Around the time NPC1 got "NOT_ENOUGH_MP" reaction
    {920, 3},
    {921, 0},
    {2048, 0},
};

std::map<int, uint64_t> testCmds30 = {
    {0, 0},
    {99, 0},
    {100, 0},
    {239, 0},
    {240, 0},
    {241, 0},
    {299, 0},
    {399, 0},
    {400, 0},
    {520, 0},
    {521, 0},
    {919, 0},
    {920, 0},
    {921, 0},
    {2048, 0},
};

std::map<int, uint64_t> testCmds31 = {
    {0, 0},
    {67, 0},
    {68, 32},
    {69, 0},
    {70, 0},
    {71, 0},
    {100, 0},
    {111, 0},
    {110, 0},
    {119, 0},
    {120, 32},
    {122, 0},
    {179, 0},
    {180, 32},
    {181, 0},
    {299, 0},
    {300, 32},
    {302, 0},
    {399, 0},
    {400, 32},
    {479, 0},
    {480, 32},
    {481, 0},
    {599, 0},
    {600, 32},
    {601, 0},
    {1024, 0}
};

std::map<int, uint64_t> testCmds32 = {
    {0, 0},
    {7, 0},
    {8, 3},
    {299, 3},
    {300, 0},
    {1024, 0}
};

std::map<int, uint64_t> testCmds33 = {
    {0, 0},
    {399, 0},
    {400, 3},   
    {799, 3},
    {800, 0},
    {1024, 0}
};

std::map<int, uint64_t> testCmds34 = {
    {0, 0},
    {32, 3},
    {50, 3},   
    {51, 0},
    {127, 0},
    {128, 512},
    {129, 0},
    {327, 0},
    {328, 512},
    {379, 0},
    {380, 0},
    {399, 0},
    {400, 32},
    {401, 0},
    {1024, 0}
};

std::map<int, uint64_t> testCmds35 = {
    {0, 0},
    {32, 3},
    {50, 3},
    {51, 0},
    {127, 0},
    {128, 512},
    {129, 0},
    {327, 0},
    {328, 512},
    {379, 0},
    {380, 0},
    {399, 0},
    {400, 32},
    {401, 0},
    {1024, 0}
};

std::map<int, uint64_t> testCmds36 = {
    {0, 0},
    {159, 0},
    {160, 32},
    {161, 0},
    {399, 0},
    {400, 32},
    {401, 0},
    {1024, 0},
};

std::map<int, uint64_t> testCmds37 = {
    {0, 0},
    {63, 0},
    {64, 1},
    {270, 1},
    {271, 0},
    {272, 4},
    {280, 4},
    {281, 0},
    {290, 1},
    {400, 1},
    {401, 0},
    {1024, 0},
};

std::map<int, uint64_t> testCmds38 = {
    {0, 0},
    {9, 0},
    {10, 3},
    {59, 3},
    {60, 19}, // Climb up the wall and air-dash to flee from npc1
    {63, 19},
    {64, 3},
    {71, 3},
    {72, 19},
    {89, 19},
    {90, 259},
    {99, 259},
    {100, 3},
    {107, 3},
    {108, 20},
    {127, 20},
    {128, 260},
    {129, 260},
    {132, 4},
    {149, 4},
    {150, 4},
    {151, 4},
    {199, 4},
    {279, 4},
    {280, 4},
    {299, 4},
    {300, 20},
    {323, 20}, 
    {324, 4},
    {359, 4},
    {360, 20},
    {399, 20},
    {400, 4},
    {499, 4},
    {500, 0},
    {2048, 0},
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

std::unordered_map<int, WsReq*> incomingUpsyncSnapshotReqs21; 
std::unordered_map<int, DownsyncSnapshot*> incomingDownsyncSnapshots21;

std::unordered_map<int, WsReq*> incomingUpsyncSnapshotReqs23Intime;
std::unordered_map<int, WsReq*> incomingUpsyncSnapshotReqs23Rollback;

std::unordered_map<int, WsReq*> incomingUpsyncSnapshotReqs37Intime;

void initTest1Data(WsReq* initializerMapData, std::vector<std::vector<float>>& hulls, google::protobuf::Arena* theAllocator) {
    auto* startRdf = mockStartRdf(theAllocator);
    TestHelper::AddHullsToWsReq(initializerMapData, hulls, std::vector<bool>(hulls.size(), true), std::vector<bool>(hulls.size(), false));
    initializerMapData->set_allocated_self_parsed_rdf(startRdf);

    // incomingUpsyncSnapshotReqs1
    {
        int receivedTimerRdfId = 120;
        int receivedEdIfdId = 26;
        int receivedStIfdId = 13;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        DownsyncSnapshot* srvDownsyncSnapshot = google::protobuf::Arena::Create<DownsyncSnapshot>(theAllocator);
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
        DownsyncSnapshot* srvDownsyncSnapshot = google::protobuf::Arena::Create<DownsyncSnapshot>(theAllocator);
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

void initTest2Data(WsReq* initializerMapData, std::vector<std::vector<float>>& hulls, google::protobuf::Arena* theAllocator) {
    auto* startRdf = mockStartRdf(theAllocator);
    TestHelper::AddHullsToWsReq(initializerMapData, hulls, std::vector<bool>(hulls.size(), true), std::vector<bool>(hulls.size(), false));
    initializerMapData->set_allocated_self_parsed_rdf(startRdf);

    // incomingUpsyncSnapshotReqs2
    {
        int receivedTimerRdfId = 120;
        int receivedEdIfdId = 26;
        int receivedStIfdId = 13;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        DownsyncSnapshot* srvDownsyncSnapshot = google::protobuf::Arena::Create<DownsyncSnapshot>(theAllocator);
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
        DownsyncSnapshot* srvDownsyncSnapshot = google::protobuf::Arena::Create<DownsyncSnapshot>(theAllocator);
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
        RenderFrame* refRdf = mockRefRdf(refRdfId, theAllocator);
        srvDownsyncSnapshot->set_ref_rdf_id(refRdfId);
        srvDownsyncSnapshot->set_allocated_ref_rdf(refRdf);
    }
}

void initTest3Data(WsReq* initializerMapData, std::vector<std::vector<float>>& hulls, google::protobuf::Arena* theAllocator) {
    auto* startRdf = mockStartRdf(theAllocator);
    TestHelper::AddHullsToWsReq(initializerMapData, hulls, std::vector<bool>(hulls.size(), true), std::vector<bool>(hulls.size(), false));
    initializerMapData->set_allocated_self_parsed_rdf(startRdf);

    // incomingUpsyncSnapshotReqs3
    {
        int receivedTimerRdfId = 120;
        int receivedEdIfdId = 26;
        int receivedStIfdId = 13;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        DownsyncSnapshot* srvDownsyncSnapshot = google::protobuf::Arena::Create<DownsyncSnapshot>(theAllocator);
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
        DownsyncSnapshot* srvDownsyncSnapshot = google::protobuf::Arena::Create<DownsyncSnapshot>(theAllocator);
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
        RenderFrame* refRdf = mockRefRdf(refRdfId, theAllocator);
        srvDownsyncSnapshot->set_ref_rdf_id(refRdfId);
        srvDownsyncSnapshot->set_allocated_ref_rdf(refRdf);
    }
}

void initTest4Data(WsReq* initializerMapData, std::vector<std::vector<float>>& hulls, google::protobuf::Arena* theAllocator) {
    auto* startRdf = mockStartRdf(theAllocator);
    TestHelper::AddHullsToWsReq(initializerMapData, hulls, std::vector<bool>(hulls.size(), true), std::vector<bool>(hulls.size(), false));
    initializerMapData->set_allocated_self_parsed_rdf(startRdf);

    // incomingDownsyncSnapshots4
    {
        int receivedEdIfdId = 4;
        int receivedStIfdId = 0;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedEdIfdId - 1) - 1; // A few rdfs after the last self-input generation, right before being used
        DownsyncSnapshot* srvDownsyncSnapshot = google::protobuf::Arena::Create<DownsyncSnapshot>(theAllocator);
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
        srvDownsyncSnapshot->set_allocated_ref_rdf(mockRefRdf(refRdfId, theAllocator));
        incomingDownsyncSnapshots4[receivedTimerRdfId] = srvDownsyncSnapshot;
    }
}

void initTest5Data(WsReq* initializerMapData, std::vector<std::vector<float>>& hulls, google::protobuf::Arena* theAllocator) {
    auto* startRdf = mockStartRdf(theAllocator);
    TestHelper::AddHullsToWsReq(initializerMapData, hulls, std::vector<bool>(hulls.size(), true), std::vector<bool>(hulls.size(), false));
    initializerMapData->set_allocated_self_parsed_rdf(startRdf);

    // incomingDownsyncSnapshots5
    {
        int receivedTimerRdfId = 180;
        int receivedEdIfdId = 26;
        int receivedStIfdId = 0;
        DownsyncSnapshot* srvDownsyncSnapshot = google::protobuf::Arena::Create<DownsyncSnapshot>(theAllocator);
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
        DownsyncSnapshot* srvDownsyncSnapshot = google::protobuf::Arena::Create<DownsyncSnapshot>(theAllocator);
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

void initTest6Data(WsReq* initializerMapData, std::vector<std::vector<float>>& hulls, google::protobuf::Arena* theAllocator) {
    auto* startRdf = mockStartRdf(theAllocator);
    TestHelper::AddHullsToWsReq(initializerMapData, hulls, std::vector<bool>(hulls.size(), true), std::vector<bool>(hulls.size(), false));
    initializerMapData->set_allocated_self_parsed_rdf(startRdf);

    // Intime reference inputs
    {
        int receivedEdIfdId = 13;
        int receivedStIfdId = 0;
        int receivedTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs6Intime[receivedTimerRdfId] = req;
    }
/////////////////////////////////////////////////////////////////////////////////////////
    {
        int receivedTimerRdfId = 120;
        int receivedEdIfdId = 26;
        int receivedStIfdId = 13;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs6Rollback[receivedTimerRdfId] = req;
    }
}

void initTest7Data(WsReq* initializerMapData, std::vector<std::vector<float>>& hulls, google::protobuf::Arena* theAllocator) {
    auto* startRdf = mockStartRdf(theAllocator);
    TestHelper::AddHullsToWsReq(initializerMapData, hulls, std::vector<bool>(hulls.size(), true), std::vector<bool>(hulls.size(), false));
    initializerMapData->set_allocated_self_parsed_rdf(startRdf);

    // Intime reference inputs
    {
        int receivedEdIfdId = 16;
        int receivedStIfdId = 0;
        int receivedTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(4);
        }
        incomingUpsyncSnapshotReqs7Intime[receivedTimerRdfId] = req;
    }
/////////////////////////////////////////////////////////////////////////////////////////
    {
        int receivedEdIfdId = 16;
        int receivedStIfdId = 0;
        int receivedTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(4);
        }
        incomingUpsyncSnapshotReqs7Rollback[receivedTimerRdfId] = req;
    }
}

void initTest8Data(WsReq* initializerMapData, std::vector<std::vector<float>>& hulls, google::protobuf::Arena* theAllocator) {
    auto blacksaber1VisionTestStartRdf = mockBlacksaber1VisionTestStartRdf(theAllocator);
    TestHelper::AddHullsToWsReq(initializerMapData, hulls, std::vector<bool>(hulls.size(), true), std::vector<bool>(hulls.size(), false));
    initializerMapData->set_allocated_self_parsed_rdf(blacksaber1VisionTestStartRdf);
}

void initTest9Data(WsReq* initializerMapData, std::vector<std::vector<float>>& hulls, google::protobuf::Arena* theAllocator) {
    auto startRdf = mockSliderTrapTestStartRdf1(theAllocator);
    TestHelper::AddHullsToWsReq(initializerMapData, hulls, std::vector<bool>(hulls.size(), true), std::vector<bool>(hulls.size(), false));
    initializerMapData->set_allocated_self_parsed_rdf(startRdf);

    auto trapConfigFromTiled1 = initializerMapData->add_trap_config_from_tile_list();
    Vec3 initVel(3.f * globalPrimitiveConsts->battle_dynamics_fps(), 4.f * globalPrimitiveConsts->battle_dynamics_fps(), 0);
    trapConfigFromTiled1->set_id(startRdf->dynamic_traps(0).id());
    trapConfigFromTiled1->set_tpt(startRdf->dynamic_traps(0).tpt());
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
    trapConfigFromTiled1->set_init_y(1500);
    trapConfigFromTiled1->set_init_z(0);

    trapConfigFromTiled1->set_cooldown_rdf_count(120);
    trapConfigFromTiled1->set_limit_1(-500.0f);
    trapConfigFromTiled1->set_limit_2(+500.0f);
}

void initTest10Data(WsReq* initializerMapData, std::vector<std::vector<float>>& hulls, google::protobuf::Arena* theAllocator) {
    auto* startRdf = mockStartRdf(theAllocator);
    TestHelper::AddHullsToWsReq(initializerMapData, hulls, std::vector<bool>(hulls.size(), true), std::vector<bool>(hulls.size(), false));
    initializerMapData->set_allocated_self_parsed_rdf(startRdf);

    // incomingUpsyncSnapshotReqs10
    {
        int receivedEdIfdId = 3;
        int receivedStIfdId = 0;
        int receivedTimerRdfId = 8;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        DownsyncSnapshot* srvDownsyncSnapshot = google::protobuf::Arena::Create<DownsyncSnapshot>(theAllocator);
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
        DownsyncSnapshot* srvDownsyncSnapshot = google::protobuf::Arena::Create<DownsyncSnapshot>(theAllocator);
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
        DownsyncSnapshot* srvDownsyncSnapshot = google::protobuf::Arena::Create<DownsyncSnapshot>(theAllocator);
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
        DownsyncSnapshot* srvDownsyncSnapshot = google::protobuf::Arena::Create<DownsyncSnapshot>(theAllocator);
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
        DownsyncSnapshot* srvDownsyncSnapshot = google::protobuf::Arena::Create<DownsyncSnapshot>(theAllocator);
        srvDownsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            InputFrameDownsync* ifdBatch = srvDownsyncSnapshot->add_ifd_batch();
            ifdBatch->add_input_list(getSelfCmdByIfdId(testCmds1, ifdId));
            ifdBatch->add_input_list(20);
        }
        incomingDownsyncSnapshots10[receivedTimerRdfId] = srvDownsyncSnapshot;
    }
}

void initTest11Data(WsReq* initializerMapData, std::vector<std::vector<float>>& hulls, google::protobuf::Arena* theAllocator) {
    auto startRdf = mockRollbackChasingAlignTestStartRdf(theAllocator);
    TestHelper::AddHullsToWsReq(initializerMapData, hulls, std::vector<bool>(hulls.size(), true), std::vector<bool>(hulls.size(), false));
    initializerMapData->set_allocated_self_parsed_rdf(startRdf);

    {
        int receivedEdIfdId = 2;
        int receivedStIfdId = 0;
        int receivedTimerRdfId = globalPrimitiveConsts->starting_render_frame_id()+2;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs11Intime[receivedTimerRdfId] = req;
    }
/////////////////////////////////////////////////////////////////////////////////////////
    {
        int receivedTimerRdfId = 240;
        int receivedEdIfdId = 6;
        int receivedStIfdId = 2;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs11Rollback[receivedTimerRdfId] = req;
    }
}

void initTest12Data(WsReq* initializerMapData, std::vector<std::vector<float>>& hulls, google::protobuf::Arena* theAllocator) {
    auto startRdf = mockFallenDeathRdf(theAllocator);
    TestHelper::AddHullsToWsReq(initializerMapData, hulls, std::vector<bool>(hulls.size(), true), std::vector<bool>(hulls.size(), false));
    initializerMapData->set_allocated_self_parsed_rdf(startRdf);

    {
        int receivedEdIfdId = 2;
        int receivedStIfdId = 0;
        int receivedTimerRdfId = globalPrimitiveConsts->starting_render_frame_id() + 2;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs12Intime[receivedTimerRdfId] = req;
    }
}

void initTest13Data(WsReq* initializerMapData, std::vector<std::vector<float>>& hulls, google::protobuf::Arena* theAllocator) {
    auto bladeGirlSkillStartRdf = mockBladeGirlSkillRdf(theAllocator);
    TestHelper::AddHullsToWsReq(initializerMapData, hulls, std::vector<bool>(hulls.size(), true), std::vector<bool>(hulls.size(), false));
    initializerMapData->set_allocated_self_parsed_rdf(bladeGirlSkillStartRdf);
}

void initTest14Data(WsReq* initializerMapData, std::vector<std::vector<float>>& hulls, google::protobuf::Arena* theAllocator) {
    auto bountyHunterSkillStartRdf = mockBountyHunterSkillRdf(theAllocator);
    TestHelper::AddHullsToWsReq(initializerMapData, hulls, std::vector<bool>(hulls.size(), true), std::vector<bool>(hulls.size(), false));
    initializerMapData->set_allocated_self_parsed_rdf(bountyHunterSkillStartRdf);
    {
        int receivedEdIfdId = 2;
        int receivedStIfdId = 0;
        int receivedTimerRdfId = globalPrimitiveConsts->starting_render_frame_id() + 2;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs14Intime[receivedTimerRdfId] = req;
    }
    {
        int receivedEdIfdId = 5;
        int receivedStIfdId = 2;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId) - 1;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(4);
        }
        incomingUpsyncSnapshotReqs14Intime[receivedTimerRdfId] = req;
    }
    {
        int receivedEdIfdId = 6;
        int receivedStIfdId = 5;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId) - 1;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(20);
        }
        incomingUpsyncSnapshotReqs14Intime[receivedTimerRdfId] = req;
    }
    {
        int receivedEdIfdId = 11;
        int receivedStIfdId = 6;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId) - 1;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        int receivedStIfdId = 11;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId) - 1;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs14Intime[receivedTimerRdfId] = req;
    }
    {
        int receivedEdIfdId = 70;
        int receivedStIfdId = 57;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId) - 1;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(4);
        }
        incomingUpsyncSnapshotReqs14Intime[receivedTimerRdfId] = req;
    }
    {
        int receivedEdIfdId = 73;
        int receivedStIfdId = 70;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId) - 1;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(20);
        }
        incomingUpsyncSnapshotReqs14Intime[receivedTimerRdfId] = req;
    }
    {
        int receivedEdIfdId = 74;
        int receivedStIfdId = 73;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId) - 1;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs14Intime[receivedTimerRdfId] = req;
    }
    {
        // BladeGirl tries to use BladeHit1 to counter charged pistol bullet, but would fail
        int receivedEdIfdId = 239;
        int receivedStIfdId = 237;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId) - 1;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(32);
        }
        incomingUpsyncSnapshotReqs14Intime[receivedTimerRdfId] = req;
    }
}

void initTest15Data(WsReq* initializerMapData, std::vector<std::vector<float>>& hulls, google::protobuf::Arena* theAllocator) {
    auto victoryTriggerStartRdf = mockVictoryRdf(theAllocator);
    TestHelper::AddHullsToWsReq(initializerMapData, hulls, std::vector<bool>(hulls.size(), true), std::vector<bool>(hulls.size(), false));
    initializerMapData->set_allocated_self_parsed_rdf(victoryTriggerStartRdf);

    auto triggerConfigFromTiled1 = initializerMapData->add_trigger_config_from_tile_list();
    triggerConfigFromTiled1->set_id(victoryTriggerStartRdf->triggers(0).id());
    triggerConfigFromTiled1->set_trt(victoryTriggerStartRdf->triggers(0).trt());
}

void initTest16Data(WsReq* initializerMapData, std::vector<std::vector<float>>& hulls, google::protobuf::Arena* theAllocator) {
    auto startRdf = mockSliderTrapTestStartRdf2(theAllocator);
    TestHelper::AddHullsToWsReq(initializerMapData, hulls, std::vector<bool>(hulls.size(), true), std::vector<bool>(hulls.size(), false));
    initializerMapData->set_allocated_self_parsed_rdf(startRdf);

    auto trapConfigFromTiled2 = initializerMapData->add_trap_config_from_tile_list();
    Vec3 initVel(-0.4f * globalPrimitiveConsts->battle_dynamics_fps(), -0.3f * globalPrimitiveConsts->battle_dynamics_fps(), 0);
    trapConfigFromTiled2->set_id(startRdf->dynamic_traps(0).id());
    trapConfigFromTiled2->set_tpt(startRdf->dynamic_traps(0).tpt());
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

void initTest17Data(WsReq* initializerMapData, std::vector<std::vector<float>>& hulls, google::protobuf::Arena* theAllocator) {
    auto startRdf = mockSliderTrapTestStartRdf3(theAllocator);
    TestHelper::AddHullsToWsReq(initializerMapData, hulls, std::vector<bool>(hulls.size(), true), std::vector<bool>(hulls.size(), false));
    initializerMapData->set_allocated_self_parsed_rdf(startRdf);

    auto trapConfigFromTiled1 = initializerMapData->add_trap_config_from_tile_list();
    Vec3 initVel1(-2.f * globalPrimitiveConsts->battle_dynamics_fps(), 0.f * globalPrimitiveConsts->battle_dynamics_fps(), 0);
    trapConfigFromTiled1->set_id(startRdf->dynamic_traps(0).id());
    trapConfigFromTiled1->set_tpt(startRdf->dynamic_traps(0).tpt());
    trapConfigFromTiled1->set_linear_speed(initVel1.Length());
    trapConfigFromTiled1->set_box_half_size_x(50.f);
    trapConfigFromTiled1->set_box_half_size_y(3.f);
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

    trapConfigFromTiled1->set_init_x(0);
    trapConfigFromTiled1->set_init_y(105);
    trapConfigFromTiled1->set_init_z(0);

    trapConfigFromTiled1->set_cooldown_rdf_count(120);
    trapConfigFromTiled1->set_limit_1(-300.0f);
    trapConfigFromTiled1->set_limit_2(+300.0f);

    auto trapConfigFromTiled2 = initializerMapData->add_trap_config_from_tile_list();
    Vec3 initVel2(+1.f * globalPrimitiveConsts->battle_dynamics_fps(), 0.f * globalPrimitiveConsts->battle_dynamics_fps(), 0);
    trapConfigFromTiled2->set_id(startRdf->dynamic_traps(1).id());
    trapConfigFromTiled2->set_tpt(startRdf->dynamic_traps(1).tpt());
    trapConfigFromTiled2->set_linear_speed(initVel2.Length());
    trapConfigFromTiled2->set_box_half_size_x(50.f);
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

    trapConfigFromTiled2->set_init_x(64);
    trapConfigFromTiled2->set_init_y(128);
    trapConfigFromTiled2->set_init_z(0);

    trapConfigFromTiled2->set_cooldown_rdf_count(120);
    trapConfigFromTiled2->set_limit_1(-200.0f);
    trapConfigFromTiled2->set_limit_2(+200.0f);
}

void initTest18Data(WsReq* initializerMapData, std::vector<std::vector<float>>& hulls, google::protobuf::Arena* theAllocator) {
    auto startRdf = mockRollbackChasingAlignTestStartRdf2(theAllocator);
    TestHelper::AddHullsToWsReq(initializerMapData, hulls, std::vector<bool>(hulls.size(), true), std::vector<bool>(hulls.size(), false));

    auto trapConfigFromTiled1 = initializerMapData->add_trap_config_from_tile_list();
    Vec3 initVel1(-2.f * globalPrimitiveConsts->battle_dynamics_fps(), 0.f * globalPrimitiveConsts->battle_dynamics_fps(), 0);
    trapConfigFromTiled1->set_id(startRdf->dynamic_traps(0).id());
    trapConfigFromTiled1->set_tpt(startRdf->dynamic_traps(0).tpt());
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

    auto trapConfigFromTiled2 = initializerMapData->add_trap_config_from_tile_list();
    Vec3 initVel2(+2.f * globalPrimitiveConsts->battle_dynamics_fps(), 0.f * globalPrimitiveConsts->battle_dynamics_fps(), 0);
    trapConfigFromTiled2->set_id(startRdf->dynamic_traps(1).id());
    trapConfigFromTiled2->set_tpt(startRdf->dynamic_traps(1).tpt());
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

    initializerMapData->set_allocated_self_parsed_rdf(startRdf);
    {
        int receivedEdIfdId = 2;
        int receivedStIfdId = 0;
        int receivedTimerRdfId = globalPrimitiveConsts->starting_render_frame_id()+2;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs18Intime[receivedTimerRdfId] = req;
    }
/////////////////////////////////////////////////////////////////////////////////////////
    {
        int receivedTimerRdfId = 240;
        int receivedEdIfdId = 6;
        int receivedStIfdId = 2;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
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
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs18Rollback[receivedTimerRdfId] = req;
    }
}

void initTest19Data(WsReq* testInitializerMapData, std::vector<std::vector<float>>& hulls, google::protobuf::Arena* theAllocator) {
    auto testStartRdf = mockSlipJumpStartRdf(theAllocator);
    std::vector<bool> slipJumpOptions(hulls.size(), false);
    slipJumpOptions[1] = true;
    slipJumpOptions[3] = true;
    TestHelper::AddHullsToWsReq(testInitializerMapData, hulls, std::vector<bool>(hulls.size(), true), slipJumpOptions);
    testInitializerMapData->set_allocated_self_parsed_rdf(testStartRdf);
}

void initTest20Data(WsReq* initializerMapData, std::vector<std::vector<float>>& hulls, google::protobuf::Arena* theAllocator) {
    auto chSpecies = globalPrimitiveConsts->ch_species();
    auto startRdf = mockNpcSpawnerRdf(theAllocator);
    TestHelper::AddHullsToWsReq(initializerMapData, hulls, std::vector<bool>(hulls.size(), true), std::vector<bool>(hulls.size(), false));

    initializerMapData->set_allocated_self_parsed_rdf(startRdf);
    auto triggerConfigFromTiled1 = initializerMapData->add_trigger_config_from_tile_list();
    triggerConfigFromTiled1->set_id(startRdf->triggers(0).id());
    triggerConfigFromTiled1->set_trt(startRdf->triggers(0).trt());
    triggerConfigFromTiled1->set_box_half_size_x(200.f);
    triggerConfigFromTiled1->set_box_half_size_y(50.f);
    triggerConfigFromTiled1->set_publishing_to_trigger_id_upon_exhausted(startRdf->triggers(2).id());

    auto triggerConfigFromTiled2 = initializerMapData->add_trigger_config_from_tile_list();
    triggerConfigFromTiled2->set_id(startRdf->triggers(1).id());
    triggerConfigFromTiled2->set_trt(startRdf->triggers(1).trt());
    triggerConfigFromTiled2->set_delayed_frames((globalPrimitiveConsts->battle_dynamics_fps() >> 1));
    triggerConfigFromTiled2->set_recovery_frames(5*globalPrimitiveConsts->battle_dynamics_fps()); 
    triggerConfigFromTiled2->set_bullet_team_id(3);
    triggerConfigFromTiled2->set_sub_cycle_trigger_frames(20);
    triggerConfigFromTiled2->set_init_q_x(0);
    triggerConfigFromTiled2->set_init_q_y(1);
    triggerConfigFromTiled2->set_init_q_z(0);
    triggerConfigFromTiled2->set_init_q_w(0);
    triggerConfigFromTiled2->set_new_revival_x(startRdf->triggers(1).x());
    triggerConfigFromTiled2->set_new_revival_y(startRdf->triggers(1).y());
    triggerConfigFromTiled2->set_publishing_to_trigger_id_upon_exhausted(startRdf->triggers(0).id());
    // Its own quota is 3, however as it subscribes to a "trts().by_movement" whose "quota" is only 1, this "indi_wave_npc_spawner" will only use the first "characterSpawnerTimeSeq"
    auto* mutableChSpanwerTimeSeq2_1 = triggerConfigFromTiled2->add_character_spawner_time_seq(); 
    mutableChSpanwerTimeSeq2_1->set_cutoff_rdf_id(1);
    mutableChSpanwerTimeSeq2_1->add_species_id_list(chSpecies.blacksaber_test_with_vision());
    mutableChSpanwerTimeSeq2_1->add_species_id_list(chSpecies.blacksaber_test_with_vision());

    auto* mutableChSpanwerTimeSeq2_2 = triggerConfigFromTiled2->add_character_spawner_time_seq(); 
    mutableChSpanwerTimeSeq2_2->set_cutoff_rdf_id(2);
    mutableChSpanwerTimeSeq2_2->add_species_id_list(chSpecies.blacksaber_test_with_vision());
    mutableChSpanwerTimeSeq2_2->add_species_id_list(chSpecies.blacksaber_test_with_vision());
    mutableChSpanwerTimeSeq2_2->add_species_id_list(chSpecies.blacksaber_test_with_vision());
    mutableChSpanwerTimeSeq2_2->add_species_id_list(chSpecies.blacksaber_test_with_vision());

    auto* mutableChSpanwerTimeSeq2_3 = triggerConfigFromTiled2->add_character_spawner_time_seq(); 
    mutableChSpanwerTimeSeq2_3->set_cutoff_rdf_id(3);
    mutableChSpanwerTimeSeq2_3->add_species_id_list(chSpecies.blacksaber_test_with_vision());

    auto triggerConfigFromTiled3 = initializerMapData->add_trigger_config_from_tile_list();
    triggerConfigFromTiled3->set_id(startRdf->triggers(2).id());
    triggerConfigFromTiled3->set_trt(startRdf->triggers(2).trt());
}

void initTest21Data(WsReq* initializerMapData, std::vector<std::vector<float>>& hulls, google::protobuf::Arena* theAllocator) {
    auto* startRdf = mockBladeGirlSkillRdf(theAllocator);
    TestHelper::AddHullsToWsReq(initializerMapData, hulls, std::vector<bool>(hulls.size(), true), std::vector<bool>(hulls.size(), false));
    initializerMapData->set_allocated_self_parsed_rdf(startRdf);

    // incomingUpsyncSnapshotReqs21
    {
        int receivedTimerRdfId = 120;
        int receivedEdIfdId = 26; 
        int receivedStIfdId = 13;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(32);
        }
        
        incomingUpsyncSnapshotReqs21[receivedTimerRdfId] = req;
    }
    {
        int receivedTimerRdfId = 155;
        int receivedEdIfdId = 13;
        int receivedStIfdId = 0;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        int mid = 6;
        for (int ifdId = receivedStIfdId; ifdId < mid; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(32);
        }
        for (int ifdId = mid; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(32);
        }
        incomingUpsyncSnapshotReqs21[receivedTimerRdfId] = req;
    }
    {
        int receivedTimerRdfId = 500;
        int receivedEdIfdId = 60;
        int receivedStIfdId = 29;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        int mid = 44;
        for (int ifdId = receivedStIfdId; ifdId < mid; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(32);
        }
        for (int ifdId = mid; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(32);
        }
        incomingUpsyncSnapshotReqs21[receivedTimerRdfId] = req;
    }
    {
        int receivedTimerRdfId = 560;
        int receivedEdIfdId = 29;
        int receivedStIfdId = 26;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(32);
        }
        incomingUpsyncSnapshotReqs21[receivedTimerRdfId] = req;
    }
    // incomingDownsyncSnapshots21
    {
        int receivedTimerRdfId = 180;
        int receivedEdIfdId = 26;
        int receivedStIfdId = 0;
        DownsyncSnapshot* srvDownsyncSnapshot = google::protobuf::Arena::Create<DownsyncSnapshot>(theAllocator);
        srvDownsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            InputFrameDownsync* ifdBatch = srvDownsyncSnapshot->add_ifd_batch();
            ifdBatch->add_input_list(getSelfCmdByIfdId(testCmds1, ifdId));
            ifdBatch->add_input_list(32);
        }
        incomingDownsyncSnapshots21[receivedTimerRdfId] = srvDownsyncSnapshot;
    }
    {
        int receivedTimerRdfId = 570;
        int receivedEdIfdId = 58;
        int receivedStIfdId = 26;
        DownsyncSnapshot* srvDownsyncSnapshot = google::protobuf::Arena::Create<DownsyncSnapshot>(theAllocator);
        srvDownsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            InputFrameDownsync* ifdBatch = srvDownsyncSnapshot->add_ifd_batch();
            ifdBatch->add_input_list(getSelfCmdByIfdId(testCmds1, ifdId));
            ifdBatch->add_input_list(32);
        }
        incomingDownsyncSnapshots21[receivedTimerRdfId] = srvDownsyncSnapshot;
    }
}

void initTest22Data(WsReq* initializerMapData, std::vector<std::vector<float>>& hulls, google::protobuf::Arena* theAllocator) {
    auto chSpecies = globalPrimitiveConsts->ch_species();
    auto* startRdf = mockAimingRayTestRdf(theAllocator);
    TestHelper::AddHullsToWsReq(initializerMapData, hulls, std::vector<bool>(hulls.size(), true), std::vector<bool>(hulls.size(), false));

    initializerMapData->set_allocated_self_parsed_rdf(startRdf);
    auto triggerConfigFromTiled1 = initializerMapData->add_trigger_config_from_tile_list();
    triggerConfigFromTiled1->set_id(startRdf->triggers(0).id());
    triggerConfigFromTiled1->set_trt(startRdf->triggers(0).trt());
    triggerConfigFromTiled1->set_box_half_size_x(5.f);
    //triggerConfigFromTiled1->set_box_half_size_x(200.f);
    triggerConfigFromTiled1->set_box_half_size_y(200.f);
}

void initTest23Data(WsReq* initializerMapData, std::vector<std::vector<float>>& hulls, google::protobuf::Arena* theAllocator) {
    auto bulletCollisionTestStartRdf = mockBulletCollisionTestStartRdf(theAllocator);
    TestHelper::AddHullsToWsReq(initializerMapData, hulls, std::vector<bool>(hulls.size(), true), std::vector<bool>(hulls.size(), false));
    initializerMapData->set_allocated_self_parsed_rdf(bulletCollisionTestStartRdf);

    {
        int receivedEdIfdId = 2;
        int receivedStIfdId = 0;
        int receivedTimerRdfId = globalPrimitiveConsts->starting_render_frame_id()+2;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs23Intime[receivedTimerRdfId] = req;
    }
    {
        int receivedEdIfdId = 6;
        int receivedStIfdId = 2;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId)-1;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs23Intime[receivedTimerRdfId] = req;
    }
    {
        int receivedEdIfdId = 17;
        int receivedStIfdId = 6;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId)-1;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs23Intime[receivedTimerRdfId] = req;
    }
    {
        int receivedEdIfdId = 33;
        int receivedStIfdId = 17;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId) - 1;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(32);
        }
        incomingUpsyncSnapshotReqs23Intime[receivedTimerRdfId] = req;
    }
    {
        int receivedEdIfdId = 48;
        int receivedStIfdId = 33;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId) - 1;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs23Intime[receivedTimerRdfId] = req;
    }
    {
        int receivedEdIfdId = 60;
        int receivedStIfdId = 48;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId) - 1;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs23Intime[receivedTimerRdfId] = req;
    }
    {
        int receivedEdIfdId = 128;
        int receivedStIfdId = 60;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId) - 1;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs23Intime[receivedTimerRdfId] = req;
    }
/////////////////////////////////////////////////////////////////////////////////////////
    {
        int receivedTimerRdfId = 240;
        int receivedEdIfdId = 6;
        int receivedStIfdId = 2;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs23Rollback[receivedTimerRdfId] = req;
    }
    {
        int receivedTimerRdfId = 280;
        int receivedEdIfdId = 2;
        int receivedStIfdId = 0;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs23Rollback[receivedTimerRdfId] = req;
    }
    {
        int receivedTimerRdfId = 320;
        int receivedEdIfdId = 17;
        int receivedStIfdId = 6;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs23Rollback[receivedTimerRdfId] = req;
    }
    {
        int receivedTimerRdfId = 360;
        int receivedEdIfdId = 128;
        int receivedStIfdId = 60;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs23Rollback[receivedTimerRdfId] = req;
    }
    {
        int receivedTimerRdfId = 400;
        int receivedEdIfdId = 33;
        int receivedStIfdId = 17;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(32);
        }
        incomingUpsyncSnapshotReqs23Rollback[receivedTimerRdfId] = req;
    }
    {
        int receivedTimerRdfId = 420;
        int receivedEdIfdId = 60;
        int receivedStIfdId = 48;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs23Rollback[receivedTimerRdfId] = req;
    }
    {
        int receivedTimerRdfId = 480;
        int receivedEdIfdId = 48;
        int receivedStIfdId = 33;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs23Rollback[receivedTimerRdfId] = req;
    }
    {
        int receivedTimerRdfId = 640;
        int receivedEdIfdId = BaseBattle::ConvertToDelayedInputFrameId(1024-1)+1;
        int receivedStIfdId = 128;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs23Rollback[receivedTimerRdfId] = req;
    }
}

void initTest24Data(WsReq* initializerMapData, std::vector<std::vector<float>>& hulls, google::protobuf::Arena* theAllocator) {
    auto pistolMagazineTestStartRdf = mockPistolMagazineTestStartRdf(theAllocator);
    TestHelper::AddHullsToWsReq(initializerMapData, hulls, std::vector<bool>(hulls.size(), true), std::vector<bool>(hulls.size(), false));
    initializerMapData->set_allocated_self_parsed_rdf(pistolMagazineTestStartRdf);
}

void initTest25Data(WsReq* initializerMapData, std::vector<std::vector<float>>& hulls, google::protobuf::Arena* theAllocator) {
    auto startRdf = mockConveyorBeltTestStartRdf(theAllocator);
    TestHelper::AddHullsToWsReq(initializerMapData, hulls, std::vector<bool>(hulls.size(), true), std::vector<bool>(hulls.size(), false));
    initializerMapData->set_allocated_self_parsed_rdf(startRdf);

    auto trapConfigFromTiled1 = initializerMapData->add_trap_config_from_tile_list();
    Vec3 initVel1(+1.5f * globalPrimitiveConsts->battle_dynamics_fps(), 0, 0);
    trapConfigFromTiled1->set_id(startRdf->dynamic_traps(0).id());
    trapConfigFromTiled1->set_tpt(startRdf->dynamic_traps(0).tpt());
    trapConfigFromTiled1->set_linear_speed(initVel1.Length());
    trapConfigFromTiled1->set_box_half_size_x(256.f);
    trapConfigFromTiled1->set_box_half_size_y(64.f);
    trapConfigFromTiled1->set_init_q_x(0);
    trapConfigFromTiled1->set_init_q_y(0);
    trapConfigFromTiled1->set_init_q_z(0);
    trapConfigFromTiled1->set_init_q_w(1);

    trapConfigFromTiled1->set_init_vel_x(initVel1.GetX());
    trapConfigFromTiled1->set_init_vel_y(initVel1.GetY());
    trapConfigFromTiled1->set_init_vel_z(initVel1.GetZ());
    
    trapConfigFromTiled1->set_init_x(0);
    trapConfigFromTiled1->set_init_y(256);
    trapConfigFromTiled1->set_init_z(0);

    auto trapConfigFromTiled2 = initializerMapData->add_trap_config_from_tile_list();
    Vec3 initAngVel2(0, JPH_PI*0.25f, 0);
    Vec3 initHingeAxis = initAngVel2.Normalized();
    trapConfigFromTiled2->set_id(startRdf->dynamic_traps(1).id());
    trapConfigFromTiled2->set_tpt(startRdf->dynamic_traps(1).tpt());
    trapConfigFromTiled2->set_box_half_size_x(64.f);
    trapConfigFromTiled2->set_box_half_size_y(8.f);
    trapConfigFromTiled2->set_init_q_x(0);
    trapConfigFromTiled2->set_init_q_y(0);
    trapConfigFromTiled2->set_init_q_z(0);
    trapConfigFromTiled2->set_init_q_w(1);

    trapConfigFromTiled2->set_init_ang_vel_x(initAngVel2.GetX());
    trapConfigFromTiled2->set_init_ang_vel_y(initAngVel2.GetY());
    trapConfigFromTiled2->set_init_ang_vel_z(initAngVel2.GetZ());

    trapConfigFromTiled2->set_slider_axis_x(initHingeAxis.GetX());
    trapConfigFromTiled2->set_slider_axis_y(initHingeAxis.GetY());
    trapConfigFromTiled2->set_slider_axis_z(initHingeAxis.GetZ());

    trapConfigFromTiled2->set_init_x(320);
    trapConfigFromTiled2->set_init_y(512);
    trapConfigFromTiled2->set_init_z(0);

    trapConfigFromTiled2->set_limit_1(0.0f);
    trapConfigFromTiled2->set_limit_2(90.0f);
}

void initTest26Data(WsReq* initializerMapData, std::vector<std::vector<float>>& hulls, google::protobuf::Arena* theAllocator) {
    auto startRdf = mockCrouchTestStartRdf(theAllocator);
    TestHelper::AddHullsToWsReq(initializerMapData, hulls, std::vector<bool>(hulls.size(), true), std::vector<bool>(hulls.size(), false));
    initializerMapData->set_allocated_self_parsed_rdf(startRdf);

    auto trapConfigFromTiled1 = initializerMapData->add_trap_config_from_tile_list();
    Vec3 initVel(3.f * globalPrimitiveConsts->battle_dynamics_fps(), 0, 0);
    trapConfigFromTiled1->set_id(startRdf->dynamic_traps(0).id());
    trapConfigFromTiled1->set_tpt(startRdf->dynamic_traps(0).tpt());
    trapConfigFromTiled1->set_linear_speed(initVel.Length());
    trapConfigFromTiled1->set_box_half_size_x(30.f);
    trapConfigFromTiled1->set_box_half_size_y(12.f);
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

    trapConfigFromTiled1->set_init_x(700);
    trapConfigFromTiled1->set_init_y(188);
    trapConfigFromTiled1->set_init_z(0);

    trapConfigFromTiled1->set_cooldown_rdf_count(120);
    trapConfigFromTiled1->set_limit_1(-100.0f);
    trapConfigFromTiled1->set_limit_2(+100.0f);
}

void initTest27Data(WsReq* initializerMapData, std::vector<std::vector<float>>& hulls, google::protobuf::Arena* theAllocator) {
    auto startRdf = mockBulletHitReactionTestStartRdf(theAllocator);
    TestHelper::AddHullsToWsReq(initializerMapData, hulls, std::vector<bool>(hulls.size(), true), std::vector<bool>(hulls.size(), false));
    initializerMapData->set_allocated_self_parsed_rdf(startRdf);
}

void initTest28Data(WsReq* initializerMapData, std::vector<std::vector<float>>& hulls, google::protobuf::Arena* theAllocator) {
    auto startRdf = mockBlackShooterTestStartRdf(theAllocator);
    TestHelper::AddHullsToWsReq(initializerMapData, hulls, std::vector<bool>(hulls.size(), true), std::vector<bool>(hulls.size(), false));
    initializerMapData->set_allocated_self_parsed_rdf(startRdf);
}

void initTest29Data(WsReq* initializerMapData, std::vector<std::vector<float>>& hulls, google::protobuf::Arena* theAllocator) {
    auto startRdf = mockBlackThrowerTestStartRdf(theAllocator);
    TestHelper::AddHullsToWsReq(initializerMapData, hulls, std::vector<bool>(hulls.size(), true), std::vector<bool>(hulls.size(), false));
    initializerMapData->set_allocated_self_parsed_rdf(startRdf);
}

void initTest30Data(WsReq* initializerMapData, std::vector<std::vector<float>>& hulls, google::protobuf::Arena* theAllocator) {
    auto startRdf = mockDeadBulletLeftShiftTestStartRdf(theAllocator);
    TestHelper::AddHullsToWsReq(initializerMapData, hulls, std::vector<bool>(hulls.size(), true), std::vector<bool>(hulls.size(), false));
    initializerMapData->set_allocated_self_parsed_rdf(startRdf);
}

void initTest31Data(WsReq* initializerMapData, std::vector<std::vector<float>>& hulls, google::protobuf::Arena* theAllocator) {
    auto chSpecies = globalPrimitiveConsts->ch_species();
    auto startRdf = mockNpcSpawnerRdf2(theAllocator);
    TestHelper::AddHullsToWsReq(initializerMapData, hulls, std::vector<bool>(hulls.size(), true), std::vector<bool>(hulls.size(), false));

    initializerMapData->set_allocated_self_parsed_rdf(startRdf);
    auto triggerConfigFromTiled1 = initializerMapData->add_trigger_config_from_tile_list();
    triggerConfigFromTiled1->set_id(startRdf->triggers(0).id());
    triggerConfigFromTiled1->set_trt(startRdf->triggers(0).trt());
    triggerConfigFromTiled1->set_box_half_size_x(200.f);
    triggerConfigFromTiled1->set_box_half_size_y(50.f);
    triggerConfigFromTiled1->set_quota(3);
    triggerConfigFromTiled1->set_publishing_to_trigger_id_upon_exhausted(startRdf->triggers(2).id());
    triggerConfigFromTiled1->set_recovery_frames(1);

    auto triggerConfigFromTiled2 = initializerMapData->add_trigger_config_from_tile_list();
    triggerConfigFromTiled2->set_id(startRdf->triggers(1).id());
    triggerConfigFromTiled2->set_trt(startRdf->triggers(1).trt());
    triggerConfigFromTiled2->set_delayed_frames((globalPrimitiveConsts->battle_dynamics_fps() >> 1));
    triggerConfigFromTiled2->set_recovery_frames(5*globalPrimitiveConsts->battle_dynamics_fps()); 
    triggerConfigFromTiled2->set_bullet_team_id(3);
    triggerConfigFromTiled2->set_sub_cycle_trigger_frames(20);
    triggerConfigFromTiled2->set_init_q_x(0);
    triggerConfigFromTiled2->set_init_q_y(1);
    triggerConfigFromTiled2->set_init_q_z(0);
    triggerConfigFromTiled2->set_init_q_w(0);
    triggerConfigFromTiled2->set_new_revival_x(startRdf->triggers(1).x());
    triggerConfigFromTiled2->set_new_revival_y(startRdf->triggers(1).y());
    triggerConfigFromTiled2->set_publishing_to_trigger_id_upon_exhausted(startRdf->triggers(0).id());

    // Its own quota is 3 and it subscribes to a "trts().by_movement" whose "quota" is also 3, this "indi_wave_npc_spawner" will use all its "characterSpawnerTimeSeq"
    auto* mutableChSpanwerTimeSeq2_1 = triggerConfigFromTiled2->add_character_spawner_time_seq(); 
    mutableChSpanwerTimeSeq2_1->set_cutoff_rdf_id(1);
    mutableChSpanwerTimeSeq2_1->add_species_id_list(chSpecies.blacksaber_test_with_vision());
    mutableChSpanwerTimeSeq2_1->add_species_id_list(chSpecies.blacksaber_test_with_vision());

    auto* mutableChSpanwerTimeSeq2_2 = triggerConfigFromTiled2->add_character_spawner_time_seq(); 
    mutableChSpanwerTimeSeq2_2->set_cutoff_rdf_id(2);
    mutableChSpanwerTimeSeq2_2->add_species_id_list(chSpecies.blacksaber_test_with_vision());
    mutableChSpanwerTimeSeq2_2->add_species_id_list(chSpecies.blacksaber_test_with_vision());
    mutableChSpanwerTimeSeq2_2->add_species_id_list(chSpecies.blacksaber_test_with_vision());
    mutableChSpanwerTimeSeq2_2->add_species_id_list(chSpecies.blacksaber_test_with_vision());

    auto* mutableChSpanwerTimeSeq2_3 = triggerConfigFromTiled2->add_character_spawner_time_seq(); 
    mutableChSpanwerTimeSeq2_3->set_cutoff_rdf_id(3);
    mutableChSpanwerTimeSeq2_3->add_species_id_list(chSpecies.blacksaber_test_with_vision());

    auto triggerConfigFromTiled3 = initializerMapData->add_trigger_config_from_tile_list();
    triggerConfigFromTiled3->set_id(startRdf->triggers(2).id());
    triggerConfigFromTiled3->set_trt(startRdf->triggers(2).trt());
}

void initTest32Data(WsReq* initializerMapData, std::vector<std::vector<float>>& hulls, google::protobuf::Arena* theAllocator) {
    auto startRdf = mockRotatingPlatformForceCrouchingTestStartRdf(theAllocator);
    TestHelper::AddHullsToWsReq(initializerMapData, hulls, std::vector<bool>(hulls.size(), true), std::vector<bool>(hulls.size(), false));
    initializerMapData->set_allocated_self_parsed_rdf(startRdf);

    auto trapConfigFromTiled1 = initializerMapData->add_trap_config_from_tile_list();
    Vec3 initAngVel1(0, 0, -0.370796325);
    trapConfigFromTiled1->set_id(startRdf->dynamic_traps(0).id());
    trapConfigFromTiled1->set_tpt(startRdf->dynamic_traps(0).tpt());
    trapConfigFromTiled1->set_box_half_size_x(48.f);
    trapConfigFromTiled1->set_box_half_size_y(8.f);
    trapConfigFromTiled1->set_init_q_x(0);
    trapConfigFromTiled1->set_init_q_y(0);
    trapConfigFromTiled1->set_init_q_z(0);
    trapConfigFromTiled1->set_init_q_w(1);

    trapConfigFromTiled1->set_init_ang_vel_x(initAngVel1.GetX());
    trapConfigFromTiled1->set_init_ang_vel_y(initAngVel1.GetY());
    trapConfigFromTiled1->set_init_ang_vel_z(initAngVel1.GetZ());

    trapConfigFromTiled1->set_init_x(0);
    trapConfigFromTiled1->set_init_y(154);
    trapConfigFromTiled1->set_init_z(0);
}

void initTest33Data(WsReq* initializerMapData, std::vector<std::vector<float>>& hulls, google::protobuf::Arena* theAllocator) {
    auto startRdf = mockRotatingPlatformForceCrouchingTestStartRdf2(theAllocator);
    TestHelper::AddHullsToWsReq(initializerMapData, hulls, std::vector<bool>(hulls.size(), true), std::vector<bool>(hulls.size(), false));
    initializerMapData->set_allocated_self_parsed_rdf(startRdf);

    auto trapConfigFromTiled1 = initializerMapData->add_trap_config_from_tile_list();
    Vec3 initAngVel1(0, 0, 0.25);
    trapConfigFromTiled1->set_id(startRdf->dynamic_traps(0).id());
    trapConfigFromTiled1->set_tpt(startRdf->dynamic_traps(0).tpt());
    trapConfigFromTiled1->set_box_half_size_x(48.f);
    trapConfigFromTiled1->set_box_half_size_y(8.f);
    trapConfigFromTiled1->set_init_q_x(0);
    trapConfigFromTiled1->set_init_q_y(0);
    trapConfigFromTiled1->set_init_q_z(0);
    trapConfigFromTiled1->set_init_q_w(1);

    trapConfigFromTiled1->set_init_ang_vel_x(initAngVel1.GetX());
    trapConfigFromTiled1->set_init_ang_vel_y(initAngVel1.GetY());
    trapConfigFromTiled1->set_init_ang_vel_z(initAngVel1.GetZ());

    trapConfigFromTiled1->set_init_x(0);
    trapConfigFromTiled1->set_init_y(154);
    trapConfigFromTiled1->set_init_z(0);
}

void initTest34Data(WsReq* initializerMapData, std::vector<std::vector<float>>& hulls, google::protobuf::Arena* theAllocator) {
    auto startRdf = mockBtnFTestStartRdf(theAllocator);
    TestHelper::AddHullsToWsReq(initializerMapData, hulls, std::vector<bool>(hulls.size(), true), std::vector<bool>(hulls.size(), false));
    initializerMapData->set_allocated_self_parsed_rdf(startRdf);

    auto triggerConfigFromTiled1 = initializerMapData->add_trigger_config_from_tile_list();
    triggerConfigFromTiled1->set_id(startRdf->triggers(0).id());
    triggerConfigFromTiled1->set_trt(startRdf->triggers(0).trt());
    triggerConfigFromTiled1->set_box_half_size_x(30.f);
    triggerConfigFromTiled1->set_box_half_size_y(50.f);
    triggerConfigFromTiled1->set_quota(2);
    triggerConfigFromTiled1->set_recovery_frames(3 * globalPrimitiveConsts->battle_dynamics_fps());

    auto triggerConfigFromTiled2 = initializerMapData->add_trigger_config_from_tile_list();
    triggerConfigFromTiled2->set_id(startRdf->triggers(1).id());
    triggerConfigFromTiled2->set_trt(startRdf->triggers(1).trt());
    triggerConfigFromTiled2->set_delayed_frames((globalPrimitiveConsts->battle_dynamics_fps() >> 1));
    triggerConfigFromTiled2->set_sub_cycle_trigger_frames(20);
    triggerConfigFromTiled2->set_init_q_x(0);
    triggerConfigFromTiled2->set_init_q_y(1);
    triggerConfigFromTiled2->set_init_q_z(0);
    triggerConfigFromTiled2->set_init_q_w(0);
    triggerConfigFromTiled2->set_new_revival_x(startRdf->triggers(1).x());
    triggerConfigFromTiled2->set_new_revival_y(startRdf->triggers(1).y());
    triggerConfigFromTiled2->set_publishing_to_trigger_id_upon_exhausted(startRdf->triggers(0).id());

    auto* mutablePkSpanwerTimeSeq2_1 = triggerConfigFromTiled2->add_pickable_spawner_time_seq();
    mutablePkSpanwerTimeSeq2_1->set_cutoff_rdf_id(1);
    mutablePkSpanwerTimeSeq2_1->add_pickup_type_list(globalPrimitiveConsts->pkts().hp_small());
    mutablePkSpanwerTimeSeq2_1->add_init_op_list(0);

    auto* mutablePkSpanwerTimeSeq2_2 = triggerConfigFromTiled2->add_pickable_spawner_time_seq();
    mutablePkSpanwerTimeSeq2_2->set_cutoff_rdf_id(2);
    mutablePkSpanwerTimeSeq2_2->add_pickup_type_list(globalPrimitiveConsts->pkts().hp_small());
    mutablePkSpanwerTimeSeq2_2->add_init_op_list(0);
    mutablePkSpanwerTimeSeq2_2->add_pickup_type_list(globalPrimitiveConsts->pkts().mp_small());
    mutablePkSpanwerTimeSeq2_2->add_init_op_list(0);

    auto triggerConfigFromTiled3 = initializerMapData->add_trigger_config_from_tile_list();
    triggerConfigFromTiled3->set_id(startRdf->triggers(2).id());
    triggerConfigFromTiled3->set_trt(startRdf->triggers(2).trt());
    triggerConfigFromTiled3->set_delayed_frames((globalPrimitiveConsts->battle_dynamics_fps() >> 1));
    triggerConfigFromTiled3->set_sub_cycle_trigger_frames(20);
    triggerConfigFromTiled3->set_init_q_x(0);
    triggerConfigFromTiled3->set_init_q_y(1);
    triggerConfigFromTiled3->set_init_q_z(0);
    triggerConfigFromTiled3->set_init_q_w(0);
    triggerConfigFromTiled3->set_new_revival_x(startRdf->triggers(2).x());
    triggerConfigFromTiled3->set_new_revival_y(startRdf->triggers(2).y());
    triggerConfigFromTiled3->set_publishing_to_trigger_id_upon_exhausted(startRdf->triggers(0).id());

    auto* mutablePkSpanwerTimeSeq3_1 = triggerConfigFromTiled3->add_pickable_spawner_time_seq();
    mutablePkSpanwerTimeSeq3_1->set_cutoff_rdf_id(1);
    mutablePkSpanwerTimeSeq3_1->add_pickup_type_list(globalPrimitiveConsts->pkts().hp_small());
    mutablePkSpanwerTimeSeq3_1->add_init_op_list(3);
}

void initTest35Data(WsReq* initializerMapData, std::vector<std::vector<float>>& hulls, google::protobuf::Arena* theAllocator) {
    auto startRdf = mockBtnFTestStartRdf(theAllocator);
    TestHelper::AddHullsToWsReq(initializerMapData, hulls, std::vector<bool>(hulls.size(), true), std::vector<bool>(hulls.size(), false));
    initializerMapData->set_allocated_self_parsed_rdf(startRdf);

    auto triggerConfigFromTiled1 = initializerMapData->add_trigger_config_from_tile_list();
    triggerConfigFromTiled1->set_id(startRdf->triggers(0).id());
    triggerConfigFromTiled1->set_trt(startRdf->triggers(0).trt());
    triggerConfigFromTiled1->set_box_half_size_x(30.f);
    triggerConfigFromTiled1->set_box_half_size_y(50.f);
    triggerConfigFromTiled1->set_quota(2);
    triggerConfigFromTiled1->set_recovery_frames(3 * globalPrimitiveConsts->battle_dynamics_fps());

    auto triggerConfigFromTiled2 = initializerMapData->add_trigger_config_from_tile_list();
    triggerConfigFromTiled2->set_id(startRdf->triggers(1).id());
    triggerConfigFromTiled2->set_trt(startRdf->triggers(1).trt());
    triggerConfigFromTiled2->set_delayed_frames((globalPrimitiveConsts->battle_dynamics_fps() >> 1));
    triggerConfigFromTiled2->set_sub_cycle_trigger_frames(20);
    triggerConfigFromTiled2->set_init_q_x(0);
    triggerConfigFromTiled2->set_init_q_y(1);
    triggerConfigFromTiled2->set_init_q_z(0);
    triggerConfigFromTiled2->set_init_q_w(0);
    triggerConfigFromTiled2->set_new_revival_x(startRdf->triggers(1).x());
    triggerConfigFromTiled2->set_new_revival_y(startRdf->triggers(1).y());
    triggerConfigFromTiled2->set_publishing_to_trigger_id_upon_exhausted(startRdf->triggers(0).id());

    auto* mutablePkSpanwerTimeSeq2_1 = triggerConfigFromTiled2->add_pickable_spawner_time_seq();
    mutablePkSpanwerTimeSeq2_1->set_cutoff_rdf_id(1);
    mutablePkSpanwerTimeSeq2_1->add_pickup_type_list(globalPrimitiveConsts->pkts().hp_small());
    mutablePkSpanwerTimeSeq2_1->add_init_op_list(3);

    auto* mutablePkSpanwerTimeSeq2_2 = triggerConfigFromTiled2->add_pickable_spawner_time_seq();
    mutablePkSpanwerTimeSeq2_2->set_cutoff_rdf_id(2);
    mutablePkSpanwerTimeSeq2_2->add_pickup_type_list(globalPrimitiveConsts->pkts().hp_small());
    mutablePkSpanwerTimeSeq2_2->add_init_op_list(0);
    mutablePkSpanwerTimeSeq2_2->add_pickup_type_list(globalPrimitiveConsts->pkts().mp_small());
    mutablePkSpanwerTimeSeq2_2->add_init_op_list(0);

    auto triggerConfigFromTiled3 = initializerMapData->add_trigger_config_from_tile_list();
    triggerConfigFromTiled3->set_id(startRdf->triggers(2).id());
    triggerConfigFromTiled3->set_trt(startRdf->triggers(2).trt());
    triggerConfigFromTiled3->set_delayed_frames((globalPrimitiveConsts->battle_dynamics_fps() >> 1));
    triggerConfigFromTiled3->set_sub_cycle_trigger_frames(20);
    triggerConfigFromTiled3->set_init_q_x(0);
    triggerConfigFromTiled3->set_init_q_y(1);
    triggerConfigFromTiled3->set_init_q_z(0);
    triggerConfigFromTiled3->set_init_q_w(0);
    triggerConfigFromTiled3->set_new_revival_x(startRdf->triggers(2).x());
    triggerConfigFromTiled3->set_new_revival_y(startRdf->triggers(2).y());
    triggerConfigFromTiled3->set_publishing_to_trigger_id_upon_exhausted(startRdf->triggers(0).id());

    auto* mutablePkSpanwerTimeSeq3_1 = triggerConfigFromTiled3->add_pickable_spawner_time_seq();
    mutablePkSpanwerTimeSeq3_1->set_cutoff_rdf_id(1);
    mutablePkSpanwerTimeSeq3_1->add_pickup_type_list(globalPrimitiveConsts->pkts().hp_small());
    mutablePkSpanwerTimeSeq3_1->add_init_op_list(4);
}

void initTest36Data(WsReq* initializerMapData, std::vector<std::vector<float>>& hulls, google::protobuf::Arena* theAllocator) {
    auto startRdf = mockTriggerTrapInteractionTestStartRdf(theAllocator);
    TestHelper::AddHullsToWsReq(initializerMapData, hulls, std::vector<bool>(hulls.size(), true), std::vector<bool>(hulls.size(), false));
    initializerMapData->set_allocated_self_parsed_rdf(startRdf);

    auto triggerConfigFromTiled1 = initializerMapData->add_trigger_config_from_tile_list();
    triggerConfigFromTiled1->set_id(startRdf->triggers(0).id());
    triggerConfigFromTiled1->set_trt(startRdf->triggers(0).trt());
    triggerConfigFromTiled1->set_delayed_frames((globalPrimitiveConsts->battle_dynamics_fps() >> 1));
    triggerConfigFromTiled1->set_sub_cycle_trigger_frames(20);
    triggerConfigFromTiled1->set_init_q_x(0);
    triggerConfigFromTiled1->set_init_q_y(0);
    triggerConfigFromTiled1->set_init_q_z(0);
    triggerConfigFromTiled1->set_init_q_w(1);
    triggerConfigFromTiled1->set_quota(12);
    triggerConfigFromTiled1->set_box_half_size_x(30.0f);
    triggerConfigFromTiled1->set_box_half_size_y(50.0f);
    triggerConfigFromTiled1->set_recovery_frames(120);
    triggerConfigFromTiled1->set_new_revival_x(startRdf->triggers(0).x());
    triggerConfigFromTiled1->set_new_revival_y(startRdf->triggers(0).y());

    auto trapConfigFromTiled1 = initializerMapData->add_trap_config_from_tile_list();
    Vec3 initAngVel1(0, JPH_PI*0.25f, 0);
    Vec3 initHingeAxis = initAngVel1.Normalized();
    trapConfigFromTiled1->set_id(startRdf->dynamic_traps(0).id());
    trapConfigFromTiled1->set_tpt(startRdf->dynamic_traps(0).tpt());
    trapConfigFromTiled1->set_box_half_size_x(64.f);
    trapConfigFromTiled1->set_box_half_size_y(8.f);
    trapConfigFromTiled1->set_init_q_x(0);
    trapConfigFromTiled1->set_init_q_y(0);
    trapConfigFromTiled1->set_init_q_z(0);
    trapConfigFromTiled1->set_init_q_w(1);

    trapConfigFromTiled1->set_init_ang_vel_x(initAngVel1.GetX());
    trapConfigFromTiled1->set_init_ang_vel_y(initAngVel1.GetY());
    trapConfigFromTiled1->set_init_ang_vel_z(initAngVel1.GetZ());

    trapConfigFromTiled1->set_slider_axis_x(initHingeAxis.GetX());
    trapConfigFromTiled1->set_slider_axis_y(initHingeAxis.GetY());
    trapConfigFromTiled1->set_slider_axis_z(initHingeAxis.GetZ());

    trapConfigFromTiled1->set_init_x(320);
    trapConfigFromTiled1->set_init_y(512);
    trapConfigFromTiled1->set_init_z(0);

    trapConfigFromTiled1->set_limit_1(0.0f);
    trapConfigFromTiled1->set_limit_2(90.0f);

    trapConfigFromTiled1->set_subscribes_to_trigger_id(startRdf->triggers(0).id());
}

void initTest37Data(WsReq* initializerMapData, std::vector<std::vector<float>>& hulls, google::protobuf::Arena* theAllocator) {
    auto startRdf = mockDef1Rdf(theAllocator);
    TestHelper::AddHullsToWsReq(initializerMapData, hulls, std::vector<bool>(hulls.size(), true), std::vector<bool>(hulls.size(), false));
    initializerMapData->set_allocated_self_parsed_rdf(startRdf);
    {
        int receivedEdIfdId = 2;
        int receivedStIfdId = 0;
        int receivedTimerRdfId = globalPrimitiveConsts->starting_render_frame_id() + 2;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs37Intime[receivedTimerRdfId] = req;
    }
    {
        int receivedEdIfdId = 6;
        int receivedStIfdId = 2;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId) - 1;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs37Intime[receivedTimerRdfId] = req;
    }
    {
        int receivedEdIfdId = 17;
        int receivedStIfdId = 6;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId) - 1;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs37Intime[receivedTimerRdfId] = req;
    }
    {
        int receivedEdIfdId = 33;
        int receivedStIfdId = 17;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId) - 1;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(32);
        }
        incomingUpsyncSnapshotReqs37Intime[receivedTimerRdfId] = req;
    }
    {
        int receivedEdIfdId = 48;
        int receivedStIfdId = 33;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId) - 1;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(32);
        }
        incomingUpsyncSnapshotReqs37Intime[receivedTimerRdfId] = req;
    }
    {
        int receivedEdIfdId = 53;
        int receivedStIfdId = 48;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId) - 1;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs37Intime[receivedTimerRdfId] = req;
    }
    {
        int receivedEdIfdId = 58;
        int receivedStIfdId = 57;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId) - 1;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(32);
        }
        incomingUpsyncSnapshotReqs37Intime[receivedTimerRdfId] = req;
    }
    {
        int receivedEdIfdId = 70;
        int receivedStIfdId = 58;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId) - 1;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs37Intime[receivedTimerRdfId] = req;
    }
    {
        int receivedEdIfdId = 71;
        int receivedStIfdId = 70;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId) - 1;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(32);
        }
        incomingUpsyncSnapshotReqs37Intime[receivedTimerRdfId] = req;
    }
    {
        int receivedEdIfdId = 128;
        int receivedStIfdId = 71;
        int receivedTimerRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(receivedStIfdId) - 1;
        WsReq* req = google::protobuf::Arena::Create<WsReq>(theAllocator);
        req->set_join_index(2);
        auto peerUpsyncSnapshot = req->mutable_upsync_snapshot();
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(0);
        }
        incomingUpsyncSnapshotReqs37Intime[receivedTimerRdfId] = req;
    }
}

void initTest38Data(WsReq* initializerMapData, std::vector<std::vector<float>>& hulls, google::protobuf::Arena* theAllocator) {
    auto startRdf = mockBat1TestStartRdf(theAllocator);
    TestHelper::AddHullsToWsReq(initializerMapData, hulls, std::vector<bool>(hulls.size(), true), std::vector<bool>(hulls.size(), false));
    initializerMapData->set_allocated_self_parsed_rdf(startRdf);
}

std::string outStr;
std::string player1OutStr, player2OutStr;
std::string referencePlayer1OutStr, referencePlayer2OutStr;
bool runTestCase1(FrontendBattle* reusedBattle, std::vector<std::vector<float>>& hulls, int inSingleJoinIndex, google::protobuf::Arena* theAllocator) {
    WsReq* initializerMapData = google::protobuf::Arena::Create<WsReq>(theAllocator);
    initTest1Data(initializerMapData, hulls, theAllocator);
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);
    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 1024;
    int printIntervalRdfCnt = (1 << 4);
    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    jtshared::RenderFrame* outRdf = google::protobuf::Arena::Create<RenderFrame>(theAllocator);
    int newLcacIfdId = -1, newUdpLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    int newChaserRdfId = 0, newReferenceBattleChaserRdfId = 0;
    while (loopRdfCnt > outerTimerRdfId) {
        bool shouldPrint = false;
        // Handling TCP packets first, and then UDP packets, the same as C# side behavior.
        if (incomingDownsyncSnapshots1.count(outerTimerRdfId)) {
            DownsyncSnapshot* srvDownsyncSnapshot = incomingDownsyncSnapshots1[outerTimerRdfId];
            int outPostTimerRdfEvictedCnt = 0, outPostTimerRdfDelayedIfdEvictedCnt = 0;
            bool applied = reusedBattle->OnDownsyncSnapshotReceived(srvDownsyncSnapshot, &outPostTimerRdfEvictedCnt, &outPostTimerRdfDelayedIfdEvictedCnt, &newChaserRdfId, &newLcacIfdId, &newUdpLcacIfdId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
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
            bool applied = reusedBattle->OnUpsyncSnapshotReceived(peerJoinIndex, peerUpsyncSnapshot, &newChaserRdfId, &newUdpLcacIfdId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
            if (13 == peerUpsyncSnapshot.st_ifd_id()) {
                JPH_ASSERT(BaseBattle::ConvertToFirstUsedRenderFrameId(13) == newChaserRdfId);
                JPH_ASSERT(-1 == reusedBattle->lcacIfdId);
            } else if (0 == peerUpsyncSnapshot.st_ifd_id()) {
                JPH_ASSERT(2 == newChaserRdfId);
                JPH_ASSERT(-1 == reusedBattle->lcacIfdId);
            } else if (29 == peerUpsyncSnapshot.st_ifd_id()) {
                JPH_ASSERT(118 == newChaserRdfId);
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

        if (231 <= outerTimerRdfId && outerTimerRdfId < 251) {
            //shouldPrint = true;
        }

        if (680 <= outerTimerRdfId && outerTimerRdfId < 705) {
            //shouldPrint = true;
        }
        
        if (770 <= outerTimerRdfId && outerTimerRdfId < 1000) {
            //shouldPrint = true;
        }
        
        if (shouldPrint) {
            std::cout << "TestCase1/outerTimerRdfId=" << outerTimerRdfId << "\n\tp1Chd ud1=" << ud1 << ", cs=" << p1Chd.ch_state() << ", fc=" << p1Chd.frames_in_ch_state() << ", dir=(" << p1Chd.q_x() << ", " << p1Chd.q_y() << ", " << p1Chd.q_z() << ", " << p1Chd.q_w() << "), pos=(" << p1Chd.x() << ", " << p1Chd.y() << "), vel=(" << p1Chd.vel_x() << ", " << p1Chd.vel_y() << "), gud=" << p1Chd.ground_ud() << "\n\tp2Chd ud2=" << ud2 << ", cs=" << p2Chd.ch_state() << ", fc=" << p2Chd.frames_in_ch_state() << ", dir=(" << p2Chd.q_x() << ", " << p2Chd.q_y() << ", " << p2Chd.q_z() << ", " << p2Chd.q_w() << "), pos=(" << p2Chd.x() << ", " << p2Chd.y() << "), vel=(" << p2Chd.vel_x() << ", " << p2Chd.vel_y() << ")" << std::endl;
        }

        if (20 == outerTimerRdfId) {
            JPH_ASSERT(CharacterState::InAirIdle1NoJump == p1Chd.ch_state());
        } else if (62 == outerTimerRdfId) {
            JPH_ASSERT(CharacterState::Idle1 == p1Chd.ch_state());
        } else if (63 <= outerTimerRdfId && outerTimerRdfId <= 80) {
            int p1Expectedfc = outerTimerRdfId - 63;
            const Skill* skill = nullptr;
            const BulletConfig* bulletConfig = nullptr;
            uint32_t p1ChdActiveSkillId = p1Chd.active_skill_id();
            int p1ChdActiveSkillHit = p1Chd.active_skill_hit();
            BaseBattle::FindBulletConfig(p1ChdActiveSkillId, p1ChdActiveSkillHit, skill, bulletConfig);
            JPH_ASSERT(nullptr != skill && nullptr != bulletConfig);
            int p1ExpectedFramesToRecover = skill->recovery_frames() - (p1Expectedfc);
            JPH_ASSERT(CharacterState::Atk1 == p1Chd.ch_state());
            JPH_ASSERT(p1Expectedfc == p1Chd.frames_in_ch_state());
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
    theAllocator->Reset();
    reusedBattle->Clear();   
    return true;
}

bool runTestCase2(FrontendBattle* reusedBattle, std::vector<std::vector<float>>& hulls, int inSingleJoinIndex, google::protobuf::Arena* theAllocator) {
    WsReq* initializerMapData = google::protobuf::Arena::Create<WsReq>(theAllocator);
    initTest2Data(initializerMapData, hulls, theAllocator);
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);
    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 1024;
    int printIntervalRdfCnt = (1 << 30);
    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    int newLcacIfdId = -1, newUdpLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    int newChaserRdfId = 0, newReferenceBattleChaserRdfId = 0;
    jtshared::RenderFrame* outRdf = google::protobuf::Arena::Create<RenderFrame>(theAllocator);
    while (loopRdfCnt > outerTimerRdfId) {
        bool shouldPrint = false;
        // Handling TCP packets first, and then UDP packets, the same as C# side behavior.
        if (incomingDownsyncSnapshots2.count(outerTimerRdfId)) {
            DownsyncSnapshot* srvDownsyncSnapshot = incomingDownsyncSnapshots2[outerTimerRdfId];
            int outPostTimerRdfEvictedCnt = 0, outPostTimerRdfDelayedIfdEvictedCnt = 0;
            bool applied = reusedBattle->OnDownsyncSnapshotReceived(srvDownsyncSnapshot, &outPostTimerRdfEvictedCnt, &outPostTimerRdfDelayedIfdEvictedCnt, &newChaserRdfId, &newLcacIfdId, &newUdpLcacIfdId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
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
        }
        if (incomingUpsyncSnapshotReqs2.count(outerTimerRdfId)) {
            auto req = incomingUpsyncSnapshotReqs2[outerTimerRdfId];
            auto peerUpsyncSnapshot = req->upsync_snapshot();
            bool applied = reusedBattle->OnUpsyncSnapshotReceived(req->join_index(), peerUpsyncSnapshot, &newChaserRdfId, &newUdpLcacIfdId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
            if (13 == peerUpsyncSnapshot.st_ifd_id()) {
                JPH_ASSERT(BaseBattle::ConvertToFirstUsedRenderFrameId(13) == newChaserRdfId);
                JPH_ASSERT(-1 == reusedBattle->lcacIfdId);
            } else if (0 == peerUpsyncSnapshot.st_ifd_id()) {
                JPH_ASSERT(BaseBattle::ConvertToFirstUsedRenderFrameId(0) == newChaserRdfId);
                JPH_ASSERT(-1 == reusedBattle->lcacIfdId);
            } else if (29 == peerUpsyncSnapshot.st_ifd_id()) {
                JPH_ASSERT(118 == newChaserRdfId);
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

        if (outerTimerRdfId <= 100) {
            //shouldPrint = true;
        }
        
        if (shouldPrint) {
            std::cout << "TestCase2/outerTimerRdfId=" << outerTimerRdfId << "\n\tp1Chd ud1=" << ud1 << ", cs=" << p1Chd.ch_state() << ", fc=" << p1Chd.frames_in_ch_state() << ", dir=(" << p1Chd.q_x() << ", " << p1Chd.q_y() << ", " << p1Chd.q_z() << ", " << p1Chd.q_w() << "), pos=(" << p1Chd.x() << ", " << p1Chd.y() << "), vel=(" << p1Chd.vel_x() << ", " << p1Chd.vel_y() << "), gud=" << p1Chd.ground_ud() << "\n\tp2Chd ud2=" << ud2 << ", cs=" << p2Chd.ch_state() << ", fc=" << p2Chd.frames_in_ch_state() << ", dir=(" << p2Chd.q_x() << ", " << p2Chd.q_y() << ", " << p2Chd.q_z() << ", " << p2Chd.q_w() << "), pos=(" << p2Chd.x() << ", " << p2Chd.y() << "), vel=(" << p2Chd.vel_x() << ", " << p2Chd.vel_y() << ")" << std::endl;
        }

        if (20 == outerTimerRdfId) {
            JPH_ASSERT(CharacterState::InAirIdle1NoJump == p1Chd.ch_state());
        } else if (62 == outerTimerRdfId) {
            JPH_ASSERT(CharacterState::Walking == p1Chd.ch_state());
        } else if (63 <= outerTimerRdfId && outerTimerRdfId <= 80) {
            JPH_ASSERT(CharacterState::WalkingAtk1 == p1Chd.ch_state());
            int p1Expectedfc = outerTimerRdfId - 63;
            const Skill* skill = nullptr;
            const BulletConfig* bulletConfig = nullptr;
            uint32_t p1ChdActiveSkillId = p1Chd.active_skill_id();
            int p1ChdActiveSkillHit = p1Chd.active_skill_hit();
            BaseBattle::FindBulletConfig(p1ChdActiveSkillId, p1ChdActiveSkillHit, skill, bulletConfig);
            JPH_ASSERT(nullptr != skill && nullptr != bulletConfig);
        } else if (81 == outerTimerRdfId) {
            JPH_ASSERT(CharacterState::Idle1 == p1Chd.ch_state());
        } else if (326 <= outerTimerRdfId && outerTimerRdfId <= 340) {
            JPH_ASSERT(CharacterState::WalkingAtk1 == p1Chd.ch_state());
        }
        
        outerTimerRdfId++;
    }

    std::cout << "Passed TestCase2\n" << std::endl;
    theAllocator->Reset();
    reusedBattle->Clear();   
    return true;
}

bool runTestCase3(FrontendBattle* reusedBattle, std::vector<std::vector<float>>& hulls, int inSingleJoinIndex, google::protobuf::Arena* theAllocator) {
    WsReq* initializerMapData = google::protobuf::Arena::Create<WsReq>(theAllocator);
    initTest3Data(initializerMapData, hulls, theAllocator);
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);
    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 1024;
    int printIntervalRdfCnt = (1 << 30);
    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    int lastSentIfdId = -1;
    int timerRdfId = -1, chaserRdfIdLowerBound = -1, oldLcacIfdId = -1, oldUdpLcacIfdId = -1, toGenIfdId = -1, localRequiredIfdId = -1;
    int newLcacIfdId = -1, newUdpLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    int newChaserRdfId = 0, newReferenceBattleChaserRdfId = 0;
    jtshared::RenderFrame* outRdf = google::protobuf::Arena::Create<RenderFrame>(theAllocator);
    while (loopRdfCnt > outerTimerRdfId) {
        bool ok1 = reusedBattle->GetRdfAndIfdIds(&timerRdfId, &newChaserRdfId, &chaserRdfIdLowerBound, &oldLcacIfdId, &oldUdpLcacIfdId, &toGenIfdId, &localRequiredIfdId);
        // Handling TCP packets first, and then UDP packets, the same as C# side behavior.
        if (incomingDownsyncSnapshots3.count(outerTimerRdfId)) {
            DownsyncSnapshot* srvDownsyncSnapshot = incomingDownsyncSnapshots3[outerTimerRdfId];
            int outPostTimerRdfEvictedCnt = 0, outPostTimerRdfDelayedIfdEvictedCnt = 0;
            bool applied = reusedBattle->OnDownsyncSnapshotReceived(srvDownsyncSnapshot, &outPostTimerRdfEvictedCnt, &outPostTimerRdfDelayedIfdEvictedCnt, &newChaserRdfId, &newLcacIfdId, &newUdpLcacIfdId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
            if (0 == srvDownsyncSnapshot->st_ifd_id()) {
                JPH_ASSERT(25 == newLcacIfdId);
            } else if (780 == srvDownsyncSnapshot->ref_rdf_id()) {
                JPH_ASSERT(190 == newLcacIfdId); // Drags "lcacIfdId" forward regardless of discontinuity
                JPH_ASSERT(0 == outPostTimerRdfEvictedCnt);
                JPH_ASSERT(0 == outPostTimerRdfDelayedIfdEvictedCnt);
                JPH_ASSERT(outerTimerRdfId == reusedBattle->timerRdfId);
                // no eviction occurred
            }
        }
        if (incomingUpsyncSnapshotReqs3.count(outerTimerRdfId)) {
            auto req = incomingUpsyncSnapshotReqs3[outerTimerRdfId];
            auto peerUpsyncSnapshot = req->upsync_snapshot();
            bool applied = reusedBattle->OnUpsyncSnapshotReceived(req->join_index(), peerUpsyncSnapshot, &newChaserRdfId, &newUdpLcacIfdId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
            if (13 == peerUpsyncSnapshot.st_ifd_id()) {
                JPH_ASSERT(54 == newChaserRdfId);
                JPH_ASSERT(-1 == reusedBattle->lcacIfdId);
            } else if (0 == peerUpsyncSnapshot.st_ifd_id()) {
                JPH_ASSERT(2 == newChaserRdfId);
                JPH_ASSERT(-1 == reusedBattle->lcacIfdId);
            } else if (29 == peerUpsyncSnapshot.st_ifd_id()) {
                JPH_ASSERT(118 == newChaserRdfId);
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
    theAllocator->Reset();
    reusedBattle->Clear();   
    return true;
}

bool runTestCase4(FrontendBattle* reusedBattle, std::vector<std::vector<float>>& hulls, int inSingleJoinIndex, google::protobuf::Arena* theAllocator) {
    WsReq* initializerMapData = google::protobuf::Arena::Create<WsReq>(theAllocator);
    initTest4Data(initializerMapData, hulls, theAllocator);
    reusedBattle->SetFrameLogEnabled(true);
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);
    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 1024;
    int printIntervalRdfCnt = (1 << 0);
    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    int newChaserRdfId = 0;
    jtshared::RenderFrame* outRdf = google::protobuf::Arena::Create<RenderFrame>(theAllocator);
    int newLcacIfdId = -1, newUdpLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    while (loopRdfCnt > outerTimerRdfId) {
        if (incomingDownsyncSnapshots4.count(outerTimerRdfId)) {
            DownsyncSnapshot* srvDownsyncSnapshot = incomingDownsyncSnapshots4[outerTimerRdfId];
            int outPostTimerRdfEvictedCnt = 0, outPostTimerRdfDelayedIfdEvictedCnt = 0;
            bool applied = reusedBattle->OnDownsyncSnapshotReceived(srvDownsyncSnapshot, &outPostTimerRdfEvictedCnt, &outPostTimerRdfDelayedIfdEvictedCnt, &newChaserRdfId, &newLcacIfdId, &newUdpLcacIfdId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
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
    theAllocator->Reset();
    reusedBattle->Clear();   
    return true;
}

bool runTestCase5(FrontendBattle* reusedBattle, std::vector<std::vector<float>>& hulls, int inSingleJoinIndex, google::protobuf::Arena* theAllocator) {
    WsReq* initializerMapData = google::protobuf::Arena::Create<WsReq>(theAllocator);
    initTest5Data(initializerMapData, hulls, theAllocator);
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);
    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 1024;
    int printIntervalRdfCnt = (1 << 0);
    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    int lastSentIfdId = -1;
    int timerRdfId = -1, chaserRdfIdLowerBound = -1, oldLcacIfdId = -1, toGenIfdId = -1, localRequiredIfdId = -1;
    int newLcacIfdId = -1, newUdpLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    int newChaserRdfId = 0, newReferenceBattleChaserRdfId = 0;
    jtshared::RenderFrame* outRdf = google::protobuf::Arena::Create<RenderFrame>(theAllocator);
    while (loopRdfCnt > outerTimerRdfId) {
        if (incomingDownsyncSnapshots5.count(outerTimerRdfId)) {
            DownsyncSnapshot* srvDownsyncSnapshot = incomingDownsyncSnapshots5[outerTimerRdfId];
            int outPostTimerRdfEvictedCnt = 0, outPostTimerRdfDelayedIfdEvictedCnt = 0;
            bool applied = reusedBattle->OnDownsyncSnapshotReceived(srvDownsyncSnapshot, &outPostTimerRdfEvictedCnt, &outPostTimerRdfDelayedIfdEvictedCnt, &newChaserRdfId, &newLcacIfdId, &newUdpLcacIfdId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
            if (0 == srvDownsyncSnapshot->st_ifd_id()) {
                JPH_ASSERT(25 == newLcacIfdId);
                JPH_ASSERT(25 == minPlayerInputFrontId && 25 == maxPlayerInputFrontId);
            } else if (26 == srvDownsyncSnapshot->st_ifd_id()) {
                JPH_ASSERT(57 == newLcacIfdId);
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
    theAllocator->Reset();
    reusedBattle->Clear();   
    return true;
}

bool runTestCase6(FrontendBattle* reusedBattle, std::vector<std::vector<float>>& hulls, int inSingleJoinIndex, google::protobuf::Arena* theAllocator) {
    WsReq* initializerMapData = google::protobuf::Arena::Create<WsReq>(theAllocator);
    initTest6Data(initializerMapData, hulls, theAllocator);
    
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);

    FrontendBattle* referenceBattle = static_cast<FrontendBattle*>(FRONTEND_CreateBattle(512, true));
    
    referenceBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);

    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 1024;
    int printIntervalRdfCnt = (1 << 5);

    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    int timerRdfId = -1, toGenIfdId = -1, localRequiredIfdId = -1; // shared 
    int chaserRdfIdLowerBound = -1, oldLcacIfdId = -1, newLcacIfdId = -1, newUdpLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    int newChaserRdfId = 0, newReferenceBattleChaserRdfId = 0;
    int referenceBattleChaserRdfId = -1, referenceBattleChaserRdfIdLowerBound = -1, referenceBattleOldLcacIfdId = -1, referenceBattleNewLcacIfdId = -1, referenceBattleMaxPlayerInputFrontId = 0, referenceBattleMinPlayerInputFrontId = 0;
    jtshared::RenderFrame* outRdf = google::protobuf::Arena::Create<RenderFrame>(theAllocator);
    jtshared::RenderFrame* referenceBattleOutRdf = google::protobuf::Arena::Create<RenderFrame>(theAllocator);
    jtshared::StepResult*  outStepResult = google::protobuf::Arena::Create<StepResult>(theAllocator);
    while (loopRdfCnt > outerTimerRdfId) {
        bool shouldPrint = false;
        if (incomingUpsyncSnapshotReqs6Intime.count(outerTimerRdfId)) {
            auto req = incomingUpsyncSnapshotReqs6Intime[outerTimerRdfId];
            auto peerUpsyncSnapshot = req->upsync_snapshot();
            referenceBattle->OnUpsyncSnapshotReceived(req->join_index(), peerUpsyncSnapshot, &newReferenceBattleChaserRdfId, &newUdpLcacIfdId, &referenceBattleMaxPlayerInputFrontId, &referenceBattleMinPlayerInputFrontId);
        }

        if (incomingUpsyncSnapshotReqs6Rollback.count(outerTimerRdfId)) {
            auto req = incomingUpsyncSnapshotReqs6Rollback[outerTimerRdfId];
            auto peerUpsyncSnapshot = req->upsync_snapshot();
            reusedBattle->OnUpsyncSnapshotReceived(req->join_index(), peerUpsyncSnapshot, &newChaserRdfId, &newUdpLcacIfdId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
            if (155 == outerTimerRdfId) {
                JPH_ASSERT(BaseBattle::ConvertToFirstUsedRenderFrameId(1) == newChaserRdfId);
            } else if (300 == outerTimerRdfId) {
                JPH_ASSERT(178 == newChaserRdfId);
            } else if (320 == outerTimerRdfId) {
                JPH_ASSERT(BaseBattle::ConvertToFirstUsedRenderFrameId(26) == newChaserRdfId);
            } else if (330 == outerTimerRdfId) {
                JPH_ASSERT(BaseBattle::ConvertToFirstUsedRenderFrameId(60) == newChaserRdfId);
            }
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

        // Printing
        if (0 < outerTimerRdfId) {
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

            // Fetch rollback battle step result
            memset(stepResultFetchBuffer, 0, sizeof(stepResultFetchBuffer));
            outBytesCnt = pbBufferSizeLimit;
            APP_GetStepResult(reusedBattle, outerTimerRdfId, stepResultFetchBuffer, &outBytesCnt);
            outStepResult->ParseFromArray(stepResultFetchBuffer, outBytesCnt);
        }
    }

    std::cout << "Passed TestCase6: Basic rollback-chasing\n" << std::endl;
    theAllocator->Reset();
    reusedBattle->Clear();   
    APP_DestroyBattle(referenceBattle);
    return true;
}

bool runTestCase7(FrontendBattle* reusedBattle, std::vector<std::vector<float>>& hulls, int inSingleJoinIndex, google::protobuf::Arena* theAllocator) {
    WsReq* initializerMapData = google::protobuf::Arena::Create<WsReq>(theAllocator);
    initTest7Data(initializerMapData, hulls, theAllocator);
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);

    FrontendBattle* referenceBattle = static_cast<FrontendBattle*>(FRONTEND_CreateBattle(512, true));
    referenceBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);

    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 69;
    int printIntervalRdfCnt = (1 << 4);

    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    int timerRdfId = -1, toGenIfdId = -1, localRequiredIfdId = -1; // shared 
    int chaserRdfIdLowerBound = -1, oldLcacIfdId = -1, newLcacIfdId = -1, newUdpLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    int newChaserRdfId = 0, newReferenceBattleChaserRdfId = 0;
    int referenceBattleChaserRdfId = -1, referenceBattleChaserRdfIdLowerBound = -1, referenceBattleOldLcacIfdId = -1, referenceBattleNewLcacIfdId = -1, referenceBattleMaxPlayerInputFrontId = 0, referenceBattleMinPlayerInputFrontId = 0;
    jtshared::RenderFrame* outRdf = google::protobuf::Arena::Create<RenderFrame>(theAllocator);
    jtshared::RenderFrame* referenceBattleOutRdf = google::protobuf::Arena::Create<RenderFrame>(theAllocator);
    while (loopRdfCnt > outerTimerRdfId) {
        bool shouldPrint = false;
        if (incomingUpsyncSnapshotReqs7Intime.count(outerTimerRdfId)) {
            auto req = incomingUpsyncSnapshotReqs7Intime[outerTimerRdfId];
            auto peerUpsyncSnapshot = req->upsync_snapshot();
            referenceBattle->OnUpsyncSnapshotReceived(req->join_index(), peerUpsyncSnapshot, &newReferenceBattleChaserRdfId, &newUdpLcacIfdId, &referenceBattleMaxPlayerInputFrontId, &referenceBattleMinPlayerInputFrontId);
            //shouldPrint = true;
        }

        if (incomingUpsyncSnapshotReqs7Rollback.count(outerTimerRdfId)) {
            auto req = incomingUpsyncSnapshotReqs7Rollback[outerTimerRdfId];
            auto peerUpsyncSnapshot = req->upsync_snapshot();
            reusedBattle->OnUpsyncSnapshotReceived(req->join_index(), peerUpsyncSnapshot, &newChaserRdfId, &newUdpLcacIfdId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
            //shouldPrint = true;
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

        //shouldPrint |= (outerTimerRdfId >= loopRdfCnt);
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
    theAllocator->Reset();
    reusedBattle->Clear();   
    APP_DestroyBattle(referenceBattle);
    return true;
}

bool runTestCase8(FrontendBattle* reusedBattle, std::vector<std::vector<float>>& hulls, int inSingleJoinIndex, google::protobuf::Arena* theAllocator) {
    WsReq* initializerMapData = google::protobuf::Arena::Create<WsReq>(theAllocator);
    initTest8Data(initializerMapData, hulls, theAllocator);
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);
    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 2048;
    int printIntervalRdfCnt = (1 << 2);
    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    jtshared::RenderFrame* outRdf = google::protobuf::Arena::Create<RenderFrame>(theAllocator);
    int newLcacIfdId = -1, newUdpLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
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
            //shouldPrint = true;
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
        } else if (220 == outerTimerRdfId) {
            // Gracing for turnaround
            JPH_ASSERT(CharacterState::Idle1 == npc2Chd.ch_state());
            JPH_ASSERT(0 != npc2.last_fled_rdf_id());
            JPH_ASSERT(400 < npc2Chd.y() && 500 >= npc2Chd.y());
            JPH_ASSERT(400 < npc2Chd.x() && 500 > npc2Chd.x());
        } else if (320 == outerTimerRdfId) {
            // It has turned around on "npcVisionHull5" and moving to the left due to vision reaction of "npcVisionHull3"
            JPH_ASSERT(CharacterState::Walking == npc2Chd.ch_state());
            JPH_ASSERT(400 < npc2Chd.y() && 500 >= npc2Chd.y());
            JPH_ASSERT(350 < npc2Chd.x() && 500 > npc2Chd.x());
            JPH_ASSERT(0 > npc2Chd.vel_x());
        } else if (580 == outerTimerRdfId) {
            // It's proactively jumping towards left onto "npcVisionHull6" and moving to the left
            JPH_ASSERT(CharacterState::InAirIdle1ByJump == npc2Chd.ch_state());
            JPH_ASSERT(0 > npc2Chd.vel_x());
        } else if (610 == outerTimerRdfId) {
            // It has jumped on "npcVisionHull6" and moving to the left
            JPH_ASSERT(CharacterState::Walking == npc2Chd.ch_state());
            JPH_ASSERT(500 < npc2Chd.y() && 520 >= npc2Chd.y());
            JPH_ASSERT(200 < npc2Chd.x() && 350 > npc2Chd.x());
            JPH_ASSERT(0 > npc2Chd.vel_x());
        } else if (855 == outerTimerRdfId) {
            // Still walking but soon to enter grace period due to blocked by "npcVisionHull7"
            JPH_ASSERT(CharacterState::Walking == npc2Chd.ch_state());
            JPH_ASSERT(500 < npc2Chd.y() && 520 >= npc2Chd.y());
            JPH_ASSERT(100 < npc2Chd.x() && 300 > npc2Chd.x());
            JPH_ASSERT(0 > npc2Chd.vel_x());
        } else if (900 == outerTimerRdfId) {
            // Gracing due to blocked by "npcVisionHull7"
            JPH_ASSERT(CharacterState::Idle1 == npc2Chd.ch_state());
            JPH_ASSERT(500 < npc2Chd.y() && 520 >= npc2Chd.y());
            JPH_ASSERT(100 < npc2Chd.x() && 300 > npc2Chd.x());
            JPH_ASSERT(850 < npc2.last_fled_rdf_id());
        } else if (1024 == outerTimerRdfId) {
            // It's still on "npcVisionHull6" but turned to move to the right due to vision reaction of "npcVisionHull7"
            JPH_ASSERT(CharacterState::Walking == npc2Chd.ch_state());
            JPH_ASSERT(500 < npc2Chd.y() && 520 >= npc2Chd.y());
            JPH_ASSERT(100 < npc2Chd.x() && 300 > npc2Chd.x());
            JPH_ASSERT(0 < npc2Chd.vel_x());
        } else if (1300 == outerTimerRdfId) {
            // When standing on the edge of "npcVisionHull6", it should've seen the lower platform  "npcVisionHull5" and decided to move to the right and fall onto "npcVisionHull5"
            JPH_ASSERT(CharacterState::Walking == npc2Chd.ch_state());
            JPH_ASSERT(400 < npc2Chd.y() && 500 >= npc2Chd.y());
            JPH_ASSERT(300 < npc2Chd.x() && 500 > npc2Chd.x());
            JPH_ASSERT(0 < npc2Chd.vel_x());
        } else if (1550 == outerTimerRdfId) {
            // Gracing
            JPH_ASSERT(CharacterState::Idle1 == npc2Chd.ch_state());
            JPH_ASSERT(400 < npc2Chd.y() && 500 >= npc2Chd.y());
            JPH_ASSERT(300 < npc2Chd.x() && 500 > npc2Chd.x());
            JPH_ASSERT(1450 < npc2.last_fled_rdf_id());
        } else if (1800 == outerTimerRdfId) {
            // Again it has turned around on "npcVisionHull5" and moving to the left due to vision reaction of "npcVisionHull3"
            JPH_ASSERT(CharacterState::Walking == npc2Chd.ch_state());
            JPH_ASSERT(400 < npc2Chd.y() && 500 >= npc2Chd.y());
            JPH_ASSERT(300 < npc2Chd.x() && 500 > npc2Chd.x());
            JPH_ASSERT(0 > npc2Chd.vel_x());
        }
        
        outerTimerRdfId++;
    }

    std::cout << "Passed TestCase8: Basic npc vision\n" << std::endl;
    theAllocator->Reset();
    reusedBattle->Clear();
    return true;
}

bool runTestCase9(FrontendBattle* reusedBattle, std::vector<std::vector<float>>& hulls, int inSingleJoinIndex, google::protobuf::Arena* theAllocator) {
    WsReq* initializerMapData = google::protobuf::Arena::Create<WsReq>(theAllocator);
    initTest9Data(initializerMapData, hulls, theAllocator);
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);
    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 2048;
    int printIntervalRdfCnt = (1 << 4);
    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    jtshared::RenderFrame* outRdf = google::protobuf::Arena::Create<RenderFrame>(theAllocator);
    int newLcacIfdId = -1, newUdpLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    int newChaserRdfId = 0, newReferenceBattleChaserRdfId = 0;
    float initTrapY = 1500;
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

        if (0 <= outerTimerRdfId && outerTimerRdfId <= 250) {
            //shouldPrint = true;
        }

        if (shouldPrint) {
            std::cout << "TestCase9/outerTimerRdfId=" << outerTimerRdfId << "\n\ttp1 pos=(" << tp1.x() << ", " << tp1.y() << "," << tp1.z() << "), dir=(" << tp1.q_x() << ", " << tp1.q_y() << ", " << tp1.q_z() << ", " << tp1.q_w() << "), vel=(" << tp1.vel_x() << ", " << tp1.vel_y() << ", " << tp1.vel_z() << ")" << std::endl;
        }

        if (50 <= outerTimerRdfId && outerTimerRdfId <= 100) {
            JPH_ASSERT(0 < tp1.x() && tp1.x() < 500);
            JPH_ASSERT(initTrapY == tp1.y());
        } else if (231 <= outerTimerRdfId && outerTimerRdfId <= 248) {
            int p1Expectedfc = outerTimerRdfId - 231;
            const Skill* skill = nullptr;
            const BulletConfig* bulletConfig = nullptr;
            uint32_t p1ChdActiveSkillId = p1Chd.active_skill_id();
            int p1ChdActiveSkillHit = p1Chd.active_skill_hit();
            BaseBattle::FindBulletConfig(p1ChdActiveSkillId, p1ChdActiveSkillHit, skill, bulletConfig);
            JPH_ASSERT(nullptr != skill && nullptr != bulletConfig);
            int p1ExpectedFramesToRecover = skill->recovery_frames() - (p1Expectedfc);
            JPH_ASSERT(CharacterState::Atk1 == p1Chd.ch_state());
            JPH_ASSERT(p1Expectedfc == p1Chd.frames_in_ch_state());
            JPH_ASSERT(p1ExpectedFramesToRecover == p1Chd.frames_to_recover());
            JPH_ASSERT(500 <= tp1.x());
            JPH_ASSERT(initTrapY == tp1.y());
        } else if (249 == outerTimerRdfId) {
            JPH_ASSERT(CharacterState::Idle1 == p1Chd.ch_state());
        } else if (303 < outerTimerRdfId && outerTimerRdfId <= 420) {
            JPH_ASSERT(0 < tp1.x() && tp1.x() < 501);
            JPH_ASSERT(initTrapY == tp1.y());
        } else if (outerTimerRdfId <= 1000) {
            JPH_ASSERT(initTrapY == tp1.y());
        }

        outerTimerRdfId++;
    }

    std::cout << "Passed TestCase9: Slider Trap\n" << std::endl;
    theAllocator->Reset();
    reusedBattle->Clear();
    return true;
}

bool runTestCase10(FrontendBattle* reusedBattle, std::vector<std::vector<float>>& hulls, int inSingleJoinIndex, google::protobuf::Arena* theAllocator) {
    WsReq* initializerMapData = google::protobuf::Arena::Create<WsReq>(theAllocator);
    initTest10Data(initializerMapData, hulls, theAllocator);
    reusedBattle->SetFrameLogEnabled(true);
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);
    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 1024;
    int printIntervalRdfCnt = (1 << 4);
    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    jtshared::RenderFrame* outRdf = google::protobuf::Arena::Create<RenderFrame>(theAllocator);
    int newLcacIfdId = -1, newUdpLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    int newChaserRdfId = 0, newReferenceBattleChaserRdfId = 0;
    auto& frameLogBuffer = reusedBattle->frameLogBuffer;

    while (loopRdfCnt > outerTimerRdfId) {
        // Handling TCP packets first, and then UDP packets, the same as C# side behavior.
        if (incomingDownsyncSnapshots10.count(outerTimerRdfId)) {
            DownsyncSnapshot* srvDownsyncSnapshot = incomingDownsyncSnapshots10[outerTimerRdfId];
            int outPostTimerRdfEvictedCnt = 0, outPostTimerRdfDelayedIfdEvictedCnt = 0;
            bool applied = reusedBattle->OnDownsyncSnapshotReceived(srvDownsyncSnapshot, &outPostTimerRdfEvictedCnt, &outPostTimerRdfDelayedIfdEvictedCnt, &newChaserRdfId, &newLcacIfdId, &newUdpLcacIfdId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
            if (308 == outerTimerRdfId) {
                JPH_ASSERT(306 == reusedBattle->chaserRdfId);
                int toTestIfdId = BaseBattle::ConvertToDelayedInputFrameId(outerTimerRdfId);
                JPH_ASSERT(76 == toTestIfdId);
                InputFrameDownsync* toTestIfd = reusedBattle->ifdBuffer.GetByFrameId(toTestIfdId);
                JPH_ASSERT(nullptr != toTestIfd);
                JPH_ASSERT(3 == toTestIfd->confirmed_list());

                FrameLog* frameLogToTest1 = frameLogBuffer.GetByFrameId(11);
                JPH_ASSERT(3 == frameLogToTest1->used_ifd_confirmed_list());
                FrameLog* frameLogToTest2 = frameLogBuffer.GetByFrameId(15);
                JPH_ASSERT(0 == frameLogToTest2->used_ifd_confirmed_list());
            }
        }
        if (incomingUpsyncSnapshotReqs10.count(outerTimerRdfId)) {
            auto req = incomingUpsyncSnapshotReqs10[outerTimerRdfId]; 
            auto peerUpsyncSnapshot = req->upsync_snapshot();
            auto peerJoinIndex = req->join_index();
            bool applied = reusedBattle->OnUpsyncSnapshotReceived(peerJoinIndex, peerUpsyncSnapshot, &newChaserRdfId, &newUdpLcacIfdId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
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

    std::cout << "Passed TestCase10: Regular DownsyncSnapshot and UpsyncSnapshot mixed handling\n" << std::endl;
    theAllocator->Reset();
    reusedBattle->Clear();   
    return true;
}

bool runTestCase11(FrontendBattle* reusedBattle, std::vector<std::vector<float>>& hulls, int inSingleJoinIndex, google::protobuf::Arena* theAllocator) {
    bool doCompareWithRollback = true; 

    WsReq* initializerMapData = google::protobuf::Arena::Create<WsReq>(theAllocator);
    initTest11Data(initializerMapData, hulls, theAllocator);
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);

    FrontendBattle* referenceBattle = static_cast<FrontendBattle*>(FRONTEND_CreateBattle(512, true));
    referenceBattle->SetFrameLogEnabled(true);
    referenceBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);

    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 1024;
    int printIntervalRdfCnt = (1 << 5);

    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    int timerRdfId = -1, toGenIfdId = -1, localRequiredIfdId = -1; // shared 
    int chaserRdfIdLowerBound = -1, oldLcacIfdId = -1, newLcacIfdId = -1, newUdpLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    int newChaserRdfId = 0, newReferenceBattleChaserRdfId = 0, newReferenceBattleUdpLcacIfdId = -1;
    int referenceBattleChaserRdfId = -1, referenceBattleChaserRdfIdLowerBound = -1, referenceBattleOldLcacIfdId = -1, referenceBattleNewLcacIfdId = -1, referenceBattleMaxPlayerInputFrontId = 0, referenceBattleMinPlayerInputFrontId = 0;
    jtshared::RenderFrame* outRdf = google::protobuf::Arena::Create<RenderFrame>(theAllocator);
    jtshared::RenderFrame* referenceBattleOutRdf = google::protobuf::Arena::Create<RenderFrame>(theAllocator);
    while (loopRdfCnt > outerTimerRdfId) {
        if (incomingUpsyncSnapshotReqs11Intime.count(outerTimerRdfId)) {
            auto req = incomingUpsyncSnapshotReqs11Intime[outerTimerRdfId];
            auto peerUpsyncSnapshot = req->upsync_snapshot();
            referenceBattle->OnUpsyncSnapshotReceived(req->join_index(), peerUpsyncSnapshot, &newReferenceBattleChaserRdfId, &newReferenceBattleUdpLcacIfdId, &referenceBattleMaxPlayerInputFrontId, &referenceBattleMinPlayerInputFrontId);
        }

        if (doCompareWithRollback) {
            if (incomingUpsyncSnapshotReqs11Rollback.count(outerTimerRdfId)) {
                auto req = incomingUpsyncSnapshotReqs11Rollback[outerTimerRdfId];
                auto peerUpsyncSnapshot = req->upsync_snapshot();
                reusedBattle->OnUpsyncSnapshotReceived(req->join_index(), peerUpsyncSnapshot, &newChaserRdfId, &newUdpLcacIfdId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
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
            StepResult*  challengingStepResult = reusedBattle->stepResultBuffer.GetByFrameId(outerTimerRdfId); 
            JPH_ASSERT(nullptr != challengingStepResult);

            const PlayerCharacterDownsync& referencedP1 = referencedRdf->players(0);
            const CharacterDownsync& p1Chd = referencedP1.chd();
            const PlayerCharacterDownsync& referencedP2 = referencedRdf->players(1);
            const CharacterDownsync& p2Chd = referencedP2.chd();
            const NpcCharacterDownsync& referencedNpc1 = referencedRdf->npcs(0);
            const CharacterDownsync& npc1Chd = referencedNpc1.chd();

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
                std::cout << "TestCase11/outerTimerRdfId=" << outerTimerRdfId << "\n\tp1Chd cs=" << p1Chd.ch_state() << ", fc=" << p1Chd.frames_in_ch_state() << ", dir=(" << p1Chd.q_x() << ", " << p1Chd.q_y() << ", " << p1Chd.q_z() << ", " << p1Chd.q_w() << "), pos=(" << p1Chd.x() << ", " << p1Chd.y() << "), vel=(" << p1Chd.vel_x() << ", " << p1Chd.vel_y() << ")\n\tp2Chd cs=" << p2Chd.ch_state() << ", fc=" << p2Chd.frames_in_ch_state() << ", dir=(" << p2Chd.q_x() << ", " << p2Chd.q_y() << ", " << p2Chd.q_z() << ", " << p2Chd.q_w() << "), pos=(" << p2Chd.x() << ", " << p2Chd.y() << "), vel=(" << p2Chd.vel_x() << ", " << p2Chd.vel_y() << ")\n\tnpc1Chd cs=" << npc1Chd.ch_state() << ", fc=" << npc1Chd.frames_in_ch_state() << ", dir=(" << npc1Chd.q_x() << ", " << npc1Chd.q_y() << ", " << npc1Chd.q_z() << ", " << npc1Chd.q_w() << "), pos=(" << npc1Chd.x() << ", " << npc1Chd.y() << "), vel=(" << npc1Chd.vel_x() << ", " << npc1Chd.vel_y() << ")" << std::endl;
            }
        }
        outerTimerRdfId++;
       
    }

    std::cout << "Passed TestCase11: Rollback-chasing with character soft-pushback\n" << std::endl;
    theAllocator->Reset();
    reusedBattle->Clear();   
    APP_DestroyBattle(referenceBattle);
    return true;
}

bool runTestCase12(FrontendBattle* reusedBattle, std::vector<std::vector<float>>& hulls, int inSingleJoinIndex, google::protobuf::Arena* theAllocator) {
    WsReq* initializerMapData = google::protobuf::Arena::Create<WsReq>(theAllocator);
    initTest12Data(initializerMapData, hulls, theAllocator);
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);

    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 512;
    int printIntervalRdfCnt = (1 << 5);

    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    int timerRdfId = -1, toGenIfdId = -1, localRequiredIfdId = -1; // shared 
    int chaserRdfIdLowerBound = -1, oldLcacIfdId = -1, newLcacIfdId = -1, newUdpLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    int newChaserRdfId = 0;
    while (loopRdfCnt > outerTimerRdfId) {
        bool shouldPrint = false;
        if (incomingUpsyncSnapshotReqs12Intime.count(outerTimerRdfId)) {
            auto req = incomingUpsyncSnapshotReqs12Intime[outerTimerRdfId];
            auto peerUpsyncSnapshot = req->upsync_snapshot();
            reusedBattle->OnUpsyncSnapshotReceived(req->join_index(), peerUpsyncSnapshot, &newChaserRdfId, &newUdpLcacIfdId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
            //shouldPrint = true;
        }

        uint64_t inSingleInput = getSelfCmdByRdfId(testCmds12, outerTimerRdfId);
        bool cmdInjected = FRONTEND_UpsertSelfCmd(reusedBattle, inSingleInput, &newChaserRdfId);
        if (!cmdInjected) {
            std::cerr << "TestCase12/Failed to inject cmd for outerTimerRdfId=" << outerTimerRdfId << ", inSingleInput=" << inSingleInput << std::endl;
            exit(1);
        }

        FRONTEND_Step(reusedBattle);

        RenderFrame* outerTimerRdf = reusedBattle->rdfBuffer.GetByFrameId(outerTimerRdfId);
        auto& p1 = outerTimerRdf->players(0);
        auto& p1Chd = p1.chd();
        auto& p2 = outerTimerRdf->players(1);
        auto& p2Chd = p2.chd();
        
        if (0 <= outerTimerRdfId && outerTimerRdfId <= 300) {    
            //shouldPrint = true;
        }

        if (shouldPrint) {
            std::cout << "TestCase12/outerTimerRdfId=" << outerTimerRdfId << "\n\tp1Chd hp=" << p1Chd.hp() << ", cs=" << p1Chd.ch_state() << ", fc=" << p1Chd.frames_in_ch_state() << ", q=(" << p1Chd.q_x() << ", " << p1Chd.q_y() << ", " << p1Chd.q_z() << ", " << p1Chd.q_w() << "), pos=(" << p1Chd.x() << ", " << p1Chd.y() << ", " << p1Chd.z() << "), vel=(" << p1Chd.vel_x() << ", " << p1Chd.vel_y() << ", " << p1Chd.vel_z() << ")\n\tp2Chd hp=" << p2Chd.hp() << ", cs=" << p2Chd.ch_state() << ", fc=" << p2Chd.frames_in_ch_state() << ", q=(" << p2Chd.q_x() << ", " << p2Chd.q_y() << ", " << p2Chd.q_z() << ", " << p2Chd.q_w() << "), pos=(" << p2Chd.x() << ", " << p2Chd.y() << ", " << p2Chd.z() << "), vel=(" << p2Chd.vel_x() << ", " << p2Chd.vel_y() << ", " << p2Chd.vel_z() << ")" << std::endl;
        }

        if (140 == outerTimerRdfId) {
            JPH_ASSERT(0 >= p2Chd.hp());
            JPH_ASSERT(CharacterState::Dying == p2Chd.ch_state());
        } else if (295 == outerTimerRdfId) {
            JPH_ASSERT(0 < p2Chd.hp());
            JPH_ASSERT(CharacterState::Idle1 == p2Chd.ch_state());
        }

        outerTimerRdfId++;
    }

    std::cout << "Passed TestCase12: Rapid firing and fallen death\n" << std::endl;
    theAllocator->Reset();
    reusedBattle->Clear();   
    return true;
}

bool runTestCase13(FrontendBattle* reusedBattle, std::vector<std::vector<float>>& hulls, int inSingleJoinIndex, google::protobuf::Arena* theAllocator) {
    WsReq* initializerMapData = google::protobuf::Arena::Create<WsReq>(theAllocator);
    initTest13Data(initializerMapData, hulls, theAllocator);
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
            //shouldPrint = true;
        }

        if (shouldPrint) {
            std::cout << "TestCase13/outerTimerRdfId=" << outerTimerRdfId << "\n\tp1Chd hp=" << p1Chd.hp() << ", cs=" << p1Chd.ch_state() << ", fc=" << p1Chd.frames_in_ch_state() << ", q=(" << p1Chd.q_x() << ", " << p1Chd.q_y() << ", " << p1Chd.q_z() << ", " << p1Chd.q_w() << "), pos=(" << p1Chd.x() << ", " << p1Chd.y() << ", " << p1Chd.z() << "), vel=(" << p1Chd.vel_x() << ", " << p1Chd.vel_y() << ", " << p1Chd.vel_z() << ")\n\tp2Chd hp=" << p2Chd.hp() << ", cs=" << p2Chd.ch_state() << ", fc=" << p2Chd.frames_in_ch_state() << ", q=(" << p2Chd.q_x() << ", " << p2Chd.q_y() << ", " << p2Chd.q_z() << ", " << p2Chd.q_w() << "), pos=(" << p2Chd.x() << ", " << p2Chd.y() << ", " << p2Chd.z() << "), vel=(" << p2Chd.vel_x() << ", " << p2Chd.vel_y() << ", " << p2Chd.vel_z() << ")" << std::endl;
        }

        if (63 == outerTimerRdfId) {
            JPH_ASSERT(CharacterState::Atk1 == p1Chd.ch_state());
            JPH_ASSERT(0 == p1Chd.frames_in_ch_state());
        } else if (70 == outerTimerRdfId) {
            JPH_ASSERT(CharacterState::Atked1 == npc1Chd.ch_state());
            //JPH_ASSERT(0 < npc1Chd.bir_count());
        } else if (103 <= outerTimerRdfId && outerTimerRdfId <= 124) {
            JPH_ASSERT(CharacterState::Sliding == p1Chd.ch_state());
            int expectedFc = outerTimerRdfId-103;
            JPH_ASSERT(p1Chd.frames_in_ch_state() == expectedFc);
        } else if (125 == outerTimerRdfId) {
            JPH_ASSERT(CharacterState::Idle1 == p1Chd.ch_state());
        } else if (247 <= outerTimerRdfId && outerTimerRdfId <= 268) {
            JPH_ASSERT(CharacterState::InAirDashing == p1Chd.ch_state());
            int expectedFc = outerTimerRdfId-247;
            JPH_ASSERT(p1Chd.frames_in_ch_state() == expectedFc);
        } else if (1023 == outerTimerRdfId) {
            JPH_ASSERT(CharacterState::OnWallIdle1 == p1Chd.ch_state());
        }

        outerTimerRdfId++;
    }

    std::cout << "Passed TestCase13: BladeGirl skill\n" << std::endl;
    theAllocator->Reset();
    reusedBattle->Clear();   

    return true;
}

bool runTestCase14(FrontendBattle* reusedBattle, std::vector<std::vector<float>>& hulls, int inSingleJoinIndex, google::protobuf::Arena* theAllocator) {
    WsReq* initializerMapData = google::protobuf::Arena::Create<WsReq>(theAllocator);
    initTest14Data(initializerMapData, hulls, theAllocator);
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);

    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 1096;
    int printIntervalRdfCnt = (1 << 5);

    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    int timerRdfId = -1, toGenIfdId = -1, localRequiredIfdId = -1; // shared 
    int chaserRdfIdLowerBound = -1, oldLcacIfdId = -1, newLcacIfdId = -1, newUdpLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    int newChaserRdfId = 0;
    while (loopRdfCnt > outerTimerRdfId) {
        bool shouldPrint = false;
        if (incomingUpsyncSnapshotReqs14Intime.count(outerTimerRdfId)) {
            auto req = incomingUpsyncSnapshotReqs14Intime[outerTimerRdfId];
            auto peerUpsyncSnapshot = req->upsync_snapshot();
            reusedBattle->OnUpsyncSnapshotReceived(req->join_index(), peerUpsyncSnapshot, &newChaserRdfId, &newUdpLcacIfdId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
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
        StepResult*  outStepResult = reusedBattle->stepResultBuffer.GetByFrameId(outerTimerRdfId); 
        JPH_ASSERT(nullptr != outStepResult);
        auto& p1 = outerTimerRdf->players(0);
        auto& p1Chd = p1.chd();
        auto& p2 = outerTimerRdf->players(1);
        auto& p2Chd = p2.chd();
        
        auto& npc1 = outerTimerRdf->npcs(0);
        auto& npc1Chd = npc1.chd();

        int bulletCount = outerTimerRdf->bullet_count();

        if (550 <= outerTimerRdfId && outerTimerRdfId < 650) {
            //shouldPrint = true;
        }

        if (900 <= outerTimerRdfId && outerTimerRdfId < 1024) {
            //shouldPrint = true;
        }
        
        if (shouldPrint) {
            std::cout << "TestCase14/outerTimerRdfId=" << outerTimerRdfId << "\n\tp1Chd hp=" << p1Chd.hp() << ", cs=" << p1Chd.ch_state() << ", fc=" << p1Chd.frames_in_ch_state() << ", q=(" << p1Chd.q_x() << ", " << p1Chd.q_y() << ", " << p1Chd.q_z() << ", " << p1Chd.q_w() << "), pos=(" << p1Chd.x() << ", " << p1Chd.y() << "), vel=(" << p1Chd.vel_x() << ", " << p1Chd.vel_y() << "), btn_b_holding_rdf_cnt=" << p1Chd.btn_b_holding_rdf_cnt() << "\n\tp2Chd hp=" << p2Chd.hp() << ", cs=" << p2Chd.ch_state() << ", fc=" << p2Chd.frames_in_ch_state() << ", q=(" << p2Chd.q_x() << ", " << p2Chd.q_y() << ", " << p2Chd.q_z() << ", " << p2Chd.q_w() << "), pos=(" << p2Chd.x() << ", " << p2Chd.y() << "), vel=(" << p2Chd.vel_x() << ", " << p2Chd.vel_y() << ")\n\tnpc1Chd hp=" << npc1Chd.hp() << ", cs=" << npc1Chd.ch_state() << ", fc=" << npc1Chd.frames_in_ch_state() << ", q=(" << npc1Chd.q_x() << ", " << npc1Chd.q_y() << ", " << npc1Chd.q_z() << ", " << npc1Chd.q_w() << "), pos=(" << npc1Chd.x() << ", " << npc1Chd.y() << "), vel=(" << npc1Chd.vel_x() << ", " << npc1Chd.vel_y() << ")" << std::endl;
        }

        if (70 <= outerTimerRdfId && outerTimerRdfId <= 90) {
            if (71 == outerTimerRdfId) {
                JPH_ASSERT(CharacterState::Atk1 == p1Chd.ch_state());
                JPH_ASSERT(0 == p1Chd.frames_in_ch_state());
            } else if (90 == outerTimerRdfId) {
                JPH_ASSERT(140 == p2Chd.hp());
            }
        } else if (110 < outerTimerRdfId && outerTimerRdfId <= 190) {
            JPH_ASSERT(150 == npc1Chd.hp());
            if (183 == outerTimerRdfId) {
                JPH_ASSERT(CharacterState::Atk1 == p1Chd.ch_state());
                JPH_ASSERT(0 == p1Chd.frames_in_ch_state());
            }
        } else if (800 == outerTimerRdfId) {
            JPH_ASSERT(globalPrimitiveConsts->btn_b_holding_rdf_cnt_threshold_2() <= p1Chd.btn_b_holding_rdf_cnt());
            JPH_ASSERT(0 < outStepResult->aiming_ray_count());
        } else if (803 == outerTimerRdfId) {
            JPH_ASSERT(CharacterState::Sliding == p1Chd.ch_state());
        } else if (947 == outerTimerRdfId) {
            JPH_ASSERT(137 == p1Chd.active_skill_id());
            JPH_ASSERT(1 == p1Chd.active_skill_hit());
            JPH_ASSERT(1 == bulletCount);
            auto& bl1 = outerTimerRdf->bullets(0);
            JPH_ASSERT(BulletState::StartUp == bl1.bl_state());
        } else if (960 == outerTimerRdfId) {
            auto& bl1 = outerTimerRdf->bullets(0);
            JPH_ASSERT(BulletState::Vanishing == bl1.bl_state());
            JPH_ASSERT(CharacterState::BlownUp1 == p2Chd.ch_state());
            JPH_ASSERT(0 < p2Chd.vel_y());
        } else if (990 == outerTimerRdfId) {
            JPH_ASSERT(CharacterState::LayDown1 == p2Chd.ch_state());
        } else if (1000 == outerTimerRdfId) {
            JPH_ASSERT(CharacterState::GetUp1 == p2Chd.ch_state());
        } else if (1024 == outerTimerRdfId) {
            JPH_ASSERT(CharacterState::Idle1 == p2Chd.ch_state());
        }

        outerTimerRdfId++;
    }

    std::cout << "Passed TestCase14: BountyHunter skill\n" << std::endl;
    theAllocator->Reset();
    reusedBattle->Clear();   

    return true;
}

bool runTestCase15(FrontendBattle* reusedBattle, std::vector<std::vector<float>>& hulls, int inSingleJoinIndex, google::protobuf::Arena* theAllocator) {
    WsReq* initializerMapData = google::protobuf::Arena::Create<WsReq>(theAllocator);
    initTest15Data(initializerMapData, hulls, theAllocator);
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
        StepResult* stepResult = reusedBattle->stepResultBuffer.GetByFrameId(outerTimerRdfId);

        auto& tr1 = outerTimerRdf->triggers(0);

        auto& p1 = outerTimerRdf->players(0);
        auto& p1Chd = p1.chd();
        
        auto& npc1 = outerTimerRdf->npcs(0);
        auto& npc1Chd = npc1.chd();

        auto& npc2 = outerTimerRdf->npcs(1);
        auto& npc2Chd = npc2.chd();

        auto& bl1 = outerTimerRdf->bullets(0);

        if (1 == outerTimerRdfId) {
            JPH_ASSERT(3 == tr1.sub_cycle_mask_to_fulfill());
            JPH_ASSERT(1 == npc1.publishing_mask_upon_exhausted());
            JPH_ASSERT(2 == npc2.publishing_mask_upon_exhausted());
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
            JPH_ASSERT(0 == stepResult->fulfilled_triggers_size());
        }

        if (300 == outerTimerRdfId) {
            JPH_ASSERT(1 == stepResult->fulfilled_triggers_size());
            auto& fulfilledTr1 = stepResult->fulfilled_triggers(0);
            JPH_ASSERT(globalPrimitiveConsts->trts().victory() == fulfilledTr1.trt());
        }

        outerTimerRdfId++;
    }

    std::cout << "Passed TestCase15: Victory trigger by killing NPC\n" << std::endl;
    theAllocator->Reset();
    reusedBattle->Clear();   

    return true;
}

bool runTestCase16(FrontendBattle* reusedBattle, std::vector<std::vector<float>>& hulls, int inSingleJoinIndex, google::protobuf::Arena* theAllocator) {
    WsReq* initializerMapData = google::protobuf::Arena::Create<WsReq>(theAllocator);
    initTest16Data(initializerMapData, hulls, theAllocator);
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);
    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 2048;
    int printIntervalRdfCnt = (1 << 4);
    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    jtshared::RenderFrame* outRdf = google::protobuf::Arena::Create<RenderFrame>(theAllocator);
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
    theAllocator->Reset();
    reusedBattle->Clear();
    return true;
}

bool runTestCase17(FrontendBattle* reusedBattle, std::vector<std::vector<float>>& hulls, int inSingleJoinIndex, google::protobuf::Arena* theAllocator) {
    WsReq* initializerMapData = google::protobuf::Arena::Create<WsReq>(theAllocator);
    initTest17Data(initializerMapData, hulls, theAllocator);
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);
    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 1024;
    int printIntervalRdfCnt = (1 << 4);
    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    jtshared::RenderFrame* outRdf = google::protobuf::Arena::Create<RenderFrame>(theAllocator);
    int newLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    int newChaserRdfId = 0, newReferenceBattleChaserRdfId = 0;

    float minTp1Y = 105.0f - .5f, maxTp1Y = 105.0f + .5f;
    float minTp2Y = 128.0f - .5f, maxTp2Y = 128.0f + .5f;

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
        auto& tp2 = outerTimerRdf->dynamic_traps(1);

        if (0 < outerTimerRdfId && outerTimerRdfId < 64) {
            //shouldPrint = true;
        }

        if (540 < outerTimerRdfId && outerTimerRdfId < 700) {
            //shouldPrint = true;
        }

        if (shouldPrint) {
            std::cout << "TestCase17/outerTimerRdfId=" << outerTimerRdfId << "\n\tp1 pos=(" << p1Chd.x() << ", " << p1Chd.y() << ", " << p1Chd.z() << "), vel=(" << p1Chd.vel_x() << ", "  << p1Chd.vel_y() << "), cs=" << (int)p1Chd.ch_state() << ", fc=" << p1Chd.frames_in_ch_state() << ", groundUd=" << p1Chd.ground_ud() << ", groundVel=(" << p1Chd.ground_vel_x() << ", " << p1Chd.ground_vel_y() << ")\n\ttrap1, pos=(" << tp1.x() << ", " << tp1.y() << "), q=(" << tp1.q_x() << ", " << tp1.q_y() << ", " << tp1.q_z() << ", " << tp1.q_w() << "), vel=(" << tp1.vel_x() << ", " << tp1.vel_y() << "), trapState=" << (int)tp1.trap_state() << ", framesInTrapState=" << tp1.frames_in_trap_state() << ", ud=" << APP_CalcTrapUserData(tp1.id()) << "\n\ttrap2, pos=(" << tp2.x() << ", " << tp2.y() << "), q=(" << tp2.q_x() << ", " << tp2.q_y() << ", " << tp2.q_z() << ", " << tp2.q_w() << "), vel=(" << tp2.vel_x() << ", " << tp2.vel_y() << "), trapState=" << (int)tp2.trap_state() << ", framesInTrapState=" << tp2.frames_in_trap_state() << ", ud=" << APP_CalcTrapUserData(tp2.id()) << std::endl;
        }

        if (10 < outerTimerRdfId && outerTimerRdfId < 24) {
            JPH_ASSERT(minTp1Y < tp1.y() && tp1.y() < maxTp1Y);
            JPH_ASSERT(0 > tp1.x() && -300 < tp1.x());
        } else if (32 == outerTimerRdfId) {
            JPH_ASSERT(p1Chd.ground_ud() == APP_CalcTrapUserData(tp1.id()));
            JPH_ASSERT(!BaseBattle::IsLengthNearZero(p1Chd.ground_vel_x()));
            JPH_ASSERT(0 < p1Chd.vel_x() * p1Chd.ground_vel_x());
        } else if (150 <= outerTimerRdfId && outerTimerRdfId <= 248) {
            JPH_ASSERT(minTp1Y < tp1.y() && tp1.y() < maxTp1Y);
        } else if (303 < outerTimerRdfId && outerTimerRdfId <= 420) {
            JPH_ASSERT(minTp1Y < tp1.y() && tp1.y() < maxTp1Y);
        } else if (550 == outerTimerRdfId) {
            JPH_ASSERT(CharacterState::Idle1 == p1Chd.ch_state());
            JPH_ASSERT(minTp2Y < tp2.y() && tp2.y() < maxTp2Y);
        } else {
            JPH_ASSERT(minTp1Y < tp1.y() && tp1.y() < maxTp1Y);
        }
        
        outerTimerRdfId++;
    }

    std::cout << "Passed TestCase17: Slider Trap and character interaction\n" << std::endl;
    theAllocator->Reset();
    reusedBattle->Clear();
    return true;
}

bool runTestCase18(FrontendBattle* reusedBattle, std::vector<std::vector<float>>& hulls, int inSingleJoinIndex, google::protobuf::Arena* theAllocator) {
    bool doCompareWithRollback = true; 
    WsReq* initializerMapData = google::protobuf::Arena::Create<WsReq>(theAllocator);
    initTest18Data(initializerMapData, hulls, theAllocator);
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);

    FrontendBattle* referenceBattle = static_cast<FrontendBattle*>(FRONTEND_CreateBattle(512, true));
    referenceBattle->SetFrameLogEnabled(true);
    referenceBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);

    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 1024;
    int printIntervalRdfCnt = (1 << 5);

    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    int timerRdfId = -1, toGenIfdId = -1, localRequiredIfdId = -1; // shared 
    int chaserRdfIdLowerBound = -1, oldLcacIfdId = -1, newLcacIfdId = -1, newUdpLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    int newChaserRdfId = 0, newReferenceBattleChaserRdfId = 0;
    int referenceBattleChaserRdfId = -1, referenceBattleChaserRdfIdLowerBound = -1, referenceBattleOldLcacIfdId = -1, referenceBattleNewLcacIfdId = -1, referenceBattleMaxPlayerInputFrontId = 0, referenceBattleMinPlayerInputFrontId = 0;
    jtshared::RenderFrame* outRdf = google::protobuf::Arena::Create<RenderFrame>(theAllocator);
    jtshared::RenderFrame* referenceBattleOutRdf = google::protobuf::Arena::Create<RenderFrame>(theAllocator);
    while (loopRdfCnt > outerTimerRdfId) {
        if (incomingUpsyncSnapshotReqs18Intime.count(outerTimerRdfId)) {
            auto req = incomingUpsyncSnapshotReqs18Intime[outerTimerRdfId];
            auto peerUpsyncSnapshot = req->upsync_snapshot();
            referenceBattle->OnUpsyncSnapshotReceived(req->join_index(), peerUpsyncSnapshot, &newReferenceBattleChaserRdfId, &newUdpLcacIfdId, &referenceBattleMaxPlayerInputFrontId, &referenceBattleMinPlayerInputFrontId);
        }

        if (doCompareWithRollback) {
            if (incomingUpsyncSnapshotReqs18Rollback.count(outerTimerRdfId)) {
                auto req = incomingUpsyncSnapshotReqs18Rollback[outerTimerRdfId];
                auto peerUpsyncSnapshot = req->upsync_snapshot();
                reusedBattle->OnUpsyncSnapshotReceived(req->join_index(), peerUpsyncSnapshot, &newChaserRdfId, &newUdpLcacIfdId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
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
                //shouldPrint = true;
            } else if (400 == outerTimerRdfId) {
                int firstToBeConsistentRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(4);
                int lastToBeConsistentRdfId = BaseBattle::ConvertToLastUsedRenderFrameId(32) + 1;
                for (int recRdfId = firstToBeConsistentRdfId; recRdfId <= lastToBeConsistentRdfId; recRdfId++) {
                    auto referencedRdf = referenceBattle->rdfBuffer.GetByFrameId(recRdfId);
                    auto challengingRdf = reusedBattle->rdfBuffer.GetByFrameId(recRdfId);
                    BaseBattle::AssertNearlySame(referencedRdf, challengingRdf);
                }
                //shouldPrint = true;
            } else if (480 == outerTimerRdfId) {
                int firstToBeConsistentRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(17);
                int lastToBeConsistentRdfId = BaseBattle::ConvertToLastUsedRenderFrameId(60) + 1;
                for (int recRdfId = firstToBeConsistentRdfId; recRdfId <= lastToBeConsistentRdfId; recRdfId++) {
                    auto referencedRdf = referenceBattle->rdfBuffer.GetByFrameId(recRdfId);
                    auto challengingRdf = reusedBattle->rdfBuffer.GetByFrameId(recRdfId);
                    BaseBattle::AssertNearlySame(referencedRdf, challengingRdf);
                }
                //shouldPrint = true;
            }
        
            if (shouldPrint) {
                std::cout << "TestCase18/outerTimerRdfId=" << outerTimerRdfId << "\n\tp1Chd hp=" << p1Chd.hp() << ", cs=" << p1Chd.ch_state() << ", fc=" << p1Chd.frames_in_ch_state() << ", dir=(" << p1Chd.q_x() << ", " << p1Chd.q_y() << ", " << p1Chd.q_z() << ", " << p1Chd.q_w() << "), pos=(" << p1Chd.x() << ", " << p1Chd.y() << "), vel=(" << p1Chd.vel_x() << ", " << p1Chd.vel_y() << ")\n\tp2Chd hp=" << p2Chd.hp() << ", cs=" << p2Chd.ch_state() << ", fc=" << p2Chd.frames_in_ch_state() << ", dir=(" << p2Chd.q_x() << ", " << p2Chd.q_y() << ", " << p2Chd.q_z() << ", " << p2Chd.q_w() << "), pos=(" << p2Chd.x() << ", " << p2Chd.y() << "), vel=(" << p2Chd.vel_x() << ", " << p2Chd.vel_y() << ")\n\tnpc1Chd hp=" << npc1Chd.hp() << ", cs=" << npc1Chd.ch_state() << ", fc=" << npc1Chd.frames_in_ch_state() << ", dir=(" << npc1Chd.q_x() << ", " << npc1Chd.q_y() << ", " << npc1Chd.q_z() << ", " << npc1Chd.q_w() << "), pos=(" << npc1Chd.x() << ", " << npc1Chd.y() << "), vel=(" << npc1Chd.vel_x() << ", " << npc1Chd.vel_y() << ")\n\ttrap1 pos=(" << referencedTrap1.x() << ", " << referencedTrap1.vel_y() << "), vel=(" << referencedTrap1.vel_x() << ", " << referencedTrap1.y() << ")\n\ttrap2 pos=(" << referencedTrap2.x() << ", " << referencedTrap2.y() << "), vel=(" << referencedTrap2.vel_x() << ", " << referencedTrap2.vel_y() << ")" << std::endl;
            }
        }
        outerTimerRdfId++;
    }

    std::cout << "Passed TestCase18: Rollback-chasing with dynamic traps\n" << std::endl;
    theAllocator->Reset();
    reusedBattle->Clear();   
    APP_DestroyBattle(referenceBattle);
    return true;
}

bool runTestCase19(FrontendBattle* reusedBattle, std::vector<std::vector<float>>& hulls, int inSingleJoinIndex, google::protobuf::Arena* theAllocator) {
    WsReq* initializerMapData = google::protobuf::Arena::Create<WsReq>(theAllocator);
    initTest19Data(initializerMapData, hulls, theAllocator);
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);
    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 1024;
    int printIntervalRdfCnt = (1 << 2);
    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    jtshared::RenderFrame* outRdf = google::protobuf::Arena::Create<RenderFrame>(theAllocator);
    int newLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    int newChaserRdfId = 0, newReferenceBattleChaserRdfId = 0;
    while (loopRdfCnt > outerTimerRdfId) {
        // Handling TCP packets first, and then UDP packets, the same as C# side behavior.

        uint64_t inSingleInput = getSelfCmdByRdfId(testCmds19, outerTimerRdfId);
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

        bool shouldPrint = false;
        
        if (400 < outerTimerRdfId && loopRdfCnt >= outerTimerRdfId) {
            //shouldPrint = true;
        }
        
        if (shouldPrint) {
            std::cout << "TestCase19/outerTimerRdfId=" << outerTimerRdfId << "\n\tp1Chd ud=" << p1Ud << ", hp=" << p1Chd.hp() << ", cs=" << p1Chd.ch_state() << ", fc=" << p1Chd.frames_in_ch_state() << ", q=(" << p1Chd.q_x() << ", " << p1Chd.q_y() << ", " << p1Chd.q_z() << ", " << p1Chd.q_w() << "), pos=(" << p1Chd.x() << ", " << p1Chd.y() << ", " << p1Chd.z() << "), vel=(" << p1Chd.vel_x() << ", " << p1Chd.vel_y() << "), groundUd=" << p1Chd.ground_ud() << std::endl;
        }
        
        if (140 == outerTimerRdfId) {
            JPH_ASSERT(166 > p1Chd.y() && 165 < p1Chd.y());
            JPH_ASSERT(2 == p1Chd.ground_ud());
        } else if (200 == outerTimerRdfId) {
            JPH_ASSERT(100 > p1Chd.y() && 99 < p1Chd.y());
            JPH_ASSERT(1 == p1Chd.ground_ud());
        } else if (440 == outerTimerRdfId) {
            JPH_ASSERT(166 > p1Chd.y() && 165 < p1Chd.y());
            JPH_ASSERT(2 == p1Chd.ground_ud());
        }

        if (430 <= outerTimerRdfId && outerTimerRdfId <= 510) {
            JPH_ASSERT(0 != p1Chd.ground_ud());
        }

        outerTimerRdfId++;
    }

    std::cout << "Passed TestCase19: Basic slip jump\n" << std::endl;
    theAllocator->Reset();
    reusedBattle->Clear();
    return true;
}

bool runTestCase20(FrontendBattle* reusedBattle, std::vector<std::vector<float>>& hulls, int inSingleJoinIndex, google::protobuf::Arena* theAllocator) {
    WsReq* initializerMapData = google::protobuf::Arena::Create<WsReq>(theAllocator);
    initTest20Data(initializerMapData, hulls, theAllocator);
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);

    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 512;
    int printIntervalRdfCnt = (1 << 5);

    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    int timerRdfId = -1, toGenIfdId = -1, localRequiredIfdId = -1; // shared 
    int chaserRdfIdLowerBound = -1, oldLcacIfdId = -1, newLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    int newChaserRdfId = 0;
    bool victoryTriggered = false;
    while (loopRdfCnt > outerTimerRdfId) {
        bool shouldPrint = false;
        uint64_t inSingleInput = getSelfCmdByRdfId(testCmds20, outerTimerRdfId);
        bool cmdInjected = FRONTEND_UpsertSelfCmd(reusedBattle, inSingleInput, &newChaserRdfId);
        if (!cmdInjected) {
            std::cerr << "TestCase20/Failed to inject cmd for outerTimerRdfId=" << outerTimerRdfId << ", inSingleInput=" << inSingleInput << std::endl;
            exit(1);
        }
        FRONTEND_Step(reusedBattle);

        RenderFrame* outerTimerRdf = reusedBattle->rdfBuffer.GetByFrameId(outerTimerRdfId);

        auto& tr1 = outerTimerRdf->triggers(0);
        auto& tr2 = outerTimerRdf->triggers(1);
        auto& tr3 = outerTimerRdf->triggers(2);

        auto& p1 = outerTimerRdf->players(0);
        auto& p1Chd = p1.chd();
        auto p1Ud = APP_CalcPlayerUserData(p1.join_index());

        auto* stepResult = reusedBattle->stepResultBuffer.GetByFrameId(outerTimerRdfId);

        if (180 > outerTimerRdfId) {
            // shouldPrint = true;
        }

        if (shouldPrint) {
            std::cout << "TestCase20/outerTimerRdfId=" << outerTimerRdfId << "\n\tp1Chd ud=" << p1Ud << ", hp=" << p1Chd.hp() << ", cs=" << p1Chd.ch_state() << ", fc=" << p1Chd.frames_in_ch_state() << ", q=(" << p1Chd.q_x() << ", " << p1Chd.q_y() << ", " << p1Chd.q_z() << ", " << p1Chd.q_w() << "), pos=(" << p1Chd.x() << ", " << p1Chd.y() << ", " << p1Chd.z() << "), vel=(" << p1Chd.vel_x() << ", " << p1Chd.vel_y() << ")" << ", stepResult fulfilled_triggers_size=" << stepResult->fulfilled_triggers_size() << std::endl;
        }

        if (1 == outerTimerRdfId) {
            JPH_ASSERT(2 == tr1.topo_lv());
            JPH_ASSERT(3 == tr2.topo_lv());
            JPH_ASSERT(1 == tr3.topo_lv());
        } else if (18 == outerTimerRdfId) {
            JPH_ASSERT(1 == stepResult->fulfilled_triggers_size());
            JPH_ASSERT(42 == stepResult->fulfilled_triggers(0).id());
        } else if (19 == outerTimerRdfId) {
            JPH_ASSERT(1 == stepResult->fulfilled_triggers_size());
            JPH_ASSERT(43 == stepResult->fulfilled_triggers(0).id());
        } else if (50 == outerTimerRdfId) {
            JPH_ASSERT(1 == outerTimerRdf->npc_count());
        } else if (108 == outerTimerRdfId) {
            JPH_ASSERT(2 == outerTimerRdf->npc_count());
        } else if (249 == outerTimerRdfId) {
            JPH_ASSERT(0 == outerTimerRdf->npc_count());
            if (TriggerState::TrReady == tr2.state() || TriggerState::TrExhaustedYetListening == tr2.state()) {
                JPH_ASSERT(1 == tr2.main_cycle_mask_to_fulfill());
            } else {
                JPH_ASSERT(0 == tr2.main_cycle_mask_to_fulfill());
            }
        } else if (250 <= outerTimerRdfId) {
            if (1 == stepResult->fulfilled_triggers_size()) {
                victoryTriggered = true;
                JPH_ASSERT(44 == stepResult->fulfilled_triggers(0).id());
                JPH_ASSERT(0 == tr3.quota());
                JPH_ASSERT(TriggerState::TrExhausted == tr3.state());
            }
        }
        
        outerTimerRdfId++;
    }

    JPH_ASSERT(victoryTriggered);
    std::cout << "Passed TestCase20: IndiWave NPC spawner\n" << std::endl;
    theAllocator->Reset();
    reusedBattle->Clear();

    return true;
}

bool runTestCase21(FrontendBattle* reusedBattle, std::vector<std::vector<float>>& hulls, int inSingleJoinIndex, google::protobuf::Arena* theAllocator) {
    WsReq* initializerMapData = google::protobuf::Arena::Create<WsReq>(theAllocator);
    initTest21Data(initializerMapData, hulls, theAllocator);
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);

    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 1024;
    int printIntervalRdfCnt = (1 << 4);
    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    jtshared::RenderFrame* outRdf = google::protobuf::Arena::Create<RenderFrame>(theAllocator);
    int newLcacIfdId = -1, newUdpLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    int newChaserRdfId = 0, newReferenceBattleChaserRdfId = 0;
    while (loopRdfCnt > outerTimerRdfId) {
        bool shouldPrint = false;
        // Handling TCP packets first, and then UDP packets, the same as C# side behavior.
        if (incomingDownsyncSnapshots21.count(outerTimerRdfId)) {
            DownsyncSnapshot* srvDownsyncSnapshot = incomingDownsyncSnapshots21[outerTimerRdfId];
            int outPostTimerRdfEvictedCnt = 0, outPostTimerRdfDelayedIfdEvictedCnt = 0;
            bool applied = reusedBattle->OnDownsyncSnapshotReceived(srvDownsyncSnapshot, &outPostTimerRdfEvictedCnt, &outPostTimerRdfDelayedIfdEvictedCnt, &newChaserRdfId, &newLcacIfdId, &newUdpLcacIfdId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
        }
        if (incomingUpsyncSnapshotReqs21.count(outerTimerRdfId)) {
            auto req = incomingUpsyncSnapshotReqs21[outerTimerRdfId]; 
            auto peerUpsyncSnapshot = req->upsync_snapshot();
            auto peerJoinIndex = req->join_index();
            bool applied = reusedBattle->OnUpsyncSnapshotReceived(peerJoinIndex, peerUpsyncSnapshot, &newChaserRdfId, &newUdpLcacIfdId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
            if (120 == outerTimerRdfId) {
                JPH_ASSERT(32ul == reusedBattle->playerInputFronts[1]);
                JPH_ASSERT(54 == newChaserRdfId);
                auto lastRecvIfd = reusedBattle->ifdBuffer.GetByFrameId(25);
                JPH_ASSERT(32ul == lastRecvIfd->input_list(1));
            }
        }
        uint64_t inSingleInput = getSelfCmdByRdfId(testCmds21, outerTimerRdfId);
        bool cmdInjected = FRONTEND_UpsertSelfCmd(reusedBattle, inSingleInput, &newChaserRdfId);
        if (!cmdInjected) {
            std::cerr << "Failed to inject cmd for outerTimerRdfId=" << outerTimerRdfId << ", inSingleInput=" << inSingleInput << std::endl;
            exit(1);
        }
        int chaserRdfIdSt = newChaserRdfId;
        int chaserRdfIdEd = newChaserRdfId + globalPrimitiveConsts->max_chasing_render_frames_per_update();
        if (chaserRdfIdEd > outerTimerRdfId) {
            chaserRdfIdEd = outerTimerRdfId;
        }
        FRONTEND_ChaseRolledBackRdfs(reusedBattle, &newChaserRdfId, true);
        FRONTEND_Step(reusedBattle);
        if (120 <= outerTimerRdfId && chaserRdfIdSt < 103 && 103 <= newChaserRdfId) {
            auto targetRdf = reusedBattle->rdfBuffer.GetByFrameId(103);
            auto& targetP2 = targetRdf->players(1);
            auto& targetP2Chd = targetP2.chd();
            JPH_ASSERT(0 < targetP2Chd.btn_b_holding_rdf_cnt());
        }

        if (120 <= outerTimerRdfId && chaserRdfIdSt < 107 && 107 <= newChaserRdfId) {
            auto targetRdf = reusedBattle->rdfBuffer.GetByFrameId(107);
            auto& targetP2 = targetRdf->players(1);
            auto& targetP2Chd = targetP2.chd();
            JPH_ASSERT(0 < targetP2Chd.btn_b_holding_rdf_cnt());
        }

        auto outerTimerRdf = reusedBattle->rdfBuffer.GetByFrameId(outerTimerRdfId);
        auto& p1 = outerTimerRdf->players(0);
        auto& p1Chd = p1.chd();
        const uint64_t ud1 = BaseBattleCollisionFilter::calcPlayerUserData(p1.join_index());
        auto& p2 = outerTimerRdf->players(1);
        auto& p2Chd = p2.chd();
        const uint64_t ud2 = BaseBattleCollisionFilter::calcPlayerUserData(p2.join_index());

        if (120 <= outerTimerRdfId && outerTimerRdfId < 240) {
            //shouldPrint = true;
        }
        
        if (shouldPrint) {
            std::cout << "TestCase21/outerTimerRdfId=" << outerTimerRdfId << "\n\tp1Chd ud1=" << ud1 << ", cs=" << p1Chd.ch_state() << ", fc=" << p1Chd.frames_in_ch_state() << ", dir=(" << p1Chd.q_x() << ", " << p1Chd.q_y() << ", " << p1Chd.q_z() << ", " << p1Chd.q_w() << "), pos=(" << p1Chd.x() << ", " << p1Chd.y() << "), vel=(" << p1Chd.vel_x() << ", " << p1Chd.vel_y() << "), gud=" << p1Chd.ground_ud() << "\n\tp2Chd ud2=" << ud2 << ", cs=" << p2Chd.ch_state() << ", fc=" << p2Chd.frames_in_ch_state() << ", btn_b_holding_rdf_cnt=" << p2Chd.btn_b_holding_rdf_cnt() << ", dir=(" << p2Chd.q_x() << ", " << p2Chd.q_y() << ", " << p2Chd.q_z() << ", " << p2Chd.q_w() << "), pos=(" << p2Chd.x() << ", " << p2Chd.y() << "), vel=(" << p2Chd.vel_x() << ", " << p2Chd.vel_y() << ")" << std::endl;
        }
        
        outerTimerRdfId++;
    }

    std::cout << "Passed TestCase21:Charge prediction\n" << std::endl;
    theAllocator->Reset();
    reusedBattle->Clear();   
    return true;
}

bool runTestCase22(FrontendBattle* reusedBattle, std::vector<std::vector<float>>& hulls, int inSingleJoinIndex, google::protobuf::Arena* theAllocator) {
    WsReq* initializerMapData = google::protobuf::Arena::Create<WsReq>(theAllocator);
    initTest22Data(initializerMapData, hulls, theAllocator);
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);

    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 512;
    int printIntervalRdfCnt = (1 << 5);

    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    int timerRdfId = -1, toGenIfdId = -1, localRequiredIfdId = -1; // shared 
    int chaserRdfIdLowerBound = -1, oldLcacIfdId = -1, newLcacIfdId = -1, newUdpLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    int newChaserRdfId = 0;
    bool victoryTriggered = false;
    while (loopRdfCnt > outerTimerRdfId) {
        bool shouldPrint = false;
        uint64_t inSingleInput = getSelfCmdByRdfId(testCmds22, outerTimerRdfId);
        bool cmdInjected = FRONTEND_UpsertSelfCmd(reusedBattle, inSingleInput, &newChaserRdfId);
        if (!cmdInjected) {
            std::cerr << "TestCase22/Failed to inject cmd for outerTimerRdfId=" << outerTimerRdfId << ", inSingleInput=" << inSingleInput << std::endl;
            exit(1);
        }
        FRONTEND_Step(reusedBattle);

        RenderFrame* outerTimerRdf = reusedBattle->rdfBuffer.GetByFrameId(outerTimerRdfId);

        auto& tr1 = outerTimerRdf->triggers(0);
        auto& tr2 = outerTimerRdf->triggers(1);
        auto& tr3 = outerTimerRdf->triggers(2);

        auto& p1 = outerTimerRdf->players(0);
        auto& p1Chd = p1.chd();
        auto p1Ud = APP_CalcPlayerUserData(p1.join_index());

        auto* stepResult = reusedBattle->stepResultBuffer.GetByFrameId(outerTimerRdfId);

        if (180 > outerTimerRdfId) {
            //shouldPrint = true;
        }

        if (shouldPrint) {
            std::cout << "TestCase22/outerTimerRdfId=" << outerTimerRdfId << "\n\tp1Chd ud=" << p1Ud << ", hp=" << p1Chd.hp() << ", cs=" << p1Chd.ch_state() << ", fc=" << p1Chd.frames_in_ch_state() << ", q=(" << p1Chd.q_x() << ", " << p1Chd.q_y() << ", " << p1Chd.q_z() << ", " << p1Chd.q_w() << "), pos=(" << p1Chd.x() << ", " << p1Chd.y() << ", " << p1Chd.z() << "), vel=(" << p1Chd.vel_x() << ", " << p1Chd.vel_y() << ")" << std::endl;
        }

        outerTimerRdfId++;
    }

    std::cout << "Passed TestCase22: Aiming Ray\n" << std::endl;
    theAllocator->Reset();
    reusedBattle->Clear();

    return true;
}

bool runTestCase23(FrontendBattle* reusedBattle, std::vector<std::vector<float>>& hulls, int inSingleJoinIndex, google::protobuf::Arena* theAllocator) {
    bool doCompareWithRollback = true; 

    WsReq* initializerMapData = google::protobuf::Arena::Create<WsReq>(theAllocator);
    initTest23Data(initializerMapData, hulls, theAllocator);
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);

    FrontendBattle* referenceBattle = static_cast<FrontendBattle*>(FRONTEND_CreateBattle(512, true));
    referenceBattle->SetFrameLogEnabled(true);
    referenceBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);

    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 1024;
    int printIntervalRdfCnt = (1 << 5);
    
    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    int timerRdfId = -1, toGenIfdId = -1, localRequiredIfdId = -1; // shared 
    int chaserRdfIdLowerBound = -1, oldLcacIfdId = -1, newLcacIfdId = -1, newUdpLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    int newChaserRdfId = 0, newReferenceBattleChaserRdfId = 0, newReferenceBattleUdpLcacIfdId = -1;
    int referenceBattleChaserRdfId = -1, referenceBattleChaserRdfIdLowerBound = -1, referenceBattleOldLcacIfdId = -1, referenceBattleNewLcacIfdId = -1, referenceBattleMaxPlayerInputFrontId = 0, referenceBattleMinPlayerInputFrontId = 0;
    jtshared::RenderFrame* outRdf = google::protobuf::Arena::Create<RenderFrame>(theAllocator);
    jtshared::RenderFrame* referenceBattleOutRdf = google::protobuf::Arena::Create<RenderFrame>(theAllocator);
    while (loopRdfCnt > outerTimerRdfId) {
        if (incomingUpsyncSnapshotReqs23Intime.count(outerTimerRdfId)) {
            auto req = incomingUpsyncSnapshotReqs23Intime[outerTimerRdfId];
            auto peerUpsyncSnapshot = req->upsync_snapshot();
            referenceBattle->OnUpsyncSnapshotReceived(req->join_index(), peerUpsyncSnapshot, &newReferenceBattleChaserRdfId, &newReferenceBattleUdpLcacIfdId, &referenceBattleMaxPlayerInputFrontId, &referenceBattleMinPlayerInputFrontId);
        }

        if (doCompareWithRollback) {
            if (incomingUpsyncSnapshotReqs23Rollback.count(outerTimerRdfId)) {
                auto req = incomingUpsyncSnapshotReqs23Rollback[outerTimerRdfId];
                auto peerUpsyncSnapshot = req->upsync_snapshot();
                reusedBattle->OnUpsyncSnapshotReceived(req->join_index(), peerUpsyncSnapshot, &newChaserRdfId, &newUdpLcacIfdId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
                JPH_ASSERT(newUdpLcacIfdId == reusedBattle->udpLcacIfdId);
                if (240 == outerTimerRdfId) {
                    JPH_ASSERT(-1 == newUdpLcacIfdId);
                } else if (280 == outerTimerRdfId) {
                    JPH_ASSERT(5 == newUdpLcacIfdId);
                } else if (320 == outerTimerRdfId) {
                    JPH_ASSERT(16 == newUdpLcacIfdId);
                } else if (360 == outerTimerRdfId) {
                    JPH_ASSERT(16 == newUdpLcacIfdId);
                } else if (400 == outerTimerRdfId) {
                    JPH_ASSERT(32 == newUdpLcacIfdId);
                } else if (420 == outerTimerRdfId) {
                    JPH_ASSERT(32 == newUdpLcacIfdId);
                } else if (480 == outerTimerRdfId) {
                    JPH_ASSERT(127 == newUdpLcacIfdId);
                }
            }
        }

        uint64_t inSingleInput = getSelfCmdByRdfId(testCmds23, outerTimerRdfId);
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
            StepResult*  challengingStepResult = reusedBattle->stepResultBuffer.GetByFrameId(outerTimerRdfId); 
            JPH_ASSERT(nullptr != challengingStepResult);

            const PlayerCharacterDownsync& referencedP1 = referencedRdf->players(0);
            const CharacterDownsync& p1Chd = referencedP1.chd();
            const PlayerCharacterDownsync& referencedP2 = referencedRdf->players(1);
            const CharacterDownsync& p2Chd = referencedP2.chd();

            if (loopRdfCnt < outerTimerRdfId + 10) {
                
                //shouldPrint = true;

                BaseBattle::AssertNearlySame(referencedRdf, challengingRdf);
            } else if (109 == outerTimerRdfId) {
                auto& refBl1 = referencedRdf->bullets(0);
                auto& refBl2 = referencedRdf->bullets(1);
                JPH_ASSERT(BulletState::Vanishing == refBl1.bl_state());
                JPH_ASSERT(BulletState::Vanishing == refBl2.bl_state());
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
                std::cout << "TestCase23/outerTimerRdfId=" << outerTimerRdfId << "\n\tp1Chd cs=" << p1Chd.ch_state() << ", fc=" << p1Chd.frames_in_ch_state() << ", dir=(" << p1Chd.q_x() << ", " << p1Chd.q_y() << ", " << p1Chd.q_z() << ", " << p1Chd.q_w() << "), pos=(" << p1Chd.x() << ", " << p1Chd.y() << "), vel=(" << p1Chd.vel_x() << ", " << p1Chd.vel_y() << ")\n\tp2Chd cs=" << p2Chd.ch_state() << ", fc=" << p2Chd.frames_in_ch_state() << ", dir=(" << p2Chd.q_x() << ", " << p2Chd.q_y() << ", " << p2Chd.q_z() << ", " << p2Chd.q_w() << "), pos=(" << p2Chd.x() << ", " << p2Chd.y() << "), vel=(" << p2Chd.vel_x() << ", " << p2Chd.vel_y() << ")" << std::endl;
            }
        }
        outerTimerRdfId++;
       
    }

    std::cout << "Passed TestCase23: Pistol bullet collision\n" << std::endl;
    theAllocator->Reset();
    reusedBattle->Clear();   
    APP_DestroyBattle(referenceBattle);
    return true;
}

bool runTestCase24(FrontendBattle* reusedBattle, std::vector<std::vector<float>>& hulls, int inSingleJoinIndex, google::protobuf::Arena* theAllocator) {
    WsReq* initializerMapData = google::protobuf::Arena::Create<WsReq>(theAllocator);
    initTest24Data(initializerMapData, hulls, theAllocator);
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);

    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 1024;
    int printIntervalRdfCnt = (1 << 5);
    
    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    int timerRdfId = -1, toGenIfdId = -1, localRequiredIfdId = -1; // shared 
    int chaserRdfIdLowerBound = -1, oldLcacIfdId = -1, newLcacIfdId = -1, newUdpLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    int newChaserRdfId = 0;
    jtshared::RenderFrame* outRdf = google::protobuf::Arena::Create<RenderFrame>(theAllocator);
    while (loopRdfCnt > outerTimerRdfId) {
        uint64_t inSingleInput = getSelfCmdByRdfId(testCmds24, outerTimerRdfId);
        bool cmdInjected = FRONTEND_UpsertSelfCmd(reusedBattle, inSingleInput, &newChaserRdfId);
        if (!cmdInjected) {
            std::cerr << "Failed to inject cmd for outerTimerRdfId=" << outerTimerRdfId << ", inSingleInput=" << inSingleInput << std::endl;
            exit(1);
        }

        bool shouldPrint = false;
        int chaserRdfIdEd = outerTimerRdfId;

        FRONTEND_ChaseRolledBackRdfs(reusedBattle, &newChaserRdfId, true);
        FRONTEND_Step(reusedBattle);

        auto outerTimerRdf = reusedBattle->rdfBuffer.GetByFrameId(outerTimerRdfId);
        StepResult*  outStepResult = reusedBattle->stepResultBuffer.GetByFrameId(outerTimerRdfId); 

        const PlayerCharacterDownsync& p1 = outerTimerRdf->players(0);
        const CharacterDownsync& p1Chd = p1.chd();
        const PlayerCharacterDownsync& p2 = outerTimerRdf->players(1);
        const CharacterDownsync& p2Chd = p2.chd();

        if (84 <= outerTimerRdfId && outerTimerRdfId < 124) {
            //shouldPrint = true;
        }

        if (shouldPrint) {
            std::cout << "TestCase24/outerTimerRdfId=" << outerTimerRdfId << "\n\tp1Chd cs=" << p1Chd.ch_state() << ", fc=" << p1Chd.frames_in_ch_state() << ", dir=(" << p1Chd.q_x() << ", " << p1Chd.q_y() << ", " << p1Chd.q_z() << ", " << p1Chd.q_w() << "), pos=(" << p1Chd.x() << ", " << p1Chd.y() << "), vel=(" << p1Chd.vel_x() << ", " << p1Chd.vel_y() << ")\n\tp2Chd hp=" << p2Chd.hp() << ", cs=" << p2Chd.ch_state() << ", fc=" << p2Chd.frames_in_ch_state() << ", dir=(" << p2Chd.q_x() << ", " << p2Chd.q_y() << ", " << p2Chd.q_z() << ", " << p2Chd.q_w() << "), pos=(" << p2Chd.x() << ", " << p2Chd.y() << "), vel=(" << p2Chd.vel_x() << ", " << p2Chd.vel_y() << ")" << std::endl;
        }

        if (420 == outerTimerRdfId) {
            JPH_ASSERT(0 >= p1Chd.atk1_magazine().quota());
        } else if (450 == outerTimerRdfId) {
            JPH_ASSERT(0 < p1Chd.atk1_magazine().quota());
        }
        outerTimerRdfId++;
       
    }

    std::cout << "Passed TestCase24: Pistol magazine reload\n" << std::endl;
    theAllocator->Reset();
    reusedBattle->Clear();   
    return true;
}

bool runTestCase25(FrontendBattle* reusedBattle, std::vector<std::vector<float>>& hulls, int inSingleJoinIndex, google::protobuf::Arena* theAllocator) {
    WsReq* initializerMapData = google::protobuf::Arena::Create<WsReq>(theAllocator);
    initTest25Data(initializerMapData, hulls, theAllocator);
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);

    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 1024;
    int printIntervalRdfCnt = (1 << 5);

    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    int timerRdfId = -1, toGenIfdId = -1, localRequiredIfdId = -1; // shared 
    int chaserRdfIdLowerBound = -1, oldLcacIfdId = -1, newLcacIfdId = -1, newUdpLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    int newChaserRdfId = 0;
    
    jtshared::RenderFrame* outRdf = google::protobuf::Arena::Create<RenderFrame>(theAllocator);
    while (loopRdfCnt > outerTimerRdfId) {
        uint64_t inSingleInput = getSelfCmdByRdfId(testCmds25, outerTimerRdfId);
        bool cmdInjected = FRONTEND_UpsertSelfCmd(reusedBattle, inSingleInput, &newChaserRdfId);
        if (!cmdInjected) {
            std::cerr << "Failed to inject cmd for outerTimerRdfId=" << outerTimerRdfId << ", inSingleInput=" << inSingleInput << std::endl;
            exit(1);
        }

        bool shouldPrint = false;
        int chaserRdfIdEd = outerTimerRdfId;

        FRONTEND_Step(reusedBattle);

        auto outerTimerRdf = reusedBattle->rdfBuffer.GetByFrameId(outerTimerRdfId);
        StepResult* outStepResult = reusedBattle->stepResultBuffer.GetByFrameId(outerTimerRdfId);

        const PlayerCharacterDownsync& p1 = outerTimerRdf->players(0);
        const CharacterDownsync& p1Chd = p1.chd();

        const Trap& trap1 = outerTimerRdf->dynamic_traps(0);
        const uint64_t trap1Ud = APP_CalcTrapUserData(trap1.id());

        const Trap& trap2 = outerTimerRdf->dynamic_traps(1);
        const uint64_t trap2Ud = APP_CalcTrapUserData(trap2.id());

        if (446 <= outerTimerRdfId && outerTimerRdfId < 550) {
            //shouldPrint = true;
        }

        if (trap1Ud == p1Chd.ground_ud()) {
            //shouldPrint = true;
        }

        if (shouldPrint) {
            std::cout << "TestCase25/outerTimerRdfId=" << outerTimerRdfId << "\n\tp1Chd cs=" << p1Chd.ch_state() << ", fc=" << p1Chd.frames_in_ch_state() << ", gud=" << p1Chd.ground_ud() << ", dir=(" << p1Chd.q_x() << ", " << p1Chd.q_y() << ", " << p1Chd.q_z() << ", " << p1Chd.q_w() << "), pos=(" << p1Chd.x() << ", " << p1Chd.y() << "), vel=(" << p1Chd.vel_x() << ", " << p1Chd.vel_y() << "), ground_vel=(" << p1Chd.ground_vel_x() << ", " << p1Chd.ground_vel_y() << ")\n\ttrap1 ud=" << trap1Ud << ", pos=(" << trap1.x() << ", " << trap1.y() << "), vel=(" << trap1.vel_x() << ", " << trap1.vel_y() << "), dir=(" << trap1.q_x() << ", " << trap1.q_y() << ", " << trap1.q_z() << ", " << trap1.q_w() << "), ang_vel=(" << trap1.ang_vel_x() << ", " << trap1.ang_vel_y() << ", " << trap1.ang_vel_z() << ")\n\ttrap2 ud=" << trap2Ud << ", pos=(" << trap2.x() << ", " << trap2.y() << "), vel=(" << trap2.vel_x() << ", " << trap2.vel_y() << "), dir=(" << trap2.q_x() << ", " << trap2.q_y() << ", " << trap2.q_z() << ", " << trap2.q_w() << "), ang_vel=(" << trap2.ang_vel_x() << ", " << trap2.ang_vel_y() << ", " << trap2.ang_vel_z() << ")" << std::endl;
        }

        if (280 == outerTimerRdfId) {
            JPH_ASSERT(trap1Ud == p1Chd.ground_ud());
            JPH_ASSERT(0 < p1Chd.vel_x());
            JPH_ASSERT(Idle1 == p1Chd.ch_state());
        } else if (490 <= outerTimerRdfId && outerTimerRdfId < 500) {
            JPH_ASSERT(CrouchIdle1 == p1Chd.ch_state());
        } else if (510 == outerTimerRdfId) {
            JPH_ASSERT(Sliding == p1Chd.ch_state());
            JPH_ASSERT(0 < p1Chd.vel_x());
        } else if (600 == outerTimerRdfId) {
            JPH_ASSERT(CrouchIdle1 == p1Chd.ch_state());
        }

        outerTimerRdfId++;
    }

    std::cout << "Passed TestCase25: Conveyor belt and basic rotating platform\n" << std::endl;
    theAllocator->Reset();
    reusedBattle->Clear();

    return true;
}

bool runTestCase26(FrontendBattle* reusedBattle, std::vector<std::vector<float>>& hulls, int inSingleJoinIndex, google::protobuf::Arena* theAllocator) {
    WsReq* initializerMapData = google::protobuf::Arena::Create<WsReq>(theAllocator);
    initTest26Data(initializerMapData, hulls, theAllocator);
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);

    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 1024;
    int printIntervalRdfCnt = (1 << 5);

    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    int timerRdfId = -1, toGenIfdId = -1, localRequiredIfdId = -1; // shared 
    int chaserRdfIdLowerBound = -1, oldLcacIfdId = -1, newLcacIfdId = -1, newUdpLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    int newChaserRdfId = 0;
    
    jtshared::RenderFrame* outRdf = google::protobuf::Arena::Create<RenderFrame>(theAllocator);
    while (loopRdfCnt > outerTimerRdfId) {
        uint64_t inSingleInput = getSelfCmdByRdfId(testCmds26, outerTimerRdfId);
        bool cmdInjected = FRONTEND_UpsertSelfCmd(reusedBattle, inSingleInput, &newChaserRdfId);
        if (!cmdInjected) {
            std::cerr << "Failed to inject cmd for outerTimerRdfId=" << outerTimerRdfId << ", inSingleInput=" << inSingleInput << std::endl;
            exit(1);
        }

        bool shouldPrint = false;
        int chaserRdfIdEd = outerTimerRdfId;

        FRONTEND_ChaseRolledBackRdfs(reusedBattle, &newChaserRdfId, true);
        FRONTEND_Step(reusedBattle);

        auto outerTimerRdf = reusedBattle->rdfBuffer.GetByFrameId(outerTimerRdfId);
        StepResult* outStepResult = reusedBattle->stepResultBuffer.GetByFrameId(outerTimerRdfId);

        const PlayerCharacterDownsync& p1 = outerTimerRdf->players(0);
        const CharacterDownsync& p1Chd = p1.chd();

        const NpcCharacterDownsync& npc1 = outerTimerRdf->npcs(0);
        const CharacterDownsync& npc1Chd = npc1.chd();

        const Trap& tp1 = outerTimerRdf->dynamic_traps(0);
        const uint64_t tp1Ud = APP_CalcTrapUserData(tp1.id());

        if (0 <= outerTimerRdfId && outerTimerRdfId < 300) {
            //shouldPrint=true;
        }

        if (shouldPrint) {
            std::cout << "TestCase26/outerTimerRdfId=" << outerTimerRdfId << "\n\tp1Chd cs=" << p1Chd.ch_state() << ", fc=" << p1Chd.frames_in_ch_state() << ", gud=" << p1Chd.ground_ud() << ", dir=(" << p1Chd.q_x() << ", " << p1Chd.q_y() << ", " << p1Chd.q_z() << ", " << p1Chd.q_w() << "), pos=(" << p1Chd.x() << ", " << p1Chd.y() << "), vel=(" << p1Chd.vel_x() << ", " << p1Chd.vel_y() << "), ground_vel=(" << p1Chd.ground_vel_x() << ", " << p1Chd.ground_vel_y() << "), ground_ud=" << p1Chd.ground_ud() << "\n\tnpc1Chd hp=" << npc1Chd.hp() << ", cs=" << npc1Chd.ch_state() << ", fc=" << npc1Chd.frames_in_ch_state() << ", dir=(" << npc1Chd.q_x() << ", " << npc1Chd.q_y() << ", " << npc1Chd.q_z() << ", " << npc1Chd.q_w() << "), pos=(" << npc1Chd.x() << ", " << npc1Chd.y() << "), vel=(" << npc1Chd.vel_x() << ", " << npc1Chd.vel_y() << "), ground_vel=(" << npc1Chd.ground_vel_x() << ", " << npc1Chd.ground_vel_y() << "), ground_ud=" << npc1Chd.ground_ud() << "\n\ttrap1, pos=(" << tp1.x() << ", " << tp1.y() << "), q=(" << tp1.q_x() << ", " << tp1.q_y() << ", " << tp1.q_z() << ", " << tp1.q_w() << "), vel=(" << tp1.vel_x() << ", " << tp1.vel_y() << "), trapState=" << (int)tp1.trap_state() << ", framesInTrapState=" << tp1.frames_in_trap_state() << ", ud=" << APP_CalcTrapUserData(tp1.id()) << std::endl;
        }

         if (105 == outerTimerRdfId) {
            JPH_ASSERT(0 == npc1Chd.ground_ud());
            JPH_ASSERT(0 < npc1Chd.vel_y());
            JPH_ASSERT(0 > npc1Chd.vel_x());
            JPH_ASSERT(CharacterState::InAirIdle1ByJump == npc1Chd.ch_state()); // Jumping onto sliding platform
        } else if (250 == outerTimerRdfId) {
            JPH_ASSERT(Sliding == p1Chd.ch_state());
            JPH_ASSERT(tp1Ud == npc1Chd.ground_ud()); 
            JPH_ASSERT(BaseBattleCollisionFilter::IsLengthNearZero(tp1.vel_x() - npc1Chd.ground_vel_x())); 
        } else if (280 == outerTimerRdfId) {
            JPH_ASSERT(tp1Ud == npc1Chd.ground_ud()); // Jumped onto sliding platform
        } else if (300 == outerTimerRdfId) {
            JPH_ASSERT(CrouchIdle1 == p1Chd.ch_state());
        }

        outerTimerRdfId++;
    }

    std::cout << "Passed TestCase26: Force crouching\n" << std::endl;
    theAllocator->Reset();
    reusedBattle->Clear();

    return true;
}

bool runTestCase27(FrontendBattle* reusedBattle, std::vector<std::vector<float>>& hulls, int inSingleJoinIndex, google::protobuf::Arena* theAllocator) {
    WsReq* initializerMapData = google::protobuf::Arena::Create<WsReq>(theAllocator);
    initTest27Data(initializerMapData, hulls, theAllocator);
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);

    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 1024;
    int printIntervalRdfCnt = (1 << 5);

    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    int timerRdfId = -1, toGenIfdId = -1, localRequiredIfdId = -1; // shared 
    int chaserRdfIdLowerBound = -1, oldLcacIfdId = -1, newLcacIfdId = -1, newUdpLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    int newChaserRdfId = 0;
    
    jtshared::RenderFrame* outRdf = google::protobuf::Arena::Create<RenderFrame>(theAllocator);
    while (loopRdfCnt > outerTimerRdfId) {
        uint64_t inSingleInput = getSelfCmdByRdfId(testCmds27, outerTimerRdfId);
        bool cmdInjected = FRONTEND_UpsertSelfCmd(reusedBattle, inSingleInput, &newChaserRdfId);
        if (!cmdInjected) {
            std::cerr << "Failed to inject cmd for outerTimerRdfId=" << outerTimerRdfId << ", inSingleInput=" << inSingleInput << std::endl;
            exit(1);
        }

        bool shouldPrint = false;
        int chaserRdfIdEd = outerTimerRdfId;

        FRONTEND_Step(reusedBattle);

        auto outerTimerRdf = reusedBattle->rdfBuffer.GetByFrameId(outerTimerRdfId);
        StepResult* outStepResult = reusedBattle->stepResultBuffer.GetByFrameId(outerTimerRdfId);

        const PlayerCharacterDownsync& p1 = outerTimerRdf->players(0);
        const CharacterDownsync& p1Chd = p1.chd();

        const NpcCharacterDownsync& npc1 = outerTimerRdf->npcs(0);
        const CharacterDownsync& npc1Chd = npc1.chd();

        if (272 <= outerTimerRdfId && outerTimerRdfId < 380) {
            //shouldPrint = true;
        }

        if (shouldPrint) {
            std::cout << "TestCase27/outerTimerRdfId=" << outerTimerRdfId << "\n\tp1Chd cs=" << p1Chd.ch_state() << ", fc=" << p1Chd.frames_in_ch_state() << ", gud=" << p1Chd.ground_ud() << ", dir=(" << p1Chd.q_x() << ", " << p1Chd.q_y() << ", " << p1Chd.q_z() << ", " << p1Chd.q_w() << "), pos=(" << p1Chd.x() << ", " << p1Chd.y() << "), vel=(" << p1Chd.vel_x() << ", " << p1Chd.vel_y() << "), ground_vel=(" << p1Chd.ground_vel_x() << ", " << p1Chd.ground_vel_y() << ")\n\tnpc1Chd hp=" << npc1Chd.hp() << ", cs=" << npc1Chd.ch_state() << ", fc=" << npc1Chd.frames_in_ch_state() << ", dir=(" << npc1Chd.q_x() << ", " << npc1Chd.q_y() << ", " << npc1Chd.q_z() << ", " << npc1Chd.q_w() << "), pos=(" << npc1Chd.x() << ", " << npc1Chd.y() << "), vel=(" << npc1Chd.vel_x() << ", " << npc1Chd.vel_y() << ")" << std::endl;
        }

        if (360 == outerTimerRdfId) {
            JPH_ASSERT(30 == npc1Chd.hp());
            JPH_ASSERT(0 > npc1Chd.vel_x());
            JPH_ASSERT(NpcGoal::NPatrol == npc1.goal_as_npc());
            JPH_ASSERT(0 == npc1Chd.locking_on_ud());
        }

        outerTimerRdfId++;
    }

    std::cout << "Passed TestCase27: NPC bullet hit reaction from far away offender\n" << std::endl;
    theAllocator->Reset();
    reusedBattle->Clear();

    return true;
}

bool runTestCase28(FrontendBattle* reusedBattle, std::vector<std::vector<float>>& hulls, int inSingleJoinIndex, google::protobuf::Arena* theAllocator) {
    WsReq* initializerMapData = google::protobuf::Arena::Create<WsReq>(theAllocator);
    initTest28Data(initializerMapData, hulls, theAllocator);
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);

    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 1024;
    int printIntervalRdfCnt = (1 << 5);

    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    int timerRdfId = -1, toGenIfdId = -1, localRequiredIfdId = -1; // shared 
    int chaserRdfIdLowerBound = -1, oldLcacIfdId = -1, newLcacIfdId = -1, newUdpLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    int newChaserRdfId = 0;
    
    jtshared::RenderFrame* outRdf = google::protobuf::Arena::Create<RenderFrame>(theAllocator);
    while (loopRdfCnt > outerTimerRdfId) {
        uint64_t inSingleInput = getSelfCmdByRdfId(testCmds28, outerTimerRdfId);
        bool cmdInjected = FRONTEND_UpsertSelfCmd(reusedBattle, inSingleInput, &newChaserRdfId);
        if (!cmdInjected) {
            std::cerr << "Failed to inject cmd for outerTimerRdfId=" << outerTimerRdfId << ", inSingleInput=" << inSingleInput << std::endl;
            exit(1);
        }

        bool shouldPrint = false;
        int chaserRdfIdEd = outerTimerRdfId;

        FRONTEND_Step(reusedBattle);

        auto outerTimerRdf = reusedBattle->rdfBuffer.GetByFrameId(outerTimerRdfId);
        StepResult* outStepResult = reusedBattle->stepResultBuffer.GetByFrameId(outerTimerRdfId);

        const PlayerCharacterDownsync& p1 = outerTimerRdf->players(0);
        const CharacterDownsync& p1Chd = p1.chd();

        const NpcCharacterDownsync& npc1 = outerTimerRdf->npcs(0);
        const CharacterDownsync& npc1Chd = npc1.chd();

        if (66 <= outerTimerRdfId && outerTimerRdfId < 72) {
            //shouldPrint = true;
        }

        if (367 <= outerTimerRdfId && outerTimerRdfId < 480) {
            //shouldPrint = true;
        }

        if (shouldPrint) {
            std::cout << "TestCase28/outerTimerRdfId=" << outerTimerRdfId << "\n\tp1Chd hp=" << p1Chd.hp() << ", cs=" << p1Chd.ch_state() << ", fc=" << p1Chd.frames_in_ch_state() << ", gud=" << p1Chd.ground_ud() << ", dir=(" << p1Chd.q_x() << ", " << p1Chd.q_y() << ", " << p1Chd.q_z() << ", " << p1Chd.q_w() << "), pos=(" << p1Chd.x() << ", " << p1Chd.y() << "), vel=(" << p1Chd.vel_x() << ", " << p1Chd.vel_y() << "), ground_vel=(" << p1Chd.ground_vel_x() << ", " << p1Chd.ground_vel_y() << ")\n\tnpc1Chd hp=" << npc1Chd.hp() << ", mp=" << npc1Chd.mp() << ", cs=" << npc1Chd.ch_state() << ", fc=" << npc1Chd.frames_in_ch_state() << ", dir=(" << npc1Chd.q_x() << ", " << npc1Chd.q_y() << ", " << npc1Chd.q_z() << ", " << npc1Chd.q_w() << "), pos=(" << npc1Chd.x() << ", " << npc1Chd.y() << "), vel=(" << npc1Chd.vel_x() << ", " << npc1Chd.vel_y() << "), cuedCmd=" << npc1.cached_cue_cmd() << std::endl;
        }

        if (67 == outerTimerRdfId) {
            JPH_ASSERT(0 == npc1Chd.vel_x());
            JPH_ASSERT(globalPrimitiveConsts->no_skill() != npc1Chd.active_skill_id());
            JPH_ASSERT(globalPrimitiveConsts->no_skill_hit() != npc1Chd.active_skill_hit());
        } else if (68 == outerTimerRdfId) {
            const Bullet& bl1 = outerTimerRdf->bullets(0);
            JPH_ASSERT(1 == bl1.id());
            JPH_ASSERT(0 > bl1.vel_x());
            JPH_ASSERT(4 == outerTimerRdf->bullet_count());
        } else if (201 == outerTimerRdfId) {
            // Fled due to not enough MP.
            JPH_ASSERT(3 == npc1.cached_cue_cmd());
            JPH_ASSERT(0 < npc1Chd.vel_x());
            JPH_ASSERT(0 == npc1Chd.locking_on_ud());
            JPH_ASSERT(NpcGoal::NPatrol == npc1.goal_as_npc());
        } else if (310 == outerTimerRdfId) {
            // Stopped by MvBlocker due to fleeing grace period.
            JPH_ASSERT(0 == npc1.cached_cue_cmd());
            JPH_ASSERT(0 < npc1.last_fled_rdf_id());
            JPH_ASSERT(300 > npc1Chd.mp());
            JPH_ASSERT(BaseBattle::IsLengthNearZero(npc1Chd.vel_x()*globalPrimitiveConsts->estimated_seconds_per_rdf()));
            JPH_ASSERT(0 == npc1Chd.locking_on_ud());
            JPH_ASSERT(NpcGoal::NPatrol == npc1.goal_as_npc());
        } else if (380 == outerTimerRdfId) {
            JPH::Quat npc1ChdQ(npc1Chd.q_x(), npc1Chd.q_y(), npc1Chd.q_z(), npc1Chd.q_w());
            JPH_ASSERT(cTurnbackAroundYAxis.IsClose(npc1ChdQ)); // Got enough MP and thus turnaround to attack again
            JPH_ASSERT(globalPrimitiveConsts->no_skill() != npc1Chd.active_skill_id());
            JPH_ASSERT(globalPrimitiveConsts->no_skill_hit() != npc1Chd.active_skill_hit());

            int p1Hp = p1Chd.hp();
            JPH_ASSERT(142 == p1Hp);
        } else if (480 == outerTimerRdfId) {
            int p1Hp = p1Chd.hp();
            JPH_ASSERT(134 == p1Hp);
        }
        
        outerTimerRdfId++;
    }

    std::cout << "Passed TestCase28: BlackShooter1 attack\n" << std::endl;
    theAllocator->Reset();
    reusedBattle->Clear();

    return true;
}

bool runTestCase29(FrontendBattle* reusedBattle, std::vector<std::vector<float>>& hulls, int inSingleJoinIndex, google::protobuf::Arena* theAllocator) {
    WsReq* initializerMapData = google::protobuf::Arena::Create<WsReq>(theAllocator);
    initTest29Data(initializerMapData, hulls, theAllocator);
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);

    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 1200;
    int printIntervalRdfCnt = (1 << 5);

    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    int timerRdfId = -1, toGenIfdId = -1, localRequiredIfdId = -1; // shared 
    int chaserRdfIdLowerBound = -1, oldLcacIfdId = -1, newLcacIfdId = -1, newUdpLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    int newChaserRdfId = 0;

    jtshared::RenderFrame* outRdf = google::protobuf::Arena::Create<RenderFrame>(theAllocator);
    float npc1NotEnoughMpX = 0;
    while (loopRdfCnt > outerTimerRdfId) {
        uint64_t inSingleInput = getSelfCmdByRdfId(testCmds29, outerTimerRdfId);
        bool cmdInjected = FRONTEND_UpsertSelfCmd(reusedBattle, inSingleInput, &newChaserRdfId);
        if (!cmdInjected) {
            std::cerr << "Failed to inject cmd for outerTimerRdfId=" << outerTimerRdfId << ", inSingleInput=" << inSingleInput << std::endl;
            exit(1);
        }

        bool shouldPrint = false;
        int chaserRdfIdEd = outerTimerRdfId;

        FRONTEND_Step(reusedBattle);

        auto outerTimerRdf = reusedBattle->rdfBuffer.GetByFrameId(outerTimerRdfId);
        StepResult* outStepResult = reusedBattle->stepResultBuffer.GetByFrameId(outerTimerRdfId);

        const PlayerCharacterDownsync& p1 = outerTimerRdf->players(0);
        const CharacterDownsync& p1Chd = p1.chd();

        const NpcCharacterDownsync& npc1 = outerTimerRdf->npcs(0);
        const CharacterDownsync& npc1Chd = npc1.chd();

        if (240 <= outerTimerRdfId && outerTimerRdfId < 300) {
            //shouldPrint = true;
        }

        if (955 <= outerTimerRdfId && outerTimerRdfId < loopRdfCnt) {
            //shouldPrint = true;
        }

        if (shouldPrint) {
            std::cout << "TestCase29/outerTimerRdfId=" << outerTimerRdfId << "\n\tp1Chd hp=" << p1Chd.hp() << ", cs=" << p1Chd.ch_state() << ", fc=" << p1Chd.frames_in_ch_state() << ", gud=" << p1Chd.ground_ud() << ", dir=(" << p1Chd.q_x() << ", " << p1Chd.q_y() << ", " << p1Chd.q_z() << ", " << p1Chd.q_w() << "), pos=(" << p1Chd.x() << ", " << p1Chd.y() << "), vel=(" << p1Chd.vel_x() << ", " << p1Chd.vel_y() << "), ground_vel=(" << p1Chd.ground_vel_x() << ", " << p1Chd.ground_vel_y() << ")\n\tnpc1Chd hp=" << npc1Chd.hp() << ", mp=" << npc1Chd.mp() << ", cs=" << npc1Chd.ch_state() << ", fc=" << npc1Chd.frames_in_ch_state() << ", dir=(" << npc1Chd.q_x() << ", " << npc1Chd.q_y() << ", " << npc1Chd.q_z() << ", " << npc1Chd.q_w() << "), pos=(" << npc1Chd.x() << ", " << npc1Chd.y() << "), vel=(" << npc1Chd.vel_x() << ", " << npc1Chd.vel_y() << "), cuedCmd=" << npc1.cached_cue_cmd() << std::endl;
        }


        if (325 == outerTimerRdfId) {
            JPH_ASSERT(1 == outerTimerRdf->bullet_count());
            const Bullet& bl1 = outerTimerRdf->bullets(0);
            JPH_ASSERT(1 == bl1.id());
            JPH_ASSERT(Active == bl1.bl_state());
            JPH_ASSERT(1 == bl1.team_id());
            JPH_ASSERT(1 == bl1.active_skill_hit());
            JPH_ASSERT(0 < bl1.vel_y());
        } else if (350 == outerTimerRdfId) {
            JPH_ASSERT(1 == outerTimerRdf->bullet_count());
            const Bullet& bl1 = outerTimerRdf->bullets(0);
            JPH_ASSERT(1 == bl1.id());
            JPH_ASSERT(Active == bl1.bl_state());
            JPH_ASSERT(1 == bl1.team_id());
            JPH_ASSERT(1 == bl1.active_skill_hit());
            JPH_ASSERT(0 > bl1.vel_y()); // Gravity flipped "vel_y"
        } else if (361 == outerTimerRdfId) {
            JPH_ASSERT(1 == outerTimerRdf->bullet_count());
            const Bullet& bl1 = outerTimerRdf->bullets(0);
            JPH_ASSERT(1 == bl1.id());
            JPH_ASSERT(Active == bl1.bl_state());
            JPH_ASSERT(1 == bl1.team_id());
            JPH_ASSERT(1 == bl1.active_skill_hit());
            JPH_ASSERT(0 < bl1.vel_y()); // Bounced up
        } else if (402 == outerTimerRdfId) {
            JPH_ASSERT(1 == outerTimerRdf->bullet_count());
            const Bullet& bl1 = outerTimerRdf->bullets(0);
            JPH_ASSERT(1 == bl1.id());
            JPH_ASSERT(Active == bl1.bl_state());
            JPH_ASSERT(1 == bl1.team_id());
            JPH_ASSERT(1 == bl1.active_skill_hit());
        } else if (730 == outerTimerRdfId) {
            JPH_ASSERT(0 == npc1.cached_cue_cmd());
            JPH_ASSERT(NpcGoal::NIdleIfGoHuntingThenPatrol == npc1.goal_as_npc());
        } else if (800 == outerTimerRdfId) {
            npc1NotEnoughMpX = npc1Chd.x();
            JPH_ASSERT(0 == npc1.cached_cue_cmd());
            JPH_ASSERT(NpcGoal::NIdleIfGoHuntingThenPatrol == npc1.goal_as_npc());
            JPH_ASSERT(0 == npc1Chd.vel_x());
        } else if (960 == outerTimerRdfId) {
            JPH_ASSERT(npc1NotEnoughMpX == npc1Chd.x());
            JPH_ASSERT(0 == npc1.cached_cue_cmd());
            JPH_ASSERT(NpcGoal::NIdleIfGoHuntingThenPatrol == npc1.goal_as_npc());
            JPH_ASSERT(0 == npc1Chd.vel_x());
        } else if (966 == outerTimerRdfId) {
            JPH_ASSERT(4 == npc1.cached_cue_cmd());
            JPH_ASSERT(NpcGoal::NHuntThenPatrol == npc1.goal_as_npc());
            JPH_ASSERT(0 > npc1Chd.vel_x());
        } else if (1038 == outerTimerRdfId) {
            JPH_ASSERT(CharacterState::Atk1 == npc1Chd.ch_state());
            JPH_ASSERT(40 == npc1Chd.mp());
        }

        outerTimerRdfId++;
    }

    std::cout << "Passed TestCase29: BlackThrower1 attack\n" << std::endl;
    theAllocator->Reset();
    reusedBattle->Clear();

    return true;
}

bool runTestCase30(FrontendBattle* reusedBattle, std::vector<std::vector<float>>& hulls, int inSingleJoinIndex, google::protobuf::Arena* theAllocator) {
    WsReq* initializerMapData = google::protobuf::Arena::Create<WsReq>(theAllocator);
    initTest30Data(initializerMapData, hulls, theAllocator);
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);

    int outerTimerRdfId = 754;
    int loopRdfCnt = 1024;
    int printIntervalRdfCnt = (1 << 5);

    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    int timerRdfId = -1, toGenIfdId = -1, localRequiredIfdId = -1; // shared 
    int chaserRdfIdLowerBound = -1, oldLcacIfdId = -1, newLcacIfdId = -1, newUdpLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    int newChaserRdfId = 0;

    jtshared::RenderFrame* outRdf = google::protobuf::Arena::Create<RenderFrame>(theAllocator);
    while (loopRdfCnt > outerTimerRdfId) {
        uint64_t inSingleInput = getSelfCmdByRdfId(testCmds30, outerTimerRdfId);
        bool cmdInjected = FRONTEND_UpsertSelfCmd(reusedBattle, inSingleInput, &newChaserRdfId);
        if (!cmdInjected) {
            std::cerr << "Failed to inject cmd for outerTimerRdfId=" << outerTimerRdfId << ", inSingleInput=" << inSingleInput << std::endl;
            exit(1);
        }

        bool shouldPrint = false;
        int chaserRdfIdEd = outerTimerRdfId;

        FRONTEND_Step(reusedBattle);

        auto outerTimerRdf = reusedBattle->rdfBuffer.GetByFrameId(outerTimerRdfId);
        StepResult* outStepResult = reusedBattle->stepResultBuffer.GetByFrameId(outerTimerRdfId);

        const PlayerCharacterDownsync& p1 = outerTimerRdf->players(0);
        const CharacterDownsync& p1Chd = p1.chd();

        const NpcCharacterDownsync& npc1 = outerTimerRdf->npcs(0);
        const CharacterDownsync& npc1Chd = npc1.chd();

        if (240 <= outerTimerRdfId && outerTimerRdfId < 300) {
            //shouldPrint = true;
        }

        if (shouldPrint) {
            std::cout << "TestCase30/outerTimerRdfId=" << outerTimerRdfId << "\n\tp1Chd hp=" << p1Chd.hp() << ", cs=" << p1Chd.ch_state() << ", fc=" << p1Chd.frames_in_ch_state() << ", gud=" << p1Chd.ground_ud() << ", dir=(" << p1Chd.q_x() << ", " << p1Chd.q_y() << ", " << p1Chd.q_z() << ", " << p1Chd.q_w() << "), pos=(" << p1Chd.x() << ", " << p1Chd.y() << "), vel=(" << p1Chd.vel_x() << ", " << p1Chd.vel_y() << "), ground_vel=(" << p1Chd.ground_vel_x() << ", " << p1Chd.ground_vel_y() << ")\n\tnpc1Chd hp=" << npc1Chd.hp() << ", mp=" << npc1Chd.mp() << ", cs=" << npc1Chd.ch_state() << ", fc=" << npc1Chd.frames_in_ch_state() << ", dir=(" << npc1Chd.q_x() << ", " << npc1Chd.q_y() << ", " << npc1Chd.q_z() << ", " << npc1Chd.q_w() << "), pos=(" << npc1Chd.x() << ", " << npc1Chd.y() << "), vel=(" << npc1Chd.vel_x() << ", " << npc1Chd.vel_y() << "), cuedCmd=" << npc1.cached_cue_cmd() << std::endl;
        }

        if (755 == outerTimerRdfId) {
            JPH_ASSERT(1 == outerTimerRdf->bullet_count());
            const Bullet& bl1 = outerTimerRdf->bullets(0);
            JPH_ASSERT(BulletState::StartUp == bl1.bl_state());
        }

        outerTimerRdfId++;
    }

    std::cout << "Passed TestCase30: Left shifting dead bullets\n" << std::endl;
    theAllocator->Reset();
    reusedBattle->Clear();

    return true;
}

bool runTestCase31(FrontendBattle* reusedBattle, std::vector<std::vector<float>>& hulls, int inSingleJoinIndex, google::protobuf::Arena* theAllocator) {
    WsReq* initializerMapData = google::protobuf::Arena::Create<WsReq>(theAllocator);
    initTest31Data(initializerMapData, hulls, theAllocator);
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);

    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 1024;
    int printIntervalRdfCnt = (1 << 5);

    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    int timerRdfId = -1, toGenIfdId = -1, localRequiredIfdId = -1; // shared 
    int chaserRdfIdLowerBound = -1, oldLcacIfdId = -1, newLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    int newChaserRdfId = 0;
    bool victoryTriggered = false;
    while (loopRdfCnt > outerTimerRdfId) {
        bool shouldPrint = false;
        uint64_t inSingleInput = getSelfCmdByRdfId(testCmds31, outerTimerRdfId);
        bool cmdInjected = FRONTEND_UpsertSelfCmd(reusedBattle, inSingleInput, &newChaserRdfId);
        if (!cmdInjected) {
            std::cerr << "TestCase31/Failed to inject cmd for outerTimerRdfId=" << outerTimerRdfId << ", inSingleInput=" << inSingleInput << std::endl;
            exit(1);
        }
        FRONTEND_Step(reusedBattle);

        RenderFrame* outerTimerRdf = reusedBattle->rdfBuffer.GetByFrameId(outerTimerRdfId);

        auto& tr1 = outerTimerRdf->triggers(0);
        auto& tr2 = outerTimerRdf->triggers(1);
        auto& tr3 = outerTimerRdf->triggers(2);

        auto& p1 = outerTimerRdf->players(0);
        auto& p1Chd = p1.chd();
        auto p1Ud = APP_CalcPlayerUserData(p1.join_index());

        auto* stepResult = reusedBattle->stepResultBuffer.GetByFrameId(outerTimerRdfId);
        int fulfilledTriggersSize = stepResult->fulfilled_triggers_size();
        int npcCount = outerTimerRdf->npc_count();

        if (180 > outerTimerRdfId) {
            // shouldPrint = true;
        }

        if (shouldPrint) {
            std::cout << "TestCase31/outerTimerRdfId=" << outerTimerRdfId << "\n\tp1Chd ud=" << p1Ud << ", hp=" << p1Chd.hp() << ", cs=" << p1Chd.ch_state() << ", fc=" << p1Chd.frames_in_ch_state() << ", q=(" << p1Chd.q_x() << ", " << p1Chd.q_y() << ", " << p1Chd.q_z() << ", " << p1Chd.q_w() << "), pos=(" << p1Chd.x() << ", " << p1Chd.y() << ", " << p1Chd.z() << "), vel=(" << p1Chd.vel_x() << ", " << p1Chd.vel_y() << ")" << ", stepResult fulfilledTriggersSize=" << fulfilledTriggersSize << ", npcCount=" << npcCount << std::endl;
        }

        if (1 == outerTimerRdfId) {
            JPH_ASSERT(2 == tr1.topo_lv());
            JPH_ASSERT(3 == tr1.quota());

            JPH_ASSERT(3 == tr2.topo_lv());
            JPH_ASSERT(1 == tr3.topo_lv());
        } else if (18 == outerTimerRdfId) {
            JPH_ASSERT(1 == fulfilledTriggersSize);
            JPH_ASSERT(42 == stepResult->fulfilled_triggers(0).id());
        } else if (19 == outerTimerRdfId) {
            JPH_ASSERT(1 == fulfilledTriggersSize);
            JPH_ASSERT(43 == stepResult->fulfilled_triggers(0).id());
        } else if (50 == outerTimerRdfId) {
            JPH_ASSERT(1 == npcCount);
        } else if (108 == outerTimerRdfId) {
            JPH_ASSERT(2 == npcCount);
        } else if (249 == outerTimerRdfId) {
            JPH_ASSERT(0 == npcCount);
            if (TriggerState::TrReady == tr2.state() || TriggerState::TrExhaustedYetListening == tr2.state()) {
                JPH_ASSERT(1 == tr2.main_cycle_mask_to_fulfill());
            } else {
                JPH_ASSERT(0 == tr2.main_cycle_mask_to_fulfill());
            }
        } else if (255 == outerTimerRdfId) {
            JPH_ASSERT(1 == fulfilledTriggersSize);
            JPH_ASSERT(42 == stepResult->fulfilled_triggers(0).id());
            JPH_ASSERT(1 == tr1.quota());
        } else if (331 == outerTimerRdfId) {
            JPH_ASSERT(3 == npcCount);
        } else if (353 == outerTimerRdfId) {
            JPH_ASSERT(4 == npcCount);
            auto& npc1 = outerTimerRdf->npcs(0);
            auto& npc1Chd = npc1.chd();
            JPH_ASSERT(0 >= npc1Chd.hp());

            auto& npc2 = outerTimerRdf->npcs(1);
            auto& npc2Chd = npc2.chd();
            JPH_ASSERT(0 < npc2Chd.hp());

            auto& npc3 = outerTimerRdf->npcs(2);
            auto& npc3Chd = npc3.chd();
            JPH_ASSERT(0 < npc3Chd.hp());
            
            auto& npc4 = outerTimerRdf->npcs(3);
            auto& npc4Chd = npc4.chd();
            JPH_ASSERT(0 < npc4Chd.hp());
            
            JPH_ASSERT(1 == tr2.quota());
        } else if (750 == outerTimerRdfId) {
            JPH_ASSERT(0 >= tr2.quota());
        }
        
        outerTimerRdfId++;
    }

    std::cout << "Passed TestCase31: IndiWave NPC spawner multiple waves\n" << std::endl;
    theAllocator->Reset();
    reusedBattle->Clear();

    return true;
}

bool runTestCase32(FrontendBattle* reusedBattle, std::vector<std::vector<float>>& hulls, int inSingleJoinIndex, google::protobuf::Arena* theAllocator) {
    WsReq* initializerMapData = google::protobuf::Arena::Create<WsReq>(theAllocator);
    initTest32Data(initializerMapData, hulls, theAllocator);
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);

    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 1024;
    int printIntervalRdfCnt = (1 << 5);

    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    int timerRdfId = -1, toGenIfdId = -1, localRequiredIfdId = -1; // shared 
    int chaserRdfIdLowerBound = -1, oldLcacIfdId = -1, newLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    int newChaserRdfId = 0;
    bool victoryTriggered = false;
    while (loopRdfCnt > outerTimerRdfId) {
        bool shouldPrint = false;
        uint64_t inSingleInput = getSelfCmdByRdfId(testCmds32, outerTimerRdfId);
        bool cmdInjected = FRONTEND_UpsertSelfCmd(reusedBattle, inSingleInput, &newChaserRdfId);
        if (!cmdInjected) {
            std::cerr << "TestCase32/Failed to inject cmd for outerTimerRdfId=" << outerTimerRdfId << ", inSingleInput=" << inSingleInput << std::endl;
            exit(1);
        }
        FRONTEND_Step(reusedBattle);

        RenderFrame* outerTimerRdf = reusedBattle->rdfBuffer.GetByFrameId(outerTimerRdfId);

        auto& tp1 = outerTimerRdf->dynamic_traps(0);

        auto& p1 = outerTimerRdf->players(0);
        auto& p1Chd = p1.chd();
        auto p1Ud = APP_CalcPlayerUserData(p1.join_index());

        auto* stepResult = reusedBattle->stepResultBuffer.GetByFrameId(outerTimerRdfId);

        if (8 < outerTimerRdfId && 240 > outerTimerRdfId) {
            //shouldPrint = true;
        }

        if (shouldPrint) {
            JPH::Quat tp1Q(tp1.q_x(), tp1.q_y(), tp1.q_z(), tp1.q_w());
            Vec3 tp1QAxis;
            float tp1QAngle;
            tp1Q.GetAxisAngle(tp1QAxis, tp1QAngle);
            float tp1QAngleDegrees = tp1QAngle * 180.0f / JPH_PI;
            std::cout << "TestCase32/outerTimerRdfId=" << outerTimerRdfId << "\n\tp1Chd ud=" << p1Ud << ", hp=" << p1Chd.hp() << ", cs=" << p1Chd.ch_state() << ", fc=" << p1Chd.frames_in_ch_state() << ", q=(" << p1Chd.q_x() << ", " << p1Chd.q_y() << ", " << p1Chd.q_z() << ", " << p1Chd.q_w() << "), pos=(" << p1Chd.x() << ", " << p1Chd.y() << ", " << p1Chd.z() << "), vel=(" << p1Chd.vel_x() << ", " << p1Chd.vel_y() << ")" << ", tpQ1Axis=(" << tp1QAxis.GetX() << "," << tp1QAxis.GetY() << "," << tp1QAxis.GetZ() << "), tp1QAngleDegrees=" << tp1QAngleDegrees << std::endl;
        }

        if (10 <= outerTimerRdfId && outerTimerRdfId <= 60) {
            JPH_ASSERT(CharacterState::CrouchIdle1 == p1Chd.ch_state());
        } else if (70 <= outerTimerRdfId) {
            JPH_ASSERT(CharacterState::CrouchIdle1 != p1Chd.ch_state());
        }
        
        outerTimerRdfId++;
    }

    std::cout << "Passed TestCase32: Rotating platform force crouching negative z-ang-vel\n" << std::endl;
    theAllocator->Reset();
    reusedBattle->Clear();

    return true;
}

bool runTestCase33(FrontendBattle* reusedBattle, std::vector<std::vector<float>>& hulls, int inSingleJoinIndex, google::protobuf::Arena* theAllocator) {
    WsReq* initializerMapData = google::protobuf::Arena::Create<WsReq>(theAllocator);
    initTest33Data(initializerMapData, hulls, theAllocator);
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);

    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 1024;
    int printIntervalRdfCnt = (1 << 5);

    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    int timerRdfId = -1, toGenIfdId = -1, localRequiredIfdId = -1; // shared 
    int chaserRdfIdLowerBound = -1, oldLcacIfdId = -1, newLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    int newChaserRdfId = 0;
    bool victoryTriggered = false;
    while (loopRdfCnt > outerTimerRdfId) {
        bool shouldPrint = false;
        uint64_t inSingleInput = getSelfCmdByRdfId(testCmds33, outerTimerRdfId);
        bool cmdInjected = FRONTEND_UpsertSelfCmd(reusedBattle, inSingleInput, &newChaserRdfId);
        if (!cmdInjected) {
            std::cerr << "TestCase33/Failed to inject cmd for outerTimerRdfId=" << outerTimerRdfId << ", inSingleInput=" << inSingleInput << std::endl;
            exit(1);
        }
        FRONTEND_Step(reusedBattle);

        RenderFrame* outerTimerRdf = reusedBattle->rdfBuffer.GetByFrameId(outerTimerRdfId);

        auto& tp1 = outerTimerRdf->dynamic_traps(0);

        auto& p1 = outerTimerRdf->players(0);
        auto& p1Chd = p1.chd();
        auto p1Ud = APP_CalcPlayerUserData(p1.join_index());

        auto* stepResult = reusedBattle->stepResultBuffer.GetByFrameId(outerTimerRdfId);

        if (400 < outerTimerRdfId && 800 >= outerTimerRdfId) {
            //shouldPrint = true;
        }

        if (shouldPrint) {
            JPH::Quat tp1Q(tp1.q_x(), tp1.q_y(), tp1.q_z(), tp1.q_w());
            Vec3 tp1QAxis;
            float tp1QAngle;
            tp1Q.GetAxisAngle(tp1QAxis, tp1QAngle);
            float tp1QAngleDegrees = tp1QAngle * 180.0f / JPH_PI;
            std::cout << "TestCase33/outerTimerRdfId=" << outerTimerRdfId << "\n\tp1Chd ud=" << p1Ud << ", hp=" << p1Chd.hp() << ", cs=" << p1Chd.ch_state() << ", fc=" << p1Chd.frames_in_ch_state() << ", q=(" << p1Chd.q_x() << ", " << p1Chd.q_y() << ", " << p1Chd.q_z() << ", " << p1Chd.q_w() << "), pos=(" << p1Chd.x() << ", " << p1Chd.y() << ", " << p1Chd.z() << "), vel=(" << p1Chd.vel_x() << ", " << p1Chd.vel_y() << ")" << ", tpQ1Axis=(" << tp1QAxis.GetX() << "," << tp1QAxis.GetY() << "," << tp1QAxis.GetZ() << "), tp1QAngleDegrees=" << tp1QAngleDegrees << std::endl;
        }

        JPH_ASSERT(CharacterState::CrouchIdle1 != p1Chd.ch_state());
        if (800 <= outerTimerRdfId) {
            JPH_ASSERT(92.f <= p1Chd.x());
        }

        outerTimerRdfId++;
    }

    std::cout << "Passed TestCase33: Rotating platform force crouching positive z-ang-vel\n" << std::endl;
    theAllocator->Reset();
    reusedBattle->Clear();

    return true;
}

bool runTestCase34(FrontendBattle* reusedBattle, std::vector<std::vector<float>>& hulls, int inSingleJoinIndex, google::protobuf::Arena* theAllocator) {
    WsReq* initializerMapData = google::protobuf::Arena::Create<WsReq>(theAllocator);
    initTest34Data(initializerMapData, hulls, theAllocator);
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);

    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 1024;
    int printIntervalRdfCnt = (1 << 5);

    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    int timerRdfId = -1, toGenIfdId = -1, localRequiredIfdId = -1; // shared 
    int chaserRdfIdLowerBound = -1, oldLcacIfdId = -1, newLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    int newChaserRdfId = 0;
    bool victoryTriggered = false;
    while (loopRdfCnt > outerTimerRdfId) {
        bool shouldPrint = false;
        uint64_t inSingleInput = getSelfCmdByRdfId(testCmds34, outerTimerRdfId);
        bool cmdInjected = FRONTEND_UpsertSelfCmd(reusedBattle, inSingleInput, &newChaserRdfId);
        if (!cmdInjected) {
            std::cerr << "TestCase34/Failed to inject cmd for outerTimerRdfId=" << outerTimerRdfId << ", inSingleInput=" << inSingleInput << std::endl;
            exit(1);
        }
        FRONTEND_Step(reusedBattle);

        RenderFrame* outerTimerRdf = reusedBattle->rdfBuffer.GetByFrameId(outerTimerRdfId);

        auto& tr1 = outerTimerRdf->triggers(0);
        auto tr1Ud = APP_CalcTriggerUserData(tr1.id());

        auto& tr2 = outerTimerRdf->triggers(1);
        auto tr2Ud = APP_CalcTriggerUserData(tr2.id());

        auto& p1 = outerTimerRdf->players(0);
        auto& p1Chd = p1.chd();
        auto p1Ud = APP_CalcPlayerUserData(p1.join_index());

        auto* stepResult = reusedBattle->stepResultBuffer.GetByFrameId(outerTimerRdfId);

        int pickableCnt = outerTimerRdf->pickable_count();

        if (130 < outerTimerRdfId && 222 >= outerTimerRdfId) {
            //shouldPrint = true;
        }

        if (420 < outerTimerRdfId && 600 >= outerTimerRdfId) {
            //shouldPrint = true;
        }

        if (shouldPrint) {
            std::cout << "TestCase34/outerTimerRdfId=" << outerTimerRdfId << "\n\tp1Chd ud=" << p1Ud << ", hp=" << p1Chd.hp() << ", cs=" << p1Chd.ch_state() << ", fc=" << p1Chd.frames_in_ch_state() << ", q=(" << p1Chd.q_x() << ", " << p1Chd.q_y() << ", " << p1Chd.q_z() << ", " << p1Chd.q_w() << "), pos=(" << p1Chd.x() << ", " << p1Chd.y() << ", " << p1Chd.z() << "), vel=(" << p1Chd.vel_x() << ", " << p1Chd.vel_y() << "), pickableCnt=" << pickableCnt << std::endl;
        }

        if (50 == outerTimerRdfId) {
            JPH_ASSERT(1 == stepResult->prepared_trigger_uds_size());
            JPH_ASSERT(1 == tr1.main_cycle_mask_to_fulfill());
            JPH_ASSERT(2 == tr1.quota());
        } else if (132 == outerTimerRdfId) {
            JPH_ASSERT(TriggerState::TrCoolingDown == tr1.state());
            JPH_ASSERT(0 == stepResult->prepared_trigger_uds_size());
            JPH_ASSERT(1 == stepResult->fulfilled_trigger_ids_size());
            JPH_ASSERT(0 == tr1.main_cycle_mask_to_fulfill());
            JPH_ASSERT(1 == tr1.quota());
        } else if (190 == outerTimerRdfId) {
            JPH_ASSERT(2 == pickableCnt);
            JPH_ASSERT(0 == tr1.main_cycle_mask_to_fulfill());
            JPH_ASSERT(1 == tr1.quota());
            JPH_ASSERT(TriggerState::TrCoolingDown == tr1.state());
        } else if (313 == outerTimerRdfId) {
            JPH_ASSERT(0 == tr1.main_cycle_mask_to_fulfill());
            JPH_ASSERT(1 == tr1.quota());
            JPH_ASSERT(TriggerState::TrCooledDown == tr1.state());
            JPH_ASSERT(1 == pickableCnt); // the first one picked up, the second one remains
        } else if (320 == outerTimerRdfId) {
            JPH_ASSERT(1 == tr1.main_cycle_mask_to_fulfill());
            JPH_ASSERT(1 == tr1.quota());
            JPH_ASSERT(TriggerState::TrReady == tr1.state());
            JPH_ASSERT(1 == stepResult->prepared_trigger_uds_size());
        } else if (360 == outerTimerRdfId) {
            JPH_ASSERT(0 == stepResult->prepared_trigger_uds_size());
            JPH_ASSERT(0 == tr1.main_cycle_mask_to_fulfill());
            JPH_ASSERT(0 == tr1.quota());
            JPH_ASSERT(TriggerState::TrExhausted == tr1.state());
            JPH_ASSERT(1 == pickableCnt); // the second one remains
            auto& pk1 = outerTimerRdf->pickables(0);
            JPH_ASSERT(PickableState::PIdle == pk1.pk_state());
        } else if (390 == outerTimerRdfId) {
            JPH_ASSERT(TriggerState::TrExhausted == tr1.state());
            JPH_ASSERT(3 == pickableCnt);
            auto& pk1 = outerTimerRdf->pickables(0);
            JPH_ASSERT(PickableState::PIdle == pk1.pk_state());
            auto& pk2 = outerTimerRdf->pickables(1);
            JPH_ASSERT(PickableState::PIdle == pk2.pk_state());
        } else if (420 == outerTimerRdfId) {
            JPH_ASSERT(TriggerState::TrExhausted == tr1.state());
            JPH_ASSERT(3 == pickableCnt); // All spawned by "triggerId=43" picked up by far
        } else if (421 == outerTimerRdfId) {
            JPH_ASSERT(TriggerState::TrExhausted == tr1.state());
            JPH_ASSERT(2 == pickableCnt); // 1 left shifted
        } else if (437 == outerTimerRdfId) {
            JPH_ASSERT(TriggerState::TrExhausted == tr1.state());
            JPH_ASSERT(3 == pickableCnt); // 1 dropped from exhausted NPC
        } else if (443 == outerTimerRdfId) {
            JPH_ASSERT(TriggerState::TrExhausted == tr1.state());
            JPH_ASSERT(TriggerState::TrExhaustedYetListening == tr2.state());
            JPH_ASSERT(2 == pickableCnt); // 1 left shifted
            auto& pk1 = outerTimerRdf->pickables(0);
            JPH_ASSERT(PickableState::PIdle == pk1.pk_state());
            auto& pk2 = outerTimerRdf->pickables(1);
            JPH_ASSERT(PickableState::PIdle == pk2.pk_state());
        }

        outerTimerRdfId++;
    }

    std::cout << "Passed TestCase34: Pattern-f trigger\n" << std::endl;
    theAllocator->Reset();
    reusedBattle->Clear();

    return true;
}

bool runTestCase35(std::vector<std::vector<float>>& hulls, int inSingleJoinIndex, google::protobuf::Arena* theAllocator) {
    FrontendBattle* offlineBattle = static_cast<FrontendBattle*>(FRONTEND_CreateBattle(512, false));
    std::cout << "TestCase35/Created Offline battle = " << offlineBattle << std::endl;

    WsReq* initializerMapData = google::protobuf::Arena::Create<WsReq>(theAllocator);
    initTest35Data(initializerMapData, hulls, theAllocator);
    offlineBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);
    int jumpedTimerRdfId = INT_MAX - 16;

    {
        offlineBattle->timerRdfId = jumpedTimerRdfId; 
        int toUseNewIfdId = BaseBattle::ConvertToDelayedInputFrameId(jumpedTimerRdfId);
        offlineBattle->ifdBuffer.StFrameId = toUseNewIfdId;
        offlineBattle->ifdBuffer.EdFrameId = toUseNewIfdId;
        offlineBattle->ifdBuffer.Cnt = 0;
        offlineBattle->ifdBuffer.St = 0;
        offlineBattle->ifdBuffer.Ed = 0;

        RenderFrame* toReuseRdf = offlineBattle->rdfBuffer.GetFirst();
        toReuseRdf->set_id(jumpedTimerRdfId);
        offlineBattle->rdfBuffer.StFrameId = jumpedTimerRdfId;
        offlineBattle->rdfBuffer.EdFrameId = jumpedTimerRdfId+1;
        offlineBattle->rdfBuffer.Cnt = 1;
        offlineBattle->rdfBuffer.St = 0;
        offlineBattle->rdfBuffer.Ed = 1;
    }

    int outerTimerRdfId = jumpedTimerRdfId;
    int loopRdfCnt = 1024;
    int printIntervalRdfCnt = (1 << 5);

    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    int timerRdfId = -1, toGenIfdId = -1, localRequiredIfdId = -1; // shared 
    int chaserRdfIdLowerBound = -1, oldLcacIfdId = -1, newLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    int newChaserRdfId = 0;
    bool victoryTriggered = false;
    while (jumpedTimerRdfId <= outerTimerRdfId || loopRdfCnt > outerTimerRdfId) {
        bool shouldPrint = false;
        uint64_t inSingleInput = getSelfCmdByRdfId(testCmds35, outerTimerRdfId);
        bool cmdInjected = FRONTEND_UpsertSelfCmd(offlineBattle, inSingleInput, &newChaserRdfId);
        if (!cmdInjected) {
            std::cerr << "TestCase35/Failed to inject cmd for outerTimerRdfId=" << outerTimerRdfId << ", inSingleInput=" << inSingleInput << std::endl;
            exit(1);
        }
        FRONTEND_Step(offlineBattle);

        RenderFrame* outerTimerRdf = offlineBattle->rdfBuffer.GetLast();

        auto& tr1 = outerTimerRdf->triggers(0);
        auto tr1Ud = APP_CalcTriggerUserData(tr1.id());

        auto& p1 = outerTimerRdf->players(0);
        auto& p1Chd = p1.chd();
        auto p1Ud = APP_CalcPlayerUserData(p1.join_index());

        auto* stepResult = offlineBattle->stepResultBuffer.GetLast();

        int pickableCnt = outerTimerRdf->pickable_count();

        if (shouldPrint) {
            std::cout << "TestCase35/outerTimerRdfId=" << outerTimerRdfId << "\n\tp1Chd ud=" << p1Ud << ", hp=" << p1Chd.hp() << ", cs=" << p1Chd.ch_state() << ", fc=" << p1Chd.frames_in_ch_state() << ", q=(" << p1Chd.q_x() << ", " << p1Chd.q_y() << ", " << p1Chd.q_z() << ", " << p1Chd.q_w() << "), pos=(" << p1Chd.x() << ", " << p1Chd.y() << ", " << p1Chd.z() << "), vel=(" << p1Chd.vel_x() << ", " << p1Chd.vel_y() << "), pickableCnt=" << pickableCnt << std::endl;
        }

        outerTimerRdfId = offlineBattle->timerRdfId;
    }

    JPH_ASSERT(outerTimerRdfId == loopRdfCnt);

    theAllocator->Reset();
    APP_DestroyBattle(offlineBattle);
    std::cout << "Passed TestCase35: TimerRdfId recycling\n" << std::endl;

    return true;
}

bool runTestCase36(FrontendBattle* reusedBattle, std::vector<std::vector<float>>& hulls, int inSingleJoinIndex, google::protobuf::Arena* theAllocator) {
    WsReq* initializerMapData = google::protobuf::Arena::Create<WsReq>(theAllocator);
    initTest36Data(initializerMapData, hulls, theAllocator);
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);

    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 1024;
    int printIntervalRdfCnt = (1 << 5);

    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    int timerRdfId = -1, toGenIfdId = -1, localRequiredIfdId = -1; // shared 
    int chaserRdfIdLowerBound = -1, oldLcacIfdId = -1, newLcacIfdId = -1, newUdpLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    int newChaserRdfId = 0;
    
    jtshared::RenderFrame* outRdf = google::protobuf::Arena::Create<RenderFrame>(theAllocator);
    while (loopRdfCnt > outerTimerRdfId) {
        uint64_t inSingleInput = getSelfCmdByRdfId(testCmds36, outerTimerRdfId);
        bool cmdInjected = FRONTEND_UpsertSelfCmd(reusedBattle, inSingleInput, &newChaserRdfId);
        if (!cmdInjected) {
            std::cerr << "Failed to inject cmd for outerTimerRdfId=" << outerTimerRdfId << ", inSingleInput=" << inSingleInput << std::endl;
            exit(1);
        }

        bool shouldPrint = false;
        int chaserRdfIdEd = outerTimerRdfId;

        FRONTEND_Step(reusedBattle);

        auto outerTimerRdf = reusedBattle->rdfBuffer.GetByFrameId(outerTimerRdfId);
        StepResult* outStepResult = reusedBattle->stepResultBuffer.GetByFrameId(outerTimerRdfId);

        const PlayerCharacterDownsync& p1 = outerTimerRdf->players(0);
        const CharacterDownsync& p1Chd = p1.chd();

        const Trap& trap1 = outerTimerRdf->dynamic_traps(0);
        const uint64_t trap1Ud = APP_CalcTrapUserData(trap1.id());

        const Trigger& trigger1 = outerTimerRdf->triggers(0);
        const uint64_t trigger1Ud = APP_CalcTrapUserData(trigger1.id());

        if (446 <= outerTimerRdfId && outerTimerRdfId < 550) {
            //shouldPrint = true;
        }

        if (shouldPrint) {
            std::cout << "TestCase36/outerTimerRdfId=" << outerTimerRdfId << "\n\tp1Chd cs=" << p1Chd.ch_state() << ", fc=" << p1Chd.frames_in_ch_state() << ", gud=" << p1Chd.ground_ud() << ", dir=(" << p1Chd.q_x() << ", " << p1Chd.q_y() << ", " << p1Chd.q_z() << ", " << p1Chd.q_w() << "), pos=(" << p1Chd.x() << ", " << p1Chd.y() << "), vel=(" << p1Chd.vel_x() << ", " << p1Chd.vel_y() << "), ground_vel=(" << p1Chd.ground_vel_x() << ", " << p1Chd.ground_vel_y() << ")\n\ttrap1 ud=" << trap1Ud << ", pos=(" << trap1.x() << ", " << trap1.y() << "), vel=(" << trap1.vel_x() << ", " << trap1.vel_y() << "), dir=(" << trap1.q_x() << ", " << trap1.q_y() << ", " << trap1.q_z() << ", " << trap1.q_w() << "), ang_vel=(" << trap1.ang_vel_x() << ", " << trap1.ang_vel_y() << ", " << trap1.ang_vel_z() << ")" << std::endl;
        }

        if (62 == outerTimerRdfId) {
            JPH_ASSERT(TrapState::TpIdle == trap1.trap_state());
            JPH_ASSERT(12 == trigger1.quota());
        } else if (150 == outerTimerRdfId) {
            JPH_ASSERT(TrapState::TpIdle == trap1.trap_state());
            JPH_ASSERT(12 == trigger1.quota());
        } else if (220 == outerTimerRdfId) {
            JPH_ASSERT(TrapState::TpWalking == trap1.trap_state());
            JPH_ASSERT(11 == trigger1.quota());
        } else if (320 == outerTimerRdfId) {
            JPH_ASSERT(TrapState::TpIdle == trap1.trap_state());
            JPH_ASSERT(11 == trigger1.quota());
        } else if (1023 == outerTimerRdfId) {
            JPH_ASSERT(TrapState::TpIdle == trap1.trap_state());
            JPH_ASSERT(10 == trigger1.quota());
        }
        
        outerTimerRdfId++;
    }

    std::cout << "Passed TestCase36: Atk trigger and rotating platform interaction\n" << std::endl;
    theAllocator->Reset();
    reusedBattle->Clear();

    return true;
}

bool runTestCase37(FrontendBattle* reusedBattle, std::vector<std::vector<float>>& hulls, int inSingleJoinIndex, google::protobuf::Arena* theAllocator) {
    WsReq* initializerMapData = google::protobuf::Arena::Create<WsReq>(theAllocator);
    initTest37Data(initializerMapData, hulls, theAllocator);
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);

    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 1024;
    int printIntervalRdfCnt = (1 << 5);

    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    int timerRdfId = -1, toGenIfdId = -1, localRequiredIfdId = -1; // shared 
    int chaserRdfIdLowerBound = -1, oldLcacIfdId = -1, newLcacIfdId = -1, newUdpLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    int newChaserRdfId = 0;

    auto characterConfigs = globalConfigConsts->character_configs();
    while (loopRdfCnt > outerTimerRdfId) {
        bool shouldPrint = false;
        if (incomingUpsyncSnapshotReqs37Intime.count(outerTimerRdfId)) {
            auto req = incomingUpsyncSnapshotReqs37Intime[outerTimerRdfId];
            auto peerUpsyncSnapshot = req->upsync_snapshot();
            reusedBattle->OnUpsyncSnapshotReceived(req->join_index(), peerUpsyncSnapshot, &newChaserRdfId, &newUdpLcacIfdId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
            //shouldPrint = true;
        }

        uint64_t inSingleInput = getSelfCmdByRdfId(testCmds37, outerTimerRdfId);
        bool cmdInjected = FRONTEND_UpsertSelfCmd(reusedBattle, inSingleInput, &newChaserRdfId);
        if (!cmdInjected) {
            std::cerr << "TestCase37/Failed to inject cmd for outerTimerRdfId=" << outerTimerRdfId << ", inSingleInput=" << inSingleInput << std::endl;
            exit(1);
        }
        FRONTEND_ChaseRolledBackRdfs(reusedBattle, &newChaserRdfId, true);
        FRONTEND_Step(reusedBattle);

        RenderFrame* outerTimerRdf = reusedBattle->rdfBuffer.GetByFrameId(outerTimerRdfId);
        auto& p1 = outerTimerRdf->players(0);
        auto& p1Chd = p1.chd();
        auto& p1Cc = characterConfigs.at(p1Chd.species_id());
        auto& p2 = outerTimerRdf->players(1);
        auto& p2Chd = p2.chd();
        
        if (326 <= outerTimerRdfId && outerTimerRdfId <= 328) {    
            //shouldPrint = true;
        }

        if (shouldPrint) {
            std::cout << "TestCase37/outerTimerRdfId=" << outerTimerRdfId << "\n\tp1Chd hp=" << p1Chd.hp() << ", cs=" << p1Chd.ch_state() << ", fc=" << p1Chd.frames_in_ch_state() << ", q=(" << p1Chd.q_x() << ", " << p1Chd.q_y() << ", " << p1Chd.q_z() << ", " << p1Chd.q_w() << "), pos=(" << p1Chd.x() << ", " << p1Chd.y() << ", " << p1Chd.z() << "), vel=(" << p1Chd.vel_x() << ", " << p1Chd.vel_y() << ", " << p1Chd.vel_z() << ")\n\tp2Chd hp=" << p2Chd.hp() << ", cs=" << p2Chd.ch_state() << ", fc=" << p2Chd.frames_in_ch_state() << ", q=(" << p2Chd.q_x() << ", " << p2Chd.q_y() << ", " << p2Chd.q_z() << ", " << p2Chd.q_w() << "), pos=(" << p2Chd.x() << ", " << p2Chd.y() << ", " << p2Chd.z() << "), vel=(" << p2Chd.vel_x() << ", " << p2Chd.vel_y() << ", " << p2Chd.vel_z() << ")" << std::endl;
        }

        if (70 <= outerTimerRdfId && outerTimerRdfId <= 101) {
            JPH_ASSERT(CharacterState::Def1 == p1Chd.ch_state());
            JPH_ASSERT(0 >= p1Chd.frames_to_recover());
            JPH_ASSERT(p1Cc.default_def1_quota() == p1Chd.remaining_def1_quota());
            JPH_ASSERT(p1Cc.hp() == p1Chd.hp());
            JPH_ASSERT(0 >= p1Chd.damaged_hint_rdf_countdown());
        } else if (105 == outerTimerRdfId) {
            // i.e. "Def1Atked1" for animation
            JPH_ASSERT(CharacterState::Def1 == p1Chd.ch_state());
            JPH_ASSERT(0 < p1Chd.frames_to_recover());
            JPH_ASSERT(p1Cc.default_def1_quota() == p1Chd.remaining_def1_quota()+1);
            JPH_ASSERT(p1Cc.hp() > p1Chd.hp());
            JPH_ASSERT(0 < p1Chd.damaged_hint_rdf_countdown());
        } else if (220 == outerTimerRdfId) {
            // i.e. "Def1Broken"
            JPH_ASSERT(CharacterState::Def1Broken == p1Chd.ch_state());
            JPH_ASSERT(50 == p1Chd.frames_to_recover());
            JPH_ASSERT(0 == p1Chd.remaining_def1_quota());
            JPH_ASSERT(p1Cc.hp() > p1Chd.hp());
            JPH_ASSERT(0 < p1Chd.damaged_hint_rdf_countdown());
        } else if (265 == outerTimerRdfId) {
            // i.e. "Def1Broken" to be extended 
            JPH_ASSERT(CharacterState::Def1Broken == p1Chd.ch_state());
            JPH_ASSERT(5 == p1Chd.frames_to_recover());
            JPH_ASSERT(0 == p1Chd.remaining_def1_quota());
            JPH_ASSERT(p1Cc.hp() > p1Chd.hp());
            JPH_ASSERT(0 < p1Chd.damaged_hint_rdf_countdown());
        } else if (266 == outerTimerRdfId) {
            // i.e. "Def1Broken" extended due to being hit
            JPH_ASSERT(CharacterState::Def1Broken == p1Chd.ch_state());
            JPH_ASSERT(8 == p1Chd.frames_to_recover());
            JPH_ASSERT(0 == p1Chd.remaining_def1_quota());
            JPH_ASSERT(p1Cc.hp() > p1Chd.hp());
            JPH_ASSERT(0 < p1Chd.damaged_hint_rdf_countdown());
        } else if (280 == outerTimerRdfId) {
            // i.e. "Def1Broken" ended
            JPH_ASSERT(CharacterState::Walking == p1Chd.ch_state());
            JPH_ASSERT(0 == p1Chd.frames_to_recover());
            JPH_ASSERT(0 == p1Chd.remaining_def1_quota());
            JPH_ASSERT(p1Cc.hp() > p1Chd.hp());
        } else if (291 == outerTimerRdfId) {
            // i.e. "Def1" started again
            JPH_ASSERT(CharacterState::Def1 == p1Chd.ch_state());
            JPH_ASSERT(0 == p1Chd.frames_to_recover());
            JPH_ASSERT(p1Cc.default_def1_quota() == p1Chd.remaining_def1_quota());
            JPH_ASSERT(p1Cc.hp() > p1Chd.hp());
        } else if (320 == outerTimerRdfId) {
            // In "Def1" but hit from behind
            JPH_ASSERT(CharacterState::Atked1 == p1Chd.ch_state());
            JPH_ASSERT(0 < p1Chd.frames_to_recover());
            JPH_ASSERT(0 == p1Chd.remaining_def1_quota());
            JPH_ASSERT(p1Cc.hp() > p1Chd.hp());
        } else if (326 == outerTimerRdfId) {
            // Last rdf of "Atked1"
            JPH_ASSERT(CharacterState::Atked1 == p1Chd.ch_state());
            JPH_ASSERT(0 == p1Chd.frames_to_recover());
            JPH_ASSERT(0 == p1Chd.remaining_def1_quota());
            JPH_ASSERT(p1Cc.hp() > p1Chd.hp());
        } else if (327 == outerTimerRdfId) {
            // Smooth transition back to "Def1"
            JPH_ASSERT(CharacterState::Def1 == p1Chd.ch_state());
            JPH_ASSERT(0 == p1Chd.frames_to_recover());
            JPH_ASSERT(p1Cc.default_def1_quota() == p1Chd.remaining_def1_quota());
            JPH_ASSERT(p1Cc.hp() > p1Chd.hp());
        } else if (500 == outerTimerRdfId) {
            // i.e. "Def1" down again
            JPH_ASSERT(CharacterState::Idle1 == p1Chd.ch_state());
            JPH_ASSERT(0 == p1Chd.frames_to_recover());
            JPH_ASSERT(0 == p1Chd.remaining_def1_quota());
            JPH_ASSERT(p1Cc.hp() > p1Chd.hp());
        }
        
        outerTimerRdfId++;
    }
    
    std::cout << "Passed TestCase37: Def1 till broken, broken extended and bullet from behind\n" << std::endl;
    theAllocator->Reset();
    reusedBattle->Clear();   
    return true;
}

bool runTestCase38(FrontendBattle* reusedBattle, std::vector<std::vector<float>>& hulls, int inSingleJoinIndex, google::protobuf::Arena* theAllocator) {
    WsReq* initializerMapData = google::protobuf::Arena::Create<WsReq>(theAllocator);
    initTest38Data(initializerMapData, hulls, theAllocator);
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);

    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 1024;
    int printIntervalRdfCnt = (1 << 5);

    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    int timerRdfId = -1, toGenIfdId = -1, localRequiredIfdId = -1; // shared 
    int chaserRdfIdLowerBound = -1, oldLcacIfdId = -1, newLcacIfdId = -1, newUdpLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    int newChaserRdfId = 0;
    
    jtshared::RenderFrame* outRdf = google::protobuf::Arena::Create<RenderFrame>(theAllocator);
    while (loopRdfCnt > outerTimerRdfId) {
        uint64_t inSingleInput = getSelfCmdByRdfId(testCmds38, outerTimerRdfId);
        bool cmdInjected = FRONTEND_UpsertSelfCmd(reusedBattle, inSingleInput, &newChaserRdfId);
        if (!cmdInjected) {
            std::cerr << "Failed to inject cmd for outerTimerRdfId=" << outerTimerRdfId << ", inSingleInput=" << inSingleInput << std::endl;
            exit(1);
        }

        bool shouldPrint = false;
        int chaserRdfIdEd = outerTimerRdfId;

        FRONTEND_Step(reusedBattle);

        auto outerTimerRdf = reusedBattle->rdfBuffer.GetByFrameId(outerTimerRdfId);
        StepResult* outStepResult = reusedBattle->stepResultBuffer.GetByFrameId(outerTimerRdfId);

        const PlayerCharacterDownsync& p1 = outerTimerRdf->players(0);
        const CharacterDownsync& p1Chd = p1.chd();
        const uint64 p1Ud = APP_CalcPlayerUserData(p1.join_index());

        const NpcCharacterDownsync& npc1 = outerTimerRdf->npcs(0);
        const CharacterDownsync& npc1Chd = npc1.chd();

        const NpcCharacterDownsync& npc2 = outerTimerRdf->npcs(1);
        const CharacterDownsync& npc2Chd = npc2.chd();

        if (0 <= outerTimerRdfId && outerTimerRdfId < 45) {
            //shouldPrint = true;
        }

        if (70 <= outerTimerRdfId && outerTimerRdfId < 160) {
            //shouldPrint = true;
        }

        if (280 <= outerTimerRdfId && outerTimerRdfId < 1024) {
            //shouldPrint = true;
        }

        if (shouldPrint) {
            //std::cout << "TestCase38/outerTimerRdfId=" << outerTimerRdfId << "\n\tp1Chd hp=" << p1Chd.hp() << ", cs=" << p1Chd.ch_state() << ", fc=" << p1Chd.frames_in_ch_state() << ", gud=" << p1Chd.ground_ud() << ", dir=(" << p1Chd.q_x() << ", " << p1Chd.q_y() << ", " << p1Chd.q_z() << ", " << p1Chd.q_w() << "), pos=(" << p1Chd.x() << ", " << p1Chd.y() << "), vel=(" << p1Chd.vel_x() << ", " << p1Chd.vel_y() << "), ground_vel=(" << p1Chd.ground_vel_x() << ", " << p1Chd.ground_vel_y() << ")\n\tnpc1Chd hp=" << npc1Chd.hp() << ", mp=" << npc1Chd.mp() << ", cs=" << npc1Chd.ch_state() << ", fc=" << npc1Chd.frames_in_ch_state() << ", fr=" << npc1Chd.frames_to_recover() << ", dir=(" << npc1Chd.q_x() << ", " << npc1Chd.q_y() << ", " << npc1Chd.q_z() << ", " << npc1Chd.q_w() << "), pos=(" << npc1Chd.x() << ", " << npc1Chd.y() << "), vel=(" << npc1Chd.vel_x() << ", " << npc1Chd.vel_y() << "), cuedCmd=" << npc1.cached_cue_cmd() << ", locking_on_ud=" << npc1Chd.locking_on_ud() << std::endl;

            //std::cout << "TestCase38/outerTimerRdfId=" << outerTimerRdfId << "\n\tp1Chd hp=" << p1Chd.hp() << ", cs=" << p1Chd.ch_state() << ", fc=" << p1Chd.frames_in_ch_state() << ", gud=" << p1Chd.ground_ud() << ", dir=(" << p1Chd.q_x() << ", " << p1Chd.q_y() << ", " << p1Chd.q_z() << ", " << p1Chd.q_w() << "), pos=(" << p1Chd.x() << ", " << p1Chd.y() << "), vel=(" << p1Chd.vel_x() << ", " << p1Chd.vel_y() << "), ground_vel=(" << p1Chd.ground_vel_x() << ", " << p1Chd.ground_vel_y() << ")\n\tnpc2Chd hp=" << npc2Chd.hp() << ", cs=" << npc2Chd.ch_state() << ", fc=" << npc2Chd.frames_in_ch_state() << ", fr=" << npc2Chd.frames_to_recover() << ", dir=(" << npc2Chd.q_x() << ", " << npc2Chd.q_y() << ", " << npc2Chd.q_z() << ", " << npc2Chd.q_w() << "), pos=(" << npc2Chd.x() << ", " << npc2Chd.y() << "), vel=(" << npc2Chd.vel_x() << ", " << npc2Chd.vel_y() << "), cuedCmd=" << npc2.cached_cue_cmd() << ", locking_on_ud=" << npc2Chd.locking_on_ud() << std::endl;

            std::cout << "TestCase38/outerTimerRdfId=" << outerTimerRdfId << "\n\tp1Chd hp=" << p1Chd.hp() << ", cs=" << p1Chd.ch_state() << ", fc=" << p1Chd.frames_in_ch_state() << ", gud=" << p1Chd.ground_ud() << ", dir=(" << p1Chd.q_x() << ", " << p1Chd.q_y() << ", " << p1Chd.q_z() << ", " << p1Chd.q_w() << "), pos=(" << p1Chd.x() << ", " << p1Chd.y() << "), vel=(" << p1Chd.vel_x() << ", " << p1Chd.vel_y() << "), ground_vel=(" << p1Chd.ground_vel_x() << ", " << p1Chd.ground_vel_y() << ")\n\tnpc1Chd hp=" << npc1Chd.hp() << ", cs=" << npc1Chd.ch_state() << ", fc=" << npc1Chd.frames_in_ch_state() << ", fr=" << npc1Chd.frames_to_recover() << ", dir=(" << npc1Chd.q_x() << ", " << npc1Chd.q_y() << ", " << npc1Chd.q_z() << ", " << npc1Chd.q_w() << "), pos=(" << npc1Chd.x() << ", " << npc1Chd.y() << "), vel=(" << npc1Chd.vel_x() << ", " << npc1Chd.vel_y() << "), cuedCmd=" << npc1.cached_cue_cmd() << ", locking_on_ud=" << npc1Chd.locking_on_ud() << "\n\tnpc2Chd hp=" << npc2Chd.hp() << ", cs=" << npc2Chd.ch_state() << ", fc=" << npc2Chd.frames_in_ch_state() << ", fr=" << npc2Chd.frames_to_recover() << ", dir=(" << npc2Chd.q_x() << ", " << npc2Chd.q_y() << ", " << npc2Chd.q_z() << ", " << npc2Chd.q_w() << "), pos=(" << npc2Chd.x() << ", " << npc2Chd.y() << "), vel=(" << npc2Chd.vel_x() << ", " << npc2Chd.vel_y() << "), cuedCmd=" << npc2.cached_cue_cmd() << ", locking_on_ud=" << npc2Chd.locking_on_ud() << std::endl;
        }

        if (7 == outerTimerRdfId) {
            //JPH_ASSERT(0 == npc1Chd.q_x() && 1 == npc1Chd.q_y() && 0 == npc1Chd.q_z() && 0 == npc1Chd.q_w()); // Turned around
        } else if (45 == outerTimerRdfId) {
            JPH_ASSERT(150 == p1Chd.hp());
            JPH_ASSERT(CharacterState::Walking == npc1Chd.ch_state());
            JPH_ASSERT(p1Ud == npc1Chd.locking_on_ud()); // still tracking target
            JPH_ASSERT(3 == npc1.cached_cue_cmd()); // horizontally

            JPH_ASSERT(0 > npc2Chd.vel_x());
            JPH_ASSERT(0 == npc2Chd.q_x() && 1 == npc2Chd.q_y() && 0 == npc2Chd.q_z() && 0 == npc2Chd.q_w());
        } else if (69 == outerTimerRdfId) {
            JPH_ASSERT(CharacterState::Walking == npc1Chd.ch_state());
            JPH_ASSERT(p1Ud == npc1Chd.locking_on_ud()); // still tracking target
            JPH_ASSERT(5 == npc1.cached_cue_cmd()); // but skewed towards the target which climbed up on wall
        } else if (73 == outerTimerRdfId) {
            //JPH_ASSERT(140 == p1Chd.hp());
            JPH_ASSERT(CharacterState::Walking == npc1Chd.ch_state());
            JPH_ASSERT(0 == npc1Chd.locking_on_ud()); // lost target
            JPH_ASSERT(3 == npc1.cached_cue_cmd());  // lost target and thus stops Y-axis force
        } else if (108 == outerTimerRdfId) {
            JPH_ASSERT(CharacterState::OnWallIdle1 == p1Chd.ch_state());
        } else if (140 == outerTimerRdfId) {
            JPH_ASSERT(CharacterState::InAirDashing == p1Chd.ch_state());
            JPH_ASSERT(0 > p1Chd.vel_x());
            JPH_ASSERT(0 < npc1Chd.vel_x());
            JPH_ASSERT(p1Chd.y() > npc1Chd.y()); // player fleeing on top of npc1
        } else if (150 == outerTimerRdfId) {
            JPH_ASSERT(0 == npc2Chd.vel_x()); // Gracing to turn around
            //JPH_ASSERT(0 == npc2.cached_cue_cmd());
            JPH_ASSERT(0 == npc2Chd.q_x() && 1 == npc2Chd.q_y() && 0 == npc2Chd.q_z() && 0 == npc2Chd.q_w());
        } else if (320 == outerTimerRdfId) {
            JPH_ASSERT(-490 < npc2Chd.x());
            JPH_ASSERT(0 < npc2Chd.vel_x());
            JPH_ASSERT(0 == npc2Chd.q_x() && 0 == npc2Chd.q_y() && 0 == npc2Chd.q_z() && 1 == npc2Chd.q_w()); // Turned around
        } else if (400 == outerTimerRdfId) {
            JPH_ASSERT(490 > npc1Chd.x());
            JPH_ASSERT(0 > npc1Chd.vel_x());
            JPH_ASSERT(0 == npc1Chd.q_x() && 1 == npc1Chd.q_y() && 0 == npc1Chd.q_z() && 0 == npc1Chd.q_w()); // Turned around after loss of hunting target
        } else if (900 == outerTimerRdfId) {
            // Rest after lingering
            JPH_ASSERT(CharacterState::InAirIdle1NoJump == npc2Chd.ch_state());
            JPH_ASSERT(0 == npc2Chd.vel_x());
        } else if (1020 == outerTimerRdfId) {
            // Rest after lingering
            JPH_ASSERT(CharacterState::InAirIdle1NoJump == npc1Chd.ch_state());
            JPH_ASSERT(0 == npc1Chd.vel_x());
        }

        outerTimerRdfId++;
    }

    std::cout << "Passed TestCase38: Bat1 vision reaction\n" << std::endl;
    theAllocator->Reset();
    reusedBattle->Clear();

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
        // Upper-Left pillar
        -300, 100,
        -300, 300,
        -320, 300,
        -320, 100
    };

    std::vector<float> slipJumpHull1 = {
        -500, 0,
        -500, 100,
        500, 100,
        500, 0
    };

    std::vector<float> slipJumpHull2 = {
        -80, 150,
        -80, 166,
        -20, 166,
        -20, 150
    };

    std::vector<float> slipJumpHull3 = {
        -20, 150,
        -20, 166,
        +40, 166,
        +40, 150
    };

    std::vector<float> slipJumpHull4 = {
        +20, 150,
        +20, 166,
        +80, 166,
        +80, 150
    };

    std::vector<float> trapMapHull1 = {
        // Lower floor
        -500, 0,
        -500, 100,
        500, 100,
        500, 0
    };

    std::vector<float> trapMapHull2 = {
        // Left pillar
        -800, 0,
        -800, 1000,
        -500, 1000,
        -500, 0
    };

    std::vector<float> trapMapHull3 = {
        // Right pillar
        500, 1000,
        800, 1000,
        800, 0,
        500, 0,
    };

    std::vector<float> trapMapHull4 = {
        // Lower-right pillar
        300, 142,
        300, 206,
        420, 206,
        420, 142
    };

    std::vector<float> trapMapHull5 = {
        // Top Ceiling
        -500, 900,
        -500, 1000,
        500, 1000,
        500, 900
    };

    std::vector<float> crouchMapHull1 = {
        // Right pillar
        1920, 528,
        2352, 528,
        2352, 0,
        1920, 0
    };

    std::vector<float> crouchMapHull2 = {
        // Left pillar
        224, 528,
        640, 528,
        640, 0,
        224, 0
    };

    std::vector<float> crouchMapHull3 = {
        // Top ceiling
        640, 528,
        1920, 528,
        1920, 512,
        640, 512,
    };

    std::vector<float> crouchMapHull4 = {
        // Crouch forcer
        832, 200,
        1088, 200,
        1088, 120,
        832, 120
    };

    std::vector<float> crouchMapHull5 = {
        // Floor
        528, 80,
        1920, 80,
        1920, 0,
        528, 0
    };
    
    std::vector<float> flyingMapHull1 = {
        // Lower floor
        -500, 0,
        -500, 100,
        500, 100,
        500, 0
    };

    std::vector<float> flyingMapHull2 = {
        // Left pillar
        -800, 0,
        -800, 1000,
        -500, 1000,
        -500, 0
    };

    std::vector<float> flyingMapHull3 = {
        // Right pillar
        500, 1000,
        800, 1000,
        800, 0,
        500, 0,
    };

    std::vector<float> flyingMapHull4 = {
        // Ceiling
        -500, 500,
        -500, 600,
        500, 600,
        500, 500
    };

    std::vector<std::vector<float>> hulls = {hull1, hull2, hull3};
    std::vector<std::vector<float>> fallenDeathHulls = {hull1, hull2};
    std::vector<std::vector<float>> npcVisionHulls = {npcVisionHull1, npcVisionHull2, npcVisionHull3, npcVisionHull4, npcVisionHull5, npcVisionHull6, npcVisionHull7};
    std::vector<std::vector<float>> wideMapHulls = {wideMapHull1, wideMapHull2, wideMapHull3, wideMapHull4};
    std::vector<std::vector<float>> slipJumpMapHulls = {slipJumpHull1, slipJumpHull2, slipJumpHull3, slipJumpHull4};
    std::vector<std::vector<float>> trapMapHulls = {trapMapHull1, trapMapHull2, trapMapHull3, trapMapHull4, trapMapHull5};
    std::vector<std::vector<float>> crouchMapHulls = {
        crouchMapHull1
        , crouchMapHull2
        , crouchMapHull3
        , crouchMapHull4
        , crouchMapHull5};

    std::vector<std::vector<float>> flyingMapHulls = { flyingMapHull1, flyingMapHull2, flyingMapHull3, flyingMapHull4 };

    JPH_Init(10*1024*1024);
    std::cout << "Initiated" << std::endl;
    
    RegisterDebugCallback(DebugLogCb);

    FrontendBattle* battle = static_cast<FrontendBattle*>(FRONTEND_CreateBattle(512, true));
    std::cout << "Created battle = " << battle << std::endl;

    int selfJoinIndex = 1;
    
    google::protobuf::Arena testCaseDataAllocator;
    google::protobuf::Arena* pbTestCaseDataAllocator = &testCaseDataAllocator;

    /*
    [WARNING] There's a "Windows static pb-arena (i.e. without Abseil DLL)" specific bug if "pbTestCaseDataAllocator->Reset()" is called AFTER "reusedBattle->Clear()" for around 20 times. 

    Hence I put "theAllocator->Reset()" BEFORE "reusedBattle->Clear()" in each "runTestCaseXxx(...)".
    */

    runTestCase1(battle, hulls, selfJoinIndex, pbTestCaseDataAllocator);
    runTestCase2(battle, hulls, selfJoinIndex, pbTestCaseDataAllocator);
    runTestCase3(battle, hulls, selfJoinIndex, pbTestCaseDataAllocator);
    runTestCase4(battle, hulls, selfJoinIndex, pbTestCaseDataAllocator);
    runTestCase5(battle, hulls, selfJoinIndex, pbTestCaseDataAllocator);
    runTestCase6(battle, hulls, selfJoinIndex, pbTestCaseDataAllocator);
    runTestCase7(battle, hulls, selfJoinIndex, pbTestCaseDataAllocator);
    runTestCase8(battle, npcVisionHulls, selfJoinIndex, pbTestCaseDataAllocator);
    runTestCase9(battle, hulls, selfJoinIndex, pbTestCaseDataAllocator);
    runTestCase10(battle, hulls, selfJoinIndex, pbTestCaseDataAllocator);
    runTestCase11(battle, hulls, selfJoinIndex, pbTestCaseDataAllocator);
    runTestCase12(battle, fallenDeathHulls, selfJoinIndex, pbTestCaseDataAllocator);
    runTestCase13(battle, wideMapHulls, selfJoinIndex, pbTestCaseDataAllocator);
    runTestCase14(battle, wideMapHulls, selfJoinIndex, pbTestCaseDataAllocator);
    runTestCase15(battle, hulls, selfJoinIndex, pbTestCaseDataAllocator);
    runTestCase16(battle, hulls, selfJoinIndex, pbTestCaseDataAllocator);
    runTestCase17(battle, wideMapHulls, selfJoinIndex, pbTestCaseDataAllocator);
    runTestCase18(battle, wideMapHulls, selfJoinIndex, pbTestCaseDataAllocator);
    runTestCase19(battle, slipJumpMapHulls, selfJoinIndex, pbTestCaseDataAllocator);
    runTestCase20(battle, hulls, selfJoinIndex, pbTestCaseDataAllocator);
    runTestCase21(battle, hulls, selfJoinIndex, pbTestCaseDataAllocator);
    runTestCase22(battle, hulls, selfJoinIndex, pbTestCaseDataAllocator);
    runTestCase23(battle, wideMapHulls, selfJoinIndex, pbTestCaseDataAllocator);
    runTestCase24(battle, hulls, selfJoinIndex, pbTestCaseDataAllocator);
    runTestCase25(battle, trapMapHulls, selfJoinIndex, pbTestCaseDataAllocator);
    runTestCase26(battle, crouchMapHulls, selfJoinIndex, pbTestCaseDataAllocator);
    runTestCase27(battle, wideMapHulls, selfJoinIndex, pbTestCaseDataAllocator);
    runTestCase28(battle, wideMapHulls, selfJoinIndex, pbTestCaseDataAllocator);
    runTestCase29(battle, wideMapHulls, selfJoinIndex, pbTestCaseDataAllocator);
    runTestCase30(battle, wideMapHulls, selfJoinIndex, pbTestCaseDataAllocator);
    runTestCase31(battle, wideMapHulls, selfJoinIndex, pbTestCaseDataAllocator);
    runTestCase32(battle, hulls, selfJoinIndex, pbTestCaseDataAllocator);
    runTestCase33(battle, hulls, selfJoinIndex, pbTestCaseDataAllocator);
    runTestCase34(battle, wideMapHulls, selfJoinIndex, pbTestCaseDataAllocator);
    runTestCase35(wideMapHulls, selfJoinIndex, pbTestCaseDataAllocator);
    runTestCase36(battle, wideMapHulls, selfJoinIndex, pbTestCaseDataAllocator);
    runTestCase37(battle, wideMapHulls, selfJoinIndex, pbTestCaseDataAllocator);
    runTestCase38(battle, wideMapHulls, selfJoinIndex, pbTestCaseDataAllocator);
    
    // clean up
    // [REMINDER] "startRdf" and "startRdf" will be automatically deallocated by the destructor of "wsReq"
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
