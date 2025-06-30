namespace joltphysics.Tests;

using Google.Protobuf;
using Google.Protobuf.Collections;
using JoltCSharp;
using jtshared;
using System;
using System.Collections.Immutable;
using System.Numerics;
using Xunit.Abstractions;

public class JoltcTest {
    // Reference https://xunit.net/docs/capturing-output.html

    private readonly ITestOutputHelper _logger;
    private static PrimitiveConsts primitives = PbPrimitives.underlying;
    private static MapField<uint, CharacterConfig> characters = PbCharacters.underlying;

    const int pbBufferSizeLimit = (1 << 14);
    byte[] rdfFetchBuffer;
    byte[] ifdFetchBuffer;

    public JoltcTest(ITestOutputHelper theLogger) {
        _logger = theLogger;
        rdfFetchBuffer = new byte[pbBufferSizeLimit];
        ifdFetchBuffer = new byte[pbBufferSizeLimit];
    }
    
    public static ImmutableArray<Vector2[]> hulls1 = ImmutableArray.Create<Vector2[]>().AddRange(new[]
    {
        new Vector2[] {
            new Vector2(-1000f, 0f),
            new Vector2(-1000f, 1000f),
            new Vector2(1000f, 1000f),
            new Vector2(1000f, 0f),
            new Vector2(0f, -25f),
        }
    });

    public static Bullet NewBullet(uint bulletId, int originatedRenderFrameId, ulong offenderUd, int teamId, BulletState blState, int framesInBlState) {
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

    public static PlayerCharacterDownsync NewPreallocatedPlayerCharacterDownsync(int buffCapacity, int debuffCapacity, int inventoryCapacity, int bulletImmuneRecordCapacity) {
        var single = new PlayerCharacterDownsync();
        single.JoinIndex = primitives.MagicJoinIndexInvalid;
        single.Chd = NewPreallocatedCharacterDownsync(buffCapacity, debuffCapacity, inventoryCapacity, bulletImmuneRecordCapacity);
        return single;
    }

    public static NpcCharacterDownsync NewPreallocatedNpcCharacterDownsync(int buffCapacity, int debuffCapacity, int inventoryCapacity, int bulletImmuneRecordCapacity) {
        var single = new NpcCharacterDownsync();
        single.Id = primitives.TerminatingCharacterId;
        single.KilledToDropBuffSpeciesId = primitives.TerminatingBuffSpeciesId;
        single.KilledToDropConsumableSpeciesId = primitives.TerminatingConsumableSpeciesId;
        single.Chd = NewPreallocatedCharacterDownsync(buffCapacity, debuffCapacity, inventoryCapacity, bulletImmuneRecordCapacity);
        return single;
    }

    public static CharacterDownsync NewPreallocatedCharacterDownsync(int buffCapacity, int debuffCapacity, int inventoryCapacity, int bulletImmuneRecordCapacity) {
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

    public static RenderFrame NewPreallocatedRdf(int roomCapacity, int preallocNpcCount, int preallocBulletCount) {
        var ret = new RenderFrame();
        ret.Id = primitives.TerminatingRenderFrameId;
        ret.BulletIdCounter = 0;

        for (int i = 0; i < roomCapacity; i++) {
            var single = NewPreallocatedPlayerCharacterDownsync(primitives.DefaultPerCharacterBuffCapacity, primitives.DefaultPerCharacterDebuffCapacity, primitives.DefaultPerCharacterInventoryCapacity, primitives.DefaultPerCharacterImmuneBulletRecordCapacity);
            ret.PlayersArr.Add(single);
        }

        for (int i = 0; i < preallocNpcCount; i++) {
            var single = NewPreallocatedNpcCharacterDownsync(primitives.DefaultPerCharacterBuffCapacity, primitives.DefaultPerCharacterDebuffCapacity, 1, primitives.DefaultPerCharacterImmuneBulletRecordCapacity);
            ret.NpcsArr.Add(single);
        }

        for (int i = 0; i < preallocBulletCount; i++) {
            var single = NewBullet(primitives.TerminatingBulletId, 0, 0, 0, BulletState.StartUp, 0);
            ret.Bullets.Add(single);
        }


        return ret;
    }

    private RenderFrame mockStartRdf() {
        const int roomCapacity = 2;
        var startRdf = NewPreallocatedRdf(roomCapacity, 8, 128);
        startRdf.Id = primitives.StartingRenderFrameId;
        startRdf.ShouldForceResync = false;
        uint pickableLocalId = 1;
        uint npcLocalId = 1;
        uint bulletLocalId = 1;

        var player1 = startRdf.PlayersArr[0];
        var ch1 = player1.Chd;
        player1.JoinIndex = 1;
        ch1.X = 50f;
        ch1.Y = 2000f;
        player1.RevivalX = ch1.X;
        player1.RevivalY = ch1.Y;
        ch1.Speed = 10f;
        ch1.ChState = CharacterState.InAirIdle1NoJump;
        ch1.FramesToRecover = 0;
        ch1.DirX = 2;
        ch1.DirY = 0;
        ch1.VelX = 0;
        ch1.VelY = 0;
        ch1.Hp = 100;
        ch1.SpeciesId = PbPrimitives.SPECIES_BLADEGIRL;

        var player2 = startRdf.PlayersArr[1];
        var ch2 = player2.Chd;
        player2.JoinIndex = 2;
        ch2.X = -50f;
        ch2.Y = 2000f;
        player2.RevivalX = ch2.X;
        player2.RevivalY = ch2.Y;
        ch2.Speed = 10f;
        ch2.ChState = CharacterState.InAirIdle1NoJump;
        ch2.FramesToRecover = 0;
        ch2.DirX = 2;
        ch2.DirY = 0;
        ch2.VelX = 0;
        ch2.VelY = 0;
        ch2.Hp = 100;
        ch2.SpeciesId = PbPrimitives.SPECIES_BOUNTYHUNTER;

        startRdf.NpcIdCounter = npcLocalId;
        startRdf.BulletIdCounter = bulletLocalId;
        startRdf.PickableIdCounter = pickableLocalId;
        
        return startRdf;
    }


    [Fact]
    public unsafe void TestSimpleAllocDealloc() {
        bool dllLoaded = JoltcPrebuilt.Loader.LoadLibrary();
        if (dllLoaded) {
            _logger.WriteLine($"Loaded joltc.dll");
            JoltCSharp.Bindings.JPH_Init(10*1024*1024);
            _logger.WriteLine($"Initialized Jolt resource allocators");

            var primitivesBytes = PbPrimitives.underlying.ToByteArray();
            fixed (byte* primitivesBytesPtr = primitivesBytes) {
                Bindings.PrimitiveConsts_Init((char*)primitivesBytesPtr, primitivesBytes.Length);
                _logger.WriteLine($"PrimitiveConsts_Init done");
            }

            var configConsts = new ConfigConsts { };
            configConsts.CharacterConfigs.Add(PbCharacters.underlying);
            var configsBytes = configConsts.ToByteArray();
            fixed (byte* configsBytesPtr = configsBytes) {
                Bindings.ConfigConsts_Init((char*)configsBytesPtr, configsBytes.Length);
                _logger.WriteLine($"ConfigConsts_Init done");
            }

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
                 battle = Bindings.APP_CreateBattle((char*)bufferPtr, buffer.Length, true, false);
                 _logger.WriteLine($"Created battle at pointer addr = 0x{battle:x}");
            }
            Assert.NotEqual(UIntPtr.Zero, battle);
            
            int timerRdfId = primitives.StartingRenderFrameId;
            int outBytesCnt = 0;
            fixed (byte* ifdFetchBufferPtr = ifdFetchBuffer) 
            fixed (byte* rdfFetchBufferPtr = rdfFetchBuffer) {
                while (4096 > timerRdfId) {
                    int* outBytesCntPtr = &outBytesCnt;
                    *outBytesCntPtr = pbBufferSizeLimit;
                    var noDelayIfdId = (timerRdfId >> primitives.InputScaleFrames);
                    bool cmdInjected = Bindings.APP_UpsertCmd(battle, noDelayIfdId, 1, 0, (char*)ifdFetchBufferPtr, outBytesCntPtr, true, false, true);
                    Assert.True(cmdInjected);
                    InputFrameDownsync ifdHolder = InputFrameDownsync.Parser.ParseFrom(ifdFetchBuffer, 0, *outBytesCntPtr);

                    bool stepped = Bindings.APP_Step(battle, timerRdfId, timerRdfId + 1, true);
                    Assert.True(stepped);
                    
                    timerRdfId++;

                    *outBytesCntPtr = pbBufferSizeLimit;
                    bool rdfFetched = Bindings.APP_GetRdf(battle, timerRdfId, (char*)rdfFetchBufferPtr, outBytesCntPtr);
                    Assert.True(rdfFetched);
                    RenderFrame rdfHolder = RenderFrame.Parser.ParseFrom(rdfFetchBuffer, 0, *outBytesCntPtr);
                    _logger.WriteLine(rdfHolder.ToString());
                }
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
