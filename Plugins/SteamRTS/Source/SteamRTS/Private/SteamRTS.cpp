// Copyright Epic Games, Inc. All Rights Reserved.

#include "SteamRTS.h"
#include "SteamRTSLog.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemNames.h"

#define LOCTEXT_NAMESPACE "FSteamRTSModule"

DEFINE_LOG_CATEGORY(LogSteamRTS);

void FSteamRTSModule::StartupModule()
{
	IOnlineSubsystem* const OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		UE_LOG(LogSteamRTS, Log, TEXT("SteamRTS started. OnlineSubsystem = '%s'."),
			*OnlineSub->GetSubsystemName().ToString());

		if (OnlineSub->GetSubsystemName() != STEAM_SUBSYSTEM)
		{
			UE_LOG(LogSteamRTS, Warning,
				TEXT("Default OnlineSubsystem is not Steam. Set DefaultPlatformService=Steam in DefaultEngine.ini for full functionality."));
		}
	}
	else
	{
		UE_LOG(LogSteamRTS, Error, TEXT("SteamRTS started but no OnlineSubsystem is available."));
	}
}

void FSteamRTSModule::ShutdownModule()
{
	UE_LOG(LogSteamRTS, Log, TEXT("SteamRTS shutting down."));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSteamRTSModule, SteamRTS)
