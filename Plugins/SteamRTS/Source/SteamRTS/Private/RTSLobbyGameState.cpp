// Copyright Epic Games, Inc. All Rights Reserved.

#include "RTSLobbyGameState.h"
#include "RTSLobbyPlayerState.h"
#include "Net/UnrealNetwork.h"

void ARTSLobbyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ARTSLobbyGameState, SelectedMap);
	DOREPLIFETIME(ARTSLobbyGameState, SelectedGameMode);
	DOREPLIFETIME(ARTSLobbyGameState, MaxPlayers);
}

TArray<FRTSPlayerInfo> ARTSLobbyGameState::GetPlayerRoster() const
{
	TArray<FRTSPlayerInfo> Roster;
	for (APlayerState* PS : PlayerArray)
	{
		if (const ARTSLobbyPlayerState* LobbyPS = Cast<ARTSLobbyPlayerState>(PS))
		{
			Roster.Add(LobbyPS->GetLobbyPlayerInfo());
		}
	}
	return Roster;
}

bool ARTSLobbyGameState::AreAllPlayersReady() const
{
	bool bAnyPlayer = false;
	for (APlayerState* PS : PlayerArray)
	{
		if (const ARTSLobbyPlayerState* LobbyPS = Cast<ARTSLobbyPlayerState>(PS))
		{
			bAnyPlayer = true;
			if (!LobbyPS->bReady)
			{
				return false;
			}
		}
	}
	return bAnyPlayer;
}

void ARTSLobbyGameState::OnRep_LobbyConfig()
{
	OnLobbyConfigChanged.Broadcast();
}
