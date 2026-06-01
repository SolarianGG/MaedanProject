// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SteamRTSTypes.h"
#include "RTSLobbyInfoObject.generated.h"

/**
 * Thin UObject wrapper around an FRTSLobbyInfo so a discovered lobby can be used
 * as a UListView item: a ListView only accepts UObject* items, not the
 * FRTSLobbyInfo struct. This mirrors how the player roster uses the live
 * ARTSLobbyPlayerState actors as its list items (see URTSLobbyWidget::
 * GetRosterPlayerStates), giving the lobby browser the same IUserObjectListEntry
 * workflow as the roster.
 */
UCLASS(BlueprintType)
class RTSPROJECT_API URTSLobbyInfoObject : public UObject
{
	GENERATED_BODY()

public:
	/** The discovered lobby this entry represents. */
	UPROPERTY(BlueprintReadOnly, Category = "SteamRTS|Lobby")
	FRTSLobbyInfo Info;
};
