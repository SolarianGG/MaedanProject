// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "SteamRTSTypes.h"
#include "SteamFriendsSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFriendsListUpdated, const TArray<FRTSFriendInfo>&, Friends);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFriendInviteReceived, const FString&, FromPlayerName);

/**
 * Exposes the Steam friends list and friend-invite flow. Reads the friends
 * list via IOnlineFriends and handles incoming "Join Game" / invite-accepted
 * callbacks by joining the corresponding session.
 */
UCLASS()
class STEAMRTS_API USteamFriendsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Asynchronously read the Steam friends list; results via OnFriendsListUpdated. */
	UFUNCTION(BlueprintCallable, Category = "SteamRTS|Friends")
	void ReadFriendsList();

	/** Last cached friends list. */
	UFUNCTION(BlueprintPure, Category = "SteamRTS|Friends")
	const TArray<FRTSFriendInfo>& GetCachedFriends() const { return CachedFriends; }

	/** Send a session invite to a friend by Steam id string. */
	UFUNCTION(BlueprintCallable, Category = "SteamRTS|Friends")
	void InviteFriend(const FString& FriendId);

	/** Open the Steam overlay invite dialog for the current session. */
	UFUNCTION(BlueprintCallable, Category = "SteamRTS|Friends")
	void OpenInviteOverlay();

	UPROPERTY(BlueprintAssignable, Category = "SteamRTS|Friends")
	FOnFriendsListUpdated OnFriendsListUpdated;

	UPROPERTY(BlueprintAssignable, Category = "SteamRTS|Friends")
	FOnFriendInviteReceived OnFriendInviteReceived;

private:
	void HandleReadFriendsListComplete(int32 LocalUserNum, bool bWasSuccessful, const FString& ListName, const FString& ErrorStr);
	void HandleSessionUserInviteAccepted(bool bWasSuccessful, int32 ControllerId, TSharedPtr<const FUniqueNetId> UserId, const FOnlineSessionSearchResult& InviteResult);
	void RebuildCachedFriends();

	FDelegateHandle SessionInviteAcceptedHandle;

	UPROPERTY()
	TArray<FRTSFriendInfo> CachedFriends;
};
