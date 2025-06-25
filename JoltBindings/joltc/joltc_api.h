// Copyright (c) Amer Koleci and Contributors.
// Licensed under the MIT License (MIT). See LICENSE in the repository root for more information.

#ifndef JOLT_C_H_
#define JOLT_C_H_ 1

#ifdef __cplusplus
#	define _JPH_EXTERN extern "C"
#else
#   define _JPH_EXTERN extern
#endif

#ifdef _WIN32
#   define JPH_API_CALL __cdecl
#else
#   define JPH_API_CALL
#endif

#define JPH_CAPI _JPH_EXTERN __declspec(dllexport)

JPH_CAPI bool PrimitiveConsts_Init(char* inBytes, int inBytesCnt);
JPH_CAPI bool ConfigConsts_Init(char* inBytes, int inBytesCnt);

JPH_CAPI bool JPH_Init(int nBytesForTempAllocator);
JPH_CAPI bool JPH_Shutdown(void);

/*
Kindly note that in Jolt, the default gravity direction is negative-y.
*/
JPH_CAPI void* APP_CreateBattle(char* inBytes, int inBytesCnt, bool isFrontend, bool isOnlineArenaMode);
JPH_CAPI bool APP_DestroyBattle(void* inBattle, bool isFrontend);
JPH_CAPI bool APP_Step(void* inBattle, int fromRdfId, int toRdfId, bool isChasing, bool isFrontend);
JPH_CAPI bool APP_GetRdf(void* inBattle, int inRdfId, char* outBytesPreallocatedStart, int* outBytesCntLimit);
JPH_CAPI bool APP_UpsertCmd(void* inBattle, int inIfdId, uint32_t inSingleJoinIndex, uint64_t inSingleInput, char* outBytesPreallocatedStart, int* outBytesCntLimit, bool fromUdp, bool fromTcp, bool isFrontend);

#endif /* JOLT_C_H_ */
