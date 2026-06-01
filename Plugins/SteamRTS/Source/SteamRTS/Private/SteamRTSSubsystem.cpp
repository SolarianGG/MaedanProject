// Copyright Epic Games, Inc. All Rights Reserved.

#include "SteamRTSSubsystem.h"
#include "SteamLobbySubsystem.h"
#include "SteamFriendsSubsystem.h"
#include "SteamRTSSettings.h"
#include "SteamRTSLog.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemNames.h"
#include "OnlineSubsystemUtils.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Interfaces/OnlinePresenceInterface.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

void USteamRTSSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	IOnlineSubsystem* const OnlineSub = Online::GetSubsystem(GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr);
	bSteamAvailable = (OnlineSub != nullptr) && (OnlineSub->GetSubsystemName() == STEAM_SUBSYSTEM);

	if (GEngine)
	{
		NetworkFailureHandle = GEngine->OnNetworkFailure().AddUObject(this, &USteamRTSSubsystem::HandleNetworkFailure);
	}

	UE_LOG(LogSteamRTS, Log, TEXT("USteamRTSSubsystem initialized. SteamAvailable=%d"), bSteamAvailable);
}

void USteamRTSSubsystem::Deinitialize()
{
	if (GEngine)
	{
		GEngine->OnNetworkFailure().Remove(NetworkFailureHandle);
	}
	UE_LOG(LogSteamRTS, Log, TEXT("USteamRTSSubsystem deinitialized."));
	Super::Deinitialize();
}

void USteamRTSSubsystem::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	UE_LOG(LogSteamRTS, Warning, TEXT("Network failure (%d): %s"), (int32)FailureType, *ErrorString);

	const ERTSSteamError ErrCode = (FailureType == ENetworkFailure::ConnectionTimeout)
		? ERTSSteamError::NetworkTimeout : ERTSSteamError::HostDisconnected;
	ReportError(ErrCode, ErrorString);
}

FString USteamRTSSubsystem::GetLocalPlayerName() const
{
	if (IOnlineSubsystem* const OnlineSub = Online::GetSubsystem(GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr))
	{
		if (const IOnlineIdentityPtr Identity = OnlineSub->GetIdentityInterface())
		{
			return Identity->GetPlayerNickname(0);
		}
	}
	return FString();
}

void USteamRTSSubsystem::ReportError(ERTSSteamError ErrorCode, const FString& Message)
{
	UE_LOG(LogSteamRTS, Warning, TEXT("SteamRTS error %d: %s"), (int32)ErrorCode, *Message);
	OnError.Broadcast(ErrorCode, Message);
}

void USteamRTSSubsystem::SetPresenceStatus(const FString& StatusText)
{
	const USteamRTSSettings* const Config = GetDefault<USteamRTSSettings>();
	if (Config && !Config->bEnablePresence)
	{
		return;
	}

	IOnlineSubsystem* const OnlineSub = Online::GetSubsystem(GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr);
	const IOnlineIdentityPtr Identity = OnlineSub ? OnlineSub->GetIdentityInterface() : nullptr;
	const IOnlinePresencePtr Presence = OnlineSub ? OnlineSub->GetPresenceInterface() : nullptr;
	if (!Identity.IsValid() || !Presence.IsValid())
	{
		return;
	}

	const TSharedPtr<const FUniqueNetId> LocalUserId = Identity->GetUniquePlayerId(0);
	if (!LocalUserId.IsValid())
	{
		return;
	}

	FOnlineUserPresenceStatus Status;
	Status.StatusStr = StatusText;
	Status.State = EOnlinePresenceState::Online;
	Presence->SetPresence(*LocalUserId, Status);
}

USteamLobbySubsystem* USteamRTSSubsystem::GetLobbySubsystem() const
{
	return GetGameInstance() ? GetGameInstance()->GetSubsystem<USteamLobbySubsystem>() : nullptr;
}

USteamFriendsSubsystem* USteamRTSSubsystem::GetFriendsSubsystem() const
{
	return GetGameInstance() ? GetGameInstance()->GetSubsystem<USteamFriendsSubsystem>() : nullptr;
}
