// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SteamRTSTypes.generated.h"

/** Visibility of a created lobby. */
UENUM(BlueprintType)
enum class ERTSLobbyVisibility : uint8
{
	/** Only joinable via direct invite. */
	Private,
	/** Joinable by Steam friends. */
	FriendsOnly,
	/** Listed in the public lobby browser. */
	Public
};

/** Categories of recoverable SteamRTS errors surfaced to UI. */
UENUM(BlueprintType)
enum class ERTSSteamError : uint8
{
	None,
	NoOnlineSubsystem,
	CreateLobbyFailed,
	FindLobbiesFailed,
	JoinLobbyFailed,
	LobbyFull,
	VersionMismatch,
	HostDisconnected,
	LobbyDestroyed,
	NetworkTimeout
};

/** Settings used by the host when creating a lobby. */
USTRUCT(BlueprintType)
struct FRTSLobbySettings
{
	GENERATED_BODY()

	/** Display name of the lobby. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SteamRTS")
	FString LobbyName;

	/** Maximum number of players allowed in the lobby. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SteamRTS")
	int32 MaxPlayers = 8;

	/** Who is allowed to find/join this lobby. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SteamRTS")
	ERTSLobbyVisibility Visibility = ERTSLobbyVisibility::Public;

	/** Map the match will be played on. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SteamRTS")
	FString MapName;

	/** Game mode identifier. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SteamRTS")
	FString GameMode;

	/** Optional free-form metadata advertised on the session. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SteamRTS")
	TMap<FString, FString> CustomMetadata;
};

/** Summary information about a discovered lobby (used by the lobby browser). */
USTRUCT(BlueprintType)
struct FRTSLobbyInfo
{
	GENERATED_BODY()

	/** Opaque id used to join this lobby. */
	UPROPERTY(BlueprintReadOnly, Category = "SteamRTS")
	FString LobbyId;

	/** Display name of the lobby. */
	UPROPERTY(BlueprintReadOnly, Category = "SteamRTS")
	FString LobbyName;

	/** Steam display name of the host. */
	UPROPERTY(BlueprintReadOnly, Category = "SteamRTS")
	FString HostName;

	UPROPERTY(BlueprintReadOnly, Category = "SteamRTS")
	int32 CurrentPlayers = 0;

	UPROPERTY(BlueprintReadOnly, Category = "SteamRTS")
	int32 MaxPlayers = 0;

	/** Ping in ms, or -1 if unavailable. */
	UPROPERTY(BlueprintReadOnly, Category = "SteamRTS")
	int32 PingInMs = -1;

	UPROPERTY(BlueprintReadOnly, Category = "SteamRTS")
	FString MapName;

	UPROPERTY(BlueprintReadOnly, Category = "SteamRTS")
	FString GameMode;

	/** True if the lobby host is a Steam friend (member list is not available pre-join). */
	UPROPERTY(BlueprintReadOnly, Category = "SteamRTS")
	bool bHostIsFriend = false;

	/** Index into the underlying search results array. */
	UPROPERTY(BlueprintReadOnly, Category = "SteamRTS")
	int32 SearchResultIndex = -1;
};

/** Per-player state inside a lobby. */
USTRUCT(BlueprintType)
struct FRTSPlayerInfo
{
	GENERATED_BODY()

	/** Steam id as a string. */
	UPROPERTY(BlueprintReadOnly, Category = "SteamRTS")
	FString PlayerId;

	UPROPERTY(BlueprintReadOnly, Category = "SteamRTS")
	FString DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "SteamRTS")
	bool bIsHost = false;

	UPROPERTY(BlueprintReadOnly, Category = "SteamRTS")
	bool bReady = false;

	UPROPERTY(BlueprintReadOnly, Category = "SteamRTS")
	int32 Faction = 0;

	UPROPERTY(BlueprintReadOnly, Category = "SteamRTS")
	int32 Team = 0;

	UPROPERTY(BlueprintReadOnly, Category = "SteamRTS")
	int32 Color = 0;

	UPROPERTY(BlueprintReadOnly, Category = "SteamRTS")
	int32 Slot = -1;
};

/** Steam friend summary. */
USTRUCT(BlueprintType)
struct FRTSFriendInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "SteamRTS")
	FString PlayerId;

	UPROPERTY(BlueprintReadOnly, Category = "SteamRTS")
	FString DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "SteamRTS")
	bool bIsOnline = false;

	UPROPERTY(BlueprintReadOnly, Category = "SteamRTS")
	bool bIsPlayingThisGame = false;

	/** True if this friend is currently in a joinable session. */
	UPROPERTY(BlueprintReadOnly, Category = "SteamRTS")
	bool bIsJoinable = false;

	/** Free-form "current game" / presence string. */
	UPROPERTY(BlueprintReadOnly, Category = "SteamRTS")
	FString PresenceStatus;
};
