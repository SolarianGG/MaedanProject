// Fill out your copyright notice in the Description page of Project Settings.

#include "RTSMenuWidget.h"

#include "Engine/GameInstance.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/EditableTextBox.h"
#include "Components/ListView.h"

#include "RTSHostConfigWidget.h"
#include "RTSLobbyInfoObject.h"
#include "SteamRTSSubsystem.h"
#include "SteamLobbySubsystem.h"
#include "SteamRTSSettings.h"

void URTSMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (USteamLobbySubsystem* Lobby = GetLobby())
	{
		Lobby->OnLobbyListUpdated.AddDynamic(this, &URTSMenuWidget::HandleLobbyListUpdated);
		Lobby->OnLobbyJoined.AddDynamic(this, &URTSMenuWidget::HandleLobbyJoined);
	}

	if (USteamRTSSubsystem* SteamRTS = GetSteamRTS())
	{
		SteamRTS->OnError.AddDynamic(this, &URTSMenuWidget::HandleError);
	}

	if (HostButton)
	{
		HostButton->OnClicked.AddDynamic(this, &URTSMenuWidget::OnHostClicked);
	}
	if (RefreshButton)
	{
		RefreshButton->OnClicked.AddDynamic(this, &URTSMenuWidget::OnRefreshClicked);
	}
	if (JoinButton)
	{
		JoinButton->OnClicked.AddDynamic(this, &URTSMenuWidget::OnJoinClicked);
	}

	if (PlayerNameText)
	{
		PlayerNameText->SetText(FText::FromString(GetLocalPlayerName()));
	}
}

void URTSMenuWidget::NativeDestruct()
{
	if (USteamLobbySubsystem* Lobby = GetLobby())
	{
		Lobby->OnLobbyListUpdated.RemoveDynamic(this, &URTSMenuWidget::HandleLobbyListUpdated);
		Lobby->OnLobbyJoined.RemoveDynamic(this, &URTSMenuWidget::HandleLobbyJoined);
	}

	if (USteamRTSSubsystem* SteamRTS = GetSteamRTS())
	{
		SteamRTS->OnError.RemoveDynamic(this, &URTSMenuWidget::HandleError);
	}

	if (HostButton)
	{
		HostButton->OnClicked.RemoveDynamic(this, &URTSMenuWidget::OnHostClicked);
	}
	if (RefreshButton)
	{
		RefreshButton->OnClicked.RemoveDynamic(this, &URTSMenuWidget::OnRefreshClicked);
	}
	if (JoinButton)
	{
		JoinButton->OnClicked.RemoveDynamic(this, &URTSMenuWidget::OnJoinClicked);
	}

	Super::NativeDestruct();
}

void URTSMenuWidget::OnHostClicked()
{
	// Show the lobby-config popup on top of the menu instead of hosting directly; the actual
	// lobby is created inside URTSHostConfigWidget once the user confirms with Create.
	OpenHostConfig();
}

void URTSMenuWidget::OpenHostConfig()
{
	if (HostConfigWidgetClass == nullptr)
	{
		return;
	}

	// Reuse the existing popup if it's already up; otherwise spawn a fresh one.
	if (HostConfigWidget == nullptr)
	{
		// GetOwningPlayer() can be null if this menu was itself created without an owning PC,
		// and CreateWidget silently returns null with a null owner. Fall back to the first local
		// player controller, then to the game instance, so the popup is always created.
		APlayerController* OwningPC = GetOwningPlayer();
		if (OwningPC == nullptr)
		{
			if (UWorld* World = GetWorld())
			{
				OwningPC = World->GetFirstPlayerController();
			}
		}

		if (OwningPC)
		{
			HostConfigWidget = CreateWidget<URTSHostConfigWidget>(OwningPC, HostConfigWidgetClass);
		}
		else if (UGameInstance* GI = GetGameInstance())
		{
			HostConfigWidget = CreateWidget<URTSHostConfigWidget>(GI, HostConfigWidgetClass);
		}

		if (HostConfigWidget)
		{
			HostConfigWidget->OnClosed.AddDynamic(this, &URTSMenuWidget::HandleHostConfigClosed);
		}
	}

	if (HostConfigWidget && !HostConfigWidget->IsInViewport())
	{
		HostConfigWidget->AddToViewport(HostConfigZOrder);

		// Hide the menu while the popup is up; restored in HandleHostConfigClosed.
		CachedVisibility = GetVisibility();
		SetVisibility(ESlateVisibility::Collapsed);
	}
}

void URTSMenuWidget::HandleHostConfigClosed()
{
	SetVisibility(CachedVisibility);
}

void URTSMenuWidget::OnRefreshClicked()
{
	const FString MapFilter = MapNameInput ? MapNameInput->GetText().ToString() : FString();
	RefreshLobbies(MapFilter, false);
}

void URTSMenuWidget::OnJoinClicked()
{
	if (LobbyListView == nullptr)
	{
		return;
	}

	// The selected item is one of the wrapper objects we put in the list; its
	// position mirrors GetDiscoveredLobbies(), which JoinDiscoveredLobby indexes
	// into. Guard against no selection so the join button is a no-op rather than
	// a crash when nothing is highlighted.
	UObject* Selected = LobbyListView->GetSelectedItem();
	const int32 Index = LobbyObjects.IndexOfByKey(Selected);
	if (Index != INDEX_NONE)
	{
		JoinDiscoveredLobby(Index);
	}
}

void URTSMenuWidget::HostLobby(const FRTSLobbySettings& Settings)
{
	if (USteamLobbySubsystem* Lobby = GetLobby())
	{
		Lobby->CreateLobby(Settings);
	}
}

void URTSMenuWidget::RefreshLobbies(const FString& MapFilter, bool bFriendsOnly)
{
	if (USteamLobbySubsystem* Lobby = GetLobby())
	{
		Lobby->FindLobbies(MapFilter, 100, bFriendsOnly);
	}
}

void URTSMenuWidget::JoinDiscoveredLobby(int32 Index)
{
	USteamLobbySubsystem* Lobby = GetLobby();
	if (Lobby == nullptr)
	{
		return;
	}

	const TArray<FRTSLobbyInfo>& Lobbies = Lobby->GetDiscoveredLobbies();
	if (Lobbies.IsValidIndex(Index))
	{
		Lobby->JoinLobby(Lobbies[Index]);
	}
}

FString URTSMenuWidget::GetLocalPlayerName() const
{
	const USteamRTSSubsystem* SteamRTS = GetSteamRTS();
	return SteamRTS ? SteamRTS->GetLocalPlayerName() : FString();
}

bool URTSMenuWidget::IsSteamAvailable() const
{
	const USteamRTSSubsystem* SteamRTS = GetSteamRTS();
	return SteamRTS ? SteamRTS->IsSteamAvailable() : false;
}

USteamRTSSubsystem* URTSMenuWidget::GetSteamRTS() const
{
	UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<USteamRTSSubsystem>() : nullptr;
}

USteamLobbySubsystem* URTSMenuWidget::GetLobby() const
{
	UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<USteamLobbySubsystem>() : nullptr;
}

TArray<URTSLobbyInfoObject*> URTSMenuWidget::GetDiscoveredLobbyObjects() const
{
	return LobbyObjects;
}

void URTSMenuWidget::HandleLobbyListUpdated(const TArray<FRTSLobbyInfo>& Lobbies)
{
	// Wrap each discovered lobby in a UObject so it can be used as a UListView item
	// (the ListView only accepts UObject* items, not the FRTSLobbyInfo struct). Built
	// before the BP event fires so OnLobbyListReady can immediately populate
	// LobbyListView via GetDiscoveredLobbyObjects(). Order mirrors the subsystem's
	// GetDiscoveredLobbies(), which JoinDiscoveredLobby / OnJoinClicked index into.
	LobbyObjects.Reset(Lobbies.Num());
	for (const FRTSLobbyInfo& Lobby : Lobbies)
	{
		URTSLobbyInfoObject* LobbyObject = NewObject<URTSLobbyInfoObject>(this);
		LobbyObject->Info = Lobby;
		LobbyObjects.Add(LobbyObject);
	}

	OnLobbyListReady(Lobbies);
}

void URTSMenuWidget::HandleLobbyJoined(bool bSuccess)
{
	OnJoinResult(bSuccess);
}

void URTSMenuWidget::HandleError(ERTSSteamError ErrorCode, const FString& Message)
{
	OnError(ErrorCode, Message);
}
