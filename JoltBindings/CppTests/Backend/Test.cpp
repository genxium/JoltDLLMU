#include "joltc_export.h" // imports the "JOLTC_EXPORT" macro for "serializable_data.pb.h"
#include <Jolt/Jolt.h> // imports the "JPH_EXPORT" macro for classes under namespace JPH
#include "serializable_data.pb.h"
#include "joltc_api.h" 
#include "PbConsts.h"
#include "CppOnlyConsts.h"
#include "DebugLog.h"

#include "BackendBattle.h"
#include <chrono>
#include <fstream>
#include <filesystem>
#include <google/protobuf/arena.h>
google::protobuf::Arena pbTestCaseDataAllocator;

using namespace jtshared;
using namespace std::filesystem;

const int pbBufferSizeLimit = (1 << 14);
char pbByteBuffer[pbBufferSizeLimit];
char downsyncSnapshotByteBuffer[pbBufferSizeLimit];

RenderFrame* mockStartRdf() {
    auto chSpecies = globalPrimitiveConsts->ch_species();
    const int roomCapacity = 2;
    auto startRdf = BaseBattle::NewPreallocatedRdf(roomCapacity, 8, 128);
    startRdf->set_id(1);
    int pickableIdCounter = 1;
    int npcIdCounter = 1;
    int bulletIdCounter = 1;
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
    player1->set_join_index(1);
    player1->set_revival_x(playerCh1->x());
    player1->set_revival_y(playerCh1->y());

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
    player2->set_join_index(2);
    player2->set_revival_x(playerCh2->x());
    player2->set_revival_y(playerCh2->y());

    startRdf->set_npc_id_counter(npcIdCounter);
    startRdf->set_npc_count(npcIdCounter-1);

    startRdf->set_bullet_id_counter(bulletIdCounter);
    startRdf->set_pickable_id_counter(pickableIdCounter);


    return startRdf;
}

void DebugLogCb(const char* message, int color, int size) {
    std::cout << message << std::endl;
}

int forceConfirmedStEvictedCnt = 0, oldLcacIfdId = 0, newLcacIfdId = 0, oldDynamicsRdfId = 0, newDynamicsRdfId = 0;
bool runTestCase1(BackendBattle* reusedBattle, WsReq* initializerMapData) {
    reusedBattle->ResetStartRdf(initializerMapData);
    DownsyncSnapshot* downsyncSnapshotHolder = google::protobuf::Arena::Create<DownsyncSnapshot>(&pbTestCaseDataAllocator);
    int maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;

    auto reqA1 = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
    reqA1->set_join_index(1);
    auto a1 = reqA1->mutable_upsync_snapshot();
    a1->set_st_ifd_id(0);
    for (int ifdId = 0; ifdId <= 30; ifdId++) {
        a1->add_cmd_list(0);
    }

    long outBytesCnt = pbBufferSizeLimit;
    memset(downsyncSnapshotByteBuffer, 0, sizeof(downsyncSnapshotByteBuffer));
    reusedBattle->OnUpsyncSnapshotReceived(reqA1->join_index(), reqA1->upsync_snapshot(), false, true, downsyncSnapshotByteBuffer, &outBytesCnt, &forceConfirmedStEvictedCnt, &oldLcacIfdId, &newLcacIfdId, &oldDynamicsRdfId, &newDynamicsRdfId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
    JPH_ASSERT(-1 == newLcacIfdId && 0 == reusedBattle->ifdBuffer.StFrameId && 31 == reusedBattle->ifdBuffer.Cnt && 0 == outBytesCnt && 0 == forceConfirmedStEvictedCnt);
    
    auto reqA2 = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
    reqA2->set_join_index(1);
    auto a2 = reqA2->mutable_upsync_snapshot();
    a2->set_st_ifd_id(150);
    for (int ifdId = 150; ifdId <= 170; ifdId++) {
        a2->add_cmd_list(16);
    }
    outBytesCnt = pbBufferSizeLimit;
    memset(downsyncSnapshotByteBuffer, 0, sizeof(downsyncSnapshotByteBuffer));
    reusedBattle->OnUpsyncSnapshotReceived(reqA2->join_index(), reqA2->upsync_snapshot(), false, true, downsyncSnapshotByteBuffer, &outBytesCnt, &forceConfirmedStEvictedCnt, &oldLcacIfdId, &newLcacIfdId, &oldDynamicsRdfId, &newDynamicsRdfId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
    JPH_ASSERT(-1 == newLcacIfdId && 0 == reusedBattle->ifdBuffer.StFrameId && 171 == reusedBattle->ifdBuffer.Cnt && 0 == outBytesCnt && 0 == forceConfirmedStEvictedCnt);

    auto reqB1 = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
    reqB1->set_join_index(2);
    auto b1 = reqB1->mutable_upsync_snapshot();
    b1->set_st_ifd_id(0);
    for (int ifdId = 0; ifdId <= 100; ifdId++) {
        b1->add_cmd_list(0);
    }
    outBytesCnt = pbBufferSizeLimit;
    memset(downsyncSnapshotByteBuffer, 0, sizeof(downsyncSnapshotByteBuffer));
    reusedBattle->OnUpsyncSnapshotReceived(reqB1->join_index(), reqB1->upsync_snapshot(), true, false, downsyncSnapshotByteBuffer, &outBytesCnt, &forceConfirmedStEvictedCnt, &oldLcacIfdId, &newLcacIfdId, &oldDynamicsRdfId, &newDynamicsRdfId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
    JPH_ASSERT(30 == newLcacIfdId && 0 == reusedBattle->ifdBuffer.StFrameId && 171 == reusedBattle->ifdBuffer.Cnt && 0 < outBytesCnt && 0 == forceConfirmedStEvictedCnt);
    downsyncSnapshotHolder->ParseFromArray(downsyncSnapshotByteBuffer, outBytesCnt);
    JPH_ASSERT(0 == downsyncSnapshotHolder->st_ifd_id() && 31 == downsyncSnapshotHolder->ifd_batch_size()); // [0, 30]

    auto reqA3 = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
    reqA3->set_join_index(1);
    auto a3 = reqA3->mutable_upsync_snapshot();
    a3->set_st_ifd_id(31);
    for (int ifdId = 31; ifdId <= 70; ifdId++) {
        a3->add_cmd_list(128);
    }
    outBytesCnt = pbBufferSizeLimit;
    memset(downsyncSnapshotByteBuffer, 0, sizeof(downsyncSnapshotByteBuffer));
    reusedBattle->OnUpsyncSnapshotReceived(reqA3->join_index(), reqA3->upsync_snapshot(), false, true, downsyncSnapshotByteBuffer, &outBytesCnt, &forceConfirmedStEvictedCnt, &oldLcacIfdId, &newLcacIfdId, &oldDynamicsRdfId, &newDynamicsRdfId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
    JPH_ASSERT(70 == newLcacIfdId && 0 == reusedBattle->ifdBuffer.StFrameId && 171 == reusedBattle->ifdBuffer.Cnt && 0 < outBytesCnt && 0 == forceConfirmedStEvictedCnt);
    downsyncSnapshotHolder->ParseFromArray(downsyncSnapshotByteBuffer, outBytesCnt);
    JPH_ASSERT(31 == downsyncSnapshotHolder->st_ifd_id() && 40 == downsyncSnapshotHolder->ifd_batch_size() && !downsyncSnapshotHolder->has_ref_rdf()); // [31, 70]

    auto reqB2 = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
    reqB2->set_join_index(2);
    auto b2 = reqB2->mutable_upsync_snapshot();
    b2->set_st_ifd_id(101);
    for (int ifdId = 101; ifdId <= 200; ifdId++) {
        b2->add_cmd_list(0);
    }
    outBytesCnt = pbBufferSizeLimit;
    memset(downsyncSnapshotByteBuffer, 0, sizeof(downsyncSnapshotByteBuffer));
    reusedBattle->OnUpsyncSnapshotReceived(reqB2->join_index(), reqB2->upsync_snapshot(), true, false, downsyncSnapshotByteBuffer, &outBytesCnt, &forceConfirmedStEvictedCnt, &oldLcacIfdId, &newLcacIfdId, &oldDynamicsRdfId, &newDynamicsRdfId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
    JPH_ASSERT(70 == newLcacIfdId && 0 == reusedBattle->ifdBuffer.StFrameId && 201 == reusedBattle->ifdBuffer.Cnt && 0 == outBytesCnt && 0 == forceConfirmedStEvictedCnt);

    std::cout << "Passed TestCase1" << std::endl;
    reusedBattle->Clear();   
    return true;
}

bool runTestCase2(BackendBattle* reusedBattle, WsReq* initializerMapData) {
    reusedBattle->ResetStartRdf(initializerMapData);
    DownsyncSnapshot* downsyncSnapshotHolder = google::protobuf::Arena::Create<DownsyncSnapshot>(&pbTestCaseDataAllocator);
    int maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;

    auto reqA1 = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
    reqA1->set_join_index(1);
    auto a1 = reqA1->mutable_upsync_snapshot();
    a1->set_st_ifd_id(0);
    for (int ifdId = 0; ifdId <= 300; ifdId++) {
        a1->add_cmd_list(0);
    }

    long outBytesCnt = pbBufferSizeLimit;
    memset(downsyncSnapshotByteBuffer, 0, sizeof(downsyncSnapshotByteBuffer));
    bool upserted = reusedBattle->OnUpsyncSnapshotReceived(reqA1->join_index(), reqA1->upsync_snapshot(), false, true, downsyncSnapshotByteBuffer, &outBytesCnt, &forceConfirmedStEvictedCnt, &oldLcacIfdId, &newLcacIfdId, &oldDynamicsRdfId, &newDynamicsRdfId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
    JPH_ASSERT(upserted);
    JPH_ASSERT(-1 == newLcacIfdId && 0 == reusedBattle->ifdBuffer.StFrameId && 301 == reusedBattle->ifdBuffer.Cnt && 0 == outBytesCnt && 0 == forceConfirmedStEvictedCnt);

    auto reqA2 = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
    reqA2->set_join_index(1);
    auto a2 = reqA2->mutable_upsync_snapshot();
    a2->set_st_ifd_id(400);
    for (int ifdId = 400; ifdId <= 400; ifdId++) {
        a2->add_cmd_list(16);
    }
    outBytesCnt = pbBufferSizeLimit;
    memset(downsyncSnapshotByteBuffer, 0, sizeof(downsyncSnapshotByteBuffer));
    upserted = reusedBattle->OnUpsyncSnapshotReceived(reqA2->join_index(), reqA2->upsync_snapshot(), true, false, downsyncSnapshotByteBuffer, &outBytesCnt, &forceConfirmedStEvictedCnt, &oldLcacIfdId, &newLcacIfdId, &oldDynamicsRdfId, &newDynamicsRdfId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
    JPH_ASSERT(upserted);
    JPH_ASSERT(-1 == newLcacIfdId && 0 == reusedBattle->ifdBuffer.StFrameId && 401 == reusedBattle->ifdBuffer.Cnt && 0 == outBytesCnt && 0 == forceConfirmedStEvictedCnt);
    
    auto reqB1 = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
    reqB1->set_join_index(2);
    auto b1 = reqB1->mutable_upsync_snapshot();
    b1->set_st_ifd_id(0);
    for (int ifdId = 0; ifdId <= 512; ifdId++) {
        b1->add_cmd_list(0);
    }
    outBytesCnt = pbBufferSizeLimit;
    memset(downsyncSnapshotByteBuffer, 0, sizeof(downsyncSnapshotByteBuffer));
    upserted = reusedBattle->OnUpsyncSnapshotReceived(reqB1->join_index(), reqB1->upsync_snapshot(), true, false, downsyncSnapshotByteBuffer, &outBytesCnt, &forceConfirmedStEvictedCnt, &oldLcacIfdId, &newLcacIfdId, &oldDynamicsRdfId, &newDynamicsRdfId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
    JPH_ASSERT(upserted);
    JPH_ASSERT(300 == newLcacIfdId && 62 == reusedBattle->ifdBuffer.StFrameId && 513 == reusedBattle->ifdBuffer.EdFrameId && 451 == reusedBattle->ifdBuffer.Cnt && 0 < outBytesCnt);
    JPH_ASSERT(0 == forceConfirmedStEvictedCnt); // i.e. all regularly confirmed and evicted, no force-confirmation occurred
    downsyncSnapshotHolder->ParseFromArray(downsyncSnapshotByteBuffer, outBytesCnt);
    JPH_ASSERT(0 == downsyncSnapshotHolder->st_ifd_id() && 301 == downsyncSnapshotHolder->ifd_batch_size() && !downsyncSnapshotHolder->has_ref_rdf()); // [0, 300]

    auto reqA3 = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
    reqA3->set_join_index(1);
    auto a3 = reqA3->mutable_upsync_snapshot();
    a3->set_st_ifd_id(401);
    for (int ifdId = 401; ifdId <= 700; ifdId++) {
        a3->add_cmd_list(16);
    }
    outBytesCnt = pbBufferSizeLimit;
    memset(downsyncSnapshotByteBuffer, 0, sizeof(downsyncSnapshotByteBuffer));
    upserted = reusedBattle->OnUpsyncSnapshotReceived(reqA3->join_index(), reqA3->upsync_snapshot(), false, true, downsyncSnapshotByteBuffer, &outBytesCnt, &forceConfirmedStEvictedCnt, &oldLcacIfdId, &newLcacIfdId, &oldDynamicsRdfId, &newDynamicsRdfId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
    JPH_ASSERT(upserted);
    JPH_ASSERT(300 == newLcacIfdId && 250 == reusedBattle->ifdBuffer.StFrameId && 701 == reusedBattle->ifdBuffer.EdFrameId && 451 == reusedBattle->ifdBuffer.Cnt && 0 == outBytesCnt && 0 == forceConfirmedStEvictedCnt); // i.e. as "ifdBuffer.StFrameId" hasn't reached "lcacIfdId" yet, there's no new downsync snapshot 

    auto reqB2 = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
    reqB2->set_join_index(2);
    auto b2 = reqB2->mutable_upsync_snapshot();
    b2->set_st_ifd_id(800);
    for (int ifdId = 800; ifdId <= 801; ifdId++) {
        b2->add_cmd_list(0);
    }
    outBytesCnt = pbBufferSizeLimit;
    memset(downsyncSnapshotByteBuffer, 0, sizeof(downsyncSnapshotByteBuffer));
    upserted = reusedBattle->OnUpsyncSnapshotReceived(reqB2->join_index(), reqB2->upsync_snapshot(), false, true, downsyncSnapshotByteBuffer, &outBytesCnt, &forceConfirmedStEvictedCnt, &oldLcacIfdId, &newLcacIfdId, &oldDynamicsRdfId, &newDynamicsRdfId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
    JPH_ASSERT(!upserted);
    JPH_ASSERT(300 == newLcacIfdId && 701 == reusedBattle->ifdBuffer.EdFrameId && 451 == reusedBattle->ifdBuffer.Cnt && 0 == outBytesCnt && 0 == forceConfirmedStEvictedCnt);

    auto reqA4 = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
    reqA4->set_join_index(1);
    auto a4 = reqA4->mutable_upsync_snapshot();
    a4->set_st_ifd_id(298);
    for (int ifdId = 298; ifdId <= 420; ifdId++) {
        a4->add_cmd_list(32);
    }
    outBytesCnt = pbBufferSizeLimit;
    memset(downsyncSnapshotByteBuffer, 0, sizeof(downsyncSnapshotByteBuffer));
    upserted = reusedBattle->OnUpsyncSnapshotReceived(reqA4->join_index(), reqA4->upsync_snapshot(), false, true, downsyncSnapshotByteBuffer, &outBytesCnt, &forceConfirmedStEvictedCnt, &oldLcacIfdId, &newLcacIfdId, &oldDynamicsRdfId, &newDynamicsRdfId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
    JPH_ASSERT(upserted);
    JPH_ASSERT(512 == newLcacIfdId && 250 == reusedBattle->ifdBuffer.StFrameId && 701 == reusedBattle->ifdBuffer.EdFrameId && 451 == reusedBattle->ifdBuffer.Cnt && 0 < outBytesCnt && 0 == forceConfirmedStEvictedCnt);
    downsyncSnapshotHolder->ParseFromArray(downsyncSnapshotByteBuffer, outBytesCnt);
    JPH_ASSERT(301 == downsyncSnapshotHolder->st_ifd_id() && 212 == downsyncSnapshotHolder->ifd_batch_size()); // [301, 512]

    std::cout << "Passed TestCase2" << std::endl;
    reusedBattle->Clear();   
    return true;
}

bool runTestCase3(BackendBattle* reusedBattle, WsReq* initializerMapData) {
    reusedBattle->ResetStartRdf(initializerMapData);
    DownsyncSnapshot* downsyncSnapshotHolder = google::protobuf::Arena::Create<DownsyncSnapshot>(&pbTestCaseDataAllocator);
    int maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;

    auto reqA1 = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
    reqA1->set_join_index(1);
    auto a1 = reqA1->mutable_upsync_snapshot();
    a1->set_st_ifd_id(0);
    for (int ifdId = 0; ifdId <= 120; ifdId++) {
        a1->add_cmd_list(0);
    }

    long outBytesCnt = pbBufferSizeLimit;
    memset(downsyncSnapshotByteBuffer, 0, sizeof(downsyncSnapshotByteBuffer));
    bool upserted = reusedBattle->OnUpsyncSnapshotReceived(reqA1->join_index(), reqA1->upsync_snapshot(), false, true, downsyncSnapshotByteBuffer, &outBytesCnt, &forceConfirmedStEvictedCnt, &oldLcacIfdId, &newLcacIfdId, &oldDynamicsRdfId, &newDynamicsRdfId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
    JPH_ASSERT(upserted);
    JPH_ASSERT(-1 == newLcacIfdId && 0 == reusedBattle->ifdBuffer.StFrameId && 121 == reusedBattle->ifdBuffer.Cnt && 0 == outBytesCnt);
 
    auto reqB1 = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
    reqB1->set_join_index(2);
    auto b1 = reqB1->mutable_upsync_snapshot();
    b1->set_st_ifd_id(0);
    for (int ifdId = 0; ifdId <= 120; ifdId++) {
        b1->add_cmd_list(0);
    }
    outBytesCnt = pbBufferSizeLimit;
    memset(downsyncSnapshotByteBuffer, 0, sizeof(downsyncSnapshotByteBuffer));
    upserted = reusedBattle->OnUpsyncSnapshotReceived(reqB1->join_index(), reqB1->upsync_snapshot(), true, false, downsyncSnapshotByteBuffer, &outBytesCnt, &forceConfirmedStEvictedCnt, &oldLcacIfdId, &newLcacIfdId, &oldDynamicsRdfId, &newDynamicsRdfId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
    JPH_ASSERT(upserted);
    JPH_ASSERT(120 == newLcacIfdId && 0 == reusedBattle->ifdBuffer.StFrameId && 121 == reusedBattle->ifdBuffer.Cnt && 0 < outBytesCnt); 
    downsyncSnapshotHolder->ParseFromArray(downsyncSnapshotByteBuffer, outBytesCnt);
    JPH_ASSERT(121 == downsyncSnapshotHolder->ifd_batch_size());

    auto reqA2 = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
    reqA2->set_join_index(1);
    auto a2 = reqA2->mutable_upsync_snapshot();
    a2->set_st_ifd_id(459);
    for (int ifdId = 459; ifdId <= 600; ifdId++) {
        a2->add_cmd_list(16);
    }
    outBytesCnt = pbBufferSizeLimit;
    memset(downsyncSnapshotByteBuffer, 0, sizeof(downsyncSnapshotByteBuffer));
    upserted = reusedBattle->OnUpsyncSnapshotReceived(reqA2->join_index(), reqA2->upsync_snapshot(), true, false, downsyncSnapshotByteBuffer, &outBytesCnt, &forceConfirmedStEvictedCnt, &oldLcacIfdId, &newLcacIfdId, &oldDynamicsRdfId, &newDynamicsRdfId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
    JPH_ASSERT(upserted);
    JPH_ASSERT(149 == newLcacIfdId && 150 == reusedBattle->ifdBuffer.StFrameId && 451 == reusedBattle->ifdBuffer.Cnt && 0 < outBytesCnt);
    downsyncSnapshotHolder->ParseFromArray(downsyncSnapshotByteBuffer, outBytesCnt);
    JPH_ASSERT(29 == downsyncSnapshotHolder->ifd_batch_size()); // [WARNING] There're 29 ifds, i.e. from 121 to 149 forced out of "ifdBuffer.StFrameId"

    std::cout << "Passed TestCase3" << std::endl;
    reusedBattle->Clear();   
    return true;
}

bool runTestCase4(BackendBattle* reusedBattle, WsReq* initializerMapData) {
    reusedBattle->ResetStartRdf(initializerMapData);
    DownsyncSnapshot* downsyncSnapshotHolder = google::protobuf::Arena::Create<DownsyncSnapshot>(&pbTestCaseDataAllocator);
    int maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;

    long outBytesCnt = pbBufferSizeLimit;

    auto reqA1 = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
    reqA1->set_join_index(1);
    auto a1 = reqA1->mutable_upsync_snapshot();
    a1->set_st_ifd_id(459);
    for (int ifdId = 459; ifdId <= 600; ifdId++) {
        a1->add_cmd_list(16);
    }
    memset(downsyncSnapshotByteBuffer, 0, sizeof(downsyncSnapshotByteBuffer));
    bool upserted = reusedBattle->OnUpsyncSnapshotReceived(reqA1->join_index(), reqA1->upsync_snapshot(), true, false, downsyncSnapshotByteBuffer, &outBytesCnt, &forceConfirmedStEvictedCnt, &oldLcacIfdId, &newLcacIfdId, &oldDynamicsRdfId, &newDynamicsRdfId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
    JPH_ASSERT(upserted);
    JPH_ASSERT(149 == newLcacIfdId && 150 == reusedBattle->ifdBuffer.StFrameId && 601 == reusedBattle->ifdBuffer.EdFrameId && 451 == reusedBattle->ifdBuffer.Cnt && 0 < outBytesCnt);
    downsyncSnapshotHolder->ParseFromArray(downsyncSnapshotByteBuffer, outBytesCnt);
    JPH_ASSERT(150 == forceConfirmedStEvictedCnt); // "gapCnt == 9" && "otherForceConfirmedStEvictedCnt == 141 == 600-459"
    JPH_ASSERT(forceConfirmedStEvictedCnt == downsyncSnapshotHolder->ifd_batch_size() && 0 == downsyncSnapshotHolder->st_ifd_id()); // [0, 149]

    std::cout << "Passed TestCase4" << std::endl;
    reusedBattle->Clear();   
    return true;
}

bool runTestCase5(BackendBattle* reusedBattle, WsReq* initializerMapData) {
    reusedBattle->ResetStartRdf(initializerMapData);
    DownsyncSnapshot* downsyncSnapshotHolder = google::protobuf::Arena::Create<DownsyncSnapshot>(&pbTestCaseDataAllocator);
    int maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;

    long outBytesCnt = pbBufferSizeLimit;

    auto reqA1 = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
    reqA1->set_join_index(1);
    auto a1 = reqA1->mutable_upsync_snapshot();
    a1->set_st_ifd_id(0);
    for (int ifdId = 0; ifdId <= 2; ifdId++) {
        a1->add_cmd_list(16);
    }
    outBytesCnt = pbBufferSizeLimit;
    memset(downsyncSnapshotByteBuffer, 0, sizeof(downsyncSnapshotByteBuffer));
    bool upserted = reusedBattle->OnUpsyncSnapshotReceived(reqA1->join_index(), reqA1->upsync_snapshot(), true, false, downsyncSnapshotByteBuffer, &outBytesCnt, &forceConfirmedStEvictedCnt, &oldLcacIfdId, &newLcacIfdId, &oldDynamicsRdfId, &newDynamicsRdfId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
    JPH_ASSERT(upserted);
    JPH_ASSERT(-1 == newLcacIfdId && 0 == reusedBattle->ifdBuffer.StFrameId && 3 == reusedBattle->ifdBuffer.EdFrameId && 3 == reusedBattle->ifdBuffer.Cnt && 0 == outBytesCnt);

    auto reqB1 = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
    reqB1->set_join_index(2);
    auto b1 = reqB1->mutable_upsync_snapshot();
    b1->set_st_ifd_id(0);
    for (int ifdId = 0; ifdId <= 2; ifdId++) {
        b1->add_cmd_list(0);
    }
    outBytesCnt = pbBufferSizeLimit;
    memset(downsyncSnapshotByteBuffer, 0, sizeof(downsyncSnapshotByteBuffer));
    upserted = reusedBattle->OnUpsyncSnapshotReceived(reqB1->join_index(), reqB1->upsync_snapshot(), true, false, downsyncSnapshotByteBuffer, &outBytesCnt, &forceConfirmedStEvictedCnt, &oldLcacIfdId, &newLcacIfdId, &oldDynamicsRdfId, &newDynamicsRdfId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
    JPH_ASSERT(upserted);
    JPH_ASSERT(2 == newLcacIfdId && 0 == reusedBattle->ifdBuffer.StFrameId && 3 == reusedBattle->ifdBuffer.EdFrameId && 3 == reusedBattle->ifdBuffer.Cnt && 0 < outBytesCnt);
    downsyncSnapshotHolder->ParseFromArray(downsyncSnapshotByteBuffer, outBytesCnt);
    JPH_ASSERT(3 == downsyncSnapshotHolder->ifd_batch_size() && 0 == downsyncSnapshotHolder->st_ifd_id()); // [0, 2]

    auto reqA2 = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
    reqA2->set_join_index(1);
    auto a2 = reqA2->mutable_upsync_snapshot();
    a2->set_st_ifd_id(459);
    for (int ifdId = 459; ifdId <= 600; ifdId++) {
        a2->add_cmd_list(16);
    }
    outBytesCnt = pbBufferSizeLimit;
    memset(downsyncSnapshotByteBuffer, 0, sizeof(downsyncSnapshotByteBuffer));
    upserted = reusedBattle->OnUpsyncSnapshotReceived(reqA2->join_index(), reqA2->upsync_snapshot(), true, false, downsyncSnapshotByteBuffer, &outBytesCnt, &forceConfirmedStEvictedCnt, &oldLcacIfdId, &newLcacIfdId, &oldDynamicsRdfId, &newDynamicsRdfId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
    JPH_ASSERT(upserted);
    JPH_ASSERT(149 == newLcacIfdId && 150 == reusedBattle->ifdBuffer.StFrameId && 601 == reusedBattle->ifdBuffer.EdFrameId && 451 == reusedBattle->ifdBuffer.Cnt && 0 < outBytesCnt);
    downsyncSnapshotHolder->ParseFromArray(downsyncSnapshotByteBuffer, outBytesCnt);
    JPH_ASSERT(147 == forceConfirmedStEvictedCnt); // Minus 3 already confirmed "ifd"s [0, 1, 2] compared with TestCase4
    JPH_ASSERT(forceConfirmedStEvictedCnt == downsyncSnapshotHolder->ifd_batch_size() && 3 == downsyncSnapshotHolder->st_ifd_id()); // [3, 149]

    std::cout << "Passed TestCase5" << std::endl;
    reusedBattle->Clear();   
    return true;
}

bool runTestCase6(BackendBattle* reusedBattle, WsReq* initializerMapData) {
    reusedBattle->ResetStartRdf(initializerMapData);
    DownsyncSnapshot* downsyncSnapshotHolder = google::protobuf::Arena::Create<DownsyncSnapshot>(&pbTestCaseDataAllocator);
    int maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;

    auto reqA1 = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
    reqA1->set_join_index(1);
    auto a1 = reqA1->mutable_upsync_snapshot();
    a1->set_st_ifd_id(0);
    for (int ifdId = 0; ifdId <= 30; ifdId++) {
        a1->add_cmd_list(0);
    }

    long outBytesCnt = pbBufferSizeLimit;
    memset(downsyncSnapshotByteBuffer, 0, sizeof(downsyncSnapshotByteBuffer));
    reusedBattle->OnUpsyncSnapshotReceived(reqA1->join_index(), reqA1->upsync_snapshot(), false, true, downsyncSnapshotByteBuffer, &outBytesCnt, &forceConfirmedStEvictedCnt, &oldLcacIfdId, &newLcacIfdId, &oldDynamicsRdfId, &newDynamicsRdfId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
    JPH_ASSERT(-1 == newLcacIfdId && 0 == reusedBattle->ifdBuffer.StFrameId && 31 == reusedBattle->ifdBuffer.Cnt && 0 == outBytesCnt && 0 == forceConfirmedStEvictedCnt);
    
    auto reqA2 = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
    reqA2->set_join_index(1);
    auto a2 = reqA2->mutable_upsync_snapshot();
    a2->set_st_ifd_id(150);
    for (int ifdId = 150; ifdId <= 170; ifdId++) {
        a2->add_cmd_list(16);
    }
    outBytesCnt = pbBufferSizeLimit;
    memset(downsyncSnapshotByteBuffer, 0, sizeof(downsyncSnapshotByteBuffer));
    reusedBattle->OnUpsyncSnapshotReceived(reqA2->join_index(), reqA2->upsync_snapshot(), false, true, downsyncSnapshotByteBuffer, &outBytesCnt, &forceConfirmedStEvictedCnt, &oldLcacIfdId, &newLcacIfdId, &oldDynamicsRdfId, &newDynamicsRdfId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
    JPH_ASSERT(-1 == newLcacIfdId && 0 == reusedBattle->ifdBuffer.StFrameId && 171 == reusedBattle->ifdBuffer.Cnt && 0 == outBytesCnt && 0 == forceConfirmedStEvictedCnt);

    auto reqB1 = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
    reqB1->set_join_index(2);
    auto b1 = reqB1->mutable_upsync_snapshot();
    b1->set_st_ifd_id(0);
    for (int ifdId = 0; ifdId <= 100; ifdId++) {
        b1->add_cmd_list(0);
    }
    outBytesCnt = pbBufferSizeLimit;
    memset(downsyncSnapshotByteBuffer, 0, sizeof(downsyncSnapshotByteBuffer));
    reusedBattle->OnUpsyncSnapshotReceived(reqB1->join_index(), reqB1->upsync_snapshot(), true, false, downsyncSnapshotByteBuffer, &outBytesCnt, &forceConfirmedStEvictedCnt, &oldLcacIfdId, &newLcacIfdId, &oldDynamicsRdfId, &newDynamicsRdfId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
    JPH_ASSERT(30 == newLcacIfdId && 0 == reusedBattle->ifdBuffer.StFrameId && 171 == reusedBattle->ifdBuffer.Cnt && 0 < outBytesCnt && 0 == forceConfirmedStEvictedCnt);
    downsyncSnapshotHolder->ParseFromArray(downsyncSnapshotByteBuffer, outBytesCnt);
    JPH_ASSERT(0 == downsyncSnapshotHolder->st_ifd_id() && 31 == downsyncSnapshotHolder->ifd_batch_size()); // [0, 30]

    auto reqA3 = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
    reqA3->set_join_index(1);
    auto a3 = reqA3->mutable_upsync_snapshot();
    a3->set_st_ifd_id(31);
    for (int ifdId = 31; ifdId <= 70; ifdId++) {
        a3->add_cmd_list(128);
    }
    outBytesCnt = pbBufferSizeLimit;
    memset(downsyncSnapshotByteBuffer, 0, sizeof(downsyncSnapshotByteBuffer));
    reusedBattle->OnUpsyncSnapshotReceived(reqA3->join_index(), reqA3->upsync_snapshot(), false, true, downsyncSnapshotByteBuffer, &outBytesCnt, &forceConfirmedStEvictedCnt, &oldLcacIfdId, &newLcacIfdId, &oldDynamicsRdfId, &newDynamicsRdfId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
    JPH_ASSERT(70 == newLcacIfdId && 0 == reusedBattle->ifdBuffer.StFrameId && 171 == reusedBattle->ifdBuffer.Cnt && 0 < outBytesCnt && 0 == forceConfirmedStEvictedCnt);
    downsyncSnapshotHolder->ParseFromArray(downsyncSnapshotByteBuffer, outBytesCnt);
    JPH_ASSERT(31 == downsyncSnapshotHolder->st_ifd_id() && 40 == downsyncSnapshotHolder->ifd_batch_size() && !downsyncSnapshotHolder->has_ref_rdf()); // [31, 70]

    auto reqB2 = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
    reqB2->set_join_index(2);
    auto b2 = reqB2->mutable_upsync_snapshot();
    b2->set_st_ifd_id(101);
    for (int ifdId = 101; ifdId <= 200; ifdId++) {
        b2->add_cmd_list(0);
    }
    outBytesCnt = pbBufferSizeLimit;
    memset(downsyncSnapshotByteBuffer, 0, sizeof(downsyncSnapshotByteBuffer));
    reusedBattle->OnUpsyncSnapshotReceived(reqB2->join_index(), reqB2->upsync_snapshot(), true, false, downsyncSnapshotByteBuffer, &outBytesCnt, &forceConfirmedStEvictedCnt, &oldLcacIfdId, &newLcacIfdId, &oldDynamicsRdfId, &newDynamicsRdfId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
    JPH_ASSERT(70 == newLcacIfdId && 0 == reusedBattle->ifdBuffer.StFrameId && 201 == reusedBattle->ifdBuffer.Cnt && 0 == outBytesCnt && 0 == forceConfirmedStEvictedCnt);

    outBytesCnt = pbBufferSizeLimit;
    memset(downsyncSnapshotByteBuffer, 0, sizeof(downsyncSnapshotByteBuffer));
    bool t = reusedBattle->MoveForwardLcacIfdIdAndStep(true, &oldLcacIfdId, &newLcacIfdId, &oldDynamicsRdfId, &newDynamicsRdfId, downsyncSnapshotByteBuffer, &outBytesCnt);
    JPH_ASSERT(true == t);
    JPH_ASSERT(pbBufferSizeLimit != outBytesCnt);
    JPH_ASSERT(0 < newDynamicsRdfId);

    std::cout << "Passed TestCase6" << std::endl;
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
    primitiveConstsFin.close();

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
        200, 0,
        200, 1000,
        100, 1000,
        100, 0
    };

    std::vector<std::vector<float>> hulls = {hull1, hull2, hull3};
    JPH_Init(10*1024*1024);
    std::cout << "Initiated" << std::endl;
    
    RegisterDebugCallback(DebugLogCb);

    BackendBattle* battle = static_cast<BackendBattle*>(BACKEND_CreateBattle(2));
    std::cout << "Created battle = " << battle << std::endl;

    auto startRdf = mockStartRdf();
    WsReq* initializerMapData = google::protobuf::Arena::Create<WsReq>(&pbTestCaseDataAllocator);
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
    runTestCase1(battle, initializerMapData);
    APP_ClearBattle(battle);
    runTestCase2(battle, initializerMapData);
    APP_ClearBattle(battle);
    runTestCase3(battle, initializerMapData);
    APP_ClearBattle(battle);
    runTestCase4(battle, initializerMapData);
    APP_ClearBattle(battle);
    runTestCase5(battle, initializerMapData);
    APP_ClearBattle(battle);
    runTestCase6(battle, initializerMapData);
    APP_ClearBattle(battle);

    pbTestCaseDataAllocator.Reset();

    // clean up
    // [REMINDER] "startRdf" will be automatically deallocated by the destructor of "wsReq"
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
