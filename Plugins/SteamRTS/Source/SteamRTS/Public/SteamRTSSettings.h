// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "SteamRTSTypes.h"
#include "SteamRTSSettings.generated.h"

/**
 * Project-level configuration for the SteamRTS plugin.
 * Editable under Project Settings > Game > Steam RTS, persisted to DefaultGame.ini.
 */
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Steam RTS"))
class STEAMRTS_API USteamRTSSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	USteamRTSSettings();

	virtual FName GetCategoryName() const override;

	/** Default maximum number of players in a lobby. */
	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Lobby", meta = (ClampMin = "2", ClampMax = "16"))
	int32 MaxLobbyPlayers;

	/** Default visibility used when creating a lobby. */
	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Lobby")
	ERTSLobbyVisibility LobbyVisibility;

	/** Whether to route gameplay traffic through Steam P2P networking. */
	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Networking")
	bool bUseSteamNetworking;

	/** Whether to publish Steam rich presence. */
	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Presence")
	bool bEnablePresence;

	/** Whether friend invites are enabled. */
	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Invites")
	bool bEnableInvites;

	/** Map travelled to (as a listen server) when a lobby is created. */
	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Lobby")
	FString LobbyMap;

	/** Map returned to when leaving a lobby. */
	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Lobby")
	FString MainMenuMap;
};
