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

static float defaultThickness = 2.0f;
static float defaultHalfThickness = defaultThickness * 0.5f;

// Deliberately NOT using enum for "room states" to make use of "C# CompareAndExchange" 
const int ROOM_ID_NONE = 0;

const uint32_t SPECIES_NONE_CH = 0;
const uint32_t SPECIES_BLADEGIRL = 1;
const uint32_t SPECIES_BOUNTYHUNTER = 7;

void NewBullet(Bullet* single, int bulletLocalId, int originatedRenderFrameId, int teamId, BulletState blState, int framesInBlState) {
    single->set_bl_state(blState);
    single->set_frames_in_bl_state(framesInBlState);
    single->set_id(bulletLocalId);
    single->set_originated_render_frame_id(originatedRenderFrameId);
    single->set_team_id(teamId);
}

void NewPreallocatedCharacterDownsync(CharacterDownsync* single, int buffCapacity, int debuffCapacity, int inventoryCapacity, int bulletImmuneRecordCapacity) {
    for (int i = 0; i < buffCapacity; i++) {
        Buff* singleBuff = single->add_buff_list();
        singleBuff->set_species_id(globalPrimitiveConsts->terminating_buff_species_id());
        singleBuff->set_originated_render_frame_id(globalPrimitiveConsts->terminating_render_frame_id());
        singleBuff->set_orig_ch_species_id(SPECIES_NONE_CH);
    }
    for (int i = 0; i < debuffCapacity; i++) {
        Buff* singleDebuff = single->add_buff_list();
        singleDebuff->set_species_id(globalPrimitiveConsts->terminating_debuff_species_id());
    }
    if (0 < inventoryCapacity) {
        Inventory* inv = single->mutable_inventory();
        for (int i = 0; i < inventoryCapacity; i++) {
            auto singleSlot = inv->add_slots();
            singleSlot->set_stock_type(InventorySlotStockType::NoneIv);
        }
    }

    for (int i = 0; i < bulletImmuneRecordCapacity; i++) {
        auto singleRecord = single->add_bullet_immune_records();
        singleRecord->set_bullet_id(globalPrimitiveConsts->terminating_bullet_id());
        singleRecord->set_remaining_lifetime_rdf_count(0);
    }
}

void NewPreallocatedPlayerCharacterDownsync(PlayerCharacterDownsync* single, int buffCapacity, int debuffCapacity, int inventoryCapacity, int bulletImmuneRecordCapacity) {
    single->set_join_index(globalPrimitiveConsts->magic_join_index_invalid());
    NewPreallocatedCharacterDownsync(single->mutable_chd(), buffCapacity, debuffCapacity, inventoryCapacity, bulletImmuneRecordCapacity);
}

void NewPreallocatedNpcCharacterDownsync(NpcCharacterDownsync* single, int buffCapacity, int debuffCapacity, int inventoryCapacity, int bulletImmuneRecordCapacity) {
    single->set_id(globalPrimitiveConsts->terminating_character_id());
    NewPreallocatedCharacterDownsync(single->mutable_chd(), buffCapacity, debuffCapacity, inventoryCapacity, bulletImmuneRecordCapacity);
}

RenderFrame* NewPreallocatedRdf(int roomCapacity, int preallocNpcCount, int preallocBulletCount) {
    auto ret = new RenderFrame();
    ret->set_id(globalPrimitiveConsts->terminating_render_frame_id());
    ret->set_bullet_id_counter(0);

    for (int i = 0; i < roomCapacity; i++) {
        auto single = ret->add_players_arr();
        NewPreallocatedPlayerCharacterDownsync(single, globalPrimitiveConsts->default_per_character_buff_capacity(), globalPrimitiveConsts->default_per_character_debuff_capacity(), globalPrimitiveConsts->default_per_character_inventory_capacity(), globalPrimitiveConsts->default_per_character_immune_bullet_record_capacity());
    }

    for (int i = 0; i < preallocNpcCount; i++) {
        auto single = ret->add_npcs_arr();
        NewPreallocatedNpcCharacterDownsync(single, globalPrimitiveConsts->default_per_character_buff_capacity(), globalPrimitiveConsts->default_per_character_debuff_capacity(), 1, globalPrimitiveConsts->default_per_character_immune_bullet_record_capacity());
    }

    for (int i = 0; i < preallocBulletCount; i++) {
        auto single = ret->add_bullets();
        NewBullet(single, globalPrimitiveConsts->terminating_bullet_id(), 0, 0, BulletState::StartUp, 0);
    }

    return ret;
}

RenderFrame* mockStartRdf() {
    const int roomCapacity = 2;
    auto startRdf = NewPreallocatedRdf(roomCapacity, 8, 128);
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
char ifdFetchBuffer[pbBufferSizeLimit];

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
    bool isFrontend = true;

    memset(pbByteBuffer, 0, sizeof(pbByteBuffer));
    long byteSize = wsReq.ByteSizeLong();
    wsReq.SerializeToArray(pbByteBuffer, byteSize);
    FrontendBattle* battle = static_cast<FrontendBattle*>(APP_CreateBattle(pbByteBuffer, (int)byteSize, isFrontend, false));
    std::cout << "Created battle = " << battle << std::endl;
    
    jtshared::RenderFrame outRdf;
    std::string outStr;
    int timerRdfId = globalPrimitiveConsts->starting_render_frame_id();
    int loopRdfCnt = 1024;
    int printIntervalRdfCnt = (1 << 0);
    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    auto nowMillis = duration_cast<milliseconds>(
        system_clock::now().time_since_epoch()
    );
    uint32_t inSingleJoinIndex = 1;
    while (loopRdfCnt > timerRdfId) {
        auto it = testCmds1.lower_bound(timerRdfId);
        int toGenerateInputFrameId = (timerRdfId >> globalPrimitiveConsts->input_scale_frames());
        uint64_t inSingleInput = it->second;
        long outBytesCnt = pbBufferSizeLimit;

        int nextRdfToGenerateInputFrameId = ((timerRdfId + 1) >> globalPrimitiveConsts->input_scale_frames());
        bool isLastRdfInIfdCoverage = (nextRdfToGenerateInputFrameId == (toGenerateInputFrameId + 1));

        bool cmdInjected = APP_UpsertCmd(battle, toGenerateInputFrameId, inSingleJoinIndex, inSingleInput, ifdFetchBuffer, &outBytesCnt, isLastRdfInIfdCoverage, false, true);
        if (!cmdInjected) {
            std::cerr << "Failed to inject cmd for timerRdfId=" << timerRdfId << ", toGenerateInputFrameId=" << toGenerateInputFrameId << ", inSingleInput=" << inSingleInput << std::endl;
            exit(1);
        }
        bool stepped = APP_Step(battle, timerRdfId, timerRdfId + 1, false, true);
        timerRdfId++;
        memset(rdfFetchBuffer, 0, sizeof(rdfFetchBuffer));
        outBytesCnt = pbBufferSizeLimit;
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
    bool destroyRes = APP_DestroyBattle(battle, isFrontend);
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
