namespace joltphysics.Tests;

using Google.Protobuf;
using Google.Protobuf.Collections;
using jtshared;
using System;
using System.Collections.Immutable;
using System.Numerics;
using Xunit.Abstractions;

public class JoltcTest {
    // Reference https://xunit.net/docs/capturing-output.html

    private readonly ITestOutputHelper _logger;
    public JoltcTest(ITestOutputHelper theLogger) {
        _logger = theLogger;
    }

    private static float defaultThickness = 2.0f;
    private static float defaultHalfThickness = defaultThickness * 0.5f;

    public static ImmutableArray<Vector2[]> hulls1 = ImmutableArray.Create<Vector2[]>().AddRange(new[]
    {
        new Vector2[] {
            new Vector2(-100f, 0f),
            new Vector2(0f, 100f),
            new Vector2(100f, 100f),
            new Vector2(100f, 0f),
            new Vector2(0f, -25f),
        },
        new Vector2[] {
            new Vector2(-200f, 0f),
            new Vector2(0f, -50f),
            new Vector2(75f, 75f),
            new Vector2(0f, -45f),
        },
    });

    public const int BATTLE_DYNAMICS_FPS = 60;
    public const int DEFAULT_TIMEOUT_FOR_LAST_ALL_CONFIRMED_IFD = 10000; // in milliseconds

    // Deliberately NOT using enum for "room states" to make use of "C# CompareAndExchange" 
    public const int ROOM_ID_NONE = 0;

    public const uint SPECIES_NONE_CH = 0;
    public const uint SPECIES_BLADEGIRL = 1;
    public const uint SPECIES_WITCHGIRL = 2;
    public const uint SPECIES_BRIGHTWITCH = 4;
    public const uint SPECIES_MAGSWORDGIRL = 6;
    public const uint SPECIES_BOUNTYHUNTER = 7;
    public const uint SPECIES_SPEARWOMAN = 8;
    public const uint SPECIES_LIGHTSPEARWOMAN = 9;
    public const uint SPECIES_YELLOWDOG = 10;
    public const uint SPECIES_BLACKDOG = 11;
    public const uint SPECIES_YELLOWCAT = 12;
    public const uint SPECIES_BLACKCAT = 13;


    public static int TERMINATING_RENDER_FRAME_ID = (-1026);
    public static int TERMINATING_INPUT_FRAME_ID = (-1027);

    public static int TERMINATING_TRAP_ID = 0;
    public static int TERMINATING_TRIGGER_ID = 0;
    public static int TERMINATING_PICKABLE_LOCAL_ID = 0;
    public static int TERMINATING_PLAYER_ID = 0;
    public static int TERMINATING_BULLET_LOCAL_ID = 0;
    public static int TERMINATING_BULLET_TEAM_ID = 0; // Default for proto int32 to save space
    public static uint TERMINATING_BUFF_SPECIES_ID = 0; // Default for proto int32 to save space in "CharacterDownsync.killedToDropBuffSpeciesId"
    public static uint TERMINATING_DEBUFF_SPECIES_ID = 0;
    public static uint TERMINATING_CONSUMABLE_SPECIES_ID = 0; // Default for proto int32 to save space in "CharacterDownsync.killedToDropConsumableSpeciesId"
    public const int MAGIC_JOIN_INDEX_INVALID = 0;
    public const int DOWNSYNC_MSG_ACT_BATTLE_START = 0;

    public static int DEFAULT_PREALLOC_BULLET_CAPACITY = 48;
    public static int DEFAULT_PREALLOC_TRAP_CAPACITY = 12;
    public static int DEFAULT_PREALLOC_TRIGGER_CAPACITY = 15;
    public static int DEFAULT_PREALLOC_PICKABLE_CAPACITY = 32;
    public static int DEFAULT_PER_CHARACTER_BUFF_CAPACITY = 1;
    public static int DEFAULT_PER_CHARACTER_DEBUFF_CAPACITY = 1;
    public static int DEFAULT_PER_CHARACTER_INVENTORY_CAPACITY = 3;
    public static int DEFAULT_PER_CHARACTER_IMMUNE_BULLET_RECORD_CAPACITY = 3;

    public static Bullet NewBullet(int bulletLocalId, int originatedRenderFrameId, int offenderJoinIndex, int teamId, BulletState blState, int framesInBlState) {
        return new Bullet {
            BlState = blState,
            FramesInBlState = framesInBlState,
            BulletLocalId = bulletLocalId,
            OriginatedRenderFrameId = originatedRenderFrameId,
            OffenderJoinIndex = offenderJoinIndex,
            TeamId = teamId,
            X = 0,
            Y = 0,
            DirX = 0,
            DirY = 0,
            VelX = 0,
            VelY = 0
        };
    }

    public static CharacterDownsync NewPreallocatedCharacterDownsync(int buffCapacity, int debuffCapacity, int inventoryCapacity, int bulletImmuneRecordCapacity) {
        var single = new CharacterDownsync();
        single.Id = TERMINATING_PLAYER_ID;
        single.KilledToDropBuffSpeciesId = TERMINATING_BUFF_SPECIES_ID;
        single.KilledToDropConsumableSpeciesId = TERMINATING_CONSUMABLE_SPECIES_ID;
        single.LastDamagedByJoinIndex = MAGIC_JOIN_INDEX_INVALID;
        single.LastDamagedByBulletTeamId = TERMINATING_BULLET_TEAM_ID;
        for (int i = 0; i < buffCapacity; i++) {
            var singleBuff = new Buff();
            singleBuff.SpeciesId = TERMINATING_BUFF_SPECIES_ID;
            singleBuff.OriginatedRenderFrameId = TERMINATING_RENDER_FRAME_ID;
            singleBuff.OrigChSpeciesId = SPECIES_NONE_CH;
            single.BuffList.Add(singleBuff);
        }
        for (int i = 0; i < debuffCapacity; i++) {
            var singleDebuff = new Debuff();
            singleDebuff.SpeciesId = TERMINATING_DEBUFF_SPECIES_ID;
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
                BulletLocalId = TERMINATING_BULLET_LOCAL_ID,
                RemainingLifetimeRdfCount = 0,
            };
            single.BulletImmuneRecords.Add(singleRecord);
        }

        return single;
    }

    public static RenderFrame NewPreallocatedRdf(int roomCapacity, int preallocNpcCount, int preallocBulletCount) {
        var ret = new RenderFrame();
        ret.Id = TERMINATING_RENDER_FRAME_ID;
        ret.BulletLocalIdCounter = 0;

        for (int i = 0; i < roomCapacity; i++) {
            var single = NewPreallocatedCharacterDownsync(DEFAULT_PER_CHARACTER_BUFF_CAPACITY, DEFAULT_PER_CHARACTER_DEBUFF_CAPACITY, DEFAULT_PER_CHARACTER_INVENTORY_CAPACITY, DEFAULT_PER_CHARACTER_IMMUNE_BULLET_RECORD_CAPACITY);
            ret.PlayersArr.Add(single);
        }

        for (int i = 0; i < preallocNpcCount; i++) {
            var single = NewPreallocatedCharacterDownsync(DEFAULT_PER_CHARACTER_BUFF_CAPACITY, DEFAULT_PER_CHARACTER_DEBUFF_CAPACITY, 1, DEFAULT_PER_CHARACTER_IMMUNE_BULLET_RECORD_CAPACITY);
            ret.NpcsArr.Add(single);
        }

        for (int i = 0; i < preallocBulletCount; i++) {
            var single = NewBullet(TERMINATING_BULLET_LOCAL_ID, 0, 0, 0, BulletState.StartUp, 0);
            ret.Bullets.Add(single);
        }


        return ret;
    }

    private RenderFrame mockStartRdf() {
        const int roomCapacity = 2;
        var startRdf = NewPreallocatedRdf(roomCapacity, 8, 128);
        startRdf.Id = DOWNSYNC_MSG_ACT_BATTLE_START;
        startRdf.ShouldForceResync = false;
        int pickableLocalId = 1;
        int npcLocalId = 1;
        int bulletLocalId = 1;

        var ch1 = startRdf.PlayersArr[0];
        ch1.Id = 10;
        ch1.JoinIndex = 1;
        ch1.X = 0;
        ch1.Y = 200f;
        ch1.RevivalX = 0;
        ch1.RevivalY = ch1.Y;
        ch1.Speed = 10f;
        ch1.ChState = CharacterState.InAirIdle1NoJump;
        ch1.FramesToRecover = 0;
        ch1.DirX = 2;
        ch1.DirY = 0;
        ch1.VelX = 0;
        ch1.VelY = 0;
        ch1.InAir = true;
        ch1.OnWall = false;
        ch1.Hp = 100;
        ch1.SpeciesId = SPECIES_BLADEGIRL;

        var ch2 = startRdf.PlayersArr[1];
        ch2.Id = 10;
        ch2.JoinIndex = -2;
        ch2.X = 0;
        ch2.Y = -200f;
        ch2.RevivalX = 0;
        ch2.RevivalY = -ch2.X;
        ch2.Speed = 10f;
        ch2.ChState = CharacterState.InAirIdle1NoJump;
        ch2.FramesToRecover = 0;
        ch2.DirX = 2;
        ch2.DirY = 0;
        ch2.VelX = 0;
        ch2.VelY = 0;
        ch2.InAir = true;
        ch2.OnWall = false;
        ch2.Hp = 100;
        ch2.SpeciesId = SPECIES_BOUNTYHUNTER;

        startRdf.NpcLocalIdCounter = npcLocalId;
        startRdf.BulletLocalIdCounter = bulletLocalId;
        startRdf.PickableLocalIdCounter = pickableLocalId;
        
        return startRdf;
    }


    [Fact]
    public unsafe void TestSimpleAllocDealloc() {
        bool dllLoaded = JoltcPrebuilt.Loader.LoadLibrary();
        if (dllLoaded) {
            _logger.WriteLine($"Loaded joltc.dll");
            JoltCSharp.Bindings.JPH_Init(10*1024*1024);
            _logger.WriteLine($"Initialized Jolt resource allocators");

            var startRdf = mockStartRdf();
            int playerRdfId = startRdf.Id;

            WsReq wsReq = new WsReq {
                SelfParsedRdf = startRdf,
            };

            var serializedBarriers = wsReq.SerializedBarrierPolygons;
            foreach (var hull in hulls1) {
                RepeatedField<float> points2 = new RepeatedField<float>();
                foreach (var point in hull) {
                    points2.Add(point.X);
                    points2.Add(point.Y);
                }
                var srcPolygon = new SerializableConvexPolygon {
                    AnchorX = 0f,
                    AnchorY = 0f,
                };
                srcPolygon.Points.AddRange(points2);
                serializedBarriers.Add(srcPolygon);
            }

            byte[] buffer = wsReq.ToByteArray();

            UIntPtr battle = UIntPtr.Zero;
            fixed (byte* bufferPtr = buffer) {
                 battle = JoltCSharp.Bindings.APP_CreateBattle((char*)bufferPtr, buffer.Length, true);
                 _logger.WriteLine($"Created battle at pointer addr = 0x{battle:x}");
            }
            Assert.NotEqual(UIntPtr.Zero, battle);

            int timerRdfId = 0;
            while (30 > timerRdfId) {
                bool stepped = JoltCSharp.Bindings.APP_Step(battle, timerRdfId, timerRdfId+1, true);
                Assert.True(stepped);
                timerRdfId++;
            }

            // Clean up
            bool destroyRes = JoltCSharp.Bindings.APP_DestroyBattle(battle, true);
            Assert.True(destroyRes);
            _logger.WriteLine($"Destroyed battle at pointer addr = 0x{battle:x} with result={destroyRes}");
            bool shutdownRes = JoltCSharp.Bindings.JPH_Shutdown();
            _logger.WriteLine($"Jolt infra shutdown result = {shutdownRes}");
        } else {
        }
    }
}
