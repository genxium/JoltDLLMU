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

JPH_CAPI bool JPH_Init(int nBytesForTempAllocator);
JPH_CAPI bool JPH_Shutdown(void);

/*
Kindly note that in Jolt, the default gravity direction is negative-y.
*/
JPH_CAPI void* APP_CreateBattle(char* inBytes, int inBytesCnt, bool isFrontend);
JPH_CAPI bool APP_DestroyBattle(void* inBattle, bool isFrontend);
JPH_CAPI bool APP_Step(void* inBattle, int fromRdfId, int toRdfId, bool isChasing);
JPH_CAPI bool APP_GetRdf(void* inBattle, int inRdfId, char* outBytesPreallocatedStart, int* outBytesCntLimit);

#endif /* JOLT_C_H_ */
