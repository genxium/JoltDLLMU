using jtshared;

namespace JoltCSharp {
    public class PbPrimitives {
        public static uint BladeGirlGroundSlash1Id = 1, BladeGirlGroundSlash2Id = 2, BladeGirlGroundSlash3Id = 3, BladeGirlGroundSuperSlashId = 4, BladeGirlDashingId = 5, BladeGirlDiverImpactId = 6, BladeGirlSlidingSlashId = 7, BladeGirlCrouchSlashId = 8, BladeGirlAirSlash1Id = 9, BladeGirlAirSlash2Id = 10, BladeGirlDragonPunchId = 11;

        public static uint HunterPistolWallId = 128, HunterPistolId = 129, HunterPistolAirId = 130, HunterPistolWalkingId = 131, HunterDragonPunchId = 132, HunterAirSlashId = 133, HunterDashingId = 134, HunterSlidingId = 135, HunterPistolCrouchId = 136, MobileThunderCannonPrimerId = 137, MobileThunderCannonPrimerAirId = 138, MobileThunderCannonPrimerCrouchId = 139, MobileThunderCannonPrimerWallId = 140;

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

        public const uint SPECIES_EKRAIL = 14;
        public const uint SPECIES_SWORDMAN = 15;
        public const uint SPECIES_FIRESWORDMAN = 16;
        public const uint SPECIES_WANDWITCHGIRL = 17;

        public const uint SPECIES_ANGEL = 18;
        public const uint SPECIES_GH_WIZARD_MAN_RED = 19;

        public const uint SPECIES_DEMON_FIRE_SLIME = 4096;
        public const uint SPECIES_GOBLIN = 4097;
        public const uint SPECIES_SKELEARCHER = 4098;
        public const uint SPECIES_BAT = 4099;
        public const uint SPECIES_FIREBAT = 4100;
        public const uint SPECIES_RIDLEYDRAKE = 4101;
        public const uint SPECIES_BOARWARRIOR = 4102;
        public const uint SPECIES_BOAR = 4103;
        public const uint SPECIES_SWORDMAN_BOSS = 4104;
        public const uint SPECIES_FIRETOTEM = 4105;
        public const uint SPECIES_BRICK1 = 4106;
        public const uint SPECIES_DARKBEAMTOWER = 4107;
        public const uint SPECIES_LIGHTGUARD_RED = 4108;
        public const uint SPECIES_HEAVYGUARD_RED = 4109;
        public const uint SPECIES_RIDERGUARD_RED = 4110;
        public const uint SPECIES_STONE_GOLEM = 4111;
        public const uint SPECIES_BOMBERGOBLIN = 4112;
        public const uint SPECIES_ARCHERGUARD_RED = 4113;
        public const uint SPECIES_FLYING_DEMON = 4114;
        public const uint SPECIES_SUCCUBUS = 4115;

        public const uint SPECIES_CMD_TRAINER = 4116;

        // Non-interauctive
        public const uint SPECIES_VIL_MALE1 = 8192;
        public const uint SPECIES_VIL_MALE2 = 8193;
        public const uint SPECIES_VIL_FEMALE1 = 8194;
        public const uint SPECIES_VIL_FEMALE2 = 8195;
        public const uint SPECIES_PINK_PIG = 8196;

        public const float DEFAULT_MIN_FALLING_VEL_Y = -4.5f;
        public const float DEFAULT_SLIP_JUMP_THRESHOLD_BELOW_TOP_FACE = 8.0f; // Currently only supports rectilinear rectangle shape.
        public const float DEFAULT_SLIP_JUMP_CHARACTER_DROP = (DEFAULT_SLIP_JUMP_THRESHOLD_BELOW_TOP_FACE * 1.5f);


        public const uint VISION_SEARCH_INTERVAL_IMMEDIATE_U = (1u << 0);
        public const uint VISION_SEARCH_INTERVAL_FAST_U = (1u << 2);
        public const uint VISION_SEARCH_INTERVAL_MID_U = (1u << 3);
        public const uint VISION_SEARCH_INTERVAL_SLOW_U = (1u << 4);
        public const int VISION_SEARCH_INTERVAL_IMMEDIATE = (int)VISION_SEARCH_INTERVAL_IMMEDIATE_U;
        public const int VISION_SEARCH_INTERVAL_FAST = (int)VISION_SEARCH_INTERVAL_FAST_U;
        public const int VISION_SEARCH_INTERVAL_MID = (int)VISION_SEARCH_INTERVAL_MID_U;
        public const int VISION_SEARCH_INTERVAL_SLOW = (int)VISION_SEARCH_INTERVAL_SLOW_U;

        public const int BATTLE_DYNAMICS_FPS = 60;
        public const int INPUT_SCALE_FRAMES = 2;
        public const int INPUT_SCALE = (1 << INPUT_SCALE_FRAMES);
        public const int FRONTEND_WS_RECV_BYTE_LENGTH = 8196;
        public const int JUMP_HOLDING_RDF_CNT_THRESHOLD_1 = (BATTLE_DYNAMICS_FPS >> 3) + (BATTLE_DYNAMICS_FPS >> 4);
        public const int JUMP_HOLDING_IFD_CNT_THRESHOLD_1 = (int)(1.0 * JUMP_HOLDING_RDF_CNT_THRESHOLD_1 + INPUT_SCALE - 1) / INPUT_SCALE; // Ceiled  

        public const int JUMP_HOLDING_RDF_CNT_THRESHOLD_2 = (BATTLE_DYNAMICS_FPS >> 1) + (BATTLE_DYNAMICS_FPS >> 4);
        public const int JUMP_HOLDING_IFD_CNT_THRESHOLD_2 = (int)(1.0 * JUMP_HOLDING_RDF_CNT_THRESHOLD_2 + INPUT_SCALE - 1) / INPUT_SCALE;

        public const int DEFAULT_BACKEND_INPUT_BUFFER_SIZE = ((30 * BATTLE_DYNAMICS_FPS) >> INPUT_SCALE_FRAMES) + 1;

        public static PrimitiveConsts underlying = new PrimitiveConsts {
            BattleDynamicsFps = BATTLE_DYNAMICS_FPS,
            DefaultTimeoutForLastAllConfirmedIfd = 10000, // in milliseconds

            RoomIdNone = 0,

            RoomStateImpossible = 0,
            RoomStateIdle = 1,
            RoomStateWaiting = 2,
            RoomStatePrepare = 3,
            RoomStateInBattle = 4,
            RoomStateInSettlement = 5,
            RoomStateStopped = 6,
            RoomStateFrontendAwaitingAutoRejoin = 7,
            RoomStateFrontendAwaitingManualRejoin = 8,
            RoomStateFrontendRejoining = 9,

            PlayerBattleStateImpossible = -2,
            PlayerBattleStateAddedPendingBattleColliderAck = 0,
            PlayerBattleStateReaddedPendingForceResync = 1,
            PlayerBattleStateActive = 2,
            PlayerBattleStateDisconnected = 3,
            PlayerBattleStateLost = 4,
            PlayerBattleStateExpelledDuringGame = 5,
            PlayerBattleStateExpelledInDismissal = 6,

            UpsyncMsgActPlayerColliderAck = 1,
            UpsyncMsgActPlayerCmd = 2,
            UpsyncMsgActHolepunchBackendUdpTunnel = 3,
            UpsyncMsgActHolepunchPeerUdpAddr = 4,

            TerminatingRenderFrameId = 0,
            TerminatingInputFrameId = 0,

            DownsyncMsgActBattleColliderInfo = 1,
            DownsyncMsgActInputBatch = 2,
            DownsyncMsgActBattleStopped = 3,
            DownsyncMsgActForcedResync = 4,
            DownsyncMsgActPeerInputBatch = 5,
            DownsyncMsgActPeerUdpAddr = 6,
            DownsyncMsgActBattleReadyToStart = -1,
            DownsyncMsgActBattleStart = 0,
            DownsyncMsgActPlayerDisconnected = -96,
            DownsyncMsgActPlayerReaddedAndAcked = -97,
            DownsyncMsgActPlayerAddedAndAcked = -98,
            DownsyncMsgWsClosed = -99,
            DownsyncMsgWsOpen = -100,

            MagicJoinIndexInvalid = 0xFFFFFFFF,
            MagicJoinIndexSrvUdpTunnel = 0,
            MagicQuotaInfinite = -1,

            NpcFleeGracePeriodRdfCnt = 8,

            MagicLastSentInputFrameIdNormalAdded = -1,
            MagicLastSentInputFrameIdReadded = -2,

            BgmNoChange = 0,

            MaxBtnHoldingRdfCnt = 999999999,
            MaxFlyingRdfCnt = 999999999,
            MaxReversePushbackFramesToRecover = 30,
            TerminatingTriggerEditorId = 0,
            SpeedNotHitNotSpecified = 0,
            DefaultPreallocNpcCapacity = 24, // 1 serialized "CharacterDownsync" is around 112 bytes per experiment, (7465 - 7017)/(28-24) 
            DefaultPreallocBulletCapacity = 48, // 1 serialized "Bullet" is around 18.5 bytes per experiment, (7465 - 7317)/(56 - 48)
            DefaultPreallocTrapCapacity = 12,
            DefaultPreallocTriggerCapacity = 15,
            DefaultPreallocPickableCapacity = 32,
            DefaultPerCharacterBuffCapacity = 1,
            DefaultPerCharacterDebuffCapacity = 1,
            DefaultPerCharacterInventoryCapacity = 1,
            DefaultPerCharacterImmuneBulletRecordCapacity = 3,

            GravityY = -0.52f * BATTLE_DYNAMICS_FPS * BATTLE_DYNAMICS_FPS,
            GravityYJumpHolding = -0.28f * BATTLE_DYNAMICS_FPS * BATTLE_DYNAMICS_FPS,

            DefaultPatrolCueWaivingFrames = 150, // in the count of render frames, should be big enough for any NPC to move across the largest patrol cue
            NoPatrolCueId = 0,
            NoVfxId = 0,

            DefaultPickableHitboxHalfSizeX = 10f,
            DefaultPickableHitboxHalfSizeY = 12f,
            DefaultPickableDisappearingAnimFrames = 10,
            DefaultPickableConsumedAnimFrames = 30,
            DefaultPickableRisingVelY = 8f,
            DefaultPickableNonpickableStartupFrames = 45,

            DefaultBlockStunFrames = 10,
            DefaultBlownupFramesForFlying = 30,

            DefaultGaugeIncByHit = 5,
            DefaultFramesDelayedOfBossSavepoint = 8,
            InputDelayFrames = 2, // in the count of render frames
            InputScaleFrames = INPUT_SCALE_FRAMES, // inputDelayedAndScaledFrameId = ((originalFrameId - InputDelayFrames) >> InputScaleFrames)
            MaxChasingRenderFramesPerUpdate = 8,
            MagicFramesToBeOnWall = (BATTLE_DYNAMICS_FPS >> 2),
            MagicFramesToBeOnWallAirJump = (BATTLE_DYNAMICS_FPS >> 1),

            DyingFramesToRecover = 100, // MUST BE SAME FOR EVERY CHARACTER FOR FAIRNESS!

            ParriedFramesToRecover = 25,
            ParriedFramesToStartCancellable = 6,

            NoSkill = 0,
            NoSkillHit = 0,

            NoLockVel = -65535f,
            SpeciesNoneCh = 0,
            UpsyncStIfdIdTolerance = 8,

            TerminatingCharacterId = 0,
            TerminatingTrapId = 0,
            TerminatingTriggerId = 0,
            TerminatingPickableId = 0,
            TerminatingBulletId = 0,
            TerminatingBulletTeamId = 0, // Default for proto int32 to save space
            TerminatingBuffSpeciesId = 0, // Default for proto int32 to save space in "CharacterDownsync.killedToDropBuffSpeciesId"
            TerminatingDebuffSpeciesId = 0,
            TerminatingConsumableSpeciesId = 0, // Default for proto int32 to save space in "CharacterDownsync.killedToDropConsumableSpeciesId"

            FrontendWsRecvBytelength = FRONTEND_WS_RECV_BYTE_LENGTH, // Expirically enough and not too big to have a graphic smoothness impact when receiving

            JammedBtnHoldingRdfCnt = -1,

            InAirDashGracePeriodRdfCnt = 3,
            InAirJumpGracePeriodRdfCnt = 6,

            SpAtkLookupFrames = 5,

            PatternIdUnableToOp = -2,
            PatternIdNoOp = -1,
            PatternB = 1,
            PatternUpB = 2,
            PatternDownB = 3,
            PatternHoldB = 4,
            PatternDownA = 5,
            PatternReleasedB = 6,

            PatternE = 7,
            PatternFrontE = 8,
            PatternBackE = 9,
            PatternUpE = 10,
            PatternDownE = 11,
            PatternHoldE = 12,

            PatternEHoldB = 13,
            PatternFrontEHoldB = 14,
            PatternBackEHoldB = 15,
            PatternUpEHoldB = 16,
            PatternDownEHoldB = 17,
            PatternHoldEHoldB = 18,

            PatternInventorySlotC = 1024,
            PatternInventorySlotD = 1025,
            PatternInventorySlotBc = 1026,

            PatternHoldInventorySlotC = 1027,
            PatternHoldInventorySlotD = 1028,

            EleNone = 0,
            EleFire = 1,
            EleWater = 2,
            EleThunder = 4,
            EleRock = 8,
            EleWind = 16,
            EleIce = 32,

            EleWeaknessDefaultYield = 1.5f,
            EleResistanceDefaultYield = 0.5f,

            DebuffArrIdxElemental = 0,

            EstimatedSecondsPerRdf = (1.0f / BATTLE_DYNAMICS_FPS),
            InputScale = INPUT_SCALE,
            DefaultBackendInputBufferSize = DEFAULT_BACKEND_INPUT_BUFFER_SIZE,

            DefaultFramesToShowDamaged = (int)(1.2 * BATTLE_DYNAMICS_FPS),
            DefaultFramesToContinueCombo = (int)(0.8f * BATTLE_DYNAMICS_FPS),

            BackendWsRecvBytelength = (FRONTEND_WS_RECV_BYTE_LENGTH + (FRONTEND_WS_RECV_BYTE_LENGTH >> 1)), // Slightly larger than FrontendWsRecvBytelength because it has to receive some initial collider information

            BtnBHoldingRdfCntThreshold2 = BATTLE_DYNAMICS_FPS + (BATTLE_DYNAMICS_FPS >> 1),
            BtnBHoldingRdfCntThreshold1 = (BATTLE_DYNAMICS_FPS >> 2) + (BATTLE_DYNAMICS_FPS >> 3),

            JumpHoldingRdfCntThreshold1 = JUMP_HOLDING_RDF_CNT_THRESHOLD_1,
            JumpHoldingIfdCntThreshold1 = JUMP_HOLDING_IFD_CNT_THRESHOLD_1,

            JumpHoldingRdfCntThreshold2 = JUMP_HOLDING_RDF_CNT_THRESHOLD_2,
            JumpHoldingIfdCntThreshold2 = JUMP_HOLDING_IFD_CNT_THRESHOLD_2,

            BtnEHoldingRdfCntThreshold1 = JUMP_HOLDING_RDF_CNT_THRESHOLD_2,
            BtnEHoldingIfdCntThreshold1 = JUMP_HOLDING_IFD_CNT_THRESHOLD_2,

            StartingRenderFrameId = 1,
            StartingInputFrameId = 1,
        };
    }
}
