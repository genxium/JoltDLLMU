namespace joltphysics.Tests;

using Google.Protobuf;
using Google.Protobuf.Collections;
using JoltCSharp;
using jtshared;
using System;
using System.Collections.Immutable;
using System.Numerics;
using Xunit.Abstractions;

public class FrontendTest {
    // Reference https://xunit.net/docs/capturing-output.html

    private readonly ITestOutputHelper _logger;
    private static PrimitiveConsts primitives = PbPrimitives.underlying;
    private static MapField<uint, CharacterConfig> characters = PbCharacters.underlying;

    const int pbBufferSizeLimit = (1 << 14);
    byte[] rdfFetchBuffer;

    public FrontendTest(ITestOutputHelper theLogger) {
        _logger = theLogger;
        rdfFetchBuffer = new byte[pbBufferSizeLimit];
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


    private RenderFrame mockStartRdf() {
        const int roomCapacity = 2;
        var startRdf = Bindings.NewPreallocatedRdf(roomCapacity, 8, 128, primitives);
        startRdf.Id = primitives.StartingRenderFrameId;
        uint pickableIdCounter = 1;
        uint npcIdCounter = 1;
        uint bulletIdCounter = 1;

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

        startRdf.NpcIdCounter = npcIdCounter;
        startRdf.BulletIdCounter = bulletIdCounter;
        startRdf.PickableIdCounter = pickableIdCounter;
        
        return startRdf;
    }

    [Fact]
        public unsafe void TestSimpleAllocDealloc() {
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
                battle = Bindings.FRONTEND_CreateBattle(512, false);
                _logger.WriteLine($"Created battle at pointer addr = 0x{battle:x}");
                uint selfJoinIndex = 1;
                string selfPlayerId = "foobar";
                int selfCmdAuthKey = 123456;
                bool res = Bindings.FRONTEND_ResetStartRdf(battle, (char*)bufferPtr, buffer.Length, selfJoinIndex, selfPlayerId, selfCmdAuthKey);
                _logger.WriteLine($"ResetStartRdf finished for battle at pointer addr = 0x{battle:x}, res={res}");
            }
            Assert.NotEqual(UIntPtr.Zero, battle);

            int timerRdfId = primitives.StartingRenderFrameId, newChaserRdfId = 0;
            long outBytesCnt = 0;
            RenderFrame rdfHolder = new RenderFrame();
            int* newChaserRdfIdPtr = &newChaserRdfId;
            fixed (byte* rdfFetchBufferPtr = rdfFetchBuffer) {
                while (4096 > timerRdfId) {
                    bool cmdInjected = Bindings.FRONTEND_UpsertSelfCmd(battle, 0, newChaserRdfIdPtr);
                    Assert.True(cmdInjected);

                    Bindings.FRONTEND_Step(battle, timerRdfId, timerRdfId + 1, false);

                    int chaserRdfId = -1, chaserRdfIdLowerBound = -1, oldLcacIfdId = -1, toGenIfdId = -1, localRequiredIfdId = -1;
                    Bindings.FRONTEND_GetRdfAndIfdIds(battle, &timerRdfId, &chaserRdfId, &chaserRdfIdLowerBound, &oldLcacIfdId, &toGenIfdId, &localRequiredIfdId);

                    long* outBytesCntPtr = &outBytesCnt;
                    *outBytesCntPtr = pbBufferSizeLimit;
                    bool rdfFetched = Bindings.APP_GetRdf(battle, timerRdfId, (char*)rdfFetchBufferPtr, outBytesCntPtr);
                    Assert.True(rdfFetched);
                    Bindings.PreemptRenderFrameBeforeMerge(rdfHolder, primitives);
                    rdfHolder.MergeFrom(rdfFetchBuffer, 0, (int)(*outBytesCntPtr));
                    _logger.WriteLine(rdfHolder.PlayersArr.ToString());
                }
            }

            // Clean up
            Bindings.APP_ClearBattle(battle);
            bool destroyRes = Bindings.APP_DestroyBattle(battle);
            Assert.True(destroyRes);
            _logger.WriteLine($"Destroyed battle at pointer addr = 0x{battle:x} with result={destroyRes}");
            bool shutdownRes = Bindings.JPH_Shutdown();
            _logger.WriteLine($"Jolt infra shutdown result = {shutdownRes}");
        }
}
