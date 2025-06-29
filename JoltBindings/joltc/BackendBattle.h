#pragma once

#include "BaseBattle.h"

using namespace JPH;

class JOLTC_EXPORT BackendBattle : public BaseBattle {
public:
    BackendBattle(char* inBytes, int inBytesCnt, int renderBufferSize, int inputBufferSize, TempAllocator* inGlobalTempAllocator) : BaseBattle(inBytes, inBytesCnt, renderBufferSize, inputBufferSize, inGlobalTempAllocator)  {
    }

    virtual ~BackendBattle() {
        // Calls base destructor (implicitly)
    }
public:
    int elongatedBattleDurationFrames;
    bool elongatedBattleDurationFramesShortenedOnce;
    int currDynamicsRdfId;
    int lastForceResyncedRdfId;
    int nstDelayFrames;

protected:
    virtual bool preprocessIfdStEviction(int inputFrameId);
    virtual void postprocessIfdStEviction();
};

/*
The "ifdBufferLock" will be managed on C# side because "sending IfdBatchSnapshot to BlockingCollection<ArraySegment<byte>> playerWsDownsyncQue" should be protected by "ifdBufferLock"
- "OnBattleCmdReceived" should be on C#
- "markConfirmationIfApplicable" & "produceIfdBatchSnapshot" should be on C++
- "inactiveMask" should be maintained by C#/OnPlayerXxx callbacks
*/
