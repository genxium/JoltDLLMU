using AOT;
using Google.Protobuf;
using Google.Protobuf.Collections;
using JoltCSharp;
using jtshared;
using SuperTiled2Unity;
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using UnityEngine;
using UnityEngine.Assertions;
using static FrontendOnlyGeometry;
using static JoltCSharp.Bindings;

public abstract class AbstractJoltMapController : MonoBehaviour {

    protected const uint KV_PREFIX_SFX_FT = (1 << 7); // 128; // Footstep, jumping, landing, pickable
    protected const uint KV_PREFIX_SFX_CH_EMIT = (KV_PREFIX_SFX_FT << 1);
    protected const uint KV_PREFIX_SFX_BULLET_ACTIVE = (KV_PREFIX_SFX_CH_EMIT << 1);
    protected const uint KV_PREFIX_SFX_BULLET_EXPLODE = (KV_PREFIX_SFX_BULLET_ACTIVE << 1);

    protected const uint KV_PREFIX_VFX = (1 << 7);
    protected const uint KV_PREFIX_VFX_DEF = (KV_PREFIX_VFX << 1);
    protected const uint KV_PREFIX_VFX_CHARGE = (KV_PREFIX_VFX_DEF << 1);
    protected const uint KV_PREFIX_VFX_CH_EMIT = (KV_PREFIX_VFX_CHARGE << 1);
    protected const uint KV_PREFIX_VFX_CH_ELE_DEBUFF = (KV_PREFIX_VFX_CH_EMIT << 1);

    public Material chMaterial;
    public Material blMaterial;
    public GameObject damageIndicatorPrefab;
    public GameObject inplaceHpBarPrefab;
    public GameObject aimingRayPrefab;
    public GameObject errStackLogPanelPrefab;
    public GameObject teamRibbonPrefab;
    public GameObject keyChPointerPrefab;
    public GameObject sfxSourcePrefab;
    protected GameObject errStackLogPanelObj;
    protected GameObject underlyingMap;
    public Canvas canvas;
    public Toast toast;

    protected uint selfJoinIndex = 0;
    public uint GetSelfJoinIndex() {
        return selfJoinIndex;
    }

    protected int selfJoinIndexInt = 0;
    protected int selfJoinIndexArrIdx = - 1;
    protected ulong selfJoinIndexMask = 0;
    protected string selfPlayerId = null;
    protected int selfCmdAuthKey = 0;
    protected int newChaserRdfId = 0;

    protected long battleState;
    protected float tilemapHalfWidth, tilemapHalfHeight;
    protected int collisionSpaceHalfWidth, collisionSpaceHalfHeight;
    public float GetTilemapHalfWidth() {
        return tilemapHalfWidth;
    }
    public float GetTilemapHalfHeight() {
        return tilemapHalfHeight;
    }

    protected float collisionSpacePaddingLeft, collisionSpacePaddingBottom;
    public float GetCollisionSpacePaddingLeft() {
        return collisionSpacePaddingLeft;
    }
    public float GetCollisionSpacePaddingBottom() {
        return collisionSpacePaddingBottom;
    }

    protected float cameraCapMinX, cameraCapMaxX, cameraCapMinY, cameraCapMaxY;
    protected float effInftyFar, npcInterpolationDis2Threshold;

    protected Dictionary<int, BattleResult> unconfirmedBattleResult;
    protected Dictionary<ulong, TriggerConfigFromTiled> triggerUdToConfigFromTiled;
    protected Dictionary<ulong, int> characterUdToColorSwapRuleLock;
    protected Dictionary<uint, int> playerSpeciesIdOccurrenceCnt;
    protected Vector3[] debugDrawPositionsHolder = new Vector3[4]; // Currently only rectangles are drawn

    protected JoltCharacterAnimPool playerAnimPool = null;
    protected JoltCharacterAnimPool npcAnimPool = null;
    protected JoltBulletAnimPool bulletAnimPool = null;
    protected JoltTrapAnimPool trapAnimPool = null;
    protected JoltTriggerAnimPool triggerAnimPool = null;
    protected JoltInplaceHpBarAnimPool inplaceHpBarAnimPool = null;
    protected JoltAimingRayAnimPool aimingRayAnimPool = null;
    protected JoltPickableAnimPool pickableAnimPool = null;

    protected bool shouldDetectRealtimeRenderHistoryCorrection = false; // Not recommended to enable in production, it might have some memory performance impact.
    protected bool frameLogEnabled = false;

    protected KvPriorityQueue<int, SFXSource> cachedSfxNodes;
    protected KvPriorityQueue<int, SFXSource>.ValScore sfxNodeScore = (x) => x.score;
    public BGMSource bgmSource;
    public Camera gameplayCamera;

    protected float defaultGameplayCamZ = -10;
    protected float lineRendererZ = +5;
    protected float trapZ = 0;
    protected float triggerZ = 0;
    protected float pickableZ = 0;
    protected float characterZ = 0;
    protected float flyingCharacterZ = -1;
    protected float inplaceHpBarZ = +10;
    protected float bulletZ = -5;
    protected float footstepAttenuationZ = 200.0f;

    private string MATERIAL_REF_THICKNESS = "_Thickness";

    protected bool hideInplaceHpBars = false;
    
    protected WsReq cachedWsReqForStartRdf = new WsReq();

    public static PrimitiveConsts primitives = PbPrimitivesOverride.Instance.getUnderlying();
    protected static MapField<uint, CharacterConfig> characters = PbCharactersOverride.Instance.getUnderlying();

    protected const int pbBufferSizeLimit = (1 << 14);
    protected byte[] rdfFetchBuffer;
    protected byte[] ifdFetchBuffer;
    protected byte[] stepResultFetchBuffer;

    protected RenderFrame rdfHolder = new RenderFrame();
    protected InputFrameDownsync ifdHolder = new InputFrameDownsync();
    protected StepResult stepResultHolder = new StepResult();

    protected int roomCapacity = 0;
    protected int preallocNpcCapacity = 32;
    protected int preallocBulletCapacity = 128;
    protected int battleDurationSeconds = 0, battleDurationFrames = 0;
    protected UIntPtr battle = UIntPtr.Zero;
    protected PlayerMetaInfo selfPlayerInfo = null;
    protected int settlementRdfId = -1;
    protected int csharpTimerRdfId = -1;
    public SelfBattleHeading selfBattleHeading;
    public FocusLossPrompt focusLossPrompt;

    public AbstractJoltMapController() {
        rdfFetchBuffer = new byte[pbBufferSizeLimit];
        ifdFetchBuffer = new byte[pbBufferSizeLimit];
        stepResultFetchBuffer = new byte[pbBufferSizeLimit];
    }

    enum DColor { red, yellow, orange, green, blue, black, white };

    [MonoPInvokeCallback(typeof(debugCallback))]
    static void OnDebugCallback(IntPtr request, int color, int size) {
        // Ptr to string
        string rawMsg = Marshal.PtrToStringAnsi(request, size);

        // Add Specified Color
        var coloredMsg = $"<color={((DColor)color)}>{rawMsg}</color>";
        Debug.Log(coloredMsg);
    }

    public BattleInputManager iptmgr;

    public static float InvSqrt32(float x) {
        float xhalf = 0.5f * x;
        int i = BitConverter.SingleToInt32Bits(x);
        i = 0x5f3759df - (i >> 1);
        x = BitConverter.Int32BitsToSingle(i);
        x = x * (1.5f - xhalf * x * x);
        return x;
    }

    public static double InvSqrt64(double x) {
        double xhalf = 0.5 * x;
        long i = BitConverter.DoubleToInt64Bits(x);
        i = 0x5fe6eb50c7b537a9 - (i >> 1);
        x = BitConverter.Int64BitsToDouble(i);
        x = x * (1.5 - xhalf * x * x);
        return x;
    }
    
    protected unsafe virtual void Start() {
        JPH_Init(10 * 1024 * 1024);
        RegisterDebugCallback(OnDebugCallback);
        Debug.Log($"Initialized Jolt resource allocators");

        var primitivesBytes = PbPrimitivesOverride.Instance.getUnderlying().ToByteArray();
        fixed (byte* primitivesBytesPtr = primitivesBytes) {
            PrimitiveConsts_Init((char*)primitivesBytesPtr, primitivesBytes.Length);
            Debug.Log($"PrimitiveConsts_Init done");
        }
        if (preallocNpcCapacity < primitives.DefaultPreallocNpcCapacity) {
            preallocNpcCapacity = primitives.DefaultPreallocNpcCapacity;
        }
        if (preallocBulletCapacity < primitives.DefaultPreallocBulletCapacity) {
            preallocBulletCapacity = primitives.DefaultPreallocBulletCapacity;
        }

        var configConsts = new ConfigConsts { };
        configConsts.CharacterConfigs.Add(PbCharactersOverride.Instance.getUnderlying());
        configConsts.SkillConfigs.Add(PbSkillsOverride.Instance.getUnderlying());
        configConsts.TriggerConfigs.Add(PbTriggersOverride.Instance.getUnderlying());
        configConsts.TrapConfigs.Add(PbTrapsOverride.Instance.getUnderlying());
        configConsts.PickableConfigs.Add(PbPickablesOverride.Instance.getUnderlying());
        var configsBytes = configConsts.ToByteArray();
        fixed (byte* configsBytesPtr = configsBytes) {
            ConfigConsts_Init((char*)configsBytesPtr, configsBytes.Length);
            Debug.Log($"ConfigConsts_Init done");
        }

        Physics2D.simulationMode = SimulationMode2D.Script;
        Application.targetFrameRate = PbPrimitivesOverride.BATTLE_DYNAMICS_FPS;
        Application.SetStackTraceLogType(LogType.Log, StackTraceLogType.None);
        Application.SetStackTraceLogType(LogType.Warning, StackTraceLogType.None);
        Application.SetStackTraceLogType(LogType.Error, StackTraceLogType.None);
        Application.SetStackTraceLogType(LogType.Exception, StackTraceLogType.None);

        iptmgr.SetMap(this);
    }

    protected virtual void OnDestroy() {
        // clean up
        onBattleStopped();
        if (UIntPtr.Zero != battle) {
            APP_DestroyBattle(battle);
            battle = UIntPtr.Zero;
        }
        
        bool shutdownRes = JPH_Shutdown();
        Debug.Log($"Jolt infra shutdown result = {shutdownRes}");
    }

    protected void preallocateFrontendOnlyHolders(int specifiedLayer = -1) {
        //---------------------------------------------FRONTEND USE ONLY SEPERARTION---------------------------------------------
        triggerUdToConfigFromTiled = new Dictionary<ulong, TriggerConfigFromTiled>();
        characterUdToColorSwapRuleLock = new Dictionary<ulong, int>();
        playerSpeciesIdOccurrenceCnt = new Dictionary<uint, int>();

        playerAnimPool = new JoltCharacterAnimPool(this);
        npcAnimPool = new JoltCharacterAnimPool(this);
        bulletAnimPool = new JoltBulletAnimPool(this);
        trapAnimPool = new JoltTrapAnimPool(this);
        triggerAnimPool = new JoltTriggerAnimPool(this);
        inplaceHpBarAnimPool = new JoltInplaceHpBarAnimPool(this);
        aimingRayAnimPool = new JoltAimingRayAnimPool(this);
        pickableAnimPool = new JoltPickableAnimPool(this);

        playerAnimPool.ResetUponBattlePreparation();
        npcAnimPool.ResetUponBattlePreparation();
        bulletAnimPool.ResetUponBattlePreparation();
        trapAnimPool.ResetUponBattlePreparation();
        triggerAnimPool.ResetUponBattlePreparation();
        inplaceHpBarAnimPool.ResetUponBattlePreparation();
        aimingRayAnimPool.ResetUponBattlePreparation();
        pickableAnimPool.ResetUponBattlePreparation();
    }

    protected int camFovW, camFovH, camPaddingX, camPaddingY;
    protected void calcCameraCaps() {
        cameraCapMinX = 0;
        cameraCapMaxX = tilemapHalfWidth + tilemapHalfWidth;
        cameraCapMinY = -tilemapHalfHeight -tilemapHalfHeight;
        cameraCapMaxY = 0;
        var grid = underlyingMap.GetComponentInChildren<Grid>();
        foreach (Transform child in grid.transform) {
            switch (child.gameObject.name) {
                case "GrandCameraBounds": {
                        foreach (Transform camBoundChild in child) {
                            var tileObj = camBoundChild.GetComponent<SuperObject>();
                            var (candidateCamMinTileX, candidateCamMinY) = (tileObj.m_X, tileObj.m_Y + tileObj.m_Height);
                            var (candidateCamMaxTileX, candidateCamMaxY) = (tileObj.m_X + tileObj.m_Width, tileObj.m_Y);

                            var (candidateCamMinWx, candidateCamMinWy) = TiledLayerPositionToWorldPosition(candidateCamMinTileX, candidateCamMinY);
                            var (candidateCamMaxWx, candidateCamMaxWy) = TiledLayerPositionToWorldPosition(candidateCamMaxTileX, candidateCamMaxY);

                            collisionSpaceHalfWidth = (int)((candidateCamMaxWx - candidateCamMinWx) * .5f);
                            collisionSpaceHalfHeight = (int)((candidateCamMaxWy - candidateCamMinWy) * .5f);
                            collisionSpacePaddingLeft = candidateCamMinWx;
                            collisionSpacePaddingBottom = collisionSpaceHalfHeight + collisionSpaceHalfHeight + candidateCamMinWy;
                            
                            // [WARNING] The inequality checks below are NOT typos! They're meant to shrink the default camera range! 
                            if (candidateCamMinWx > cameraCapMinX) {
                                cameraCapMinX = candidateCamMinWx;
                            }
                            if (candidateCamMaxWx < cameraCapMaxX) {
                                cameraCapMaxX = candidateCamMaxWx;
                            }
                            if (candidateCamMinWy > cameraCapMinY) {
                                cameraCapMinY = candidateCamMinWy;
                            }
                            if (candidateCamMaxWy < cameraCapMaxY) {
                                cameraCapMaxY = candidateCamMaxWy;
                            }
                        }
                    }
                    break;
            }
        }

        camFovW = (int)(2.0f * gameplayCamera.orthographicSize * gameplayCamera.aspect);
        camFovH = (int)(2.0f * gameplayCamera.orthographicSize);
        camPaddingX = (camFovW >> 1);
        camPaddingY = (camFovH >> 1);
        cameraCapMinX += camPaddingX;
        cameraCapMaxX -= camPaddingX;

        cameraCapMinY += camPaddingY;
        cameraCapMaxY -= camPaddingY;

        effInftyFar = 4f * Math.Max(tilemapHalfWidth, tilemapHalfHeight);
        npcInterpolationDis2Threshold = (effInftyFar * 0.125f) * (effInftyFar * 0.125f);
        playerAnimPool.SetGeometricConsts(effInftyFar, characterZ);
        npcAnimPool.SetGeometricConsts(effInftyFar, characterZ);
        bulletAnimPool.SetGeometricConsts(effInftyFar, bulletZ);
        trapAnimPool.SetGeometricConsts(effInftyFar, trapZ);
        triggerAnimPool.SetGeometricConsts(effInftyFar, triggerZ);
        inplaceHpBarAnimPool.SetGeometricConsts(effInftyFar, inplaceHpBarZ);
        pickableAnimPool.SetGeometricConsts(effInftyFar, pickableZ);

        selfBattleHeading.ResetSelf();
    }

    private const uint TiledHorizontalFlipFlag = 0x80000000;
    protected bool isXFlipped(uint superTileId) {
        return 0 < (superTileId & TiledHorizontalFlipFlag);
    }

    protected string ArrToString(uint[] speciesIdList) {
        var ret = "";
        for (int i = 0; i < speciesIdList.Length; i++) {
            ret += speciesIdList[i].ToString();
            if (i < speciesIdList.Length - 1) ret += ", ";
        }
        return ret;
    }

    public GameObject loadCharacterPrefab(CharacterConfig chConfig) {
        string path = $"JoltChPrefabs/{chConfig.SpeciesName}";
        return Resources.Load(path) as GameObject;
    }

    public GameObject loadBulletPrefab(BulletConfig bulletConfig) {
        string path = $"JoltBulletPrefabs/{bulletConfig.AnimName}";
        return Resources.Load(path) as GameObject;
    }

    public GameObject loadTrapPrefab(TrapConfig trapConfig) {
        string path = $"JoltTpPrefabs/{trapConfig.Name}";
        return Resources.Load(path) as GameObject;
    }

    public GameObject loadTriggerPrefab(TriggerConfigFromTiled triggerConfigFromTiled) {
        var triggerConfig = PbTriggersOverride.Instance.getUnderlying()[triggerConfigFromTiled.Trt];
        string path = $"JoltTrPrefabs/{triggerConfig.Name}";
        return Resources.Load(path) as GameObject;
    }

    public GameObject loadPickablePrefab(PickableConfig pickableConfig) {
        var pkConfig = PbPickablesOverride.Instance.getUnderlying()[pickableConfig.PickupType];
        string path = $"JoltPkPrefabs/{pkConfig.ActiveAnimName}";
        return Resources.Load(path) as GameObject;
    }

    public GameObject loadInplaceHpBarPrefab(CharacterConfig chConfig) {
        return inplaceHpBarPrefab;
    }

    public GameObject loadAimingRayPrefab() {
        return aimingRayPrefab;
    }


    protected virtual void onBattleStopped() {
        enableBattleInput(false);
        if (null != triggerUdToConfigFromTiled) {
            triggerUdToConfigFromTiled.Clear();
        }
        if (UIntPtr.Zero != battle) {
            APP_ClearBattle(battle);
        }
        battleState = PbPrimitivesOverride.ROOM_STATE_STOPPED;
    }
    
    protected void enableBattleInput(bool yesOrNo) {
        if (null == iptmgr) return;
        if (false == yesOrNo && !iptmgr.gameObject.activeSelf) return;
        if (iptmgr.enable(yesOrNo)) {
            iptmgr.gameObject.SetActive(yesOrNo);
        }
    }

    protected void patchStartRdf(RenderFrame srcRdf, RenderFrame dstRdf) {
        for (int i = 0; i < roomCapacity; i++) {
            if (selfPlayerInfo.JoinIndex == i + 1) {
                continue;
            }
            var srcPlayer = srcRdf.Players[i];
            if (PbPrimitivesOverride.SPECIES_NONE_CH == srcPlayer.Chd.SpeciesId) continue;

            var dstPlayer = dstRdf.Players[i];

            Bindings.PreemptPlayerCharacterDownsyncBeforeMerge(dstPlayer, PbPrimitivesOverride.Instance.getUnderlying());
            dstPlayer.MergeFrom(srcPlayer);
        }
    }

    protected WsReq createSelfParsedStartRdf(uint[] speciesIdList) {
        Debug.Log($"createSelfParsedStartRdf with speciesIdList={ArrToString(speciesIdList)} for selfJoinIndex={selfJoinIndex}");
        triggerUdToConfigFromTiled.Clear();

        var grid = underlyingMap.GetComponentInChildren<Grid>();
        uint pickableIdCounter = 1;
        uint npcIdCounter = 1;
        uint dynamicTrapCount = 0;
        uint triggerCount = 0;

        var mapProps = underlyingMap.GetComponent<SuperCustomProperties>();
        CustomProperty battleDurationSecsProp;
        mapProps.TryGetCustomProperty("battleDurationSeconds", out battleDurationSecsProp);
        battleDurationSeconds = (null == battleDurationSecsProp || battleDurationSecsProp.IsEmpty) ? 60 : battleDurationSecsProp.GetValueAsInt();

        battleDurationFrames = battleDurationSeconds * PbPrimitivesOverride.BATTLE_DYNAMICS_FPS;
        WsReq result = new WsReq();
        var startRdf = NewPreallocatedRdf(roomCapacity, preallocNpcCapacity, 128, primitives);
        startRdf.BulletIdCounter = 1;
        startRdf.Id = primitives.StartingRenderFrameId;

        foreach (Transform child in grid.transform) {
            switch (child.gameObject.name) {
                case "Barrier":
                    foreach (Transform barrierChild in child) {
                        var barrierTileObj = barrierChild.GetComponent<SuperObject>();
                        var inMapCollider = barrierChild.GetComponent<EdgeCollider2D>();
                        var tileProps = barrierChild.GetComponent<SuperCustomProperties>();

                        CustomProperty providesSlipJump;
                        tileProps.TryGetCustomProperty("providesSlipJump", out providesSlipJump);

                        bool providesSlipJumpVal = null == providesSlipJump || providesSlipJump.IsEmpty ? false : (0 < providesSlipJump.GetValueAsInt());

                        if (null == inMapCollider || 0 >= inMapCollider.pointCount) {
                            var (tiledRectCenterX, tiledRectCenterY) = (barrierTileObj.m_X + barrierTileObj.m_Width * 0.5f, barrierTileObj.m_Y + barrierTileObj.m_Height * 0.5f);
                            var (rectCx, rectCy) = TiledLayerPositionToCollisionSpacePosition(tiledRectCenterX, tiledRectCenterY, tilemapHalfHeight, collisionSpacePaddingLeft, collisionSpacePaddingBottom);
                            /*
                             [WARNING] 

                            The "Unity World (0, 0)" is aligned with the top-left corner of the rendered "TiledMap (via SuperMap)".

                            It's noticeable that all the "Collider"s in "CollisionSpace" must be of positive coordinates to work due to the implementation details of "resolv". Thus I'm using a "Collision Space (0, 0)" aligned with the bottom-left of the rendered "TiledMap (via SuperMap)". 
                            */
                            List<PbVec2> points2 = new List<PbVec2>(); 
                            // upper-left
                            points2.Add(new PbVec2 {
                                X = rectCx - .5f * barrierTileObj.m_Width,
                                Y = rectCy + .5f * barrierTileObj.m_Height
                            });

                            // upper-right
                            points2.Add(new PbVec2 {
                                X = rectCx + .5f * barrierTileObj.m_Width,
                                Y = rectCy + .5f * barrierTileObj.m_Height
                            });

                            // bottom-right
                            points2.Add(new PbVec2 {
                                X = rectCx + .5f * barrierTileObj.m_Width,
                                Y = rectCy - .5f * barrierTileObj.m_Height
                            });

                            // bottom-left
                            points2.Add(new PbVec2 {
                                X = rectCx - .5f * barrierTileObj.m_Width,
                                Y = rectCy - .5f * barrierTileObj.m_Height
                            });

                            var srcPolygon = new SerializableConvexPolygon {
                                Anchor = new PbVec2 {
                                    X = rectCx,
                                    Y = rectCy
                                },
                                IsBox = true
                            };
                            srcPolygon.Points.Add(points2);
                            var barrierCollider = new SerializedBarrierCollider {
                                Polygon = srcPolygon,
                                Attr = new BarrierColliderAttr {
                                    ProvidesSlipJump = providesSlipJumpVal
                                }
                            };
                            result.SerializedBarriers.Add(barrierCollider);
                        } else {
                            var points = inMapCollider.points;
                            List<PbVec2> points2 = new List<PbVec2>();
                            foreach (var point in points) {
                                points2.Add(new PbVec2 {
                                    X = point.x,
                                    Y = point.y,
                                });
                            }
                            var (anchorCx, anchorCy) = TiledLayerPositionToCollisionSpacePosition(barrierTileObj.m_X, barrierTileObj.m_Y, tilemapHalfHeight, collisionSpacePaddingLeft, collisionSpacePaddingBottom);
                            var srcPolygon = new SerializableConvexPolygon {
                                Anchor = new PbVec2 {
                                    X = anchorCx,
                                    Y = anchorCy
                                }
                            };
                            srcPolygon.Points.Add(points2);
                            var barrierCollider = new SerializedBarrierCollider {
                                Polygon = srcPolygon,
                                Attr = new BarrierColliderAttr {
                                    ProvidesSlipJump = providesSlipJumpVal
                                }
                            };
                            result.SerializedBarriers.Add(barrierCollider);
                        }

                        // TODO: By now I have to enable the import of all colliders to see the "inMapCollider: EdgeCollider2D" component, then remove unused components here :(
                        Destroy(barrierChild.GetComponent<EdgeCollider2D>());
                        Destroy(barrierChild.GetComponent<BoxCollider2D>());
                        Destroy(barrierChild.GetComponent<SuperColliderComponent>());
                    }
                    Destroy(child.gameObject); // Delete the whole "ObjectLayer"
                    break;
                case "PlayerStartingPos":
                    foreach (Transform playerPos in child) {
                        var posTileObj = playerPos.gameObject.GetComponent<SuperObject>();
                        var tileProps = playerPos.gameObject.gameObject.GetComponent<SuperCustomProperties>();
                        CustomProperty teamId, dirX;
                        tileProps.TryGetCustomProperty("teamId", out teamId);
                        tileProps.TryGetCustomProperty("dirX", out dirX);
                        var teamIdVal = null == teamId || teamId.IsEmpty ? primitives.TerminatingBulletTeamId: teamId.GetValueAsInt();
                        int j = teamIdVal - 1;
                        if (j >= speciesIdList.Length) {
                            continue;
                        }
                        if (teamIdVal != selfPlayerInfo.JoinIndex && speciesIdList[j] == PbPrimitivesOverride.SPECIES_NONE_CH) {
                            continue; // [TODO] Use a dedicated "joinIndex" property!
                        }
                        var dirXVal = null == dirX || dirX.IsEmpty ? +2 : dirX.GetValueAsInt();
                        var (cx, cy) = TiledLayerPositionToCollisionSpacePosition(posTileObj.m_X, posTileObj.m_Y, tilemapHalfHeight, collisionSpacePaddingLeft, collisionSpacePaddingBottom);
                        var chConfig = characters[speciesIdList[j]];
                        var playerChd = startRdf.Players[j];
                        var chd = playerChd.Chd;
                        uint joinIndex = (uint)(j + 1);
                        playerChd.JoinIndex = joinIndex;
                        chd.X = cx;
                        chd.Y = cy;
                        playerChd.RevivalX = chd.X;
                        playerChd.RevivalY = chd.Y;
                        chd.Speed = chConfig.Speed;
                        chd.RemainingAirJumpQuota = chConfig.DefaultAirJumpQuota;
                        chd.RemainingAirDashQuota = chConfig.DefaultAirDashQuota;
                        chd.ChState = CharacterState.InAirIdle1NoJump;
                        chd.FramesToRecover = 0;
                        if (0 < dirXVal) {
                            chd.QX = 0;
                            chd.QY = 0;
                            chd.QZ = 0;
                            chd.QW = 1;
                            playerChd.RevivalQX = 0;
                            playerChd.RevivalQY = 0;
                            playerChd.RevivalQZ = 0;
                            playerChd.RevivalQW = 1;
                        } else {
                            chd.QX = 0;
                            chd.QY = 1;
                            chd.QZ = 0;
                            chd.QW = 0;
                            playerChd.RevivalQX = 0;
                            playerChd.RevivalQY = 1;
                            playerChd.RevivalQZ = 0;
                            playerChd.RevivalQW = 0;
                        }
                        chd.AimingQX = 0;
                        chd.AimingQY = 0;
                        chd.AimingQZ = 0;
                        chd.AimingQW = 1;
                        chd.VelX = 0;
                        chd.VelY = 0;
                        chd.BulletTeamId = teamIdVal;
                        chd.Hp = chConfig.Hp;
                        chd.Mp = chConfig.Mp;
                        chd.SpeciesId = chConfig.SpeciesId;
                    }
                    Destroy(child.gameObject); // Delete the whole "ObjectLayer"
                    break;
                case "NpcStartingPos":
                    foreach (Transform npcPos in child) {
                        var tileObj = npcPos.gameObject.GetComponent<SuperObject>();
                        var tileProps = npcPos.gameObject.gameObject.GetComponent<SuperCustomProperties>();
                        var (cx, cy) = TiledLayerPositionToCollisionSpacePosition(tileObj.m_X, tileObj.m_Y, tilemapHalfHeight, collisionSpacePaddingLeft, collisionSpacePaddingBottom);
                        if (0 != tileObj.m_Width) {
                            var (tiledRectCenterX, tiledRectCenterY) = (tileObj.m_X + tileObj.m_Width * 0.5f, tileObj.m_Y - tileObj.m_Height * 0.5f);
                            (cx, cy) = TiledLayerPositionToCollisionSpacePosition(tiledRectCenterX, tiledRectCenterY, tilemapHalfHeight, collisionSpacePaddingLeft, collisionSpacePaddingBottom);
                        }

                        CustomProperty speciesId, teamId, initGoal, publishingToTriggerIdUponExhausted, subscribesToTriggerId, exhaustedToDropConsumableSpeciesId, exhaustedToDropBuffSpeciesId, exhaustedToDropPickupSkillId, isMainTowerOfTeam;
                        tileProps.TryGetCustomProperty("speciesId", out speciesId);
                        tileProps.TryGetCustomProperty("teamId", out teamId);
                        tileProps.TryGetCustomProperty("initGoal", out initGoal);
                        tileProps.TryGetCustomProperty("publishingToTriggerIdUponExhausted", out publishingToTriggerIdUponExhausted);
                        tileProps.TryGetCustomProperty("subscribesToTriggerId", out subscribesToTriggerId);
                        tileProps.TryGetCustomProperty("exhaustedToDropConsumableSpeciesId", out exhaustedToDropConsumableSpeciesId);
                        tileProps.TryGetCustomProperty("exhaustedToDropBuffSpeciesId", out exhaustedToDropBuffSpeciesId);
                        tileProps.TryGetCustomProperty("exhaustedToDropPickupSkillId", out exhaustedToDropPickupSkillId);
                        tileProps.TryGetCustomProperty("isMainTowerOfTeam", out isMainTowerOfTeam);

                        uint speciesIdVal = null == speciesId || speciesId.IsEmpty ? primitives.ChSpecies.None : (uint)speciesId.GetValueAsInt();
                        bool isMainTowerOfTeamVal = null == isMainTowerOfTeam || isMainTowerOfTeam.IsEmpty ? false : (0 < isMainTowerOfTeam.GetValueAsInt());

                        NpcGoal initGoalVal = NpcGoal.Nidle;
                        if (null != initGoal && !initGoal.IsEmpty) {
                            var initGoalStr = initGoal.GetValueAsString();
                            Enum.TryParse(initGoalStr, out initGoalVal);
                        }

                        bool xFlipped = isXFlipped(tileObj.m_TileId);
                        var teamIdVal = null == teamId || teamId.IsEmpty ? primitives.TerminatingBulletTeamId: teamId.GetValueAsInt();

                        var chConfig = characters[speciesIdVal];
                        int npcArrIdx = (int)(npcIdCounter - 1);
                        var npc = startRdf.Npcs[npcArrIdx];
                        npc.Id = npcIdCounter++;
                        npc.GoalAsNpc = initGoalVal;
                        npc.PublishingToTriggerIdUponExhausted = (null == publishingToTriggerIdUponExhausted || publishingToTriggerIdUponExhausted.IsEmpty ? 0u : (uint)publishingToTriggerIdUponExhausted.GetValueAsInt());
                        npc.SubscribesToTriggerId = (null == subscribesToTriggerId || subscribesToTriggerId.IsEmpty ? primitives.TerminatingTriggerId : (uint)subscribesToTriggerId.GetValueAsInt()); 

                        npc.IsMainTowerOfTeam = isMainTowerOfTeamVal;

                        var chd = npc.Chd;
                        chd.X = cx;
                        chd.Y = cy;
                        chd.Speed = chConfig.Speed;
                        chd.RemainingAirJumpQuota = chConfig.DefaultAirJumpQuota;
                        chd.RemainingAirDashQuota = chConfig.DefaultAirDashQuota;
                        chd.ChState = CharacterState.InAirIdle1NoJump;
                        chd.FramesToRecover = 0;
                        if (!xFlipped) {
                            chd.QX = 0;
                            chd.QY = 0;
                            chd.QZ = 0;
                            chd.QW = 1;
                        } else {
                            chd.QX = 0;
                            chd.QY = 1;
                            chd.QZ = 0;
                            chd.QW = 0;
                        }
                        chd.AimingQX = 0;
                        chd.AimingQY = 0;
                        chd.AimingQZ = 0;
                        chd.AimingQW = 1;
                        chd.VelX = 0;
                        chd.VelY = 0;
                        chd.BulletTeamId = teamIdVal;
                        chd.Hp = chConfig.Hp;
                        chd.Mp = chConfig.Mp;
                        chd.SpeciesId = chConfig.SpeciesId;
                    }
                    Destroy(child.gameObject); // Delete the whole "ObjectLayer"
                    break;
                case "TrapStartingPos":
                    foreach (Transform trapChild in child) {
                        var tileObj = trapChild.GetComponent<SuperObject>();
                        var tileProps = trapChild.GetComponent<SuperCustomProperties>();
                        
                        CustomProperty id, tpt, initVelX, initVelY, initVelZ, initAngVelX, initAngVelY, initAngVelZ, prohibitsWallGrabbing, subscribesToTriggerId, cooldownRdfCount, sliderAxisX, sliderAxisY, sliderAxisZ, limit1, limit2, limit3, limit4;
                    
                        tileProps.TryGetCustomProperty("id", out id);
                        tileProps.TryGetCustomProperty("tpt", out tpt);
                        tileProps.TryGetCustomProperty("initVelX", out initVelX);
                        tileProps.TryGetCustomProperty("initVelY", out initVelY);
                        tileProps.TryGetCustomProperty("initVelZ", out initVelZ);

                        tileProps.TryGetCustomProperty("initAngVelX", out initAngVelX);
                        tileProps.TryGetCustomProperty("initAngVelY", out initAngVelY);
                        tileProps.TryGetCustomProperty("initAngVelZ", out initAngVelZ);

                        tileProps.TryGetCustomProperty("prohibitsWallGrabbing", out prohibitsWallGrabbing);
                        tileProps.TryGetCustomProperty("subscribesToTriggerId", out subscribesToTriggerId);
                        tileProps.TryGetCustomProperty("cooldownRdfCount", out cooldownRdfCount);
                        tileProps.TryGetCustomProperty("sliderAxisX", out sliderAxisX);
                        tileProps.TryGetCustomProperty("sliderAxisY", out sliderAxisY);
                        tileProps.TryGetCustomProperty("sliderAxisZ", out sliderAxisZ);
                        tileProps.TryGetCustomProperty("limit1", out limit1);
                        tileProps.TryGetCustomProperty("limit2", out limit2);
                        tileProps.TryGetCustomProperty("limit3", out limit3);
                        tileProps.TryGetCustomProperty("limit4", out limit4);
                        uint trapId = (uint)id.GetValueAsInt(); 
                        uint tptVal = (uint)tpt.GetValueAsInt(); // Not checking null or empty for this property because it shouldn't be, and in case it comes empty anyway, this automatically throws an error 
                        float initVelXVal = (null != initVelX && !initVelX.IsEmpty ? initVelX.GetValueAsFloat() : 0);
                        float initVelYVal = (null != initVelY && !initVelY.IsEmpty ? initVelY.GetValueAsFloat() : 0);
                        float initVelZVal = (null != initVelZ && !initVelY.IsEmpty ? initVelZ.GetValueAsFloat() : 0);

                        int cooldownRdfCountVal = (null != cooldownRdfCount && !cooldownRdfCount.IsEmpty ? cooldownRdfCount.GetValueAsInt() : 0);
                        
                        float initAngVelXVal = (null != initAngVelX && !initAngVelX.IsEmpty ? initAngVelX.GetValueAsFloat() : 0);
                        float initAngVelYVal = (null != initAngVelY && !initAngVelY.IsEmpty ? initAngVelY.GetValueAsFloat() : 0);
                        float initAngVelZVal = (null != initAngVelZ && !initAngVelZ.IsEmpty ? initAngVelZ.GetValueAsFloat() : 0);

                        float sliderAxisXVal = (null != sliderAxisX && !sliderAxisX.IsEmpty ? sliderAxisX.GetValueAsFloat() : 0);
                        float sliderAxisYVal = (null != sliderAxisY && !sliderAxisY.IsEmpty ? sliderAxisX.GetValueAsFloat() : 0);
                        float sliderAxisZVal = (null != sliderAxisZ && !sliderAxisZ.IsEmpty ? sliderAxisZ.GetValueAsFloat() : 0);
                        
                        float limit1Val = (null != limit1 && !limit1.IsEmpty ? limit1.GetValueAsFloat() : 0);
                        float limit2Val = (null != limit2 && !limit2.IsEmpty ? limit2.GetValueAsFloat() : 0);
                        float limit3Val = (null != limit3 && !limit3.IsEmpty ? limit3.GetValueAsFloat() : 0);
                        float limit4Val = (null != limit4 && !limit4.IsEmpty ? limit4.GetValueAsFloat() : 0);

                        bool prohibitsWallGrabbingVal = (null != prohibitsWallGrabbing && !prohibitsWallGrabbing.IsEmpty && 1 == prohibitsWallGrabbing.GetValueAsInt()) ? true : false;

                        bool xFlipped = isXFlipped(tileObj.m_TileId); 
                        uint subscribesToTriggerIdVal = (null == subscribesToTriggerId || subscribesToTriggerId.IsEmpty ? primitives.TerminatingTriggerId : (uint)subscribesToTriggerId.GetValueAsInt());

                        bool isBottomAnchor = (null != tileObj.m_SuperTile && (null != tileObj.m_SuperTile.m_Sprite || null != tileObj.m_SuperTile.m_AnimationSprites));
                        var (tiledRectCenterX, tiledRectCenterY) = isBottomAnchor ? (tileObj.m_X + tileObj.m_Width * 0.5f, tileObj.m_Y - tileObj.m_Height * 0.5f) : (tileObj.m_X + tileObj.m_Width * 0.5f, tileObj.m_Y + tileObj.m_Height * 0.5f);

                        TrapConfig tpConfig = PbTrapsOverride.Instance.getUnderlying()[tptVal];
                        int effCooldownRdfCount = (0 >= cooldownRdfCountVal ? tpConfig.DefaultCooldownRdfCount : cooldownRdfCountVal);

                        var (rectCx, rectCy) = TiledLayerPositionToCollisionSpacePosition(tiledRectCenterX, tiledRectCenterY, tilemapHalfHeight, collisionSpacePaddingLeft, collisionSpacePaddingBottom);
                        TrapConfigFromTiled trapConfigFromTiled = new TrapConfigFromTiled {
                            Id = trapId,
                            Tpt = tptVal,
                            InitX = rectCx,
                            InitY = rectCy,
                            InitZ = 0,
                            InitVelX = initVelXVal,
                            InitVelY = initVelYVal,
                            InitVelZ = initVelZVal,

                            InitAngVelX = initAngVelXVal,
                            InitAngVelY = initAngVelYVal,
                            InitAngVelZ = initAngVelZVal,

                            SliderAxisX = sliderAxisXVal,
                            SliderAxisY = sliderAxisYVal,
                            SliderAxisZ = sliderAxisZVal,
                            BoxHalfSizeX = .5f*tileObj.m_Width,
                            BoxHalfSizeY = .5f*tileObj.m_Height,

                            SubscribesToTriggerId = subscribesToTriggerIdVal,
                            CooldownRdfCount = effCooldownRdfCount,
                        };

                        if (PbPrimitivesOverride.Instance.getUnderlying().Tpts.SlidingPlatform == tptVal) {
                            if (null != limit1 && !limit1.IsEmpty && null != limit2 && !limit2.IsEmpty) {
                                Assert.IsTrue(limit1Val <= limit2Val);
                                trapConfigFromTiled.Limit1 = limit1Val;
                                trapConfigFromTiled.Limit2 = limit2Val;
                            }
                            if (null != limit3 && !limit3.IsEmpty) {
                                trapConfigFromTiled.Limit3 = limit3Val;
                            }
                            if (null != limit4 && !limit4.IsEmpty) {
                                trapConfigFromTiled.Limit4 = limit4Val;
                            }
                        } else if (PbPrimitivesOverride.Instance.getUnderlying().Tpts.RotatingPlatform == tptVal) {
                            Vector3 initAngVel = new Vector3(initAngVelXVal, initAngVelYVal, initAngVelZVal);
                            Vector3 rotationAxis = initAngVel.normalized;
                            trapConfigFromTiled.SliderAxisX = rotationAxis.x;
                            trapConfigFromTiled.SliderAxisY = rotationAxis.y;
                            trapConfigFromTiled.SliderAxisZ = rotationAxis.z;
                            if (null != limit1 && !limit1.IsEmpty && null != limit2 && !limit2.IsEmpty) {
                                Assert.IsTrue(limit1Val <= limit2Val);

                                float radianLimit1 = Mathf.Deg2Rad * (limit1Val);
                                float radianLimit2 = Mathf.Deg2Rad * (limit2Val);
                                
                                float initAngSpeed = initAngVel.magnitude;

                                float cooldownRadians = initAngSpeed * effCooldownRdfCount / PbPrimitivesOverride.BATTLE_DYNAMICS_FPS;
                                if (radianLimit1 + cooldownRadians > radianLimit2) {
                                    Assert.IsTrue(radianLimit1 + cooldownRadians <= radianLimit2);
                                }

                                trapConfigFromTiled.Limit1 = limit1Val;
                                trapConfigFromTiled.Limit2 = limit2Val;
                            }
                        }

                        Trap trap = new Trap {
                                Id = trapId,
                                Tpt = tptVal,
                                X = rectCx,
                                Y = rectCy,
                                Z = 0,
                            };

                        if (!xFlipped) {
                            trap.QX = 0;
                            trap.QY = 0;
                            trap.QZ = 0;
                            trap.QW = 1;
                            trapConfigFromTiled.InitQX = 0;
                            trapConfigFromTiled.InitQY = 0;
                            trapConfigFromTiled.InitQZ = 0;
                            trapConfigFromTiled.InitQW = 1;
                        } else {
                            trap.QX = 0;
                            trap.QY = 1;
                            trap.QZ = 0;
                            trap.QW = 0;
                            trapConfigFromTiled.InitQX = 0;
                            trapConfigFromTiled.InitQY = 1;
                            trapConfigFromTiled.InitQZ = 0;
                            trapConfigFromTiled.InitQW = 0;
                        }
                        startRdf.DynamicTraps.Add(trap);
                        result.TrapConfigFromTileList.Add(trapConfigFromTiled);
                        dynamicTrapCount++;
                        Destroy(trapChild.gameObject); // [WARNING] It'll be animated by "TrapPrefab" in "applyRoomDownsyncFrame" instead!
                    }
                    Destroy(child.gameObject); // Delete the whole "ObjectLayer"
                    break;
                case "TriggerPos":
                    foreach (Transform triggerChild in child) {
                        var tileObj = triggerChild.GetComponent<SuperObject>();
                        var tileProps = triggerChild.GetComponent<SuperCustomProperties>();
                        CustomProperty id, bulletTeamId, delayedFrames, quota, recoveryFrames, trt, subCycleTriggerFrames, subCycleQuota, newRevivalX, newRevivalY, storyPointId, bgmId, publishingToTriggerIdUponExhausted, isBossSavepoint, bossSpeciesIds, forceCtrlRdfCount, forceCtrlCmd;
                        CustomProperty characterSpawnerTimeSeq, pickableSpawnerTimeSeq;
                        tileProps.TryGetCustomProperty("id", out id);
                        tileProps.TryGetCustomProperty("trt", out trt);
                        tileProps.TryGetCustomProperty("bulletTeamId", out bulletTeamId);
                        tileProps.TryGetCustomProperty("delayedFrames", out delayedFrames);
                        tileProps.TryGetCustomProperty("quota", out quota);
                        tileProps.TryGetCustomProperty("recoveryFrames", out recoveryFrames);
                        tileProps.TryGetCustomProperty("subCycleTriggerFrames", out subCycleTriggerFrames);
                        tileProps.TryGetCustomProperty("subCycleQuota", out subCycleQuota);
                        tileProps.TryGetCustomProperty("characterSpawnerTimeSeq", out characterSpawnerTimeSeq);
                        tileProps.TryGetCustomProperty("pickableSpawnerTimeSeq", out pickableSpawnerTimeSeq);
                        tileProps.TryGetCustomProperty("newRevivalX", out newRevivalX);
                        tileProps.TryGetCustomProperty("newRevivalY", out newRevivalY);
                        tileProps.TryGetCustomProperty("storyPointId", out storyPointId);
                        tileProps.TryGetCustomProperty("bgmId", out bgmId);
                        tileProps.TryGetCustomProperty("publishingToTriggerIdUponExhausted", out publishingToTriggerIdUponExhausted);
                        tileProps.TryGetCustomProperty("isBossSavepoint", out isBossSavepoint);
                        tileProps.TryGetCustomProperty("bossSpeciesIds", out bossSpeciesIds);
                        tileProps.TryGetCustomProperty("forceCtrlRdfCount", out forceCtrlRdfCount);
                        tileProps.TryGetCustomProperty("forceCtrlCmd", out forceCtrlCmd);

                        if (null == id || id.IsEmpty) {
                            throw new ArgumentNullException("Property id MUST be set for child in TriggerPos");
                        }

                        if (null == trt || trt.IsEmpty) {
                            throw new ArgumentNullException("Property trt MUST be set for child in TriggerPos");
                        }

                        uint trtVal = (uint)trt.GetValueAsInt(); // must have  
                        uint triggerId = (uint)id.GetValueAsInt(); // must have 
                        
                        int bulletTeamIdVal = (null != bulletTeamId && !bulletTeamId.IsEmpty ? bulletTeamId.GetValueAsInt() : 0);
                        int delayedFramesVal = (null != delayedFrames && !delayedFrames.IsEmpty ? delayedFrames.GetValueAsInt() : 0);
                        int quotaVal = (null != quota && !quota.IsEmpty ? quota.GetValueAsInt() : 1);
                        int recoveryFramesVal = (null != recoveryFrames && !recoveryFrames.IsEmpty ? recoveryFrames.GetValueAsInt() : delayedFramesVal + 1); // By default we must have "recoveryFramesVal > delayedFramesVal"
                        int subCycleTriggerFramesVal = (null != subCycleTriggerFrames && !subCycleTriggerFrames.IsEmpty ? subCycleTriggerFrames.GetValueAsInt() : 0);
                        int subCycleQuotaVal = (null != subCycleQuota && !subCycleQuota.IsEmpty ? subCycleQuota.GetValueAsInt() : 0);
                        var characterSpawnerTimeSeqStr = (null != characterSpawnerTimeSeq && !characterSpawnerTimeSeq.IsEmpty ? characterSpawnerTimeSeq.GetValueAsString() : "");
                        var pickableSpawnerTimeSeqStr = (null != pickableSpawnerTimeSeq && !pickableSpawnerTimeSeq.IsEmpty ? pickableSpawnerTimeSeq.GetValueAsString() : "");
                        bool isBossSavepointVal = (null != isBossSavepoint && !isBossSavepoint.IsEmpty && 1 == isBossSavepoint.GetValueAsInt()) ? true : false;
                        var bossSpeciesIdsStr = (null != bossSpeciesIds && !bossSpeciesIds.IsEmpty ? bossSpeciesIds.GetValueAsString() : "");

                        float newRevivalCx = 0, newRevivalCy = 0;
                        int newRevivalXVal = 0, newRevivalYVal = 0;
                        if (null != newRevivalX && !newRevivalX.IsEmpty && null != newRevivalY && !newRevivalY.IsEmpty) {
                            float newRevivalXTiled = newRevivalX.GetValueAsFloat();
                            float newRevivalYTiled = newRevivalY.GetValueAsFloat();
                            (newRevivalCx, newRevivalCy) = TiledLayerPositionToCollisionSpacePosition(newRevivalXTiled, newRevivalYTiled, tilemapHalfHeight, collisionSpacePaddingLeft, collisionSpacePaddingBottom);
                        }

                        uint publishingToTriggerIdUponExhaustedVal = (null != publishingToTriggerIdUponExhausted && !publishingToTriggerIdUponExhausted.IsEmpty ? (uint)publishingToTriggerIdUponExhausted.GetValueAsInt() : PbPrimitivesOverride.Instance.getUnderlying().TerminatingTriggerId);

                        int storyPointIdVal = (null != storyPointId && !storyPointId.IsEmpty ? storyPointId.GetValueAsInt() : 0);
                        int bgmIdVal = (null != bgmId && !bgmId.IsEmpty ? bgmId.GetValueAsInt() : PbPrimitivesOverride.Instance.getUnderlying().BgmNoChange);

                        bool isBottomAnchor = (null != tileObj.m_SuperTile && (null != tileObj.m_SuperTile.m_Sprite || null != tileObj.m_SuperTile.m_AnimationSprites));
                        var (tiledRectCenterX, tiledRectCenterY) = isBottomAnchor ? (tileObj.m_X + tileObj.m_Width * 0.5f, tileObj.m_Y - tileObj.m_Height * 0.5f) : (tileObj.m_X + tileObj.m_Width * 0.5f, tileObj.m_Y + tileObj.m_Height * 0.5f);

                        var (rectCx, rectCy) = TiledLayerPositionToCollisionSpacePosition(tiledRectCenterX, tiledRectCenterY, tilemapHalfHeight, collisionSpacePaddingLeft, collisionSpacePaddingBottom);

                        var trigger = new Trigger {
                            Id = triggerId,
                            Trt = trtVal,
                            BulletTeamId = bulletTeamIdVal,
                            State = TriggerState.TrReady,
                            X = rectCx,
                            Y = rectCy,
                            Z = 0,
                        };

                        var forceCtrlRdfCountVal = (null != forceCtrlRdfCount && !forceCtrlRdfCount.IsEmpty ? forceCtrlRdfCount.GetValueAsInt() : 0);
                        var forceCtrlCmdVal = (null != forceCtrlCmd && !forceCtrlCmd.IsEmpty ? (ulong)forceCtrlCmd.GetValueAsInt() : 0u);
                        var configFromTiled = new TriggerConfigFromTiled {
                            Id = triggerId,
                            Trt = trtVal,
                            BulletTeamId = bulletTeamIdVal,
                            DelayedFrames = delayedFramesVal,
                            RecoveryFrames = recoveryFramesVal,
                            SubCycleTriggerFrames = subCycleTriggerFramesVal,
                            SubCycleQuota = subCycleQuotaVal,
                            Quota= quotaVal,
                            NewRevivalX = newRevivalXVal,
                            NewRevivalY = newRevivalYVal,
                            StoryPointId = storyPointIdVal,
                            PublishingToTriggerIdUponExhausted = publishingToTriggerIdUponExhaustedVal,
                            BgmId = bgmIdVal,
                            IsBossSavepoint = isBossSavepointVal,
                            ForceCtrlRdfCount = forceCtrlRdfCountVal,
                            ForceCtrlCmd = forceCtrlCmdVal,
                        };
                        bool xFlipped = isXFlipped(tileObj.m_TileId);

                        if (!xFlipped) {
                            configFromTiled.InitQX = 0;
                            configFromTiled.InitQY = 0;
                            configFromTiled.InitQZ = 0;
                            configFromTiled.InitQW = 1;
                        } else {
                            configFromTiled.InitQX = 0;
                            configFromTiled.InitQY = 1;
                            configFromTiled.InitQZ = 0;
                            configFromTiled.InitQW = 0;
                        }

                        if (PbPrimitivesOverride.Instance.getUnderlying().Trts.ByMovement == trtVal || PbPrimitivesOverride.Instance.getUnderlying().Trts.ByAttack == trtVal || PbPrimitivesOverride.Instance.getUnderlying().Trts.ByPatternF == trtVal) {
                            configFromTiled.BoxHalfSizeX = .5f * tileObj.m_Width;
                            configFromTiled.BoxHalfSizeY = .5f * tileObj.m_Height;
                        }

                        string[] characterSpawnerTimeSeqStrParts = characterSpawnerTimeSeqStr.Split(';', StringSplitOptions.RemoveEmptyEntries);
                        foreach (var part in characterSpawnerTimeSeqStrParts) {
                            if (String.IsNullOrEmpty(part)) continue;
                            string[] subParts = part.Split(':', StringSplitOptions.RemoveEmptyEntries);
                            if (2 != subParts.Length) continue;
                            if (String.IsNullOrEmpty(subParts[0])) continue;
                            if (String.IsNullOrEmpty(subParts[1])) continue;
                            int cutoffRdfFrameId = subParts[0].ToInt();
                            var chSpawnerConfig = new CharacterSpawnerConfig {
                                CutoffRdfId = cutoffRdfFrameId
                            };
                            string[] speciesIdAndOpParts = subParts[1].Split(',', StringSplitOptions.RemoveEmptyEntries);
                            foreach (var speciesIdAndOpPart in speciesIdAndOpParts) {
                                string[] speciesIdAndOpSplitted = speciesIdAndOpPart.Split('|', StringSplitOptions.RemoveEmptyEntries);
                                chSpawnerConfig.SpeciesIdList.Add((uint)speciesIdAndOpSplitted[0].ToInt());
                                if (2 <= speciesIdAndOpSplitted.Length) {
                                    chSpawnerConfig.InitOpList.Add(Convert.ToUInt64(speciesIdAndOpSplitted[1]));
                                } else {
                                    chSpawnerConfig.InitOpList.Add(0);
                                }
                            }
                            configFromTiled.CharacterSpawnerTimeSeq.Add(chSpawnerConfig);
                        }

                        string[] pickableSpawnerTimeSeqStrParts = pickableSpawnerTimeSeqStr.Split(';', StringSplitOptions.RemoveEmptyEntries);
                        foreach (var part in pickableSpawnerTimeSeqStrParts) {
                            if (String.IsNullOrEmpty(part)) continue;
                            string[] subParts = part.Split(':', StringSplitOptions.RemoveEmptyEntries);
                            if (2 != subParts.Length) continue;
                            if (String.IsNullOrEmpty(subParts[0])) continue;
                            if (String.IsNullOrEmpty(subParts[1])) continue;
                            int cutoffRdfFrameId = subParts[0].ToInt();
                            var pkSpawnerConfig = new PickableSpawnerConfig {
                                CutoffRdfId = cutoffRdfFrameId
                            };
                            string[] speciesIdAndOpParts = subParts[1].Split(',', StringSplitOptions.RemoveEmptyEntries);
                            foreach (var speciesIdAndOpPart in speciesIdAndOpParts) {
                                string[] pktAndOpSplitted = speciesIdAndOpPart.Split('|', StringSplitOptions.RemoveEmptyEntries);
                                pkSpawnerConfig.PickupTypeList.Add((uint)pktAndOpSplitted[0].ToInt());
                                if (2 <= pktAndOpSplitted.Length) {
                                    pkSpawnerConfig.InitOpList.Add(Convert.ToUInt64(pktAndOpSplitted[1]));
                                } else {
                                    pkSpawnerConfig.InitOpList.Add(0);
                                }
                            }
                            configFromTiled.PickableSpawnerTimeSeq.Add(pkSpawnerConfig);
                        }
                        triggerUdToConfigFromTiled[APP_CalcTriggerUserData(triggerId)] = configFromTiled;
                        trigger.Quota = quotaVal;
                        startRdf.Triggers.Add(trigger);
                        result.TriggerConfigFromTileList.Add(configFromTiled);
                        ++triggerCount;
                    }
                    Destroy(child.gameObject); // Delete the whole "ObjectLayer"
                    break;
                default:
                    break;
            }
        }

        startRdf.NpcIdCounter = npcIdCounter;
        startRdf.NpcCount = npcIdCounter-1;

        startRdf.PickableIdCounter = pickableIdCounter;
        startRdf.PickableCount = pickableIdCounter-1;

        startRdf.DynamicTrapCount = dynamicTrapCount;
        startRdf.TriggerCount = triggerCount;
        
        result.SelfParsedRdf = startRdf;
        
        return result;
    }

    protected Vector3 newPosHolder = new Vector3();
    protected Vector3 newTlPosHolder = new Vector3(), newTrPosHolder = new Vector3(), newBlPosHolder = new Vector3(), newBrPosHolder = new Vector3();
    protected Vector3 pointInCamViewPortHolder = new Vector3();
    protected Vector2 camDiffDstHolder = new Vector2();

    protected void clampToMapBoundary(ref Vector3 posHolder) {
        float newX = posHolder.x, newY = posHolder.y, newZ = posHolder.z;
        if (newX > cameraCapMaxX) newX = cameraCapMaxX;
        if (newX < cameraCapMinX) newX = cameraCapMinX;
        if (newY > cameraCapMaxY) newY = cameraCapMaxY;
        if (newY < cameraCapMinY) newY = cameraCapMinY;
        posHolder.Set(newX, newY, newZ);
    }

    
    protected virtual void cameraTrack(RenderFrame rdf, RenderFrame prevRdf, bool battleResultIsSet, bool forceTeleport = false) {
        uint targetJoinIndex = selfJoinIndex; // TODO
        ulong playerUd = APP_CalcPlayerUserData(targetJoinIndex);
        int joinIndexArrIdx = (int)targetJoinIndex - 1;
        var playerChd = rdf.Players[joinIndexArrIdx];
        var chd = playerChd.Chd;
        var chConfig = characters[chd.SpeciesId];

        var (chGameObj, oldUd) = playerAnimPool.GetOrCreateAnimNode(playerUd, chd.SpeciesId, chConfig, underlyingMap.transform, chMaterial);
        float camSpeed = chd.Speed;
        bool isChdMoving = (0 != chd.VelX || 0 != chd.VelY);
        newPosHolder.Set(chGameObj.transform.position.x, chGameObj.transform.position.y, chGameObj.transform.position.z);
        var (offCameraCenterRadioSquaredX, offCameraCenterRadioSquaredY) = calcOffCameraCenterRatioSquared(newPosHolder);
        bool justDead = ((0 >= chd.Hp) || (0 < chd.NewBirthRdfCountdown));
        if (justDead || battleResultIsSet) {
            camSpeed *= 500f;
        } else {
            if (isChdMoving || offCameraCenterRadioSquaredX > 0.0625 || offCameraCenterRadioSquaredY > 0.0625) {
                float offCamTrackingBoost = 5f * (offCameraCenterRadioSquaredX + offCameraCenterRadioSquaredY - 0.0625f); 
                float verticalTrackingBoost = 0.1f * (Math.Abs(chd.VelY)/chConfig.Speed);
                camSpeed *= (1f + offCamTrackingBoost + verticalTrackingBoost);
            } else if (!forceTeleport) {
                return; // Don't trace if the offset is too small
            }
        }

        var camOldPos = gameplayCamera.transform.position;
        var dst = chGameObj.transform.position;

        if (forceTeleport) {
            newPosHolder.Set(dst.x, dst.y, defaultGameplayCamZ);
        } else {
            camDiffDstHolder.Set(dst.x - camOldPos.x, dst.y - camOldPos.y);
            float camDiffMagnitude = camDiffDstHolder.magnitude;
            var stepLength = Time.deltaTime * camSpeed;
            if (stepLength > camDiffMagnitude) {
                // Immediately teleport
                newPosHolder.Set(dst.x, dst.y, defaultGameplayCamZ);
            } else {
                var newMapPosDiff2 = camDiffDstHolder.normalized * stepLength;
                newPosHolder.Set(camOldPos.x + newMapPosDiff2.x, camOldPos.y + newMapPosDiff2.y, defaultGameplayCamZ);
            }
            clampToMapBoundary(ref newPosHolder);
        }

        gameplayCamera.transform.position = newPosHolder;
    }

    protected void applyRdf(RenderFrame rdf, StepResult? stepResult, float dtSeconds) {
        int rdfId = rdf.Id;
        float selfPlayerWx = 0f, selfPlayerWy = 0f;

        for (int k = 0; k < roomCapacity; k++) {
            var player = rdf.Players[k];
            ulong playerUd = Bindings.APP_CalcPlayerUserData(player.JoinIndex);
            var currCharacterDownsync = player.Chd;
            var chConfig = characters[currCharacterDownsync.SpeciesId];
            
            var (wx, wy) = CollisionSpacePositionToWorldPosition(currCharacterDownsync.X, currCharacterDownsync.Y, tilemapHalfHeight, collisionSpacePaddingLeft, collisionSpacePaddingBottom);

            if (player.JoinIndex == selfJoinIndex) {
                selfPlayerWx = wx;
                selfPlayerWy = wy;
                selfBattleHeading.SetCharacter(player, currCharacterDownsync);

                if (null != chConfig.Atk1Magazine) {
                    var slotData = currCharacterDownsync.Atk1Magazine;
                    if (null != slotData && InventorySlotStockType.NoneIv != slotData.StockType && InventorySlotStockType.DummyIv != slotData.StockType) {
                        var ivSlotGui = iptmgr.btnB.GetComponent<InventorySlotBadge>();
                        ivSlotGui.updateData(slotData, chConfig.Atk1Magazine);
                        iptmgr.btnB.gameObject.SetActive(true);
                    } else {
                        iptmgr.btnB.gameObject.SetActive(false);
                    }
                }
                
                var battleSpecificConfig = cachedWsReqForStartRdf.BattleSpecificConfig;
                var characterOverrides = (null == battleSpecificConfig ? null : battleSpecificConfig.CharacterOverrides);
                var chOverride = ((null == characterOverrides || !characterOverrides.ContainsKey(playerUd)) ? null : characterOverrides[playerUd]);
                if (null != chOverride && null != chOverride.Atk1Magazine) {
                    var slotData = currCharacterDownsync.Atk1Magazine;
                    var ivSlotGui = iptmgr.btnB.GetComponent<InventorySlotBadge>();
                    ivSlotGui.updateData(slotData, chOverride.Atk1Magazine);
                    iptmgr.btnB.gameObject.SetActive(true);
                }
                
                int effSlotsCountLimit = (currCharacterDownsync.InventorySlots.Count < chConfig.InitInventorySlots.Count ? currCharacterDownsync.InventorySlots.Count : chConfig.InitInventorySlots.Count);
                int effInventoryCount = 0;

                for (int i = 0; i < effSlotsCountLimit; i++) {
                    var slotData = currCharacterDownsync.InventorySlots[i];
                    InventorySlotConfig slotConfig = chConfig.InitInventorySlots[i];
                    if (null != chOverride && null != chOverride.InitInventorySlots && i < chOverride.InitInventorySlots.Count) {
                        slotConfig = chOverride.InitInventorySlots[i];
                    }
                    if (InventorySlotStockType.NoneIv == slotData.StockType) break;
                    if (InventorySlotStockType.DummyIv == slotData.StockType) continue;
                    var targetBtn = (0 == i ? iptmgr.btnC : iptmgr.btnD);
                    targetBtn.gameObject.SetActive(true);
                    var ivSlotGui = targetBtn.GetComponent<InventorySlotBadge>();
                    ivSlotGui.updateData(slotData, slotConfig);
                }

                if (0 >= effInventoryCount) {
                    iptmgr.btnC.gameObject.SetActive(false);
                    iptmgr.btnD.gameObject.SetActive(false);
                }
            }

            var (chAnimCtrl, oldUd) = playerAnimPool.GetOrCreateAnimNode(playerUd, currCharacterDownsync.SpeciesId, chConfig, underlyingMap.transform, chMaterial);

            var origPlayerSpeciesId = cachedWsReqForStartRdf.SelfParsedRdf.Players[k].Chd.SpeciesId; // In case Character used "Transform".
            var material = chAnimCtrl.GetMaterial();
            if (characterUdToColorSwapRuleLock.ContainsKey(playerUd)) {
                var colorSwapRuleOrder = characterUdToColorSwapRuleLock[playerUd];
                if (TeamRibbon.COLOR_SWAP_RULE.ContainsKey(origPlayerSpeciesId) && TeamRibbon.COLOR_SWAP_RULE[origPlayerSpeciesId].ContainsKey(colorSwapRuleOrder)) {
                    var rule = TeamRibbon.COLOR_SWAP_RULE[origPlayerSpeciesId][colorSwapRuleOrder];
                    material.SetColor("_Palette1From", rule.P1From);
                    material.SetColor("_Palette1To", rule.P1To);
                    material.SetFloat("_Palette1FromRange", rule.P1FromRange);
                    material.SetFloat("_Palette1ToFuzziness", rule.P1ToFuzziness);
                }
            }
            
            bool shouldInterpolate = (playerUd == oldUd);

            if (shouldInterpolate) {
                float dWx = (wx - chAnimCtrl.gameObject.transform.position.x);
                float dWy = (wy - chAnimCtrl.gameObject.transform.position.y);
                float dis2 = dWx * dWx + dWy * dWy;
                setCharacterGameObjectPosByInterpolation(null, currCharacterDownsync, chConfig, chAnimCtrl.gameObject, wx, wy, dtSeconds, dWx, dWy, dis2);
            } else {
                float effZ = calcEffCharacterZ(currCharacterDownsync, chConfig);
                newPosHolder.Set(wx, wy, effZ);
            }

            chAnimCtrl.gameObject.transform.position = newPosHolder; // [WARNING] Even if not selfPlayer, we have to set position of the other players regardless of new positions being visible within camera or not, otherwise outdated other players' node might be rendered within camera! 

            chAnimCtrl.updateAnim(rdfId, playerUd, currCharacterDownsync, currCharacterDownsync.ChState, chConfig, currCharacterDownsync.FramesInChState);

            // Add character vfx
            float distanceAttenuationZ = Math.Abs(wx - selfPlayerWx) + Math.Abs(wy - selfPlayerWy);
            playCharacterDamagedVfx(rdf.Id, currCharacterDownsync, chConfig, chAnimCtrl.gameObject, wx, wy, chConfig.CapsuleHalfHeight, chAnimCtrl, playerUd, material, false);
        }

        for (int k = 0; k < rdf.NpcCount; k++) {
            var npc = rdf.Npcs[k];
            if (PbPrimitivesOverride.Instance.getUnderlying().TerminatingCharacterId == npc.Id) break;
            ulong npcUd = Bindings.APP_CalcNpcUserData(npc.Id);
            var currCharacterDownsync = npc.Chd;
            var chConfig = characters[currCharacterDownsync.SpeciesId];
            
            var (wx, wy) = CollisionSpacePositionToWorldPosition(currCharacterDownsync.X, currCharacterDownsync.Y, tilemapHalfHeight, collisionSpacePaddingLeft, collisionSpacePaddingBottom);

            var (chAnimCtrl, oldUd) = npcAnimPool.GetOrCreateAnimNode(npcUd, currCharacterDownsync.SpeciesId, chConfig, underlyingMap.transform, chMaterial);
            var material = chAnimCtrl.GetMaterial();
            bool shouldInterpolate = (npcUd == oldUd);

            if (shouldInterpolate) {
                float dWx = (wx - chAnimCtrl.gameObject.transform.position.x);
                float dWy = (wy - chAnimCtrl.gameObject.transform.position.y);
                float dis2 = dWx * dWx + dWy * dWy;
                shouldInterpolate = (dis2 <= npcInterpolationDis2Threshold); // [WARNING] In case NPC id assignment order was changed across rollback-chasing, we don't want to interpolate from too far away.
                if (shouldInterpolate) {
                    setCharacterGameObjectPosByInterpolation(null, currCharacterDownsync, chConfig, chAnimCtrl.gameObject, wx, wy, dtSeconds, dWx, dWy, dis2);
                } else {
                    float effZ = calcEffCharacterZ(currCharacterDownsync, chConfig);
                    newPosHolder.Set(wx, wy, effZ);
                }
            } else {
                float effZ = calcEffCharacterZ(currCharacterDownsync, chConfig);
                newPosHolder.Set(wx, wy, effZ);
            }

            chAnimCtrl.gameObject.transform.position = newPosHolder;

            chAnimCtrl.updateAnim(rdfId, npcUd, currCharacterDownsync, currCharacterDownsync.ChState, chConfig, currCharacterDownsync.FramesInChState);

            // Add character vfx
            float distanceAttenuationZ = Math.Abs(wx - selfPlayerWx) + Math.Abs(wy - selfPlayerWy);
            playCharacterDamagedVfx(rdf.Id, currCharacterDownsync, chConfig, chAnimCtrl.gameObject, wx, wy, chConfig.CapsuleHalfHeight, chAnimCtrl, npcUd, material, false);
        }

        for (int k = 0; k < rdf.BulletCount; k++) {
            var bullet = rdf.Bullets[k];
            if (primitives.TerminatingBulletId == bullet.Id) break;
            var (skillConfig, bulletConfig) = FindBulletConfig(bullet.SkillId, bullet.ActiveSkillHit, PbPrimitivesOverride.Instance.getUnderlying(), PbSkillsOverride.Instance);
            if (null == skillConfig || null == bulletConfig) continue;
            
            (newTlPosHolder.x, newTlPosHolder.y) = CollisionSpacePositionToWorldPosition(bullet.X - bulletConfig.HitboxHalfSizeX, bullet.Y + bulletConfig.HitboxHalfSizeY, tilemapHalfHeight, collisionSpacePaddingLeft, collisionSpacePaddingBottom); // Top-left

            (newTrPosHolder.x, newTrPosHolder.y) = CollisionSpacePositionToWorldPosition(bullet.X + bulletConfig.HitboxHalfSizeX, bullet.Y + bulletConfig.HitboxHalfSizeY, tilemapHalfHeight, collisionSpacePaddingLeft, collisionSpacePaddingBottom); // Top-right

            (newBlPosHolder.x, newBlPosHolder.y) = CollisionSpacePositionToWorldPosition(bullet.X - bulletConfig.HitboxHalfSizeX, bullet.Y - bulletConfig.HitboxHalfSizeY, tilemapHalfHeight, collisionSpacePaddingLeft, collisionSpacePaddingBottom); // bottom-left

            (newBrPosHolder.x, newBrPosHolder.y) = CollisionSpacePositionToWorldPosition(bullet.X + bulletConfig.HitboxHalfSizeX, bullet.Y - bulletConfig.HitboxHalfSizeY, tilemapHalfHeight, collisionSpacePaddingLeft, collisionSpacePaddingBottom); // bottom-right

            if (!bulletConfig.BeamCollision && !bulletConfig.BeamRendering) {
                if (!isGameObjPositionWithinCamera(newTlPosHolder) && !isGameObjPositionWithinCamera(newTrPosHolder) && !isGameObjPositionWithinCamera(newBlPosHolder) && !isGameObjPositionWithinCamera(newBrPosHolder)) continue;
            }

            var bulletUd = Bindings.APP_CalcBulletUserData(bullet.Id);
            if (!String.IsNullOrEmpty(bulletConfig.AnimName)) {
                var (bulletAnimHolder, oldUd) = bulletAnimPool.GetOrCreateAnimNode(bulletUd, bulletConfig.AnimName, bulletConfig, underlyingMap.transform, blMaterial);
                bulletAnimHolder.damageDealedIndicatorPrefab = damageIndicatorPrefab;
                bulletAnimHolder.updateAnim(rdfId, bulletUd, bullet, bullet.BlState, bulletConfig, bullet.FramesInBlState);

                var (wx, wy) = CollisionSpacePositionToWorldPosition(bullet.X, bullet.Y, tilemapHalfHeight, collisionSpacePaddingLeft, collisionSpacePaddingBottom);

                newPosHolder.Set(wx, wy, bulletAnimHolder.gameObject.transform.position.z);
                bulletAnimHolder.gameObject.transform.position = newPosHolder;
                /*
                float distanceAttenuationZ = Math.Abs(wx - selfPlayerWx) + Math.Abs(wy - selfPlayerWy);
                playBulletSfx(bullet, bulletConfig, isExploding, wx, wy, rdf.Id, distanceAttenuationZ);
                playBulletVfx(bullet, bulletConfig, isStartup, isExploding, wx, wy, rdf);
                */
            }
        }

        for (int k = 0; k < rdf.DynamicTrapCount; k++) {
            var tp = rdf.DynamicTraps[k];
            if (PbPrimitivesOverride.Instance.getUnderlying().TerminatingTrapId == tp.Id) break;
            ulong trapUd = Bindings.APP_CalcTrapUserData(tp.Id);
            var tpConfig = PbTrapsOverride.Instance.getUnderlying()[tp.Tpt];

            var (tpAnimCtrl, oldUd) = trapAnimPool.GetOrCreateAnimNode(trapUd, tp.Tpt, tpConfig, underlyingMap.transform);

            var (wx, wy) = CollisionSpacePositionToWorldPosition(tp.X, tp.Y, tilemapHalfHeight, collisionSpacePaddingLeft, collisionSpacePaddingBottom);

            newPosHolder.Set(wx, wy, trapZ);

            tpAnimCtrl.gameObject.transform.position = newPosHolder;

            tpAnimCtrl.updateAnim(rdfId, trapUd, tp, tp.TrapState, tpConfig, tp.FramesInTrapState);
        }

        for (int k = 0; k < rdf.TriggerCount; k++) {
            var trigger = rdf.Triggers[k];
            if (PbPrimitivesOverride.Instance.getUnderlying().TerminatingTriggerId == trigger.Id) break;
            ulong triggerUd = Bindings.APP_CalcTriggerUserData(trigger.Id);
            if (PbPrimitivesOverride.Instance.getUnderlying().Trts.IndiWaveNpcSpawner != trigger.Trt && PbPrimitivesOverride.Instance.getUnderlying().Trts.IndiWavePickableSpawner != trigger.Trt) {
                continue;
            }
            var triggerConfigFromTiled = triggerUdToConfigFromTiled[triggerUd];

            var (trAnimCtrl, oldUd) = triggerAnimPool.GetOrCreateAnimNode(triggerUd, trigger.Trt, triggerConfigFromTiled, underlyingMap.transform);

            var (wx, wy) = CollisionSpacePositionToWorldPosition(trigger.X, trigger.Y, tilemapHalfHeight, collisionSpacePaddingLeft, collisionSpacePaddingBottom);

            newPosHolder.Set(wx, wy, triggerZ);

            trAnimCtrl.gameObject.transform.position = newPosHolder;

            trAnimCtrl.updateAnim(rdfId, triggerUd, trigger, trigger.State, triggerConfigFromTiled, trigger.FramesInState);
        }

        for (int k = 0; k < rdf.PickableCount; k++) {
            var pickable = rdf.Pickables[k];
            if (PbPrimitivesOverride.Instance.getUnderlying().TerminatingPickableId == pickable.Id) break;
            ulong ud = Bindings.APP_CalcPickableUserData(pickable.Id);
            
            var pickableConfig = PbPickablesOverride.Instance.getUnderlying()[pickable.PickupType];

            var (animCtrl, oldUd) = pickableAnimPool.GetOrCreateAnimNode(ud, pickable.PickupType, pickableConfig, underlyingMap.transform);

            var (wx, wy) = CollisionSpacePositionToWorldPosition(pickable.X, pickable.Y, tilemapHalfHeight, collisionSpacePaddingLeft, collisionSpacePaddingBottom);

            newPosHolder.Set(wx, wy, trapZ);

            animCtrl.gameObject.transform.position = newPosHolder;

            animCtrl.updateAnim(rdfId, ud, pickable, pickable.PkState, pickableConfig, pickable.FramesInPkState);
        }

        if (null != stepResultHolder) {
            /*
            if (0 < stepResultHolder.AimingRayCount) {
                Debug.Log($"There's {stepResultHolder.AimingRayCount} aiming rays at csharpTimerRdfId={csharpTimerRdfId}#2");
            }
            */
            for (int k = 0; k < stepResultHolder.AimingRayCount; k++) {
                var aimingRay = stepResultHolder.AimingRays[k];

                var (animCtrl, oldUd) = aimingRayAnimPool.GetOrCreateAnimNode(aimingRay.OffenderUd, 0, this, underlyingMap.transform);
                animCtrl.updateAnim(rdfId, aimingRay.OffenderUd, aimingRay, CharacterState.Idle1, this, 0);
            }

            foreach (var preparedTriggerUd in stepResultHolder.PreparedTriggerUds) {
                // [TODO]
            }
        }

        playerAnimPool.HideInactiveAnimNodes(rdfId);
        npcAnimPool.HideInactiveAnimNodes(rdfId);
        bulletAnimPool.HideInactiveAnimNodes(rdfId);
        trapAnimPool.HideInactiveAnimNodes(rdfId);
        triggerAnimPool.HideInactiveAnimNodes(rdfId);
        inplaceHpBarAnimPool.HideInactiveAnimNodes(rdfId);
        aimingRayAnimPool.HideInactiveAnimNodes(rdfId);
        pickableAnimPool.HideInactiveAnimNodes(rdfId);

        cameraTrack(rdf, null, false);
    }
#nullable enable

    protected bool skipInterpolation = false;
    private float calcEffCharacterZ(CharacterDownsync currCd, CharacterConfig chConfig) {
        bool isNonAttacking = false; // TODO
        if (!isNonAttacking) {
            return flyingCharacterZ - 1;
        } else {
            return (!currCd.OmitGravity && !chConfig.OmitGravity) ? characterZ : flyingCharacterZ;
        }
    }

    protected void setCharacterGameObjectPosByInterpolation(CharacterDownsync? prevCharacterDownsync, CharacterDownsync currCharacterDownsync, CharacterConfig chConfig, GameObject chGameObj, in float newWx, in float newWy, in float dtSeconds, in float dWx, in float dWy, in float dis2) {
        float effZ = calcEffCharacterZ(currCharacterDownsync, chConfig);
        if (skipInterpolation) {
            newPosHolder.Set(newWx, newWy, effZ);
            return;
        }
        bool justDead = (null != prevCharacterDownsync && (CharacterState.Dying == prevCharacterDownsync.ChState || 0 >= prevCharacterDownsync.Hp));
        justDead |= (0 < currCharacterDownsync.NewBirthRdfCountdown);
        if (justDead) {
            // Revived from Dying state.
            newPosHolder.Set(newWx, newWy, effZ);
            return;
        }
        
        float dtSecondsSquared = (dtSeconds*dtSeconds);
        var speedReachable2 = (currCharacterDownsync.VelX * currCharacterDownsync.VelX + currCharacterDownsync.VelY * currCharacterDownsync.VelY) * dtSecondsSquared;
        float defaultSpeedReachable2 = (chConfig.Speed * chConfig.Speed) * dtSecondsSquared;
        float maxReachable2 = Math.Max(speedReachable2, defaultSpeedReachable2);
        //float tolerance2 = 0.01f * maxReachable2;
        //float dis2Bias = Math.Abs(dis2-maxReachable2);
        if (dis2 <= maxReachable2) {
            newPosHolder.Set(newWx, newWy, effZ);
        } else {
            float invMag = InvSqrt32(dis2);
            float ratio = 0;
            if (0 < speedReachable2 && speedReachable2 > defaultSpeedReachable2) {
                float speedReachable = speedReachable2 * InvSqrt32(speedReachable2);
                ratio = speedReachable * invMag;
            } else {
                ratio = chConfig.Speed * dtSeconds * invMag;
            }
            ratio *= 1.5f; // [WARNING] Empirically, either "speedReachable" or "chConfigSpeedReachable" could be too small when being dragged by a Rider (i.e. with hardPushback)
            if (ratio > 1.0f) ratio = 1.0f;
            float interpolatedWx = chGameObj.transform.position.x + ratio * dWx;
            float interpolatedWy = chGameObj.transform.position.y + ratio * dWy;
            newPosHolder.Set(interpolatedWx, interpolatedWy, effZ);
        }
    }
#nullable disable

    protected void preallocateSfxNodes() {
        // TODO: Shall I use the same preallocation strategy for VFX? Would run for a while and see the difference...
        Debug.Log("preallocateSfxNodes begins");
        if (null != cachedSfxNodes) {
            while (0 < cachedSfxNodes.Cnt()) {
                var g = cachedSfxNodes.Pop();
                if (null != g) {
                    Destroy(g.gameObject);
                }
            }
        }
        int sfxNodeCacheCapacity = 24;
        cachedSfxNodes = new KvPriorityQueue<int, SFXSource>(sfxNodeCacheCapacity, sfxNodeScore);
        string[] allSfxClipsNames = new string[] {
            "Explosion1",
            "Explosion2",
            "Explosion3",
            "Explosion4",
            "Explosion8",
            "Piercing",
            "Jump1",
            "Landing1",
            "Melee_Explosion1",
            "Melee_Explosion2",
            "Melee_Explosion3",
            "Melee_Explosion4",
            "PistolEmit",
            "FlameBurning1",
            "FlameEmit1",
            "SlashEmitSpd1",
            "SlashEmitSpd2",
            "SlashEmitSpd3",
            "FootStep1",
            "DoorOpen",
            "DoorClose",
            "WaterSplashSpd1",
            "Pickup1"
        };
        var audioClipDict = new Dictionary<string, AudioClip>();
        foreach (string name in allSfxClipsNames) {
            string prefabPathUnderResources = "SFX/" + name;
            var theClip = Resources.Load(prefabPathUnderResources) as AudioClip;
            audioClipDict[name] = theClip;
        }

        for (int i = 0; i < sfxNodeCacheCapacity; i++) {
            GameObject newSfxNode = Instantiate(sfxSourcePrefab, new Vector3(effInftyFar, effInftyFar, bulletZ), Quaternion.identity, underlyingMap.transform);
            SFXSource newSfxSource = newSfxNode.GetComponent<SFXSource>();
            newSfxSource.score = -1;
            newSfxSource.maxDistanceInWorld = effInftyFar * 0.25f;
            newSfxSource.audioClipDict = audioClipDict;
            int initLookupKey = i;
            cachedSfxNodes.Put(initLookupKey, newSfxSource);
        }

        Debug.Log("preallocateSfxNodes ends");
    }

    protected void resetLevelIdAndBgm() {
        var mapProps = underlyingMap.GetComponent<SuperCustomProperties>();
        CustomProperty levelIdProp, defaultBgmIdProp;
        mapProps.TryGetCustomProperty("levelId", out levelIdProp);
        mapProps.TryGetCustomProperty("bgmId", out defaultBgmIdProp);
       
        int defaultBgmId = (null == defaultBgmIdProp || defaultBgmIdProp.IsEmpty ? PbPrimitivesOverride.Instance.getUnderlying().BgmNoChange : defaultBgmIdProp.GetValueAsInt());
        if (PbPrimitivesOverride.Instance.getUnderlying().BgmNoChange != defaultBgmId) {
            if (null != bgmSource) {
                bgmSource.PlaySpecifiedBgm(defaultBgmId);
            }
        } else {
            if (null != bgmSource) {
                bgmSource.PlaySpecifiedBgm(1);
            }
        }
    }

    protected bool isBattleResultSet(StepResult stepResult) {
        if (null == stepResult) return false;
        if (null == stepResult.FulfilledTriggers) return false;
        foreach (var fulfilledTrigger in stepResult.FulfilledTriggers) {
            if (PbPrimitivesOverride.Instance.getUnderlying().Trts.Victory == fulfilledTrigger.Trt) {
                return true;
            }
        }
        return false;
    }

    protected Vector2 inplaceHpBarOffset = new Vector2(-8f, +16f);
    public void showInplaceHpBar(int rdfId, CharacterDownsync currCharacterDownsync, float wx, float wy, float halfBoxCw, float halfBoxCh, ulong lookupKey, CharacterConfig chConfig) {
        if (hideInplaceHpBars) return;

        var (inplaceHpBarAnimCtrl, oldUd) = inplaceHpBarAnimPool.GetOrCreateAnimNode(lookupKey, chConfig.SpeciesId, chConfig, underlyingMap.transform);

        newPosHolder.Set(wx + inplaceHpBarOffset.x, wy + halfBoxCh + inplaceHpBarOffset.y, inplaceHpBarZ);

        inplaceHpBarAnimCtrl.gameObject.transform.position = newPosHolder;

        inplaceHpBarAnimCtrl.updateAnim(rdfId, lookupKey, currCharacterDownsync, currCharacterDownsync.ChState, chConfig, currCharacterDownsync.FramesInChState);
    }

    public bool playCharacterDamagedVfx(int rdfId, CharacterDownsync currCharacterDownsync, CharacterConfig chConfig, GameObject theGameObj, float wx, float wy, float halfBoxCh, JoltCharacterAnimController chAnimCtrl, ulong lookupKey, Material material, bool isActiveBoss) {
        var spr = theGameObj.GetComponent<SpriteRenderer>();
        material.SetFloat("_CrackOpacity", 0f);

        if (!hideInplaceHpBars) {
            if (!isActiveBoss && lookupKey != Bindings.APP_CalcPlayerUserData(selfPlayerInfo.JoinIndex) && CharacterState.Dying != currCharacterDownsync.ChState && 0 < currCharacterDownsync.DamagedHintRdfCountdown) {
                showInplaceHpBar(rdfId, currCharacterDownsync, wx, wy, halfBoxCh, inplaceHpBarZ, lookupKey, chConfig);
            }
        }

        if (null != currCharacterDownsync.DebuffList) {
            for (int i = 0; i < currCharacterDownsync.DebuffList.Count; i++) {
                Debuff debuff = currCharacterDownsync.DebuffList[i];
                if (primitives.TerminatingDebuffSpeciesId == debuff.SpeciesId) break;
                /*
                var debuffConfig = debuffConfigs[debuff.SpeciesId];
                switch (debuffConfig.Type) {
                    case DebuffType.FrozenPositionLocked:
                        if (0 < debuff.Stock) {
                            material.SetFloat("_CrackOpacity", 0.90f);
                            CharacterState overwriteChState = currCharacterDownsync.CharacterState;
                            if (!noOpSet.Contains(overwriteChState)) {
                                overwriteChState = CharacterState.Atked1;
                            }
                            chAnimCtrl.updateCharacterAnim(currCharacterDownsync, overwriteChState, prevCharacterDownsync, false, chConfig);
                        }
                        break;
                    case DebuffType.PositionLockedOnly:
                        if (0 < debuff.Stock) {
                            int vfxSpeciesId = VfxThunderCharged.SpeciesId;
                            if (!pixelatedVfxDict.ContainsKey(vfxSpeciesId)) return false;

                            var vfxConfig = pixelatedVfxDict[vfxSpeciesId];
                            var vfxAnimName = vfxConfig.Name;
                            int vfxLookupKey = KV_PREFIX_VFX_CH_ELE_DEBUFF + currCharacterDownsync.JoinIndex;
                            newPosHolder.Set(wx, wy, bulletZ);

                            if (0 < vfxLookupKey) {
                                var pixelVfxHolder = cachedPixelVfxNodes.PopAny(vfxLookupKey);
                                if (null == pixelVfxHolder) {
                                    pixelVfxHolder = cachedPixelVfxNodes.Pop();
                                }
                                if (null != pixelVfxHolder && null != pixelVfxHolder.lookUpTable) {
                                    if (pixelVfxHolder.lookUpTable.ContainsKey(vfxAnimName)) {
                                        pixelVfxHolder.updateAnim(vfxAnimName, debuff.Stock, currCharacterDownsync.DirX, false, rdfId);
                                        pixelVfxHolder.gameObject.transform.position = newPosHolder;
                                    }
                                    pixelVfxHolder.score = rdfId;
                                    cachedPixelVfxNodes.Put(vfxLookupKey, pixelVfxHolder);
                                }
                            }
                        }
                        break;
                }
                */
            }
        }

        return true;
    }

    public bool isGameObjPositionWithinCamera(in Vector3 positionHolder) {
        var posInMainCamViewport = gameplayCamera.WorldToViewportPoint(positionHolder);
        return (0f <= posInMainCamViewport.x && posInMainCamViewport.x <= 1f && 0f <= posInMainCamViewport.y && posInMainCamViewport.y <= 1f && 0f < posInMainCamViewport.z);
    }

    public (float, float) calcOffCameraCenterRatioSquared(in Vector3 positionHolder) {
        var posInMainCamViewport = gameplayCamera.WorldToViewportPoint(positionHolder);
        float dx = posInMainCamViewport.x - 0.5f;
        float dy = posInMainCamViewport.y - 0.5f;
        return (dx*dx, dy*dy);
    }

    public void pauseAllAnimatingCharacters(bool toPause) {
        iptmgr.gameObject.SetActive(!toPause);
        for (int k = 0; k < roomCapacity; k++) {
            var player = rdfHolder.Players[k];
            var playerUd = APP_CalcPlayerUserData(player.JoinIndex);
            var chConfig = PbCharactersOverride.Instance.getUnderlying()[player.Chd.SpeciesId];
            var (chAnimCtrl, oldUd) = playerAnimPool.GetOrCreateAnimNode(playerUd, player.Chd.SpeciesId, chConfig, underlyingMap.transform, chMaterial);
            chAnimCtrl.pause(toPause);
        }
        
        for (int k = 0; k < rdfHolder.NpcCount; k++) {
            var npc = rdfHolder.Npcs[k];
            if (PbPrimitivesOverride.Instance.getUnderlying().TerminatingCharacterId == npc.Id) break;
            var npcUd = APP_CalcNpcUserData(npc.Id);
            var chConfig = PbCharactersOverride.Instance.getUnderlying()[npc.Chd.SpeciesId];
            var (chAnimCtrl, oldUd) = playerAnimPool.GetOrCreateAnimNode(npcUd, npc.Chd.SpeciesId, chConfig, underlyingMap.transform, chMaterial);
            if (null == chAnimCtrl) continue;
            chAnimCtrl.pause(toPause);
        }
    }

    public virtual void OnSettingsClicked() { }

    protected void popupErrStackPanel(string msg) {
        if (null == errStackLogPanelObj) {
            errStackLogPanelObj = Instantiate(errStackLogPanelPrefab, new Vector3(canvas.transform.position.x, canvas.transform.position.y, +5), Quaternion.identity, canvas.transform);
        }
        var errStackLogPanel = errStackLogPanelObj.GetComponent<ErrStackLogPanel>();
        errStackLogPanel.content.text = msg;
    }

    protected void attachParallaxEffect() {
        var grid = underlyingMap.GetComponentInChildren<Grid>();
        foreach (SuperTileLayer layer in grid.GetComponentsInChildren<SuperTileLayer>()) {
            if (1.0f == layer.m_ParallaxX) continue;
            var parallaxEffect = layer.gameObject.AddComponent<ParallaxEffect>();
            parallaxEffect.SetParallaxAmount(layer.m_ParallaxX, gameplayCamera);
        }
    }
}
