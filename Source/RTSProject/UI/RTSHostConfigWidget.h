// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SteamRTSTypes.h"
#include "RTSHostConfigWidget.generated.h"

class USteamRTSSubsystem;
class USteamLobbySubsystem;

class UButton;
class UEditableTextBox;
class UComboBoxString;
class UPanelWidget;
class URTSMapEntryWidget;

/** Broadcast when the popup dismisses itself, so the owning menu can restore its visibility. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHostConfigClosed);

/**
 * Lobby-creation popup shown on top of the main menu. The host enters a lobby name, picks a max
 * player count (MaxPlayersCombo) and a map (one of the pre-authored URTSMapEntryWidget tiles in
 * MapEntryContainer). Create -> create the lobby and travel; Close -> dismiss back to the menu.
 */
UCLASS()
class RTSPROJECT_API URTSHostConfigWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** Set the currently selected map by path (also callable from Blueprint). */
	UFUNCTION(BlueprintCallable, Category = "SteamRTS|Host")
	void SetSelectedMap(const FString& MapPath);

	/** Dismiss the popup (RemoveFromParent) and broadcast OnClosed. */
	UFUNCTION(BlueprintCallable, Category = "SteamRTS|Host")
	void CloseConfig();

	/** Fired right before the popup is removed; the owning menu restores its visibility here. */
	FOnHostConfigClosed OnClosed;

	/** Fired when an error prevents lobby creation (e.g. no map selected). */
	UFUNCTION(BlueprintImplementableEvent, Category = "SteamRTS|Host")
	void OnError(ERTSSteamError ErrorCode, const FString& Message);

protected:
	USteamRTSSubsystem* GetSteamRTS() const;
	USteamLobbySubsystem* GetLobby() const;

	// --- Auto-bound widgets (name them identically in the WBP). All optional. ---

	/** Lobby name entered by the host. */
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	UEditableTextBox* LobbyNameInput;

	/** Max-player selector; options ("2","4",...) authored in the WBP. */
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	UComboBoxString* MaxPlayersCombo;

	/** Container (e.g. a WrapBox) holding the pre-authored URTSMapEntryWidget tiles. */
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	UPanelWidget* MapEntryContainer;

	/** Click -> create the lobby from the inputs and selected map. */
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	UButton* CreateButton;

	/** Click -> close the popup without creating a lobby. */
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	UButton* CloseButton;

	/** Map path of the currently selected entry; empty until a tile is clicked. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "SteamRTS|Host")
	FString SelectedMapPath;

private:
	UFUNCTION()
	void OnCreateClicked();

	UFUNCTION()
	void OnCloseClicked();

	UFUNCTION()
	void HandleMapEntrySelected(URTSMapEntryWidget* Entry);

	/** Bind every URTSMapEntryWidget child of MapEntryContainer to HandleMapEntrySelected. */
	void BindMapEntries();

	/** Parse the selected ComboBox option to a player count, falling back to project settings. */
	int32 GetSelectedMaxPlayers() const;
};
