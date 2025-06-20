#pragma once

#include "BaseBattle.h"

using namespace JPH;

class JOLTC_EXPORT BackendBattle : public BaseBattle {
public:
    inline BackendBattle(const int aPlayersCnt, const int renderBufferSize, const int inputBufferSize, PhysicsSystem* aPhySys, JobSystemThreadPool* aJobSys) : BaseBattle(aPlayersCnt, renderBufferSize, inputBufferSize, aPhySys, aJobSys) {
    }

    virtual ~BackendBattle() {
        // Calls base destructor (implicitly)
    }
public:
    int elongatedBattleDurationFrames;
    bool elongatedBattleDurationFramesShortenedOnce;
    int curDynamicsRenderFrameId;
    int lastForceResyncedRdfId;
    int nstDelayFrames;
};

/*
The "ifdBufferLock" will be managed on C# side because "sending IfdBatchSnapshot to BlockingCollection<ArraySegment<byte>> playerWsDownsyncQue" should be protected by "ifdBufferLock"
- "OnBattleCmdReceived" should be on C#
- "markConfirmationIfApplicable" & "produceIfdBatchSnapshot" should be on C++
- "inactiveMask" should be maintained by C#/OnPlayerXxx callbacks
*/
