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

    public void JoinTargetLobby(CSteamID targetLobbyId, in string motivation) {
        Debug.Log($"Joining targetLobbyId={targetLobbyId} with motivation={motivation}...");
        SteamMatchmaking.JoinLobby(targetLobbyId);
    }

    public void LeaveCurrentLobbyReentrantSafe() {
        if (CSteamID.Nil == m_currentLobbyId) {
            return;
        }

        if (null != lockedMemberIdentities) {
            for (int i = 0; i < lockedMemberIdentities.Length; i++) {
                SteamNetworkingIdentity remoteUser = lockedMemberIdentities[i];
                if (remoteUser.GetSteamID() == SteamUser.GetSteamID()) {
                    continue;
                }
                if (SteamNetworkingMessages.CloseSessionWithUser(ref remoteUser)) {
                    Debug.Log($"LeaveCurrentLobbyReentrantSafe, closed session with remoteUser={remoteUser.GetSteamID()}.");
                }
            }
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

    public BlockingCollection<byte[]> senderBuffer;
    public BlockingCollection<byte[]> ownerSignalSenderBuffer;
    public Queue<byte[]> localDownsyncSnapshotBytesBuffer;
    public Queue<byte[]> localUpsyncSnapshotBytesBuffer;

    private int sendBufferReadTimeoutMillis = 512;
    private uint localSeqNo = 0;

    public void ResetP2PSession(in SteamOnlineMapController theMap, in string motivation) {
        map = theMap;
        localSeqNo = 0;

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
    private SteamOnlineMapController map;
    private CallResult<LobbyMatchList_t> m_LobbyMatchList;

    private Callback<LobbyCreated_t> m_LobbyCreated;
    private Callback<LobbyEnter_t> m_LobbyEntered;

    private Callback<LobbyDataUpdate_t> m_LobbyDataUpdated;
    private Callback<LobbyKicked_t> m_LobbyKicked;

    private Callback<LobbyChatUpdate_t> m_LobbyChatUpdated;
    private Callback<LobbyChatMsg_t> m_LobbyChatMsg;

    private Callback<SteamNetworkingMessagesSessionRequest_t> m_SessionRequestCallback;
    private Callback<SteamNetworkingMessagesSessionFailed_t> m_SessionFailedCallback;
    private Callback<SteamNetConnectionStatusChangedCallback_t> m_connectionStatusChangedCallback;

    private CSteamID m_currentLobbyId = CSteamID.Nil;
    private CSteamID m_currentLobbyOwnerId = CSteamID.Nil;
    private bool m_isCurrentLobbyOwner = false;
    private string GDK_HOST_PLAYER_NAME = "HPN";
    private string GDK_GAME_NAME_AND_VER = "GNV";
    private string GDK_CH_SPECIES = "CHS";
    private int lobbyCapacity = 2;
    private SteamBinding[] m_lobbyMemberBindings;
    private SteamBinding[] lockedLobbyMemberBindings;
    private SteamNetworkingIdentity[] lockedMemberIdentities;
    private Dictionary<ulong, uint> lockedLobbyMemberUlSteamIdToJoinIndex;
    private HashSet<uint> disconnectedPeerJoinIndices = new HashSet<uint>();
    public HashSet<uint> GetDisconnectedPeerJoinIndices() {
        return disconnectedPeerJoinIndices;
    }

    public bool RemoveDisconnectedRecord(uint joinIndex) {
        if (disconnectedPeerJoinIndices.Contains(joinIndex)) {
            disconnectedPeerJoinIndices.Remove(joinIndex);
            if (null != map) {
                map.OnLobbyMemberRejoined(joinIndex);
            }
            return true;
        }
        return false;
    }

    private byte[] chatRecvBuff = new byte[PbPrimitivesOverride.Instance.getUnderlying().FrontendWsRecvBytelength];

    public void SetLockedLobbyMemberBindings(RepeatedField<SteamBinding> lockedLobbyMemberBindingsFromOwner) {
        lockedLobbyMemberBindings = new SteamBinding[lockedLobbyMemberBindingsFromOwner.Count];
        lockedMemberIdentities = new SteamNetworkingIdentity[lockedLobbyMemberBindingsFromOwner.Count];
        lockedLobbyMemberUlSteamIdToJoinIndex = new Dictionary<ulong, uint>();
        for (int i = 0; i < lockedLobbyMemberBindingsFromOwner.Count; i++) {
            lockedLobbyMemberBindings[i] = lockedLobbyMemberBindingsFromOwner[i].Clone();
            lockedMemberIdentities[i] = new SteamNetworkingIdentity();
            lockedMemberIdentities[i].SetSteamID64(lockedLobbyMemberBindings[i].UlSteamId);
            lockedLobbyMemberUlSteamIdToJoinIndex[lockedLobbyMemberBindings[i].UlSteamId] = (uint)(i + 1);
        }
        if (null != map) {
            map.OnLobbyMembersUpdated(lockedLobbyMemberBindings);
        }
    }

    private int updateMemberBindings() {
        int effLobbyMemberCnt = 0;
        m_lobbyMemberBindings = new SteamBinding[lobbyCapacity];
        int lobbyMembersCnt = CSteamID.Nil == m_currentLobbyId ? 0 : SteamMatchmaking.GetNumLobbyMembers(m_currentLobbyId);
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
            ++effLobbyMemberCnt;
        }

        for (int j = effLobbyMemberCnt; j < lobbyCapacity; j++) {
            m_lobbyMemberBindings[j] = new SteamBinding {
                UlSteamId = CSteamID.Nil.m_SteamID,
                ChSpeciesId = PbPrimitivesOverride.SPECIES_NONE_CH,
            };
        }

        return effLobbyMemberCnt;
    }

    private void onLobbyMatchList(LobbyMatchList_t callback, bool bIOFailure) {
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

    private void onLobbyEntered(LobbyEnter_t callback) {
        EChatRoomEnterResponse resp = (EChatRoomEnterResponse)((int)callback.m_EChatRoomEnterResponse);

        CSteamID targetLobbyId = new CSteamID(callback.m_ulSteamIDLobby);
        if (EChatRoomEnterResponse.k_EChatRoomEnterResponseSuccess == resp) {
            m_currentLobbyId = targetLobbyId;

            // Hand off to your underlying P2P Network Transport layer here
            // The host's SteamID can be discovered by querying the lobby owner
            CSteamID lobbyOwner = SteamMatchmaking.GetLobbyOwner(m_currentLobbyId);
            m_currentLobbyOwnerId = lobbyOwner;
            int lobbyMembersCnt = SteamMatchmaking.GetNumLobbyMembers(targetLobbyId);

            if (m_isCurrentLobbyOwner) {
                Debug.Log($"Entered as Owner of lobby: {m_currentLobbyId}, now lobbyMembersCnt={lobbyMembersCnt}");
            } else {
                Debug.Log($"Entered as a Participant of lobby: {m_currentLobbyId} owned by {m_currentLobbyOwnerId}, now lobbyMembersCnt={lobbyMembersCnt}");
            }

            SteamMatchmaking.SetLobbyMemberData(m_currentLobbyId, GDK_CH_SPECIES, JoltWsSessionManager.Instance.GetSpeciesId().ToString());
            if (null != map) {
                map.OnLobbyEntered();
            }
        } else {
            CreateLobby(ELobbyType.k_ELobbyTypePublic, lobbyCapacity, $"###Failed to enter targetLobbyId={targetLobbyId} due to {resp}###");
        }
    }

    private void onLobbyDataUpdated(LobbyDataUpdate_t callback) {
        CSteamID incomingLobbyId = new CSteamID(callback.m_ulSteamIDLobby);
        if (incomingLobbyId != m_currentLobbyId) {
            // Invalid incoming message.
            Debug.LogWarning($"Received invalid LobbyDataUpdate_t from incomingLobbyId={incomingLobbyId}");
            return;
        }

        int lobbyMembersCnt = SteamMatchmaking.GetNumLobbyMembers(incomingLobbyId);
        bool shouldStartMatch = false;
        if (m_isCurrentLobbyOwner && lobbyMembersCnt >= lobbyCapacity && callback.m_ulSteamIDLobby != callback.m_ulSteamIDMember) {
            Debug.Log($"Will lock and detect whether match should start in LobbyDataUpdate_t from incomingLobbyId={incomingLobbyId}, now lobbyMembersCnt={lobbyMembersCnt}");

            SteamMatchmaking.SetLobbyJoinable(m_currentLobbyId, false);
            int effLobbyMemberCnt = updateMemberBindings();
            if (null != map) {
                map.OnLobbyMembersUpdated(m_lobbyMemberBindings);
            }
            if (effLobbyMemberCnt == lobbyMembersCnt) {
                // [WARNING] DON'T check "shouldStartMatch" in "onLobbyChatUpdated", because we need players to set their chosen "chSpeciesId"s before locking and starting the match.
                shouldStartMatch = true;
            }

            if (!shouldStartMatch) {
                Debug.LogWarning($"Will unlock in LobbyDataUpdate_t from incomingLobbyId={incomingLobbyId}, now lobbyMembersCnt={lobbyMembersCnt} but effLobbyMemberCnt={effLobbyMemberCnt}");
                SteamMatchmaking.SetLobbyJoinable(m_currentLobbyId, true);
            } else {
                lockedLobbyMemberBindings = new SteamBinding[lobbyCapacity];
                lockedMemberIdentities = new SteamNetworkingIdentity[lobbyCapacity];
                lockedLobbyMemberUlSteamIdToJoinIndex = new Dictionary<ulong, uint>();
                Debug.Log($"About to start match in incomingLobbyId={incomingLobbyId} with");
                for (int i = 0; i < lobbyCapacity; i++) {
                    lockedLobbyMemberBindings[i] = m_lobbyMemberBindings[i].Clone();
                    lockedMemberIdentities[i] = new SteamNetworkingIdentity();
                    lockedMemberIdentities[i].SetSteamID64(lockedLobbyMemberBindings[i].UlSteamId);
                    lockedLobbyMemberUlSteamIdToJoinIndex[lockedLobbyMemberBindings[i].UlSteamId] = (uint)i + 1u;
                    Debug.Log($"joinIndex={i + 1}: SteamID={lockedMemberIdentities[i].GetSteamID()}, ");
                }
                Debug.Log($"Updated `lockedLobbyMemberBindings`, will keep lobby locked and start the match in LobbyDataUpdate_t from incomingLobbyId={incomingLobbyId}, now lobbyMembersCnt={lobbyMembersCnt}");
            }
        }

        if (shouldStartMatch) {
            // [REMINDER] From now on, "lockedLobbyMemberBindings" is locked from "lobby owner" perspective and broadcasted to every other peer, i.e. other peers SHOULDN'T fetch "lockedLobbyMemberBindings" by themselves after reception of "DaBattlePrepare" to avoid misalignment with "lobby owner".
            for (int peerJoinIndexInt = 2; peerJoinIndexInt <= lockedLobbyMemberBindings.Length; peerJoinIndexInt++) {
                uint peerJoinIndex = (uint)peerJoinIndexInt;
                int peerJoinIndexArrIdx = peerJoinIndexInt - 1;
                SteamBinding lobbyMemberBinding = lockedLobbyMemberBindings[peerJoinIndexArrIdx];
                if (null == lobbyMemberBinding) {
                    Debug.LogWarning($"lobbyMemberBinding for lockedLobbyMemberBindings[peerJoinIndexArrIdx={peerJoinIndexArrIdx}] is null, skipping...");
                    continue;
                }
                DownsyncSnapshot prepareSignal = new DownsyncSnapshot {
                    Act = DownsyncAct.DaBattlePrepare,
                    PeerJoinIndex = peerJoinIndex,
                    PeerSpeciesId = lobbyMemberBinding.ChSpeciesId,
                    PrepareInfo = new BattlePrepareInfo {
                        StageName = "JoltOnlinePlayground", // [TODO] Don't hardcode
                        FrameLogEnabled = false,
                    }
                };
                prepareSignal.PeerSteamBindingList.AddRange(lockedLobbyMemberBindings);
                byte[] prepareSignalBytes = prepareSignal.ToByteArray();
                ownerSignalSenderBuffer.Add(prepareSignalBytes);
                Debug.Log($"LobbyDataUpdate_t buffered into ownerSignalSenderBuffer prepareSignalBytes.Length={prepareSignalBytes.Length} to current lobby from incomingLobbyId={incomingLobbyId}");
            }
        } else {
            if (callback.m_ulSteamIDLobby != callback.m_ulSteamIDMember) {
                CSteamID memberSteamID = new CSteamID(callback.m_ulSteamIDMember);
                string memberPersonaName = SteamFriends.GetFriendPersonaName(memberSteamID);
                Debug.Log($"Received member-type LobbyDataUpdate_t from incomingLobbyId={incomingLobbyId} of memberSteamID={memberSteamID}, memberPersonaName={memberPersonaName}, now lobbyMembersCnt={lobbyMembersCnt}");
                int effLobbyMemberCnt = updateMemberBindings();
                if (null != map) {
                    map.OnLobbyMembersUpdated(m_lobbyMemberBindings);
                }
            } else {
                //Debug.Log($"Received lobby-type LobbyDataUpdate_t from incomingLobbyId={incomingLobbyId}, now lobbyMembersCnt={lobbyMembersCnt}");
            }
        }
    }

    private void onLobbyKicked(LobbyKicked_t callback) {
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

    private void onLobbyChatUpdated(LobbyChatUpdate_t callback) {
        CSteamID incomingLobbyId = new CSteamID(callback.m_ulSteamIDLobby);
        if (incomingLobbyId != m_currentLobbyId) {
            // Invalid incoming message.
            Debug.LogWarning($"Received invalid LobbyChatUpdate_t from incomingLobbyId={incomingLobbyId}");
            return;
        }

        int lobbyMembersCnt = SteamMatchmaking.GetNumLobbyMembers(incomingLobbyId);
        CSteamID changedUserSteamID = new CSteamID(callback.m_ulSteamIDUserChanged);
        CSteamID makingChangeUserSteamID = new CSteamID(callback.m_ulSteamIDMakingChange);
        EChatMemberStateChange memberStateChange = (EChatMemberStateChange)((int)callback.m_rgfChatMemberStateChange);

        Debug.Log($"Received LobbyChatUpdate_t from incomingLobbyId={incomingLobbyId} of changedUserSteamID={changedUserSteamID}, makingChangeUserSteamID={makingChangeUserSteamID}, memberStateChange={memberStateChange}, now lobbyMembersCnt={lobbyMembersCnt}");

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
                        Debug.Log($"Host left. New lobby owner: {effLobbyOwnerSteamID}, I'm the new owner now!");
                    } else {
                        Debug.Log($"Host left. New lobby owner: {effLobbyOwnerSteamID}");
                    }
                }
            }

            if (null != lockedLobbyMemberUlSteamIdToJoinIndex && lockedLobbyMemberUlSteamIdToJoinIndex.ContainsKey(changedUserSteamID.m_SteamID) && null != map) {
                uint joinIndex = lockedLobbyMemberUlSteamIdToJoinIndex[changedUserSteamID.m_SteamID];
                if (joinIndex != map.GetSelfJoinIndex()) {
                    disconnectedPeerJoinIndices.Add(joinIndex);
                }
                map.OnLobbyMemberLeft(joinIndex, $"onLobbyChatUpdated/{memberStateChange}");
            }
        } else if (EChatMemberStateChange.k_EChatMemberStateChangeEntered == memberStateChange) {

        }
    }

    private void onLobbyChatMsg(LobbyChatMsg_t callback) {
        CSteamID incomingLobbyId = new CSteamID(callback.m_ulSteamIDLobby);
        if (incomingLobbyId != m_currentLobbyId) {
            // Invalid incoming message.
            Debug.LogWarning($"Received invalid LobbyChatMsg_t from incomingLobbyId={incomingLobbyId}");
            return;
        }

        CSteamID fromUserSteamID = new CSteamID(callback.m_ulSteamIDUser);

        EChatEntryType chatEntryType;
        int nBytes = SteamMatchmaking.GetLobbyChatEntry(incomingLobbyId, (int)callback.m_iChatID, out CSteamID pSteamIDUser, chatRecvBuff, PbPrimitivesOverride.Instance.getUnderlying().FrontendWsRecvBytelength, out chatEntryType);
        byte[] copiedDownsyncSnapshot = new byte[nBytes];
        Buffer.BlockCopy(chatRecvBuff, 0, copiedDownsyncSnapshot, 0, copiedDownsyncSnapshot.Length);

        Debug.Log($"Received LobbyChatMsg_t from incomingLobbyId={incomingLobbyId} of fromUserSteamID={fromUserSteamID}, chatID={callback.m_iChatID}, nBytes={nBytes}, chatEntryType={chatEntryType}");
    }

    private void onSessionRequest(SteamNetworkingMessagesSessionRequest_t callback) {
        SteamNetworkingIdentity remoteUser = callback.m_identityRemote;
        bool shouldAccept = false;
        updateMemberBindings();
        // [REMINDER] DON'T use "lockedLobbyMemberBindings" here, because a "Lobby non-owner" might not have collected locked information by now
        foreach (var single in m_lobbyMemberBindings) {
            if (single.UlSteamId == remoteUser.GetSteamID64()) {
                shouldAccept = true;
                break;
            }
        }
        if (shouldAccept && SteamNetworkingMessages.AcceptSessionWithUser(ref remoteUser)) {
            // Accepted the session. This accepts ALL incoming packet types from this user.
            Debug.Log($"Accepted network session with: {remoteUser.GetSteamID()}");
        } else {
            Debug.Log($"Rejected network session with: {remoteUser.GetSteamID()}");
        }
    }

    private void onSessionFailed(SteamNetworkingMessagesSessionFailed_t callback) {
        var connInfo = callback.m_info;

        if (m_currentLobbyOwnerId != connInfo.m_identityRemote.GetSteamID() && null != lockedLobbyMemberUlSteamIdToJoinIndex && lockedLobbyMemberUlSteamIdToJoinIndex.ContainsKey(connInfo.m_identityRemote.GetSteamID64()) && null != map) {
            uint nonOwnerJoinIndex = lockedLobbyMemberUlSteamIdToJoinIndex[connInfo.m_identityRemote.GetSteamID64()];
            Debug.Log($"Session failed with: remoteUser={connInfo.m_identityRemote.GetSteamID()}, nonOwnerJoinIndex={nonOwnerJoinIndex}, POPRemote={connInfo.m_idPOPRemote}, POPRelay={connInfo.m_idPOPRelay}, reason={connInfo.m_eEndReason}");
            disconnectedPeerJoinIndices.Add(nonOwnerJoinIndex);
            map.OnLobbyMemberLeft(nonOwnerJoinIndex, "onSessionFailed/withNonOwner");
        } else {
            Debug.Log($"Session failed with: remoteUser={connInfo.m_identityRemote.GetSteamID()}, POPRemote={connInfo.m_idPOPRemote}, POPRelay={connInfo.m_idPOPRelay}, reason={connInfo.m_eEndReason}");
        }
    }

    private void onConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t callback) {
        var connInfo = callback.m_info;
        if (null != lockedLobbyMemberUlSteamIdToJoinIndex && lockedLobbyMemberUlSteamIdToJoinIndex.ContainsKey(connInfo.m_identityRemote.GetSteamID64()) && null != map) {
            uint joinIndex = lockedLobbyMemberUlSteamIdToJoinIndex[connInfo.m_identityRemote.GetSteamID64()];
            var battleState = map.GetBattleState();
            Debug.LogWarning($"onConnectionStatusChanged: remoteUser={connInfo.m_identityRemote.GetSteamID()}, joinIndex={joinIndex}, battleState={battleState}, newEState={connInfo.m_eState}, POPRemote={connInfo.m_idPOPRemote}, POPRelay={connInfo.m_idPOPRelay}, reason={connInfo.m_eEndReason}");

            if (PbPrimitivesOverride.ROOM_STATE_IN_BATTLE == battleState) {
                if (ESteamNetworkingConnectionState.k_ESteamNetworkingConnectionState_ClosedByPeer == connInfo.m_eState
                || ESteamNetworkingConnectionState.k_ESteamNetworkingConnectionState_Dead == connInfo.m_eState
                || ESteamNetworkingConnectionState.k_ESteamNetworkingConnectionState_ProblemDetectedLocally == connInfo.m_eState
                || ESteamNetworkingConnectionState.k_ESteamNetworkingConnectionState_Linger == connInfo.m_eState
                || ESteamNetworkingConnectionState.k_ESteamNetworkingConnectionState_Connecting == connInfo.m_eState) {
                    
                    if (m_currentLobbyOwnerId != connInfo.m_identityRemote.GetSteamID()) {
                        if (joinIndex != map.GetSelfJoinIndex()) {
                            disconnectedPeerJoinIndices.Add(joinIndex);
                        }
                        map.OnLobbyMemberLeft(joinIndex, "onConnectionStatusChanged/withNonOwner");
                    } else if (m_isCurrentLobbyOwner) {
                        map.OnLobbyMemberLeft(joinIndex, "onConnectionStatusChanged/asOwner");
                    }
                }
            }
        } else {
            Debug.Log($"onConnectionStatusChanged: remoteUser={connInfo.m_identityRemote.GetSteamID()}, newEState={connInfo.m_eState}, POPRemote={connInfo.m_idPOPRemote}, POPRelay={connInfo.m_idPOPRelay}, reason={connInfo.m_eEndReason}");
        }
    }

    private static SteamP2PSessionManager _instance;
    private static readonly object _padLock = new object();

    private SteamP2PSessionManager() {
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

    private void onLobbyCreated(LobbyCreated_t callback) {
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

    private void clearPassiveCallbacks() {
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

    private void setPassiveCallbacks() {
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

    private void ownerSignalSend(CancellationToken sessionCancellationToken) {
        // [REMINDER] Only "lobby owner" in realtime manner will be using this task (i.e. respecting migration of "lobby owner").
        Debug.Log($"Starts p2pSession 'ownerSignalSend' loop, now ownerSignalSenderBuffer.Count={ownerSignalSenderBuffer.Count}");
        byte[] toSendBuffer;
        try {
            while (!sessionCancellationToken.IsCancellationRequested) {
                if (ownerSignalSenderBuffer.TryTake(out toSendBuffer, sendBufferReadTimeoutMillis, sessionCancellationToken)) {
                    unsafe {
                        fixed (byte* pArray = toSendBuffer) {
                            //Debug.Log($"In 'ownerSignalSend' loop, sending to {lockedLobbyMemberBindings.Length} recipients including self.");
                            for (int i = lockedLobbyMemberBindings.Length-1; i >= 0; i--) {
                                SteamNetworkingIdentity memberSteamId = lockedMemberIdentities[i];
                                if (SteamUser.GetSteamID() == memberSteamId.GetSteamID()) {
                                    /*
                                     [WARNING] DON'T apply "SteamNetworkingMessages.SendMessageToUser" to self, in practice I got the following exception and crashed.

                                    ```
                                    src\steamnetworkingsockets\clientlib\steamnetworkingsockets_p2p_ice.cpp (823) : Assertion Failed: We gathered candidate type 0x400, but 0x202 is allowed
                                    ```
                                    */
                                    localDownsyncSnapshotBytesBuffer.Enqueue(toSendBuffer);
                                } else {
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

    private void send(CancellationToken sessionCancellationToken) {
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
