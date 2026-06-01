// Fill out your copyright notice in the Description page of Project Settings.

#include "RTSLobbyWidget.h"

#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/ComboBoxString.h"
#include "Components/ListView.h"

#include "RTSLobbyGameState.h"
#include "RTSLobbyPlayerState.h"
#include "SteamLobbySubsystem.h"
#include "SteamFriendsSubsystem.h"
#include "SteamRTSSettings.h"

void URTSLobbyWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ARTSLobbyGameState* LobbyGS = GetLobbyGameState())
	{
		LobbyGS->OnLobbyConfigChanged.AddDynamic(this, &URTSLobbyWidget::HandleConfigChanged);
	}

	if (ReadyButton)
	{
		ReadyButton->OnClicked.AddDynamic(this, &URTSLobbyWidget::OnReadyClicked);
	}
	if (LeaveButton)
	{
		LeaveButton->OnClicked.AddDynamic(this, &URTSLobbyWidget::OnLeaveClicked);
	}
	if (StartMatchButton)
	{
		StartMatchButton->OnClicked.AddDynamic(this, &URTSLobbyWidget::OnStartClicked);
	}
	if (InviteButton)
	{
		InviteButton->OnClicked.AddDynamic(this, &URTSLobbyWidget::OnInviteClicked);
	}
	if (TeamComboBox)
	{
		TeamComboBox->OnSelectionChanged.AddDynamic(this, &URTSLobbyWidget::OnTeamSelectionChanged);
	}

	// Seamless travel keeps the viewport (and this widget) alive across the lobby->match
	// level change, so tear the lobby UI down ourselves when the match starts.
	SeamlessTravelStartHandle = FWorldDelegates::OnSeamlessTravelStart.AddUObject(this, &URTSLobbyWidget::HandleSeamlessTravelStart);

	RebindPlayerStates();
	RefreshBoundWidgets();

	// Players can join after this widget is constructed; rebind periodically so
	// late joiners' state changes are observed. Cheap idempotent rebind.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(RebindTimerHandle, this, &URTSLobbyWidget::RebindPlayerStates, 0.5f, true);
	}
}

void URTSLobbyWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RebindTimerHandle);
	}

	if (SeamlessTravelStartHandle.IsValid())
	{
		FWorldDelegates::OnSeamlessTravelStart.Remove(SeamlessTravelStartHandle);
		SeamlessTravelStartHandle.Reset();
	}

	if (ARTSLobbyGameState* LobbyGS = GetLobbyGameState())
	{
		LobbyGS->OnLobbyConfigChanged.RemoveDynamic(this, &URTSLobbyWidget::HandleConfigChanged);
	}

	for (ARTSLobbyPlayerState* PS : BoundPlayerStates)
	{
		if (IsValid(PS))
		{
			PS->OnLobbyStateChanged.RemoveDynamic(this, &URTSLobbyWidget::HandlePlayerStateChanged);
		}
	}
	BoundPlayerStates.Reset();

	if (ReadyButton)
	{
		ReadyButton->OnClicked.RemoveDynamic(this, &URTSLobbyWidget::OnReadyClicked);
	}
	if (LeaveButton)
	{
		LeaveButton->OnClicked.RemoveDynamic(this, &URTSLobbyWidget::OnLeaveClicked);
	}
	if (StartMatchButton)
	{
		StartMatchButton->OnClicked.RemoveDynamic(this, &URTSLobbyWidget::OnStartClicked);
	}
	if (InviteButton)
	{
		InviteButton->OnClicked.RemoveDynamic(this, &URTSLobbyWidget::OnInviteClicked);
	}
	if (TeamComboBox)
	{
		TeamComboBox->OnSelectionChanged.RemoveDynamic(this, &URTSLobbyWidget::OnTeamSelectionChanged);
	}

	Super::NativeDestruct();
}

void URTSLobbyWidget::SetReady(bool bReady)
{
	if (ARTSLobbyPlayerState* PS = GetLocalLobbyPlayerState())
	{
		PS->SetReady(bReady);
	}
}

void URTSLobbyWidget::SetTeam(int32 Team)
{
	if (ARTSLobbyPlayerState* PS = GetLocalLobbyPlayerState())
	{
		PS->SetTeam(Team);
	}
}

void URTSLobbyWidget::SetSlot(int32 PlayerSlot)
{
	if (ARTSLobbyPlayerState* PS = GetLocalLobbyPlayerState())
	{
		PS->SetSlot(PlayerSlot);
	}
}

void URTSLobbyWidget::LeaveLobby()
{
	if (USteamLobbySubsystem* Lobby = GetLobbySubsystem())
	{
		Lobby->LeaveLobby();
	}

	// Session teardown is async (and the subsystem only auto-travels clients, not
	// the host or the no-session case), so return to the main menu here. The
	// subsystem is a GameInstanceSubsystem and survives the travel, so DestroySession
	// still completes in the background.
	const USteamRTSSettings* Settings = GetDefault<USteamRTSSettings>();
	const FString MenuMap = (Settings && !Settings->MainMenuMap.IsEmpty())
		? Settings->MainMenuMap
		: TEXT("/Game/Maps/M_MainMenu");

	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->ClientTravel(MenuMap, ETravelType::TRAVEL_Absolute);
	}
}

void URTSLobbyWidget::OpenFriendInviteOverlay()
{
	if (USteamFriendsSubsystem* Friends = GetFriends())
	{
		Friends->OpenInviteOverlay();
	}
}

void URTSLobbyWidget::HostStartMatch()
{
	if (ARTSLobbyPlayerState* PS = GetLocalLobbyPlayerState())
	{
		PS->HostStartMatch();
	}
}

void URTSLobbyWidget::HostKick(ARTSLobbyPlayerState* Target)
{
	if (ARTSLobbyPlayerState* PS = GetLocalLobbyPlayerState())
	{
		PS->HostKickPlayer(Target);
	}
}

void URTSLobbyWidget::HostSetMap(const FString& MapName)
{
	if (ARTSLobbyPlayerState* PS = GetLocalLobbyPlayerState())
	{
		PS->HostSetMap(MapName);
	}
}

TArray<FRTSPlayerInfo> URTSLobbyWidget::GetRoster() const
{
	if (const ARTSLobbyGameState* LobbyGS = GetLobbyGameState())
	{
		return LobbyGS->GetPlayerRoster();
	}
	return TArray<FRTSPlayerInfo>();
}

TArray<ARTSLobbyPlayerState*> URTSLobbyWidget::GetRosterPlayerStates() const
{
	TArray<ARTSLobbyPlayerState*> Result;
	if (const ARTSLobbyGameState* LobbyGS = GetLobbyGameState())
	{
		for (APlayerState* PS : LobbyGS->PlayerArray)
		{
			if (ARTSLobbyPlayerState* LobbyPS = Cast<ARTSLobbyPlayerState>(PS))
			{
				Result.Add(LobbyPS);
			}
		}
	}
	return Result;
}

ARTSLobbyPlayerState* URTSLobbyWidget::GetLocalLobbyPlayerState() const
{
	const APlayerController* PC = GetOwningPlayer();
	return PC ? PC->GetPlayerState<ARTSLobbyPlayerState>() : nullptr;
}

bool URTSLobbyWidget::IsLocalPlayerHost() const
{
	const ARTSLobbyPlayerState* PS = GetLocalLobbyPlayerState();
	return PS ? PS->bIsHost : false;
}

bool URTSLobbyWidget::AreAllPlayersReady() const
{
	const ARTSLobbyGameState* LobbyGS = GetLobbyGameState();
	return LobbyGS ? LobbyGS->AreAllPlayersReady() : false;
}

ARTSLobbyGameState* URTSLobbyWidget::GetLobbyGameState() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetGameState<ARTSLobbyGameState>() : nullptr;
}

USteamLobbySubsystem* URTSLobbyWidget::GetLobbySubsystem() const
{
	UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<USteamLobbySubsystem>() : nullptr;
}

USteamFriendsSubsystem* URTSLobbyWidget::GetFriends() const
{
	UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<USteamFriendsSubsystem>() : nullptr;
}

void URTSLobbyWidget::RebindPlayerStates()
{
	ARTSLobbyGameState* LobbyGS = GetLobbyGameState();
	if (LobbyGS == nullptr)
	{
		return;
	}

	bool bRosterChanged = false;

	for (APlayerState* PS : LobbyGS->PlayerArray)
	{
		ARTSLobbyPlayerState* LobbyPS = Cast<ARTSLobbyPlayerState>(PS);
		if (LobbyPS == nullptr || BoundPlayerStates.Contains(LobbyPS))
		{
			continue;
		}

		LobbyPS->OnLobbyStateChanged.AddDynamic(this, &URTSLobbyWidget::HandlePlayerStateChanged);
		BoundPlayerStates.Add(LobbyPS);
		bRosterChanged = true;
	}

	// Drop stale entries for players that have left.
	const int32 Removed = BoundPlayerStates.RemoveAll([](const ARTSLobbyPlayerState* PS)
	{
		return !IsValid(PS);
	});

	if (bRosterChanged || Removed > 0)
	{
		RefreshBoundWidgets();
		OnRosterChanged();
	}
}

void URTSLobbyWidget::RefreshBoundWidgets()
{
	if (StartMatchButton)
	{
		StartMatchButton->SetIsEnabled(IsLocalPlayerHost() && AreAllPlayersReady());
	}

	if (const ARTSLobbyGameState* LobbyGS = GetLobbyGameState())
	{
		if (MapText)
		{
			MapText->SetText(FText::FromString(LobbyGS->SelectedMap));
		}
	}

	RefreshTeamOptions();
}

void URTSLobbyWidget::RefreshTeamOptions()
{
	if (!TeamComboBox)
	{
		return;
	}

	// One selectable team per possible player, driven by the lobby capacity.
	const ARTSLobbyGameState* LobbyGS = GetLobbyGameState();
	const int32 DesiredCount = FMath::Max(1, LobbyGS ? LobbyGS->MaxPlayers : 1);

	// Rebuild the option list only when the team count actually changed, so the
	// periodic refresh tick doesn't keep clobbering the combo box.
	if (TeamComboBox->GetOptionCount() != DesiredCount)
	{
		TeamComboBox->ClearOptions();
		for (int32 TeamIndex = 0; TeamIndex < DesiredCount; ++TeamIndex)
		{
			TeamComboBox->AddOption(FString::Printf(TEXT("Team %d"), TeamIndex + 1));
		}
	}

	// Reflect the local player's replicated team (clamped into range). The combo's
	// option index is the team value, so OnTeamSelectionChanged stays consistent.
	if (const ARTSLobbyPlayerState* PS = GetLocalLobbyPlayerState())
	{
		const int32 SelectedTeam = FMath::Clamp(PS->Team, 0, DesiredCount - 1);
		if (TeamComboBox->GetSelectedIndex() != SelectedTeam)
		{
			// Programmatic change -> ESelectInfo::Direct, ignored by OnTeamSelectionChanged.
			TeamComboBox->SetSelectedIndex(SelectedTeam);
		}
	}
}

void URTSLobbyWidget::HandlePlayerStateChanged()
{
	RefreshBoundWidgets();
	OnRosterChanged();
}

void URTSLobbyWidget::HandleConfigChanged()
{
	RefreshBoundWidgets();
	OnConfigChanged();
}

void URTSLobbyWidget::HandleSeamlessTravelStart(UWorld* CurrentWorld, const FString& LevelName)
{
	// Only react to our own world's travel (the lobby leaving for the match map).
	if (CurrentWorld != GetWorld())
	{
		return;
	}

	if (IsInViewport())
	{
		RemoveFromParent();
	}
}

void URTSLobbyWidget::OnReadyClicked()
{
	if (ARTSLobbyPlayerState* PS = GetLocalLobbyPlayerState())
	{
		PS->SetReady(!PS->bReady);
	}
}

void URTSLobbyWidget::OnLeaveClicked()
{
	LeaveLobby();
}

void URTSLobbyWidget::OnStartClicked()
{
	HostStartMatch();
}

void URTSLobbyWidget::OnInviteClicked()
{
	OpenFriendInviteOverlay();
}

void URTSLobbyWidget::OnTeamSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (SelectionType == ESelectInfo::Direct || !TeamComboBox)
	{
		return;
	}

	SetTeam(TeamComboBox->GetSelectedIndex());
}
