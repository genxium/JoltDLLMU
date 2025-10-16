#include "joltc_export.h" // imports the "JOLTC_EXPORT" macro for "serializable_data.pb.h"
#include "serializable_data.pb.h"
#include "joltc_api.h" 
#include "PbConsts.h"
#include "CppOnlyConsts.h"
#include "DebugLog.h"

#include <Jolt/Jolt.h> // imports the "JPH_EXPORT" macro for classes under namespace JPH
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

const uint32_t SPECIES_BLADEGIRL = 1;
const uint32_t SPECIES_BOUNTYHUNTER = 7;

const int pbBufferSizeLimit = (1 << 14);
char pbByteBuffer[pbBufferSizeLimit];
char rdfFetchBuffer[pbBufferSizeLimit];
char upsyncSnapshotBuffer[pbBufferSizeLimit];
const char* const selfPlayerId = "foobar";
int selfCmdAuthKey = 123456;

RenderFrame* mockStartRdf() {
    const int roomCapacity = 2;
    auto startRdf = BaseBattle::NewPreallocatedRdf(roomCapacity, 8, 128);
    startRdf->set_id(globalPrimitiveConsts->starting_render_frame_id());
    int pickableLocalId = 1;
    int npcLocalId = 1;
    int bulletLocalId = 1;

    auto characterConfigs = globalConfigConsts->character_configs();

    auto playerCh1 = startRdf->mutable_players_arr(0);
    auto ch1 = playerCh1->mutable_chd();
    auto ch1Species = SPECIES_BOUNTYHUNTER;
    auto cc1 = characterConfigs[ch1Species];
    ch1->set_x(-85);
    ch1->set_y(200);
    ch1->set_speed(cc1.speed());
    ch1->set_ch_state(CharacterState::InAirIdle1NoJump);
    ch1->set_frames_to_recover(0);
    ch1->set_dir_x(2);
    ch1->set_dir_y(0);
    ch1->set_vel_x(0);
    ch1->set_vel_y(0);
    ch1->set_hp(cc1.hp());
    ch1->set_species_id(ch1Species);
    ch1->set_bullet_team_id(1);
    playerCh1->set_join_index(1);
    playerCh1->set_revival_x(ch1->x());
    playerCh1->set_revival_y(ch1->y());

    auto playerCh2 = startRdf->mutable_players_arr(1);
    auto ch2 = playerCh2->mutable_chd();
    auto ch2Species = SPECIES_BLADEGIRL;
    auto cc2 = characterConfigs[ch2Species];
    ch2->set_x(+90);
    ch2->set_y(300);
    ch2->set_speed(cc2.speed());
    ch2->set_ch_state(CharacterState::InAirIdle1NoJump);
    ch2->set_frames_to_recover(0);
    ch2->set_dir_x(-2);
    ch2->set_dir_y(0);
    ch2->set_vel_x(0);
    ch2->set_vel_y(0);
    ch2->set_hp(cc2.hp());
    ch2->set_species_id(ch2Species);
    ch1->set_bullet_team_id(2);
    playerCh2->set_join_index(2);
    playerCh2->set_revival_x(ch2->x());
    playerCh2->set_revival_y(ch2->y());

    startRdf->set_npc_id_counter(npcLocalId);
    startRdf->set_bullet_id_counter(bulletLocalId);
    startRdf->set_pickable_id_counter(pickableLocalId);

    return startRdf;
}

RenderFrame* mockRefRdf(int refRdfId) {
    const int roomCapacity = 2;
    auto refRdf = BaseBattle::NewPreallocatedRdf(roomCapacity, 8, 128);
    refRdf->set_id(refRdfId);
    int pickableLocalId = 1;
    int npcLocalId = 1;
    int bulletLocalId = 1;

    auto playerCh1 = refRdf->mutable_players_arr(0);
    auto ch1 = playerCh1->mutable_chd();
    ch1->set_x(-93);
    ch1->set_y(100);
    ch1->set_speed(10);
    ch1->set_ch_state(CharacterState::InAirIdle1NoJump);
    ch1->set_frames_to_recover(0);
    ch1->set_dir_x(-2);
    ch1->set_dir_y(0);
    ch1->set_vel_x(0);
    ch1->set_vel_y(0);
    ch1->set_hp(100);
    ch1->set_species_id(SPECIES_BLADEGIRL);
    playerCh1->set_join_index(1);
    playerCh1->set_revival_x(ch1->x());
    playerCh1->set_revival_y(ch1->y());

    auto playerCh2 = refRdf->mutable_players_arr(1);
    auto ch2 = playerCh2->mutable_chd();
    ch2->set_x(50);
    ch2->set_y(100);
    ch2->set_speed(10);
    ch2->set_ch_state(CharacterState::InAirIdle1NoJump);
    ch2->set_frames_to_recover(0);
    ch2->set_dir_x(+2);
    ch2->set_dir_y(0);
    ch2->set_vel_x(0);
    ch2->set_vel_y(0);
    ch2->set_hp(100);
    ch2->set_species_id(SPECIES_BOUNTYHUNTER);
    playerCh2->set_join_index(2);
    playerCh2->set_revival_x(ch2->x());
    playerCh2->set_revival_y(ch2->y());

    refRdf->set_npc_id_counter(npcLocalId);
    refRdf->set_bullet_id_counter(bulletLocalId);
    refRdf->set_pickable_id_counter(pickableLocalId);

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
    {231, 16},
    {251, 0},
    {252, 16},
    {253, 16},
    {260, 0},
    {699, 0},
    {700, 20},
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

std::map<int, uint64_t> testCmds2 = {
    {0, 3},
    {50, 3},
    {60, 0},
    {64, 32},
    {65, 3},
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
    {760, 4},
    {780, 20},
    {781, 4},
    {820, 20},
    {821, 4},
    {910, 4},
    {1100, 4},
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
    {256, 0}
};

std::map<int, uint64_t> testCmds9 = {
    {0, 3},
    {59, 3},
    {60, 0},
    {61, 4},
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

void initTest8Data() {
   
}

void initTest9Data() {
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

void initTest11Data() {
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

std::string outStr;
std::string player1OutStr, player2OutStr;
std::string referencePlayer1OutStr, referencePlayer2OutStr;
bool runTestCase1(FrontendBattle* reusedBattle, const WsReq* initializerMapData, int inSingleJoinIndex) {
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);
    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 1024;
    int printIntervalRdfCnt = (1 << 4);
    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    jtshared::RenderFrame* outRdf = google::protobuf::Arena::Create<RenderFrame>(&pbTestCaseDataAllocator);
    int newLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    int newChaserRdfId = 0, newReferenceBattleChaserRdfId = 0;
    while (loopRdfCnt > outerTimerRdfId) {
        // Handling TCP packets first, and then UDP packets, the same as C# side behaviour.
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
        auto& p1 = outerTimerRdf->players_arr(0);
        auto& p1Chd = p1.chd();
        if (20 == outerTimerRdfId) {
            JPH_ASSERT(CharacterState::InAirIdle1NoJump == p1Chd.ch_state());
        } else if (21 == outerTimerRdfId) {
            JPH_ASSERT(CharacterState::Idle1 == p1Chd.ch_state());
        } else if (62 == outerTimerRdfId) {
            JPH_ASSERT(CharacterState::Idle1 == p1Chd.ch_state());
        } else if (63 <= outerTimerRdfId && outerTimerRdfId <= 81) {
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
        } else if (82 == outerTimerRdfId) {
            JPH_ASSERT(CharacterState::Idle1 == p1Chd.ch_state());
        }
        outerTimerRdfId++;
    }

    std::cout << "Passed TestCase1\n" << std::endl;
    reusedBattle->Clear();   
    return true;
}

bool runTestCase2(FrontendBattle* reusedBattle, const WsReq* initializerMapData, int inSingleJoinIndex) {
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);
    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 1536;
    int printIntervalRdfCnt = (1 << 30);
    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    int newLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    int newChaserRdfId = 0, newReferenceBattleChaserRdfId = 0;
    jtshared::RenderFrame* outRdf = google::protobuf::Arena::Create<RenderFrame>(&pbTestCaseDataAllocator);
    while (loopRdfCnt > outerTimerRdfId) {
        // Handling TCP packets first, and then UDP packets, the same as C# side behaviour.
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
                srvDownsyncSnapshot->release_ref_rdf();
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
        auto& p1 = outerTimerRdf->players_arr(0);
        auto& p1Chd = p1.chd();
        /*
        if (60 >= outerTimerRdfId) {
            std::cout << "TestCase2/outerTimerRdfId=" << outerTimerRdfId << ", p1Chd chState=" << p1Chd.ch_state() << ", framesInChState=" << p1Chd.frames_in_ch_state() << ", pos=(" << p1Chd.x() << "," << p1Chd.y() << "," << p1Chd.z() << "), vel=(" << p1Chd.vel_x() << "," << p1Chd.vel_y() << "," << p1Chd.vel_z() << ")" << std::endl;
        }
        */
        if (20 == outerTimerRdfId) {
            JPH_ASSERT(CharacterState::InAirIdle1NoJump == p1Chd.ch_state());
        } else if (21 == outerTimerRdfId) {
            JPH_ASSERT(CharacterState::Walking == p1Chd.ch_state());
        } else if (62 == outerTimerRdfId) {
            JPH_ASSERT(CharacterState::Walking == p1Chd.ch_state());
        } else if (63 <= outerTimerRdfId && outerTimerRdfId <= 81) {
            JPH_ASSERT(CharacterState::WalkingAtk1 == p1Chd.ch_state());
            int p1ExpectedFramesInChState = outerTimerRdfId - 63;
            const Skill* skill = nullptr;
            const BulletConfig* bulletConfig = nullptr;
            uint32_t p1ChdActiveSkillId = p1Chd.active_skill_id();
            int p1ChdActiveSkillHit = p1Chd.active_skill_hit();
            BaseBattle::FindBulletConfig(p1ChdActiveSkillId, p1ChdActiveSkillHit, skill, bulletConfig);
            JPH_ASSERT(nullptr != skill && nullptr != bulletConfig);
        } else if (82 == outerTimerRdfId) {
            JPH_ASSERT(CharacterState::Walking == p1Chd.ch_state());
        }
        outerTimerRdfId++;
    }

    std::cout << "Passed TestCase2\n" << std::endl;
    reusedBattle->Clear();   
    return true;
}

bool runTestCase3(FrontendBattle* reusedBattle, const WsReq* initializerMapData, int inSingleJoinIndex) {
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
        // Handling TCP packets first, and then UDP packets, the same as C# side behaviour.
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
                srvDownsyncSnapshot->release_ref_rdf();
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

    std::cout << "Passed TestCase3\n" << std::endl;
    reusedBattle->Clear();   
    return true;
}

bool runTestCase4(FrontendBattle* reusedBattle, const WsReq* initializerMapData, int inSingleJoinIndex) {
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
                srvDownsyncSnapshot->release_ref_rdf();
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

bool runTestCase5(FrontendBattle* reusedBattle, const WsReq* initializerMapData, int inSingleJoinIndex) {
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
                srvDownsyncSnapshot->release_ref_rdf();
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

    std::cout << "Passed TestCase5\n" << std::endl;
    reusedBattle->Clear();   
    return true;
}

bool runTestCase6(FrontendBattle* reusedBattle, const WsReq* initializerMapData, int inSingleJoinIndex) {
    
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
                auto referencedP2 = referencedRdf->players_arr(1);

                auto challengingRdf = reusedBattle->rdfBuffer.GetByFrameId(recRdfId);
                auto challengingP2 = challengingRdf->players_arr(1);

                BaseBattle::AssertNearlySame(referencedP2, challengingP2);
            }
        } else if (320 == outerTimerRdfId) {
            int lastToBeConsistentRdfId = BaseBattle::ConvertToLastUsedRenderFrameId(59) + 1;  

            auto referencedRdf = referenceBattle->rdfBuffer.GetByFrameId(lastToBeConsistentRdfId);
            auto referencedP2 = referencedRdf->players_arr(1);

            auto challengingRdf = reusedBattle->rdfBuffer.GetByFrameId(lastToBeConsistentRdfId);
            auto challengingP2 = challengingRdf->players_arr(1);

            BaseBattle::AssertNearlySame(referencedP2, challengingP2);
        } else if (330 == outerTimerRdfId) {
            int lastToBeConsistentRdfId = BaseBattle::ConvertToLastUsedRenderFrameId(60) + 1;
            for (int tRdfId = globalPrimitiveConsts->starting_render_frame_id(); tRdfId <= lastToBeConsistentRdfId; tRdfId++) {
                auto referencedRdf = referenceBattle->rdfBuffer.GetByFrameId(tRdfId);
                auto referencedP2 = referencedRdf->players_arr(1);

                auto challengingRdf = reusedBattle->rdfBuffer.GetByFrameId(tRdfId);
                auto challengingP2 = challengingRdf->players_arr(1);

                BaseBattle::AssertNearlySame(referencedP2, challengingP2);
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

    std::cout << "Passed TestCase6: Rollback-chasing\n" << std::endl;
    reusedBattle->Clear();   
    APP_DestroyBattle(referenceBattle);
    return true;
}

bool runTestCase7(FrontendBattle* reusedBattle, const WsReq* initializerMapData, int inSingleJoinIndex) {
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
                auto referencedP2 = referencedRdf->players_arr(1);

                auto challengingRdf = reusedBattle->rdfBuffer.GetByFrameId(tRdfId);
                auto challengingP2 = challengingRdf->players_arr(1);

                BaseBattle::AssertNearlySame(referencedP2, challengingP2);
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

    std::cout << "Passed TestCase7: Rollback-chasing\n" << std::endl;
    reusedBattle->Clear();   
    APP_DestroyBattle(referenceBattle);
    return true;
}

bool runTestCase8(FrontendBattle* reusedBattle, const WsReq* initializerMapData, int inSingleJoinIndex) {
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);
    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 1024;
    int printIntervalRdfCnt = (1 << 4);
    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    jtshared::RenderFrame* outRdf = google::protobuf::Arena::Create<RenderFrame>(&pbTestCaseDataAllocator);
    int newLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    int newChaserRdfId = 0, newReferenceBattleChaserRdfId = 0;
    while (loopRdfCnt > outerTimerRdfId) {
        // Handling TCP packets first, and then UDP packets, the same as C# side behaviour.

        uint64_t inSingleInput = getSelfCmdByRdfId(testCmds8, outerTimerRdfId);
        bool cmdInjected = FRONTEND_UpsertSelfCmd(reusedBattle, inSingleInput, &newChaserRdfId);
        if (!cmdInjected) {
            std::cerr << "Failed to inject cmd for outerTimerRdfId=" << outerTimerRdfId << ", inSingleInput=" << inSingleInput << std::endl;
            exit(1);
        }
        FRONTEND_Step(reusedBattle);
        auto outerTimerRdf = reusedBattle->rdfBuffer.GetByFrameId(outerTimerRdfId);
        auto& p1 = outerTimerRdf->players_arr(0);
        auto& p1Chd = p1.chd();
       
        if (20 == outerTimerRdfId) {
            JPH_ASSERT(CharacterState::InAirIdle1NoJump == p1Chd.ch_state());
        } else if (21 == outerTimerRdfId) {
            JPH_ASSERT(CharacterState::Idle1 == p1Chd.ch_state());
        } else if (62 == outerTimerRdfId) {
            JPH_ASSERT(CharacterState::Idle1 == p1Chd.ch_state());
        } else if (63 <= outerTimerRdfId && outerTimerRdfId <= 81) {
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
        } else if (82 == outerTimerRdfId) {
            JPH_ASSERT(CharacterState::Walking == p1Chd.ch_state());
        }
        outerTimerRdfId++;
    }

    std::cout << "Passed TestCase8\n" << std::endl;
    reusedBattle->Clear();
    return true;
}

bool runTestCase9(FrontendBattle* reusedBattle, const WsReq* initializerMapData, int inSingleJoinIndex) {
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex, selfPlayerId, selfCmdAuthKey);
    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 1024;
    int printIntervalRdfCnt = (1 << 4);
    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    jtshared::RenderFrame* outRdf = google::protobuf::Arena::Create<RenderFrame>(&pbTestCaseDataAllocator);
    int newLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;
    int newChaserRdfId = 0, newReferenceBattleChaserRdfId = 0;
    while (loopRdfCnt > outerTimerRdfId) {
        // Handling TCP packets first, and then UDP packets, the same as C# side behaviour.
        
        uint64_t inSingleInput = getSelfCmdByRdfId(testCmds9, outerTimerRdfId);
        bool cmdInjected = FRONTEND_UpsertSelfCmd(reusedBattle, inSingleInput, &newChaserRdfId);
        if (!cmdInjected) {
            std::cerr << "Failed to inject cmd for outerTimerRdfId=" << outerTimerRdfId << ", inSingleInput=" << inSingleInput << std::endl;
            exit(1);
        }
        FRONTEND_Step(reusedBattle);
        auto outerTimerRdf = reusedBattle->rdfBuffer.GetByFrameId(outerTimerRdfId);
        auto& p1 = outerTimerRdf->players_arr(0);
        auto& p1Chd = p1.chd();
        if (231 <= outerTimerRdfId && outerTimerRdfId <= 249) {
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
        } else if (250 == outerTimerRdfId) {
            JPH_ASSERT(CharacterState::Idle1 == p1Chd.ch_state());
        }
        outerTimerRdfId++;
    }

    std::cout << "Passed TestCase9\n" << std::endl;
    reusedBattle->Clear();
    return true;
}

bool runTestCase10(FrontendBattle* reusedBattle, const WsReq* initializerMapData, int inSingleJoinIndex) {
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
        // Handling TCP packets first, and then UDP packets, the same as C# side behaviour.
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
                srvDownsyncSnapshot->release_ref_rdf();
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
        auto& p1 = outerTimerRdf->players_arr(0);
        auto& p1Chd = p1.chd();

        outerTimerRdfId++;
    }

    FrameRingBuffer<FrameLog>& frameLogBuffer = reusedBattle->frameLogBuffer;
    FrameLog* frameLogToTest1 = frameLogBuffer.GetByFrameId(11);
    JPH_ASSERT(3 == frameLogToTest1->used_ifd_confirmed_list());
    FrameLog* frameLogToTest2 = frameLogBuffer.GetByFrameId(15);
    JPH_ASSERT(0 == frameLogToTest2->used_ifd_confirmed_list());

    std::cout << "Passed TestCase10\n" << std::endl;
    reusedBattle->Clear();   
    return true;
}

bool runTestCase11(FrontendBattle* reusedBattle, const WsReq* initializerMapData, int inSingleJoinIndex) {
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
        bool shouldPrint = false;
        if (incomingUpsyncSnapshotReqs11Intime.count(outerTimerRdfId)) {
            auto req = incomingUpsyncSnapshotReqs11Intime[outerTimerRdfId];
            auto peerUpsyncSnapshot = req->upsync_snapshot();
            referenceBattle->OnUpsyncSnapshotReceived(req->join_index(), peerUpsyncSnapshot, &newReferenceBattleChaserRdfId, &referenceBattleMaxPlayerInputFrontId, &referenceBattleMinPlayerInputFrontId);
            shouldPrint = true;
        }

        if (doCompareWithRollback) {
            if (incomingUpsyncSnapshotReqs11Rollback.count(outerTimerRdfId)) {
                auto req = incomingUpsyncSnapshotReqs11Rollback[outerTimerRdfId];
                auto peerUpsyncSnapshot = req->upsync_snapshot();
                reusedBattle->OnUpsyncSnapshotReceived(req->join_index(), peerUpsyncSnapshot, &newChaserRdfId, &maxPlayerInputFrontId, &minPlayerInputFrontId);
                shouldPrint = true;
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

        if (130 <= outerTimerRdfId && outerTimerRdfId <= 133) {
            auto referencedRdf = referenceBattle->rdfBuffer.GetByFrameId(outerTimerRdfId);
            auto referencedP1 = referencedRdf->players_arr(0);
            auto p1Chd = referencedP1.chd();
            auto referencedP2 = referencedRdf->players_arr(1);
            auto p2Chd = referencedP2.chd();

            //std::cout << "TestCase11/outerTimerRdfId=" << outerTimerRdfId << "\n\tp1Chd chState=" << p1Chd.ch_state() << ", framesInChState=" << p1Chd.frames_in_ch_state() << ", dir=(" << p1Chd.dir_x() << ", " << p1Chd.dir_y() << "), pos=(" << p1Chd.x() << ", " << p1Chd.y() << ", " << p1Chd.z() << "), vel=(" << p1Chd.vel_x() << ", " << p1Chd.vel_y() << ", " << p1Chd.vel_z() << ")" << std::endl;

            std::cout << "TestCase11/outerTimerRdfId=" << outerTimerRdfId << "\n\tp1Chd chState=" << p1Chd.ch_state() << ", framesInChState=" << p1Chd.frames_in_ch_state() << ", dir=(" << p1Chd.dir_x() << ", " << p1Chd.dir_y() << "), pos=(" << p1Chd.x() << ", " << p1Chd.y() << ", " << p1Chd.z() << "), vel=(" << p1Chd.vel_x() << ", " << p1Chd.vel_y() << ", " << p1Chd.vel_z() << ")\n\tp2Chd chState=" << p2Chd.ch_state() << ", framesInChState=" << p2Chd.frames_in_ch_state() << ", dir=(" << p2Chd.dir_x() << ", " << p2Chd.dir_y() << "), pos=(" << p2Chd.x() << ", " << p2Chd.y() << ", " << p2Chd.z() << "), vel=(" << p2Chd.vel_x() << ", " << p2Chd.vel_y() << ", " << p2Chd.vel_z() << ")" << std::endl;
        }

        if (doCompareWithRollback) {
            bool cmdInjected = FRONTEND_UpsertSelfCmd(reusedBattle, inSingleInput, &newChaserRdfId);
            if (!cmdInjected) {
                std::cerr << "Failed to inject cmd for outerTimerRdfId=" << outerTimerRdfId << ", inSingleInput=" << inSingleInput << std::endl;
                exit(1);
            }

            int chaserRdfIdEd = outerTimerRdfId;

            FRONTEND_ChaseRolledBackRdfs(reusedBattle, &newChaserRdfId, true);
            FRONTEND_Step(reusedBattle);
            if (280 == outerTimerRdfId) {
                int firstToBeConsistentRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(0);
                int lastToBeConsistentRdfId = BaseBattle::ConvertToLastUsedRenderFrameId(4) + 1;
                for (int recRdfId = firstToBeConsistentRdfId; recRdfId <= lastToBeConsistentRdfId; recRdfId++) {
                    auto referencedRdf = referenceBattle->rdfBuffer.GetByFrameId(recRdfId);
                    auto referencedP1 = referencedRdf->players_arr(0);
                    auto referencedP2 = referencedRdf->players_arr(1);

                    auto challengingRdf = reusedBattle->rdfBuffer.GetByFrameId(recRdfId);
                    auto challengingP1 = challengingRdf->players_arr(0);
                    auto challengingP2 = challengingRdf->players_arr(1);

                    BaseBattle::AssertNearlySame(referencedP1, challengingP1);
                    BaseBattle::AssertNearlySame(referencedP2, challengingP2);
                }
            } else if (400 == outerTimerRdfId) {
                int firstToBeConsistentRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(4);
                int lastToBeConsistentRdfId = BaseBattle::ConvertToLastUsedRenderFrameId(32) + 1;
                for (int recRdfId = firstToBeConsistentRdfId; recRdfId <= lastToBeConsistentRdfId; recRdfId++) {
                    auto referencedRdf = referenceBattle->rdfBuffer.GetByFrameId(recRdfId);
                    auto referencedP1 = referencedRdf->players_arr(0);
                    auto referencedP2 = referencedRdf->players_arr(1);

                    auto challengingRdf = reusedBattle->rdfBuffer.GetByFrameId(recRdfId);
                    auto challengingP1 = challengingRdf->players_arr(0);
                    auto challengingP2 = challengingRdf->players_arr(1);

                    BaseBattle::AssertNearlySame(referencedP1, challengingP1);
                    BaseBattle::AssertNearlySame(referencedP2, challengingP2);
                }
            } else if (480 == outerTimerRdfId) {
                int firstToBeConsistentRdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(17);
                int lastToBeConsistentRdfId = BaseBattle::ConvertToLastUsedRenderFrameId(60) + 1;
                for (int recRdfId = firstToBeConsistentRdfId; recRdfId <= lastToBeConsistentRdfId; recRdfId++) {
                    auto referencedRdf = referenceBattle->rdfBuffer.GetByFrameId(recRdfId);
                    auto referencedP1 = referencedRdf->players_arr(0);
                    auto referencedP2 = referencedRdf->players_arr(1);

                    auto challengingRdf = reusedBattle->rdfBuffer.GetByFrameId(recRdfId);
                    auto challengingP1 = challengingRdf->players_arr(0);
                    auto challengingP2 = challengingRdf->players_arr(1);

                    BaseBattle::AssertNearlySame(referencedP1, challengingP1);
                    BaseBattle::AssertNearlySame(referencedP2, challengingP2);
                }
            }
        }
        outerTimerRdfId++;
    }

    std::cout << "Passed TestCase11: Rollback-chasing with character soft-pushback\n" << std::endl;
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

    std::vector<std::vector<float>> hulls = {hull1, hull2, hull3};

    JPH_Init(10*1024*1024);
    std::cout << "Initiated" << std::endl;
    
    RegisterDebugCallback(DebugLogCb);

    FrontendBattle* battle = static_cast<FrontendBattle*>(FRONTEND_CreateBattle(512, true));
    std::cout << "Created battle = " << battle << std::endl;

    google::protobuf::Arena pbStarterWsReqAllocator;
    auto startRdf = mockStartRdf();
    WsReq* initializerMapData = google::protobuf::Arena::Create<WsReq>(&pbStarterWsReqAllocator);
    for (auto hull : hulls) {
        SerializableConvexPolygon* srcPolygon = initializerMapData->add_serialized_barrier_polygons();
        for (auto xOrY : hull) {
            srcPolygon->add_points(xOrY);
        }
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

    initTest8Data();
    runTestCase8(battle, initializerMapData, selfJoinIndex);
    pbTestCaseDataAllocator.Reset();

    initTest9Data();
    runTestCase9(battle, initializerMapData, selfJoinIndex);
    pbTestCaseDataAllocator.Reset();

    initTest10Data();
    runTestCase10(battle, initializerMapData, selfJoinIndex);
    pbTestCaseDataAllocator.Reset();
    
    initTest11Data();
    runTestCase11(battle, initializerMapData, selfJoinIndex);
    pbTestCaseDataAllocator.Reset();

    pbStarterWsReqAllocator.Reset();

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
