// Fill out your copyright notice in the Description page of Project Settings.

#include "RTSMatchGameMode.h"

#include "EngineUtils.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Controller.h"

#include "SteamRTSSubsystem.h"
#include "SteamRTSTypes.h"
#include "RTSPlayerStart.h"
#include "RTSPlayerState.h"
#include "RTSTeamInfo.h"

ARTSMatchGameMode::ARTSMatchGameMode(const FObjectInitializer& ObjectInitializer /*= FObjectInitializer::Get()*/)
	: Super(ObjectInitializer)
{
	// Required so the seamless travel started by ARTSLobbyGameMode::StartMatch
	// completes (seamless travel needs both source and destination game modes to opt in).
	bUseSeamlessTravel = true;
}

void ARTSMatchGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	// Size the number of teams to the lobby roster before Super spawns them.
	int32 MaxTeam = -1;
	if (UGameInstance* GI = GetGameInstance())
	{
		if (USteamRTSSubsystem* SteamRTS = GI->GetSubsystem<USteamRTSSubsystem>())
		{
			for (const FRTSPlayerInfo& Info : SteamRTS->GetMatchRoster())
			{
				MaxTeam = FMath::Max(MaxTeam, Info.Team);
			}
		}
	}

	if (MaxTeam >= 0)
	{
		SetNumTeams(static_cast<uint8>(MaxTeam + 1));
	}

	Super::InitGame(MapName, Options, ErrorMessage);
}

void ARTSMatchGameMode::RestartPlayer(AController* NewPlayer)
{
	if (NewPlayer == nullptr || NewPlayer->IsPendingKillPending())
	{
		return;
	}

	ARTSPlayerStart* StartSpot = FindStartForPlayer(NewPlayer);
	RestartPlayerAtPlayerStart(NewPlayer, StartSpot);
}

void ARTSMatchGameMode::RestartPlayerAtPlayerStart(AController* NewPlayer, AActor* StartSpot)
{
	// Base spawns units and assigns the team based on the player start's team index.
	Super::RestartPlayerAtPlayerStart(NewPlayer, StartSpot);

	// Override the team with the player's lobby selection, if we have a roster.
	const FRTSPlayerInfo* Info = FindRosterEntry(NewPlayer);
	if (Info == nullptr)
	{
		return;
	}

	ARTSPlayerState* PlayerState = Cast<ARTSPlayerState>(NewPlayer->PlayerState);
	if (PlayerState == nullptr)
	{
		return;
	}

	const TArray<ARTSTeamInfo*> AvailableTeams = GetTeams();
	if (!AvailableTeams.IsValidIndex(Info->Team))
	{
		return;
	}

	ARTSTeamInfo* DesiredTeam = AvailableTeams[Info->Team];
	if (DesiredTeam != nullptr && PlayerState->GetTeam() != DesiredTeam)
	{
		// AddToTeam removes the player from the previous team first.
		DesiredTeam->AddToTeam(NewPlayer);
	}
}

const FRTSPlayerInfo* ARTSMatchGameMode::FindRosterEntry(AController* Player) const
{
	if (Player == nullptr || Player->PlayerState == nullptr)
	{
		return nullptr;
	}

	const FUniqueNetIdRepl& Id = Player->PlayerState->GetUniqueId();
	if (!Id.IsValid())
	{
		return nullptr;
	}
	const FString IdStr = Id->ToString();

	UGameInstance* GI = GetGameInstance();
	USteamRTSSubsystem* SteamRTS = GI ? GI->GetSubsystem<USteamRTSSubsystem>() : nullptr;
	if (SteamRTS == nullptr)
	{
		return nullptr;
	}

	for (const FRTSPlayerInfo& Info : SteamRTS->GetMatchRoster())
	{
		if (Info.PlayerId == IdStr)
		{
			return &Info;
		}
	}

	return nullptr;
}

ARTSPlayerStart* ARTSMatchGameMode::FindStartForPlayer(AController* Player)
{
	const FRTSPlayerInfo* Info = FindRosterEntry(Player);
	if (Info != nullptr && Info->Slot >= 0)
	{
		// Deterministic slot -> player start mapping (sorted by name for stability).
		TArray<ARTSPlayerStart*> Starts;
		for (TActorIterator<ARTSPlayerStart> It(GetWorld()); It; ++It)
		{
			Starts.Add(*It);
		}

		Starts.Sort([](const ARTSPlayerStart& A, const ARTSPlayerStart& B)
		{
			return A.GetName() < B.GetName();
		});

		if (Starts.IsValidIndex(Info->Slot) && Starts[Info->Slot]->GetPlayer() == nullptr)
		{
			return Starts[Info->Slot];
		}
	}

	// Fall back to the base random/free player start selection.
	return FindRTSPlayerStart(Player);
}
