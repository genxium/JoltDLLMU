#ifndef COLLISION_LAYERS_H_
#define COLLISION_LAYERS_H_ 1

#pragma once

#include "joltc_export.h"

////////////////////////////////// App specific logic below //////////////////////////////////
namespace MonInsObjectLayers
{
    static constexpr ObjectLayer NON_MOVING = 0;
    static constexpr ObjectLayer MOVING = 1;
    static constexpr ObjectLayer NUM_LAYERS = 2;
};

/// Class that determines if two object layers can collide
class JOLTC_EXPORT ObjectLayerPairFilterImpl : public ObjectLayerPairFilter
{
public:
    virtual bool ShouldCollide(ObjectLayer inObject1, ObjectLayer inObject2) const override
    {
        switch (inObject1)
        {
        case MonInsObjectLayers::NON_MOVING:
            return inObject2 == MonInsObjectLayers::MOVING; // Non moving only collides with moving
        case MonInsObjectLayers::MOVING:
            return true; // Moving collides with everything
        default:
            JPH_ASSERT(false);
            return false;
        }
    }
};

// Each broadphase layer results in a separate bounding volume tree in the broad phase. You at least want to have
// a layer for non-moving and moving objects to avoid having to update a tree full of static objects every frame.
// You can have a 1-on-1 mapping between object layers and broadphase layers (like in this case) but if you have
// many object layers you'll be creating many broad phase trees, which is not efficient. If you want to fine tune
// your broadphase layers define JPH_TRACK_BROADPHASE_STATS and look at the stats reported on the TTY.
namespace MonInsBroadPhaseLayers
{
    static constexpr BroadPhaseLayer NON_MOVING(0);
    static constexpr BroadPhaseLayer MOVING(1);
    static constexpr uint NUM_LAYERS(2);
};

// BroadPhaseLayerInterface implementation
// This defines a mapping between object and broadphase layers.
class JOLTC_EXPORT BPLayerInterfaceImpl final : public BroadPhaseLayerInterface
{
public:
    BPLayerInterfaceImpl()
    {
        // Create a mapping table from object to broad phase layer
        mObjectToBroadPhase[MonInsObjectLayers::NON_MOVING] = MonInsBroadPhaseLayers::NON_MOVING;
        mObjectToBroadPhase[MonInsObjectLayers::MOVING] = MonInsBroadPhaseLayers::MOVING;
    }

    virtual uint GetNumBroadPhaseLayers() const override
    {
        return MonInsBroadPhaseLayers::NUM_LAYERS;
    }

    virtual BroadPhaseLayer GetBroadPhaseLayer(ObjectLayer inLayer) const override
    {
        JPH_ASSERT(inLayer < MonInsObjectLayers::NUM_LAYERS);
        return mObjectToBroadPhase[inLayer];
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    virtual const char* GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const override
    {
        switch ((BroadPhaseLayer::Type)inLayer)
        {
        case (BroadPhaseLayer::Type)MonInsBroadPhaseLayers::NON_MOVING:   return "NON_MOVING";
        case (BroadPhaseLayer::Type)MonInsBroadPhaseLayers::MOVING:       return "MOVING";
        default:                                                    JPH_ASSERT(false); return "INVALID";
        }
    }
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

private:
    BroadPhaseLayer mObjectToBroadPhase[MonInsObjectLayers::NUM_LAYERS];
};

/// Class that determines if an object layer can collide with a broadphase layer
class JOLTC_EXPORT ObjectVsBroadPhaseLayerFilterImpl : public ObjectVsBroadPhaseLayerFilter
{
public:
    virtual bool ShouldCollide(ObjectLayer inLayer1, BroadPhaseLayer inLayer2) const override
    {
        switch (inLayer1)
        {
        case MonInsObjectLayers::NON_MOVING:
            return inLayer2 == MonInsBroadPhaseLayers::MOVING;
        case MonInsObjectLayers::MOVING:
            return true;
        default:
            JPH_ASSERT(false);
            return false;
        }
    }
};

#endif /* COLLISION_LAYERS_H_ */
