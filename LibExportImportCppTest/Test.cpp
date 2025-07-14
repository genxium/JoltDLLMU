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

using namespace jtshared;
using namespace std::chrono;
using namespace std::filesystem;

const uint32_t SPECIES_BLADEGIRL = 1;
const uint32_t SPECIES_BOUNTYHUNTER = 7;

RenderFrame* mockStartRdf() {
    const int roomCapacity = 2;
    auto startRdf = BaseBattle::NewPreallocatedRdf(roomCapacity, 8, 128);
    startRdf->set_id(globalPrimitiveConsts->starting_render_frame_id());
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

const int pbBufferSizeLimit = (1 << 14);
char pbByteBuffer[pbBufferSizeLimit];
char rdfFetchBuffer[pbBufferSizeLimit];

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

void DebugLogCb(const char* message, int color, int size) {
    std::cout << message << std::endl;
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

    auto startRdf = mockStartRdf();

    WsReq wsReq;
    for (auto hull : hulls) {
        SerializableConvexPolygon* srcPolygon = wsReq.add_serialized_barrier_polygons();
        for (auto xOrY : hull) {
            srcPolygon->add_points(xOrY);
        }
    }
    wsReq.set_allocated_self_parsed_rdf(startRdf);

    memset(pbByteBuffer, 0, sizeof(pbByteBuffer));
    long byteSize = wsReq.ByteSizeLong();
    wsReq.SerializeToArray(pbByteBuffer, byteSize);
    int selfJoinIndex = 1;
    FrontendBattle* battle = static_cast<FrontendBattle*>(FRONTEND_CreateBattle(pbByteBuffer, (int)byteSize, false, selfJoinIndex));
    std::cout << "Created battle = " << battle << std::endl;
    
    jtshared::RenderFrame outRdf;
    std::string outStr;
    int timerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 1024;
    int printIntervalRdfCnt = (1 << 4);
    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    auto nowMillis = duration_cast<milliseconds>(
        system_clock::now().time_since_epoch()
    );
    uint32_t inSingleJoinIndex = 1;
    while (loopRdfCnt > timerRdfId) {
        auto it = testCmds1.lower_bound(timerRdfId);
        if (it == testCmds1.end()) {
            --it;
        }
        uint64_t inSingleInput = it->second;
        bool cmdInjected = FRONTEND_UpsertSelfCmd(battle, inSingleInput);
        if (!cmdInjected) {
            std::cerr << "Failed to inject cmd for timerRdfId=" << timerRdfId << ", inSingleInput=" << inSingleInput << std::endl;
            exit(1);
        }
        bool stepped = FRONTEND_Step(battle, timerRdfId, timerRdfId + 1, false);
        timerRdfId++;
        memset(rdfFetchBuffer, 0, sizeof(rdfFetchBuffer));
        long outBytesCnt = pbBufferSizeLimit;
        APP_GetRdf(battle, timerRdfId, rdfFetchBuffer, &outBytesCnt);
        outRdf.ParseFromArray(rdfFetchBuffer, outBytesCnt);
        if (0 < timerRdfId && 0 == (timerRdfId & printIntervalRdfCntMinus1)) {
            auto firstPlayerChd = outRdf.players_arr(0);
            outStr.clear();
            google::protobuf::util::Status status = google::protobuf::util::MessageToJsonString(firstPlayerChd, &outStr);
            if (status.ok()) {
                auto newNowMillis = duration_cast<milliseconds>(
                    system_clock::now().time_since_epoch()
                );
                auto elapsed = newNowMillis - nowMillis;
                std::cout << "Elapsed=" << elapsed.count() << "ms/rdfCnt=" << printIntervalRdfCnt << ", @timerRdfId = " << timerRdfId << ", now firstPlayerChd = \n" << outStr << std::endl;
                nowMillis = newNowMillis;
            } else {
                std::cerr << "Step result = " << stepped << " at timerRdfId = " << timerRdfId << ", error converting firstPlayerChd to JSON:" << status.ToString() << std::endl;
            }
        }
    }
    
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
