// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SteamRTSTypes.h"
#include "RTSMenuWidget.generated.h"

class USteamRTSSubsystem;
class USteamLobbySubsystem;

class UButton;
class UTextBlock;
class UEditableTextBox;
class UListView;
class URTSHostConfigWidget;
class URTSLobbyInfoObject;

/**
 * Base widget for the main menu. Wraps the SteamRTS subsystems so the Blueprint
 * only handles layout: call HostLobby/RefreshLobbies/JoinDiscoveredLobby and
 * implement the OnLobbyListReady/OnError/OnJoinResult events for the visuals.
 */
UCLASS()
class RTSPROJECT_API URTSMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** Create a lobby as host and travel to the lobby map. */
	UFUNCTION(BlueprintCallable, Category = "SteamRTS|Menu")
	void HostLobby(const FRTSLobbySettings& Settings);

	/** Search for joinable lobbies; results delivered via OnLobbyListReady. */
	UFUNCTION(BlueprintCallable, Category = "SteamRTS|Menu")
	void RefreshLobbies(const FString& MapFilter, bool bFriendsOnly = false);

	/** Join a lobby from the last discovered list by its index. */
	UFUNCTION(BlueprintCallable, Category = "SteamRTS|Menu")
	void JoinDiscoveredLobby(int32 Index);

	/**
	 * Last discovered lobbies wrapped as UObjects, in the same order as the
	 * subsystem's GetDiscoveredLobbies() (the order JoinDiscoveredLobby indexes).
	 * Use these as UListView items (a ListView only accepts UObject* items, not the
	 * FRTSLobbyInfo struct). The entry widget reads its lobby off the bound object.
	 * Populate LobbyListView from OnLobbyListReady in the BP with this array.
	 */
	UFUNCTION(BlueprintPure, Category = "SteamRTS|Menu")
	TArray<URTSLobbyInfoObject*> GetDiscoveredLobbyObjects() const;

	/** Create and show the host-config popup on top of the menu. */
	UFUNCTION(BlueprintCallable, Category = "SteamRTS|Menu")
	void OpenHostConfig();

	/** Local Steam display name, or empty if unavailable. */
	UFUNCTION(BlueprintPure, Category = "SteamRTS|Menu")
	FString GetLocalPlayerName() const;

	UFUNCTION(BlueprintPure, Category = "SteamRTS|Menu")
	bool IsSteamAvailable() const;

	// --- Implement these in the Blueprint for visuals ---

	UFUNCTION(BlueprintImplementableEvent, Category = "SteamRTS|Menu")
	void OnLobbyListReady(const TArray<FRTSLobbyInfo>& Lobbies);

	UFUNCTION(BlueprintImplementableEvent, Category = "SteamRTS|Menu")
	void OnError(ERTSSteamError ErrorCode, const FString& Message);

	UFUNCTION(BlueprintImplementableEvent, Category = "SteamRTS|Menu")
	void OnJoinResult(bool bSuccess);

protected:
	USteamRTSSubsystem* GetSteamRTS() const;
	USteamLobbySubsystem* GetLobby() const;

	// --- Auto-bound widgets (name them identically in the WBP). All optional. ---

	/** Click -> HostLobby using the bound input fields / settings defaults. */
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	UButton* HostButton;

	/** Click -> RefreshLobbies using MapFilterInput. */
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	UButton* RefreshButton;

	/** Click -> join the lobby currently selected in LobbyListView. */
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	UButton* JoinButton;

	/** Optional map name / filter used by the lobby search. */
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	UEditableTextBox* MapNameInput;

	/** Displays the local Steam player name. */
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	UTextBlock* PlayerNameText;

	/** Selection source for JoinButton; populate from OnLobbyListReady in the BP. */
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	UListView* LobbyListView;

	/** WBP class spawned on top of the menu when HostButton is clicked. Assign in the menu WBP. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SteamRTS|Menu")
	TSubclassOf<URTSHostConfigWidget> HostConfigWidgetClass;

	/** Z-order used when adding the host-config popup to the viewport (above the menu HUD). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SteamRTS|Menu")
	int32 HostConfigZOrder = 10;

	/** Live instance of the host-config popup, reused to avoid stacking duplicates. */
	UPROPERTY(Transient)
	URTSHostConfigWidget* HostConfigWidget;

	/** Menu visibility captured before hiding, restored when the popup closes. */
	ESlateVisibility CachedVisibility = ESlateVisibility::Visible;

	/**
	 * UObject wrappers for the last discovered lobbies, used as LobbyListView items.
	 * Held as a UPROPERTY so the wrappers stay alive (GC) while shown; rebuilt on
	 * every OnLobbyListUpdated, mirroring USteamLobbySubsystem::GetDiscoveredLobbies().
	 */
	UPROPERTY(Transient)
	TArray<URTSLobbyInfoObject*> LobbyObjects;

private:
	UFUNCTION()
	void HandleLobbyListUpdated(const TArray<FRTSLobbyInfo>& Lobbies);

	UFUNCTION()
	void HandleLobbyJoined(bool bSuccess);

	UFUNCTION()
	void HandleError(ERTSSteamError ErrorCode, const FString& Message);

	// Native click handlers wired to the bound buttons.
	UFUNCTION()
	void OnHostClicked();

	UFUNCTION()
	void OnRefreshClicked();

	UFUNCTION()
	void OnJoinClicked();

	/** Restore the menu's visibility when the host-config popup closes. */
	UFUNCTION()
	void HandleHostConfigClosed();
};
