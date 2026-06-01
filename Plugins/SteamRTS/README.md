# SteamRTS

Steam multiplayer integration plugin for the RTS project (Unreal Engine 4.27).
Provides lobby creation/browse/join, friend invites, replicated lobby state and
a seamless transition into the match, built on top of `IOnlineSubsystem`
(Steam). UX modelled on a StarCraft II–style pre-game lobby.

## Requirements

- Unreal Engine 4.27 (Win64).
- A running Steam client logged into an account.
- `OnlineSubsystemSteam` enabled (handled by `RTSProject.uproject` and this
  plugin's `.uplugin`).

## Architecture

All gameplay-facing functionality is exposed through three
`UGameInstanceSubsystem`s (Blueprint-accessible):

| Class | Role |
|---|---|
| `USteamRTSSubsystem` | Facade. `IsSteamAvailable`, local player name, accessors for the other subsystems, the `OnError` event, rich-presence helper, and the match-roster cache that survives travel. |
| `USteamLobbySubsystem` | Wraps `IOnlineSessionInterface`: `CreateLobby`, `FindLobbies`, `JoinLobby`, `LeaveLobby`. All async, results via `BlueprintAssignable` delegates. |
| `USteamFriendsSubsystem` | Wraps `IOnlineFriends`/presence: `ReadFriendsList`, `InviteFriend`, `OpenInviteOverlay`, and handles accepted invites (`OnSessionUserInviteAccepted`). |

Replicated pre-game lobby actors (use as bases for your `BP_Lobby*`):

| Class | Role |
|---|---|
| `ARTSLobbyGameMode` | Server-authoritative: host/slot assignment, kick, change map/mode, `StartMatch` (seamless travel). `bUseSeamlessTravel = true`. |
| `ARTSLobbyGameState` | Replicated selected map/mode; roster derived from `PlayerArray`; `AreAllPlayersReady`. |
| `ARTSLobbyPlayerState` | Replicated per-player `bReady/Faction/Team/Color/Slot/bIsHost`; Server RPCs for player-self and host-only actions. |

`USteamRTSSettings` (Project Settings → Game → Steam RTS) exposes
`MaxLobbyPlayers`, `LobbyVisibility`, `bUseSteamNetworking`, `bEnablePresence`,
`bEnableInvites`, `LobbyMap`, `MainMenuMap`.

## Lobby lifecycle

1. Host calls `USteamLobbySubsystem::CreateLobby(Settings)` →
   `CreateSession` (with `bUseLobbiesIfAvailable`) → on success the host
   `ServerTravel`s to `LobbyMap?listen` and presence is set to "In Lobby".
2. Client calls `FindLobbies` → `OnLobbyListUpdated(TArray<FRTSLobbyInfo>)` →
   `JoinLobby(LobbyInfo)` → `JoinSession` → resolve connect string →
   `ClientTravel` to the host.
3. In the lobby map players change their state via `ARTSLobbyPlayerState`
   (`SetReady`, `SetFaction`, …). Host actions (`HostSetMap`, `HostKickPlayer`,
   `HostStartMatch`) are Server RPCs validated against `bIsHost`.
4. `HostStartMatch` → `ARTSLobbyGameMode::StartMatch` caches the roster into
   `USteamRTSSubsystem::SetMatchRoster` and `ServerTravel`s (seamless) to the
   selected map. The match game mode reads `GetMatchRoster()` and applies each
   player's selections by matching `PlayerId`.

## Steam callback flow

Session delegates are bound per-operation and cleared in the handler (and in
`Deinitialize`). Incoming invites / "Join Game" arrive via
`OnSessionUserInviteAccepted` and route into `USteamLobbySubsystem::JoinFromInvite`.
`USteamRTSSubsystem` binds `GEngine->OnNetworkFailure` and surfaces host
disconnects / timeouts through `OnError`.

> Cold-start join (launching the game from a Steam invite, `+connect_lobby` on
> the command line) is **not** handled inside the plugin. Parse the command line
> in your `UGameInstance::Init` and call into the lobby subsystem once it is
> initialized.

## Setup / Steam AppID

`Config/DefaultEngine.ini` is preconfigured with `DefaultPlatformService=Steam`,
`OnlineSubsystemSteam bEnabled=true`, `SteamDevAppId=480` (Spacewar — Valve's
public test app). For a shipping build:

1. Replace `SteamDevAppId=480` with your own AppID.
2. Place a `steam_appid.txt` containing that AppID next to the packaged
   executable (do **not** commit a production AppID).

## Testing

Two Steam accounts on two machines, or two Steam installs. With Spacewar (480):
host `CreateLobby`, client `FindLobbies` → `JoinLobby`; verify both appear in
the same listen session, invites work via the overlay, ready/faction/slot
replicate, and `StartMatch` seamlessly travels everyone into the match while the
Steam session stays valid.

## Configurable settings (DefaultEngine.ini / Project Settings)

```
MaxLobbyPlayers=
LobbyVisibility=
UseSteamNetworking=
EnablePresence=
EnableInvites=
```

## Logging

All plugin logging uses the `LogSteamRTS` category.
