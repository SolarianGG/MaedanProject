// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "SteamRTSTypes.h"
#include "RTSLobbyGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLobbyConfigChanged);

/**
 * Replicated lobby-wide configuration (selected map / game mode). The player
 * roster is derived from PlayerArray (ARTSLobbyPlayerState). Use as a base for
 * BP_LobbyGameState.
 */
UCLASS()
class STEAMRTS_API ARTSLobbyGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Build a roster snapshot from the current PlayerArray. */
	UFUNCTION(BlueprintPure, Category = "SteamRTS|Lobby")
	TArray<FRTSPlayerInfo> GetPlayerRoster() const;

	/** True if every connected player is marked ready. */
	UFUNCTION(BlueprintPure, Category = "SteamRTS|Lobby")
	bool AreAllPlayersReady() const;

	UPROPERTY(ReplicatedUsing = OnRep_LobbyConfig, BlueprintReadOnly, Category = "SteamRTS|Lobby")
	FString SelectedMap;

	UPROPERTY(ReplicatedUsing = OnRep_LobbyConfig, BlueprintReadOnly, Category = "SteamRTS|Lobby")
	FString SelectedGameMode;

	/** Maximum players for this lobby; mirrors the session's public connections. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "SteamRTS|Lobby")
	int32 MaxPlayers = 8;

	UPROPERTY(BlueprintAssignable, Category = "SteamRTS|Lobby")
	FOnLobbyConfigChanged OnLobbyConfigChanged;

protected:
	UFUNCTION()
	void OnRep_LobbyConfig();
};
