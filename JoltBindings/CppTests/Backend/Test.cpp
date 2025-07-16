#include "joltc_export.h" // imports the "JOLTC_EXPORT" macro for "serializable_data.pb.h"
#include "serializable_data.pb.h"
#include "joltc_api.h" 
#include "PbConsts.h"
#include "CppOnlyConsts.h"
#include "DebugLog.h"

#include <Jolt/Jolt.h> // imports the "JPH_EXPORT" macro for classes under namespace JPH
#include "BackendBattle.h"
#include <google/protobuf/util/json_util.h>
#include <chrono>
#include <fstream>
#include <filesystem>
#include <google/protobuf/arena.h>
google::protobuf::Arena pbTempAllocator;

using namespace jtshared;
using namespace std::filesystem;

const uint32_t SPECIES_BLADEGIRL = 1;
const uint32_t SPECIES_BOUNTYHUNTER = 7;

const int pbBufferSizeLimit = (1 << 14);
char pbByteBuffer[pbBufferSizeLimit];
char downsyncSnapshotByteBuffer[pbBufferSizeLimit];

RenderFrame* mockStartRdf() {
    const int roomCapacity = 2;
    auto startRdf = BaseBattle::NewPreallocatedRdf(roomCapacity, 8, 128);
    startRdf->set_id(1);
    int pickableLocalId = 1;
    int npcLocalId = 1;
    int bulletLocalId = 1;

    auto playerCh1 = startRdf->mutable_players_arr(0);
    auto ch1 = playerCh1->mutable_chd();
    ch1->set_x(-85);
    ch1->set_y(200);
    ch1->set_speed(10);
    ch1->set_ch_state(CharacterState::InAirIdle1NoJump);
    ch1->set_frames_to_recover(0);
    ch1->set_dir_x(2);
    ch1->set_dir_y(0);
    ch1->set_vel_x(0);
    ch1->set_vel_y(0);
    ch1->set_hp(100);
    ch1->set_species_id(SPECIES_BLADEGIRL);
    playerCh1->set_join_index(1);
    playerCh1->set_revival_x(ch1->x());
    playerCh1->set_revival_y(ch1->y());

    auto playerCh2 = startRdf->mutable_players_arr(1);
    auto ch2 = playerCh2->mutable_chd();
    ch2->set_x(+90);
    ch2->set_y(300);
    ch2->set_speed(10);
    ch2->set_ch_state(CharacterState::InAirIdle1NoJump);
    ch2->set_frames_to_recover(0);
    ch2->set_dir_x(-2);
    ch2->set_dir_y(0);
    ch2->set_vel_x(0);
    ch2->set_vel_y(0);
    ch2->set_hp(100);
    ch2->set_species_id(SPECIES_BOUNTYHUNTER);
    playerCh2->set_join_index(2);
    playerCh2->set_revival_x(ch2->x());
    playerCh2->set_revival_y(ch2->y());

    startRdf->set_npc_id_counter(npcLocalId);
    startRdf->set_bullet_id_counter(bulletLocalId);
    startRdf->set_pickable_id_counter(pickableLocalId);

    return startRdf;
}

void DebugLogCb(const char* message, int color, int size) {
    std::cout << message << std::endl;
}

int forceConfirmedStEvictedCnt = 0;
bool runTestCase1(BackendBattle* reusedBattle, const WsReq* initializerMapData) {
    reusedBattle->ResetStartRdf(initializerMapData);
    DownsyncSnapshot* downsyncSnapshotHolder = google::protobuf::Arena::Create<DownsyncSnapshot>(&pbTempAllocator);
    
    UpsyncSnapshot* a1 = google::protobuf::Arena::Create<UpsyncSnapshot>(&pbTempAllocator);
    a1->set_join_index(1);
    a1->set_st_ifd_id(0);
    for (int ifdId = 0; ifdId <= 30; ifdId++) {
        a1->add_cmd_list(0);
    }

    long outBytesCnt = pbBufferSizeLimit;
    memset(downsyncSnapshotByteBuffer, 0, sizeof(downsyncSnapshotByteBuffer));
    reusedBattle->OnUpsyncSnapshotReceived(a1, false, true, downsyncSnapshotByteBuffer, &outBytesCnt, &forceConfirmedStEvictedCnt);
    JPH_ASSERT(-1 == reusedBattle->lcacIfdId && 0 == reusedBattle->ifdBuffer.StFrameId && 31 == reusedBattle->ifdBuffer.Cnt && 0 == outBytesCnt && 0 == forceConfirmedStEvictedCnt);
    
    UpsyncSnapshot* a2 = google::protobuf::Arena::Create<UpsyncSnapshot>(&pbTempAllocator);
    a2->set_join_index(1);
    a2->set_st_ifd_id(150);
    for (int ifdId = 150; ifdId <= 170; ifdId++) {
        a2->add_cmd_list(16);
    }
    outBytesCnt = pbBufferSizeLimit;
    memset(downsyncSnapshotByteBuffer, 0, sizeof(downsyncSnapshotByteBuffer));
    reusedBattle->OnUpsyncSnapshotReceived(a2, true, false, downsyncSnapshotByteBuffer, &outBytesCnt, &forceConfirmedStEvictedCnt);
    JPH_ASSERT(-1 == reusedBattle->lcacIfdId && 0 == reusedBattle->ifdBuffer.StFrameId && 171 == reusedBattle->ifdBuffer.Cnt && 0 == outBytesCnt && 0 == forceConfirmedStEvictedCnt);

    UpsyncSnapshot* b1 = google::protobuf::Arena::Create<UpsyncSnapshot>(&pbTempAllocator);
    b1->set_join_index(2);
    b1->set_st_ifd_id(0);
    for (int ifdId = 0; ifdId <= 100; ifdId++) {
        b1->add_cmd_list(0);
    }
    outBytesCnt = pbBufferSizeLimit;
    memset(downsyncSnapshotByteBuffer, 0, sizeof(downsyncSnapshotByteBuffer));
    reusedBattle->OnUpsyncSnapshotReceived(b1, true, false, downsyncSnapshotByteBuffer, &outBytesCnt, &forceConfirmedStEvictedCnt);
    JPH_ASSERT(30 == reusedBattle->lcacIfdId && 0 == reusedBattle->ifdBuffer.StFrameId && 171 == reusedBattle->ifdBuffer.Cnt && 0 < outBytesCnt && 0 == forceConfirmedStEvictedCnt);
    downsyncSnapshotHolder->ParseFromArray(downsyncSnapshotByteBuffer, outBytesCnt);
    JPH_ASSERT(0 == downsyncSnapshotHolder->st_ifd_id() && 31 == downsyncSnapshotHolder->ifd_batch_size()); // [0, 30]

    UpsyncSnapshot* a3 = google::protobuf::Arena::Create<UpsyncSnapshot>(&pbTempAllocator);
    a3->set_join_index(1);
    a3->set_st_ifd_id(31);
    for (int ifdId = 31; ifdId <= 70; ifdId++) {
        a3->add_cmd_list(128);
    }
    outBytesCnt = pbBufferSizeLimit;
    memset(downsyncSnapshotByteBuffer, 0, sizeof(downsyncSnapshotByteBuffer));
    reusedBattle->OnUpsyncSnapshotReceived(a3, false, true, downsyncSnapshotByteBuffer, &outBytesCnt, &forceConfirmedStEvictedCnt);
    JPH_ASSERT(70 == reusedBattle->lcacIfdId && 0 == reusedBattle->ifdBuffer.StFrameId && 171 == reusedBattle->ifdBuffer.Cnt && 0 < outBytesCnt && 0 == forceConfirmedStEvictedCnt);
    downsyncSnapshotHolder->ParseFromArray(downsyncSnapshotByteBuffer, outBytesCnt);
    JPH_ASSERT(31 == downsyncSnapshotHolder->st_ifd_id() && 40 == downsyncSnapshotHolder->ifd_batch_size()); // [31, 70]

    UpsyncSnapshot* b2 = google::protobuf::Arena::Create<UpsyncSnapshot>(&pbTempAllocator);
    b2->set_join_index(2);
    b2->set_st_ifd_id(101);
    for (int ifdId = 101; ifdId <= 200; ifdId++) {
        b2->add_cmd_list(0);
    }
    outBytesCnt = pbBufferSizeLimit;
    memset(downsyncSnapshotByteBuffer, 0, sizeof(downsyncSnapshotByteBuffer));
    reusedBattle->OnUpsyncSnapshotReceived(b2, false, true, downsyncSnapshotByteBuffer, &outBytesCnt, &forceConfirmedStEvictedCnt);
    JPH_ASSERT(70 == reusedBattle->lcacIfdId && 0 == reusedBattle->ifdBuffer.StFrameId && 201 == reusedBattle->ifdBuffer.Cnt && 0 == outBytesCnt && 0 == forceConfirmedStEvictedCnt);

    std::cout << "Passed TestCase1" << std::endl;
    return true;
}

bool runTestCase2(BackendBattle* reusedBattle, const WsReq* initializerMapData) {
    reusedBattle->ResetStartRdf(initializerMapData);
    DownsyncSnapshot* downsyncSnapshotHolder = google::protobuf::Arena::Create<DownsyncSnapshot>(&pbTempAllocator);
    
    UpsyncSnapshot* a1 = google::protobuf::Arena::Create<UpsyncSnapshot>(&pbTempAllocator);
    a1->set_join_index(1);
    a1->set_st_ifd_id(0);
    for (int ifdId = 0; ifdId <= 300; ifdId++) {
        a1->add_cmd_list(0);
    }

    long outBytesCnt = pbBufferSizeLimit;
    memset(downsyncSnapshotByteBuffer, 0, sizeof(downsyncSnapshotByteBuffer));
    bool upserted = reusedBattle->OnUpsyncSnapshotReceived(a1, false, true, downsyncSnapshotByteBuffer, &outBytesCnt, &forceConfirmedStEvictedCnt);
    JPH_ASSERT(upserted);
    JPH_ASSERT(-1 == reusedBattle->lcacIfdId && 0 == reusedBattle->ifdBuffer.StFrameId && 301 == reusedBattle->ifdBuffer.Cnt && 0 == outBytesCnt && 0 == forceConfirmedStEvictedCnt);

    UpsyncSnapshot* a2 = google::protobuf::Arena::Create<UpsyncSnapshot>(&pbTempAllocator);
    a2->set_join_index(1);
    a2->set_st_ifd_id(400);
    for (int ifdId = 400; ifdId <= 400; ifdId++) {
        a2->add_cmd_list(16);
    }
    outBytesCnt = pbBufferSizeLimit;
    memset(downsyncSnapshotByteBuffer, 0, sizeof(downsyncSnapshotByteBuffer));
    upserted = reusedBattle->OnUpsyncSnapshotReceived(a2, true, false, downsyncSnapshotByteBuffer, &outBytesCnt, &forceConfirmedStEvictedCnt);
    JPH_ASSERT(upserted);
    JPH_ASSERT(-1 == reusedBattle->lcacIfdId && 0 == reusedBattle->ifdBuffer.StFrameId && 401 == reusedBattle->ifdBuffer.Cnt && 0 == outBytesCnt && 0 == forceConfirmedStEvictedCnt);
    
    UpsyncSnapshot* b1 = google::protobuf::Arena::Create<UpsyncSnapshot>(&pbTempAllocator);
    b1->set_join_index(2);
    b1->set_st_ifd_id(0);
    for (int ifdId = 0; ifdId <= 512; ifdId++) {
        b1->add_cmd_list(0);
    }
    outBytesCnt = pbBufferSizeLimit;
    memset(downsyncSnapshotByteBuffer, 0, sizeof(downsyncSnapshotByteBuffer));
    upserted = reusedBattle->OnUpsyncSnapshotReceived(b1, true, false, downsyncSnapshotByteBuffer, &outBytesCnt, &forceConfirmedStEvictedCnt);
    JPH_ASSERT(upserted);
    JPH_ASSERT(300 == reusedBattle->lcacIfdId && 62 == reusedBattle->ifdBuffer.StFrameId && 513 == reusedBattle->ifdBuffer.EdFrameId && 451 == reusedBattle->ifdBuffer.Cnt && 0 < outBytesCnt);
    JPH_ASSERT(0 == forceConfirmedStEvictedCnt); // i.e. all regularly confirmed and evicted, no force-confirmation occurred
    downsyncSnapshotHolder->ParseFromArray(downsyncSnapshotByteBuffer, outBytesCnt);
    JPH_ASSERT(0 == downsyncSnapshotHolder->st_ifd_id() && 301 == downsyncSnapshotHolder->ifd_batch_size()); // [0, 300]

    UpsyncSnapshot* a3 = google::protobuf::Arena::Create<UpsyncSnapshot>(&pbTempAllocator);
    a3->set_join_index(1);
    a3->set_st_ifd_id(401);
    for (int ifdId = 401; ifdId <= 700; ifdId++) {
        a3->add_cmd_list(16);
    }
    outBytesCnt = pbBufferSizeLimit;
    memset(downsyncSnapshotByteBuffer, 0, sizeof(downsyncSnapshotByteBuffer));
    upserted = reusedBattle->OnUpsyncSnapshotReceived(a3, false, true, downsyncSnapshotByteBuffer, &outBytesCnt, &forceConfirmedStEvictedCnt);
    JPH_ASSERT(upserted);
    JPH_ASSERT(300 == reusedBattle->lcacIfdId && 250 == reusedBattle->ifdBuffer.StFrameId && 701 == reusedBattle->ifdBuffer.EdFrameId && 451 == reusedBattle->ifdBuffer.Cnt && 0 == outBytesCnt && 0 == forceConfirmedStEvictedCnt); // i.e. as "ifdBuffer.StFrameId" hasn't reached "lcacIfdId" yet, there's no new downsync snapshot 

    UpsyncSnapshot* b2 = google::protobuf::Arena::Create<UpsyncSnapshot>(&pbTempAllocator);
    b2->set_join_index(2);
    b2->set_st_ifd_id(800);
    for (int ifdId = 800; ifdId <= 801; ifdId++) {
        b2->add_cmd_list(0);
    }
    outBytesCnt = pbBufferSizeLimit;
    memset(downsyncSnapshotByteBuffer, 0, sizeof(downsyncSnapshotByteBuffer));
    upserted = reusedBattle->OnUpsyncSnapshotReceived(b2, false, true, downsyncSnapshotByteBuffer, &outBytesCnt, &forceConfirmedStEvictedCnt);
    JPH_ASSERT(!upserted);
    JPH_ASSERT(300 == reusedBattle->lcacIfdId && 701 == reusedBattle->ifdBuffer.EdFrameId && 451 == reusedBattle->ifdBuffer.Cnt && 0 == outBytesCnt && 0 == forceConfirmedStEvictedCnt);

    UpsyncSnapshot* a4 = google::protobuf::Arena::Create<UpsyncSnapshot>(&pbTempAllocator);
    a4->set_join_index(1);
    a4->set_st_ifd_id(298);
    for (int ifdId = 298; ifdId <= 420; ifdId++) {
        a4->add_cmd_list(32);
    }
    outBytesCnt = pbBufferSizeLimit;
    memset(downsyncSnapshotByteBuffer, 0, sizeof(downsyncSnapshotByteBuffer));
    upserted = reusedBattle->OnUpsyncSnapshotReceived(a4, false, true, downsyncSnapshotByteBuffer, &outBytesCnt, &forceConfirmedStEvictedCnt);
    JPH_ASSERT(upserted);
    JPH_ASSERT(512 == reusedBattle->lcacIfdId && 250 == reusedBattle->ifdBuffer.StFrameId && 701 == reusedBattle->ifdBuffer.EdFrameId && 451 == reusedBattle->ifdBuffer.Cnt && 0 < outBytesCnt && 0 == forceConfirmedStEvictedCnt);
    downsyncSnapshotHolder->ParseFromArray(downsyncSnapshotByteBuffer, outBytesCnt);
    JPH_ASSERT(301 == downsyncSnapshotHolder->st_ifd_id() && 212 == downsyncSnapshotHolder->ifd_batch_size()); // [301, 512]

    std::cout << "Passed TestCase2" << std::endl;
    return true;
}

bool runTestCase3(BackendBattle* reusedBattle, const WsReq* initializerMapData) {
    reusedBattle->ResetStartRdf(initializerMapData);
    DownsyncSnapshot* downsyncSnapshotHolder = google::protobuf::Arena::Create<DownsyncSnapshot>(&pbTempAllocator);

    UpsyncSnapshot* a1 = google::protobuf::Arena::Create<UpsyncSnapshot>(&pbTempAllocator);
    a1->set_join_index(1);
    a1->set_st_ifd_id(0);
    for (int ifdId = 0; ifdId <= 120; ifdId++) {
        a1->add_cmd_list(0);
    }

    long outBytesCnt = pbBufferSizeLimit;
    memset(downsyncSnapshotByteBuffer, 0, sizeof(downsyncSnapshotByteBuffer));
    bool upserted = reusedBattle->OnUpsyncSnapshotReceived(a1, false, true, downsyncSnapshotByteBuffer, &outBytesCnt, &forceConfirmedStEvictedCnt);
    JPH_ASSERT(upserted);
    JPH_ASSERT(-1 == reusedBattle->lcacIfdId && 0 == reusedBattle->ifdBuffer.StFrameId && 121 == reusedBattle->ifdBuffer.Cnt && 0 == outBytesCnt);
 
    UpsyncSnapshot* b1 = google::protobuf::Arena::Create<UpsyncSnapshot>(&pbTempAllocator);
    b1->set_join_index(2);
    b1->set_st_ifd_id(0);
    for (int ifdId = 0; ifdId <= 120; ifdId++) {
        b1->add_cmd_list(0);
    }
    outBytesCnt = pbBufferSizeLimit;
    memset(downsyncSnapshotByteBuffer, 0, sizeof(downsyncSnapshotByteBuffer));
    upserted = reusedBattle->OnUpsyncSnapshotReceived(b1, true, false, downsyncSnapshotByteBuffer, &outBytesCnt, &forceConfirmedStEvictedCnt);
    JPH_ASSERT(upserted);
    JPH_ASSERT(120 == reusedBattle->lcacIfdId && 0 == reusedBattle->ifdBuffer.StFrameId && 121 == reusedBattle->ifdBuffer.Cnt && 0 < outBytesCnt); 
    downsyncSnapshotHolder->ParseFromArray(downsyncSnapshotByteBuffer, outBytesCnt);
    JPH_ASSERT(121 == downsyncSnapshotHolder->ifd_batch_size());

    UpsyncSnapshot* a2 = google::protobuf::Arena::Create<UpsyncSnapshot>(&pbTempAllocator);
    a2->set_join_index(1);
    a2->set_st_ifd_id(459);
    for (int ifdId = 459; ifdId <= 600; ifdId++) {
        a2->add_cmd_list(16);
    }
    outBytesCnt = pbBufferSizeLimit;
    memset(downsyncSnapshotByteBuffer, 0, sizeof(downsyncSnapshotByteBuffer));
    upserted = reusedBattle->OnUpsyncSnapshotReceived(a2, true, false, downsyncSnapshotByteBuffer, &outBytesCnt, &forceConfirmedStEvictedCnt);
    JPH_ASSERT(upserted);
    JPH_ASSERT(149 == reusedBattle->lcacIfdId && 150 == reusedBattle->ifdBuffer.StFrameId && 451 == reusedBattle->ifdBuffer.Cnt && 0 < outBytesCnt);
    downsyncSnapshotHolder->ParseFromArray(downsyncSnapshotByteBuffer, outBytesCnt);
    JPH_ASSERT(29 == downsyncSnapshotHolder->ifd_batch_size()); // [WARNING] There're 29 ifds, i.e. from 121 to 149 forced out of "ifdBuffer.StFrameId"

    std::cout << "Passed TestCase3" << std::endl;
    return true;
}

bool runTestCase4(BackendBattle* reusedBattle, const WsReq* initializerMapData) {
    reusedBattle->ResetStartRdf(initializerMapData);
    DownsyncSnapshot* downsyncSnapshotHolder = google::protobuf::Arena::Create<DownsyncSnapshot>(&pbTempAllocator);
    long outBytesCnt = pbBufferSizeLimit;

    UpsyncSnapshot* a1 = google::protobuf::Arena::Create<UpsyncSnapshot>(&pbTempAllocator);
    a1->set_join_index(1);
    a1->set_st_ifd_id(459);
    for (int ifdId = 459; ifdId <= 600; ifdId++) {
        a1->add_cmd_list(16);
    }
    memset(downsyncSnapshotByteBuffer, 0, sizeof(downsyncSnapshotByteBuffer));
    bool upserted = reusedBattle->OnUpsyncSnapshotReceived(a1, true, false, downsyncSnapshotByteBuffer, &outBytesCnt, &forceConfirmedStEvictedCnt);
    JPH_ASSERT(upserted);
    JPH_ASSERT(149 == reusedBattle->lcacIfdId && 150 == reusedBattle->ifdBuffer.StFrameId && 601 == reusedBattle->ifdBuffer.EdFrameId && 451 == reusedBattle->ifdBuffer.Cnt && 0 < outBytesCnt);
    downsyncSnapshotHolder->ParseFromArray(downsyncSnapshotByteBuffer, outBytesCnt);
    JPH_ASSERT(150 == forceConfirmedStEvictedCnt); // "gapCnt == 9" && "otherForceConfirmedStEvictedCnt == 141 == 600-459"
    JPH_ASSERT(forceConfirmedStEvictedCnt == downsyncSnapshotHolder->ifd_batch_size() && 0 == downsyncSnapshotHolder->st_ifd_id()); // [0, 149]

    std::cout << "Passed TestCase4" << std::endl;
    return true;
}

bool runTestCase5(BackendBattle* reusedBattle, const WsReq* initializerMapData) {
    reusedBattle->ResetStartRdf(initializerMapData);
    DownsyncSnapshot* downsyncSnapshotHolder = google::protobuf::Arena::Create<DownsyncSnapshot>(&pbTempAllocator);
    long outBytesCnt = pbBufferSizeLimit;

    UpsyncSnapshot* a1 = google::protobuf::Arena::Create<UpsyncSnapshot>(&pbTempAllocator);
    a1->set_join_index(1);
    a1->set_st_ifd_id(0);
    for (int ifdId = 0; ifdId <= 2; ifdId++) {
        a1->add_cmd_list(16);
    }
    outBytesCnt = pbBufferSizeLimit;
    memset(downsyncSnapshotByteBuffer, 0, sizeof(downsyncSnapshotByteBuffer));
    bool upserted = reusedBattle->OnUpsyncSnapshotReceived(a1, true, false, downsyncSnapshotByteBuffer, &outBytesCnt, &forceConfirmedStEvictedCnt);
    JPH_ASSERT(upserted);
    JPH_ASSERT(-1 == reusedBattle->lcacIfdId && 0 == reusedBattle->ifdBuffer.StFrameId && 3 == reusedBattle->ifdBuffer.EdFrameId && 3 == reusedBattle->ifdBuffer.Cnt && 0 == outBytesCnt);

    UpsyncSnapshot* b1 = google::protobuf::Arena::Create<UpsyncSnapshot>(&pbTempAllocator);
    b1->set_join_index(2);
    b1->set_st_ifd_id(0);
    for (int ifdId = 0; ifdId <= 2; ifdId++) {
        b1->add_cmd_list(0);
    }
    outBytesCnt = pbBufferSizeLimit;
    memset(downsyncSnapshotByteBuffer, 0, sizeof(downsyncSnapshotByteBuffer));
    upserted = reusedBattle->OnUpsyncSnapshotReceived(b1, true, false, downsyncSnapshotByteBuffer, &outBytesCnt, &forceConfirmedStEvictedCnt);
    JPH_ASSERT(upserted);
    JPH_ASSERT(2 == reusedBattle->lcacIfdId && 0 == reusedBattle->ifdBuffer.StFrameId && 3 == reusedBattle->ifdBuffer.EdFrameId && 3 == reusedBattle->ifdBuffer.Cnt && 0 < outBytesCnt);
    downsyncSnapshotHolder->ParseFromArray(downsyncSnapshotByteBuffer, outBytesCnt);
    JPH_ASSERT(3 == downsyncSnapshotHolder->ifd_batch_size() && 0 == downsyncSnapshotHolder->st_ifd_id()); // [0, 2]

    UpsyncSnapshot* a2 = google::protobuf::Arena::Create<UpsyncSnapshot>(&pbTempAllocator);
    a2->set_join_index(1);
    a2->set_st_ifd_id(459);
    for (int ifdId = 459; ifdId <= 600; ifdId++) {
        a2->add_cmd_list(16);
    }
    outBytesCnt = pbBufferSizeLimit;
    memset(downsyncSnapshotByteBuffer, 0, sizeof(downsyncSnapshotByteBuffer));
    upserted = reusedBattle->OnUpsyncSnapshotReceived(a2, true, false, downsyncSnapshotByteBuffer, &outBytesCnt, &forceConfirmedStEvictedCnt);
    JPH_ASSERT(upserted);
    JPH_ASSERT(149 == reusedBattle->lcacIfdId && 150 == reusedBattle->ifdBuffer.StFrameId && 601 == reusedBattle->ifdBuffer.EdFrameId && 451 == reusedBattle->ifdBuffer.Cnt && 0 < outBytesCnt);
    downsyncSnapshotHolder->ParseFromArray(downsyncSnapshotByteBuffer, outBytesCnt);
    JPH_ASSERT(147 == forceConfirmedStEvictedCnt); // Minus 3 already confirmed "ifd"s [0, 1, 2] compared with TestCase4
    JPH_ASSERT(forceConfirmedStEvictedCnt == downsyncSnapshotHolder->ifd_batch_size() && 3 == downsyncSnapshotHolder->st_ifd_id()); // [3, 149]

    std::cout << "Passed TestCase5" << std::endl;
    return true;
}

// Program entry point
int main(int argc, char** argv)
{
    std::cout << "Starting" << std::endl;
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

    BackendBattle* battle = static_cast<BackendBattle*>(BACKEND_CreateBattle());
    std::cout << "Created battle = " << battle << std::endl;

    auto startRdf = mockStartRdf();
    WsReq* initializerMapData = google::protobuf::Arena::Create<WsReq>(&pbTempAllocator);
    for (auto hull : hulls) {
        SerializableConvexPolygon* srcPolygon = initializerMapData->add_serialized_barrier_polygons();
        for (auto xOrY : hull) {
            srcPolygon->add_points(xOrY);
        }
    }
    initializerMapData->set_allocated_self_parsed_rdf(startRdf); // "initializerMapData" will own "startRdf" and deallocate it implicitly
    runTestCase1(battle, initializerMapData);
    runTestCase2(battle, initializerMapData);
    runTestCase3(battle, initializerMapData);
    runTestCase4(battle, initializerMapData);
    runTestCase5(battle, initializerMapData);

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
