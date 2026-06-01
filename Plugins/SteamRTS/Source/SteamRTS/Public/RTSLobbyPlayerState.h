// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "SteamRTSTypes.h"
#include "RTSLobbyPlayerState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLobbyPlayerStateChanged);

/**
 * Replicated per-player lobby state (ready / faction / team / color / slot).
 * Players change their own state via the Server* RPCs; the server is
 * authoritative. Use as a base for BP_LobbyPlayerState.
 */
UCLASS()
class STEAMRTS_API ARTSLobbyPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	ARTSLobbyPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Snapshot of this player's lobby state. */
	UFUNCTION(BlueprintPure, Category = "SteamRTS|Lobby")
	FRTSPlayerInfo GetLobbyPlayerInfo() const;

	// --- Player-controlled (call from owning client) ---
	UFUNCTION(BlueprintCallable, Category = "SteamRTS|Lobby")
	void SetReady(bool bInReady) { ServerSetReady(bInReady); }

	UFUNCTION(BlueprintCallable, Category = "SteamRTS|Lobby")
	void SetFaction(int32 InFaction) { ServerSetFaction(InFaction); }

	UFUNCTION(BlueprintCallable, Category = "SteamRTS|Lobby")
	void SetTeam(int32 InTeam) { ServerSetTeam(InTeam); }

	UFUNCTION(BlueprintCallable, Category = "SteamRTS|Lobby")
	void SetColor(int32 InColor) { ServerSetColor(InColor); }

	UFUNCTION(BlueprintCallable, Category = "SteamRTS|Lobby")
	void SetSlot(int32 InSlot) { ServerSetSlot(InSlot); }

	// --- Host-only actions (validated server-side against bIsHost) ---
	UFUNCTION(BlueprintCallable, Category = "SteamRTS|Lobby")
	void HostKickPlayer(ARTSLobbyPlayerState* Target) { ServerKickPlayer(Target); }

	UFUNCTION(BlueprintCallable, Category = "SteamRTS|Lobby")
	void HostSetMap(const FString& MapName) { ServerSetMap(MapName); }

	UFUNCTION(BlueprintCallable, Category = "SteamRTS|Lobby")
	void HostSetGameMode(const FString& GameMode) { ServerSetGameMode(GameMode); }

	UFUNCTION(BlueprintCallable, Category = "SteamRTS|Lobby")
	void HostStartMatch() { ServerStartMatch(); }

	/** Fired on all clients whenever any replicated lobby field changes. */
	UPROPERTY(BlueprintAssignable, Category = "SteamRTS|Lobby")
	FOnLobbyPlayerStateChanged OnLobbyStateChanged;

	UPROPERTY(ReplicatedUsing = OnRep_LobbyState, BlueprintReadOnly, Category = "SteamRTS|Lobby")
	bool bIsHost = false;

	UPROPERTY(ReplicatedUsing = OnRep_LobbyState, BlueprintReadOnly, Category = "SteamRTS|Lobby")
	bool bReady = false;

	UPROPERTY(ReplicatedUsing = OnRep_LobbyState, BlueprintReadOnly, Category = "SteamRTS|Lobby")
	int32 Faction = 0;

	UPROPERTY(ReplicatedUsing = OnRep_LobbyState, BlueprintReadOnly, Category = "SteamRTS|Lobby")
	int32 Team = 0;

	UPROPERTY(ReplicatedUsing = OnRep_LobbyState, BlueprintReadOnly, Category = "SteamRTS|Lobby")
	int32 Color = 0;

	UPROPERTY(ReplicatedUsing = OnRep_LobbyState, BlueprintReadOnly, Category = "SteamRTS|Lobby")
	int32 Slot = -1;

protected:
	UFUNCTION()
	void OnRep_LobbyState();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSetReady(bool bInReady);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSetFaction(int32 InFaction);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSetTeam(int32 InTeam);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSetColor(int32 InColor);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSetSlot(int32 InSlot);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerKickPlayer(ARTSLobbyPlayerState* Target);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSetMap(const FString& MapName);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSetGameMode(const FString& GameMode);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerStartMatch();
};
