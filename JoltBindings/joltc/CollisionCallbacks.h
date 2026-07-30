#ifndef COLLISION_CALLBACKS_H_
#define COLLISION_CALLBACKS_H_ 1

#include "joltc_export.h"
#include <Jolt/Physics/Collision/ContactListener.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
using namespace JPH;


class JOLTC_EXPORT MyBodyActivationListener : public BodyActivationListener
{
    public:
        virtual void OnBodyActivated(const BodyID &inBodyID, uint64 inBodyUserData) override
        {
            ///////////////////
        }

        virtual void OnBodyDeactivated(const BodyID &inBodyID, uint64 inBodyUserData) override
        {
            ///////////////////
        }
};

#endif
