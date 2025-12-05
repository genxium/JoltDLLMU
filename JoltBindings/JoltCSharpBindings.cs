using System;
using System.Runtime.InteropServices;
using jtshared;

namespace JoltCSharp {
    public unsafe class Bindings {
        private const string JOLT_LIB = "joltc";
        /* All defined in 'joltc.h'*/

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: MarshalAs(UnmanagedType.U1)]
        public static extern bool PrimitiveConsts_Init(char* inBytes, int inBytesCnt);

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: MarshalAs(UnmanagedType.U1)]
        public static extern bool ConfigConsts_Init(char* inBytes, int inBytesCnt);

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: MarshalAs(UnmanagedType.U1)]
        public static extern bool JPH_Init(int nBytesForTempAllocator); // Creates default allocators

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: MarshalAs(UnmanagedType.U1)]
        public static extern bool JPH_Shutdown(); // Destroys default allocators

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void APP_ClearBattle(UIntPtr inBattle);

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: MarshalAs(UnmanagedType.U1)]
        public static extern bool APP_DestroyBattle(UIntPtr inBattle);

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: MarshalAs(UnmanagedType.U1)]
        public static extern bool APP_GetRdf(UIntPtr inBattle, int inRdfId, char* outBytesPreallocatedStart, long* outBytesCntLimit);

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: MarshalAs(UnmanagedType.U1)]
        public static extern bool APP_GetRdfBufferBounds(UIntPtr inBattle, int* outStRdfId, int* outEdRdfId);

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: MarshalAs(UnmanagedType.U1)]
        public static extern bool APP_GetFrameLog(UIntPtr inBattle, int inRdfId, char* outBytesPreallocatedStart, long* outBytesCntLimit);

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: MarshalAs(UnmanagedType.U1)]
        public static extern bool APP_SetFrameLogEnabled(UIntPtr inBattle, [MarshalAs(UnmanagedType.U1)] bool val);

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern ulong APP_SetPlayerActive(UIntPtr inBattle, uint joinIndex);

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern ulong APP_SetPlayerInactive(UIntPtr inBattle, uint joinIndex);

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern ulong APP_GetInactiveJoinMask(UIntPtr inBattle);

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern ulong APP_SetInactiveJoinMask(UIntPtr inBattle, ulong newValue);

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern uint APP_GetUDPayload(ulong ud);

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern ulong APP_CalcStaticColliderUserData(uint staticColliderId);

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern ulong APP_CalcPlayerUserData(uint joinIndex);

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern ulong APP_CalcNpcUserData(uint npcId);

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern ulong APP_CalcBulletUserData(uint bulletId);

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern ulong APP_CalcTriggerUserData(uint triggerId);

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern ulong APP_CalcTrapUserData(uint trapId);

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern ulong APP_CalcPickableUserData(uint pickableId);

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern int APP_EncodePatternForCancelTransit(int patternId, [MarshalAs(UnmanagedType.U1)] bool currEffInAir, [MarshalAs(UnmanagedType.U1)] bool currCrouching, [MarshalAs(UnmanagedType.U1)] bool currOnWall, [MarshalAs(UnmanagedType.U1)] bool currDashing);

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern int APP_EncodePatternForInitSkill(int patternId, [MarshalAs(UnmanagedType.U1)] bool currEffInAir, [MarshalAs(UnmanagedType.U1)] bool currCrouching, [MarshalAs(UnmanagedType.U1)] bool currOnWall, [MarshalAs(UnmanagedType.U1)] bool currDashing, [MarshalAs(UnmanagedType.U1)] bool currInBlockStun, [MarshalAs(UnmanagedType.U1)] bool currAtked, [MarshalAs(UnmanagedType.U1)] bool currParalyzed);

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern UIntPtr FRONTEND_CreateBattle(int rdfBufferSize, [MarshalAs(UnmanagedType.U1)] bool isOnlineArenaMode);

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: MarshalAs(UnmanagedType.U1)]
        public static extern bool FRONTEND_ResetStartRdf(UIntPtr inBattle, char* inBytes, int inBytesCnt, uint inSelfJoinIndex, [MarshalAs(UnmanagedType.LPStr)] string inSelfPlayerId, int inSelfCmdAuthKey);

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: MarshalAs(UnmanagedType.U1)]
        public static extern bool FRONTEND_UpsertSelfCmd(UIntPtr inBattle, ulong inSingleInput, int* outChaserRdfId);

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: MarshalAs(UnmanagedType.U1)]
        public static extern bool FRONTEND_Step(UIntPtr inBattle);

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: MarshalAs(UnmanagedType.U1)]
        public static extern bool FRONTEND_ChaseRolledBackRdfs(UIntPtr inBattle, int* outChaserRdfId, [MarshalAs(UnmanagedType.U1)] bool toTimerRdfId);

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: MarshalAs(UnmanagedType.U1)]
        public static extern bool FRONTEND_GetRdfAndIfdIds(UIntPtr inBattle, int* outTimerRdfId, int* outChaserRdfId, int* outChaserRdfIdLowerBound, int* outLcacIfdId, int* outTimerRdfIdGenIfdId, int* outTimerRdfIdToUseIfdId); 

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: MarshalAs(UnmanagedType.U1)]
        public static extern bool FRONTEND_ProduceUpsyncSnapshotRequest(UIntPtr inBattle, int seqNo, int proposedBatchIfdIdSt, int proposedBatchIfdIdEd, int* outLcacIfdId, char* outBytesPreallocatedStart, long* outBytesCntLimit);

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: MarshalAs(UnmanagedType.U1)]
        public static extern bool FRONTEND_OnUpsyncSnapshotReqReceived(UIntPtr inBattle, char* inBytes, int inBytesCnt, int* outChaserRdfId, int* outMaxPlayerInputFrontId, int* outMinPlayerInputFrontId);

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: MarshalAs(UnmanagedType.U1)]
        public static extern bool FRONTEND_OnDownsyncSnapshotReceived(UIntPtr inBattle, char* inBytes, int inBytesCnt, int* outPostTimerRdfEvictedCnt, int* outPostTimerRdfDelayedIfdEvictedCnt, int* outChaserRdfId, int* outLcacIfdId, int* outMaxPlayerInputFrontId, int* outMinPlayerInputFrontId);

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern UIntPtr BACKEND_CreateBattle(int rdfBufferSize);

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: MarshalAs(UnmanagedType.U1)]
        public static extern bool BACKEND_ResetStartRdf(UIntPtr inBattle, char* inBytes, int inBytesCnt);

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: MarshalAs(UnmanagedType.U1)]
        public static extern bool BACKEND_OnUpsyncSnapshotReqReceived(UIntPtr inBattle, char* inBytes, int inBytesCnt, [MarshalAs(UnmanagedType.U1)] bool fromUdp, [MarshalAs(UnmanagedType.U1)] bool fromTcp, char* outBytesPreallocatedStart, long* outBytesCntLimit, int* outStEvictedCnt, int* outOldLcacIfdId, int* outNewLcacIfdId, int* outOldDynamicsRdfId, int* outNewDynamicsRdfId, int* outMaxPlayerInputFrontId, int* outMinPlayerInputFrontId);

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: MarshalAs(UnmanagedType.U1)]
        public static extern bool BACKEND_ProduceDownsyncSnapshot(UIntPtr inBattle, ulong unconfirmedMask, int stIfdId, int edIfdId, [MarshalAs(UnmanagedType.U1)] bool withRefRdf, char* outBytesPreallocatedStart, long* outBytesCntLimit);

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern int BACKEND_Step(UIntPtr inBattle, int fromRdfId, int toRdfId);

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern int BACKEND_MoveForwardLcacIfdIdAndStep(UIntPtr inBattle, [MarshalAs(UnmanagedType.U1)] bool withRefRdf, int* oldLcacIfdId, int* newLcacIfdId, int* oldDynamicsRdfId, int* newDynamicsRdfId, char* outBytesPreallocatedStart, long* outBytesCntLimit);

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern int BACKEND_GetDynamicsRdfId(UIntPtr inBattle);

        //------------------------------------------------------------------------------------------------
        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl)]
        public static extern void RegisterDebugCallback(debugCallback cb);
        public delegate void debugCallback(IntPtr request, int color, int size);

        /*
        // To be put on Unity side
        enum DColor { red, yellow, orange, green, blue, black, white };

        [MonoPInvokeCallback(typeof(debugCallback))]
        static void OnDebugCallback(IntPtr request, int color, int size) {
            // Ptr to string
            string rawMsg = Marshal.PtrToStringAnsi(request, size);

            // Add Specified DColor
            var coloredMsg = $"<color={((DColor)color).ToString()}>{rawMsg}</color>";
            Debug.Log(coloredMsg);
        }
        */

        public static Bullet NewPreallocatedBullet(uint bulletId, int originatedRenderFrameId, ulong offenderUd, int teamId, BulletState blState, int framesInBlState) {
            return new Bullet {
                BlState = blState,
                FramesInBlState = framesInBlState,
                Id = bulletId,
                OriginatedRenderFrameId = originatedRenderFrameId,
                OffenderUd = offenderUd,
                TeamId = teamId,
                X = 0,
                Y = 0,
                QX = 0,
                QY = 0,
                QZ = 0,
                QW = 1,
                VelX = 0,
                VelY = 0
            };
        }

        public static PlayerCharacterDownsync NewPreallocatedPlayerCharacterDownsync(int buffCapacity, int debuffCapacity, int inventoryCapacity, int bulletImmuneRecordCapacity, PrimitiveConsts primitives) {
            var single = new PlayerCharacterDownsync();
            single.JoinIndex = primitives.MagicJoinIndexInvalid;
            single.Chd = NewPreallocatedCharacterDownsync(buffCapacity, debuffCapacity, inventoryCapacity, bulletImmuneRecordCapacity, primitives);
            return single;
        }

        public static NpcCharacterDownsync NewPreallocatedNpcCharacterDownsync(int buffCapacity, int debuffCapacity, int inventoryCapacity, int bulletImmuneRecordCapacity, PrimitiveConsts primitives) {
            var single = new NpcCharacterDownsync();
            single.Id = primitives.TerminatingCharacterId;
            single.ExhaustedToDropBuffSpeciesId = primitives.TerminatingBuffSpeciesId;
            single.ExhaustedToDropConsumableSpeciesId = primitives.TerminatingConsumableSpeciesId;
            single.Chd = NewPreallocatedCharacterDownsync(buffCapacity, debuffCapacity, inventoryCapacity, bulletImmuneRecordCapacity, primitives);
            return single;
        }

        public static CharacterDownsync NewPreallocatedCharacterDownsync(int buffCapacity, int debuffCapacity, int inventoryCapacity, int bulletImmuneRecordCapacity, PrimitiveConsts primitives) {
            var single = new CharacterDownsync();
            single.LastDamagedByUd = 0;
            single.LastDamagedByBulletTeamId = primitives.TerminatingBulletTeamId;
            for (int i = 0; i < buffCapacity; i++) {
                var singleBuff = new Buff();
                singleBuff.SpeciesId = primitives.TerminatingBuffSpeciesId;
                singleBuff.OriginatedRenderFrameId = primitives.TerminatingRenderFrameId;
                singleBuff.OrigChSpeciesId = PbPrimitives.SPECIES_NONE_CH;
                single.BuffList.Add(singleBuff);
            }
            for (int i = 0; i < debuffCapacity; i++) {
                var singleDebuff = new Debuff();
                singleDebuff.SpeciesId = primitives.TerminatingDebuffSpeciesId;
                single.DebuffList.Add(singleDebuff);
            }
            if (0 < inventoryCapacity) {
                single.Inventory = new Inventory();
                for (int i = 0; i < inventoryCapacity; i++) {
                    var singleSlot = new InventorySlot();
                    singleSlot.StockType = InventorySlotStockType.NoneIv;
                    single.Inventory.Slots.Add(singleSlot);
                }
            }
            for (int i = 0; i < bulletImmuneRecordCapacity; i++) {
                var singleRecord = new BulletImmuneRecord {
                    BulletId = primitives.TerminatingBulletId,
                    RemainingLifetimeRdfCount = 0,
                };
                single.BulletImmuneRecords.Add(singleRecord);
            }

            return single;
        }

        public static RenderFrame NewPreallocatedRdf(int roomCapacity, int preallocNpcCount, int preallocBulletCount, PrimitiveConsts primitives) {
            var ret = new RenderFrame();
            ret.Id = primitives.TerminatingRenderFrameId;
            ret.BulletIdCounter = 0;

            for (int i = 0; i < roomCapacity; i++) {
                var single = NewPreallocatedPlayerCharacterDownsync(primitives.DefaultPerCharacterBuffCapacity, primitives.DefaultPerCharacterDebuffCapacity, primitives.DefaultPerCharacterInventoryCapacity, primitives.DefaultPerCharacterImmuneBulletRecordCapacity, primitives);
                ret.PlayersArr.Add(single);
            }

            for (int i = 0; i < preallocNpcCount; i++) {
                var single = NewPreallocatedNpcCharacterDownsync(primitives.DefaultPerCharacterBuffCapacity, primitives.DefaultPerCharacterDebuffCapacity, 1, primitives.DefaultPerCharacterImmuneBulletRecordCapacity, primitives);
                ret.NpcsArr.Add(single);
            }

            for (int i = 0; i < preallocBulletCount; i++) {
                var single = NewPreallocatedBullet(primitives.TerminatingBulletId, 0, 0, 0, BulletState.StartUp, 0);
                ret.Bullets.Add(single);
            }

            return ret;
        }

        public static void PreemptInventorySlotBeforeMerge(InventorySlot slot) {
            slot.StockType = InventorySlotStockType.NoneIv;
            slot.Quota = 2; 
            slot.FramesToRecover = 0; 
            slot.GaugeCharged = 0;
        } 

        public static void PreemptCharacterDownsyncBeforeMerge(CharacterDownsync chd, PrimitiveConsts primitives) {
            if (null != chd.BuffList) {
                chd.BuffList.Clear();
            }
            if (null != chd.DebuffList) {
                chd.DebuffList.Clear();
            }  
            if (null != chd.Inventory && null != chd.Inventory.Slots) {
                chd.Inventory.Slots.Clear();
            }
            if (null != chd.BulletImmuneRecords) {
                chd.BulletImmuneRecords.Clear();
            }
            if (null != chd.KinematicKnobs) {
                chd.KinematicKnobs.Clear();
            }
            if (null != chd.Atk1Magazine) {
                PreemptInventorySlotBeforeMerge(chd.Atk1Magazine);
            } 
            if (null != chd.SuperAtkGauge) {
                PreemptInventorySlotBeforeMerge(chd.SuperAtkGauge);
            } 

            // [WARNING] For numeric fields, the default value is 0 (or 0F), and if we don't manually reset the numeric fields to their default values here, the C# "MergeFrom" method would NOT copy default values from the assigner.
            chd.X = 0; 
            chd.Y = 0; 
            chd.Z = 0; 
            
            chd.QX = 0; 
            chd.QY = 0; 
            chd.QZ = 0; 
            chd.QW = 0; 

            chd.AimingQX = 0; 
            chd.AimingQY = 0; 
            chd.AimingQZ = 0; 
            chd.AimingQW = 0; 

            chd.VelX = 0; 
            chd.VelY = 0; 
            chd.VelZ = 0; 
    
            chd.Speed = 0;
            chd.SpeciesId = PbPrimitives.SPECIES_NONE_CH;

            chd.FramesToRecover = 0;

            chd.Hp = 0;
            chd.Mp = 0;

            chd.ChCollisionTeamId = 0;

            chd.ChState = CharacterState.InvalidChState;
            chd.FramesInChState = 0;   
            chd.JumpTriggered = false;
            chd.OmitGravity = false;

            chd.ForcedCrouching = false;     
            chd.SlipJumpTriggered = false;
            chd.PrimarilyOnSlippableHardPushback = false;
            chd.NewBirthRdfCountdown = 0;

            chd.FramesInvinsible = 0;
            chd.JumpStarted = false;
            chd.FramesToStartJump = 0;

            chd.BulletTeamId = 0;
            chd.RemainingAirJumpQuota = 0;
            chd.RemainingAirDashQuota = 0;

            chd.DamagedHintRdfCountdown = 0;
            chd.DamagedElementalAttrs = 0;

            chd.RemainingDef1Quota = 0;

            chd.ComboHitCnt = 0;
            chd.ComboFramesRemained = 0;

            chd.LastDamagedByUd = 0;
            chd.LastDamagedByBulletTeamId = 0;
    
            chd.ActiveSkillId = 0;
            chd.ActiveSkillHit = 0;

            chd.BtnAHoldingRdfCnt = 0;
            chd.BtnBHoldingRdfCnt = 0;
            chd.BtnCHoldingRdfCnt = 0;
            chd.BtnDHoldingRdfCnt = 0;
            chd.BtnEHoldingRdfCnt = 0;
            chd.ParryPrepRdfCntDown = 0;
            chd.MpRegenRdfCountdown = 0;
            chd.FlyingRdfCountdown = 0;
            chd.LockingOnUd = 0;
        }

        public static void PreemptPlayerCharacterDownsyncBeforeMerge(PlayerCharacterDownsync playerCharacterDownsync, PrimitiveConsts primitives) {
            PreemptCharacterDownsyncBeforeMerge(playerCharacterDownsync.Chd, primitives);
            playerCharacterDownsync.RevivalX = 0;
            playerCharacterDownsync.RevivalY = 0;
            playerCharacterDownsync.RevivalQX = 0;
            playerCharacterDownsync.RevivalQY = 0;
            playerCharacterDownsync.RevivalQZ = 0;
            playerCharacterDownsync.RevivalQW = 0;
        }

        public static void PreemptInputFrameDownsyncBeforeMerge(InputFrameDownsync ifd, PrimitiveConsts primitives) {
            ifd.ConfirmedList = 0;
            ifd.UdpConfirmedList = 0;
            ifd.InputList.Clear();
        }

        public static void PreemptRenderFrameBeforeMerge(RenderFrame rdf, PrimitiveConsts primitives) {
            rdf.Id = primitives.TerminatingRenderFrameId;
            rdf.PlayersArr.Clear();
            rdf.NpcsArr.Clear();
            rdf.Bullets.Clear();
            rdf.DynamicTrapsArr.Clear();
            rdf.TriggersArr.Clear();
            rdf.Pickables.Clear();

            rdf.BulletIdCounter = primitives.TerminatingBulletId;
            rdf.NpcIdCounter = primitives.TerminatingCharacterId;
            rdf.DynamicTrapIdCounter = primitives.TerminatingTrapId;
            rdf.PickableIdCounter = primitives.TerminatingPickableId;

            rdf.BulletCount = 0;
            rdf.NpcCount = 0;
            rdf.DynamicTrapCount = 0;
            rdf.PickableCount = 0;

            rdf.CountdownNanos = 0;
        }

        public static void PreemptUpsyncSnapshotBeforeMerge(UpsyncSnapshot upsyncSnapshot, PrimitiveConsts primitives) {
            upsyncSnapshot.StIfdId = primitives.TerminatingInputFrameId;
            upsyncSnapshot.CmdList.Clear();
        }

        public static void PreemptDownsyncSnapshotBeforeMerge(DownsyncSnapshot downsyncSnapshot, PrimitiveConsts primitives) {
            downsyncSnapshot.RefRdfId = primitives.TerminatingRenderFrameId;
            downsyncSnapshot.UnconfirmedMask = 0u;
            downsyncSnapshot.StIfdId = primitives.TerminatingInputFrameId;
            if (null != downsyncSnapshot.RefRdf) {
                PreemptRenderFrameBeforeMerge(downsyncSnapshot.RefRdf, primitives);
            }
            if (null != downsyncSnapshot.IfdBatch) {
                downsyncSnapshot.IfdBatch.Clear();
            }
            if (null != downsyncSnapshot.PeerUdpAddrList) {
                downsyncSnapshot.PeerUdpAddrList.Clear();
            }
            if (null != downsyncSnapshot.AssignedUdpTunnel) {
                downsyncSnapshot.AssignedUdpTunnel = null;
            }
            if (null != downsyncSnapshot.PrepareInfo) {
                downsyncSnapshot.PrepareInfo = null;
            }
        }

        public static void PreemptWsReqBeforeMerge(WsReq req, PrimitiveConsts primitives) {
            if (null != req.UpsyncSnapshot) {
                PreemptUpsyncSnapshotBeforeMerge(req.UpsyncSnapshot, primitives);
            }
            if (null != req.SelfParsedRdf) {
                PreemptRenderFrameBeforeMerge(req.SelfParsedRdf, primitives);
            }
            if (null != req.SerializedBarriers) {
                req.SerializedBarriers.Clear();
            }
            if (null != req.SerializedStaticTraps) {
                req.SerializedStaticTraps.Clear();
            }
        }

        public static void PreemptFrameLog(FrameLog log, PrimitiveConsts primitives) {
            if (null != log.Rdf) {
                PreemptRenderFrameBeforeMerge(log.Rdf, primitives);
            }
            if (null != log.UsedIfdInputList) {
                log.UsedIfdInputList.Clear();
            }

            log.UsedIfdConfirmedList = 0;
            log.UsedIfdUdpConfirmedList = 0; 
            log.TimerRdfId = 0;
            log.LcacIfdId = 0;
            log.ChaserRdfId = 0;
            log.ChaserRdfIdLowerBound = 0;
            log.ChaserStRdfId = 0;
            log.ChaserEdRdfId = 0;
            log.ChaserRdfIdLowerBoundSnatched = false;
        }

        public static (Skill?, BulletConfig?) FindBulletConfig(uint skillId, int skillHit) {
            if (PbPrimitives.underlying.NoSkill == skillId) return (null, null);
            if (PbPrimitives.underlying.NoSkillHit == skillHit) return (null, null);
            var skillConfigs = PbSkills.underlying;
            if (!skillConfigs.ContainsKey(skillId)) return (null, null);
            var outSkill = skillConfigs[skillId];
            if (null == outSkill.Hits || skillHit > outSkill.Hits.Count) {
                return (null, null);
            }
            var outBulletConfig = outSkill.Hits[skillHit - 1];
            return (outSkill, outBulletConfig);
        }

        public static bool IsBulletVanishing(Bullet bullet, BulletConfig bc) {
            return BulletState.Vanishing == bullet.BlState;
        }

        public static bool IsBulletExploding(Bullet bullet, BulletConfig bc) {
            switch (bc.BType) {
            case BulletType.Melee:
                return ((BulletState.Exploding == bullet.BlState || BulletState.Vanishing == bullet.BlState) && bullet.FramesInBlState < bc.ExplosionFrames);
            case BulletType.MechanicalCartridge:
            case BulletType.MagicalFireball:
            case BulletType.GroundWave:
                return (BulletState.Exploding == bullet.BlState || BulletState.Vanishing == bullet.BlState);
            default:
                return false;
            }
        }

        public static bool IsBulletStartingUp(Bullet bullet, BulletConfig bc, int currRdfId) {
            return BulletState.StartUp == bullet.BlState;
        }

        public static bool IsBulletActive(Bullet bullet) {
            return (BulletState.Active == bullet.BlState);
        }

        public static bool IsBulletActive(Bullet bullet, BulletConfig bc, int currRdfId) {
            if (BulletState.Exploding == bullet.BlState || BulletState.Vanishing == bullet.BlState) {
                return false;
            }
            return (bullet.OriginatedRenderFrameId + bc.StartupFrames < currRdfId) && (currRdfId < bullet.OriginatedRenderFrameId + bc.StartupFrames + bc.ActiveFrames);
        }

        public static bool IsBulletJustActive(Bullet bullet, BulletConfig bc, int currRdfId) {
            if (BulletState.Exploding == bullet.BlState || BulletState.Vanishing == bullet.BlState) {
                return false;
            }
            // [WARNING] Practically a bullet might propagate for a few render frames before hitting its visually "VertMovingTrapLocalIdUponActive"!
            int visualBufferRdfCnt = 3;
            if (BulletState.Active == bullet.BlState) {
                return visualBufferRdfCnt >= bullet.FramesInBlState;
            }
            return (bullet.OriginatedRenderFrameId + bc.StartupFrames < currRdfId && currRdfId <= bullet.OriginatedRenderFrameId + bc.StartupFrames + visualBufferRdfCnt);
        }

        public static bool IsPickableAlive(Pickable pickable, int currRdfId) {
            return 0 < pickable.RemainingLifetimeRdfCount;
        }

        public static bool IsBulletAlive(Bullet bullet, BulletConfig bc, int currRdfId) {
            if (BulletState.Vanishing == bullet.BlState) {
                return bullet.FramesInBlState < bc.ActiveFrames + bc.ExplosionFrames;
            }
            if (BulletState.Exploding == bullet.BlState && MultiHitType.FromEmission != bc.MhType) {
                return bullet.FramesInBlState < bc.ActiveFrames;
            }
            return (currRdfId < bullet.OriginatedRenderFrameId + bc.StartupFrames + bc.ActiveFrames);
        }
    }
}

