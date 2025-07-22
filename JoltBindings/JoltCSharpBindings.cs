using System;
using System.Runtime.InteropServices;
using jtshared;

namespace JoltCSharp {
    public unsafe class Bindings {
        private const string JOLT_LIB = JoltcPrebuilt.Loader.JOLT_LIB;
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
        [return: MarshalAs(UnmanagedType.U1)]
        public static extern bool APP_DestroyBattle(UIntPtr inBattle);
        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: MarshalAs(UnmanagedType.U1)]
        public static extern bool APP_GetRdf(UIntPtr inBattle, int inRdfId, char* outBytesPreallocatedStart, long* outBytesCntLimit);

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern UIntPtr FRONTEND_CreateBattle([MarshalAs(UnmanagedType.U1)] bool isOnlineArenaMode);

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: MarshalAs(UnmanagedType.U1)]
        public static extern bool FRONTEND_ResetStartRdf(UIntPtr inBattle, char* inBytes, int inBytesCnt, int inSelfJoinIndex);

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: MarshalAs(UnmanagedType.U1)]
        public static extern bool FRONTEND_UpsertSelfCmd(UIntPtr inBattle, ulong inSingleInput);

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: MarshalAs(UnmanagedType.U1)]
        public static extern bool FRONTEND_Step(UIntPtr inBattle, int fromRdfId, int toRdfId, [MarshalAs(UnmanagedType.U1)] bool isChasing);
        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: MarshalAs(UnmanagedType.U1)]
        public static extern bool FRONTEND_OnUpsyncSnapshotReceived(UIntPtr inBattle, char* inBytes, int inBytesCnt);

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern UIntPtr BACKEND_CreateBattle();

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: MarshalAs(UnmanagedType.U1)]
        public static extern bool BACKEND_ResetStartRdf(UIntPtr inBattle, char* inBytes, int inBytesCnt);

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: MarshalAs(UnmanagedType.U1)]
        public static extern bool BACKEND_OnUpsyncSnapshotReceived(UIntPtr inBattle, char* inBytes, int inBytesCnt, char* outBytesPreallocatedStart, long* outBytesCntLimit);

        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: MarshalAs(UnmanagedType.U1)]
        public static extern bool BACKEND_ProduceDownsyncSnapshot(UIntPtr inBattle, ulong unconfirmedMask, int stIfdId, int edIfdId, [MarshalAs(UnmanagedType.U1)] bool withRefRdf, char* outBytesPreallocatedStart, long* outBytesCntLimit);
        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: MarshalAs(UnmanagedType.U1)]
        public static extern bool BACKEND_Step(UIntPtr inBattle, int fromRdfId, int toRdfId);

        //------------------------------------------------------------------------------------------------
        [DllImport(JOLT_LIB, CallingConvention = CallingConvention.Cdecl)]
        public static extern void RegisterDebugCallback(debugCallback cb);
        public delegate void debugCallback(IntPtr request, int color, int size);

        /*
        // To be put on Unity side
        enum DColor { red, green, blue, black, white, yellow, orange };

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
                DirX = 0,
                DirY = 0,
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
            single.KilledToDropBuffSpeciesId = primitives.TerminatingBuffSpeciesId;
            single.KilledToDropConsumableSpeciesId = primitives.TerminatingConsumableSpeciesId;
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

        public static void preemptInputFrameDownsyncBeforeMerge(InputFrameDownsync ifd, PrimitiveConsts primitives) {
            ifd.ConfirmedList = 0;
            ifd.UdpConfirmedList = 0;
            ifd.InputList.Clear();
        }

        public static void preemptRenderFrameBeforeMerge(RenderFrame rdf, PrimitiveConsts primitives) {
            rdf.Id = primitives.TerminatingRenderFrameId;
            rdf.PlayersArr.Clear();
            rdf.NpcsArr.Clear();
            rdf.Bullets.Clear();
            rdf.TrapsArr.Clear();
            rdf.TriggersArr.Clear();
            rdf.Pickables.Clear();
            rdf.BulletIdCounter = primitives.TerminatingBulletId;
            rdf.NpcIdCounter = primitives.TerminatingCharacterId;
            rdf.PickableIdCounter = primitives.TerminatingPickableId;
            rdf.CountdownNanos = long.MaxValue;
        }

        public static void preemptUpsyncSnapshotBeforeMerge(UpsyncSnapshot upsyncSnapshot, PrimitiveConsts primitives) {
            upsyncSnapshot.JoinIndex = primitives.MagicJoinIndexInvalid; 
            upsyncSnapshot.StIfdId = primitives.TerminatingInputFrameId;
            upsyncSnapshot.CmdList.Clear();
        }

        public static void preemptDownsyncSnapshotBeforeMerge(DownsyncSnapshot downsyncSnapshot, PrimitiveConsts primitives) {
            downsyncSnapshot.RefRdfId = primitives.TerminatingRenderFrameId;
            downsyncSnapshot.UnconfirmedMask = 0u;
            downsyncSnapshot.StIfdId = primitives.TerminatingInputFrameId;
            if (null != downsyncSnapshot.RefRdf) {
                preemptRenderFrameBeforeMerge(downsyncSnapshot.RefRdf, primitives);
            }
            if (null != downsyncSnapshot.IfdBatch) {
                downsyncSnapshot.IfdBatch.Clear();
            }
        }
    }
}

