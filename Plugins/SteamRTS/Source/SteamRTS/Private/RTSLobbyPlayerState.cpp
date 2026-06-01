// Copyright Epic Games, Inc. All Rights Reserved.

#include "RTSLobbyPlayerState.h"
#include "RTSLobbyGameMode.h"
#include "RTSLobbyGameState.h"
#include "SteamRTSSettings.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"

ARTSLobbyPlayerState::ARTSLobbyPlayerState()
{
	bReplicates = true;
}

void ARTSLobbyPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ARTSLobbyPlayerState, bIsHost);
	DOREPLIFETIME(ARTSLobbyPlayerState, bReady);
	DOREPLIFETIME(ARTSLobbyPlayerState, Faction);
	DOREPLIFETIME(ARTSLobbyPlayerState, Team);
	DOREPLIFETIME(ARTSLobbyPlayerState, Color);
	DOREPLIFETIME(ARTSLobbyPlayerState, Slot);
}

FRTSPlayerInfo ARTSLobbyPlayerState::GetLobbyPlayerInfo() const
{
	FRTSPlayerInfo Info;
	Info.PlayerId = GetUniqueId().IsValid() ? GetUniqueId()->ToString() : FString();
	Info.DisplayName = GetPlayerName();
	Info.bIsHost = bIsHost;
	Info.bReady = bReady;
	Info.Faction = Faction;
	Info.Team = Team;
	Info.Color = Color;
	Info.Slot = Slot;
	return Info;
}

void ARTSLobbyPlayerState::OnRep_LobbyState()
{
	OnLobbyStateChanged.Broadcast();
}

void ARTSLobbyPlayerState::ServerSetReady_Implementation(bool bInReady)
{
	bReady = bInReady;
	OnRep_LobbyState();
}
bool ARTSLobbyPlayerState::ServerSetReady_Validate(bool) { return true; }

void ARTSLobbyPlayerState::ServerSetFaction_Implementation(int32 InFaction)
{
	if (InFaction < 0)
	{
		return;
	}
	Faction = InFaction;
	OnRep_LobbyState();
}
bool ARTSLobbyPlayerState::ServerSetFaction_Validate(int32 InFaction) { return InFaction >= 0; }

void ARTSLobbyPlayerState::ServerSetTeam_Implementation(int32 InTeam)
{
	if (InTeam < 0)
	{
		return;
	}
	Team = InTeam;
	OnRep_LobbyState();
}
bool ARTSLobbyPlayerState::ServerSetTeam_Validate(int32 InTeam) { return InTeam >= 0; }

void ARTSLobbyPlayerState::ServerSetColor_Implementation(int32 InColor)
{
	if (InColor < 0)
	{
		return;
	}
	Color = InColor;
	OnRep_LobbyState();
}
bool ARTSLobbyPlayerState::ServerSetColor_Validate(int32 InColor) { return InColor >= 0; }

void ARTSLobbyPlayerState::ServerSetSlot_Implementation(int32 InSlot)
{
	const ARTSLobbyGameState* LobbyGS = GetWorld() ? GetWorld()->GetGameState<ARTSLobbyGameState>() : nullptr;

	// Bound the slot to the lobby's actual capacity rather than the project-wide
	// default, so it stays consistent with the session's public connections.
	const int32 MaxPlayers = LobbyGS ? LobbyGS->MaxPlayers : GetDefault<USteamRTSSettings>()->MaxLobbyPlayers;
	if (InSlot < 0 || InSlot >= MaxPlayers)
	{
		return;
	}

	// Reject slots already taken by another player.
	if (LobbyGS)
	{
		for (APlayerState* PS : LobbyGS->PlayerArray)
		{
			const ARTSLobbyPlayerState* LobbyPS = Cast<ARTSLobbyPlayerState>(PS);
			if (LobbyPS && LobbyPS != this && LobbyPS->Slot == InSlot)
			{
				return;
			}
		}
	}

	Slot = InSlot;
	OnRep_LobbyState();

	// The host is derived from the lowest occupied slot, so a slot change can
	// move host ownership — recompute it server-side.
	if (ARTSLobbyGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ARTSLobbyGameMode>() : nullptr)
	{
		GM->UpdateHostAssignment();
	}
}
bool ARTSLobbyPlayerState::ServerSetSlot_Validate(int32 InSlot) { return InSlot >= 0; }

void ARTSLobbyPlayerState::ServerKickPlayer_Implementation(ARTSLobbyPlayerState* Target)
{
	if (bIsHost)
	{
		if (ARTSLobbyGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ARTSLobbyGameMode>() : nullptr)
		{
			GM->KickPlayer(Target);
		}
	}
}
bool ARTSLobbyPlayerState::ServerKickPlayer_Validate(ARTSLobbyPlayerState*) { return true; }

void ARTSLobbyPlayerState::ServerSetMap_Implementation(const FString& MapName)
{
	if (bIsHost)
	{
		if (ARTSLobbyGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ARTSLobbyGameMode>() : nullptr)
		{
			GM->SetSelectedMap(MapName);
		}
	}
}
bool ARTSLobbyPlayerState::ServerSetMap_Validate(const FString&) { return true; }

void ARTSLobbyPlayerState::ServerSetGameMode_Implementation(const FString& GameMode)
{
	if (bIsHost)
	{
		if (ARTSLobbyGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ARTSLobbyGameMode>() : nullptr)
		{
			GM->SetSelectedGameMode(GameMode);
		}
	}
}
bool ARTSLobbyPlayerState::ServerSetGameMode_Validate(const FString&) { return true; }

void ARTSLobbyPlayerState::ServerStartMatch_Implementation()
{
	if (bIsHost)
	{
		if (ARTSLobbyGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ARTSLobbyGameMode>() : nullptr)
		{
			GM->StartMatch();
		}
	}
}
bool ARTSLobbyPlayerState::ServerStartMatch_Validate() { return true; }
