#ifndef BACKEND_BATTLE_H_
#define BACKEND_BATTLE_H_ 1

#include "BaseBattle.h"
#ifndef NDEBUG
#include "DebugLog.h"
#endif

using namespace JPH;

class JOLTC_EXPORT BackendBattle : public BaseBattle {
public:
    BackendBattle(int renderBufferSize, int inputBufferSize, TempAllocator* inGlobalTempAllocator) : BaseBattle(renderBufferSize, inputBufferSize, inGlobalTempAllocator)  {
        downsyncSnapshotHolder = new DownsyncSnapshot();
        wsReqHolder = new WsReq();
    }

    virtual ~BackendBattle() {
        // Calls base destructor (implicitly)
        if (nullptr != downsyncSnapshotHolder) {
            delete downsyncSnapshotHolder;
            downsyncSnapshotHolder = nullptr;
        }
        if (nullptr != wsReqHolder) {
            delete wsReqHolder;
            wsReqHolder = nullptr;
        }
#ifndef NDEBUG
        Debug::Log("~BackendBattle/C++", DColor::Green);
#endif
    }
public:
    int elongatedBattleDurationFrames;
    bool elongatedBattleDurationFramesShortenedOnce;
    int dynamicsRdfId;
    int lastForceResyncedRdfId;
    int nstDelayFrames;

public:
    bool OnUpsyncSnapshotReqReceived(char* inBytes, int inBytesCnt, bool fromUdp, bool fromTcp, char* outBytesPreallocatedStart, long* outBytesCntLimit, int* outForceConfirmedStEvictedCnt, int* outOldLcacIfdId, int* outNewLcacIfdId, int* outOldDynamicsRdfId, int* outNewDynamicsRdfId, int* outMaxPlayerInputFrontId, int* outMinPlayerInputFrontId);

    bool OnUpsyncSnapshotReceived(const uint32_t peerJoinIndex, const UpsyncSnapshot& upsyncSnapshot, bool fromUdp, bool fromTcp, char* outBytesPreallocatedStart, long* outBytesCntLimit, int* outForceConfirmedStEvictedCnt, int* outOldLcacIfdId, int* outNewLcacIfdId, int* outOldDynamicsRdfId, int* outNewDynamicsRdfId, int* outMaxPlayerInputFrontId, int* outMinPlayerInputFrontId);

    virtual bool Step(int fromRdfId, int toRdfId, DownsyncSnapshot* virtualIfds = nullptr);

    virtual bool ResetStartRdf(char* inBytes, int inBytesCnt);
    virtual bool ResetStartRdf(const WsReq* initializerMapData);

    bool MoveForwardLcacIfdIdAndStep(bool withRefRdf, int* outOldLcacIfdId, int* outNewLcacIfdId, int* outOldDynamicsRdfId, int* outNewDynamicsRdfId, char* outBytesPreallocatedStart, long* outBytesCntLimit);

    int GetDynamicsRdfId();

protected:
    void produceDownsyncSnapshot(uint64_t unconfirmedMask, int stIfdId, int edIfdId, bool withRefRdf, DownsyncSnapshot** outResult);
    void releaseDownsyncSnapshotArenaOwnership(DownsyncSnapshot* downsyncSnapshot);
    DownsyncSnapshot* downsyncSnapshotHolder = nullptr;
    WsReq* wsReqHolder = nullptr;
};

/*
The "ifdBufferLock" will be managed on C# side because "sending IfdBatchSnapshot to BlockingCollection<ArraySegment<byte>> playerWsDownsyncQue" should be protected by "ifdBufferLock"
- "OnBattleCmdReceived" should be on C#
- "markConfirmationIfApplicable" & "produceIfdBatchSnapshot" should be on C++
- "inactiveMask" should be maintained by C#/OnPlayerXxx callbacks
*/

#endif
