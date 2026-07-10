using Google.Protobuf;
using JoltCSharp;
using jtshared;
using Steamworks;
using SuperTiled2Unity;
using System;
using System.Collections;
using System.Runtime.InteropServices;
using System.Threading;
using System.Threading.Tasks;
using UnityEngine;
using UnityEngine.SceneManagement;
using static JoltCSharp.Bindings;

public class SteamOnlineMapController : AbstractJoltMapController {
    public UISoundSource uiSoundSource;
    public SysBtnsHintController sysBtnsHint;

    Task p2pSessionTask;
    protected int upsyncSeqNo = 0;
    protected int autoRejoinQuota = 1;

    CancellationTokenSource p2pSessionCancellationTokenSource;
    CancellationToken p2pSessionCancellationToken;

    private const float freezeIfdLagMilliseconds = 300.0f;
    private const int freezeRdfLagThresHold = (int)(PbPrimitivesOverride.BATTLE_DYNAMICS_FPS * freezeIfdLagMilliseconds/1000.0f);
    private const int freezeIfdLagThresHold = (freezeRdfLagThresHold >> PbPrimitivesOverride.INPUT_SCALE_FRAMES);

    private const float slowDownIfdLagMilliseconds = 130.0f;
    private const int slowDownRdfLagThreshold = (int)(PbPrimitivesOverride.BATTLE_DYNAMICS_FPS * slowDownIfdLagMilliseconds/1000.0f);
    private const int slowDownIfdLagThreshold = (slowDownRdfLagThreshold >> PbPrimitivesOverride.INPUT_SCALE_FRAMES); 

    private int frozenRdfCount = 0;
    private int frozenGracingRdfCnt = 0;
    private const int frozenRdfCountLimit = (PbPrimitivesOverride.BATTLE_DYNAMICS_FPS);
    private const int frozenRdfCountDaRegularBroadcastingThreshold = (frozenRdfCountLimit * 1/2);
    private const int frozenGracePeriodRdfCount = (int)(1.5f*PbPrimitivesOverride.BATTLE_DYNAMICS_FPS);

    private const int acIfdLagThresHold = (PbPrimitivesOverride.FRONTEND_WS_RECV_BYTE_LENGTH / 5);
    bool acLagShouldLockStep = false;

    public bool useFreezingLockStep = true; // [WARNING] If set to "false", expect more teleports due to "FRONTEND_ChaseRolledBackRdfs" but less frozen graphics when your device has above average network among all peers in the same battle -- yet "useFreezingLockStep" could NOT completely rule out teleports as long as potential floating point mismatch between devices exists (especially between backend .NET 7.0 and frontend .NET 2.1).
    public SteamNetworkDoctorInfo networkInfoPanel;
    bool ifdFrontShouldLockStep = false;
    bool ifdFrontShouldFreeze = false;
    bool localTimerEnded = false;
    bool timerEndedRdfDerivedFromAllConfirmedInputFrameDownsync = false;

    public const int DEFAULT_TIMEOUT_FOR_LAST_ALL_CONFIRMED_IFD = 10000; // in milliseconds;
    int timeoutMillisAwaitingLastAllConfirmedInputFrameDownsync = DEFAULT_TIMEOUT_FOR_LAST_ALL_CONFIRMED_IFD;

    public SteamPlayerWaitingPanel playerWaitingPanel;
    public SteamArenaCharacterSelectPanel characterSelectPanel;

    public ArenaModeSettings arenaModeSettings;

    public SteamRejoinPrompt rejoinPrompt;

    protected DownsyncSnapshot downsyncSnapshotHolder = new DownsyncSnapshot();

    public ReadyGo readyGoPanel;
    public SettlementPanel settlementPanel;

    protected byte[] upsyncSnapshotReqBuffer;
    protected long upsyncSnapshotReqBufferBytesCnt = 0;
    private int lastSentIfdId = -1, newLcacIfdId = -1, newUdpLcacIfdId = -1, maxPlayerInputFrontId = 0, minPlayerInputFrontId = 0;

    public bool stepShadowBattleAlongWithMainBattle = false;
    public bool broadcastShadowBattleDownsyncSnapshotAlongWithMainBattle = false;
    private byte[] shadowBattleDownsyncSnapshotBytes;
    long shadowBattleDownsyncSnapshotByteCnt = 0;
    private int shadowBattleEvictedStCnt = 0, shadowBattleOldLcacIfdId = -1, shadowBattleNewLcacIfdId = -1, shadowBattleOldDynamicsRdfId = 0, shadowBattleNewDynamicsRdfId = 0, shadowBattleMaxPlayerInputFrontId = 0, shadowBattleMinPlayerInputFrontId = 0;

    private int lastSentRefRdfIdAsOwner = -1, lastSentRefRdfAtTimerRdfIdAsOwner = -1;
    private int REF_RDF_ID_INTERVAL_RDF_CNT = 5*PbPrimitivesOverride.BATTLE_DYNAMICS_FPS;

    private ulong allConfirmedMask = 0UL;

    private UIntPtr shadowBattle = UIntPtr.Zero; // No rollback, only used for "broadcastDaRegularAsOwner(...)"
    public SteamOnlineMapController() : base() {
        upsyncSnapshotReqBuffer = new byte[pbBufferSizeLimit];
        shadowBattleDownsyncSnapshotBytes = new byte[pbBufferSizeLimit];
    }

    public void OnLobbyCreated() {
        
    }

    public void OnLobbyCreateFailed() {
        characterSelectPanel.ToggleUIInteractability(true);
        characterSelectPanel.TogglePlayerInput(true);
    }

    public void OnLobbyEntered() {
        playerWaitingPanel.ToggleUIInteractability(true);
        playerWaitingPanel.gameObject.SetActive(true);
        battleState = PbPrimitivesOverride.ROOM_STATE_WAITING;

        bool startedp2pSessionTask = startP2PSessionTaskIfNotYet();
        if (startedp2pSessionTask) {
            Debug.Log($"Started new p2pSessionTask in readyGoPanel.OnComplete");
        }
    }

    public void OnLobbyEnterFailed() {
        characterSelectPanel.ToggleUIInteractability(true);
        characterSelectPanel.TogglePlayerInput(true);
    }

    public void OnLobbyMembersUpdated(SteamBinding[] newMemberList) {
        Debug.Log($"Lobby members updated:\n");
        for (int i = 0; i < newMemberList.Length; i++) {
            var member = newMemberList[i];
            if (CSteamID.Nil.m_SteamID == member.UlSteamId || PbPrimitivesOverride.SPECIES_NONE_CH == member.ChSpeciesId) {
                break;
            }
            Debug.Log($"joinIndex={i+1}: memberSteamID={member.UlSteamId}, chSpeciesId={member.ChSpeciesId}");
        }

        if (null != playerWaitingPanel && playerWaitingPanel.gameObject.activeSelf) {
            playerWaitingPanel.OnParticipantChange(newMemberList);
        }
    }

    public void OnSelfKickedOutOfLobby() {
        if (null != playerWaitingPanel && playerWaitingPanel.gameObject.activeSelf) {
            playerWaitingPanel.OnCancel(null);
            OnWaitingInterrupted(); 
        }
    }

    public void OnLobbyMemberLeft(in uint joinIndex, in string motivation) {
        if (joinIndex == selfJoinIndex) {
            Debug.Log($"{motivation}: OnLobbyMemberLeft with selfJoinIndex={joinIndex}, battleState={battleState}, thread ud={Thread.CurrentThread.ManagedThreadId}.");
            if (PbPrimitivesOverride.ROOM_STATE_IN_BATTLE == battleState) {
                battleState = PbPrimitivesOverride.ROOM_STATE_FRONTEND_AWAITING_MANUAL_REJOIN;
            }
        } else {
            Debug.Log($"{motivation}: OnLobbyMemberLeft with peer joinIndex={joinIndex}, battleState={battleState}, thread ud={Thread.CurrentThread.ManagedThreadId}.");
            if (PbPrimitivesOverride.ROOM_STATE_IN_BATTLE == battleState) {
                int joinIndexArrIdx = ((int)joinIndex) - 1;
                ulong joinIndexMask = (1UL << joinIndexArrIdx);
                {
                    ulong existingInactiveJoinIndexMask = APP_GetInactiveJoinMask(battle);
                    APP_SetInactiveJoinMask(battle, (existingInactiveJoinIndexMask | joinIndexMask));
                }
                {
                    ulong existingInactiveJoinIndexMask = APP_GetInactiveJoinMask(shadowBattle);
                    APP_SetInactiveJoinMask(shadowBattle, (existingInactiveJoinIndexMask | joinIndexMask));
                }
            }
        }
    }

    public void OnLobbyMemberRejoined(uint joinIndex) {
        Debug.Log($"OnLobbyMemberRejoined with joinIndex={joinIndex}, battleState={battleState}, thread ud={Thread.CurrentThread.ManagedThreadId}.");

        int joinIndexArrIdx = ((int)joinIndex) - 1;
        ulong joinIndexMask = (1UL << joinIndexArrIdx);
        {
            ulong existingInactiveJoinIndexMask = APP_GetInactiveJoinMask(battle);
            if (0 < (existingInactiveJoinIndexMask & joinIndexMask)) {
                APP_SetInactiveJoinMask(battle, (existingInactiveJoinIndexMask ^ joinIndexMask));
            }
        }
        {
            ulong existingInactiveJoinIndexMask = APP_GetInactiveJoinMask(shadowBattle);
            if (0 < (existingInactiveJoinIndexMask & joinIndexMask)) {
                APP_SetInactiveJoinMask(shadowBattle, (existingInactiveJoinIndexMask ^ joinIndexMask));
            }
        }

        if (SteamP2PSessionManager.Instance.GetIsCurrentLobbyOwner()) {
            forceConfirmAllPeersAndBroadcastDaRegularAsOwner($"LobbyMember joinIndex={joinIndex} rejoined"); 
        }
    }

    private byte[] downsyncSnapshotRecvBuff = new byte[PbPrimitivesOverride.Instance.getUnderlying().FrontendWsRecvBytelength];

    private unsafe void handleSingleDownsyncSnapshotBytes(in IntPtr pData, in int bytesCnt) {
        Marshal.Copy(pData, downsyncSnapshotRecvBuff, 0, bytesCnt);

        Bindings.PreemptDownsyncSnapshotBeforeMerge(downsyncSnapshotHolder, PbPrimitivesOverride.Instance.getUnderlying());
        downsyncSnapshotHolder.MergeFrom(downsyncSnapshotRecvBuff, 0, bytesCnt); // [TODO] Reduce redundant parsing in C# by fetching parsed results from C++ interface

        switch (downsyncSnapshotHolder.Act) {
            case DownsyncAct.DaBattlePrepare:
                Debug.Log($"@csharpTimerRdfId={csharpTimerRdfId} with battleState={battleState}, handling DaBattlePrepare in main thread");
                prepareForBattle(downsyncSnapshotHolder);
                break;
            case DownsyncAct.DaBattleStopped:
                Debug.LogWarning($"@csharpTimerRdfId={csharpTimerRdfId} with battleState={battleState}, handling DaBattleStopped in main thread");
                onBattleStopped();
                StartCoroutine(delayToShowSettlementPanel());
                break;
            case DownsyncAct.DaRegular:
                // [REMINDER] When the "Lobby owner" handles a "DaRegular" (sent from itself) by "FRONTEND_OnDownsyncSnapshotReceived(...)", the fields "lcacIfdId" and "udpLcacIfdId" will be incremented and thus break "lag-induced-freezing".   
                int postTimerRdfEvictedCnt = 0, postTimerRdfDelayedIfdEvictedCnt = 0;
                fixed (int* newChaserRdfIdPtr = &newChaserRdfId, newLcacIfdIdPtr = &newLcacIfdId, newUdpLcacIfdIdPtr = &newUdpLcacIfdId, maxPlayerInputFrontIdPtr = &maxPlayerInputFrontId, minPlayerInputFrontIdPtr = &minPlayerInputFrontId) {
                    Bindings.FRONTEND_OnDownsyncSnapshotReceived(battle, (char*)pData, bytesCnt, &postTimerRdfEvictedCnt, &postTimerRdfDelayedIfdEvictedCnt, newChaserRdfIdPtr, newLcacIfdIdPtr, newUdpLcacIfdIdPtr, maxPlayerInputFrontIdPtr, minPlayerInputFrontIdPtr);

                    if (0 < postTimerRdfEvictedCnt) {
                        SteamNetworkDoctor.Instance.LogForceResyncImmediatePump();
                    }
                }

                var ifdBatch = downsyncSnapshotHolder.IfdBatch;
                if (null != ifdBatch && 0 < ifdBatch.Count) {
                    var firstInBatch = ifdBatch[0];
                    var lastInBatch = ifdBatch[ifdBatch.Count - 1];
                    SteamNetworkDoctor.Instance.LogInputFrameDownsync(downsyncSnapshotHolder.StIfdId, downsyncSnapshotHolder.StIfdId + ifdBatch.Count - 1);
                }

                if (null != downsyncSnapshotHolder.RefRdf && PbPrimitivesOverride.Instance.getUnderlying().TerminatingRenderFrameId != downsyncSnapshotHolder.RefRdfId) {
                    readyGoPanel.hideReady();
                    readyGoPanel.hideGo();
                    if (PbPrimitivesOverride.ROOM_STATE_FRONTEND_REJOINING == battleState
                        || PbPrimitivesOverride.ROOM_STATE_FRONTEND_AWAITING_AUTO_REJOIN == battleState
                        || PbPrimitivesOverride.ROOM_STATE_FRONTEND_AWAITING_MANUAL_REJOIN == battleState) {
                        Debug.LogWarning($"Potentially rejoining battleState={battleState}, about to cancel rejoinPrompt");
                        rejoinPrompt.OnCancel(null);
                    }
                    battleState = PbPrimitivesOverride.ROOM_STATE_IN_BATTLE;
                    enableBattleInput(true);
                }
                break;
        }
    }

    bool startP2PSessionTaskIfNotYet() {
        if (null != p2pSessionTask) return false;
        p2pSessionCancellationTokenSource = new CancellationTokenSource();
        p2pSessionCancellationToken = p2pSessionCancellationTokenSource.Token;
        p2pSessionTask = Task.Run(async () => {
            await SteamP2PSessionManager.Instance.OpenSession(p2pSessionCancellationToken);
        });
        return true;
    }

    private const int MAX_MSG_COUNT_PER_POLL = 8;
    private static IntPtr[] messagePointers = new IntPtr[MAX_MSG_COUNT_PER_POLL]; // [WARNING] Using "static" intentionally such that "SteamNetworkingMessages.ReceiveMessagesOnChannel(0, messagePointers, MAX_MSG_COUNT_PER_POLL)" would NOT crash due to "messagePointers" being deallocated by another thread (e.g. when application quits).
    private byte[] localRecvBytes = new byte[PbPrimitivesOverride.Instance.getUnderlying().FrontendWsRecvBytelength];

    private unsafe void pollAndHandleFromOwnerRecvBuffer(int* pTimerRdfId, int* pNewChaserRdfId, int* pChaserRdfIdLowerBound, int* pToGenIfdId, int* pLocalRequiredIfdId) {
        int messageCount = 0;
        do {
            // [REMINDER] Mostly native-method wrappers, no need to use try-catch-finally or try-finally
            messageCount = SteamNetworkingMessages.ReceiveMessagesOnChannel(SteamP2PSessionManager.DOWNSYNC_SNAPSHOT_IO_CHANNEL, messagePointers, MAX_MSG_COUNT_PER_POLL); // [WARNING] This is non-blocking, see "SteamNetworkingMessage" official documentation for details; yet this invocation WILL IMPLICITLY ALLOCATE MEMORY AT each "messagePointers[i]".
            /*
            if (0 < messageCount) {
                Debug.Log($"@csharpTimerRdfId={csharpTimerRdfId} with battleState={battleState}, handling {messageCount} DOWNSYNC_SNAPSHOT_IO_CHANNEL messsages in main thread");
            }
            */
            for (int i = 0; i < messageCount; i++) {
                SteamNetworkingMessage_t netMessage = Marshal.PtrToStructure<SteamNetworkingMessage_t>(messagePointers[i]);
                CSteamID fromSteamID = netMessage.m_identityPeer.GetSteamID();
                if (fromSteamID == SteamP2PSessionManager.Instance.GetCurrentLobbyOwnerId()) {
                    handleSingleDownsyncSnapshotBytes(netMessage.m_pData, netMessage.m_cbSize);
                } // else check why it's coming here
                SteamNetworkingMessage_t.Release(messagePointers[i]);
                messagePointers[i] = IntPtr.Zero;
            }

        } while (messageCount >= MAX_MSG_COUNT_PER_POLL);

        while (SteamP2PSessionManager.Instance.localDownsyncSnapshotBytesBuffer.TryDequeue(out localRecvBytes)) {
            fixed (byte* pData = localRecvBytes) {
                handleSingleDownsyncSnapshotBytes((IntPtr)pData, localRecvBytes.Length);
            }
        }

        int oldLcacIfdId = -1, oldUdpLcacIfdId = -1;
        bool ok1 = Bindings.FRONTEND_GetRdfAndIfdIds(battle, pTimerRdfId, pNewChaserRdfId, pChaserRdfIdLowerBound, &oldLcacIfdId, &oldUdpLcacIfdId, pToGenIfdId, pLocalRequiredIfdId);

        if (ok1 && 0 < messageCount && !SteamP2PSessionManager.Instance.GetIsCurrentLobbyOwner()) {
            Debug.Log($"pollAndHandleFromOwnerRecvBuffer, @csharpTimerRdfId={csharpTimerRdfId}, timerRdfId={*pTimerRdfId}, newChaserRdfId={*pNewChaserRdfId}, chaserRdfIdLowerBound={*pChaserRdfIdLowerBound}, toGenIfdId={*pToGenIfdId}, localRequiredIfdId={*pLocalRequiredIfdId}");
        }
    }

    private unsafe void handleSingleUpsyncSnapshotBytes(in IntPtr pData, in int bytesCnt) {
        fixed (int* newChaserRdfIdPtr = &newChaserRdfId, newUdpLcacIfdIdPtr = &newUdpLcacIfdId, maxPlayerInputFrontIdPtr = &maxPlayerInputFrontId, minPlayerInputFrontIdPtr = &minPlayerInputFrontId) {
            Bindings.FRONTEND_OnUpsyncSnapshotReqReceived(battle, (char*)pData, bytesCnt, newChaserRdfIdPtr, newUdpLcacIfdIdPtr, maxPlayerInputFrontIdPtr, minPlayerInputFrontIdPtr);
        }

        shadowBattleDownsyncSnapshotByteCnt = pbBufferSizeLimit;
        fixed (byte* shadowBattleDownsyncSnapshotBytesPtr = shadowBattleDownsyncSnapshotBytes)
        fixed (long* shadowBattleDownsyncSnapshotByteCntPtr = &shadowBattleDownsyncSnapshotByteCnt) 
        fixed (int* shadowBattleOldLcacIfdIdPtr = &shadowBattleOldLcacIfdId, shadowBattleNewLcacIfdIdPtr = &shadowBattleNewLcacIfdId, shadowBattleOldDynamicsRdfIdPtr = &shadowBattleOldDynamicsRdfId, shadowBattleNewDynamicsRdfIdPtr = &shadowBattleNewDynamicsRdfId, shadowBattleEvictedStCntPtr = &shadowBattleEvictedStCnt, shadowBattleMaxPlayerInputFrontIdPtr = &shadowBattleMaxPlayerInputFrontId, shadowBattleMinPlayerInputFrontIdPtr = &shadowBattleMinPlayerInputFrontId) {
            Bindings.BACKEND_OnUpsyncSnapshotReqReceived(shadowBattle, (char*)pData, bytesCnt, true, false, (char*)shadowBattleDownsyncSnapshotBytesPtr, shadowBattleDownsyncSnapshotByteCntPtr, shadowBattleEvictedStCntPtr, shadowBattleOldLcacIfdIdPtr, shadowBattleNewLcacIfdIdPtr, shadowBattleOldDynamicsRdfIdPtr, shadowBattleNewDynamicsRdfIdPtr, shadowBattleMaxPlayerInputFrontIdPtr, shadowBattleMinPlayerInputFrontIdPtr);
            if (SteamP2PSessionManager.Instance.GetIsCurrentLobbyOwner() && broadcastShadowBattleDownsyncSnapshotAlongWithMainBattle) {
                if (0 < *shadowBattleDownsyncSnapshotByteCntPtr) {
                    if (SteamP2PSessionManager.Instance.GetIsCurrentLobbyOwner()) {
                        byte[] copiedBytes = new byte[(int)(*shadowBattleDownsyncSnapshotByteCntPtr)];
                        Buffer.BlockCopy(shadowBattleDownsyncSnapshotBytes, 0, copiedBytes, 0, copiedBytes.Length);
                        broadcastDaRegularAsOwner(copiedBytes, shadowBattleNewDynamicsRdfId);
                    }
                }
            }
        }
    }

    private unsafe void pollAndHandleUdpRecvBuffer(in int oldUdpLcacIfdId) {
        int messageCount = 0;
        int realtimeUdpLcacIfdId = oldUdpLcacIfdId;
        do {
            // [REMINDER] All native-method wrappers, no need to use try-catch-finally or try-finally
            messageCount = SteamNetworkingMessages.ReceiveMessagesOnChannel(SteamP2PSessionManager.UPSYNC_SNAPSHOT_IO_CHANNEL, messagePointers, MAX_MSG_COUNT_PER_POLL); // [WARNING] This is non-blocking, see "SteamNetworkingMessage" official documentation for details; yet this invocation WILL IMPLICITLY ALLOCATE MEMORY AT each "messagePointers[i]".
            /*
            if (0 < messageCount) {
                Debug.Log($"@csharpTimerRdfId={csharpTimerRdfId} with battleState={battleState}, handling {messageCount} UPSYNC_SNAPSHOT_IO_CHANNEL messsages in main thread");
            }
            */
            for (int i = 0; i < messageCount; i++) {
                SteamNetworkingMessage_t netMessage = Marshal.PtrToStructure<SteamNetworkingMessage_t>(messagePointers[i]);
                ulong fromUllSteamID = netMessage.m_identityPeer.GetSteamID64();
                uint fromJoinIndex = SteamP2PSessionManager.Instance.GetJoinIndexInLobby(fromUllSteamID);
                if (PbPrimitivesOverride.Instance.getUnderlying().MagicJoinIndexInvalid != fromJoinIndex) {
                    SteamP2PSessionManager.Instance.RemoveDisconnectedRecord(fromJoinIndex);
                    
                    handleSingleUpsyncSnapshotBytes(netMessage.m_pData, netMessage.m_cbSize);
                }
                SteamNetworkDoctor.Instance.LogUpsyncSnapshot(i: realtimeUdpLcacIfdId, j: newUdpLcacIfdId);
                SteamNetworkingMessage_t.Release(messagePointers[i]);
                messagePointers[i] = IntPtr.Zero;
                realtimeUdpLcacIfdId = newUdpLcacIfdId;
            }
        } while (messageCount >= MAX_MSG_COUNT_PER_POLL);

        while (SteamP2PSessionManager.Instance.localUpsyncSnapshotBytesBuffer.TryDequeue(out localRecvBytes)) {
            fixed (byte* pData = localRecvBytes) {
                handleSingleUpsyncSnapshotBytes((IntPtr)pData, localRecvBytes.Length);
            }
        }
    }

    public void OnWaitingInterrupted() {
        Debug.Log("SteamOnlineMapController.onWaitingInterrupted, calling cleanupNetworkSessionsReentrantSafe");
        cleanupNetworkSessionsReentrantSafe(); // Make sure that all resources are properly deallocated        
        Debug.Log($"Replaying from character-select due to onWaitingInterrupted at battleState={battleState}.");
        battleState = PbPrimitivesOverride.ROOM_STATE_STOPPED;
        showCharacterSelection();
    }


    public void onCharacterSelectGoAction(uint speciesId) {
        Debug.Log($"Executing SteamOnlineMapController.onCharacterSelectGoAction with selectedSpeciesId={speciesId}, battleState={battleState}");
        
        // [WARNING] Deliberately NOT declaring this method as "async" to make tests related to `<proj-root>/GOROUTINE_TO_ASYNC_TASK.md` more meaningful.
        battleState = PbPrimitivesOverride.ROOM_STATE_IDLE;

        JoltWsSessionManager.Instance.SetSpeciesId(speciesId);
        SteamP2PSessionManager.Instance.FindAvailableLobbies();
    }

    protected unsafe override void Start() {
        base.Start();
        roomCapacity = 2; // [TODO] Don't hardcode!
        allConfirmedMask = ((1UL << roomCapacity) - 1); 
        playerWaitingPanel.InitPlayerSlots(this, roomCapacity);
        SteamP2PSessionManager.Instance.ResetP2PSession(this, "SteamOnlineMapController init");
        selfPlayerInfo = new PlayerMetaInfo();
        
        enableBattleInput(false);

        arenaModeSettings.SetCallbacks(uiSoundSource, thePostCancelledCb: () => {
            
        }, theExitCb: () => {
            Debug.LogWarning("Calling onBattleStopped with Settings > Exit");
            OnBattleStopped("Exit from settings"); // [WARNING] Deliberately NOT calling "pauseAllAnimatingCharacters(false)" such that "iptmgr.gameObject" remains inactive, unblocking the keyboard control to "characterSelectPanel"! 
            showCharacterSelection();
        });

        int rdfBufferSize = 60*PbPrimitivesOverride.BATTLE_DYNAMICS_FPS;
        battle = Bindings.FRONTEND_CreateBattle(rdfBufferSize, true);
        shadowBattle = Bindings.BACKEND_CreateBattle(rdfBufferSize);

        characterSelectPanel.SetCallbacks(this, uiSoundSource, () => {
            SceneManager.LoadScene("SteamLoginScene", LoadSceneMode.Single);
        });
    }

    unsafe void prepareForBattle(DownsyncSnapshot prepareSignal) {
        upsyncSeqNo = 0;
        if (SteamP2PSessionManager.Instance.GetIsCurrentLobbyOwner()) {
            selfJoinIndex = 1;
        } else {
            selfJoinIndex = prepareSignal.PeerJoinIndex;
            SteamP2PSessionManager.Instance.SetLockedLobbyMemberBindings(prepareSignal.PeerSteamBindingList);
        }

        selfJoinIndexInt = (int)selfJoinIndex;
        selfJoinIndexArrIdx = selfJoinIndexInt - 1;
        selfJoinIndexMask = (1UL << selfJoinIndexArrIdx);
        selfPlayerId = $"steam_{SteamUser.GetSteamID()}";
        selfPlayerInfo = new PlayerMetaInfo {
            PlayerId = selfPlayerId,
            JoinIndex = selfJoinIndex,
            BulletTeamId = downsyncSnapshotHolder.PeerBulletTeamId
        };

        var speciesIdList = new uint[roomCapacity];
        for (int i = 0; i < roomCapacity; i++) {
            speciesIdList[i] = prepareSignal.PeerSteamBindingList[i].ChSpeciesId;
        }
        frameLogEnabled = downsyncSnapshotHolder.PrepareInfo.FrameLogEnabled;
        Bindings.APP_SetFrameLogEnabled(battle, frameLogEnabled);
        resetCurrentMatch(downsyncSnapshotHolder.PrepareInfo.StageName);
        preallocateFrontendOnlyHolders();
        calcCameraCaps();
        cachedWsReqForStartRdf = createSelfParsedStartRdf(speciesIdList);

        int colorSwapRuleOrder = 1;
        foreach (var player in cachedWsReqForStartRdf.SelfParsedRdf.Players) {
            if (player.JoinIndex == selfJoinIndex) {
                selfPlayerInfo.BulletTeamId = player.Chd.BulletTeamId;
            }
            var playerUd = APP_CalcPlayerUserData(player.JoinIndex);
            var speciesId = player.Chd.SpeciesId;
            if (characterUdToColorSwapRuleLock.ContainsKey(playerUd)) {
                colorSwapRuleOrder = characterUdToColorSwapRuleLock[playerUd];
            } else if (playerSpeciesIdOccurrenceCnt.ContainsKey(speciesId)) {
                colorSwapRuleOrder += playerSpeciesIdOccurrenceCnt[speciesId];
            }

            characterUdToColorSwapRuleLock[playerUd] = colorSwapRuleOrder;
            if (playerSpeciesIdOccurrenceCnt.ContainsKey(speciesId)) {
                playerSpeciesIdOccurrenceCnt[speciesId] += 1;
            } else {
                playerSpeciesIdOccurrenceCnt[speciesId] = 1;
            }
        }

        cachedWsReqForStartRdf.Act = UpsyncAct.UaSelfParsedRdf;
        cachedWsReqForStartRdf.BattleDurationSeconds = battleDurationSeconds;
        cachedWsReqForStartRdf.JoinIndex = selfJoinIndex;

        preallocateSfxNodes();
        applyRdf(cachedWsReqForStartRdf.SelfParsedRdf, null, 0);
        if (null != networkInfoPanel) {
            networkInfoPanel.gameObject.SetActive(true);
        }
        playerWaitingPanel.gameObject.SetActive(false);
        characterSelectPanel.gameObject.SetActive(false);

        battleState = PbPrimitivesOverride.ROOM_STATE_PREPARE;

        // Debug.Log($"[DaBattlePrepare] with battleState={battleState}, teleport camera to selfPlayer");
        cameraTrack(cachedWsReqForStartRdf.SelfParsedRdf, null, false, true);

        byte[] buffer = cachedWsReqForStartRdf.ToByteArray();
        fixed (byte* bufferPtr = buffer) {
            FRONTEND_ResetStartRdf(battle, (char*)bufferPtr, buffer.Length, selfJoinIndex, selfPlayerId, selfCmdAuthKey);
            BACKEND_ResetStartRdf(shadowBattle, (char*)bufferPtr, buffer.Length);

            shadowBattleEvictedStCnt = 0;
            shadowBattleOldLcacIfdId = -1;
            shadowBattleNewLcacIfdId = -1;
            shadowBattleOldDynamicsRdfId = 0;
            shadowBattleNewDynamicsRdfId = 0;
            shadowBattleMaxPlayerInputFrontId = 0;
            shadowBattleMinPlayerInputFrontId = 0;

            newChaserRdfId = 0;

            int preparedStRdfId = 0, preparedEdRdfId = 0;
            Bindings.APP_GetRdfBufferBounds(battle, &preparedStRdfId, &preparedEdRdfId);
            Debug.Log($"[DaBattlePrepare] Now preparedStRdfId={preparedStRdfId}, preparedEdRdfId={preparedEdRdfId}");
        }

        rejoinPrompt.SetCallbacks(this, uiSoundSource, () => {

        }, sysBtnsHint);

        readyGoPanel.playReadyAnim(() => { }, () => {
            int startedStRdfId = 0, startedEdRdfId = 0;
            Bindings.APP_GetRdfBufferBounds(battle, &startedStRdfId, &startedEdRdfId);
            Debug.Log($"About to start battle, now startedStRdfId={startedStRdfId}, startedEdRdfId={startedEdRdfId}");
            readyGoPanel.playGoAnim();
            battleState = PbPrimitivesOverride.ROOM_STATE_IN_BATTLE;
            bgmSource.Play();
            enableBattleInput(true);
            frozenRdfCount = 0;
            frozenGracingRdfCnt = 0;
            Debug.Log($"Battle started by local ready-go timer!");
            /*
             [WARNING] Peers having big network-delay with the "Lobby owner" and/or "slow CPU" might start the battle significantly slower than others (i.e. more unfair compared with "OnlineJoltMapController" using "jtbattlesrv").
             */
        });
    }

    public long GetBattleState() {
        return battleState;
    }

    public unsafe void AttemptToRejoinBattle() {
        battleState = PbPrimitivesOverride.ROOM_STATE_FRONTEND_REJOINING;
        if (newLcacIfdId > lastSentIfdId) {
            lastSentIfdId = newLcacIfdId;
        }
        upsyncSnapshotReqBufferBytesCnt = pbBufferSizeLimit;
        fixed (byte* upsyncSnapshotPtr = upsyncSnapshotReqBuffer)
        fixed (long* outBytesCntPtr = &upsyncSnapshotReqBufferBytesCnt) {
            int proposedBatchIfdIdSt = lastSentIfdId;
            int proposedBatchIfdIdEd = lastSentIfdId + 1;
            int actualLastIfdInBatch = -1;
            FRONTEND_ProduceUpsyncSnapshotRequest(battle, upsyncSeqNo++, proposedBatchIfdIdSt, proposedBatchIfdIdEd, &actualLastIfdInBatch, (char*)upsyncSnapshotPtr, outBytesCntPtr);
            if (0 < upsyncSnapshotReqBufferBytesCnt) {
                byte[] copiedBytes = new byte[(int)upsyncSnapshotReqBufferBytesCnt];
                Buffer.BlockCopy(upsyncSnapshotReqBuffer, 0, copiedBytes, 0, copiedBytes.Length);
                SteamP2PSessionManager.Instance.senderBuffer.Add(copiedBytes);
                Debug.Log($"Attempt to rejoin battle with upsyncSnapshotReqBufferBytesCnt={upsyncSnapshotReqBufferBytesCnt}.");
            } else {
                Debug.Log($"Failed to attempt a rejoin with upsyncSnapshotReqBufferBytesCnt=0.");
            }
        }
    }

    public void OnRejoinFailed(in long expectedOldBattleState) {
        long origBattleBattle = Interlocked.CompareExchange(ref battleState, PbPrimitivesOverride.ROOM_STATE_FRONTEND_AWAITING_MANUAL_REJOIN, expectedOldBattleState); 
        if (expectedOldBattleState == origBattleBattle) {
            Debug.Log($"rejoinTimer ticked with battleState={origBattleBattle}, about to re-enable SteamRejoinPrompt UI");
        }
    }

    public unsafe void OnBattleStopped(in string motivation) {
        Debug.LogWarning($"Beginning SteamSteamOnlineMapController.OnBattleStopped: {motivation}, thread ud={Thread.CurrentThread.ManagedThreadId}.");
        // Reference https://docs.unity3d.com/ScriptReference/Application-persistentDataPath.html
        if (frameLogEnabled && UIntPtr.Zero != battle) {
            try {
                int stRdfId = 0, edRdfId = 0;
                Bindings.APP_GetRdfBufferBounds(battle, &stRdfId, &edRdfId);
                if (0 < edRdfId) {
                    FrameLogPrinter.wrapUpFrameLogs(battle, stRdfId, edRdfId, false, Application.persistentDataPath, $"p{selfJoinIndex}-jolt-{DateTime.Now.ToString("yyyy_MM_ddTHH_mm_ss")}.log");
                    Debug.Log($"Frame log written with stRdfId={stRdfId}, edRdfId={edRdfId}");
                }
            } catch (Exception e) {
                Debug.LogError($"Error occurred when wrapUpFrameLogs with battle={battle:X}");
                Debug.LogException(e);
            }   
        }
        
        autoRejoinQuota = 1;
        cleanupNetworkSessionsReentrantSafe(); // Make sure that all resources are properly deallocated
        base.onBattleStopped();
        if (UIntPtr.Zero != shadowBattle) {
            APP_ClearBattle(shadowBattle);
            shadowBattle = UIntPtr.Zero;
        }
        if (null != readyGoPanel) {
            readyGoPanel.resetCountdown();
        } 
        if (null != bgmSource) {
            bgmSource.Stop();
        }
        Debug.LogWarning($"Ending SteamSteamOnlineMapController.onBattleStopped: thread ud={Thread.CurrentThread.ManagedThreadId}.");
    }

    private unsafe bool forceConfirmAllPeersAndBroadcastDaRegularAsOwner(in string motivation) {
        shadowBattleDownsyncSnapshotByteCnt = pbBufferSizeLimit;
        fixed (byte* shadowBattleDownsyncSnapshotBytesPtr = shadowBattleDownsyncSnapshotBytes)
        fixed (long* shadowBattleDownsyncSnapshotByteCntPtr = &shadowBattleDownsyncSnapshotByteCnt)
        fixed (int* shadowBattleOldLcacIfdIdPtr = &shadowBattleOldLcacIfdId, shadowBattleNewLcacIfdIdPtr = &shadowBattleNewLcacIfdId, shadowBattleOldDynamicsRdfIdPtr = &shadowBattleOldDynamicsRdfId, shadowBattleNewDynamicsRdfIdPtr = &shadowBattleNewDynamicsRdfId, shadowBattleEvictedStCntPtr = &shadowBattleEvictedStCnt, shadowBattleMaxPlayerInputFrontIdPtr = &shadowBattleMaxPlayerInputFrontId, shadowBattleMinPlayerInputFrontIdPtr = &shadowBattleMinPlayerInputFrontId) {
            ulong existingInactiveJoinIndexMask = APP_GetInactiveJoinMask(shadowBattle);
            ulong inactiveJoinIndexMaskToForceConfirm = (allConfirmedMask ^ selfJoinIndexMask);
            APP_SetInactiveJoinMask(shadowBattle, inactiveJoinIndexMaskToForceConfirm); // [REMINDER] Even when all other peers are skippable, the progress of force-confirmation is still limited by local "toGenIfdId"
            BACKEND_MoveForwardLcacIfdIdAndStep(shadowBattle, withRefRdf: true, shadowBattleOldLcacIfdIdPtr, shadowBattleNewLcacIfdIdPtr, shadowBattleOldDynamicsRdfIdPtr, shadowBattleNewDynamicsRdfIdPtr, (char*)shadowBattleDownsyncSnapshotBytesPtr, shadowBattleDownsyncSnapshotByteCntPtr);
            APP_SetInactiveJoinMask(shadowBattle, existingInactiveJoinIndexMask); // recover
            if (0 < *shadowBattleDownsyncSnapshotByteCntPtr && shadowBattleOldDynamicsRdfId < shadowBattleNewDynamicsRdfId) {
                byte[] copiedBytes = new byte[(int)(*shadowBattleDownsyncSnapshotByteCntPtr)];
                Buffer.BlockCopy(shadowBattleDownsyncSnapshotBytes, 0, copiedBytes, 0, copiedBytes.Length);

                Debug.Log($"@csharpTimerRdfId={csharpTimerRdfId}, about to broadcast DaRegular from owner [lastSentIfdId={lastSentIfdId}, newLcacIfdId={newLcacIfdId}, shadowBattleNewLcacIfdId={shadowBattleNewLcacIfdId}, newUdpLcacIfdId={newUdpLcacIfdId}, shadowBattleOldDynamicsRdfId={shadowBattleOldDynamicsRdfId}, shadowBattleNewDynamicsRdfId={shadowBattleNewDynamicsRdfId}]: {motivation}");

                broadcastDaRegularAsOwner(copiedBytes, shadowBattleNewDynamicsRdfId);
                return true;
            } else {
                // Otherwise "shadowBattleNewDynamicsRdfId" has already been broadcasted as DaRegular earlier.
                return false;
            }
        }
    }

    private unsafe bool broadcastDaRegularAsOwner(in byte[] toSendBytes, in int refRdfId) {
        if (refRdfId <= lastSentRefRdfIdAsOwner) {
            return false;
        }
        fixed (byte* rdfFetchBufferPtr = rdfFetchBuffer, ifdFetchBufferPtr = ifdFetchBuffer) {
            SteamP2PSessionManager.Instance.ownerSignalSenderBuffer.Add(toSendBytes);

            //Debug.Log($"@csharpTimerRdfId={csharpTimerRdfId}, buffered to be broadcasted DaRegular from owner due to [refRdfId={refRdfId}, lastSentIfdId={lastSentIfdId}, shadowBattleNewLcacIfdId={shadowBattleNewLcacIfdId}, pre-lastSentRefRdfAtTimerRdfIdAsOwner={lastSentRefRdfAtTimerRdfIdAsOwner}, pre-lastSentRefRdfIdAsOwner={lastSentRefRdfIdAsOwner}");

            lastSentRefRdfAtTimerRdfIdAsOwner = csharpTimerRdfId;
            lastSentRefRdfIdAsOwner = refRdfId;
        }
        return true;
    }

    // Update is called once per frame
    unsafe void Update() {
        try {
            var disconnectedPeerJoinIndices = SteamP2PSessionManager.Instance.GetDisconnectedPeerJoinIndices();

            if (PbPrimitivesOverride.ROOM_STATE_STOPPED == battleState) {
                // For proactive exit
                return;
            }

            if (PbPrimitivesOverride.ROOM_STATE_IN_SETTLEMENT == battleState) {
                // For settlement 
                return;
            }

            if (PbPrimitivesOverride.ROOM_STATE_FRONTEND_AWAITING_MANUAL_REJOIN == battleState) {
                rejoinPrompt.ToggleUIInteractability(true);
                rejoinPrompt.TogglePlayerInput(true);
                rejoinPrompt.gameObject.SetActive(true);
            }

            int timerRdfId = -1, chaserRdfIdLowerBound = -1, oldLcacIfdId = -1, oldUdpLcacIfdId = -1, toGenIfdId = -1, localRequiredIfdId = -1;
            
            bool inFrozenGracePeriod = (frozenRdfCount >= frozenRdfCountLimit);
            ifdFrontShouldFreeze = false;
            fixed (int* newChaserRdfIdPtr = &newChaserRdfId) {
                bool ok1 = Bindings.FRONTEND_GetRdfAndIfdIds(battle, &timerRdfId, newChaserRdfIdPtr, &chaserRdfIdLowerBound, &oldLcacIfdId, &oldUdpLcacIfdId, &toGenIfdId, &localRequiredIfdId);
                if (!ok1) {
                    return;
                }

                pollAndHandleFromOwnerRecvBuffer(&timerRdfId, newChaserRdfIdPtr, & chaserRdfIdLowerBound, &toGenIfdId, &localRequiredIfdId);
                
                pollAndHandleUdpRecvBuffer(oldUdpLcacIfdId);

                int chaserRdfIdValueNow = newChaserRdfId;

                SteamNetworkDoctor.Instance.LogLocalRequiredIfdId(localRequiredIfdId);

                if (useFreezingLockStep && !acLagShouldLockStep && !ifdFrontShouldLockStep) {
                    var (tooFastOrNot, ifdLag, sendingFps, peerUpsyncFps, rollbackFrames, ifdFrontLockedStepsCnt) = SteamNetworkDoctor.Instance.IsTooFast(roomCapacity, selfJoinIndexInt, maxPlayerInputFrontId, minPlayerInputFrontId: newUdpLcacIfdId, ifdLagTolerance: freezeIfdLagThresHold, disconnectedPeerJoinIndices);
                    int acIfdLag = (localRequiredIfdId - newLcacIfdId);
                    if (0 > acIfdLag) acIfdLag = 0;
                    if (acIfdLag < ifdLag) {
                        Debug.LogWarning($"@csharpTimerRdfId={csharpTimerRdfId}, acIfdLag={acIfdLag} < ifdLag={ifdLag}, how is this possible#1? localRequiredIfdId={localRequiredIfdId}, lcacIfdId={newLcacIfdId}, newUdpLcacIfdId={newUdpLcacIfdId}");
                    }
                    ifdFrontShouldFreeze = (!inFrozenGracePeriod && tooFastOrNot);

                    if (null != networkInfoPanel) {
                        networkInfoPanel.SetValues(sendingFps, peerUpsyncFps, ifdLag, ifdFrontLockedStepsCnt, rollbackFrames);
                    }
                }

                // [WARNING] Chasing should be executed regardless of whether or not "shouldLockStep" -- in fact it's even better to chase during "shouldLockStep"!
                Bindings.FRONTEND_ChaseRolledBackRdfs(battle, newChaserRdfIdPtr, false);
                if (chaserRdfIdValueNow + 1 >= csharpTimerRdfId) {
                    SteamNetworkDoctor.Instance.LogChasedToPlayerRdfId();
                }

                SteamNetworkDoctor.Instance.LogRollbackFrames(timerRdfId > newChaserRdfId ? (timerRdfId - newChaserRdfId) : 0);
            }

            if (PbPrimitivesOverride.ROOM_STATE_IN_BATTLE != battleState) {
                return;
            }

            if (localTimerEnded) {
                bool rdfAllConfirmed = (newLcacIfdId >= localRequiredIfdId);
                if (rdfAllConfirmed) {
                    timerEndedRdfDerivedFromAllConfirmedInputFrameDownsync = true;
                }
                if (!timerEndedRdfDerivedFromAllConfirmedInputFrameDownsync && 0 < timeoutMillisAwaitingLastAllConfirmedInputFrameDownsync) {
                    // TODO: Popup some GUI hint to tell the player that we're awaiting downsync only, as the local "timerRdfId" is monotonically increasing, there's no way to rewind and change any input from here!
                    timeoutMillisAwaitingLastAllConfirmedInputFrameDownsync -= 16; // hardcoded for now
                } else {
                    Debug.LogWarning($"Calling onBattleStopped with localTimerEnded @csharpTimerRdfId={csharpTimerRdfId}");
                    // settlementRdfId = timerRdfId;
                    onBattleStopped();
                    StartCoroutine(delayToShowSettlementPanel());
                }
                return;
            }


            if (acLagShouldLockStep || ifdFrontShouldFreeze || ifdFrontShouldLockStep) {
                if (acLagShouldLockStep) {
                    //Debug.LogWarning($"Frozen by acLagShouldLockStep @csharpTimerRdfId={csharpTimerRdfId}");
                    frozenRdfCount = 0;
                    frozenGracingRdfCnt = 0;
                } else if (ifdFrontShouldFreeze) {
                    SteamNetworkDoctor.Instance.LogIfdFrontLockedStepCnt();
                    //Debug.LogWarning($"Frozen by ifdFrontShouldLockStep @csharpTimerRdfId={csharpTimerRdfId}, frozenRdfCount={frozenRdfCount}/{frozenRdfCountLimit}");
                    if (!inFrozenGracePeriod && frozenRdfCount + 1 >= frozenRdfCountLimit) {
                        frozenGracingRdfCnt = 0;
                    }
                    ++frozenRdfCount;
                    if (SteamP2PSessionManager.Instance.GetIsCurrentLobbyOwner() && frozenRdfCountDaRegularBroadcastingThreshold == frozenRdfCount) {
                        if (-1 != lastSentRefRdfAtTimerRdfIdAsOwner) {
                            forceConfirmAllPeersAndBroadcastDaRegularAsOwner($"frozenRdfCount={frozenRdfCount}"); // For breaking "lag-induced-freezing avalanche across all players"
                        }
                    }
                } else {
                    SteamNetworkDoctor.Instance.LogIfdFrontLockedStepCnt();
                }
                ifdFrontShouldLockStep = false;
                ifdFrontShouldFreeze = false;
                acLagShouldLockStep = false;

                return;
            }

            if (inFrozenGracePeriod) {
                if (frozenGracingRdfCnt >= frozenGracePeriodRdfCount) {
                    frozenRdfCount = 0;
                    frozenGracingRdfCnt = 0;
                } else {
                    ++frozenGracingRdfCnt;
                }
            }

            var currSelfInput = null == iptmgr ? 0 : iptmgr.GetEncodedInput();
            int rdfBufferStRdfId = 0, rdfBufferEdRdfId = 0;

            bool toGenIfdSelfConfirmed = false;
            fixed (byte* shadowBattleDownsyncSnapshotBytesPtr = shadowBattleDownsyncSnapshotBytes)
            fixed (long* shadowBattleDownsyncSnapshotByteCntPtr = &shadowBattleDownsyncSnapshotByteCnt)
            fixed (int* shadowBattleOldLcacIfdIdPtr = &shadowBattleOldLcacIfdId, shadowBattleNewLcacIfdIdPtr = &shadowBattleNewLcacIfdId, shadowBattleOldDynamicsRdfIdPtr = &shadowBattleOldDynamicsRdfId, shadowBattleNewDynamicsRdfIdPtr = &shadowBattleNewDynamicsRdfId, shadowBattleEvictedStCntPtr = &shadowBattleEvictedStCnt, shadowBattleMaxPlayerInputFrontIdPtr = &shadowBattleMaxPlayerInputFrontId, shadowBattleMinPlayerInputFrontIdPtr = &shadowBattleMinPlayerInputFrontId)
            fixed (byte* ifdFetchBufferPtr = ifdFetchBuffer, rdfFetchBufferPtr = rdfFetchBuffer, stepResultFetchBufferPtr = stepResultFetchBuffer)
            fixed (int* newChaserRdfIdPtr = &newChaserRdfId) {
                long outBytesCnt = 0;
                long* outBytesCntPtr = &outBytesCnt;
                *outBytesCntPtr = pbBufferSizeLimit;
                *shadowBattleDownsyncSnapshotByteCntPtr = pbBufferSizeLimit;

                bool cmdInjected = FRONTEND_UpsertSelfCmd_With_Ifd_Output(battle, currSelfInput, newChaserRdfIdPtr, (char*)ifdFetchBufferPtr, outBytesCntPtr);
                if (!cmdInjected) {
                    Debug.LogWarning($"@csharpTimerRdfId={csharpTimerRdfId}, failed to inject currSelfInput={currSelfInput}, localRequiredIfdId={localRequiredIfdId}, lcacIfdId={newLcacIfdId}, newUdpLcacIfdId={newUdpLcacIfdId}");
                } else {
                    PreemptInputFrameDownsyncBeforeMerge(ifdHolder, PbPrimitivesOverride.Instance.getUnderlying());
                    ifdHolder.MergeFrom(ifdFetchBuffer, 0, (int)(*outBytesCntPtr));

                    ulong effSelfInput = ifdHolder.InputList[selfJoinIndexArrIdx];
                    toGenIfdSelfConfirmed = (0 < (ifdHolder.ConfirmedList & selfJoinIndexMask) || 0 < (ifdHolder.UdpConfirmedList & selfJoinIndexMask));
                    if (effSelfInput != currSelfInput && toGenIfdSelfConfirmed) {
                        //Debug.LogWarning($"@csharpTimerRdfId={csharpTimerRdfId}, effSelfInput={effSelfInput} conflicting currSelfInput={currSelfInput} with selfJoinIndex={selfJoinIndex}, confirmedList={ifdHolder.ConfirmedList}, udpConfirmedList={ifdHolder.UdpConfirmedList}");
                    }
                }
                FRONTEND_Step(battle);
                
                if (stepShadowBattleAlongWithMainBattle) {
                    BACKEND_MoveForwardLcacIfdIdAndStep(shadowBattle, withRefRdf: true, shadowBattleOldLcacIfdIdPtr, shadowBattleNewLcacIfdIdPtr, shadowBattleOldDynamicsRdfIdPtr, shadowBattleNewDynamicsRdfIdPtr, (char*)shadowBattleDownsyncSnapshotBytesPtr, shadowBattleDownsyncSnapshotByteCntPtr);
                    if (SteamP2PSessionManager.Instance.GetIsCurrentLobbyOwner() && broadcastShadowBattleDownsyncSnapshotAlongWithMainBattle) {
                        if (0 < *shadowBattleDownsyncSnapshotByteCntPtr && shadowBattleOldDynamicsRdfId < shadowBattleNewDynamicsRdfId) {
                            if (SteamP2PSessionManager.Instance.GetIsCurrentLobbyOwner()) {
                                byte[] copiedBytes = new byte[(int)(*shadowBattleDownsyncSnapshotByteCntPtr)];
                                Buffer.BlockCopy(shadowBattleDownsyncSnapshotBytes, 0, copiedBytes, 0, copiedBytes.Length);
                                broadcastDaRegularAsOwner(copiedBytes, shadowBattleNewDynamicsRdfId);
                            }
                        }
                    }
                } else {
                    if (SteamP2PSessionManager.Instance.GetIsCurrentLobbyOwner() && !broadcastShadowBattleDownsyncSnapshotAlongWithMainBattle) {
                        if (-1 == lastSentRefRdfAtTimerRdfIdAsOwner && csharpTimerRdfId > (REF_RDF_ID_INTERVAL_RDF_CNT << 1)) {
                            forceConfirmAllPeersAndBroadcastDaRegularAsOwner($"csharpTimerRdfId={csharpTimerRdfId}, {lastSentRefRdfAtTimerRdfIdAsOwner}=-1, initial force-confirmation to all peers");
                        } else if (csharpTimerRdfId > (lastSentRefRdfAtTimerRdfIdAsOwner + REF_RDF_ID_INTERVAL_RDF_CNT)) {
                            forceConfirmAllPeersAndBroadcastDaRegularAsOwner($"csharpTimerRdfId={csharpTimerRdfId} > lastSentRefRdfAtTimerRdfIdAsOwner={lastSentRefRdfAtTimerRdfIdAsOwner} + REF_RDF_ID_INTERVAL_RDF_CNT={REF_RDF_ID_INTERVAL_RDF_CNT}");
                        }
                    }
                }

                csharpTimerRdfId = timerRdfId+1;
                bool snatched = (csharpTimerRdfId == chaserRdfIdLowerBound);
                if (snatched) {
                    SteamNetworkDoctor.Instance.LogForceResyncFutureApplied();
                } else if (csharpTimerRdfId + slowDownRdfLagThreshold < chaserRdfIdLowerBound) {
                    *outBytesCntPtr = pbBufferSizeLimit;
                    bool extraSteppingCmdInjected = FRONTEND_UpsertSelfCmd_With_Ifd_Output(battle, currSelfInput, newChaserRdfIdPtr, (char*)ifdFetchBufferPtr, outBytesCntPtr); // [WARNING] Don't forget to call "FRONTEND_UpsertSelfCmd_With_Ifd_Output(...)" for extra stepping, otherwise some "FrontendBattle.ifdBuffer" would lack "self-confirmed bitmask" and thus blocking "FRONTEND_ProduceUpsyncSnapshotRequest(...)" from setting "actualLastIfdInBatch" correctly. 
                    if (extraSteppingCmdInjected) {
                        PreemptInputFrameDownsyncBeforeMerge(ifdHolder, PbPrimitivesOverride.Instance.getUnderlying());
                        ifdHolder.MergeFrom(ifdFetchBuffer, 0, (int)(*outBytesCntPtr));
                        toGenIfdSelfConfirmed = (0 < (ifdHolder.ConfirmedList & selfJoinIndexMask) || 0 < (ifdHolder.UdpConfirmedList & selfJoinIndexMask));
                    }
                    FRONTEND_Step(battle);
                    Debug.LogWarning($"@csharpTimerRdfId={csharpTimerRdfId}, toGenIfdId={toGenIfdId}, chaserRdfIdLowerBound={chaserRdfIdLowerBound}, localRequiredIfdId={localRequiredIfdId}, lcacIfdId={newLcacIfdId}, newUdpLcacIfdId={newUdpLcacIfdId}: 1 extra FRONTEND_Step for quicker snatching");
                    csharpTimerRdfId = timerRdfId+1;
                }
                
                *outBytesCntPtr = pbBufferSizeLimit;
                bool rdfFetched = APP_GetRdf(battle, csharpTimerRdfId, (char*)rdfFetchBufferPtr, outBytesCntPtr);
                Bindings.APP_GetRdfBufferBounds(battle, &rdfBufferStRdfId, &rdfBufferEdRdfId);
                if (false == rdfFetched) {
                    throw new ArgumentNullException($"@csharpTimerRdfId={csharpTimerRdfId}, failed to fetch rdf for rendering with rdfBufferStRdfId={rdfBufferStRdfId}, rdfBufferEdRdfId={rdfBufferEdRdfId}, selfJoinIndex={selfJoinIndex}");
                }
                PreemptRenderFrameBeforeMerge(rdfHolder, PbPrimitivesOverride.Instance.getUnderlying());
                rdfHolder.MergeFrom(rdfFetchBuffer, 0, (int)(*outBytesCntPtr));

                *outBytesCntPtr = pbBufferSizeLimit;
                PreemptStepResult(stepResultHolder, primitives);
                bool stepResultFetched = APP_GetStepResult(battle, csharpTimerRdfId, (char*)stepResultFetchBufferPtr, outBytesCntPtr);
                if (stepResultFetched) {
                    stepResultHolder.MergeFrom(stepResultFetchBuffer, 0, (int)(*outBytesCntPtr));
                    /*
                    if (0 < stepResultHolder.AimingRayCount) {
                        Debug.Log($"There's {stepResultHolder.AimingRayCount} aiming rays at csharpTimerRdfId={csharpTimerRdfId}#1");
                    }
                    */
                }

                applyRdf(rdfHolder, stepResultHolder, PbPrimitivesOverride.Instance.getUnderlying().EstimatedSecondsPerRdf);
            }

            if (newLcacIfdId > lastSentIfdId) {
                lastSentIfdId = newLcacIfdId;
            }
            if (lastSentIfdId < toGenIfdId) {
                upsyncSnapshotReqBufferBytesCnt = pbBufferSizeLimit;
                fixed (byte* upsyncSnapshotPtr = upsyncSnapshotReqBuffer) 
                fixed (long *outBytesCntPtr = &upsyncSnapshotReqBufferBytesCnt) {
                    int proposedBatchIfdIdSt = lastSentIfdId + 1;
                    int proposedBatchIfdIdEd = toGenIfdId + 1;
                    int actualLastIfdInBatch = -1;
                    FRONTEND_ProduceUpsyncSnapshotRequest(battle, upsyncSeqNo++, proposedBatchIfdIdSt, proposedBatchIfdIdEd, &actualLastIfdInBatch, (char*)upsyncSnapshotPtr, outBytesCntPtr);
                    if (0 < upsyncSnapshotReqBufferBytesCnt) {
                        if (newLcacIfdId < actualLastIfdInBatch) {
                            byte[] copiedBytes = new byte[(int)upsyncSnapshotReqBufferBytesCnt];
                            Buffer.BlockCopy(upsyncSnapshotReqBuffer, 0, copiedBytes, 0, copiedBytes.Length);
                            SteamP2PSessionManager.Instance.senderBuffer.Add(copiedBytes);
                        } else {
                            // else no need to waste bandwidth in sending an already all-confirmed "UpsyncSnapshot".
                            Debug.Log($"@csharpTimerRdfId={csharpTimerRdfId}, lastSentIfdId={lastSentIfdId}, toGenIfdId={toGenIfdId}, newLcacIfdId={newLcacIfdId}, actualLastIfdInBatch={actualLastIfdInBatch}, newUdpLcacIfdId={newUdpLcacIfdId}: skip sending (but still log sending rate for reference)");
                        }

                        lastSentIfdId = actualLastIfdInBatch;
                        SteamNetworkDoctor.Instance.LogSending(proposedBatchIfdIdSt, proposedBatchIfdIdEd);
                    } else if (toGenIfdSelfConfirmed) {
                        Debug.LogWarning($"@csharpTimerRdfId={csharpTimerRdfId}, upsyncSnapshotReqBufferBytesCnt=0 while toGenIfdSelfConfirmed=true, couldn't send or log sending for lastSentIfdId={lastSentIfdId}, toGenIfdId={toGenIfdId}, newLcacIfdId={newLcacIfdId}, actualLastIfdInBatch={actualLastIfdInBatch}, newUdpLcacIfdId={newUdpLcacIfdId}");
                    }
                }
            }

            {  
                var (tooFastOrNot, ifdLag, sendingFps, peerUpsyncFps, rollbackFrames, ifdFrontLockedStepsCnt) = SteamNetworkDoctor.Instance.IsTooFast(roomCapacity, selfJoinIndexInt, maxPlayerInputFrontId, minPlayerInputFrontId: newUdpLcacIfdId, ifdLagTolerance: slowDownIfdLagThreshold, disconnectedPeerJoinIndices);

                ifdFrontShouldLockStep = tooFastOrNot;
                if (inFrozenGracePeriod && ifdFrontShouldLockStep) {
                    if (!SteamP2PSessionManager.Instance.GetIsCurrentLobbyOwner() && (csharpTimerRdfId > chaserRdfIdLowerBound + (freezeRdfLagThresHold << 1))) {
                        // [REMINDER] This is the "P2P analogy" of "acLagShouldLockStep".
                        Debug.LogWarning($"@csharpTimerRdfId={csharpTimerRdfId}, chaserRdfIdLowerBound={chaserRdfIdLowerBound} inFrozenGracePeriod but too advanced as Lobby non-owner, will disable frozenGracePeriod: localRequiredIfdId={localRequiredIfdId}, lcacIfdId={newLcacIfdId}, newUdpLcacIfdId={newUdpLcacIfdId}");
                    } else {
                        ifdFrontShouldLockStep = false;
                    }
                }
                int acIfdLag = (localRequiredIfdId - newLcacIfdId);
                if (0 > acIfdLag) acIfdLag = 0;
                if (acIfdLag < ifdLag) {
                    Debug.LogWarning($"@csharpTimerRdfId={csharpTimerRdfId}, acIfdLag={acIfdLag} < ifdLag={ifdLag}, how is this possible#2? localRequiredIfdId={localRequiredIfdId}, lcacIfdId={newLcacIfdId}, newUdpLcacIfdId={newUdpLcacIfdId}");
                }
                if (null != networkInfoPanel) {
                    networkInfoPanel.SetValues(sendingFps, peerUpsyncFps, ifdLag, ifdFrontLockedStepsCnt, rollbackFrames);
                }
            }

            if (csharpTimerRdfId > battleDurationFrames) {
                localTimerEnded = true;
            } else {
                readyGoPanel.setCountdown(csharpTimerRdfId, battleDurationFrames);
            }

            bool battleResultIsSet = isBattleResultSet(stepResultHolder);
            if (battleResultIsSet) {
                Debug.LogWarning($"Calling onBattleStopped with battleResultIsSet=true @csharpTimerRdfId={csharpTimerRdfId}");
                //settlementRdfId = timerRdfId;
                onBattleStopped();
                StartCoroutine(delayToShowSettlementPanel());
            } else {
                if (skipInterpolation) {
                    skipInterpolation = false;
                }
            }
            //throw new NotImplementedException("Intended");
        } catch (Exception ex) {
            Debug.LogError($"Error during OnlineMap.Update csharpTimerRdfId={csharpTimerRdfId}, calling cleanupNetworkSessionsReentrantSafe for manual rejoin: {ex.StackTrace}");
            battleState = PbPrimitivesOverride.ROOM_STATE_FRONTEND_AWAITING_MANUAL_REJOIN; 
            autoRejoinQuota = 0; // To require manual rejoin.
            cleanupNetworkSessionsReentrantSafe();
        }
    }

    protected void resetCurrentMatch(string theme) {
        if (null != underlyingMap) {
            Destroy(underlyingMap);
        }
        string path = $"Tiled/{theme}/map";
        var underlyingMapPrefab = Resources.Load(path) as GameObject;
        if (null == underlyingMapPrefab) {
            Debug.LogError($"underlyingMapPrefab is null for theme={theme}");
        }
        underlyingMap = GameObject.Instantiate(underlyingMapPrefab, this.gameObject.transform);
        var superMap = underlyingMap.GetComponent<SuperMap>();
        int mapWidth = superMap.m_Width, tileWidth = superMap.m_TileWidth, mapHeight = superMap.m_Height, tileHeight = superMap.m_TileHeight;
        tilemapHalfWidth = (mapWidth * tileWidth) >> 1;
        tilemapHalfHeight = (mapHeight * tileHeight) >> 1;
        collisionSpaceHalfWidth = (int)tilemapHalfWidth;
        collisionSpaceHalfHeight = (int)tilemapHalfHeight;
        collisionSpacePaddingLeft = 0;
        collisionSpacePaddingBottom = 0;

        // Reset lockstep
        ifdFrontShouldLockStep = false;
        ifdFrontShouldFreeze = false;
        frozenRdfCount = 0;
        frozenGracingRdfCnt = 0;
        localTimerEnded = false;
        lastSentIfdId = -1;
        lastSentRefRdfIdAsOwner = -1;
        lastSentRefRdfAtTimerRdfIdAsOwner = -1;
        timerEndedRdfDerivedFromAllConfirmedInputFrameDownsync = false;
        timeoutMillisAwaitingLastAllConfirmedInputFrameDownsync = DEFAULT_TIMEOUT_FOR_LAST_ALL_CONFIRMED_IFD;

        SteamNetworkDoctor.Instance.Reset();
    }

    protected void cleanupNetworkSessionsReentrantSafe() {
        SteamNetworkDoctor.Instance.Reset();

        if (null != p2pSessionCancellationTokenSource) {
            try {
                if (!p2pSessionCancellationTokenSource.IsCancellationRequested) {
                    //Debug.Log(String.Format("SteamOnlineMapController.cleanupNetworkSessions, cancelling udp session"));
                    p2pSessionCancellationTokenSource.Cancel();
                } else {
                    //Debug.LogWarning(String.Format("SteamOnlineMapController.cleanupNetworkSessions, udpCancellationTokenSource is already cancelled!"));
                }
                p2pSessionCancellationTokenSource.Dispose();
            } catch (ObjectDisposedException ex) {
                //Debug.LogWarning(String.Format("SteamOnlineMapController.cleanupNetworkSessions, udpCancellationTokenSource is already disposed: {0}", ex));
            } finally {
                Debug.Log("SteamOnlineMapController.cleanupNetworkSessionsReentrantSafe, udpCancellationTokenSource disposed");
            }
            p2pSessionCancellationTokenSource = null;
        }

        if (null != p2pSessionTask) {
            if (TaskStatus.Canceled != p2pSessionTask.Status && TaskStatus.Faulted != p2pSessionTask.Status) {
                try {
                    //Debug.Log($"SteamOnlineMapController.cleanupNetworkSessions, about to wait for p2pSessionTask with status={p2pSessionTask.Status}");
                    var waitingSuccess = p2pSessionTask.Wait(3000);
                    //Debug.Log($"SteamOnlineMapController.cleanupNetworkSessions, p2pSessionTask returns with waitingSuccess={waitingSuccess}, status={p2pSessionTask.Status}");
                } catch (Exception ex) {
                    //Debug.LogWarning(String.Format("SteamOnlineMapController.cleanupNetworkSessions, p2pSessionTask is already cancelled: {0}", ex));
                } finally {
                    try {
                        p2pSessionTask.Dispose(); // frontend of this project targets ".NET Standard 2.1", thus calling "Task.Dispose()" explicitly, reference, reference https://learn.microsoft.com/en-us/dotnet/api/system.threading.tasks.task.dispose?view=net-7.0
                    } catch (ObjectDisposedException ex) {
                        //Debug.LogWarning(String.Format("SteamOnlineMapController.cleanupNetworkSessions, p2pSessionTask is already disposed: {0}", ex));
                    } finally {
                        Debug.Log(String.Format("SteamOnlineMapController.cleanupNetworkSessionsReentrantSafe, p2pSessionTask disposed"));
                    }
                }
            }
            p2pSessionTask = null;
        }

        for (int i = 0; i < MAX_MSG_COUNT_PER_POLL; i++) {
            if (IntPtr.Zero != messagePointers[i]) {
                SteamNetworkingMessage_t.Release(messagePointers[i]);
                messagePointers[i] = IntPtr.Zero;
            }
        }

        SteamP2PSessionManager.Instance.LeaveCurrentLobbyReentrantSafe();
    }

    protected override void OnDestroy() {
        Debug.LogWarning($"SteamOnlineMapController.OnDestroy#1, about to clean up network sessions");
        onBattleStopped();
        if (UIntPtr.Zero != battle) {
            APP_DestroyBattle(battle);
            battle = UIntPtr.Zero;
        }

        if (UIntPtr.Zero != shadowBattle) {
            APP_DestroyBattle(shadowBattle);
            shadowBattle = UIntPtr.Zero;
        }
        bool shutdownRes = JPH_Shutdown();
        Debug.Log($"SteamOnlineMapController.OnDestroy#2, Jolt infra shutdown result = {shutdownRes}");
        
        cleanupNetworkSessionsReentrantSafe();
        Debug.LogWarning($"SteamOnlineMapController.OnDestroy#3, cleaned network sessions");

    }

    void OnApplicationQuit() {
        Debug.LogWarning(String.Format("SteamOnlineMapController.OnApplicationQuit"));
    }

    protected void showCharacterSelection() {
        if (characterSelectPanel.gameObject.activeSelf) {
            Debug.LogWarning("Calling showCharacterSelection while the panel is already active");
            return;
        }
        characterSelectPanel.ToggleUIInteractability(true);
        characterSelectPanel.gameObject.SetActive(true);
        Debug.LogWarning("Called showCharacterSelection");

        if (settlementPanel.gameObject.activeSelf) {
            settlementPanel.gameObject.SetActive(false);
        }
    }

    protected IEnumerator delayToShowSettlementPanel() {
        yield return new WaitForEndOfFrame();
        var arenaSettlementPanel = settlementPanel as ArenaSettlementPanel;
        if (PbPrimitivesOverride.ROOM_STATE_IN_BATTLE == battleState) {
            Debug.LogWarning($"Why calling delayToShowSettlementPanel during active battle? timerRdfId = {csharpTimerRdfId}");
        } else {
            battleState = PbPrimitivesOverride.ROOM_STATE_IN_SETTLEMENT;
            arenaSettlementPanel.postSettlementCallback = () => {
                showCharacterSelection();
            };
            arenaSettlementPanel.gameObject.SetActive(true);
            arenaSettlementPanel.toggleUIInteractability(true);
            /*
            var (ok, rdf) = renderBuffer.GetByFrameId(settlementRdfId - 1);
            if (ok && null != rdf) {
                arenaSettlementPanel.SetCharacters(rdf.PlayersArr);
            } else {
                Debug.LogWarning("No character info to show for settlementRdfId=" + settlementRdfId);
            }
            */
        }
    }

    public override void OnSettingsClicked() {
        if (PbPrimitivesOverride.ROOM_STATE_IN_BATTLE != battleState) return;
        arenaModeSettings.ToggleUIInteractability(true);
        arenaModeSettings.gameObject.SetActive(true);
    }

    void OnApplicationFocus(bool hasFocus) {
        if (!hasFocus) {
            Debug.Log("Application lost focus at: " + Time.time);
            if (null != focusLossPrompt) {
                focusLossPrompt.gameObject.SetActive(true);
                focusLossPrompt.toggleUIInteractability(true);
            }
        } else {
            Debug.Log("Application regained focus.");
            focusLossPrompt.OnExit();
        }
    }
}
