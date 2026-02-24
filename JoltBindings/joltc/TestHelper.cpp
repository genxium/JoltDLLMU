#include "TestHelper.h"

void TestHelper::ProvisionPreallocatedBullet(Bullet* single, int bulletId, int originatedRenderFrameId, int teamId, BulletState blState, int framesInBlState) {
    single->set_id(bulletId);
    single->set_bl_state(blState);
    single->set_frames_in_bl_state(framesInBlState);
    single->set_originated_render_frame_id(originatedRenderFrameId);
    single->set_team_id(teamId);
    single->set_q_x(0);
    single->set_q_y(0);
    single->set_q_z(0);
    single->set_q_w(1);
}

void TestHelper::ProvisionPreallocatedCharacterDownsync(CharacterDownsync* single, int buffCapacity, int debuffCapacity, int inventoryCapacity, int bulletImmuneRecordCapacity) {
    single->set_q_x(0);
    single->set_q_y(0);
    single->set_q_z(0);
    single->set_q_w(1);

    single->set_aiming_q_x(0);
    single->set_aiming_q_y(0);
    single->set_aiming_q_z(0);
    single->set_aiming_q_w(1);

    for (int i = 0; i < buffCapacity; i++) {
        Buff* singleBuff = single->add_buff_list();
        singleBuff->set_species_id(globalPrimitiveConsts->terminating_buff_species_id());
        singleBuff->set_originated_render_frame_id(globalPrimitiveConsts->terminating_render_frame_id());
        singleBuff->set_orig_ch_species_id(globalPrimitiveConsts->species_none_ch());
    }

    for (int i = 0; i < debuffCapacity; i++) {
        Buff* singleDebuff = single->add_buff_list();
        singleDebuff->set_species_id(globalPrimitiveConsts->terminating_debuff_species_id());
    }

    for (int i = 0; i < inventoryCapacity; i++) {
        auto singleSlot = single->add_inventory_slots();
        singleSlot->set_stock_type(InventorySlotStockType::NoneIv);
    }
    
    for (int i = 0; i < bulletImmuneRecordCapacity; i++) {
        auto singleRecord = single->add_bullet_immune_records();
        singleRecord->set_bullet_id(globalPrimitiveConsts->terminating_bullet_id());
        singleRecord->set_remaining_lifetime_rdf_count(0);
    }
}

void TestHelper::ProvisionPreallocatedPlayerCharacterDownsync(PlayerCharacterDownsync* single, int buffCapacity, int debuffCapacity, int inventoryCapacity, int bulletImmuneRecordCapacity) {
    single->set_join_index(globalPrimitiveConsts->magic_join_index_invalid());
    ProvisionPreallocatedCharacterDownsync(single->mutable_chd(), buffCapacity, debuffCapacity, inventoryCapacity, bulletImmuneRecordCapacity);
}

void TestHelper::ProvisionPreallocatedNpcCharacterDownsync(NpcCharacterDownsync* single, int buffCapacity, int debuffCapacity, int inventoryCapacity, int bulletImmuneRecordCapacity) {
    single->set_id(globalPrimitiveConsts->terminating_character_id());
    ProvisionPreallocatedCharacterDownsync(single->mutable_chd(), buffCapacity, debuffCapacity, inventoryCapacity, bulletImmuneRecordCapacity);
}

RenderFrame* TestHelper::NewPreallocatedRdf(int roomCapacity, int preallocNpcCount, int preallocBulletCount, google::protobuf::Arena* theAllocator) {
    auto ret = google::protobuf::Arena::CreateMessage<RenderFrame>(theAllocator);
    ret->set_id(globalPrimitiveConsts->terminating_render_frame_id());
    ret->set_bullet_id_counter(0);

    for (int i = 0; i < roomCapacity; i++) {
        auto single = ret->add_players();
        ProvisionPreallocatedPlayerCharacterDownsync(single, globalPrimitiveConsts->default_per_character_buff_capacity(), globalPrimitiveConsts->default_per_character_debuff_capacity(), globalPrimitiveConsts->default_per_character_inventory_capacity(), globalPrimitiveConsts->default_per_character_immune_bullet_record_capacity());
    }

    for (int i = 0; i < preallocNpcCount; i++) {
        auto single = ret->add_npcs();
        ProvisionPreallocatedNpcCharacterDownsync(single, globalPrimitiveConsts->default_per_character_buff_capacity(), globalPrimitiveConsts->default_per_character_debuff_capacity(), 1, globalPrimitiveConsts->default_per_character_immune_bullet_record_capacity());
    }

    for (int i = 0; i < preallocBulletCount; i++) {
        auto single = ret->add_bullets();
        ProvisionPreallocatedBullet(single, globalPrimitiveConsts->terminating_bullet_id(), 0, 0, BulletState::StartUp, 0);
    }

    return ret;
}
