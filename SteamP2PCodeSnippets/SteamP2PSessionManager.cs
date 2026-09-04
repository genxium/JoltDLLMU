using Google.Protobuf;
using Google.Protobuf.Collections;
using JoltCSharp;
using jtshared;
using Steamworks;
using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Threading;
using System.Threading.Tasks;
using UnityEngine;

public class SteamP2PSessionManager {

    /////////////////////////////////////////////////////////////////////////////////////////
    public const int UPSYNC_SNAPSHOT_IO_CHANNEL = 0;
    public const int DOWNSYNC_SNAPSHOT_IO_CHANNEL = 1;

    public CSteamID GetCurrentLobbyId() {
        return m_currentLobbyId;
    }

    public int GetLobbyCapacity() {
        return lobbyCapacity;
    }

    public bool GetIsCurrentLobbyOwner() {
        return m_isCurrentLobbyOwner;
    }

    public CSteamID GetCurrentLobbyOwnerId() {
        return m_currentLobbyOwnerId;
    }

    public uint GetJoinIndexInLobby(ulong ulSteamID) {
        if (null == lockedLobbyMemberUlSteamIdToJoinIndex) {
            return PbPrimitivesOverride.Instance.getUnderlying().MagicJoinIndexInvalid;
        }
        if (!lockedLobbyMemberUlSteamIdToJoinIndex.ContainsKey(ulSteamID)) {
            return PbPrimitivesOverride.Instance.getUnderlying().MagicJoinIndexInvalid;
        }
        return lockedLobbyMemberUlSteamIdToJoinIndex[ulSteamID];
    }

    public void CreateLobby(in ELobbyType lobbyType, in int maxParticipantCount, in string motivation) {
        Debug.Log($"Creating lobby with lobbyType={lobbyType}, maxParticipantCount={maxParticipantCount} due to motivation={motivation}...");
        SteamMatchmaking.CreateLobby(lobbyType, maxParticipantCount);
    }

    public void FindAvailableLobbies() {
        Debug.Log($"Searching for active lobbies for filter `{GDK_GAME_NAME_AND_VER}`=`{Application.productName}_{Application.version}`...");
        // Filter results so clients only see matches with matching metadata game strings
        SteamMatchmaking.AddRequestLobbyListStringFilter(GDK_GAME_NAME_AND_VER, $"{Application.productName}_{Application.version}", ELobbyComparison.k_ELobbyComparisonEqual);

        // Request the filtered list from Steam
        SteamAPICall_t steamCall = SteamMatchmaking.RequestLobbyList();
        m_LobbyMatchList.Set(steamCall);
    }

    public bool JoinTargetLobby(CSteamID targetLobbyId, in string motivation) {
        int lobbyMembersCnt = SteamMatchmaking.GetNumLobbyMembers(targetLobbyId);
        bool isArealdyInTargetLobby = false;
        for (int i = 0; i < lobbyMembersCnt; i++) {
            CSteamID memberSteamID = SteamMatchmaking.GetLobbyMemberByIndex(m_currentLobbyId, i);
            if (CSteamID.Nil == memberSteamID) {
                break;
            }
            if (memberSteamID == SteamUser.GetSteamID()) {
                isArealdyInTargetLobby = true;
                break;
            }
        }

        if (isArealdyInTargetLobby) {
            Debug.Log($"Self is already in targetLobbyId={targetLobbyId} with existingNumLobbyMembers={lobbyMembersCnt}, motivation ={motivation}...");
            return false;
        }

        Debug.Log($"Joining targetLobbyId={targetLobbyId} with motivation={motivation}...");
        SteamMatchmaking.JoinLobby(targetLobbyId);
        return true;
    }

    public void LeaveCurrentLobbyReentrantSafe() {
        if (CSteamID.Nil == m_currentLobbyId) {
            return;
        }

        SteamMatchmaking.LeaveLobby(m_currentLobbyId);
        Debug.Log($"Left targetLobbyId={m_currentLobbyId}");
        ResetP2PSession(map, "Proactively leaving lobby");
    }

    public static SteamP2PSessionManager Instance {
        get {
            if (null != _instance) {
                return _instance;
            }
            lock (_padLock) {
                if (null == _instance) {
                    _instance = new SteamP2PSessionManager();
                }
            }
            return _instance;
        }
    }

    protected BlockingCollection<byte[]> senderBuffer;
    public void EnqueSenderBuffer(in byte[] bytes) {
        senderBuffer.Add(bytes);
    }

    protected BlockingCollection<byte[]> ownerSignalSenderBuffer;
    public void EnqueOwnerSignalSenderBuffer(in byte[] bytes) {
        ownerSignalSenderBuffer.Add(bytes);
    }

    protected Queue<byte[]> localDownsyncSnapshotBytesBuffer;
    public bool DequeLocalDownsyncSnapshotBytesBuffer(out byte[] recvBytes) {
        return localDownsyncSnapshotBytesBuffer.TryDequeue(out recvBytes);
    }

    protected Queue<byte[]> localUpsyncSnapshotBytesBuffer;
    public bool DequeLocalUpsyncSnapshotBytesBuffer(out byte[] recvBytes) {
        return localUpsyncSnapshotBytesBuffer.TryDequeue(out recvBytes);
    }

    protected int sendBufferReadTimeoutMillis = 512;
    protected uint localSeqNo = 0;

    public void ResetP2PSession(in SteamOnlineMapController theMap, in string motivation) {
        map = theMap;
        localSeqNo = 0;

        closeSessionsWithLockedLobbyMembers($"ResetP2PSession/{motivation}"); // [REMINDER] Such that there's no backlogged messages upon network switch
        clearPassiveCallbacks();
        m_currentLobbyId = CSteamID.Nil;
        m_currentLobbyOwnerId = CSteamID.Nil;
        m_isCurrentLobbyOwner = false;
        m_lobbyMemberBindings = null;
        lockedLobbyMemberBindings = null;
        lockedMemberIdentities = null;
        lockedLobbyMemberUlSteamIdToJoinIndex = null;
        localDownsyncSnapshotBytesBuffer.Clear();
        localUpsyncSnapshotBytesBuffer.Clear();
        disconnectedPeerJoinIndices.Clear();
        setPassiveCallbacks();

        Debug.Log($"{motivation}: Called `ResetP2PSession` at thread ud={Thread.CurrentThread.ManagedThreadId}.");
    }

    public async Task OpenSession(CancellationToken sessionCancellationToken) {
        try {
            ++localSeqNo;
            Debug.Log($"OpenSession#1: thread ud={Thread.CurrentThread.ManagedThreadId}...");
            await Task.WhenAll(Task.Run(async () => send(sessionCancellationToken)), Task.Run(async () => ownerSignalSend(sessionCancellationToken)));
            Debug.Log($"All p2pSession tasks are ended @localSeqNo={localSeqNo}.");
        } catch (Exception ex) {
            Debug.LogError($"Error opening p2pSession @localSeqNo={localSeqNo}: {ex}");
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////////
    protected SteamOnlineMapController map;
    protected CallResult<LobbyMatchList_t> m_LobbyMatchList;

    protected Callback<LobbyCreated_t> m_LobbyCreated;
    protected Callback<LobbyEnter_t> m_LobbyEntered;

    protected Callback<LobbyDataUpdate_t> m_LobbyDataUpdated;
    protected Callback<LobbyKicked_t> m_LobbyKicked;

    protected Callback<LobbyChatUpdate_t> m_LobbyChatUpdated;
    protected Callback<LobbyChatMsg_t> m_LobbyChatMsg;

    protected Callback<SteamNetworkingMessagesSessionRequest_t> m_SessionRequestCallback;
    protected Callback<SteamNetworkingMessagesSessionFailed_t> m_SessionFailedCallback;
    protected Callback<SteamNetConnectionStatusChangedCallback_t> m_connectionStatusChangedCallback;

    protected CSteamID m_currentLobbyId = CSteamID.Nil;
    protected CSteamID m_currentLobbyOwnerId = CSteamID.Nil;
    protected bool m_isCurrentLobbyOwner = false;
    protected string GDK_HOST_PLAYER_NAME = "HPN";
    protected string GDK_GAME_NAME_AND_VER = "GNV";
    protected string GDK_CH_SPECIES = "CHS";
    protected int lobbyCapacity = 2;
    protected SteamBinding[] m_lobbyMemberBindings;
    protected SteamBinding[] lockedLobbyMemberBindings;
    protected SteamNetworkingIdentity[] lockedMemberIdentities;
    protected Dictionary<ulong, uint> lockedLobbyMemberUlSteamIdToJoinIndex;
    protected HashSet<uint> disconnectedPeerJoinIndices = new HashSet<uint>();
    public HashSet<uint> GetDisconnectedPeerJoinIndices() {
        return disconnectedPeerJoinIndices;
    }

    public bool AddDisconnectedRecord(in uint peerJoinIndex, in string motivation) {
        if (!disconnectedPeerJoinIndices.Contains(peerJoinIndex)) {
            disconnectedPeerJoinIndices.Add(peerJoinIndex);
            var single = lockedMemberIdentities[peerJoinIndex - 1];
            SteamNetworkingMessages.CloseSessionWithUser(ref single);
            Debug.Log($"Closed SteamNetworkingMessages session with {single.GetSteamID()}, peerJoinIndex={peerJoinIndex}: AddDisconnectedRecord/{motivation}");
            if (null != map) {
                map.ToggleInactiveJoinIndexMask(peerJoinIndex);
            }
            return true;
        }

        return false;
    }

    public bool RemoveDisconnectedRecord(in uint joinIndex, in string motivation) {
        if (disconnectedPeerJoinIndices.Contains(joinIndex)) {
            disconnectedPeerJoinIndices.Remove(joinIndex);
            if (null != map) {
                map.ToggleInactiveJoinIndexMask(joinIndex);
            }
            Debug.Log($"Peer joinIndex={joinIndex} is removed from disconnectedPeerJoinIndices: {motivation}");
            return true;
        }
        return false;
    }

    protected byte[] chatRecvBuff = new byte[PbPrimitivesOverride.Instance.getUnderlying().FrontendWsRecvBytelength];

    public void SetLockedLobbyMemberBindingsWhenWaiting(RepeatedField<SteamBinding> lockedLobbyMemberBindingsFromOwner) {
        int cap = (lockedLobbyMemberBindingsFromOwner.Count < lobbyCapacity ? lockedLobbyMemberBindingsFromOwner.Count : lobbyCapacity); 
        lockedLobbyMemberBindings = new SteamBinding[cap];
        lockedMemberIdentities = new SteamNetworkingIdentity[cap];
        lockedLobbyMemberUlSteamIdToJoinIndex = new Dictionary<ulong, uint>();
        for (int i = 0; i < cap; i++) {
            lockedLobbyMemberBindings[i] = lockedLobbyMemberBindingsFromOwner[i].Clone();
            lockedMemberIdentities[i] = new SteamNetworkingIdentity();
            lockedMemberIdentities[i].SetSteamID64(lockedLobbyMemberBindings[i].UlSteamId);
            lockedLobbyMemberUlSteamIdToJoinIndex[lockedLobbyMemberBindings[i].UlSteamId] = (uint)(i + 1);
        }
        if (null != map) {
            map.OnLobbyMembersUpdatedWhenWaiting(lockedLobbyMemberBindings, "SetLockedLobbyMemberBindingsWhenWaiting");
        }
    }

    protected int updateMemberBindings() {
        int effLobbyMemberCnt = 0;
        m_lobbyMemberBindings = new SteamBinding[lobbyCapacity];
        int lobbyMembersCnt = (CSteamID.Nil == m_currentLobbyId ? 0 : SteamMatchmaking.GetNumLobbyMembers(m_currentLobbyId));
        //Debug.Log($"There're {lobbyMembersCnt} members in lobby {m_currentLobbyId}");
        for (int i = 0; i < lobbyMembersCnt; i++) {
            CSteamID memberSteamID = SteamMatchmaking.GetLobbyMemberByIndex(m_currentLobbyId, i);
            if (CSteamID.Nil == memberSteamID) {
                break;
            }
            uint chSpeciesId = PbPrimitivesOverride.SPECIES_NONE_CH;
            string chSpeciesIdStr = SteamMatchmaking.GetLobbyMemberData(m_currentLobbyId, memberSteamID, GDK_CH_SPECIES);
            if (!uint.TryParse(chSpeciesIdStr, out chSpeciesId)) {
                Debug.LogError($"Error parsing chSpeciesIdStr={chSpeciesIdStr}");
            }
            m_lobbyMemberBindings[i] = new SteamBinding {
                UlSteamId = memberSteamID.m_SteamID,
                ChSpeciesId = chSpeciesId,
            };
            //Debug.Log($"JoinIndex={i+1} having chSpeciesId={chSpeciesId} in lobby {m_currentLobbyId}");
            ++effLobbyMemberCnt;
        }

        //Debug.Log($"Counted effLobbyMemberCnt={effLobbyMemberCnt} in lobby {m_currentLobbyId}");
        for (int j = effLobbyMemberCnt; j < lobbyCapacity; j++) {
            m_lobbyMemberBindings[j] = new SteamBinding {
                UlSteamId = CSteamID.Nil.m_SteamID,
                ChSpeciesId = PbPrimitivesOverride.SPECIES_NONE_CH,
            };
        }

        return effLobbyMemberCnt;
    }

    protected virtual void onLobbyMatchList(LobbyMatchList_t callback, bool bIOFailure) {
        if (bIOFailure) {
            Debug.LogError("Lobby match list query failed due to a critical network I/O error.");
            return;
        }
        Debug.Log($"Found {callback.m_nLobbiesMatching} lobbies.");

        if (0 < callback.m_nLobbiesMatching) {
            // Loop through the results and join the first valid match found
            for (int i = 0; i < callback.m_nLobbiesMatching; i++) {
                CSteamID lobbyId = SteamMatchmaking.GetLobbyByIndex(i);
                string hostName = SteamMatchmaking.GetLobbyData(lobbyId, GDK_HOST_PLAYER_NAME);
                Debug.Log($"Attempting to join lobby hosted by: {hostName}");
                JoinTargetLobby(lobbyId, "###Found available hosted lobby###");
                break; // Stop loop after choosing a lobby
            }
        } else {
            CreateLobby(ELobbyType.k_ELobbyTypePublic, lobbyCapacity, "###No available lobby###");
        }
    }

    protected void onLobbyEntered(LobbyEnter_t callback) {
        EChatRoomEnterResponse resp = (EChatRoomEnterResponse)((int)callback.m_EChatRoomEnterResponse);
        var battleState = map.GetBattleState();
        CSteamID targetLobbyId = new CSteamID(callback.m_ulSteamIDLobby);
        if (EChatRoomEnterResponse.k_EChatRoomEnterResponseSuccess == resp) {
            m_currentLobbyId = targetLobbyId;

            // Hand off to your underlying P2P Network Transport layer here
            // The host's SteamID can be discovered by querying the lobby owner
            CSteamID lobbyOwner = SteamMatchmaking.GetLobbyOwner(m_currentLobbyId);
            m_currentLobbyOwnerId = lobbyOwner;
            m_isCurrentLobbyOwner = (m_currentLobbyOwnerId == SteamUser.GetSteamID());
            int lobbyMembersCnt = SteamMatchmaking.GetNumLobbyMembers(targetLobbyId);

            if (m_isCurrentLobbyOwner) {
                Debug.Log($"[I am OWNER] Entered as Owner of lobby: {m_currentLobbyId}, now battleState={battleState}, lobbyMembersCnt={lobbyMembersCnt}");
            } else {
                Debug.Log($"Entered as a Participant of lobby: {m_currentLobbyId} owned by {m_currentLobbyOwnerId}, now battleState={battleState}, lobbyMembersCnt={lobbyMembersCnt}");
            }

            SteamMatchmaking.SetLobbyMemberData(m_currentLobbyId, GDK_CH_SPECIES, JoltWsSessionManager.Instance.GetSpeciesId().ToString());
            disconnectedPeerJoinIndices.Clear();
            map.OnLobbyEntered();
        } else if (EChatRoomEnterResponse.k_EChatRoomEnterResponseFull == resp) {
            if (PbPrimitivesOverride.ROOM_STATE_FRONTEND_REJOINING == battleState) {
                map.OnRejoinFailed(PbPrimitivesOverride.ROOM_STATE_FRONTEND_REJOINING);
            } else if (PbPrimitivesOverride.ROOM_STATE_IDLE < battleState) {
                Debug.LogWarning($"###Failed to enter targetLobbyId={targetLobbyId} at battleState={battleState} due to {resp}###");
            } else {
                CreateLobby(ELobbyType.k_ELobbyTypePublic, lobbyCapacity, $"###Failed to enter targetLobbyId={targetLobbyId} at battleState={battleState} due to {resp}###");
            }
        } else {
            CreateLobby(ELobbyType.k_ELobbyTypePublic, lobbyCapacity, $"###Failed to enter targetLobbyId={targetLobbyId} at battleState={battleState} due to {resp}###");
        }
    }

    protected bool shouldRejectAsOwner(in uint fromJoinIndex, in CSteamID fromMemberSteamID, in long battleState, in string callStackHint) {
        if (m_isCurrentLobbyOwner
            &&
            (
            PbPrimitivesOverride.ROOM_STATE_PREPARE == battleState
            ||
            PbPrimitivesOverride.ROOM_STATE_IN_BATTLE == battleState
            )
        ) {
            if (PbPrimitivesOverride.Instance.getUnderlying().MagicJoinIndexInvalid == fromJoinIndex) {
                Debug.LogWarning($"[I am OWNER] {callStackHint} incoming {fromMemberSteamID} is not a valid rejoin member in lobby {m_currentLobbyId} at battleState={battleState}, will ignore");
                return true;
            }
        }

        return false;
    }

    protected void onLobbyDataUpdated(LobbyDataUpdate_t callback) {
        CSteamID incomingLobbyId = new CSteamID(callback.m_ulSteamIDLobby);
        if (incomingLobbyId != m_currentLobbyId) {
            // Invalid incoming message.
            Debug.LogWarning($"Received invalid LobbyDataUpdate_t from incomingLobbyId={incomingLobbyId}");
            return;
        }

        ulong fromUllSteamID = callback.m_ulSteamIDMember;
        CSteamID fromMemberSteamID = new CSteamID(fromUllSteamID);
        uint fromPeerJoinIndex = GetJoinIndexInLobby(fromUllSteamID);
        var battleState = map.GetBattleState();

        bool notFromLobby = (callback.m_ulSteamIDLobby != fromUllSteamID);

        if (PbPrimitivesOverride.ROOM_STATE_WAITING >= battleState) {
            int lobbyMembersCnt = SteamMatchmaking.GetNumLobbyMembers(incomingLobbyId);
            bool shouldStartMatch = false;
            if (m_isCurrentLobbyOwner && lobbyMembersCnt >= lobbyCapacity && notFromLobby) {
                Debug.Log($"[I am OWNER] Will detect whether match should start in LobbyDataUpdate_t from incomingLobbyId={incomingLobbyId}, fromMemberSteamID={fromMemberSteamID}, now lobbyMembersCnt={lobbyMembersCnt}, battleState={battleState}");

                int effLobbyMemberCnt = updateMemberBindings();
                if (null != map) {
                    map.OnLobbyMembersUpdatedWhenWaiting(m_lobbyMemberBindings, $"onLobbyDataUpdated@Case1");
                }
                if (effLobbyMemberCnt == lobbyMembersCnt) {
                    // [WARNING] DON'T check "shouldStartMatch" in "onLobbyChatUpdated", because we need players to set their chosen "chSpeciesId"s before locking and starting the match.
                    shouldStartMatch = true;
                }

                if (!shouldStartMatch) {
                    Debug.LogWarning($"[I am OWNER] In LobbyDataUpdate_t from incomingLobbyId={incomingLobbyId}, now lobbyMembersCnt={lobbyMembersCnt} but effLobbyMemberCnt={effLobbyMemberCnt}");
                } else {
                    lockedLobbyMemberBindings = new SteamBinding[lobbyCapacity];
                    lockedMemberIdentities = new SteamNetworkingIdentity[lobbyCapacity];
                    lockedLobbyMemberUlSteamIdToJoinIndex = new Dictionary<ulong, uint>();
                    Debug.Log($"[I am OWNER] About to start match in incomingLobbyId={incomingLobbyId} with");
                    for (int i = 0; i < lobbyCapacity; i++) {
                        lockedLobbyMemberBindings[i] = m_lobbyMemberBindings[i].Clone();
                        lockedMemberIdentities[i] = new SteamNetworkingIdentity();
                        lockedMemberIdentities[i].SetSteamID64(lockedLobbyMemberBindings[i].UlSteamId);
                        lockedLobbyMemberUlSteamIdToJoinIndex[lockedLobbyMemberBindings[i].UlSteamId] = (uint)i + 1u;
                        Debug.Log($"\t`lockedLobbyMemberBindings` peerJoinIndex={i+1}, chSpeciesId={lockedLobbyMemberBindings[i].ChSpeciesId}");
                    }
                    Debug.Log($"[I am OWNER] Updated `lockedLobbyMemberBindings`, will start the match in LobbyDataUpdate_t from incomingLobbyId={incomingLobbyId}, now lobbyMembersCnt={lobbyMembersCnt}");
                }
            }

            if (shouldStartMatch) {
                // [REMINDER] From now on, "lockedLobbyMemberBindings" is locked from "lobby owner" perspective and broadcasted to every other peer, i.e. other peers SHOULDN'T fetch "lockedLobbyMemberBindings" by themselves after reception of "DaBattlePrepare" to avoid misalignment with "lobby owner".
                DownsyncSnapshot prepareSignal = new DownsyncSnapshot {
                    Act = DownsyncAct.DaBattlePrepare,
                    PrepareInfo = new BattlePrepareInfo {
                        StageName = "JoltOnlinePlayground", // [TODO] Don't hardcode
                        FrameLogEnabled = false,
                    }
                };
                prepareSignal.PeerSteamBindingList.AddRange(lockedLobbyMemberBindings);
                byte[] prepareSignalBytes = prepareSignal.ToByteArray();

                SteamMatchmaking.SendLobbyChatMsg(m_currentLobbyId, prepareSignalBytes, prepareSignalBytes.Length);
                Debug.Log($"[I am OWNER] LobbyDataUpdate_t sent via LobbyChat prepareSignalBytes.Length={prepareSignalBytes.Length}, prepareSignal.PeerSteamBindingList.Count={prepareSignal.PeerSteamBindingList.Count} to current lobby from incomingLobbyId={incomingLobbyId}");
            } else {
                if (notFromLobby) {
                    string memberPersonaName = SteamFriends.GetFriendPersonaName(fromMemberSteamID);
                    //Debug.Log($"Received member-type LobbyDataUpdate_t from incomingLobbyId={incomingLobbyId} of fromMemberSteamID={fromMemberSteamID}, memberPersonaName={memberPersonaName}, now lobbyMembersCnt={lobbyMembersCnt}");
                    int effLobbyMemberCnt = updateMemberBindings();
                    if (null != map) {
                        map.OnLobbyMembersUpdatedWhenWaiting(m_lobbyMemberBindings, $"onLobbyDataUpdated@Case2");
                    }
                } else {
                    //Debug.Log($"Received lobby-type LobbyDataUpdate_t from incomingLobbyId={incomingLobbyId}, now lobbyMembersCnt={lobbyMembersCnt}");
                }
            }
        } else {
            if (notFromLobby) {
                if (!shouldRejectAsOwner(fromPeerJoinIndex, fromMemberSteamID, battleState, "onLobbyDataUpdated")) {
                    //Debug.Log($"Received member-type LobbyDataUpdate_t from incomingLobbyId={incomingLobbyId} of fromMemberSteamID={fromMemberSteamID}, isCurrentLobbyOwner={m_isCurrentLobbyOwner}, at battleState={battleState}");
                    if (
                        RemoveDisconnectedRecord(fromPeerJoinIndex, $"onLobbyDataUpdated/battleState={battleState}") // [REMINDER] To stop the current owner from force-confirming inputs for this just rejoined peer
                    ) {
                        map.OnLobbyMemberRejoined(fromPeerJoinIndex);
                    }
                }
            } else {
                //Debug.Log($"Received lobby-type LobbyDataUpdate_t from incomingLobbyId={incomingLobbyId}, at battleState={battleState}");
            }
        }
    }

    protected void onLobbyKicked(LobbyKicked_t callback) {
        CSteamID incomingLobbyId = new CSteamID(callback.m_ulSteamIDLobby);
        if (incomingLobbyId != m_currentLobbyId) {
            // Invalid incoming message.
            Debug.LogWarning($"Received invalid LobbyKicked_t from incomingLobbyId={incomingLobbyId}");
            return;
        }
        CSteamID adminSteamID = new CSteamID(callback.m_ulSteamIDAdmin);

        Debug.Log($"You're kicked out of lobby {m_currentLobbyId} by adminSteamID={adminSteamID}");
        if (null != map) {
            map.OnSelfKickedOutOfLobby();
        }
    }

    protected void onLobbyChatUpdated(LobbyChatUpdate_t callback) {
        CSteamID incomingLobbyId = new CSteamID(callback.m_ulSteamIDLobby);
        if (incomingLobbyId != m_currentLobbyId) {
            // Invalid incoming message.
            Debug.LogWarning($"Received invalid LobbyChatUpdate_t from incomingLobbyId={incomingLobbyId}");
            return;
        }

        ulong fromUllSteamID = callback.m_ulSteamIDUserChanged;
        CSteamID fromMemberSteamID = new CSteamID(fromUllSteamID);
        uint fromPeerJoinIndex = GetJoinIndexInLobby(fromUllSteamID);
        var battleState = map.GetBattleState();
        if (shouldRejectAsOwner(fromPeerJoinIndex, fromMemberSteamID, battleState, "onLobbyChatUpdated")) {
            return;
        }

        int lobbyMembersCnt = SteamMatchmaking.GetNumLobbyMembers(incomingLobbyId);
        CSteamID makingChangeUserSteamID = new CSteamID(callback.m_ulSteamIDMakingChange);
        EChatMemberStateChange memberStateChange = (EChatMemberStateChange)((int)callback.m_rgfChatMemberStateChange);

        //Debug.Log($"Received LobbyChatUpdate_t from incomingLobbyId={incomingLobbyId} of changedUserSteamID={fromMemberSteamID}, makingChangeUserSteamID={makingChangeUserSteamID}, memberStateChange={memberStateChange}, now lobbyMembersCnt={lobbyMembersCnt}, battleState={battleState}");

        if (EChatMemberStateChange.k_EChatMemberStateChangeLeft == memberStateChange ||
            EChatMemberStateChange.k_EChatMemberStateChangeDisconnected == memberStateChange) {
            // Fetch the new lobby owner
            var effLobbyOwnerSteamID = SteamMatchmaking.GetLobbyOwner(m_currentLobbyId);
            if (m_currentLobbyOwnerId != effLobbyOwnerSteamID) {
                if (CSteamID.Nil == effLobbyOwnerSteamID) {
                    // Meaning "SteamMatchmaking.GetLobbyOwner(...)" failed, possibly due to self network disconnected, don't reset lobby information immediately in case a re-connect action is coming.
                } else {
                    m_currentLobbyOwnerId = effLobbyOwnerSteamID;
                    if (effLobbyOwnerSteamID == SteamUser.GetSteamID()) {
                        m_isCurrentLobbyOwner = true;
                        Debug.Log($"[I am OWNER] Host left. New lobby owner: {effLobbyOwnerSteamID} of lobbyId={m_currentLobbyId}, battleState={battleState}, I'm the new owner now!");
                    } else {
                        Debug.Log($"Host left. New lobby owner: {effLobbyOwnerSteamID} of lobbyId={m_currentLobbyId}, battleState={battleState}");
                    }
                }
            }

            if (fromPeerJoinIndex == map.GetSelfJoinIndex()) {
                map.OnLobbySelfLeft($"onLobbyChatUpdated/{memberStateChange}");
            } else {
                if (AddDisconnectedRecord(fromPeerJoinIndex, $"onLobbyChatUpdated/{memberStateChange}")) {
                    map.OnLobbyPeerLeft(fromPeerJoinIndex, $"onLobbyChatUpdated/{memberStateChange}");
                }
            }
        } else if (EChatMemberStateChange.k_EChatMemberStateChangeEntered == memberStateChange) {
            // Intentionally left blank, relevant logic is in "onLobbyEntered".
        }
    }

    protected void onLobbyChatMsg(LobbyChatMsg_t callback) {
        CSteamID incomingLobbyId = new CSteamID(callback.m_ulSteamIDLobby);
        if (incomingLobbyId != m_currentLobbyId) {
            // Invalid incoming message.
            Debug.LogWarning($"Received invalid LobbyChatMsg_t from incomingLobbyId={incomingLobbyId}");
            return;
        }

        ulong fromUllSteamID = callback.m_ulSteamIDUser;
        CSteamID fromMemberSteamID = new CSteamID(fromUllSteamID);
        if (m_currentLobbyOwnerId != fromMemberSteamID) {
            //Debug.Log($"Rejected LobbyChatMsg_t in incomingLobbyId={incomingLobbyId} of fromMemberSteamID={fromMemberSteamID}, chatID={callback.m_iChatID}");
            return;
        }
        EChatEntryType chatEntryType;
        int nBytes = SteamMatchmaking.GetLobbyChatEntry(incomingLobbyId, (int)callback.m_iChatID, out CSteamID pSteamIDUser, chatRecvBuff, PbPrimitivesOverride.Instance.getUnderlying().FrontendWsRecvBytelength, out chatEntryType);
        byte[] copiedDownsyncSnapshot = new byte[nBytes];
        Buffer.BlockCopy(chatRecvBuff, 0, copiedDownsyncSnapshot, 0, copiedDownsyncSnapshot.Length);

        //Debug.Log($"Received LobbyChatMsg_t in incomingLobbyId={incomingLobbyId} of fromMemberSteamID={fromMemberSteamID}, chatID={callback.m_iChatID}, nBytes={nBytes}, chatEntryType={chatEntryType}");

        localDownsyncSnapshotBytesBuffer.Enqueue(copiedDownsyncSnapshot);
    }

    protected void closeSessionsWithLockedLobbyMembers(in string motivation) {
        if (null == lockedMemberIdentities) {
            return;
        }
        for (int i = 0; i < lockedMemberIdentities.Length; i++) {
            var single = lockedMemberIdentities[i];
            if (single.GetSteamID() == SteamUser.GetSteamID()) {
                continue;
            }
            if (single.GetSteamID() == CSteamID.Nil) {
                continue;
            }
            SteamNetworkingMessages.CloseSessionWithUser(ref single);
            Debug.Log($"Closed SteamNetworkingMessages session with {single.GetSteamID()}: {motivation}");
        }
    }

    protected void onSessionRequest(SteamNetworkingMessagesSessionRequest_t callback) {
        SteamNetworkingIdentity remoteUser = callback.m_identityRemote;
        ulong peerUlSteamID = remoteUser.GetSteamID64();
        bool shouldAccept = false;
        var battleState = map.GetBattleState();
        if (
            PbPrimitivesOverride.ROOM_STATE_PREPARE == battleState
            ||
            PbPrimitivesOverride.ROOM_STATE_IN_BATTLE == battleState
            ) {
            foreach (var single in lockedLobbyMemberBindings) {
                if (single.UlSteamId == peerUlSteamID) {
                    shouldAccept = true;
                    break;
                }
            }
        } else {
            // [REMINDER] DON'T use "lockedLobbyMemberBindings" in this case, because a "Lobby non-owner" might not have collected locked information by now
            updateMemberBindings();
            foreach (var single in m_lobbyMemberBindings) {
                if (single.UlSteamId == peerUlSteamID) {
                    shouldAccept = true;
                    break;
                }
            }
        }
        
        if (shouldAccept && SteamNetworkingMessages.AcceptSessionWithUser(ref remoteUser)) {
            // Accepted the session. This accepts ALL incoming packet types from this user.         
            uint peerJoinIndex = GetJoinIndexInLobby(peerUlSteamID);
            RemoveDisconnectedRecord(peerJoinIndex, $"onSessionRequest/peerUlSteamID={peerUlSteamID}&battleState={battleState}");
        } else {
            Debug.Log($"Rejected network session with: {peerUlSteamID} at battleState={battleState}");
        }
    }

    protected void onSessionFailed(SteamNetworkingMessagesSessionFailed_t callback) {
        var connInfo = callback.m_info;
        var disconnectedFromPeerUlSteamID = connInfo.m_identityRemote.GetSteamID64();
        if (null != lockedLobbyMemberUlSteamIdToJoinIndex && lockedLobbyMemberUlSteamIdToJoinIndex.ContainsKey(disconnectedFromPeerUlSteamID) && null != map) {
            uint disconnectedFromPeerJoinIndex = lockedLobbyMemberUlSteamIdToJoinIndex[disconnectedFromPeerUlSteamID];
            //Debug.Log($"Session failed with: remoteUser={disconnectedFromPeerUlSteamID}, disconnectedFromPeerJoinIndex={disconnectedFromPeerJoinIndex}, POPRemote={connInfo.m_idPOPRemote}, POPRelay={connInfo.m_idPOPRelay}, reason={connInfo.m_eEndReason}");
            map.OnSessionWithPeerFailed(disconnectedFromPeerUlSteamID, disconnectedFromPeerJoinIndex);
        } else {
            Debug.Log($"Session failed with: remoteUser={connInfo.m_identityRemote.GetSteamID()}, POPRemote={connInfo.m_idPOPRemote}, POPRelay={connInfo.m_idPOPRelay}, reason={connInfo.m_eEndReason}");
        }
    }

    protected void onConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t callback) {
        var conn = callback.m_hConn;
        var connInfo = callback.m_info;
        var oldEState = callback.m_eOldState;
        var newEState = connInfo.m_eState;
        var fromPeerUlSteamID = connInfo.m_identityRemote.GetSteamID64();
        if (null != lockedLobbyMemberUlSteamIdToJoinIndex && lockedLobbyMemberUlSteamIdToJoinIndex.ContainsKey(fromPeerUlSteamID) && null != map) {
            uint peerJoinIndex = lockedLobbyMemberUlSteamIdToJoinIndex[fromPeerUlSteamID];
            var battleState = map.GetBattleState();
           
            Debug.Log($"onConnectionStatusChanged: fromPeerUlSteamID={fromPeerUlSteamID}, peerJoinIndex={peerJoinIndex}, battleState={battleState}, oldEState={oldEState}, newEState={newEState}, POPRemote={connInfo.m_idPOPRemote}, POPRelay={connInfo.m_idPOPRelay}, reason={connInfo.m_eEndReason}");
        } else {
            Debug.Log($"onConnectionStatusChanged: remoteUser={fromPeerUlSteamID}, oldEState={oldEState}, newEState={newEState}, POPRemote={connInfo.m_idPOPRemote}, POPRelay={connInfo.m_idPOPRelay}, reason={connInfo.m_eEndReason}");
        }
    }

    protected static SteamP2PSessionManager _instance;
    protected static readonly object _padLock = new object();

    protected SteamP2PSessionManager() {
        if (!SteamManager.Initialized) {
            Debug.LogError("Steamworks not initialized.");
            return;
        }

        senderBuffer = new BlockingCollection<byte[]>();
        ownerSignalSenderBuffer = new BlockingCollection<byte[]>();
        localDownsyncSnapshotBytesBuffer = new Queue<byte[]>();
        localUpsyncSnapshotBytesBuffer = new Queue<byte[]>();

        m_LobbyMatchList = CallResult<LobbyMatchList_t>.Create(onLobbyMatchList);
    }

    protected void onLobbyCreated(LobbyCreated_t callback) {
        if (callback.m_eResult == EResult.k_EResultOK) {
            m_currentLobbyId = new CSteamID(callback.m_ulSteamIDLobby);
            m_currentLobbyOwnerId = SteamUser.GetSteamID();
            m_isCurrentLobbyOwner = true;
            
            Debug.Log($"Lobby created successfully! ID: {m_currentLobbyId} with owner steam id={m_currentLobbyOwnerId}");

            // Set metadata variables so other clients can search/identify your Lobby
            SteamMatchmaking.SetLobbyData(m_currentLobbyId, GDK_HOST_PLAYER_NAME, SteamFriends.GetPersonaName());
            SteamMatchmaking.SetLobbyData(m_currentLobbyId, GDK_GAME_NAME_AND_VER, $"{Application.productName}_{Application.version}");
            if (null != map) {
                map.OnLobbyCreated();
            }
        } else {
            Debug.LogError($"Steam rejected lobby creation request. Error code: {callback.m_eResult}");
            if (null != map) {
                map.OnLobbyCreateFailed();
            }
        }
    }

    protected void clearPassiveCallbacks() {
        if (null != m_LobbyCreated) {
            m_LobbyCreated.Unregister();
            m_LobbyCreated.Dispose();
            m_LobbyCreated = null;
        }
        if (null != m_LobbyEntered) {
            m_LobbyEntered.Unregister();
            m_LobbyEntered.Dispose();
            m_LobbyEntered = null;
        }
        if (null != m_LobbyDataUpdated) {
            m_LobbyDataUpdated.Unregister();
            m_LobbyDataUpdated.Dispose();
            m_LobbyDataUpdated = null;
        }
        if (null != m_LobbyKicked) {
            m_LobbyKicked.Unregister();
            m_LobbyKicked.Dispose();
            m_LobbyKicked = null;
        }
        if (null != m_LobbyChatUpdated) {
            m_LobbyChatUpdated.Unregister();
            m_LobbyChatUpdated.Dispose();
            m_LobbyChatUpdated = null;
        }
        if (null != m_LobbyChatMsg) {
            m_LobbyChatMsg.Unregister();
            m_LobbyChatMsg.Dispose();
            m_LobbyChatMsg = null;
        }
        if (null != m_SessionRequestCallback) {
            m_SessionRequestCallback.Unregister();
            m_SessionRequestCallback.Dispose();
            m_SessionRequestCallback = null;
        }
        if (null != m_SessionFailedCallback) {
            m_SessionFailedCallback.Unregister();
            m_SessionFailedCallback.Dispose();
            m_SessionFailedCallback = null;
        }
        if (null != m_connectionStatusChangedCallback) {
            m_connectionStatusChangedCallback.Unregister();
            m_connectionStatusChangedCallback.Dispose();
            m_connectionStatusChangedCallback = null;
        }
    }

    protected void setPassiveCallbacks() {
        m_LobbyCreated = Callback<LobbyCreated_t>.Create(onLobbyCreated);
        m_LobbyEntered = Callback<LobbyEnter_t>.Create(onLobbyEntered);

        m_LobbyDataUpdated = Callback<LobbyDataUpdate_t>.Create(onLobbyDataUpdated); // [REMINDER] "Callback<T>.Create" implicitly registers it within "SteamWorks.NET" singleton(s).
        m_LobbyKicked = Callback<LobbyKicked_t>.Create(onLobbyKicked);
        m_LobbyChatUpdated = Callback<LobbyChatUpdate_t>.Create(onLobbyChatUpdated);
        m_LobbyChatMsg = Callback<LobbyChatMsg_t>.Create(onLobbyChatMsg);

        m_SessionRequestCallback = Callback<SteamNetworkingMessagesSessionRequest_t>.Create(onSessionRequest);
        m_SessionFailedCallback = Callback<SteamNetworkingMessagesSessionFailed_t>.Create(onSessionFailed);

        m_connectionStatusChangedCallback = Callback<SteamNetConnectionStatusChangedCallback_t>.Create(onConnectionStatusChanged);
    }

    protected void ownerSignalSend(CancellationToken sessionCancellationToken) {
        // [REMINDER] Only "lobby owner" in realtime manner will be using this task (i.e. respecting migration of "lobby owner").
        Debug.Log($"Starts p2pSession 'ownerSignalSend' loop, now ownerSignalSenderBuffer.Count={ownerSignalSenderBuffer.Count}");
        byte[] toSendBuffer;
        try {
            while (!sessionCancellationToken.IsCancellationRequested) {
                if (ownerSignalSenderBuffer.TryTake(out toSendBuffer, sendBufferReadTimeoutMillis, sessionCancellationToken)) {
                    unsafe {
                        fixed (byte* pArray = toSendBuffer) {
                            //Debug.Log($"In 'ownerSignalSend' loop, sending to {lockedLobbyMemberBindings.Length} recipients including self.");
                            for (int i = lockedLobbyMemberBindings.Length - 1; i >= 0; i--) {
                                SteamNetworkingIdentity memberSteamId = lockedMemberIdentities[i];
                                if (SteamUser.GetSteamID() == memberSteamId.GetSteamID()) {
                                    // [WARNING] DON'T apply "SteamNetworkingMessages.SendMessageToUser" to self, in practice I got the following exception and crashed (i.e. "src\steamnetworkingsockets\clientlib\steamnetworkingsockets_p2p_ice.cpp (823) : Assertion Failed: We gathered candidate type 0x400, but 0x202 is allowed").
                                    localDownsyncSnapshotBytesBuffer.Enqueue(toSendBuffer);
                                } else {
                                    uint joinIndex = (uint)i + 1;
                                    if (disconnectedPeerJoinIndices.Contains(joinIndex)) {
                                        continue;
                                    }
                                    //Debug.Log($"Sending DOWNSYNC_SNAPSHOT_IO_CHANNEL message from steamId={SteamUser.GetSteamID()}, to memberSteamId={memberSteamId.GetSteamID()}...");
                                    SteamNetworkingMessages.SendMessageToUser(
                                        ref memberSteamId,
                                        (IntPtr)pArray,
                                        (uint)toSendBuffer.Length,
                                        (Steamworks.Constants.k_nSteamNetworkingSend_Reliable | Steamworks.Constants.k_nSteamNetworkingSend_AutoRestartBrokenSession),
                                        DOWNSYNC_SNAPSHOT_IO_CHANNEL
                                    ); //  [WARNING] This is synchronous, hence we need wrap "ownerSignalSend" as "Task.Run(() => ownerSignalSend(...))"

                                    //Debug.Log($"Sent DOWNSYNC_SNAPSHOT_IO_CHANNEL message from steamId={SteamUser.GetSteamID()}, to memberSteamId={memberSteamId.GetSteamID()}");
                                }
                            }
                        }
                    }
                }
            }
        } catch (ObjectDisposedException ex1) {

        } catch (Exception ex) {
            Debug.LogWarning($"p2pSession is stopping for 'ownerSignalSend' upon exception @localSeqNo={localSeqNo}; ex={ex}");
        } finally {
            while (senderBuffer.TryTake(out _, sendBufferReadTimeoutMillis, sessionCancellationToken)) { }
            Debug.Log($"Ends p2pSession 'ownerSignalSend' loop @localSeqNo={localSeqNo}");
        }
    }

    protected void send(CancellationToken sessionCancellationToken) {
        Debug.Log($"Starts p2pSession 'send' loop, now senderBuffer.Count={senderBuffer.Count}");
        byte[] toSendBuffer;
        try {
            while (!sessionCancellationToken.IsCancellationRequested) {
                if (senderBuffer.TryTake(out toSendBuffer, sendBufferReadTimeoutMillis, sessionCancellationToken)) {
                    unsafe {
                        fixed (byte* pArray = toSendBuffer) {
                            for (int i = 0; i < lockedLobbyMemberBindings.Length; i++) {
                                SteamNetworkingIdentity memberSteamId = lockedMemberIdentities[i];
                                if (memberSteamId.GetSteamID() == SteamUser.GetSteamID()) {
                                    localUpsyncSnapshotBytesBuffer.Enqueue(toSendBuffer);
                                } else {
                                    uint joinIndex = (uint)i + 1;
                                    if (disconnectedPeerJoinIndices.Contains(joinIndex)) {
                                        continue;
                                    }
                                    SteamNetworkingMessages.SendMessageToUser(
                                        ref memberSteamId,
                                        (IntPtr)pArray,
                                        (uint)toSendBuffer.Length,
                                        /*
                                           [REMINDER]

                                           We used "last-consecutively-all-confirmed-InputFrameDownsync-id (i.e. `FrontendBattle.lcacIfdId`, `FrontendBattle.udpLcacIfdId`)" for lag recognition, hence a "reliable-UDP (i.e. k_nSteamNetworkingSend_Reliable)" is most preferred for overall performance.

                                           When using "dedicated-server as both RenderFrame-authority and UDP-relay", it's easier to get good PvP performance because the TCP delay is so small (often < 128ms) and "raw-UDP" packet-loss-rate is so low such that the "raw-UDP" caveats of high packet-loss-rate or delay-surge are seldom exposed -- backthen I didn't have a convenient "reliable-UDP" option either, otherwise it'd have been a better choice over "raw-UDP" too. 

                                           Moreover, we're intentionally NOT using the "k_nSteamNetworkingSend_NoDelay" flag to improve success rate.
                                         */
                                        (Steamworks.Constants.k_nSteamNetworkingSend_Reliable | Steamworks.Constants.k_nSteamNetworkingSend_AutoRestartBrokenSession | Steamworks.Constants.k_nSteamNetworkingSend_NoNagle),  
                                        UPSYNC_SNAPSHOT_IO_CHANNEL
                                    ); // [WARNING] This is synchronous, hence we need wrap "send" as "Task.Run(() => send(...))"
                                }
                            }
                        }
                    }
                }
            }
        } catch (ObjectDisposedException ex1) {

        } catch (Exception ex) {
            Debug.LogWarning($"p2pSession is stopping for 'send' upon exception @localSeqNo={localSeqNo}; ex={ex}");
        } finally {
            while (senderBuffer.TryTake(out _, sendBufferReadTimeoutMillis, sessionCancellationToken)) { }
            Debug.Log($"Ends p2pSession 'send' loop @localSeqNo={localSeqNo}");
        }
    }

    ~SteamP2PSessionManager() {
        if (null != senderBuffer) {
            senderBuffer.Dispose();
            senderBuffer = null;
        }
        if (null != ownerSignalSenderBuffer) {
            ownerSignalSenderBuffer.Dispose();
            ownerSignalSenderBuffer = null;
        }
        clearPassiveCallbacks();
        if (null != m_LobbyMatchList) {
            m_LobbyMatchList.Dispose();
            m_LobbyMatchList = null;
        }

        Debug.Log($"~SteamP2PSessionManager done");
    }
}
