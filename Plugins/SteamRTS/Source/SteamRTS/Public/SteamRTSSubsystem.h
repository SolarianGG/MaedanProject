// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/EngineBaseTypes.h"
#include "SteamRTSTypes.h"
#include "SteamRTSSubsystem.generated.h"

class USteamLobbySubsystem;
class USteamFriendsSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSteamRTSError, ERTSSteamError, ErrorCode, const FString&, Message);

/**
 * Top-level facade for the SteamRTS plugin. Provides convenient access to the
 * lobby and friends subsystems and reports whether a Steam online subsystem is
 * available.
 */
UCLASS()
class STEAMRTS_API USteamRTSSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** True if a Steam online subsystem is the active platform service. */
	UFUNCTION(BlueprintPure, Category = "SteamRTS")
	bool IsSteamAvailable() const { return bSteamAvailable; }

	/** Local player's Steam display name, or empty if unavailable. */
	UFUNCTION(BlueprintPure, Category = "SteamRTS")
	FString GetLocalPlayerName() const;

	UFUNCTION(BlueprintPure, Category = "SteamRTS")
	USteamLobbySubsystem* GetLobbySubsystem() const;

	UFUNCTION(BlueprintPure, Category = "SteamRTS")
	USteamFriendsSubsystem* GetFriendsSubsystem() const;

	/**
	 * Server-side roster snapshot captured at match start. Survives the
	 * seamless travel into the match (GameInstance persists), letting the match
	 * game mode apply each player's lobby selections by matching PlayerId.
	 */
	UFUNCTION(BlueprintCallable, Category = "SteamRTS|Match")
	void SetMatchRoster(const TArray<FRTSPlayerInfo>& Roster) { CachedMatchRoster = Roster; }

	UFUNCTION(BlueprintPure, Category = "SteamRTS|Match")
	const TArray<FRTSPlayerInfo>& GetMatchRoster() const { return CachedMatchRoster; }

	/**
	 * Publish a Steam rich-presence status string (e.g. "In Lobby", "In Match").
	 * No-op if presence is disabled in settings or Steam is unavailable.
	 */
	UFUNCTION(BlueprintCallable, Category = "SteamRTS|Presence")
	void SetPresenceStatus(const FString& StatusText);

	/** Broadcast a recoverable error to any listening UI. */
	UFUNCTION(BlueprintCallable, Category = "SteamRTS")
	void ReportError(ERTSSteamError ErrorCode, const FString& Message);

	UPROPERTY(BlueprintAssignable, Category = "SteamRTS")
	FOnSteamRTSError OnError;

private:
	void HandleNetworkFailure(UWorld* World, class UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);

	bool bSteamAvailable = false;

	FDelegateHandle NetworkFailureHandle;

	UPROPERTY()
	TArray<FRTSPlayerInfo> CachedMatchRoster;
};
