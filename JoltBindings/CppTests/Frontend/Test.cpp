#include "joltc_export.h" // imports the "JOLTC_EXPORT" macro for "serializable_data.pb.h"
#include "serializable_data.pb.h"
#include "joltc_api.h" 
#include "PbConsts.h"
#include "CppOnlyConsts.h"
#include "DebugLog.h"

#include <Jolt/Jolt.h> // imports the "JPH_EXPORT" macro for classes under namespace JPH
#include "FrontendBattle.h"
#include <google/protobuf/util/json_util.h>
#include <chrono>
#include <fstream>
#include <filesystem>
#include <google/protobuf/arena.h>
#include <google/protobuf/repeated_field.h>
google::protobuf::Arena pbTempAllocator;

using namespace jtshared;
using namespace std::filesystem;

const uint32_t SPECIES_BLADEGIRL = 1;
const uint32_t SPECIES_BOUNTYHUNTER = 7;

const int pbBufferSizeLimit = (1 << 14);
char pbByteBuffer[pbBufferSizeLimit];
char rdfFetchBuffer[pbBufferSizeLimit];

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
    {0, 3},
    {227, 0},
    {228, 16},
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

uint64_t getSelfCmdByRdfId(int rdfId) {
    auto it = testCmds1.lower_bound(rdfId);
    if (it == testCmds1.end()) {
        --it;
    }
    return it->second;
}

uint64_t getSelfCmdByIfdId(int ifdId) {
    int rdfId = BaseBattle::ConvertToFirstUsedRenderFrameId(ifdId);
    return getSelfCmdByRdfId(rdfId);
}

std::unordered_map<int, UpsyncSnapshot*> incomingUpsyncSnapshots1; // key is "timerRdfId"; random order of "UpsyncSnapshot.st_ifd_id", relatively big packet loss 
std::unordered_map<int, DownsyncSnapshot*> incomingDownsyncSnapshots1; // key is "timerRdfId"; non-descending order of "DownsyncSnapshot.st_ifd_id" (see comments on "inputBufferLock" in "joltc_api.h"), relatively small packet loss

std::unordered_map<int, UpsyncSnapshot*> incomingUpsyncSnapshots2;
std::unordered_map<int, DownsyncSnapshot*> incomingDownsyncSnapshots2; 

void initTest1Data() {
    // incomingUpsyncSnapshots1
    {
        int receivedTimerRdfId = 120;
        int receivedEdIfdId = 26;
        int receivedStIfdId = 13;
        UpsyncSnapshot* peerUpsyncSnapshot = google::protobuf::Arena::Create<UpsyncSnapshot>(&pbTempAllocator);
        peerUpsyncSnapshot->set_join_index(2);
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(4);
        }
        incomingUpsyncSnapshots1[receivedTimerRdfId] = peerUpsyncSnapshot;
    }
    {
        int receivedTimerRdfId = 155;
        int receivedEdIfdId = 13;
        int receivedStIfdId = 0;
        UpsyncSnapshot* peerUpsyncSnapshot = google::protobuf::Arena::Create<UpsyncSnapshot>(&pbTempAllocator);
        peerUpsyncSnapshot->set_join_index(2);
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        int mid = 6;
        for (int ifdId = receivedStIfdId; ifdId < mid; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(4);
        }
        for (int ifdId = mid; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(19);
        }
        incomingUpsyncSnapshots1[receivedTimerRdfId] = peerUpsyncSnapshot;
    }
    {
        int receivedTimerRdfId = 500;
        int receivedEdIfdId = 60;
        int receivedStIfdId = 29;
        UpsyncSnapshot* peerUpsyncSnapshot = google::protobuf::Arena::Create<UpsyncSnapshot>(&pbTempAllocator);
        peerUpsyncSnapshot->set_join_index(2);
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        int mid = 44;
        for (int ifdId = receivedStIfdId; ifdId < mid; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(4);
        }
        for (int ifdId = mid; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(20);
        }
        incomingUpsyncSnapshots1[receivedTimerRdfId] = peerUpsyncSnapshot;
    }
    {
        int receivedTimerRdfId = 560;
        int receivedEdIfdId = 29;
        int receivedStIfdId = 26;
        UpsyncSnapshot* peerUpsyncSnapshot = google::protobuf::Arena::Create<UpsyncSnapshot>(&pbTempAllocator);
        peerUpsyncSnapshot->set_join_index(2);
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(2);
        }
        incomingUpsyncSnapshots1[receivedTimerRdfId] = peerUpsyncSnapshot;
    }
    // incomingDownsyncSnapshots1
    {
        int receivedTimerRdfId = 180;
        int receivedEdIfdId = 26;
        int receivedStIfdId = 0;
        DownsyncSnapshot* srvDownsyncSnapshot = google::protobuf::Arena::Create<DownsyncSnapshot>(&pbTempAllocator);
        srvDownsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            InputFrameDownsync* ifdBatch = srvDownsyncSnapshot->add_ifd_batch();
            ifdBatch->add_input_list(getSelfCmdByIfdId(ifdId));
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
        DownsyncSnapshot* srvDownsyncSnapshot = google::protobuf::Arena::Create<DownsyncSnapshot>(&pbTempAllocator);
        srvDownsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            InputFrameDownsync* ifdBatch = srvDownsyncSnapshot->add_ifd_batch();
            ifdBatch->add_input_list(getSelfCmdByIfdId(ifdId));
            ifdBatch->add_input_list(
                29 > ifdId ?
                2
                :
                (44 > ifdId ? 4 : 20)
            );
        }
        incomingDownsyncSnapshots1[receivedTimerRdfId] = srvDownsyncSnapshot;
    }
};

void initTest2Data() {
    // incomingUpsyncSnapshots2
    {
        int receivedTimerRdfId = 120;
        int receivedEdIfdId = 26;
        int receivedStIfdId = 13;
        UpsyncSnapshot* peerUpsyncSnapshot = google::protobuf::Arena::Create<UpsyncSnapshot>(&pbTempAllocator);
        peerUpsyncSnapshot->set_join_index(2);
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(4);
        }
        incomingUpsyncSnapshots2[receivedTimerRdfId] = peerUpsyncSnapshot;
    }
    {
        int receivedTimerRdfId = 155;
        int receivedEdIfdId = 13;
        int receivedStIfdId = 0;
        UpsyncSnapshot* peerUpsyncSnapshot = google::protobuf::Arena::Create<UpsyncSnapshot>(&pbTempAllocator);
        peerUpsyncSnapshot->set_join_index(2);
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        int mid = 6;
        for (int ifdId = receivedStIfdId; ifdId < mid; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(4);
        }
        for (int ifdId = mid; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(19);
        }
        incomingUpsyncSnapshots2[receivedTimerRdfId] = peerUpsyncSnapshot;
    }
    {
        int receivedTimerRdfId = 500;
        int receivedEdIfdId = 60;
        int receivedStIfdId = 29;
        UpsyncSnapshot* peerUpsyncSnapshot = google::protobuf::Arena::Create<UpsyncSnapshot>(&pbTempAllocator);
        peerUpsyncSnapshot->set_join_index(2);
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        int mid = 44;
        for (int ifdId = receivedStIfdId; ifdId < mid; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(4);
        }
        for (int ifdId = mid; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(20);
        }
        incomingUpsyncSnapshots2[receivedTimerRdfId] = peerUpsyncSnapshot;
    }
    {
        int receivedTimerRdfId = 560;
        int receivedEdIfdId = 29;
        int receivedStIfdId = 26;
        UpsyncSnapshot* peerUpsyncSnapshot = google::protobuf::Arena::Create<UpsyncSnapshot>(&pbTempAllocator);
        peerUpsyncSnapshot->set_join_index(2);
        peerUpsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            peerUpsyncSnapshot->add_cmd_list(2);
        }
        incomingUpsyncSnapshots2[receivedTimerRdfId] = peerUpsyncSnapshot;
    }
    // incomingDownsyncSnapshots2
    {
        int receivedTimerRdfId = 180;
        int receivedEdIfdId = 26;
        int receivedStIfdId = 0;
        DownsyncSnapshot* srvDownsyncSnapshot = google::protobuf::Arena::Create<DownsyncSnapshot>(&pbTempAllocator);
        srvDownsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            InputFrameDownsync* ifdBatch = srvDownsyncSnapshot->add_ifd_batch();
            ifdBatch->add_input_list(getSelfCmdByIfdId(ifdId));
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
        DownsyncSnapshot* srvDownsyncSnapshot = google::protobuf::Arena::Create<DownsyncSnapshot>(&pbTempAllocator);
        srvDownsyncSnapshot->set_st_ifd_id(receivedStIfdId);
        for (int ifdId = receivedStIfdId; ifdId < receivedEdIfdId; ifdId++) {
            InputFrameDownsync* ifdBatch = srvDownsyncSnapshot->add_ifd_batch();
            ifdBatch->add_input_list(getSelfCmdByIfdId(ifdId));
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

std::string outStr;
std::string player1OutStr, player2OutStr;
bool runTestCase1(FrontendBattle* reusedBattle, const WsReq* initializerMapData, int inSingleJoinIndex) {
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex);
    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 1024;
    int printIntervalRdfCnt = (1 << 4);
    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    jtshared::RenderFrame* outRdf = google::protobuf::Arena::Create<RenderFrame>(&pbTempAllocator);
    while (loopRdfCnt > outerTimerRdfId) {
        // Handling TCP packets first, and then UDP packets, the same as C# side behaviour.
        if (incomingDownsyncSnapshots1.count(outerTimerRdfId)) {
            DownsyncSnapshot* srvDownsyncSnapshot = incomingDownsyncSnapshots1[outerTimerRdfId];
            int outPostTimerRdfEvictedCnt = 0, outPostTimerRdfDelayedIfdEvictedCnt = 0;
            bool applied = reusedBattle->OnDownsyncSnapshotReceived(srvDownsyncSnapshot, &outPostTimerRdfEvictedCnt, &outPostTimerRdfDelayedIfdEvictedCnt);
            outStr.clear();
            google::protobuf::util::Status status = google::protobuf::util::MessageToJsonString(*srvDownsyncSnapshot, &outStr);
            std::cout << "@outerTimerRdfId = " << outerTimerRdfId << ", applied srvDownsyncSnapshot = " << outStr << std::endl;
            if (0 == srvDownsyncSnapshot->st_ifd_id()) {
                JPH_ASSERT(25 == reusedBattle->lcacIfdId);
            } else if (26 == srvDownsyncSnapshot->st_ifd_id()) {
                JPH_ASSERT(57 == reusedBattle->lcacIfdId);
            }
        }
        if (incomingUpsyncSnapshots1.count(outerTimerRdfId)) {
            UpsyncSnapshot* peerUpsyncSnapshot = incomingUpsyncSnapshots1[outerTimerRdfId]; 
            bool applied = reusedBattle->OnUpsyncSnapshotReceived(peerUpsyncSnapshot);
            outStr.clear();
            google::protobuf::util::Status status = google::protobuf::util::MessageToJsonString(*peerUpsyncSnapshot, &outStr);
            std::cout << "@outerTimerRdfId = " << outerTimerRdfId << ", applied peerUpsyncSnapshot = " << outStr << std::endl;
            if (13 == peerUpsyncSnapshot->st_ifd_id()) {
                JPH_ASSERT(54 == reusedBattle->chaserRdfId);
                JPH_ASSERT(-1 == reusedBattle->lcacIfdId);
            } else if (0 == peerUpsyncSnapshot->st_ifd_id()) {
                JPH_ASSERT(2 == reusedBattle->chaserRdfId);
                JPH_ASSERT(-1 == reusedBattle->lcacIfdId);
            } else if (29 == peerUpsyncSnapshot->st_ifd_id()) {
                JPH_ASSERT(118 == reusedBattle->chaserRdfId);
                JPH_ASSERT(25 == reusedBattle->lcacIfdId);
            } else if (26 == peerUpsyncSnapshot->st_ifd_id()) {
                JPH_ASSERT(106 == reusedBattle->chaserRdfId);
                JPH_ASSERT(25 == reusedBattle->lcacIfdId);
            }
        }
        uint64_t inSingleInput = getSelfCmdByRdfId(outerTimerRdfId);
        bool cmdInjected = FRONTEND_UpsertSelfCmd(reusedBattle, inSingleInput);
        if (!cmdInjected) {
            std::cerr << "Failed to inject cmd for outerTimerRdfId=" << outerTimerRdfId << ", inSingleInput=" << inSingleInput << std::endl;
            exit(1);
        }
        int chaserRdfIdEd = reusedBattle->chaserRdfId + globalPrimitiveConsts->max_chasing_render_frames_per_update();
        if (chaserRdfIdEd > outerTimerRdfId) {
            chaserRdfIdEd = outerTimerRdfId;
        }
        bool chaserStepped = FRONTEND_Step(reusedBattle, reusedBattle->chaserRdfId, chaserRdfIdEd, true);
        bool stepped = FRONTEND_Step(reusedBattle, outerTimerRdfId, outerTimerRdfId + 1, false);
        outerTimerRdfId++;
        memset(rdfFetchBuffer, 0, sizeof(rdfFetchBuffer));
        long outBytesCnt = pbBufferSizeLimit;
        APP_GetRdf(reusedBattle, outerTimerRdfId, rdfFetchBuffer, &outBytesCnt);
        outRdf->ParseFromArray(rdfFetchBuffer, outBytesCnt);
        if (0 < outerTimerRdfId && 0 == (outerTimerRdfId & printIntervalRdfCntMinus1)) {
            player1OutStr.clear();
            google::protobuf::util::Status status1 = google::protobuf::util::MessageToJsonString(outRdf->players_arr(0), &player1OutStr);

            player2OutStr.clear();
            google::protobuf::util::Status status2 = google::protobuf::util::MessageToJsonString(outRdf->players_arr(1), &player2OutStr);

            std::cout << "@outerTimerRdfId = " << outerTimerRdfId << "\nplayer1 = \n" << player1OutStr << "\nplayer2 = \n" << player2OutStr << std::endl;
        }
    }

    std::cout << "Passed TestCase1" << std::endl;
    return true;
}

bool runTestCase2(FrontendBattle* reusedBattle, const WsReq* initializerMapData, int inSingleJoinIndex) {
    reusedBattle->ResetStartRdf(initializerMapData, inSingleJoinIndex);
    int outerTimerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 1536;
    int printIntervalRdfCnt = (1 << 4);
    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    jtshared::RenderFrame* outRdf = google::protobuf::Arena::Create<RenderFrame>(&pbTempAllocator);
    while (loopRdfCnt > outerTimerRdfId) {
        // Handling TCP packets first, and then UDP packets, the same as C# side behaviour.
        if (incomingDownsyncSnapshots2.count(outerTimerRdfId)) {
            DownsyncSnapshot* srvDownsyncSnapshot = incomingDownsyncSnapshots2[outerTimerRdfId];
            int outPostTimerRdfEvictedCnt = 0, outPostTimerRdfDelayedIfdEvictedCnt = 0;
            bool applied = reusedBattle->OnDownsyncSnapshotReceived(srvDownsyncSnapshot, &outPostTimerRdfEvictedCnt, &outPostTimerRdfDelayedIfdEvictedCnt);
            outStr.clear();
            google::protobuf::util::Status status = google::protobuf::util::MessageToJsonString(*srvDownsyncSnapshot, &outStr);
            std::cout << "@outerTimerRdfId = " << outerTimerRdfId << ", applied srvDownsyncSnapshot = " << outStr << std::endl;
            if (0 == srvDownsyncSnapshot->st_ifd_id()) {
                JPH_ASSERT(25 == reusedBattle->lcacIfdId);
            } else if (1100 == srvDownsyncSnapshot->ref_rdf_id()) {
                JPH_ASSERT(271 == reusedBattle->lcacIfdId);
                JPH_ASSERT(18 == outPostTimerRdfEvictedCnt);
                JPH_ASSERT(0 == outPostTimerRdfDelayedIfdEvictedCnt);
                JPH_ASSERT(srvDownsyncSnapshot->ref_rdf_id() == reusedBattle->timerRdfId);
                JPH_ASSERT(srvDownsyncSnapshot->ref_rdf_id() == reusedBattle->chaserRdfId);
                JPH_ASSERT(srvDownsyncSnapshot->ref_rdf_id() == reusedBattle->chaserRdfIdLowerBound);
                outerTimerRdfId = reusedBattle->timerRdfId;
            }
        }
        if (incomingUpsyncSnapshots2.count(outerTimerRdfId)) {
            UpsyncSnapshot* peerUpsyncSnapshot = incomingUpsyncSnapshots2[outerTimerRdfId]; 
            bool applied = reusedBattle->OnUpsyncSnapshotReceived(peerUpsyncSnapshot);
            outStr.clear();
            google::protobuf::util::Status status = google::protobuf::util::MessageToJsonString(*peerUpsyncSnapshot, &outStr);
            std::cout << "@outerTimerRdfId = " << outerTimerRdfId << ", applied peerUpsyncSnapshot = " << outStr << std::endl;
            if (13 == peerUpsyncSnapshot->st_ifd_id()) {
                JPH_ASSERT(54 == reusedBattle->chaserRdfId);
                JPH_ASSERT(-1 == reusedBattle->lcacIfdId);
            } else if (0 == peerUpsyncSnapshot->st_ifd_id()) {
                JPH_ASSERT(2 == reusedBattle->chaserRdfId);
                JPH_ASSERT(-1 == reusedBattle->lcacIfdId);
            } else if (29 == peerUpsyncSnapshot->st_ifd_id()) {
                JPH_ASSERT(118 == reusedBattle->chaserRdfId);
                JPH_ASSERT(25 == reusedBattle->lcacIfdId);
            } else if (26 == peerUpsyncSnapshot->st_ifd_id()) {
                JPH_ASSERT(106 == reusedBattle->chaserRdfId);
                JPH_ASSERT(25 == reusedBattle->lcacIfdId);
            }
        }
        int oldIfdEdFrameId = reusedBattle->ifdBuffer.EdFrameId;
        uint64_t inSingleInput = getSelfCmdByRdfId(outerTimerRdfId);
        bool cmdInjected = FRONTEND_UpsertSelfCmd(reusedBattle, inSingleInput);
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
        int chaserRdfIdEd = reusedBattle->chaserRdfId + globalPrimitiveConsts->max_chasing_render_frames_per_update();
        if (chaserRdfIdEd > outerTimerRdfId) {
            chaserRdfIdEd = outerTimerRdfId;
        }
        bool chaserStepped = FRONTEND_Step(reusedBattle, reusedBattle->chaserRdfId, chaserRdfIdEd, true);
        bool stepped = FRONTEND_Step(reusedBattle, outerTimerRdfId, outerTimerRdfId + 1, false);
        outerTimerRdfId++;
        memset(rdfFetchBuffer, 0, sizeof(rdfFetchBuffer));
        long outBytesCnt = pbBufferSizeLimit;
        APP_GetRdf(reusedBattle, outerTimerRdfId, rdfFetchBuffer, &outBytesCnt);
        outRdf->ParseFromArray(rdfFetchBuffer, outBytesCnt);
        if (0 < outerTimerRdfId && 0 == (outerTimerRdfId & printIntervalRdfCntMinus1)) {
            player1OutStr.clear();
            google::protobuf::util::Status status1 = google::protobuf::util::MessageToJsonString(outRdf->players_arr(0), &player1OutStr);

            player2OutStr.clear();
            google::protobuf::util::Status status2 = google::protobuf::util::MessageToJsonString(outRdf->players_arr(1), &player2OutStr);

            std::cout << "@outerTimerRdfId = " << outerTimerRdfId << "\nplayer1 = \n" << player1OutStr << "\nplayer2 = \n" << player2OutStr << std::endl;
        }
    }

    std::cout << "Passed TestCase2" << std::endl;
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

    FrontendBattle* battle = static_cast<FrontendBattle*>(FRONTEND_CreateBattle(true));
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

    int selfJoinIndex = 1;
    initTest1Data();
    runTestCase1(battle, initializerMapData, selfJoinIndex);
    initTest2Data();
    runTestCase2(battle, initializerMapData, selfJoinIndex);

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
