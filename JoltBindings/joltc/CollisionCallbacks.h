#ifndef COLLISION_CALLBACKS_H_
#define COLLISION_CALLBACKS_H_ 1

#pragma once

#include "joltc_export.h"
#include <Jolt/Physics/Collision/ContactListener.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
using namespace JPH;

class JOLTC_EXPORT MyContactListener : public ContactListener
{
    public:
        // See: ContactListener
        virtual ValidateResult  OnContactValidate(const Body &inBody1, const Body &inBody2, RVec3Arg inBaseOffset, const CollideShapeResult &inCollisionResult) override
        {
            return ValidateResult::AcceptAllContactsForThisBodyPair;
        }

        virtual void OnContactAdded(const Body &inBody1, const Body &inBody2, const ContactManifold &inManifold, ContactSettings &ioSettings) override
        {
            ///////////////////
        }

        virtual void OnContactPersisted(const Body &inBody1, const Body &inBody2, const ContactManifold &inManifold, ContactSettings &ioSettings) override
        {
            ///////////////////
        }

        virtual void OnContactRemoved(const SubShapeIDPair &inSubShapePair) override
        {
            ///////////////////
        }
};

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
