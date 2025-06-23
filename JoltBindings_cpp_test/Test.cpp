#include "joltc_export.h" // imports the "JOLTC_EXPORT" macro for "serializable_data.pb.h"
#include "serializable_data.pb.h"
#include "joltc_api.h" 
#include "BattleConsts.h"

#include <Jolt/Jolt.h> // imports the "JPH_EXPORT" macro for classes under namespace JPH
#include "FrontendBattle.h"
#include <google/protobuf/util/json_util.h>

using namespace jtshared;

static float defaultThickness = 2.0f;
static float defaultHalfThickness = defaultThickness * 0.5f;

// Deliberately NOT using enum for "room states" to make use of "C# CompareAndExchange" 
const int ROOM_ID_NONE = 0;

const uint32_t SPECIES_NONE_CH = 0;
const uint32_t SPECIES_BLADEGIRL = 1;
const uint32_t SPECIES_WITCHGIRL = 2;
const uint32_t SPECIES_BRIGHTWITCH = 4;
const uint32_t SPECIES_MAGSWORDGIRL = 6;
const uint32_t SPECIES_BOUNTYHUNTER = 7;
const uint32_t SPECIES_SPEARWOMAN = 8;
const uint32_t SPECIES_LIGHTSPEARWOMAN = 9;
const uint32_t SPECIES_YELLOWDOG = 10;
const uint32_t SPECIES_BLACKDOG = 11;
const uint32_t SPECIES_YELLOWCAT = 12;
const uint32_t SPECIES_BLACKCAT = 13;


void NewBullet(Bullet* single, int bulletLocalId, int originatedRenderFrameId, int offenderJoinIndex, int teamId, BulletState blState, int framesInBlState) {
    single->set_bl_state(blState);
    single->set_frames_in_bl_state(framesInBlState);
    single->set_bullet_local_id(bulletLocalId);
    single->set_originated_render_frame_id(originatedRenderFrameId);
    single->set_offender_join_index(offenderJoinIndex);
    single->set_team_id(teamId);
}

void NewPreallocatedCharacterDownsync(CharacterDownsync* single, int buffCapacity, int debuffCapacity, int inventoryCapacity, int bulletImmuneRecordCapacity) {
    single->set_id(TERMINATING_CHARACTER_ID);
    single->set_join_index(JOIN_INDEX_NOT_INITIALIZED);
    for (int i = 0; i < buffCapacity; i++) {
        Buff* singleBuff = single->add_buff_list();
        singleBuff->set_species_id(TERMINATING_BUFF_SPECIES_ID);
        singleBuff->set_originated_render_frame_id(TERMINATING_RENDER_FRAME_ID);
        singleBuff->set_orig_ch_species_id(SPECIES_NONE_CH);
    }
    for (int i = 0; i < debuffCapacity; i++) {
        Buff* singleDebuff = single->add_buff_list();
        singleDebuff->set_species_id(TERMINATING_DEBUFF_SPECIES_ID);
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
        singleRecord->set_bullet_local_id(TERMINATING_BULLET_LOCAL_ID);
        singleRecord->set_remaining_lifetime_rdf_count(0);
    }
}

RenderFrame* NewPreallocatedRdf(int roomCapacity, int preallocNpcCount, int preallocBulletCount) {
    auto ret = new RenderFrame();
    ret->set_id(TERMINATING_RENDER_FRAME_ID);
    ret->set_bullet_local_id_counter(0);

    for (int i = 0; i < roomCapacity; i++) {
        auto single = ret->add_players_arr();
        NewPreallocatedCharacterDownsync(single, DEFAULT_PER_CHARACTER_BUFF_CAPACITY, DEFAULT_PER_CHARACTER_DEBUFF_CAPACITY, DEFAULT_PER_CHARACTER_INVENTORY_CAPACITY, DEFAULT_PER_CHARACTER_IMMUNE_BULLET_RECORD_CAPACITY);
    }

    for (int i = 0; i < preallocNpcCount; i++) {
        auto single = ret->add_npcs_arr();
        NewPreallocatedCharacterDownsync(single, DEFAULT_PER_CHARACTER_BUFF_CAPACITY, DEFAULT_PER_CHARACTER_DEBUFF_CAPACITY, 1, DEFAULT_PER_CHARACTER_IMMUNE_BULLET_RECORD_CAPACITY);
    }

    for (int i = 0; i < preallocBulletCount; i++) {
        auto single = ret->add_bullets();
        NewBullet(single, TERMINATING_BULLET_LOCAL_ID, 0, 0, 0, BulletState::StartUp, 0);
    }

    return ret;
}

RenderFrame* mockStartRdf() {
    const int roomCapacity = 2;
    auto startRdf = NewPreallocatedRdf(roomCapacity, 8, 128);
    startRdf->set_id(DOWNSYNC_MSG_ACT_BATTLE_START);
    startRdf->set_should_force_resync(false);
    int pickableLocalId = 1;
    int npcLocalId = 1;
    int bulletLocalId = 1;

    auto ch1 = startRdf->mutable_players_arr(0);
    ch1->set_id(10);
    ch1->set_join_index(1);
    ch1->set_x(-100);
    ch1->set_y(200);
    ch1->set_revival_x(ch1->x());
    ch1->set_revival_y(ch1->y());
    ch1->set_speed(10);
    ch1->set_character_state(CharacterState::InAirIdle1NoJump);
    ch1->set_frames_to_recover(0);
    ch1->set_dir_x(2);
    ch1->set_dir_y(0);
    ch1->set_vel_x(0);
    ch1->set_vel_y(0);
    ch1->set_in_air(true);
    ch1->set_on_wall(false);
    ch1->set_hp(100);
    ch1->set_species_id(SPECIES_BLADEGIRL);

    auto ch2 = startRdf->mutable_players_arr(1);
    ch2->set_id(11);
    ch2->set_join_index(2);
    ch2->set_x(+100);
    ch2->set_y(200);
    ch2->set_revival_x(ch2->x());
    ch2->set_revival_y(ch2->y());
    ch2->set_speed(10);
    ch2->set_character_state(CharacterState::InAirIdle1NoJump);
    ch2->set_frames_to_recover(0);
    ch2->set_dir_x(-2);
    ch2->set_dir_y(0);
    ch2->set_vel_x(0);
    ch2->set_vel_y(0);
    ch2->set_in_air(true);
    ch2->set_on_wall(false);
    ch2->set_hp(100);
    ch2->set_species_id(SPECIES_BOUNTYHUNTER);

    startRdf->set_npc_local_id_counter(npcLocalId);
    startRdf->set_bullet_local_id_counter(bulletLocalId);
    startRdf->set_pickable_local_id_counter(pickableLocalId);

    return startRdf;
}

const int pbBufferSizeLimit = (1 << 14);
char wsReqBuffer[pbBufferSizeLimit];
char rdfFetchBuffer[pbBufferSizeLimit];

// Program entry point
int main(int argc, char** argv)
{
    std::cout << "Starting" << std::endl;
    
    std::vector<float> hull1 = {
        -100, 0,
        0, -100,
        100, 100,
        100, 0,
        0, -25,
    };

    std::vector<float> hull2 = {
        -200, 0,
        0, -50,
        75, 75,
        0, -45,
    };

    std::vector<std::vector<float>> hulls;
    hulls.push_back(hull1);
    hulls.push_back(hull2);
    JPH_Init(10*1024*1024);
    std::cout << "Initiated" << std::endl;
    auto startRdf = mockStartRdf();

    WsReq wsReq;
    for (auto hull : hulls) {
        SerializableConvexPolygon* srcPolygon = wsReq.add_serialized_barrier_polygons();
        for (auto xOrY : hull) {
            srcPolygon->add_points(xOrY);
        }
    }
    wsReq.set_allocated_self_parsed_rdf(startRdf);

    memset(wsReqBuffer, 0, sizeof(wsReqBuffer));
    int byteSize = wsReq.ByteSize();
    wsReq.SerializeToArray(wsReqBuffer, byteSize);
    FrontendBattle* battle = static_cast<FrontendBattle*>(APP_CreateBattle(wsReqBuffer, byteSize, true));
    std::cout << "Created\nbattle = " << battle << std::endl;
    
    jtshared::RenderFrame outRdf;
    std::string outStr;
    int timerRdfId = 0;
    int loopRdfCnt = (1 << 11);
    int printIntervalRdfCnt = (1 << 8);
    int printIntervalRdfCntMinus1 = printIntervalRdfCnt - 1;
    while (loopRdfCnt > timerRdfId) {
        bool stepped = APP_Step(battle, timerRdfId, timerRdfId + 1, true);
        memset(rdfFetchBuffer, 0, sizeof(rdfFetchBuffer));
        int outBytesCnt = pbBufferSizeLimit;
        APP_GetRdf(battle, timerRdfId, rdfFetchBuffer, &outBytesCnt);
        std::string initializerMapDataStr(rdfFetchBuffer, outBytesCnt);
        outRdf.ParseFromString(initializerMapDataStr);
        if (0 == (timerRdfId & printIntervalRdfCntMinus1)) {
            auto firstPlayerChd = outRdf.players_arr(0);
            outStr.clear();
            google::protobuf::util::Status status = google::protobuf::util::MessageToJsonString(firstPlayerChd, &outStr);
            if (status.ok()) {
                std::cout << "Step result = " << stepped << " at timerRdfId = " << timerRdfId << ", now firstPlayerChd=\n" << outStr << std::endl;
            } else {
                std::cerr << "Step result = " << stepped << " at timerRdfId = " << timerRdfId << ", error converting firstPlayerChd to JSON:" << status.ToString() << std::endl;
            }
        }
        timerRdfId++;
    }
    
    // clean up
    // [REMINDER] "startRdf" will be automatically deallocated by the destructor of "wsReq"
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
