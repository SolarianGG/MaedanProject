// Copyright Epic Games, Inc. All Rights Reserved.

#include "SteamFriendsSubsystem.h"
#include "SteamLobbySubsystem.h"
#include "SteamRTSSettings.h"
#include "SteamRTSLog.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Interfaces/OnlineFriendsInterface.h"
#include "Interfaces/OnlineExternalUIInterface.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Interfaces/OnlinePresenceInterface.h"
#include "Engine/GameInstance.h"

void USteamFriendsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (IOnlineSubsystem* const OnlineSub = Online::GetSubsystem(GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr))
	{
		if (const IOnlineSessionPtr Session = OnlineSub->GetSessionInterface())
		{
			SessionInviteAcceptedHandle = Session->AddOnSessionUserInviteAcceptedDelegate_Handle(
				FOnSessionUserInviteAcceptedDelegate::CreateUObject(this, &USteamFriendsSubsystem::HandleSessionUserInviteAccepted));
		}
	}

	UE_LOG(LogSteamRTS, Log, TEXT("USteamFriendsSubsystem initialized."));
}

void USteamFriendsSubsystem::Deinitialize()
{
	if (IOnlineSubsystem* const OnlineSub = Online::GetSubsystem(GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr))
	{
		if (const IOnlineSessionPtr Session = OnlineSub->GetSessionInterface())
		{
			Session->ClearOnSessionUserInviteAcceptedDelegate_Handle(SessionInviteAcceptedHandle);
		}
	}
	Super::Deinitialize();
}

void USteamFriendsSubsystem::ReadFriendsList()
{
	IOnlineSubsystem* const OnlineSub = Online::GetSubsystem(GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr);
	const IOnlineFriendsPtr Friends = OnlineSub ? OnlineSub->GetFriendsInterface() : nullptr;
	if (!Friends.IsValid())
	{
		UE_LOG(LogSteamRTS, Error, TEXT("ReadFriendsList: no friends interface."));
		OnFriendsListUpdated.Broadcast(CachedFriends);
		return;
	}

	Friends->ReadFriendsList(0, EFriendsLists::ToString(EFriendsLists::Default),
		FOnReadFriendsListComplete::CreateUObject(this, &USteamFriendsSubsystem::HandleReadFriendsListComplete));
}

void USteamFriendsSubsystem::HandleReadFriendsListComplete(int32 LocalUserNum, bool bWasSuccessful, const FString& ListName, const FString& ErrorStr)
{
	UE_LOG(LogSteamRTS, Log, TEXT("HandleReadFriendsListComplete: success=%d error='%s'"), bWasSuccessful, *ErrorStr);

	if (bWasSuccessful)
	{
		RebuildCachedFriends();
	}
	OnFriendsListUpdated.Broadcast(CachedFriends);
}

void USteamFriendsSubsystem::RebuildCachedFriends()
{
	CachedFriends.Reset();

	IOnlineSubsystem* const OnlineSub = Online::GetSubsystem(GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr);
	const IOnlineFriendsPtr Friends = OnlineSub ? OnlineSub->GetFriendsInterface() : nullptr;
	if (!Friends.IsValid())
	{
		return;
	}

	TArray<TSharedRef<FOnlineFriend>> FriendList;
	Friends->GetFriendsList(0, EFriendsLists::ToString(EFriendsLists::Default), FriendList);

	for (const TSharedRef<FOnlineFriend>& Friend : FriendList)
	{
		FRTSFriendInfo Info;
		Info.PlayerId = Friend->GetUserId()->ToString();
		Info.DisplayName = Friend->GetDisplayName();

		const FOnlineUserPresence& Presence = Friend->GetPresence();
		Info.bIsOnline = Presence.bIsOnline;
		Info.bIsPlayingThisGame = Presence.bIsPlayingThisGame;
		Info.bIsJoinable = Presence.bIsJoinable;
		Info.PresenceStatus = Presence.Status.StatusStr;

		CachedFriends.Add(Info);
	}
}

void USteamFriendsSubsystem::InviteFriend(const FString& FriendId)
{
	if (!GetDefault<USteamRTSSettings>()->bEnableInvites)
	{
		UE_LOG(LogSteamRTS, Log, TEXT("InviteFriend: invites are disabled in settings."));
		return;
	}

	IOnlineSubsystem* const OnlineSub = Online::GetSubsystem(GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr);
	const IOnlineSessionPtr Session = OnlineSub ? OnlineSub->GetSessionInterface() : nullptr;
	const IOnlineIdentityPtr Identity = OnlineSub ? OnlineSub->GetIdentityInterface() : nullptr;
	if (!Session.IsValid() || !Identity.IsValid())
	{
		UE_LOG(LogSteamRTS, Error, TEXT("InviteFriend: missing session/identity interface."));
		return;
	}

	const TSharedPtr<const FUniqueNetId> FriendNetId = Identity->CreateUniquePlayerId(FriendId);
	if (FriendNetId.IsValid())
	{
		Session->SendSessionInviteToFriend(0, NAME_GameSession, *FriendNetId);
	}
}

void USteamFriendsSubsystem::OpenInviteOverlay()
{
	if (!GetDefault<USteamRTSSettings>()->bEnableInvites)
	{
		UE_LOG(LogSteamRTS, Log, TEXT("OpenInviteOverlay: invites are disabled in settings."));
		return;
	}

	IOnlineSubsystem* const OnlineSub = Online::GetSubsystem(GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr);
	const IOnlineExternalUIPtr ExternalUI = OnlineSub ? OnlineSub->GetExternalUIInterface() : nullptr;
	if (ExternalUI.IsValid())
	{
		ExternalUI->ShowInviteUI(0, NAME_GameSession);
	}
	else
	{
		UE_LOG(LogSteamRTS, Error, TEXT("OpenInviteOverlay: no external UI interface."));
	}
}

void USteamFriendsSubsystem::HandleSessionUserInviteAccepted(bool bWasSuccessful, int32 ControllerId, TSharedPtr<const FUniqueNetId> UserId, const FOnlineSessionSearchResult& InviteResult)
{
	UE_LOG(LogSteamRTS, Log, TEXT("HandleSessionUserInviteAccepted: success=%d valid=%d"), bWasSuccessful, InviteResult.IsValid());

	OnFriendInviteReceived.Broadcast(InviteResult.Session.OwningUserName);

	if (!bWasSuccessful || !InviteResult.IsValid())
	{
		return;
	}

	USteamLobbySubsystem* const Lobby = GetGameInstance() ? GetGameInstance()->GetSubsystem<USteamLobbySubsystem>() : nullptr;
	if (!Lobby)
	{
		return;
	}

	// Join directly via the invite's search result.
	IOnlineSubsystem* const OnlineSub = Online::GetSubsystem(GetGameInstance()->GetWorld());
	const IOnlineSessionPtr Session = OnlineSub ? OnlineSub->GetSessionInterface() : nullptr;
	if (Session.IsValid())
	{
		Lobby->JoinFromInvite(InviteResult);
	}
}
