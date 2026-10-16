using System;
using System.IO;
using System.Collections.Generic;
using Google.Protobuf.Collections;
using jtshared;
using JoltCSharp;
using Google.Protobuf;

public class FrameLogPrinter {

    public static void trimIfdInPlace(InputFrameDownsync ifd) {
        // Removed bullets with TERMINATING_ID
        ifd.ConfirmedList = 0;
    }

    public static string stringifyBulletImmuneRecords(RepeatedField<BulletImmuneRecord> records) {
        if (null == records) return "";
        var recordsSb = new List<String>();
        for (int k = 0; k < records.Count; k++) {
            var record = records[k];
            if (null == record) break;
            if (PbPrimitivesOverride.Instance.getUnderlying().TerminatingBulletId == record.BulletId) break;
            recordsSb.Add($"bid:{record.BulletId},lfc:{record.RemainingLifetimeRdfCount}");
        }
        return String.Join('|', recordsSb);
    }

    public static string stringifyPlayer(PlayerCharacterDownsync pd) {
        if (null == pd) return "";
        var chd = pd.Chd;
        return $"j:{pd.JoinIndex},x:{chd.X},y:{chd.Y},vx:{chd.VelX},vy:{chd.VelY},gvx:{chd.GroundVelX},gvy:{chd.GroundVelY},fr:{chd.FramesToRecover},cs:{chd.ChState},fc:{chd.FramesInChState},fri:{chd.FramesInvinsible},qx:{chd.QX},qy:{chd.QY},qz:{chd.QZ},qw:{chd.QW},wscd:{chd.WalkstoppingRdfCountdown},fscd:{chd.FallstoppingRdfCountdown},gud:{chd.GroundUd},wud:{chd.WallUd},ahld:{chd.BtnAHoldingRdfCnt},bhld:{chd.BtnBHoldingRdfCnt},chld:{chd.BtnCHoldingRdfCnt},dhld:{chd.BtnDHoldingRdfCnt},ehld:{chd.BtnEHoldingRdfCnt},bim:{stringifyBulletImmuneRecords(chd.BulletImmuneRecords)}";
    }

    public static string stringifyNpc(NpcCharacterDownsync npc) {
        if (null == npc) return "";
        var chd = npc.Chd;
        return $"i:{npc.Id},x:{chd.X},y:{chd.Y},vx:{chd.VelX},vy:{chd.VelY},gvx:{chd.GroundVelX},gvy:{chd.GroundVelY},fr:{chd.FramesToRecover},cs:{chd.ChState},fc:{chd.FramesInChState},fri:{chd.FramesInvinsible},qx:{chd.QX},qy:{chd.QY},qz:{chd.QZ},qw:{chd.QW},wscd:{chd.WalkstoppingRdfCountdown},fscd:{chd.FallstoppingRdfCountdown},gud:{chd.GroundUd},ahld:{chd.BtnAHoldingRdfCnt},bhld:{chd.BtnBHoldingRdfCnt},chld:{chd.BtnCHoldingRdfCnt},dhld:{chd.BtnDHoldingRdfCnt},ehld:{chd.BtnEHoldingRdfCnt},bim:{stringifyBulletImmuneRecords(chd.BulletImmuneRecords)}";
    }

    public static string stringifyBullet(Bullet bl) {
        if (null == bl) return "";
        return $"i:{bl.Id},x:{bl.X},y:{bl.Y},vx:{bl.VelX},vy:{bl.VelY},bs:{bl.BlState},fb:{bl.FramesInBlState},qx:{bl.QX},qy:{bl.QY},qz:{bl.QZ},qw:{bl.QW}";
    }

    public static string stringifyDynamicTrap(Trap tp) {
        if (null == tp) return "";
        return $"i:{tp.Id},x:{tp.X},y:{tp.Y},vx:{tp.VelX},vy:{tp.VelY},bs:{tp.TrapState},fb:{tp.FramesInTrapState},qx:{tp.QX},qy:{tp.QY},qz:{tp.QZ},qw:{tp.QW}";
    }

    public static string stringifyRdf(RenderFrame rdf) {
        var playerSb = new List<String>();
        for (int k = 0; k < rdf.Players.Count; k++) {
            playerSb.Add(stringifyPlayer(rdf.Players[k]));
        }

        var npcSb = new List<String>();
        for (int k = 0; k < rdf.Npcs.Count; k++) {
            var npc = rdf.Npcs[k];
            if (PbPrimitivesOverride.Instance.getUnderlying().TerminatingCharacterId == npc.Id) break;
            npcSb.Add(stringifyNpc(npc));
        }

        var bulletSb = new List<String>();
        for (int k = 0; k < rdf.Bullets.Count; k++) {
            var bl = rdf.Bullets[k];
            if (PbPrimitivesOverride.Instance.getUnderlying().TerminatingBulletId == bl.Id) break;
            bulletSb.Add(stringifyBullet(bl));
        }

        var trapSb = new List<String>();
        for (int k = 0; k < rdf.DynamicTraps.Count; k++) {
            var tp = rdf.DynamicTraps[k];
            if (PbPrimitivesOverride.Instance.getUnderlying().TerminatingTrapId == tp.Id) break;
            trapSb.Add(stringifyDynamicTrap(tp));
        }

        var rdfSb = new List<String>();

        var playerS = String.Join(';', playerSb);
        var npcS = String.Join(';', npcSb);
        var bulletS = String.Join(';', bulletSb);
        var trapS = String.Join(';', trapSb);
        rdfSb.Add($"ps:{playerS}");
        rdfSb.Add($"nc:{rdf.NpcCount}; ns:{npcS}");
        rdfSb.Add($"bc:{rdf.BulletCount}; bs:{bulletS}");
        rdfSb.Add($"tpc: {rdf.DynamicTrapCount}; tps:{trapS}");
        
        return $"{{ {String.Join('\n', rdfSb)} }}";
    }

    public static string stringifyIfd(int ifdId, RepeatedField<ulong> inputList, ulong confirmedList, ulong udpConfirmedList, bool trimConfirmedList) {
        var inputListSb = new List<String>();
        for (int k = 0; k < inputList.Count; k++) {
            inputListSb.Add($"{inputList[k]}");
        }
        if (trimConfirmedList) {
            return $"{{ ifdId:{ifdId},ipts:{String.Join(',', inputListSb)} }}";
        } else {
            return $"{{ ifdId:{ifdId},ipts:{String.Join(',', inputListSb)},cfd:{confirmedList},ucfd:{udpConfirmedList} }}";
        }
    }

    public static string stringifyChaser(int timerRdfId, int chaserStRdfId, int chaserEdRdfId, int lcacIfdId, int chaserRdfId, int chaserRdfIdLowerBound, bool snatched) {
        if (0 == timerRdfId) {
            return "";
        } else if (0 == chaserStRdfId) {
            return $"{{ timerRdfId:{timerRdfId},lcacIfdId:{lcacIfdId},chaserRdfId:{chaserRdfId},chaserRdfIdLowerBound:{chaserRdfIdLowerBound},snatched:{snatched} }}";
        } else {
            return $"{{ timerRdfId:{timerRdfId},lcacIfdId:{lcacIfdId},chaserRdfId:{chaserRdfId},chaserRdfIdLowerBound:{chaserRdfIdLowerBound},snatched:{snatched},chaserStRdfId:{chaserStRdfId},chaserEdRdfId:{chaserEdRdfId} }}";
        }
    }

    public static string stringifyFrameLog(FrameLog fl, bool trimConfirmedList) {
        // Why do we need an extra class definition of "FrameLog" while having methods "stringifyRdf" & "stringifyIfd"? That's because we might need put "FrameLog" on transmission, i.e. sending to backend upon battle stopped, thus a wrapper class would provide some convenience though not 100% necessary.
        return $"rdfId:{fl.Rdf.Id} from\n\t{stringifyChaser(fl.TimerRdfId, fl.ChaserStRdfId, fl.ChaserEdRdfId, fl.LcacIfdId, fl.ChaserRdfId, fl.ChaserRdfIdLowerBound, fl.ChaserRdfIdLowerBoundSnatched)}\n\t{stringifyIfd(fl.ActuallyUsedIfdId, fl.UsedIfdInputList, fl.UsedIfdConfirmedList, fl.UsedIfdUdpConfirmedList, trimConfirmedList)}\n{stringifyRdf(fl.Rdf)}";
    }

    public static string stringifyAimingRay(in AimingRay ar) {
        return $"ofud:{ar.OffenderUd}, st:({ar.StX}, {ar.StY}), ed:({ar.EdX}, {ar.EdY})";
    }

    public static string stringifyStepResult(in int rdfId, in StepResult sr) {
        var aimingRaysSb = new List<String>();
        for (int k = 0; k < sr.AimingRayCount; k++) {
            aimingRaysSb.Add(stringifyAimingRay(sr.AimingRays[k]));
        }
        var aimingRaysS = String.Join("|", aimingRaysSb);
        return $"sr:{rdfId}, arc={sr.AimingRayCount}, ars={aimingRaysS}";
    }

    public static unsafe void wrapUpFrameLogs(UIntPtr battle, int fromRdfId, int toRdfId, bool trimConfirmedList, string dirPath, string filename) {
        const int frameLogBufferSizeLimit = (1 << 15);
        byte[] frameLogFetchBuffer = new byte[frameLogBufferSizeLimit];
        FrameLog frameLogHolder = new FrameLog();
        const int stepResultBufferSizeLimit = (1 << 15);
        byte[] stepResultFetchBuffer = new byte[stepResultBufferSizeLimit];
        StepResult stepResultHolder = new StepResult();
        
        using (StreamWriter outputFile = new StreamWriter(Path.Combine(dirPath, filename))) {
            outputFile.AutoFlush = true;
            fixed (byte* frameLogBufferPtr = frameLogFetchBuffer, stepResultBufferPtr = stepResultFetchBuffer) {
                for (int rdfId = fromRdfId; rdfId <= toRdfId; rdfId++) {
                    long outBytesCnt = 0;
                    long* outBytesCntPtr = &outBytesCnt;
                    *outBytesCntPtr = frameLogBufferSizeLimit;
                    bool fetched = Bindings.APP_GetFrameLog(battle, rdfId, (char*)frameLogBufferPtr, outBytesCntPtr);
                    if (fetched && 0 < *outBytesCntPtr) {
                        Bindings.PreemptFrameLog(frameLogHolder, PbPrimitivesOverride.Instance.getUnderlying());
                        frameLogHolder.MergeFrom(frameLogFetchBuffer, 0, (int)(*outBytesCntPtr));
                        outputFile.WriteLine(stringifyFrameLog(frameLogHolder, trimConfirmedList));
                    }

                    *outBytesCntPtr = stepResultBufferSizeLimit;
                    fetched = Bindings.APP_GetStepResult(battle, rdfId, (char*)stepResultBufferPtr, outBytesCntPtr);
                    if (fetched && 0 < *outBytesCntPtr) {
                        Bindings.PreemptStepResult(stepResultHolder, PbPrimitivesOverride.Instance.getUnderlying());
                        stepResultHolder.MergeFrom(stepResultFetchBuffer, 0, (int)(*outBytesCntPtr));
                        outputFile.WriteLine(stringifyStepResult(rdfId, stepResultHolder));
                    }
                }
            }
        }
    }
}
