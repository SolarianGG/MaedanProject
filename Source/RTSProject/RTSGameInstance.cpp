// Fill out your copyright notice in the Description page of Project Settings.

#include "RTSGameInstance.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/NetDriver.h"

void URTSGameInstance::Init()
{
	Super::Init();

	if (GEngine)
	{
		NetworkFailureHandle = GEngine->OnNetworkFailure().AddUObject(this, &URTSGameInstance::NetworkFailureMessage);
	}
}

void URTSGameInstance::Shutdown()
{
	if (GEngine)
	{
		GEngine->OnNetworkFailure().Remove(NetworkFailureHandle);
	}

	Super::Shutdown();
}

void URTSGameInstance::NetworkFailureMessage(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	const FString FailureTypeName = ENetworkFailure::ToString(FailureType);
	const FString DriverName = NetDriver ? NetDriver->NetDriverName.ToString() : TEXT("None");

	UE_LOG(LogNet, Error, TEXT("NetworkFailure: Type=%s Driver=%s Error=%s"),
		*FailureTypeName, *DriverName, *ErrorString);

	if (GEngine)
	{
		const FString ScreenMsg = FString::Printf(TEXT("NetworkFailure [%s]: %s"), *FailureTypeName, *ErrorString);
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, ScreenMsg);
	}
}
