// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Engine/EngineTypes.h"
#include "RTSGameInstance.generated.h"

class UNetDriver;

/**
 * Game instance that surfaces network failure diagnostics. Binds to
 * GEngine->OnNetworkFailure() so connection/listen errors during lobby and
 * match travel are logged (and shown on screen) instead of silently dropping
 * the player back to the main menu.
 */
UCLASS()
class RTSPROJECT_API URTSGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
	virtual void Shutdown() override;

private:
	void NetworkFailureMessage(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);

	FDelegateHandle NetworkFailureHandle;
};
