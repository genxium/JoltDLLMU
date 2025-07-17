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
    }

    virtual ~BackendBattle() {
        // Calls base destructor (implicitly)
#ifndef NDEBUG
        Debug::Log("~BackendBattle/C++", DColor::Green);
#endif
    }
public:
    int elongatedBattleDurationFrames;
    bool elongatedBattleDurationFramesShortenedOnce;
    int currDynamicsRdfId;
    int lastForceResyncedRdfId;
    int nstDelayFrames;

public:
    bool OnUpsyncSnapshotReceived(char* inBytes, int inBytesCnt, bool fromUdp, bool fromTcp, char* outBytesPreallocatedStart, long* outBytesCntLimit, int* outForceConfirmedStEvictedCnt);
    bool OnUpsyncSnapshotReceived(const UpsyncSnapshot* upsyncSnapshot, bool fromUdp, bool fromTcp, char* outBytesPreallocatedStart, long* outBytesCntLimit, int* outForceConfirmedStEvictedCnt);
    bool ProduceDownsyncSnapshotAndSerialize(uint64_t unconfirmedMask, int stIfdId, int edIfdId, bool withRefRdf, char* outBytesPreallocatedStart, long* outBytesCntLimit);
    virtual void Step(int fromRdfId, int toRdfId, DownsyncSnapshot* virtualIfds = nullptr);

    virtual bool ResetStartRdf(char* inBytes, int inBytesCnt);
    virtual bool ResetStartRdf(const WsReq* initializerMapData);

protected:
    void produceDownsyncSnapshot(uint64_t unconfirmedMask, int stIfdId, int edIfdId, bool withRefRdf, DownsyncSnapshot** outResult);
    void releaseDownsyncSnapshotArenaOwnership(DownsyncSnapshot* downsyncSnapshot);
};

/*
The "ifdBufferLock" will be managed on C# side because "sending IfdBatchSnapshot to BlockingCollection<ArraySegment<byte>> playerWsDownsyncQue" should be protected by "ifdBufferLock"
- "OnBattleCmdReceived" should be on C#
- "markConfirmationIfApplicable" & "produceIfdBatchSnapshot" should be on C++
- "inactiveMask" should be maintained by C#/OnPlayerXxx callbacks
*/

#endif
