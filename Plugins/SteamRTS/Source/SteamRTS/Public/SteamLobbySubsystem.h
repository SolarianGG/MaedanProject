// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "SteamRTSTypes.h"
#include "SteamLobbySubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLobbyCreated, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLobbyJoined, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLobbyLeft);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLobbyListUpdated, const TArray<FRTSLobbyInfo>&, Lobbies);

/**
 * Wraps the Steam-backed IOnlineSessionInterface to provide RTS lobby
 * create / find / join / leave operations. All operations are async and
 * report completion through Blueprint-assignable delegates.
 */
UCLASS()
class STEAMRTS_API USteamLobbySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Create a lobby as the host and travel to the lobby map as a listen server. */
	UFUNCTION(BlueprintCallable, Category = "SteamRTS|Lobby")
	void CreateLobby(const FRTSLobbySettings& Settings);

	/** Search for joinable lobbies. Results are delivered via OnLobbyListUpdated. */
	UFUNCTION(BlueprintCallable, Category = "SteamRTS|Lobby")
	void FindLobbies(const FString& MapFilter, int32 MaxResults = 100, bool bFriendsOnly = false);

	/** Join a lobby previously returned by FindLobbies. */
	UFUNCTION(BlueprintCallable, Category = "SteamRTS|Lobby")
	void JoinLobby(const FRTSLobbyInfo& Lobby);

	/** Leave the current lobby / destroy the session if hosting. */
	UFUNCTION(BlueprintCallable, Category = "SteamRTS|Lobby")
	void LeaveLobby();

	/** Last set of discovered lobbies. */
	UFUNCTION(BlueprintPure, Category = "SteamRTS|Lobby")
	const TArray<FRTSLobbyInfo>& GetDiscoveredLobbies() const { return DiscoveredLobbies; }

	/** Lobby settings pre-filled from project settings (USteamRTSSettings). */
	UFUNCTION(BlueprintPure, Category = "SteamRTS|Lobby")
	FRTSLobbySettings GetDefaultLobbySettings() const;

	/** Settings the host used for the in-flight / current lobby. */
	const FRTSLobbySettings& GetPendingLobbySettings() const { return PendingCreateSettings; }

	/** Join directly from a search result delivered by an accepted invite. */
	void JoinFromInvite(const FOnlineSessionSearchResult& InviteResult);

	UPROPERTY(BlueprintAssignable, Category = "SteamRTS|Lobby")
	FOnLobbyCreated OnLobbyCreated;

	UPROPERTY(BlueprintAssignable, Category = "SteamRTS|Lobby")
	FOnLobbyJoined OnLobbyJoined;

	UPROPERTY(BlueprintAssignable, Category = "SteamRTS|Lobby")
	FOnLobbyLeft OnLobbyLeft;

	UPROPERTY(BlueprintAssignable, Category = "SteamRTS|Lobby")
	FOnLobbyListUpdated OnLobbyListUpdated;

private:
	IOnlineSessionPtr GetSessionInterface() const;

	/** Issue the actual session search using the pending find parameters. */
	void ExecuteSessionSearch();

	/** Locate a search result in the current SessionSearch by its stable lobby id. */
	const FOnlineSessionSearchResult* FindSearchResultByLobbyId(const FString& LobbyId) const;

	/** One-shot handler: resume a deferred friends-only search once the friends cache is ready. */
	UFUNCTION()
	void HandleFriendsReadyForFind(const TArray<FRTSFriendInfo>& Friends);

	// Completion handlers bound to the session interface.
	void HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleFindSessionsComplete(bool bWasSuccessful);
	void HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful);

	void JoinResolvedResult(const FOnlineSessionSearchResult& Result);

	void TravelToListenServerLobby();
	void TravelClientToSession();
	void TravelToMainMenu();

	FRTSLobbyInfo BuildLobbyInfoFromSearchResult(const FOnlineSessionSearchResult& Result, int32 Index, const TSet<FString>& FriendIds) const;

	void ReportError(ERTSSteamError ErrorCode, const FString& Message);

	// Delegate handles for cleanup.
	FDelegateHandle CreateSessionCompleteHandle;
	FDelegateHandle FindSessionsCompleteHandle;
	FDelegateHandle JoinSessionCompleteHandle;
	FDelegateHandle DestroySessionCompleteHandle;

	TSharedPtr<class FOnlineSessionSearch> SessionSearch;

	UPROPERTY()
	TArray<FRTSLobbyInfo> DiscoveredLobbies;

	/** Pending settings used while CreateSession is in flight. */
	FRTSLobbySettings PendingCreateSettings;

	/** Whether the in-flight FindLobbies query should keep only lobbies with friends. */
	bool bPendingFriendsOnly = false;

	/** Map filter saved for a (possibly deferred) FindLobbies query. */
	FString PendingFindMapFilter;

	/** Max results saved for a (possibly deferred) FindLobbies query. */
	int32 PendingFindMaxResults = 100;

	/** Result awaiting join after an existing session is torn down (invite flow). */
	TSharedPtr<FOnlineSessionSearchResult> PendingJoinResult;

	bool bIsHost = false;
};
