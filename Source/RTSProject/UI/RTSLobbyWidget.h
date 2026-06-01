// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/SlateEnums.h"
#include "SteamRTSTypes.h"
#include "RTSLobbyWidget.generated.h"

class ARTSLobbyGameState;
class ARTSLobbyPlayerState;
class USteamLobbySubsystem;
class USteamFriendsSubsystem;

class UButton;
class UTextBlock;
class UComboBoxString;
class UListView;

/**
 * Base widget for the pre-game lobby. Subscribes to the replicated lobby state
 * (config + per-player) and exposes thin wrappers over ARTSLobbyPlayerState so
 * the Blueprint only handles layout. Implement OnRosterChanged / OnConfigChanged
 * for the visuals.
 */
UCLASS()
class RTSPROJECT_API URTSLobbyWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// --- Local player actions ---
	UFUNCTION(BlueprintCallable, Category = "SteamRTS|Lobby")
	void SetReady(bool bReady);

	UFUNCTION(BlueprintCallable, Category = "SteamRTS|Lobby")
	void SetTeam(int32 Team);

	UFUNCTION(BlueprintCallable, Category = "SteamRTS|Lobby")
	void SetSlot(int32 PlayerSlot);

	UFUNCTION(BlueprintCallable, Category = "SteamRTS|Lobby")
	void LeaveLobby();

	/** Open the Steam overlay invite dialog for the current lobby session. */
	UFUNCTION(BlueprintCallable, Category = "SteamRTS|Lobby")
	void OpenFriendInviteOverlay();

	// --- Host-only actions (validated server-side) ---
	UFUNCTION(BlueprintCallable, Category = "SteamRTS|Lobby")
	void HostStartMatch();

	UFUNCTION(BlueprintCallable, Category = "SteamRTS|Lobby")
	void HostKick(ARTSLobbyPlayerState* Target);

	UFUNCTION(BlueprintCallable, Category = "SteamRTS|Lobby")
	void HostSetMap(const FString& MapName);

	// --- Queries ---
	UFUNCTION(BlueprintPure, Category = "SteamRTS|Lobby")
	TArray<FRTSPlayerInfo> GetRoster() const;

	/**
	 * Roster as the live ARTSLobbyPlayerState actors, in the same order as GetRoster().
	 * Use these as UListView items (a ListView only accepts UObject* items, not the
	 * FRTSPlayerInfo struct). The entry widget reads its player off the bound object.
	 */
	UFUNCTION(BlueprintPure, Category = "SteamRTS|Lobby")
	TArray<ARTSLobbyPlayerState*> GetRosterPlayerStates() const;

	UFUNCTION(BlueprintPure, Category = "SteamRTS|Lobby")
	ARTSLobbyPlayerState* GetLocalLobbyPlayerState() const;

	UFUNCTION(BlueprintPure, Category = "SteamRTS|Lobby")
	bool IsLocalPlayerHost() const;

	UFUNCTION(BlueprintPure, Category = "SteamRTS|Lobby")
	bool AreAllPlayersReady() const;

	// --- Implement these in the Blueprint for visuals ---
	UFUNCTION(BlueprintImplementableEvent, Category = "SteamRTS|Lobby")
	void OnRosterChanged();

	UFUNCTION(BlueprintImplementableEvent, Category = "SteamRTS|Lobby")
	void OnConfigChanged();

protected:
	ARTSLobbyGameState* GetLobbyGameState() const;
	USteamLobbySubsystem* GetLobbySubsystem() const;
	USteamFriendsSubsystem* GetFriends() const;

	// --- Auto-bound widgets (name them identically in the WBP). All optional. ---

	/** Click -> toggle the local player's ready state. */
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	UButton* ReadyButton;

	/** Click -> leave the lobby. */
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	UButton* LeaveButton;

	/** Click -> open the Steam invite overlay for the lobby session. */
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	UButton* InviteButton;

	/** Click -> host starts the match (auto-enabled only for the host when all are ready). */
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	UButton* StartMatchButton;

	/** Selection -> SetTeam (the selected option index is used as the team). */
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	UComboBoxString* TeamComboBox;

	/** Shows the selected map. */
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	UTextBlock* MapText;

	/** Player roster list; populate from OnRosterChanged in the BP. */
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	UListView* PlayerListView;

private:
	/** (Re)bind the per-player change delegate for every player in the roster. */
	void RebindPlayerStates();

	/** Update bound widgets (start button enabled state, map/mode text). */
	void RefreshBoundWidgets();

	/**
	 * Rebuild the team selector so the number of options matches the lobby's
	 * maximum player count (ARTSLobbyGameState::MaxPlayers), then reflect the
	 * local player's current team. Cheap and idempotent: only rebuilds the
	 * option list when the team count actually changed.
	 */
	void RefreshTeamOptions();

	UFUNCTION()
	void HandlePlayerStateChanged();

	UFUNCTION()
	void HandleConfigChanged();

	/**
	 * Remove the lobby UI when the match starts. The lobby uses seamless travel
	 * (ARTSLobbyGameMode::bUseSeamlessTravel), which keeps the GameViewportClient —
	 * and therefore every widget added to the viewport — alive across the level
	 * change, so the lobby would otherwise linger on top of the loaded game world.
	 * (A hard travel clears viewport widgets via RemoveAllViewportWidgets; seamless
	 * travel does not.)
	 */
	void HandleSeamlessTravelStart(UWorld* CurrentWorld, const FString& LevelName);

	// Native handlers wired to the bound widgets.
	UFUNCTION()
	void OnReadyClicked();

	UFUNCTION()
	void OnLeaveClicked();

	UFUNCTION()
	void OnStartClicked();

	UFUNCTION()
	void OnInviteClicked();

	UFUNCTION()
	void OnTeamSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	/** Player states we have bound a delegate to, for clean teardown. */
	UPROPERTY()
	TArray<ARTSLobbyPlayerState*> BoundPlayerStates;

	FTimerHandle RebindTimerHandle;

	/** Handle for the FWorldDelegates::OnSeamlessTravelStart binding, removed on destruct. */
	FDelegateHandle SeamlessTravelStartHandle;
};
