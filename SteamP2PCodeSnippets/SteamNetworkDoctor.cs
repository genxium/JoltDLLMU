using System;
using JoltCSharp;
using System.Collections.Generic;

public class SteamNetworkDoctor {
    private int EXPIRY_MILLIS = 1500;

    private class QEle {
        public int i, j;
        public long t;
    }

    private static SteamNetworkDoctor _instance;
    private static readonly object syncLock = new object();

    public static SteamNetworkDoctor Instance {
        get {
            if (null != _instance) return _instance; 
            lock (syncLock) {
                if (null == _instance) {
                    _instance = new SteamNetworkDoctor();
                }
            }
            return _instance;
        }
    }

    private SteamNetworkDoctor() {
        preallocateHolders(128);
        Reset();
    }

    private void preallocateHolders(int bufferSize) {
        sendingQ = new FrameRingBuffer<QEle>(bufferSize);
        inputFrameDownsyncQ = new FrameRingBuffer<QEle>(bufferSize);
        upsyncSnapshotQ = new FrameRingBuffer<QEle>(bufferSize);
        for (int i = 0; i < bufferSize; i++) {
            sendingQ.Put(new QEle {
                i = 0, j = 0, t = 0
            });
            inputFrameDownsyncQ.Put(new QEle {
                i = 0, j = 0, t = 0
            });
            upsyncSnapshotQ.Put(new QEle {
                i = 0, j = 0, t = 0
            });
        }

        // Clear by "Reset()", and then use by "DryPut()"
    }

    private int localRequiredIfdId;
    private float inputRateThreshold;
    private FrameRingBuffer<QEle> sendingQ;
    private FrameRingBuffer<QEle> inputFrameDownsyncQ;
    private FrameRingBuffer<QEle> upsyncSnapshotQ;
    int immediateRollbackFrames;
    int ifdFrontLockedStepsCnt;

    // For display on SteamNetworkDoctorInfo panel only
    public float DEFAULT_INDICATOR_COUNTDOWN_RDF_CNT_1 = 7.0f; // 60.0f == 1 second  
    public float DEFAULT_INDICATOR_COUNTDOWN_RDF_CNT_2 = 120.0f;   
    public float DEFAULT_INDICATOR_COUNTDOWN_RDF_CNT_3 = 240.0f;   
    public float chasedToPlayerRdfIdIndicatorCountdown;
    public float forceResyncImmediatePumpIndicatorCountdown;
    public float forceResyncFutureAppliedIndicatorCountdown;

    public void Reset() {
        localRequiredIfdId = 0;
        immediateRollbackFrames = 0;
        ifdFrontLockedStepsCnt = 0;
        inputRateThreshold = (PbPrimitivesOverride.BATTLE_DYNAMICS_FPS-8.0f) / (1 << PbPrimitivesOverride.INPUT_SCALE_FRAMES);

        chasedToPlayerRdfIdIndicatorCountdown = 0;
        forceResyncImmediatePumpIndicatorCountdown = 0;
        forceResyncFutureAppliedIndicatorCountdown = 0;

        sendingQ.Clear();
        inputFrameDownsyncQ.Clear();
        upsyncSnapshotQ.Clear();
    }

    public void LogChasedToPlayerRdfId() {
        // At 60 fps, 7 rdf is roughly 7*16.66ms = 116ms, hence if "immediateRollbackFrames" is always small enough relatively to "smallChasingRenderFramesPerUpdate & bigChasingRenderFramesPerUpdate", we should see "chasedToPlayerRdfIdIndicator" always lit. 
        chasedToPlayerRdfIdIndicatorCountdown = DEFAULT_INDICATOR_COUNTDOWN_RDF_CNT_1;
    }

    public void LogForceResyncImmediatePump() {
        forceResyncImmediatePumpIndicatorCountdown = DEFAULT_INDICATOR_COUNTDOWN_RDF_CNT_2;
    }

    public void LogForceResyncFutureApplied() {
        forceResyncFutureAppliedIndicatorCountdown = DEFAULT_INDICATOR_COUNTDOWN_RDF_CNT_3;
    }

    public void LogLocalRequiredIfdId(int val) {
        localRequiredIfdId = val;
    }

    public void LogSending(int i, int j) {
        if (i > j) return; 
        int oldEd = sendingQ.EdFrameId;
        sendingQ.DryPut();
        var (ok, holder) = sendingQ.GetByFrameId(oldEd);
        if (!ok || null == holder) {
            return;
        }
        holder.i = i;
        holder.j = j;

        var nowMillis = DateTimeOffset.Now.ToUnixTimeMilliseconds(); 
        holder.t = nowMillis;

        while (0 < sendingQ.Cnt) {
            var st = sendingQ.GetFirst();
            if (st.t + EXPIRY_MILLIS < nowMillis ) {
                sendingQ.Pop();
            } else {
                break;
            } 
        }
    }

    public void LogInputFrameDownsync(int i, int j) {
        if (i > j) return; 
        int oldEd = inputFrameDownsyncQ.EdFrameId;
        inputFrameDownsyncQ.DryPut();
        var (ok, holder) = inputFrameDownsyncQ.GetByFrameId(oldEd);
        if (!ok || null == holder) {
            return;
        }
        holder.i = i;
        holder.j = j;

        var nowMillis = DateTimeOffset.Now.ToUnixTimeMilliseconds(); 
        holder.t = nowMillis;

        while (0 < inputFrameDownsyncQ.Cnt) {
            var st = inputFrameDownsyncQ.GetFirst();
            if (st.t + EXPIRY_MILLIS < nowMillis ) {
                inputFrameDownsyncQ.Pop();
            } else {
                break;
            }
        }
    }

    public void LogUpsyncSnapshot(int i, int j) {
        if (i > j) return; 
        int oldEd = upsyncSnapshotQ.EdFrameId;
        upsyncSnapshotQ.DryPut();
        var (ok, holder) = upsyncSnapshotQ.GetByFrameId(oldEd);
        if (!ok || null == holder) {
            return;
        }
        holder.i = i;
        holder.j = j;

        var nowMillis = DateTimeOffset.Now.ToUnixTimeMilliseconds(); 
        holder.t = nowMillis;

        while (0 < upsyncSnapshotQ.Cnt) {
            var st = upsyncSnapshotQ.GetFirst();
            if (st.t + EXPIRY_MILLIS < nowMillis ) {
                upsyncSnapshotQ.Pop();
            } else {
                break;
            } 
        }
    }

    public void LogRollbackFrames(int val) {
        immediateRollbackFrames = val;
    }

    public void LogIfdFrontLockedStepCnt() {
        ifdFrontLockedStepsCnt += 1;
    }

    public (float, float, float) Stats() {
        float sendingFps = 0f,
        srvDownsyncFps = 0f,
        peerUpsyncFps = 0f;
        if (1 < sendingQ.Cnt) {
            var st = sendingQ.GetFirst();
            var ed = sendingQ.GetLast();
            long elapsedMillis = ed.t - st.t;
            if (null != st && null != ed && 0 < elapsedMillis) {
                sendingFps = ((ed.j - st.i) * 1000f / elapsedMillis);
            }
        }
        if (1 < inputFrameDownsyncQ.Cnt) {
            var st = inputFrameDownsyncQ.GetFirst();
            var ed = inputFrameDownsyncQ.GetLast();
            long elapsedMillis = ed.t - st.t;
            if (null != st && null != ed && 0 < elapsedMillis) {
                srvDownsyncFps = ((ed.j - st.i) * 1000f / elapsedMillis);
            }
        }
        if (1 < upsyncSnapshotQ.Cnt) {
            var st = upsyncSnapshotQ.GetFirst();
            var ed = upsyncSnapshotQ.GetLast();
            long elapsedMillis = ed.t - st.t;
            if (null != st && null != ed && 0 < elapsedMillis) {
                peerUpsyncFps = ((ed.j - st.i) * 1000f / elapsedMillis);
            }
        }
        return (sendingFps, srvDownsyncFps, peerUpsyncFps);
    }
    
    public (bool, int, float, float, int, int) IsTooFast(int roomCapacity, int selfJoinIndex, int maxPlayerInputFrontId, int minPlayerInputFrontId, int ifdLagTolerance, in HashSet<uint> disconnectedPeerJoinIndices) {
        var (sendingFps, _, peerUpsyncFps) = Stats();
        chasedToPlayerRdfIdIndicatorCountdown -= 1.0f;
        if (0 > chasedToPlayerRdfIdIndicatorCountdown) {
            chasedToPlayerRdfIdIndicatorCountdown = 0;
        }
        forceResyncImmediatePumpIndicatorCountdown -= 1.0f;
        if (0 > forceResyncImmediatePumpIndicatorCountdown) {
            forceResyncImmediatePumpIndicatorCountdown = 0;
        }
        forceResyncFutureAppliedIndicatorCountdown -= 1.0f;
        if (0 > forceResyncFutureAppliedIndicatorCountdown) {
            forceResyncFutureAppliedIndicatorCountdown = 0;
        }

        int ifdIdLag = (localRequiredIfdId - minPlayerInputFrontId);
        if (0 > ifdIdLag) {
            ifdIdLag = 0;
        }

        bool sendingFpsNormal = (sendingFps > inputRateThreshold);
        bool ifdLagSignificant = ifdIdLag > ifdLagTolerance;
        bool allOthersDisconnected = (disconnectedPeerJoinIndices.Count + 1 >= roomCapacity);

        if (ifdLagSignificant && !allOthersDisconnected) {
            return (true, ifdIdLag, sendingFps, peerUpsyncFps, immediateRollbackFrames, ifdFrontLockedStepsCnt);
        }

        return (false, ifdIdLag, sendingFps, peerUpsyncFps, immediateRollbackFrames, ifdFrontLockedStepsCnt);
    }
}
