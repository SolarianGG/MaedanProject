// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RTSLobbyGameMode.generated.h"

class ARTSLobbyPlayerState;

/**
 * Server-authoritative pre-game lobby game mode. Assigns the host, manages
 * slots and host-only actions (kick / change map / start match) and triggers a
 * seamless travel into the match. Use as a base for BP_LobbyGameMode.
 */
UCLASS()
class STEAMRTS_API ARTSLobbyGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARTSLobbyGameMode();

	virtual void InitGameState() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	/** Host-only: remove a player from the lobby. */
	void KickPlayer(ARTSLobbyPlayerState* Target);

	/** Host-only: set the map the match will load. */
	void SetSelectedMap(const FString& MapName);

	/** Host-only: set the game mode identifier. */
	void SetSelectedGameMode(const FString& GameMode);

	/** Host-only: travel all players into the match if everyone is ready. */
	void StartMatch();

	/** Re-evaluate which player is the host (the lobby owner). Server-only. */
	void UpdateHostAssignment();

protected:
	/** Assign the first free slot to a player. */
	int32 AllocateSlot() const;
};
