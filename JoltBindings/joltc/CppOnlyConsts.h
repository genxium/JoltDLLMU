#ifndef CPP_ONLY_CONSTS_H
#define CPP_ONLY_CONSTS_H

#pragma once

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
const uint64_t U64_MAX = 0xFFFFFFFFFFFFFFFF;

const uint64_t UDT_STRIPPER = 0xFFFFFFFF00000000; // UDT = "User Data Type"
const uint64_t UD_PAYLOAD_STRIPPER = ~UDT_STRIPPER;
const uint64_t UDT_BARRIER = (U64_0 << 32);
const uint64_t UDT_PLAYER = (U64_1 << 32);
const uint64_t UDT_NPC = (U64_1 << 33);
const uint64_t UDT_BL = (U64_1 << 33) + (U64_1 << 32);
const uint64_t UDT_TRAP = (U64_1 << 34);
const uint64_t UDT_TRIGGER = (U64_1 << 34) + (U64_1 << 32);

const uint32_t cMaxBodies = 1024;
const uint32_t cNumBodyMutexes = 0;
const uint32_t cMaxBodyPairs = 1024;
const uint32_t cMaxContactConstraints = 1024;
const float  cDefaultThickness = 2.0f;
const float  cDefaultHalfThickness = cDefaultThickness * 0.5f;

#endif
