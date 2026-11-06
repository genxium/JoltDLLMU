using AOT;
using Assets.Scripts.MapParser;
using Google.Protobuf;
using Google.Protobuf.Collections;
using JoltCSharp;
using jtshared;
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using UnityEngine;
using UnityEngine.AddressableAssets;
using static JoltCSharp.PbPrimitivesOverride;
using static JoltCSharp.Bindings;

public abstract class AbstractJoltMapController : MonoBehaviour {
    public static Quaternion cTurnbackAroundYAxis = new Quaternion(0, 1, 0, 0);

    public Material sprDefaultMaterial;

    public Material chMaterial;
    public Material blMaterial;

    public GameObject damageIndicatorPrefab;
    public GameObject inplaceHpBarPrefab;
    public GameObject aimingRayPrefab;
    public GameObject gameplayBtnsHintPrefab;
    public GameObject errStackLogPanelPrefab;
    public GameObject teamRibbonPrefab;
    
    public GameObject sfxSourcePrefab;
    public GameObject loadSfxSourcePrefab() {
        return sfxSourcePrefab;
    }

    public GameObject keyChLightSource2DPrefab;
    public GameObject loadKeyChLightSource2DPrefab() {
        return keyChLightSource2DPrefab;
    }

    public GameObject flameTorchLightSource2DPrefab;

    protected GameObject errStackLogPanelObj;
    protected GameObject underlyingMap;
    public Canvas canvas;
    public Toast toast;

    protected uint selfJoinIndex = 0;
    public uint GetSelfJoinIndex() {
        return selfJoinIndex;
    }

    protected ulong selfUd = 0;
    public ulong GetSelfUd() {
        if (0 != selfUd) {
            return selfUd;
        }
        selfUd = APP_CalcPlayerUserData(selfJoinIndex);
        return selfUd;
    }

    protected int selfJoinIndexInt = 0;
    protected int selfJoinIndexArrIdx = - 1;
    protected ulong selfJoinIndexMask = 0;
    protected string selfPlayerId = null;
    protected int selfCmdAuthKey = 0;
    protected int newChaserRdfId = 0;

    protected long battleState;
    protected bool battleResultIsSet;

    protected const float SFX_ATTENUATION_REFERENCE_DISTANCE = 16.0f; // Same magnitude as a typical character capsule radius
    protected const float SFX_ATTENUATION_REFERENCE_DISTANCE2 = SFX_ATTENUATION_REFERENCE_DISTANCE * SFX_ATTENUATION_REFERENCE_DISTANCE;

    protected float cameraCapMinX, cameraCapMaxX, cameraCapMinY, cameraCapMaxY; // The active bounds
    protected float grandCameraCapMinX, grandCameraCapMaxX, grandCameraCapMinY, grandCameraCapMaxY;

    protected float effInftyFar, npcInterpolationDis2Threshold;

    protected Dictionary<int, BattleResult> unconfirmedBattleResult;
    protected Dictionary<ulong, TriggerConfigFromTiled> triggerUdToConfigFromTiled;
    protected Dictionary<ulong, TrapConfigFromTiled> trapUdToConfigFromTiled;
    protected Dictionary<ulong, int> characterUdToColorSwapRuleLock;
    protected Dictionary<uint, int> playerSpeciesIdOccurrenceCnt;

    protected JoltCharacterAnimPool playerAnimPool = null;
    protected JoltCharacterAnimPool npcAnimPool = null;
    protected Dictionary<ulong, CharacterDownsync> activeChdDictionary = new Dictionary<ulong, CharacterDownsync>();
    protected JoltBulletAnimPool bulletAnimPool = null;
    protected JoltTrapAnimPool trapAnimPool = null;
    protected JoltTriggerAnimPool triggerAnimPool = null;
    protected Dictionary<ulong, Trigger> activeTriggerDictionary = new Dictionary<ulong, Trigger>();
    protected JoltInplaceHpBarAnimPool inplaceHpBarAnimPool = null;
    protected JoltAimingRayAnimPool aimingRayAnimPool = null;
    protected JoltPickableAnimPool pickableAnimPool = null;
    protected GameplayBtnsHintAnimPool gameplayBtnsHintAnimPool = null;
    protected KeyChLightSourceAnimPool keyChLightSourceAnimPool = null;
    protected SFXSourceAnimPool sfxSourceAnimPool = null;

    protected bool allowSelfEmittingLight = true;
    protected bool shouldDetectRealtimeRenderHistoryCorrection = false; // Not recommended to enable in production, it might have some memory performance impact.
    protected bool frameLogEnabled = false;

    public BGMSource bgmSource;
    public Camera gameplayCamera;

    [Header("Hit stun camera shake")]
    [SerializeField] protected float hitSelfStunShakeAmplitude = 0.0325f;
    [SerializeField] protected float hitSelfStunShakeFrequency = 13.0f;
    [SerializeField] protected int hitSelfStunShakeDurationFrames = PbPrimitivesOverride.BATTLE_DYNAMICS_FPS/2;

    protected int hitSelfStunShakeFramesRemaining = 0;
    protected float hitSelfStunShakeSeed;

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

    protected bool cameraRoundToPixel = true;

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

    protected bool shouldInitJoltInfra = true;
    protected unsafe virtual void Start() {
        if (shouldInitJoltInfra) {
            JPH_Init(10 * 1024 * 1024);
            RegisterDebugCallback(OnDebugCallback);
            Debug.Log($"{this.GetType()}: Initialized Jolt resource allocators");
        }

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
        QualitySettings.vSyncCount = 1;
        Application.targetFrameRate = -1; // Ignored by "vSyncCount = 1"
        Time.fixedDeltaTime = (1.0f / PbPrimitivesOverride.BATTLE_DYNAMICS_FPS); // Determines the frequency of calling "FixedUpdate"
        Application.SetStackTraceLogType(LogType.Log, StackTraceLogType.None);
        Application.SetStackTraceLogType(LogType.Warning, StackTraceLogType.None);
        Application.SetStackTraceLogType(LogType.Error, StackTraceLogType.None);
        Application.SetStackTraceLogType(LogType.Exception, StackTraceLogType.None);

        iptmgr.SetMap(this);

        if (null == sprDefaultMaterial) {
            sprDefaultMaterial = Resources.Load("DefaultSpriteLitMaterial") as Material;
        }
    }

    protected bool shouldShutdownJoltInfra = true;
    protected virtual void OnDestroy() {
        // clean up
        onBattleStopped();
        if (UIntPtr.Zero != battle) {
            APP_DestroyBattle(battle);
            battle = UIntPtr.Zero;
        }

        // [WARNING] DON'T call "JPH_Shutdown()" if you're transitioning from one "AbstractJoltMapController" instance to another using "Addressables.LoadSceneAsync(...)".
        if (shouldShutdownJoltInfra) {
            bool shutdownRes = JPH_Shutdown();
            Debug.Log($"{this.GetType()}: Jolt infra shutdown result = {shutdownRes}");
        }
    }

    protected bool lazyInitDebugColliderAnimPool(int rdfId) {
        if (null != debugColliderAnimPool) return true;
        debugColliderAnimPool = new JoltDebugColliderAnimPool(this);
        debugColliderAnimPool.ResetUponBattlePreparation(rdfId);
        debugColliderAnimPool.SetGeometricConsts(effInftyFar, bulletZ);
        return true;
    }

    protected virtual void preallocateFrontendOnlyHolders(int specifiedLayer = -1) {
        shouldShakeCamera = false;
        forceCameraTeleport = false;
        skipInterpolation = false;

        //---------------------------------------------FRONTEND USE ONLY SEPERARTION---------------------------------------------
        triggerUdToConfigFromTiled = new Dictionary<ulong, TriggerConfigFromTiled>();
        trapUdToConfigFromTiled = new Dictionary<ulong, TrapConfigFromTiled>();
        characterUdToColorSwapRuleLock = new Dictionary<ulong, int>();
        playerSpeciesIdOccurrenceCnt = new Dictionary<uint, int>();

        playerAnimPool = new JoltCharacterAnimPool(this, chMaterial);
        npcAnimPool = new JoltCharacterAnimPool(this, chMaterial);
        bulletAnimPool = new JoltBulletAnimPool(this, blMaterial);
        trapAnimPool = new JoltTrapAnimPool(this, sprDefaultMaterial);
        triggerAnimPool = new JoltTriggerAnimPool(this, sprDefaultMaterial);
        inplaceHpBarAnimPool = new JoltInplaceHpBarAnimPool(this);
        aimingRayAnimPool = new JoltAimingRayAnimPool(this);
        pickableAnimPool = new JoltPickableAnimPool(this, sprDefaultMaterial);
        gameplayBtnsHintAnimPool = new GameplayBtnsHintAnimPool(this);
        keyChLightSourceAnimPool = new KeyChLightSourceAnimPool(this);
        sfxSourceAnimPool = new SFXSourceAnimPool(this);

        playerAnimPool.ResetUponBattlePreparation(-1);
        npcAnimPool.ResetUponBattlePreparation(-1);
        bulletAnimPool.ResetUponBattlePreparation(-1);
        trapAnimPool.ResetUponBattlePreparation(-1);
        triggerAnimPool.ResetUponBattlePreparation(-1);
        inplaceHpBarAnimPool.ResetUponBattlePreparation(-1);
        aimingRayAnimPool.ResetUponBattlePreparation(-1);
        pickableAnimPool.ResetUponBattlePreparation(-1);
        gameplayBtnsHintAnimPool.ResetUponBattlePreparation(-1);
        keyChLightSourceAnimPool.ResetUponBattlePreparation(-1);
        sfxSourceAnimPool.ResetUponBattlePreparation(-1);

        if (debugDrawingEnabled && null != debugColliderPrefab) {
            lazyInitDebugColliderAnimPool(-1);
        }
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
        return Addressables.LoadAssetAsync<GameObject>(path).WaitForCompletion();
    }

    public GameObject loadBulletPrefab(BulletConfig bulletConfig) {
        string path = $"JoltBulletPrefabs/{bulletConfig.AnimName}";
        return Addressables.LoadAssetAsync<GameObject>(path).WaitForCompletion();
    }

    public GameObject loadTrapPrefab(TrapConfigFromTiled trapConfigFromTiled) {
        var effName = trapConfigFromTiled.Name;
        if (String.IsNullOrEmpty(effName)) {
            var trapConfig = PbTrapsOverride.Instance.getUnderlying()[trapConfigFromTiled.Tpt];
            effName = trapConfig.Name;
        }
        string path = $"JoltTpPrefabs/{effName}";
        return Addressables.LoadAssetAsync<GameObject>(path).WaitForCompletion();
    }

    public GameObject loadTriggerPrefab(TriggerConfigFromTiled triggerConfigFromTiled) {
        var effName = triggerConfigFromTiled.Name;
        if (String.IsNullOrEmpty(effName)) {
            if (PbTriggersOverride.Instance.getUnderlying().ContainsKey(triggerConfigFromTiled.Trt)) {
                var triggerConfig = PbTriggersOverride.Instance.getUnderlying()[triggerConfigFromTiled.Trt];
                effName = triggerConfig.Name;
            }
        }
        string path = $"JoltTrPrefabs/{effName}";
        return Addressables.LoadAssetAsync<GameObject>(path).WaitForCompletion();
    }

    public GameObject loadPickablePrefab(PickableConfig pickableConfig) {
        var pkConfig = PbPickablesOverride.Instance.getUnderlying()[pickableConfig.PickupType];
        string path = $"JoltPkPrefabs/{pkConfig.ActiveAnimName}";
        return Addressables.LoadAssetAsync<GameObject>(path).WaitForCompletion();
    }

    public GameObject loadInplaceHpBarPrefab(CharacterConfig chConfig) {
        return inplaceHpBarPrefab;
    }

    public GameObject loadAimingRayPrefab() {
        return aimingRayPrefab;
    }

    public GameObject loadGameplayBtnsHintPrefab() {
        return gameplayBtnsHintPrefab;
    }

    protected virtual void onBattleStopped() {
        enableBattleInput(false);
        if (null != triggerUdToConfigFromTiled) {
            triggerUdToConfigFromTiled.Clear();
        }
        if (null != trapUdToConfigFromTiled) {
            trapUdToConfigFromTiled.Clear();
        }
        if (UIntPtr.Zero != battle) {
            APP_ClearBattle(battle);
            // [REMINDER] Only call "battle = UIntPtr.Zero" after "APP_DestroyBattle(battle)". 
        }
        battleState = PbPrimitivesOverride.ROOM_STATE_STOPPED;
        battleResultIsSet = false;
        csharpTimerRdfId = -1;
        PreemptRenderFrameBeforeMerge(rdfHolder, PbPrimitivesOverride.Instance.getUnderlying());
    }
    
    protected void enableBattleInput(bool yesOrNo) {
        if (null == iptmgr) return;
        if (false == yesOrNo && !iptmgr.gameObject.activeSelf) return;
        if (iptmgr.enable(yesOrNo)) {
            iptmgr.gameObject.SetActive(yesOrNo);
            iptmgr.TogglePlayerInput(yesOrNo);
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

    protected virtual WsReq createSelfParsedStartRdf(uint[] speciesIdList) {
        Debug.Log($"createSelfParsedStartRdf with speciesIdList={ArrToString(speciesIdList)} for selfJoinIndex={selfJoinIndex}");
        triggerUdToConfigFromTiled.Clear();
        trapUdToConfigFromTiled.Clear();
        var grid = underlyingMap.GetComponent<Grid>();
        uint pickableIdCounter = 1;
        uint npcIdCounter = 1;
        uint dynamicTrapCount = 0;
        uint triggerCount = 0;

        var mapProps = underlyingMap.GetComponent<LevelPlaceholder>();
        if (null != mapProps && 0 != mapProps.durationSeconds) {
            battleDurationSeconds = mapProps.durationSeconds;
            battleDurationFrames = battleDurationSeconds * PbPrimitivesOverride.BATTLE_DYNAMICS_FPS;
        } else {
            battleDurationFrames = PbPrimitivesOverride.Instance.getUnderlying().MagicRdfCntInfinite;
            battleDurationSeconds = battleDurationFrames / PbPrimitivesOverride.BATTLE_DYNAMICS_FPS;
        }

        WsReq result = new WsReq();
        var startRdf = NewPreallocatedRdf(roomCapacity, preallocNpcCapacity, 128, primitives);
        startRdf.BulletIdCounter = 1;
        startRdf.Id = primitives.StartingRenderFrameId;

        foreach (Transform child in grid.transform) {
            var mapLayerMeta = child.GetComponent<MapLayer>();
            if (null == mapLayerMeta) continue;
            switch (mapLayerMeta.primaryLayerType) {
                case MapLayerType.Background:
                case MapLayerType.Foreground:
                    var parallaxEffect = child.gameObject.AddComponent<ParallaxEffect>();
                    parallaxEffect.SetParallaxAmount(mapLayerMeta.xParallaxFactor, mapLayerMeta.yParallaxFactor, gameplayCamera);
                    break;
                case MapLayerType.Barrier:
                    /*
                    // [REMINDER] The "CompositeCollider2D" was once supported, but now intentionally removed due to being too special when compared with the parsing of "Trap" and "Trigger" layers.

                    var colliderOptions = mapLayerMeta.colliderOptions;
                    var unifiedColliderAttr = new BarrierColliderAttr {
                        ProvidesSlipJump = colliderOptions.providesSlipJump,
                        ProvidesStairsP = colliderOptions.providesStairsP,
                        ProvidesStairsN = colliderOptions.providesStairsN,
                        ProhibitsWallGrabbing = !colliderOptions.providesWallGrabbing,
                    };
                    var compositeCollider2D = child.GetComponent<CompositeCollider2D>();
                    if (null != compositeCollider2D) {
                        for (int i = 0; i < compositeCollider2D.pathCount; i++) {
                            int pathPointCnt = compositeCollider2D.GetPathPointCount(i);
                            Vector2[] points = new Vector2[pathPointCnt];
                            compositeCollider2D.GetPath(i, points);
                            var barrierCollider = new SerializedBarrierCollider {
                                Polygon = new SerializableConvexPolygon { }.AddPoints(points.Select(point => new PbVec2 { X = point.x, Y = point.y }).ToList()),
                                Attr = unifiedColliderAttr,
                            };
                            result.SerializedBarriers.Add(barrierCollider);
                        }
                    }
                    */
                    foreach (Transform shapeChild in child) {
                        var serializedConvexPolygon = MapLayer.parseSerializableConvexPolygon(shapeChild);
                        var colliderAttr = new BarrierColliderAttr {
                            ProvidesSlipJump = false,
                            ProvidesStairsP = false,
                            ProvidesStairsN = false,
                            ProhibitsWallGrabbing = true,
                        };
                        var barrierPlaceholder = shapeChild.GetComponent<BarrierPlaceholder>();
                        if (null != barrierPlaceholder) {
                            var colliderOptions = barrierPlaceholder.colliderOptions;
                            colliderAttr.ProvidesSlipJump = colliderOptions.providesSlipJump;
                            colliderAttr.ProvidesStairsP = colliderOptions.providesStairsP;
                            colliderAttr.ProvidesStairsN = colliderOptions.providesStairsN;
                            colliderAttr.ProhibitsWallGrabbing = !colliderOptions.providesWallGrabbing;
                        }
                        if (null != serializedConvexPolygon) {
                            var barrierCollider = new SerializedBarrierCollider {
                                Polygon = serializedConvexPolygon,
                                Attr = colliderAttr,
                            };
                            result.SerializedBarriers.Add(barrierCollider);
                        }
                    }
                    break;
                case MapLayerType.Player:
                    foreach (Transform playerChild in child) {
                        var playerPlaceholder = playerChild.GetComponent<PlayerPlaceholder>();
                        var characterOptions = playerPlaceholder.characterOptions;
                        var teamIdVal = characterOptions.teamId;
                        int j = teamIdVal - 1;
                        if (j >= speciesIdList.Length) {
                            continue;
                        }
                        uint speciesId = speciesIdList[j];

                        if (teamIdVal != selfPlayerInfo.JoinIndex && speciesId == PbPrimitivesOverride.SPECIES_NONE_CH) {
                            continue; // [TODO] Use a dedicated "joinIndex" property!
                        }
                        var playerChd = startRdf.Players[j];
                        var chd = playerChd.Chd;
                        PreemptCharacterDownsyncBeforeMerge(chd, PbPrimitivesOverride.Instance.getUnderlying());
                        parseCharacterDownsync(playerChild, speciesId, characterOptions, ref chd);

                        uint joinIndex = (uint)(j + 1);
                        playerChd.JoinIndex = joinIndex;
                        playerChd.RevivalX = chd.X;
                        playerChd.RevivalY = chd.Y;
                        playerChd.RevivalQX = chd.QX;
                        playerChd.RevivalQY = chd.QY;
                        playerChd.RevivalQZ = chd.QZ;
                        playerChd.RevivalQW = chd.QW;
                    }
                    Destroy(child.gameObject); // Delete the whole "ObjectLayer"
                    break;
                case MapLayerType.Npc:
                    foreach (Transform npcChild in child) {
                        var npcPlaceholder = npcChild.GetComponent<NpcPlaceholder>();
                        var characterOptions = npcPlaceholder.characterOptions;
                        var teamIdVal = characterOptions.teamId;
                        var characterConfig = PbCharactersOverride.Instance.GetCharacterConfig(npcChild.name);
                        uint speciesId = characterConfig.SpeciesId;

                        int npcArrIdx = (int)(npcIdCounter - 1);
                        var npc = startRdf.Npcs[npcArrIdx];
                        
                        npc.Id = npcIdCounter++;
                        npc.GoalAsNpc = npcPlaceholder.initGoal;
                        npc.PublishingToTriggerIdUponExhausted = npcPlaceholder.publishingToTriggerIdUponExhausted;
                        npc.SubscribesToTriggerId = npcPlaceholder.subscribesToTriggerId;
                        npc.IsMainTowerOfTeam = npcPlaceholder.isMainTowerOfTeam;
                        var chd = npc.Chd;
                        PreemptCharacterDownsyncBeforeMerge(chd, PbPrimitivesOverride.Instance.getUnderlying());
                        parseCharacterDownsync(npcChild, speciesId, characterOptions, ref chd);
                        if (primitives.TerminatingTriggerId != npc.SubscribesToTriggerId) {
                            chd.ChState = CharacterState.Dimmed;
                        }
                    }
                    Destroy(child.gameObject); // Delete the whole "ObjectLayer"
                    break;
                case MapLayerType.Trap:
                    foreach (Transform trapChild in child) {
                        var (trap, configFromTiled) = parseTrap(trapChild);
                        startRdf.DynamicTraps.Add(trap);
                        result.TrapConfigFromTileList.Add(configFromTiled);
                        var ud = APP_CalcTrapUserData(trap.Id);
                        trapUdToConfigFromTiled[ud] = configFromTiled;
                        dynamicTrapCount++;
                    }
                    Destroy(child.gameObject); // Delete the whole "ObjectLayer"
                    break;
                case MapLayerType.Trigger:
                    foreach (Transform triggerChild in child) {
                        var (trigger, configFromTiled) = parseTrigger(triggerChild);
                        startRdf.Triggers.Add(trigger);
                        result.TriggerConfigFromTileList.Add(configFromTiled);
                        var ud = APP_CalcTriggerUserData(trigger.Id);
                        triggerUdToConfigFromTiled[ud] = configFromTiled;
                        ++triggerCount;
                    }
                    Destroy(child.gameObject); // Delete the whole "ObjectLayer"
                    break;
                default:
                    break;
            }
        }

        startRdf.NpcIdCounter = npcIdCounter;
        startRdf.NpcCount = npcIdCounter - 1;

        startRdf.PickableIdCounter = pickableIdCounter;
        startRdf.PickableCount = pickableIdCounter - 1;

        startRdf.DynamicTrapCount = dynamicTrapCount;
        startRdf.TriggerCount = triggerCount;

        result.SelfParsedRdf = startRdf;

        return result;
    }

    protected float camFovW, camFovH, camPaddingX, camPaddingY;
    protected virtual void calcCameraCaps() {
        effInftyFar = 65535f;
        cameraCapMinX = -effInftyFar;
        cameraCapMaxX = +effInftyFar;
        cameraCapMinY = -effInftyFar;
        cameraCapMaxY = +effInftyFar;
        var grid = underlyingMap.GetComponent<Grid>();
        foreach (Transform child in grid.transform) {
            var mapLayerMeta = child.GetComponent<MapLayer>();
            if (null == mapLayerMeta) continue;
            switch (mapLayerMeta.primaryLayerType) {
                case MapLayerType.GrandCameraBound: {
                        var boxCollider2D = child.GetComponent<BoxCollider2D>();

                        // This layer can be absent.
                        var (candidateCamMinWx, candidateCamMinWy) = (boxCollider2D.bounds.min.x, boxCollider2D.bounds.min.y);
                        var (candidateCamMaxWx, candidateCamMaxWy) = (boxCollider2D.bounds.max.x, boxCollider2D.bounds.max.y);

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
                    Destroy(child.gameObject); // Delete the whole "ObjectLayer"
                    break;
            }
        }

        // Now pad the camera caps.
        camFovW = (2.0f * gameplayCamera.orthographicSize * gameplayCamera.aspect);
        camFovH = (2.0f * gameplayCamera.orthographicSize);
        camPaddingX = (camFovW * .5f);
        camPaddingY = (camFovH * .5f);
        cameraCapMinX += camPaddingX;
        cameraCapMaxX -= camPaddingX;
        cameraCapMinY += camPaddingY;
        cameraCapMaxY -= camPaddingY;

        grandCameraCapMinX = cameraCapMinX;
        grandCameraCapMaxX = cameraCapMaxX;
        grandCameraCapMinY = cameraCapMinY;
        grandCameraCapMaxY = cameraCapMaxY;

        npcInterpolationDis2Threshold = (effInftyFar * 0.125f) * (effInftyFar * 0.125f);
        playerAnimPool.SetGeometricConsts(effInftyFar, characterZ);
        npcAnimPool.SetGeometricConsts(effInftyFar, characterZ);
        bulletAnimPool.SetGeometricConsts(effInftyFar, bulletZ);
        trapAnimPool.SetGeometricConsts(effInftyFar, trapZ);
        triggerAnimPool.SetGeometricConsts(effInftyFar, triggerZ);
        inplaceHpBarAnimPool.SetGeometricConsts(effInftyFar, inplaceHpBarZ);
        pickableAnimPool.SetGeometricConsts(effInftyFar, pickableZ);
        gameplayBtnsHintAnimPool.SetGeometricConsts(effInftyFar, bulletZ);
        keyChLightSourceAnimPool.SetGeometricConsts(effInftyFar, characterZ);
        sfxSourceAnimPool.SetGeometricConsts(effInftyFar, characterZ);
        aimingRayAnimPool.SetGeometricConsts(effInftyFar, bulletZ);
        if (debugDrawingEnabled && null != debugColliderPrefab) {
            lazyInitDebugColliderAnimPool(-1);
        }
        selfBattleHeading.ResetSelf();
    }

    protected virtual void parseCharacterDownsync(Transform ts, uint speciesId, CharacterOptions characterOptions, ref CharacterDownsync chd) {
        var chConfig = characters[speciesId];
        chd.X = ts.position.x;
        chd.Y = ts.position.y;
        chd.Speed = chConfig.Speed;
        chd.RemainingAirJumpQuota = chConfig.DefaultAirJumpQuota;
        chd.RemainingAirDashQuota = chConfig.DefaultAirDashQuota;
        chd.ChState = CharacterState.InAirIdle1NoJump;
        chd.FramesToRecover = 0;
        chd.QX = ts.rotation.x;
        chd.QY = ts.rotation.y;
        chd.QZ = ts.rotation.z;
        chd.QW = ts.rotation.w;
        chd.AimingQX = 0;
        chd.AimingQY = 0;
        chd.AimingQZ = 0;
        chd.AimingQW = 1;
        chd.VelX = 0;
        chd.VelY = 0;
        chd.BulletTeamId = characterOptions.teamId;
        chd.Hp = chConfig.Hp;
        chd.Mp = chConfig.Mp;
        chd.SpeciesId = chConfig.SpeciesId;
    }

    protected virtual (Trap, TrapConfigFromTiled) parseTrap(Transform child) {
        
        TrapPlaceholder trapPlaceholder = child.GetComponent<TrapPlaceholder>();
        uint trapId = trapPlaceholder.id;
        uint tptVal = trapPlaceholder.tpt;

        if (PbPrimitivesOverride.TPT_NONE == tptVal) {
            throw new ArgumentNullException($"Property tpt MUST be set for ts in TrapStartingPos for trapId={trapId}");
        }

        TrapConfigFromTiled trapConfigFromTiled = trapPlaceholder.buildTrapConfigFromTiled();

        Trap trap = new Trap {
            Id = trapId,
            Tpt = tptVal,
            X = trapConfigFromTiled.InitX,
            Y = trapConfigFromTiled.InitY,
            Z = trapConfigFromTiled.InitZ,
            QX = trapConfigFromTiled.InitQX,
            QY = trapConfigFromTiled.InitQY,
            QZ = trapConfigFromTiled.InitQZ,
            QW = trapConfigFromTiled.InitQW,
        };

        return (trap, trapConfigFromTiled);
    }

    protected virtual (Trigger, TriggerConfigFromTiled) parseTrigger(Transform child) {
        TriggerPlaceholder triggerPlaceholder = child.GetComponent<TriggerPlaceholder>();

        uint triggerId = triggerPlaceholder.id;
        uint trtVal = triggerPlaceholder.trt;

        if (PbPrimitivesOverride.TRT_NONE == trtVal) {
            throw new ArgumentNullException($"Property trt MUST be set for ts in TrapStartingPos for triggerId={triggerId}");
        }

        TriggerConfigFromTiled triggerConfigFromTiled = triggerPlaceholder.buildTriggerConfigFromTiled();

        Trigger trigger = new Trigger {
            Id = triggerId,
            Trt = trtVal,
            X = child.position.x,
            Y = child.position.y,
            Z = child.position.z,
        };

        return (trigger, triggerConfigFromTiled);
    }

    protected Vector3 newPosHolder = new Vector3();
    protected Vector3 newTlPosHolder = new Vector3(), newTrPosHolder = new Vector3(), newBlPosHolder = new Vector3(), newBrPosHolder = new Vector3();
    protected Vector3 pointInCamViewPortHolder = new Vector3();
    protected Vector2 camDiffDstHolder = new Vector2();

    protected virtual void clampToMapBoundary(ref Vector3 posHolder, in float dt) {
        float newX = posHolder.x, newY = posHolder.y, newZ = posHolder.z;
        if (newX > grandCameraCapMaxX) {
            newX = grandCameraCapMaxX;
        }
        if (newX < grandCameraCapMinX) {
            newX = grandCameraCapMinX;
        }
        if (newY > grandCameraCapMaxY) {
            newY = grandCameraCapMaxY;
        }
        if (newY < grandCameraCapMinY) {
            newY = grandCameraCapMinY;
        }

        posHolder.Set(newX, newY, newZ);
    }

    protected float SQUARED_DISTANCE_NO_MOVE_THRESHOLD = 0.02125f / DEFAULT_PIXELS_PER_UNIT;
    protected float CROUCH_PEEKING_CAM_SPEED_FACTOR = 1.5f;
    protected float camCurrSpeedPerSecond = 0.0f;
    protected float CAM_SPEED_LINEAR_DAMPING = 0.2f;
    protected float CAM_REGULAR_ACC_PER_SECOND = 6.5f;
    protected float CAM_FAST_ACC_PER_SECOND = 8.0f;

    protected virtual void shakeCameraIfApplicable(ref Vector3 posHolder) {
        if (!shouldShakeCamera) {
            return;
        }

        // Only the local player's hit stun should shake the gameplay camera.
        if (0 == hitSelfStunShakeFramesRemaining) {
            hitSelfStunShakeSeed = UnityEngine.Random.Range(0.0f, 1000.0f);
        }

        hitSelfStunShakeFramesRemaining = Mathf.Max(
            hitSelfStunShakeFramesRemaining,
            hitSelfStunShakeDurationFrames);

        if (null != gameplayCamera) {
            float progress = (float)hitSelfStunShakeFramesRemaining / Mathf.Max(1, hitSelfStunShakeDurationFrames);
            float noiseTime = hitSelfStunShakeSeed + Time.time * hitSelfStunShakeFrequency;
            float shakeX = Mathf.PerlinNoise(noiseTime, 0.0f) * 2.0f - 1.0f;
            float shakeY = Mathf.PerlinNoise(0.0f, noiseTime) * 2.0f - 1.0f;

            posHolder += new Vector3(
                shakeX * hitSelfStunShakeAmplitude * progress,
                shakeY * hitSelfStunShakeAmplitude * progress,
                0.0f);
        }

        --hitSelfStunShakeFramesRemaining;
    }

    protected virtual void cameraTrack(RenderFrame rdf, RenderFrame prevRdf) {
        float dt = Time.deltaTime;
        uint targetJoinIndex = selfJoinIndex; // TODO
        ulong playerUd = APP_CalcPlayerUserData(targetJoinIndex);
        int joinIndexArrIdx = (int)targetJoinIndex - 1;
        var playerChd = rdf.Players[joinIndexArrIdx];
        var chd = playerChd.Chd;
        var chConfig = characters[chd.SpeciesId];
        var (chGameObj, oldUd) = playerAnimPool.GetOrCreateAnimNode(playerUd, chd.SpeciesId, chConfig, underlyingMap.transform);

        if (forceCameraTeleport) {
            var (wx, wy) = (chd.X, chd.Y);
            newPosHolder.Set(wx, wy, chGameObj.transform.position.z);
            chGameObj.transform.position = newPosHolder;

            newPosHolder.Set(wx, wy, defaultGameplayCamZ);
            clampToMapBoundary(ref newPosHolder, dt);
            gameplayCamera.transform.position = newPosHolder;
            forceCameraTeleport = false;
            return;
        }

        bool isCrouching = (
                             CharacterState.CrouchIdle1 == chd.ChState
                           || CharacterState.CrouchAtk1 == chd.ChState
                           || CharacterState.CrouchAtked1 == chd.ChState
                            );

        bool isProactivelyJumping = (
                             CharacterState.InAirIdle1ByJump == chd.ChState
                           || CharacterState.InAirIdle2ByJump == chd.ChState
                           || CharacterState.InAirIdle1BySlipJump == chd.ChState
                           || CharacterState.InAirIdle1ByWallJump == chd.ChState
                            );
        float camMaxSpeedPerSecond = chd.Speed;
        float camEffAccPerSecond = CAM_REGULAR_ACC_PER_SECOND;
        bool isChdMoving = (PbPrimitivesOverride.Instance.getUnderlying().LengthEps < Mathf.Abs(chd.VelX - chd.GroundVelX) || PbPrimitivesOverride.Instance.getUnderlying().LengthEps < Mathf.Abs(chd.VelY - chd.GroundVelY));

        var camOldPos = gameplayCamera.transform.position;
        newPosHolder.Set(chGameObj.transform.position.x, chGameObj.transform.position.y, defaultGameplayCamZ);

        var (offCameraCenterRadioSquaredX, offCameraCenterRadioSquaredY) = calcOffCameraCenterRatioSquared(newPosHolder);
        bool justDead = ((0 >= chd.Hp) || (0 < chd.NewBirthRdfCountdown));
        if (justDead || battleResultIsSet) {
            newPosHolder.Set(chGameObj.transform.position.x, chGameObj.transform.position.y, defaultGameplayCamZ);
            camEffAccPerSecond = CAM_FAST_ACC_PER_SECOND;
            camCurrSpeedPerSecond = Mathf.Lerp(camCurrSpeedPerSecond, camMaxSpeedPerSecond, camEffAccPerSecond * dt);
        } else {
            if (offCameraCenterRadioSquaredX > SQUARED_DISTANCE_NO_MOVE_THRESHOLD || offCameraCenterRadioSquaredY > SQUARED_DISTANCE_NO_MOVE_THRESHOLD) {
                newPosHolder.Set(chGameObj.transform.position.x, chGameObj.transform.position.y, defaultGameplayCamZ);
                if (isChdMoving) {
                    float offCamTrackingBoost = 5f * (offCameraCenterRadioSquaredX + offCameraCenterRadioSquaredY - SQUARED_DISTANCE_NO_MOVE_THRESHOLD);
                    float verticalTrackingBoost = 0.1f * (Math.Abs(chd.VelY) / chConfig.Speed);
                    camMaxSpeedPerSecond *= (1f + offCamTrackingBoost + verticalTrackingBoost);
                } else if (isCrouching) {
                    camMaxSpeedPerSecond = CROUCH_PEEKING_CAM_SPEED_FACTOR * chd.Speed;
                }
                camCurrSpeedPerSecond = Mathf.Lerp(camCurrSpeedPerSecond, camMaxSpeedPerSecond, camEffAccPerSecond * dt);

                // [REMINDER] Regularly looking a bit upward.
                newPosHolder.y = (chGameObj.transform.position.y + chConfig.CapsuleHalfHeight * 1.5f);
            } else {
                newPosHolder.Set(camOldPos.x, camOldPos.y, defaultGameplayCamZ);
                camCurrSpeedPerSecond = camCurrSpeedPerSecond * Math.Max(0.0f, 1.0f - CAM_SPEED_LINEAR_DAMPING);
                if (isCrouching) {
                    camMaxSpeedPerSecond = CROUCH_PEEKING_CAM_SPEED_FACTOR * chd.Speed;
                    camCurrSpeedPerSecond = Mathf.Lerp(camCurrSpeedPerSecond, camMaxSpeedPerSecond, camEffAccPerSecond * dt);
                }
            }

            // [REMINDER] When crouching or jumping, fine tune camera position.
            if (isCrouching) {
                newPosHolder.y = (chGameObj.transform.position.y - chConfig.CapsuleHalfHeight * 3.5f);
            } else if (isProactivelyJumping) {
                newPosHolder.y = (chGameObj.transform.position.y + chConfig.CapsuleHalfHeight * 3.0f);
            }
        }

        camCurrSpeedPerSecond = Math.Max(0.1f * camMaxSpeedPerSecond, camCurrSpeedPerSecond); // Don't be too small such that "stepLength" becomes essentially 0
        camDiffDstHolder.Set(newPosHolder.x - camOldPos.x, newPosHolder.y - camOldPos.y);
        float camDiffMagnitude = camDiffDstHolder.magnitude;
        var stepLength = camCurrSpeedPerSecond*dt;
        if (stepLength > camDiffMagnitude) {
            // Immediately teleport
        } else {
            var newMapPosDiff2 = camDiffDstHolder.normalized * stepLength;
            newPosHolder.Set(camOldPos.x + newMapPosDiff2.x, camOldPos.y + newMapPosDiff2.y, defaultGameplayCamZ);
        }
        shakeCameraIfApplicable(ref newPosHolder);
        clampToMapBoundary(ref newPosHolder, dt);

        gameplayCamera.transform.position = newPosHolder;
    }

    protected void assignShouldShakeCameraIfApplicable(in PlayerCharacterDownsync playerChd) {
        var chd = playerChd.Chd;
        if ((CharacterState.Dying == chd.ChState || CharacterState.BlownUp1 == chd.ChState || CharacterState.Def1Broken == chd.ChState) && chd.FramesInChState < hitSelfStunShakeDurationFrames) {
            if (playerChd.JoinIndex == selfJoinIndex) {
                shouldShakeCamera = true;
            } else if (chd.LastDamagedByUd== GetSelfUd()) {
                shouldShakeCamera = true;
            }
        } else if (0 < chd.HitSelfStunFrames) {
            if (playerChd.JoinIndex == selfJoinIndex) {
                shouldShakeCamera = true;
            }
        }
    } 

    protected void assignShouldShakeCameraIfApplicable(in NpcCharacterDownsync npcChd) {
        var chd = npcChd.Chd;
        if ((CharacterState.Dying == chd.ChState || CharacterState.BlownUp1 == chd.ChState || CharacterState.Def1Broken == chd.ChState) && chd.FramesInChState < hitSelfStunShakeDurationFrames) {
            if (chd.LastDamagedByUd == GetSelfUd()) {
                shouldShakeCamera = true;
            }
        }
    } 

    protected virtual float calcOnMapSfxAttenuation(in float wx, in float wy, in float anchorWx, in float anchorWy) {
        float dx = (wx - anchorWx), dy = (wy - anchorWy);
        float dis2 = dx*dx + dy*dy;
        float distanceAttenuation = (dis2 < SFX_ATTENUATION_REFERENCE_DISTANCE2) ? 0 : 0.1f*(SFX_ATTENUATION_REFERENCE_DISTANCE2/dis2);
        return distanceAttenuation;
    }

    protected virtual void applyRdf(RenderFrame rdf, StepResult? stepResult, float dtSeconds) {
        int rdfId = rdf.Id;
        float selfPlayerWx = 0f, selfPlayerWy = 0f;
        activeChdDictionary.Clear();
        activeTriggerDictionary.Clear();
        shouldShakeCamera = false;
        for (int k = 0; k < roomCapacity; k++) {
            var player = rdf.Players[k];
            ulong playerUd = Bindings.APP_CalcPlayerUserData(player.JoinIndex);
            var currCharacterDownsync = player.Chd;
            var chConfig = characters[currCharacterDownsync.SpeciesId];
            activeChdDictionary.Add(playerUd, currCharacterDownsync);

            var (wx, wy) = (currCharacterDownsync.X, currCharacterDownsync.Y);

            bool shouldAttachLightSource = false;

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

                if (allowSelfEmittingLight && null != keyChLightSource2DPrefab) {
                    shouldAttachLightSource = true;
                }
            }

            assignShouldShakeCameraIfApplicable(in player); 

            var (chAnimCtrlRaw, oldUd) = playerAnimPool.GetOrCreateAnimNode(playerUd, currCharacterDownsync.SpeciesId, chConfig, underlyingMap.transform);
            var chAnimCtrl = chAnimCtrlRaw as JoltCharacterAnimController;
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
            int effFramesInChState = currCharacterDownsync.FramesInChState;
            if (0 < currCharacterDownsync.HitSelfStunFrames) {     
                if (PbPrimitivesOverride.Instance.getUnderlying().NoSkill != currCharacterDownsync.ActiveSkillId && PbPrimitivesOverride.Instance.getUnderlying().NoSkillHit != currCharacterDownsync.ActiveSkillHit) {
                    var (skillConfig, bulletConfig) = FindBulletConfig(currCharacterDownsync.ActiveSkillId, currCharacterDownsync.ActiveSkillHit, PbPrimitivesOverride.Instance.getUnderlying(), PbSkillsOverride.Instance);
                    int deltaRdfCnt = (bulletConfig.MeleeHitSelfStunFrames - currCharacterDownsync.HitSelfStunFrames);
                    if (effFramesInChState > 1 + bulletConfig.StartupFrames + deltaRdfCnt) {
                        effFramesInChState -= deltaRdfCnt;
                    }
                    /*
                    Debug.Log($"@csharpTimerRdfId={csharpTimerRdfId}, self HitSelfStunFrames={currCharacterDownsync.HitSelfStunFrames}, oldFc={currCharacterDownsync.FramesInChState}, fr={currCharacterDownsync.FramesToRecover}, activeSkillId={currCharacterDownsync.ActiveSkillId}, activeSkillHit={currCharacterDownsync.ActiveSkillHit} for effFramesInChState calc, newEffFramesInChState={effFramesInChState}");
                    */
                }
            }
            chAnimCtrl.updateAnim(rdfId, playerUd, currCharacterDownsync, currCharacterDownsync.ChState, chConfig, effFramesInChState);

            if (debugDrawingEnabled && null != debugColliderPrefab) {
                lazyInitDebugColliderAnimPool(rdfId);
                var cacheGroupId = debugColliderAnimPool.CalcMeshType(currCharacterDownsync, chConfig);
                var (animCtrl, oldAnimUd) = debugColliderAnimPool.GetOrCreateAnimNode(playerUd, 0, chConfig, underlyingMap.transform);
                animCtrl.updateAnim(rdfId, playerUd, currCharacterDownsync, currCharacterDownsync.ChState, chConfig, 0);
                var cachedNewPosHolder = newPosHolder;
                newPosHolder.Set(newPosHolder.x, newPosHolder.y + animCtrl.GetCapsuleHalfHeight(), newPosHolder.z - 1);
                animCtrl.transform.position = newPosHolder;
                newPosHolder = cachedNewPosHolder;
            }

            if (shouldAttachLightSource) {
                var (keyChLightSourceCtrl, oldLightSourceUd) = keyChLightSourceAnimPool.GetOrCreateAnimNode(playerUd, currCharacterDownsync.SpeciesId, chConfig, underlyingMap.transform);
                newPosHolder.y = (newPosHolder.y + chConfig.CapsuleHalfHeight);
                keyChLightSourceCtrl.gameObject.transform.position = newPosHolder;
                keyChLightSourceCtrl.updateAnim(rdfId, playerUd, currCharacterDownsync, currCharacterDownsync.ChState, chConfig, currCharacterDownsync.FramesInChState);
            }

            // Add character vfx
            string sfxName = sfxSourceAnimPool.CalcSfxName(currCharacterDownsync, chConfig);
            if (!String.IsNullOrEmpty(sfxName)) {
                var (sfxHolder, oldSfxUd) = sfxSourceAnimPool.GetOrCreateAnimNode(playerUd, sfxName, chConfig, underlyingMap.transform);
                sfxHolder.SetRealtimeAttenuation(calcOnMapSfxAttenuation(wx, wy, selfPlayerWx, selfPlayerWy));
                sfxHolder.updateAnim(rdfId, playerUd, currCharacterDownsync, currCharacterDownsync.ChState, chConfig, currCharacterDownsync.FramesInChState);
                sfxHolder.gameObject.transform.position = newPosHolder;
            }

            playCharacterDamagedVfx(rdf.Id, currCharacterDownsync, chConfig, chAnimCtrl.gameObject, wx, wy, chAnimCtrl, playerUd, material, false);
        }

        for (int k = 0; k < rdf.NpcCount; k++) {
            var npc = rdf.Npcs[k];
            if (PbPrimitivesOverride.Instance.getUnderlying().TerminatingCharacterId == npc.Id) break;
            ulong npcUd = Bindings.APP_CalcNpcUserData(npc.Id);
            var currCharacterDownsync = npc.Chd;
            var chConfig = characters[currCharacterDownsync.SpeciesId];
            activeChdDictionary.Add(npcUd, currCharacterDownsync);

            var (wx, wy) = (currCharacterDownsync.X, currCharacterDownsync.Y);

            var (chAnimCtrlRaw, oldUd) = npcAnimPool.GetOrCreateAnimNode(npcUd, currCharacterDownsync.SpeciesId, chConfig, underlyingMap.transform);
            var chAnimCtrl = chAnimCtrlRaw as JoltCharacterAnimController;
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

            if (debugDrawingEnabled && null != debugColliderPrefab) {
                lazyInitDebugColliderAnimPool(rdfId);
                var cacheGroupId = debugColliderAnimPool.CalcMeshType(currCharacterDownsync, chConfig);
                var (animCtrl, oldAnimUd) = debugColliderAnimPool.GetOrCreateAnimNode(npcUd, 0, chConfig, underlyingMap.transform);
                animCtrl.updateAnim(rdfId, npcUd, currCharacterDownsync, currCharacterDownsync.ChState, chConfig, 0);
                var cachedNewPosHolder = newPosHolder;
                newPosHolder.Set(newPosHolder.x, newPosHolder.y + animCtrl.GetCapsuleHalfHeight(), newPosHolder.z - 1);
                animCtrl.transform.position = newPosHolder;
                newPosHolder = cachedNewPosHolder;
            }

            // Add character vfx
            float distanceAttenuationZ = Math.Abs(wx - selfPlayerWx) + Math.Abs(wy - selfPlayerWy);
            playCharacterDamagedVfx(rdf.Id, currCharacterDownsync, chConfig, chAnimCtrl.gameObject, wx, wy, chAnimCtrl, npcUd, material, false);

            assignShouldShakeCameraIfApplicable(in npc); 
        }

        for (int k = 0; k < rdf.BulletCount; k++) {
            var bullet = rdf.Bullets[k];
            if (primitives.TerminatingBulletId == bullet.Id) break;
            var (skillConfig, bulletConfig) = FindBulletConfig(bullet.SkillId, bullet.ActiveSkillHit, PbPrimitivesOverride.Instance.getUnderlying(), PbSkillsOverride.Instance);
            if (null == skillConfig || null == bulletConfig) continue;

            (newTlPosHolder.x, newTlPosHolder.y) = (bullet.X - bulletConfig.HitboxHalfSizeX, bullet.Y + bulletConfig.HitboxHalfSizeY); // Top-left

            (newTrPosHolder.x, newTrPosHolder.y) = (bullet.X + bulletConfig.HitboxHalfSizeX, bullet.Y + bulletConfig.HitboxHalfSizeY); // Top-right

            (newBrPosHolder.x, newBrPosHolder.y) = (bullet.X + bulletConfig.HitboxHalfSizeX, bullet.Y - bulletConfig.HitboxHalfSizeY); // bottom-right

            (newBlPosHolder.x, newBlPosHolder.y) = (bullet.X - bulletConfig.HitboxHalfSizeX, bullet.Y - bulletConfig.HitboxHalfSizeY); // bottom-left

            if (!bulletConfig.BeamCollision && !bulletConfig.BeamRendering) {
                if (!isGameObjPositionWithinCamera(newTlPosHolder) && !isGameObjPositionWithinCamera(newTrPosHolder) && !isGameObjPositionWithinCamera(newBlPosHolder) && !isGameObjPositionWithinCamera(newBrPosHolder)) continue;
            }

            var bulletUd = Bindings.APP_CalcBulletUserData(bullet.Id);
            var (wx, wy) = (bullet.X, bullet.Y);
            newPosHolder.Set(wx, wy, bulletZ);

            string sfxName = sfxSourceAnimPool.CalcSfxName(bullet, bulletConfig);
            if (!String.IsNullOrEmpty(sfxName)) {
                var (sfxHolder, oldUd) = sfxSourceAnimPool.GetOrCreateAnimNode(bulletUd, sfxName, bulletConfig, underlyingMap.transform);
                sfxHolder.SetRealtimeAttenuation(calcOnMapSfxAttenuation(wx, wy, selfPlayerWx, selfPlayerWy));
                sfxHolder.updateAnim(rdfId, bulletUd, bullet, bullet.BlState, bulletConfig, bullet.FramesInBlState);
                sfxHolder.gameObject.transform.position = newPosHolder;
            }

            if (!String.IsNullOrEmpty(bulletConfig.AnimName)) {
                var (bulletAnimHolder, oldUd) = bulletAnimPool.GetOrCreateAnimNode(bulletUd, bulletConfig.AnimName, bulletConfig, underlyingMap.transform);
                bulletAnimHolder.damageDealedIndicatorPrefab = damageIndicatorPrefab;
                bulletAnimHolder.gameObject.transform.position = newPosHolder;
                bulletAnimHolder.updateAnim(rdfId, bulletUd, bullet, bullet.BlState, bulletConfig, bullet.FramesInBlState);
                /*
                playBulletVfx(bullet, bulletConfig, isStartup, isExploding, wx, wy, rdf);
                */
            }

            if (debugDrawingEnabled && null != debugColliderPrefab) {
                if (BulletState.Active == bullet.BlState) {
                    bool shouldRenderDebugDraw = true;
                    if (BulletType.Melee == bulletConfig.BType && 0 >= bullet.FramesInBlState) {
                        shouldRenderDebugDraw = false;
                    }
                    if (shouldRenderDebugDraw) {
                        lazyInitDebugColliderAnimPool(rdfId);
                        var cacheGroupId = debugColliderAnimPool.CalcMeshType(bullet, bulletConfig);
                        var (animCtrl, oldUd) = debugColliderAnimPool.GetOrCreateAnimNode(bulletUd, cacheGroupId, bulletConfig, underlyingMap.transform);
                        animCtrl.updateAnim(rdfId, bulletUd, bullet, bullet.BlState, bulletConfig, 0);
                        newPosHolder.Set(newPosHolder.x, newPosHolder.y, newPosHolder.z - 1);
                        animCtrl.transform.position = newPosHolder;
                        newPosHolder.Set(newPosHolder.x, newPosHolder.y, newPosHolder.z + 1);
                    }
                }
            }
        }

        for (int k = 0; k < rdf.DynamicTrapCount; k++) {
            var tp = rdf.DynamicTraps[k];
            if (PbPrimitivesOverride.Instance.getUnderlying().TerminatingTrapId == tp.Id) break;
            ulong trapUd = Bindings.APP_CalcTrapUserData(tp.Id);
            var tpConfigFromTiled = trapUdToConfigFromTiled[trapUd];
            var (tpAnimCtrl, oldUd) = trapAnimPool.GetOrCreateAnimNode(trapUd, tp.Tpt, tpConfigFromTiled, underlyingMap.transform);
            if (trapUd != oldUd) {
                tpAnimCtrl.SetAbsScale(tpConfigFromTiled.RenderBoxHalfSizeX, tpConfigFromTiled.RenderBoxHalfSizeY);
            }
            TrapState effTpState = tp.TrapState;
            int effFramesInTpState = tp.FramesInTrapState;
            var (wx, wy) = (tp.X, tp.Y);
            newPosHolder.Set(wx, wy, trapZ);

            tpAnimCtrl.gameObject.transform.position = newPosHolder;
            tpAnimCtrl.updateAnim(rdfId, trapUd, tp, effTpState, tpConfigFromTiled, effFramesInTpState);

        }

        for (int k = 0; k < rdf.TriggerCount; k++) {
            var trigger = rdf.Triggers[k];
            if (PbPrimitivesOverride.Instance.getUnderlying().TerminatingTriggerId == trigger.Id) break;
            ulong triggerUd = Bindings.APP_CalcTriggerUserData(trigger.Id);
            activeTriggerDictionary[triggerUd] = trigger;
            var triggerConfigFromTiled = triggerUdToConfigFromTiled[triggerUd];

            if (!triggerConfigFromTiled.ShouldRender) {
                continue;
            }

            var (trAnimCtrl, oldUd) = triggerAnimPool.GetOrCreateAnimNode(triggerUd, trigger.Trt, triggerConfigFromTiled, underlyingMap.transform);
            if (triggerUd != oldUd) {
                trAnimCtrl.SetAbsScale(triggerConfigFromTiled.RenderBoxHalfSizeX, triggerConfigFromTiled.RenderBoxHalfSizeY);
            }

            var (wx, wy) = (trigger.X, trigger.Y);

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

            var (wx, wy) = (pickable.X, pickable.Y);

            newPosHolder.Set(wx, wy, trapZ);

            animCtrl.gameObject.transform.position = newPosHolder;

            animCtrl.updateAnim(rdfId, ud, pickable, pickable.PkState, pickableConfig, pickable.FramesInPkState);
        }

        if (null != stepResultHolder) {
            for (int k = 0; k < stepResultHolder.AimingRayCount; k++) {
                var aimingRay = stepResultHolder.AimingRays[k];

                var (animCtrl, oldUd) = aimingRayAnimPool.GetOrCreateAnimNode(aimingRay.OffenderUd, cacheGroupId: 0, insConfig: 0, underlyingMap.transform);
                animCtrl.updateAnim(rdfId, aimingRay.OffenderUd, aimingRay, 0, 0, 0);

                var (wStX, wStY) = (0.5f * (aimingRay.StX + aimingRay.EdX), 0.5f * (aimingRay.StY + aimingRay.EdY));

                newPosHolder.Set(wStX, wStY, 0);
                animCtrl.transform.position = newPosHolder;
            }

            foreach (var preparedTriggerUd in stepResultHolder.PreparedTriggerUds) {
                var trigger = activeTriggerDictionary[preparedTriggerUd];
                var triggerConfigFromTiled = triggerUdToConfigFromTiled[preparedTriggerUd];

                var (wx, wy) = (trigger.X, trigger.Y + triggerConfigFromTiled.BoxHalfSizeY * 2);

                var (gameplayBtnsHintCtrl, oldUd) = gameplayBtnsHintAnimPool.GetOrCreateAnimNode(preparedTriggerUd, triggerConfigFromTiled.Trt, triggerConfigFromTiled, underlyingMap.transform);

                newPosHolder.Set(wx, wy, bulletZ);

                gameplayBtnsHintCtrl.gameObject.transform.position = newPosHolder;

                gameplayBtnsHintCtrl.updateAnim(rdfId, preparedTriggerUd, trigger, TriggerState.TrReady, triggerConfigFromTiled, 0);
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
        gameplayBtnsHintAnimPool.HideInactiveAnimNodes(rdfId);
        keyChLightSourceAnimPool.HideInactiveAnimNodes(rdfId);
        sfxSourceAnimPool.HideInactiveAnimNodes(rdfId);

        if (debugDrawingEnabled) {
            if (null != debugColliderAnimPool) {
                debugColliderAnimPool.HideInactiveAnimNodes(rdfId);
            } else if (null != debugColliderPrefab) {
                lazyInitDebugColliderAnimPool(rdfId);
                debugColliderAnimPool.HideInactiveAnimNodes(rdfId);
            }
        }
    }
#nullable enable

    protected bool shouldShakeCamera = false;
    protected bool skipInterpolation = false;
    protected bool forceCameraTeleport = false;

    protected virtual float calcEffCharacterZ(CharacterDownsync currCd, CharacterConfig chConfig) {
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

    protected virtual void resetLevelIdAndBgm() {

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

    public void showInplaceHpBar(int rdfId, CharacterDownsync currCharacterDownsync, float wx, float wy, ulong lookupKey, CharacterConfig chConfig) {
        if (hideInplaceHpBars) return;

        var (inplaceHpBarAnimCtrl, oldUd) = inplaceHpBarAnimPool.GetOrCreateAnimNode(lookupKey, chConfig.SpeciesId, chConfig, underlyingMap.transform);

        newPosHolder.Set(wx, wy + chConfig.CapsuleRadius + chConfig.CapsuleHalfHeight + chConfig.InplacePromptYOffset, inplaceHpBarZ);

        inplaceHpBarAnimCtrl.gameObject.transform.position = newPosHolder;

        inplaceHpBarAnimCtrl.updateAnim(rdfId, lookupKey, currCharacterDownsync, currCharacterDownsync.ChState, chConfig, currCharacterDownsync.FramesInChState);
    }

    public bool playCharacterDamagedVfx(int rdfId, CharacterDownsync currCharacterDownsync, CharacterConfig chConfig, GameObject theGameObj, float wx, float wy, JoltCharacterAnimController chAnimCtrl, ulong lookupKey, Material material, bool isActiveBoss) {
        var spr = theGameObj.GetComponent<SpriteRenderer>();
        material.SetFloat("_CrackOpacity", 0f);

        if (!hideInplaceHpBars) {
            if (!isActiveBoss && lookupKey != Bindings.APP_CalcPlayerUserData(selfPlayerInfo.JoinIndex) && CharacterState.Dying != currCharacterDownsync.ChState && 0 < currCharacterDownsync.DamagedHintRdfCountdown) {
                showInplaceHpBar(rdfId, currCharacterDownsync, wx, wy, lookupKey, chConfig);
            }
        }

        if (null != currCharacterDownsync.DebuffList) {
            for (int i = 0; i < currCharacterDownsync.DebuffList.Count; i++) {
                Debuff debuff = currCharacterDownsync.DebuffList[i];
                if (primitives.TerminatingDebuffSpeciesId == debuff.SpeciesId) break;
                // [TODO]
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
            var (chAnimCtrl, oldUd) = playerAnimPool.GetOrCreateAnimNode(playerUd, player.Chd.SpeciesId, chConfig, underlyingMap.transform);
            chAnimCtrl.pause(toPause);
        }
        
        for (int k = 0; k < rdfHolder.NpcCount; k++) {
            var npc = rdfHolder.Npcs[k];
            if (PbPrimitivesOverride.Instance.getUnderlying().TerminatingCharacterId == npc.Id) break;
            var npcUd = APP_CalcNpcUserData(npc.Id);
            var chConfig = PbCharactersOverride.Instance.getUnderlying()[npc.Chd.SpeciesId];
            var (chAnimCtrl, oldUd) = npcAnimPool.GetOrCreateAnimNode(npcUd, npc.Chd.SpeciesId, chConfig, underlyingMap.transform);
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

    protected bool debugDrawingEnabled = false;
    public GameObject debugColliderPrefab;
    public GameObject loadDebugColliderPrefab() {
        return debugColliderPrefab;
    }

    protected JoltDebugColliderAnimPool debugColliderAnimPool = null;
}
