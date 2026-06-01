// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RTSGameMode.h"
#include "RTSMatchGameMode.generated.h"

struct FRTSPlayerInfo;
class ARTSPlayerStart;

/**
 * Match game mode that bridges the SteamRTS lobby into actual gameplay. Reads the
 * roster captured at lobby StartMatch (USteamRTSSubsystem::GetMatchRoster) and
 * applies each player's lobby team and slot after the seamless travel.
 */
UCLASS()
class RTSPROJECT_API ARTSMatchGameMode : public ARTSGameMode
{
	GENERATED_BODY()

public:
	ARTSMatchGameMode(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void RestartPlayer(AController* NewPlayer) override;
	virtual void RestartPlayerAtPlayerStart(AController* NewPlayer, AActor* StartSpot) override;

private:
	/** Finds the roster entry matching the player's Steam id, or nullptr. */
	const FRTSPlayerInfo* FindRosterEntry(AController* Player) const;

	/** Picks a player start by the player's lobby slot; falls back to the base random pick. */
	ARTSPlayerStart* FindStartForPlayer(AController* Player);
};
