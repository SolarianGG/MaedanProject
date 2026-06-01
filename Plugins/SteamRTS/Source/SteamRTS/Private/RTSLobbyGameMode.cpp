// Copyright Epic Games, Inc. All Rights Reserved.

#include "RTSLobbyGameMode.h"
#include "RTSLobbyGameState.h"
#include "RTSLobbyPlayerState.h"
#include "SteamRTSSubsystem.h"
#include "SteamLobbySubsystem.h"
#include "SteamRTSSettings.h"
#include "SteamRTSLog.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameSession.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

ARTSLobbyGameMode::ARTSLobbyGameMode()
{
	bUseSeamlessTravel = true;
	PlayerStateClass = ARTSLobbyPlayerState::StaticClass();
	GameStateClass = ARTSLobbyGameState::StaticClass();
}

void ARTSLobbyGameMode::InitGameState()
{
	Super::InitGameState();

	ARTSLobbyGameState* LobbyGS = GetGameState<ARTSLobbyGameState>();
	if (!LobbyGS)
	{
		return;
	}

	// Lobby capacity defaults to the project setting and is refined from the
	// host's actual create settings below, so the slot bound stays consistent
	// with the number of connections the session was created with.
	LobbyGS->MaxPlayers = GetDefault<USteamRTSSettings>()->MaxLobbyPlayers;

	// Seed the lobby config from the host's create settings so StartMatch has a
	// map even before the host explicitly changes it.
	if (UGameInstance* GI = GetGameInstance())
	{
		if (USteamLobbySubsystem* Lobby = GI->GetSubsystem<USteamLobbySubsystem>())
		{
			const FRTSLobbySettings& Settings = Lobby->GetPendingLobbySettings();
			if (Settings.MaxPlayers > 0)
			{
				LobbyGS->MaxPlayers = Settings.MaxPlayers;
			}
			if (LobbyGS->SelectedMap.IsEmpty() && !Settings.MapName.IsEmpty())
			{
				LobbyGS->SelectedMap = Settings.MapName;
				LobbyGS->SelectedGameMode = Settings.GameMode;
				LobbyGS->OnLobbyConfigChanged.Broadcast();
			}
		}
	}
}

void ARTSLobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (ARTSLobbyPlayerState* PS = NewPlayer ? NewPlayer->GetPlayerState<ARTSLobbyPlayerState>() : nullptr)
	{
		PS->Slot = AllocateSlot();
	}

	UpdateHostAssignment();
}

void ARTSLobbyGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);
	UpdateHostAssignment();
}

int32 ARTSLobbyGameMode::AllocateSlot() const
{
	const ARTSLobbyGameState* LobbyGS = GetGameState<ARTSLobbyGameState>();
	if (!LobbyGS)
	{
		return 0;
	}

	TSet<int32> UsedSlots;
	for (APlayerState* PS : LobbyGS->PlayerArray)
	{
		if (const ARTSLobbyPlayerState* LobbyPS = Cast<ARTSLobbyPlayerState>(PS))
		{
			UsedSlots.Add(LobbyPS->Slot);
		}
	}

	for (int32 Candidate = 0; ; ++Candidate)
	{
		if (!UsedSlots.Contains(Candidate))
		{
			return Candidate;
		}
	}
}

void ARTSLobbyGameMode::UpdateHostAssignment()
{
	ARTSLobbyGameState* LobbyGS = GetGameState<ARTSLobbyGameState>();
	if (!LobbyGS)
	{
		return;
	}

	// The player with the lowest slot is treated as the host/owner.
	ARTSLobbyPlayerState* Host = nullptr;
	for (APlayerState* PS : LobbyGS->PlayerArray)
	{
		ARTSLobbyPlayerState* LobbyPS = Cast<ARTSLobbyPlayerState>(PS);
		if (!LobbyPS)
		{
			continue;
		}
		if (!Host || LobbyPS->Slot < Host->Slot)
		{
			Host = LobbyPS;
		}
	}

	for (APlayerState* PS : LobbyGS->PlayerArray)
	{
		if (ARTSLobbyPlayerState* LobbyPS = Cast<ARTSLobbyPlayerState>(PS))
		{
			const bool bNewHost = (LobbyPS == Host);
			if (LobbyPS->bIsHost != bNewHost)
			{
				LobbyPS->bIsHost = bNewHost;
				LobbyPS->OnLobbyStateChanged.Broadcast();
			}
		}
	}
}

void ARTSLobbyGameMode::KickPlayer(ARTSLobbyPlayerState* Target)
{
	if (!HasAuthority() || !Target)
	{
		return;
	}

	if (APlayerController* PC = Cast<APlayerController>(Target->GetOwner()))
	{
		if (GameSession)
		{
			GameSession->KickPlayer(PC, NSLOCTEXT("SteamRTS", "Kicked", "Kicked from lobby by host."));
		}
	}
}

void ARTSLobbyGameMode::SetSelectedMap(const FString& MapName)
{
	if (ARTSLobbyGameState* LobbyGS = GetGameState<ARTSLobbyGameState>())
	{
		LobbyGS->SelectedMap = MapName;
		LobbyGS->OnLobbyConfigChanged.Broadcast();
	}
}

void ARTSLobbyGameMode::SetSelectedGameMode(const FString& GameMode)
{
	if (ARTSLobbyGameState* LobbyGS = GetGameState<ARTSLobbyGameState>())
	{
		LobbyGS->SelectedGameMode = GameMode;
		LobbyGS->OnLobbyConfigChanged.Broadcast();
	}
}

void ARTSLobbyGameMode::StartMatch()
{
	if (!HasAuthority())
	{
		return;
	}

	ARTSLobbyGameState* LobbyGS = GetGameState<ARTSLobbyGameState>();
	if (!LobbyGS || LobbyGS->SelectedMap.IsEmpty())
	{
		UE_LOG(LogSteamRTS, Error, TEXT("StartMatch: no map selected."));
		return;
	}

	if (!LobbyGS->AreAllPlayersReady())
	{
		UE_LOG(LogSteamRTS, Warning, TEXT("StartMatch: not all players are ready."));
		return;
	}

	// Capture the roster so the match game mode can re-apply selections after travel.
	if (UGameInstance* GI = GetGameInstance())
	{
		if (USteamRTSSubsystem* SteamRTS = GI->GetSubsystem<USteamRTSSubsystem>())
		{
			SteamRTS->SetMatchRoster(LobbyGS->GetPlayerRoster());
		}
	}

	// No ?game= override: the match always uses the map's standard game mode
	// (its World Settings GameModeOverride, or the project default for the map
	// prefix). The lobby intentionally does not let players pick a game mode.
	const FString TravelURL = FString::Printf(TEXT("%s?listen"), *LobbyGS->SelectedMap);

	UE_LOG(LogSteamRTS, Log, TEXT("StartMatch: seamless travel to %s"), *TravelURL);
	GetWorld()->ServerTravel(TravelURL, /*bAbsolute*/ false);
}
