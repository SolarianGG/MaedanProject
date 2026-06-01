// Copyright Epic Games, Inc. All Rights Reserved.

#include "SteamLobbySubsystem.h"
#include "SteamRTSSubsystem.h"
#include "SteamFriendsSubsystem.h"
#include "SteamRTSLog.h"
#include "SteamRTSSettings.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Misc/App.h"

namespace
{
	const FName LobbySessionName = NAME_GameSession;
	const FName SETTING_RTS_LOBBYNAME = FName(TEXT("RTSLOBBYNAME"));
	const FName SETTING_RTS_BUILDVERSION = FName(TEXT("RTSBUILDVERSION"));
}

void USteamLobbySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogSteamRTS, Log, TEXT("USteamLobbySubsystem initialized."));
}

void USteamLobbySubsystem::Deinitialize()
{
	if (const IOnlineSessionPtr Session = GetSessionInterface())
	{
		Session->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteHandle);
		Session->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
		Session->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);
		Session->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteHandle);
	}
	Super::Deinitialize();
}

IOnlineSessionPtr USteamLobbySubsystem::GetSessionInterface() const
{
	if (IOnlineSubsystem* const OnlineSub = Online::GetSubsystem(GetWorld()))
	{
		return OnlineSub->GetSessionInterface();
	}
	return nullptr;
}

FRTSLobbySettings USteamLobbySubsystem::GetDefaultLobbySettings() const
{
	FRTSLobbySettings Settings;
	if (const USteamRTSSettings* const Config = GetDefault<USteamRTSSettings>())
	{
		Settings.MaxPlayers = Config->MaxLobbyPlayers;
		Settings.Visibility = Config->LobbyVisibility;
	}
	return Settings;
}

void USteamLobbySubsystem::CreateLobby(const FRTSLobbySettings& Settings)
{
	const IOnlineSessionPtr Session = GetSessionInterface();
	if (!Session.IsValid())
	{
		UE_LOG(LogSteamRTS, Error, TEXT("CreateLobby: no session interface."));
		ReportError(ERTSSteamError::NoOnlineSubsystem, TEXT("No online subsystem available."));
		OnLobbyCreated.Broadcast(false);
		return;
	}

	// If a session already exists, tear it down before creating a new one.
	if (Session->GetNamedSession(LobbySessionName) != nullptr)
	{
		UE_LOG(LogSteamRTS, Warning, TEXT("CreateLobby: destroying existing session first."));
		LeaveLobby();
	}

	PendingCreateSettings = Settings;

	const USteamRTSSettings* const Config = GetDefault<USteamRTSSettings>();
	const bool bUseSteamNetworking = (Config == nullptr) || Config->bUseSteamNetworking;
	const bool bInvitesEnabled = (Config == nullptr) || Config->bEnableInvites;

	TSharedRef<FOnlineSessionSettings> SessionSettings = MakeShared<FOnlineSessionSettings>();
	SessionSettings->bIsLANMatch = false;
	SessionSettings->NumPublicConnections = FMath::Max(1, Settings.MaxPlayers);
	SessionSettings->NumPrivateConnections = 0;
	SessionSettings->bAllowJoinInProgress = true;
	SessionSettings->bAllowInvites = bInvitesEnabled;
	SessionSettings->bUsesPresence = true;
	// Prefer Steam lobby (P2P) hosting when Steam networking is enabled in settings.
	SessionSettings->bUseLobbiesIfAvailable = bUseSteamNetworking;
	SessionSettings->bShouldAdvertise = (Settings.Visibility != ERTSLobbyVisibility::Private);
	SessionSettings->bAllowJoinViaPresence = (Settings.Visibility == ERTSLobbyVisibility::Public);
	SessionSettings->bAllowJoinViaPresenceFriendsOnly = (Settings.Visibility == ERTSLobbyVisibility::FriendsOnly);

	SessionSettings->Set(SETTING_RTS_LOBBYNAME, Settings.LobbyName, EOnlineDataAdvertisementType::ViaOnlineService);
	SessionSettings->Set(SETTING_MAPNAME, Settings.MapName, EOnlineDataAdvertisementType::ViaOnlineService);
	SessionSettings->Set(SETTING_GAMEMODE, Settings.GameMode, EOnlineDataAdvertisementType::ViaOnlineService);
	// Advertise the build version so clients can refuse to join an incompatible host.
	SessionSettings->Set(SETTING_RTS_BUILDVERSION, FString(FApp::GetBuildVersion()), EOnlineDataAdvertisementType::ViaOnlineService);

	for (const TPair<FString, FString>& Pair : Settings.CustomMetadata)
	{
		SessionSettings->Set(FName(*Pair.Key), Pair.Value, EOnlineDataAdvertisementType::ViaOnlineService);
	}

	CreateSessionCompleteHandle = Session->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &USteamLobbySubsystem::HandleCreateSessionComplete));

	bIsHost = true;
	if (!Session->CreateSession(0, LobbySessionName, *SessionSettings))
	{
		UE_LOG(LogSteamRTS, Error, TEXT("CreateLobby: CreateSession call failed synchronously."));
		Session->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteHandle);
		bIsHost = false;
		OnLobbyCreated.Broadcast(false);
	}
}

void USteamLobbySubsystem::HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogSteamRTS, Log, TEXT("HandleCreateSessionComplete: '%s' success=%d"), *SessionName.ToString(), bWasSuccessful);

	if (const IOnlineSessionPtr Session = GetSessionInterface())
	{
		Session->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteHandle);
	}

	OnLobbyCreated.Broadcast(bWasSuccessful);

	if (bWasSuccessful)
	{
		if (USteamRTSSubsystem* Facade = GetGameInstance() ? GetGameInstance()->GetSubsystem<USteamRTSSubsystem>() : nullptr)
		{
			Facade->SetPresenceStatus(TEXT("In Lobby"));
		}
		TravelToListenServerLobby();
	}
	else
	{
		bIsHost = false;
		ReportError(ERTSSteamError::CreateLobbyFailed, TEXT("Failed to create lobby."));
	}
}

void USteamLobbySubsystem::FindLobbies(const FString& MapFilter, int32 MaxResults, bool bFriendsOnly)
{
	bPendingFriendsOnly = bFriendsOnly;
	PendingFindMapFilter = MapFilter;
	PendingFindMaxResults = MaxResults;

	// A friends-only search needs the friends cache populated, otherwise every lobby
	// would be filtered out. If it is empty, read the friends list first and resume
	// the search from HandleFriendsReadyForFind once it completes.
	if (bFriendsOnly)
	{
		if (USteamFriendsSubsystem* Friends = GetGameInstance() ? GetGameInstance()->GetSubsystem<USteamFriendsSubsystem>() : nullptr)
		{
			if (Friends->GetCachedFriends().Num() == 0)
			{
				UE_LOG(LogSteamRTS, Log, TEXT("FindLobbies: friends cache empty, reading friends list before search."));
				Friends->OnFriendsListUpdated.AddDynamic(this, &USteamLobbySubsystem::HandleFriendsReadyForFind);
				Friends->ReadFriendsList();
				return;
			}
		}
	}

	ExecuteSessionSearch();
}

void USteamLobbySubsystem::HandleFriendsReadyForFind(const TArray<FRTSFriendInfo>& Friends)
{
	if (USteamFriendsSubsystem* FriendsSub = GetGameInstance() ? GetGameInstance()->GetSubsystem<USteamFriendsSubsystem>() : nullptr)
	{
		FriendsSub->OnFriendsListUpdated.RemoveDynamic(this, &USteamLobbySubsystem::HandleFriendsReadyForFind);
	}

	ExecuteSessionSearch();
}

void USteamLobbySubsystem::ExecuteSessionSearch()
{
	const IOnlineSessionPtr Session = GetSessionInterface();
	if (!Session.IsValid())
	{
		UE_LOG(LogSteamRTS, Error, TEXT("FindLobbies: no session interface."));
		ReportError(ERTSSteamError::FindLobbiesFailed, TEXT("No online subsystem available."));
		OnLobbyListUpdated.Broadcast(DiscoveredLobbies);
		return;
	}

	SessionSearch = MakeShared<FOnlineSessionSearch>();
	SessionSearch->bIsLanQuery = false;
	SessionSearch->MaxSearchResults = FMath::Max(1, PendingFindMaxResults);
	SessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);

	if (!PendingFindMapFilter.IsEmpty())
	{
		SessionSearch->QuerySettings.Set(SETTING_MAPNAME, PendingFindMapFilter, EOnlineComparisonOp::Equals);
	}

	FindSessionsCompleteHandle = Session->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &USteamLobbySubsystem::HandleFindSessionsComplete));

	if (!Session->FindSessions(0, SessionSearch.ToSharedRef()))
	{
		UE_LOG(LogSteamRTS, Error, TEXT("FindLobbies: FindSessions call failed synchronously."));
		Session->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
		ReportError(ERTSSteamError::FindLobbiesFailed, TEXT("Failed to start lobby search."));
		OnLobbyListUpdated.Broadcast(DiscoveredLobbies);
	}
}

void USteamLobbySubsystem::HandleFindSessionsComplete(bool bWasSuccessful)
{
	if (const IOnlineSessionPtr Session = GetSessionInterface())
	{
		Session->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
	}

	DiscoveredLobbies.Reset();

	if (!bWasSuccessful)
	{
		UE_LOG(LogSteamRTS, Error, TEXT("HandleFindSessionsComplete: lobby search failed."));
		ReportError(ERTSSteamError::FindLobbiesFailed, TEXT("Failed to search for lobbies."));
	}

	if (bWasSuccessful && SessionSearch.IsValid())
	{
		// Collect the cached friend ids once so we can flag lobbies hosted by friends.
		TSet<FString> FriendIds;
		if (USteamFriendsSubsystem* Friends = GetGameInstance() ? GetGameInstance()->GetSubsystem<USteamFriendsSubsystem>() : nullptr)
		{
			for (const FRTSFriendInfo& Friend : Friends->GetCachedFriends())
			{
				FriendIds.Add(Friend.PlayerId);
			}
		}

		for (int32 Index = 0; Index < SessionSearch->SearchResults.Num(); ++Index)
		{
			FRTSLobbyInfo Info = BuildLobbyInfoFromSearchResult(SessionSearch->SearchResults[Index], Index, FriendIds);
			if (bPendingFriendsOnly && !Info.bHostIsFriend)
			{
				continue;
			}
			DiscoveredLobbies.Add(Info);
		}
	}

	UE_LOG(LogSteamRTS, Log, TEXT("HandleFindSessionsComplete: success=%d found=%d"), bWasSuccessful, DiscoveredLobbies.Num());
	OnLobbyListUpdated.Broadcast(DiscoveredLobbies);
}

void USteamLobbySubsystem::ReportError(ERTSSteamError ErrorCode, const FString& Message)
{
	if (USteamRTSSubsystem* Facade = GetGameInstance() ? GetGameInstance()->GetSubsystem<USteamRTSSubsystem>() : nullptr)
	{
		Facade->ReportError(ErrorCode, Message);
	}
}

FRTSLobbyInfo USteamLobbySubsystem::BuildLobbyInfoFromSearchResult(const FOnlineSessionSearchResult& Result, int32 Index, const TSet<FString>& FriendIds) const
{
	FRTSLobbyInfo Info;
	Info.SearchResultIndex = Index;
	Info.LobbyId = Result.GetSessionIdStr();
	Info.HostName = Result.Session.OwningUserName;
	Info.MaxPlayers = Result.Session.SessionSettings.NumPublicConnections;
	Info.CurrentPlayers = Info.MaxPlayers - Result.Session.NumOpenPublicConnections;
	Info.PingInMs = Result.PingInMs;

	if (Result.Session.OwningUserId.IsValid())
	{
		Info.bHostIsFriend = FriendIds.Contains(Result.Session.OwningUserId->ToString());
	}

	Result.Session.SessionSettings.Get(SETTING_RTS_LOBBYNAME, Info.LobbyName);
	Result.Session.SessionSettings.Get(SETTING_MAPNAME, Info.MapName);
	Result.Session.SessionSettings.Get(SETTING_GAMEMODE, Info.GameMode);

	return Info;
}

void USteamLobbySubsystem::JoinLobby(const FRTSLobbyInfo& Lobby)
{
	// Match by the stable lobby id rather than the search index: a subsequent FindLobbies
	// rebuilds SessionSearch, so a stale index could otherwise resolve to the wrong lobby.
	const FOnlineSessionSearchResult* const Found = FindSearchResultByLobbyId(Lobby.LobbyId);
	if (Found == nullptr)
	{
		UE_LOG(LogSteamRTS, Error, TEXT("JoinLobby: lobby '%s' not found in current search results."), *Lobby.LobbyId);
		ReportError(ERTSSteamError::JoinLobbyFailed, TEXT("The selected lobby is no longer available."));
		OnLobbyJoined.Broadcast(false);
		return;
	}

	JoinResolvedResult(*Found);
}

const FOnlineSessionSearchResult* USteamLobbySubsystem::FindSearchResultByLobbyId(const FString& LobbyId) const
{
	if (!SessionSearch.IsValid() || LobbyId.IsEmpty())
	{
		return nullptr;
	}

	for (const FOnlineSessionSearchResult& Result : SessionSearch->SearchResults)
	{
		if (Result.GetSessionIdStr() == LobbyId)
		{
			return &Result;
		}
	}

	return nullptr;
}

void USteamLobbySubsystem::JoinFromInvite(const FOnlineSessionSearchResult& InviteResult)
{
	JoinResolvedResult(InviteResult);
}

void USteamLobbySubsystem::JoinResolvedResult(const FOnlineSessionSearchResult& Result)
{
	const IOnlineSessionPtr Session = GetSessionInterface();
	if (!Session.IsValid() || !Result.IsValid())
	{
		UE_LOG(LogSteamRTS, Error, TEXT("JoinLobby: invalid session interface or search result."));
		OnLobbyJoined.Broadcast(false);
		return;
	}

	// Refuse to join a host running an incompatible build version.
	FString RemoteBuildVersion;
	Result.Session.SessionSettings.Get(SETTING_RTS_BUILDVERSION, RemoteBuildVersion);
	const FString LocalBuildVersion = FString(FApp::GetBuildVersion());
	if (!RemoteBuildVersion.IsEmpty() && RemoteBuildVersion != LocalBuildVersion)
	{
		UE_LOG(LogSteamRTS, Warning, TEXT("JoinLobby: build version mismatch (local='%s', host='%s')."), *LocalBuildVersion, *RemoteBuildVersion);
		ReportError(ERTSSteamError::VersionMismatch, TEXT("The host is running a different game version."));
		OnLobbyJoined.Broadcast(false);
		return;
	}

	// If we are already in a session (e.g. accepting an invite while in a lobby),
	// tear it down first and resume the join from HandleDestroySessionComplete.
	if (Session->GetNamedSession(LobbySessionName) != nullptr)
	{
		UE_LOG(LogSteamRTS, Log, TEXT("JoinLobby: destroying existing session before joining."));
		PendingJoinResult = MakeShared<FOnlineSessionSearchResult>(Result);

		DestroySessionCompleteHandle = Session->AddOnDestroySessionCompleteDelegate_Handle(
			FOnDestroySessionCompleteDelegate::CreateUObject(this, &USteamLobbySubsystem::HandleDestroySessionComplete));

		if (!Session->DestroySession(LobbySessionName))
		{
			UE_LOG(LogSteamRTS, Error, TEXT("JoinLobby: DestroySession call failed synchronously."));
			Session->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteHandle);
			PendingJoinResult.Reset();
			OnLobbyJoined.Broadcast(false);
		}
		return;
	}

	JoinSessionCompleteHandle = Session->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &USteamLobbySubsystem::HandleJoinSessionComplete));

	bIsHost = false;
	if (!Session->JoinSession(0, LobbySessionName, Result))
	{
		UE_LOG(LogSteamRTS, Error, TEXT("JoinLobby: JoinSession call failed synchronously."));
		Session->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);
		OnLobbyJoined.Broadcast(false);
	}
}

void USteamLobbySubsystem::HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (const IOnlineSessionPtr Session = GetSessionInterface())
	{
		Session->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);
	}

	const bool bSuccess = (Result == EOnJoinSessionCompleteResult::Success);
	UE_LOG(LogSteamRTS, Log, TEXT("HandleJoinSessionComplete: '%s' result=%d"), *SessionName.ToString(), (int32)Result);

	if (!bSuccess)
	{
		const ERTSSteamError ErrCode = (Result == EOnJoinSessionCompleteResult::SessionIsFull)
			? ERTSSteamError::LobbyFull : ERTSSteamError::JoinLobbyFailed;
		ReportError(ErrCode, TEXT("Failed to join lobby."));
	}

	OnLobbyJoined.Broadcast(bSuccess);

	if (bSuccess)
	{
		if (USteamRTSSubsystem* Facade = GetGameInstance() ? GetGameInstance()->GetSubsystem<USteamRTSSubsystem>() : nullptr)
		{
			Facade->SetPresenceStatus(TEXT("In Lobby"));
		}
		TravelClientToSession();
	}
}

void USteamLobbySubsystem::LeaveLobby()
{
	const IOnlineSessionPtr Session = GetSessionInterface();
	if (!Session.IsValid() || Session->GetNamedSession(LobbySessionName) == nullptr)
	{
		OnLobbyLeft.Broadcast();
		return;
	}

	DestroySessionCompleteHandle = Session->AddOnDestroySessionCompleteDelegate_Handle(
		FOnDestroySessionCompleteDelegate::CreateUObject(this, &USteamLobbySubsystem::HandleDestroySessionComplete));

	if (!Session->DestroySession(LobbySessionName))
	{
		UE_LOG(LogSteamRTS, Error, TEXT("LeaveLobby: DestroySession call failed synchronously."));
		Session->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteHandle);
		OnLobbyLeft.Broadcast();
	}
}

void USteamLobbySubsystem::HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (const IOnlineSessionPtr Session = GetSessionInterface())
	{
		Session->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteHandle);
	}

	UE_LOG(LogSteamRTS, Log, TEXT("HandleDestroySessionComplete: '%s' success=%d"), *SessionName.ToString(), bWasSuccessful);

	// If a join was deferred until the old session was destroyed (invite flow),
	// resume it now instead of leaving to the menu.
	if (PendingJoinResult.IsValid())
	{
		const FOnlineSessionSearchResult ResultToJoin = *PendingJoinResult;
		PendingJoinResult.Reset();
		bIsHost = false;
		JoinResolvedResult(ResultToJoin);
		return;
	}

	const bool bWasHost = bIsHost;
	bIsHost = false;
	OnLobbyLeft.Broadcast();

	// Clients return to the menu; the host already left via travel.
	if (!bWasHost)
	{
		TravelToMainMenu();
	}
}

void USteamLobbySubsystem::TravelToListenServerLobby()
{
	const USteamRTSSettings* const Config = GetDefault<USteamRTSSettings>();
	const FString LobbyMap = (Config && !Config->LobbyMap.IsEmpty()) ? Config->LobbyMap : TEXT("/Game/Maps/L_Lobby");

	if (UWorld* const World = GetWorld())
	{
		const FString TravelURL = FString::Printf(TEXT("%s?listen"), *LobbyMap);
		UE_LOG(LogSteamRTS, Log, TEXT("Host travelling to listen server: %s"), *TravelURL);
		World->ServerTravel(TravelURL);
	}
}

void USteamLobbySubsystem::TravelClientToSession()
{
	const IOnlineSessionPtr Session = GetSessionInterface();
	if (!Session.IsValid())
	{
		return;
	}

	FString ConnectString;
	if (!Session->GetResolvedConnectString(LobbySessionName, ConnectString))
	{
		UE_LOG(LogSteamRTS, Error, TEXT("TravelClientToSession: could not resolve connect string."));
		return;
	}

	if (APlayerController* const PC = GetGameInstance() ? GetGameInstance()->GetFirstLocalPlayerController() : nullptr)
	{
		UE_LOG(LogSteamRTS, Log, TEXT("Client travelling to %s"), *ConnectString);
		PC->ClientTravel(ConnectString, ETravelType::TRAVEL_Absolute);
	}
}

void USteamLobbySubsystem::TravelToMainMenu()
{
	const USteamRTSSettings* const Config = GetDefault<USteamRTSSettings>();
	const FString MenuMap = (Config && !Config->MainMenuMap.IsEmpty()) ? Config->MainMenuMap : TEXT("/Game/Maps/M_MainMenu");

	if (APlayerController* const PC = GetGameInstance() ? GetGameInstance()->GetFirstLocalPlayerController() : nullptr)
	{
		PC->ClientTravel(MenuMap, ETravelType::TRAVEL_Absolute);
	}
}
