// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "RTSLobbyPlayerEntryWidget.generated.h"

class ARTSLobbyPlayerState;

/**
 * ListView entry for a single player in the lobby roster. URTSLobbyWidget's
 * PlayerListView is populated with ARTSLobbyPlayerState items (see
 * GetRosterPlayerStates); this widget binds to its assigned player state's
 * OnLobbyStateChanged delegate so the WBP can refresh its inner widgets
 * (ready indicator, name, team, faction, color, ...) whenever that player
 * updates their lobby status. Subclass as WBP_PlayerLobbyEntry and implement
 * OnPlayerStateUpdated for the visuals.
 */
UCLASS()
class RTSPROJECT_API URTSLobbyPlayerEntryWidget : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

public:
	virtual void NativeDestruct() override;

	// IUserObjectListEntry
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
	virtual void NativeOnEntryReleased() override;

	/** The player state this entry currently represents, if any. */
	UFUNCTION(BlueprintPure, Category = "SteamRTS|Lobby")
	ARTSLobbyPlayerState* GetLobbyPlayer() const { return BoundPlayer; }

	/**
	 * Implement in the WBP to (re)populate the inner widgets from the player state.
	 * Called once when the entry is assigned a player, and again on every replicated
	 * lobby-state change (ready/team/faction/color/slot/host) for that player.
	 * Player may be null when the entry is recycled with no item.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "SteamRTS|Lobby")
	void OnPlayerStateUpdated(ARTSLobbyPlayerState* Player);

private:
	/** Stop listening to the currently bound player state, if any. */
	void UnbindCurrentPlayer();

	UFUNCTION()
	void HandlePlayerStateChanged();

	/** Player state we are currently bound to. */
	UPROPERTY()
	ARTSLobbyPlayerState* BoundPlayer = nullptr;
};
