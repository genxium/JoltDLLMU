#ifndef TEST_HELPER_H_
#define TEST_HELPER_H_ 1

#include "joltc_export.h" // imports the "JOLTC_EXPORT" macro for "serializable_data.pb.h"
#include <Jolt/Jolt.h> // imports the "JPH_EXPORT" macro for classes under namespace JPH
#include "serializable_data.pb.h"
#include "joltc_api.h" 
#include "PbConsts.h"
#include "CppOnlyConsts.h"
#include <google/protobuf/arena.h>

using namespace jtshared;

class TestHelper {
private:
    static void ProvisionPreallocatedBullet(Bullet* single, int bulletLocalId, int originatedRenderFrameId, int teamId, BulletState blState, int framesInBlState);
    static void ProvisionPreallocatedCharacterDownsync(CharacterDownsync* single, int buffCapacity, int debuffCapacity, int inventoryCapacity, int bulletImmuneRecordCapacity);
    static void ProvisionPreallocatedPlayerCharacterDownsync(PlayerCharacterDownsync* single, int buffCapacity, int debuffCapacity, int inventoryCapacity, int bulletImmuneRecordCapacity);
    static void ProvisionPreallocatedNpcCharacterDownsync(NpcCharacterDownsync* single, int buffCapacity, int debuffCapacity, int inventoryCapacity, int bulletImmuneRecordCapacity);
public:
    static RenderFrame* NewPreallocatedRdf(int roomCapacity, int preallocNpcCount, int preallocBulletCount, google::protobuf::Arena* theAllocator);
    static void AddHullsToWsReq(WsReq* wsReq, std::vector<std::vector<float>>& hulls, std::vector<bool>& isBoxOptions, std::vector<bool>& providesSlipJumpOptions) {
        for (int i = 0; i < hulls.size(); i++) {
            auto& hull = hulls[i];
            auto srcBarrier = wsReq->add_serialized_barriers();
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
            if (i < isBoxOptions.size()) {
                srcPolygon->set_is_box(isBoxOptions[i]);
            }
            auto attr = srcBarrier->mutable_attr();
            if (i < providesSlipJumpOptions.size()) {
                attr->set_provides_slip_jump(providesSlipJumpOptions[i]);
            }
        }
    }
};

#endif
