using jtshared;

namespace JoltCSharp { 
    public class PbConsts {
        const int BATTLE_DYNAMICS_FPS = 60;
        const int INPUT_SCALE_FRAMES = 2;
        const int INPUT_SCALE = (1 << INPUT_SCALE_FRAMES);
        const int FRONTEND_WS_RECV_BYTE_LENGTH = 8196;
        const int JUMP_HOLDING_RDF_CNT_THRESHOLD_1 = (BATTLE_DYNAMICS_FPS >> 3) + (BATTLE_DYNAMICS_FPS >> 4);
        const int JUMP_HOLDING_IFD_CNT_THRESHOLD_1 = (int)(1.0 * JUMP_HOLDING_RDF_CNT_THRESHOLD_1 + INPUT_SCALE - 1) / INPUT_SCALE; // Ceiled  

        const int JUMP_HOLDING_RDF_CNT_THRESHOLD_2 = (BATTLE_DYNAMICS_FPS >> 1) + (BATTLE_DYNAMICS_FPS >> 4);
        const int JUMP_HOLDING_IFD_CNT_THRESHOLD_2 = (int)(1.0 * JUMP_HOLDING_RDF_CNT_THRESHOLD_2 + INPUT_SCALE - 1) / INPUT_SCALE;

        public PrimitiveConsts primitiveConsts = new PrimitiveConsts {
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

            TerminatingRenderFrameId = -1024,
            TerminatingInputFrameId = -1024,

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
            TerminatingEvtsubId = 0,
            MagicEvtsubIdDummy = 65535,
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

            GravityY = -9.8f, // Per second
            GravityYJumpHolding = -4.9f,

            DefaultPatrolCueWaivingFrames = 150, // in the count of render frames, should be big enough for any NPC to move across the largest patrol cue
            NoPatrolCueId = 0,
            NoVfxId = 0,

            DefaultPickableHitboxSizeX = 10f,
            DefaultPickableHitboxSizeY = 12f,
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

            MagicFramesToBeOnWall = 12,

            DyingFramesToRecover = 100, // MUST BE SAME FOR EVERY CHARACTER FOR FAIRNESS!

            ParriedFramesToRecover = 25,
            ParriedFramesToStartCancellable = 6,

            NoSkill = 0,
            NoSkillHit = 0,

            NoLockVel = -65535f,

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
            DefaultBackendInputBufferSize = ((30 * BATTLE_DYNAMICS_FPS) >> INPUT_SCALE_FRAMES) + 1,

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
        };

        public ConfigConsts configConsts = new ConfigConsts { };
    }
}
