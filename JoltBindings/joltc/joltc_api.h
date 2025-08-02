// Copyright (c) Amer Koleci and Contributors.
// Licensed under the MIT License (MIT). See LICENSE in the repository root for more information.

#ifndef JOLT_C_H_
#define JOLT_C_H_ 1

#ifdef __cplusplus
#	define _JPH_EXTERN extern "C"
#else
#   define _JPH_EXTERN extern
#endif

#ifdef JPH_SHARED_LIBRARY
	#ifdef JPH_BUILD_SHARED_LIBRARY
		// While building the shared library, we must export these symbols
		#if defined(_WIN32) && !defined(JPH_COMPILER_MINGW)
			#define JOLTC_API_EXPORT __declspec(dllexport)
		#else
			#define JOLTC_API_EXPORT __attribute__ ((visibility ("default")))
		#endif
	#else
		// When linking against Jolt, we must import these symbols
		#if defined(_WIN32) && !defined(JPH_COMPILER_MINGW)
			#define JOLTC_API_EXPORT __declspec(dllimport)
		#else
			#define JOLTC_API_EXPORT __attribute__ ((visibility ("default")))
		#endif
	#endif
#else
	// If the define is not set, we use static linking and symbols don't need to be imported or exported
	#define JOLTC_API_EXPORT
#endif

#define JPH_CAPI _JPH_EXTERN JOLTC_API_EXPORT 

JPH_CAPI bool PrimitiveConsts_Init(char* inBytes, int inBytesCnt);
JPH_CAPI bool ConfigConsts_Init(char* inBytes, int inBytesCnt);

JPH_CAPI bool JPH_Init(int nBytesForTempAllocator);
JPH_CAPI bool JPH_Shutdown(void);

/*
Kindly note that in Jolt, the default gravity direction is negative-y.
*/
JPH_CAPI bool APP_DestroyBattle(void* inBattle);
JPH_CAPI bool APP_GetRdf(void* inBattle, int inRdfId, char* outBytesPreallocatedStart, long* outBytesCntLimit);
JPH_CAPI uint64_t APP_SetPlayerActive(void* inBattle, uint32_t joinIndex); // returns the new value
JPH_CAPI uint64_t APP_SetPlayerInactive(void* inBattle, uint32_t joinIndex); // returns the new value
JPH_CAPI uint64_t APP_GetInactiveJoinMask(void* inBattle);
JPH_CAPI uint64_t APP_SetInactiveJoinMask(void* inBattle, uint64_t newValue); // returns the old value

/*
[WARNING] 

These "BACKEND_Xxx" functions DON'T apply any lock by themselves. 

The equivalent to [DLLMU-v2.3.4 "inputBufferLock"](https://github.com/genxium/DelayNoMoreUnity/blob/v2.3.4/backend/Battle/Room.cs#L144) will be handled in C# because I/O will be handled there. 

Note that in addition to guarding write-operations to "inputBuffer/ifdBuffer", "inputBufferLock" MUST also guard "sending of DownsyncSnapshot" to preserve the same "order of message sending" as the "order of message generation", i.e. order of "DownsyncSnapshot.st_ifd_id" received on frontend via TCP must be non-descending, see https://github.com/genxium/DelayNoMoreUnity/blob/v2.3.4/backend/Battle/Room.cs#L1371 for more information.
*/
JPH_CAPI void* BACKEND_CreateBattle();
JPH_CAPI bool BACKEND_ResetStartRdf(void* inBattle, char* inBytes, int inBytesCnt);
JPH_CAPI bool BACKEND_OnUpsyncSnapshotReceived(void* inBattle, char* inBytes, int inBytesCnt, bool fromUdp, bool fromTcp, char* outBytesPreallocatedStart, long* outBytesCntLimit); // [WARNING] Possibly writes "DownsyncSnapshot" into "outBytesPreallocatedStart" 
JPH_CAPI bool BACKEND_ProduceDownsyncSnapshot(void* inBattle, uint64_t unconfirmedMask, int stIfdId, int edIfdId, bool withRefRdf, char* outBytesPreallocatedStart, long* outBytesCntLimit);
JPH_CAPI void BACKEND_Step(void* inBattle, int fromRdfId, int toRdfId);
JPH_CAPI int BACKEND_GetDynamicsRdfId(void* inBattle);
JPH_CAPI bool BACKEND_MoveForwardLcacIfdIdAndStep(void* inBattle, bool withRefRdf, int* outOldLcacIfdId, int* outNewLcacIfdId, int* outOldDynamicsRdfId, int* outNewDynamicsRdfId, char* outBytesPreallocatedStart, long* outBytesCntLimit);

JPH_CAPI void* FRONTEND_CreateBattle(bool isOnlineArenaMode);
JPH_CAPI bool FRONTEND_ResetStartRdf(void* inBattle, char* inBytes, int inBytesCnt, uint32_t inSelfJoinIndex);
JPH_CAPI bool FRONTEND_UpsertSelfCmd(void* inBattle, uint64_t inSingleInput);
JPH_CAPI bool FRONTEND_ProduceUpsyncSnapshot(void* inBattle, int proposedBatchIfdIdSt, int proposedBatchIfdIdEd, char* outBytesPreallocatedStart, long* outBytesCntLimit);
JPH_CAPI bool FRONTEND_OnUpsyncSnapshotReceived(void* inBattle, char* inBytes, int inBytesCnt, int* outMaxPlayerInputFrontId, int* outMinPlayerInputFrontId);
JPH_CAPI bool FRONTEND_OnDownsyncSnapshotReceived(void* inBattle, char* inBytes, int inBytesCnt, int* outPostTimerRdfEvictedCnt, int* outPostTimerRdfDelayedIfdEvictedCnt, int* outLcacIfdId, int* outMaxPlayerInputFrontId, int* outMinPlayerInputFrontId);
JPH_CAPI void FRONTEND_Step(void* inBattle, int fromRdfId, int toRdfId, bool isChasing);
JPH_CAPI void FRONTEND_GetRdfAndIfdIds(void* inBattle, int* outTimerRdfId, int* outChaserRdfId, int* outChaserRdfIdLowerBound, int* outLcacIfdId, int* outTimerRdfIdGenIfdId, int* outTimerRdfIdToUseIfdId);

#endif /* JOLT_C_H_ */
