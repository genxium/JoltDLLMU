#ifndef CPP_ONLY_CONSTS_H
#define CPP_ONLY_CONSTS_H 1

#include "PbConsts.h"
#include <vector>

/*
The "const nomenclature" is as follows.
- Constant integer
    - "const int" instead of "int const"
- Pointer to constant integer
    - "const int *" instead of "int const *"
- Constant pointer to constant integer
    - "const int * const" instead of "int const * const"

This is chosen on purpose to align with Jolt Physics engine v5.3.0.
*/

const uint64_t U64_0 = static_cast<uint64_t>(0);
const uint64_t U64_1 = static_cast<uint64_t>(1);
const uint64_t U64_15 = static_cast<uint64_t>(15);
const uint64_t U64_16 = static_cast<uint64_t>(16);
const uint64_t U64_32 = static_cast<uint64_t>(32);
const uint64_t U64_64 = static_cast<uint64_t>(64);
const uint64_t U64_128 = static_cast<uint64_t>(128);
const uint64_t U64_256 = static_cast<uint64_t>(256);

const uint64_t U64_MAX = 0xFFFFFFFFFFFFFFFF;

const uint64_t UDT_STRIPPER = 0xFFFFFFFF00000000; // UDT = "User Data Type"
const uint64_t UD_PAYLOAD_STRIPPER = ~UDT_STRIPPER;
const uint64_t UDT_OBSTACLE = (U64_0 << 32);
const uint64_t UDT_PLAYER = (U64_1 << 32);
const uint64_t UDT_NPC = (U64_1 << 33);
const uint64_t UDT_BL = (U64_1 << 33) + (U64_1 << 32);
const uint64_t UDT_TRIGGER = (U64_1 << 34);
const uint64_t UDT_TRAP = (U64_1 << 34) + (U64_1 << 32);
const uint64_t UDT_PICKABLE = (U64_1 << 34) + (U64_1 << 33); 

const uint32_t cMaxBodies = 1024;
const uint32_t cNumBodyMutexes = 0;
const uint32_t cMaxBodyPairs = 1024;
const uint32_t cMaxContactConstraints = 1024;
const float  cDefaultChDensity = 0.5f;
const float  cDefaultChFriction = 0.01f; 
const float  cDefaultThickness = 0.02f; // An impossibly small value
const float  cDefaultHalfThickness = cDefaultThickness * 0.5f;
const float  cDefaultBarrierFriction = 0.1f; 
const float  cDefaultBarrierThickness = cDefaultThickness * 100.0f;
const float  cDefaultBarrierHalfThickness = cDefaultBarrierThickness * 0.5f;
const float  cDefaultBlHalfLength = cDefaultHalfThickness;
const float  cDefaultBlDensity = 0.1f;
const float  cDefaultTpHalfLength = cDefaultHalfThickness;
const float  cDefaultTpDensity = 100.f;
const float  cCollisionTolerance = 0.05f;
const float  cLengthEps = 1e-3;
const float  cLengthEpsSquared = cLengthEps*cLengthEps;
const float  cLengthNearlySameEps = cLengthEps;
const float  cLengthNearlySameEpsSquared = cLengthNearlySameEps* cLengthNearlySameEps;

const std::vector<std::vector<int>> DIRECTION_DECODER = {
    { 0, 0 }, // 0
    { 0, +2 }, // 1
    { 0, -2 }, // 2
    { +2, 0 }, // 3
    { -2, 0 }, // 4
    { +1, +1 }, // 5
    { -1, -1 }, // 6
    { +1, -1 }, // 7
    { -1, +1 }, // 8
    /* The rest is for safe access from malicious inputs */
    { 0, 0 }, // 9
    { 0, 0 }, // 10
    { 0, 0 }, // 11
    { 0, 0 }, // 12
    { 0, 0 }, // 13
    { 0, 0 }, // 14
    { 0, 0 }, // 15
};

#endif
