// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "SteamRTSTypes.h"
#include "RTSLobbyRoomEntryWidget.generated.h"

/**
 * ListView entry for a single discovered lobby in the menu's lobby browser.
 * URTSMenuWidget's LobbyListView is populated with URTSLobbyInfoObject items
 * (see URTSMenuWidget::GetDiscoveredLobbyObjects); this widget reads its assigned
 * lobby off the bound object. Subclass as WBP_LobbyRoomEntry and implement
 * OnLobbyInfoUpdated for the visuals.
 *
 * Counterpart to URTSLobbyPlayerEntryWidget. Unlike a player entry, a discovered
 * lobby is an immutable snapshot from the last search, so there is no per-entry
 * delegate to bind: the entry is fully refreshed whenever the list is rebuilt.
 */
UCLASS()
class RTSPROJECT_API URTSLobbyRoomEntryWidget : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

public:
	// IUserObjectListEntry
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

	/** The lobby this entry currently represents. */
	UFUNCTION(BlueprintPure, Category = "SteamRTS|Lobby")
	FRTSLobbyInfo GetLobbyInfo() const { return LobbyInfo; }

	/**
	 * Implement in the WBP to (re)populate the inner widgets from the lobby info
	 * (name, host, player count, ping, map, ...). Called whenever this entry is
	 * assigned a lobby.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "SteamRTS|Lobby")
	void OnLobbyInfoUpdated(const FRTSLobbyInfo& Info);

private:
	/** Cached copy of the bound lobby's info. */
	UPROPERTY()
	FRTSLobbyInfo LobbyInfo;
};
