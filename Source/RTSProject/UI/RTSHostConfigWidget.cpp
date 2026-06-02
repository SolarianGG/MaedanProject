// Fill out your copyright notice in the Description page of Project Settings.

#include "RTSHostConfigWidget.h"

#include "Engine/GameInstance.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/ComboBoxString.h"
#include "Components/PanelWidget.h"

#include "RTSMapEntryWidget.h"
#include "SteamRTSSubsystem.h"
#include "SteamLobbySubsystem.h"
#include "SteamRTSSettings.h"

void URTSHostConfigWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (CreateButton)
	{
		CreateButton->OnClicked.AddDynamic(this, &URTSHostConfigWidget::OnCreateClicked);
	}
	if (CloseButton)
	{
		CloseButton->OnClicked.AddDynamic(this, &URTSHostConfigWidget::OnCloseClicked);
	}

	BindMapEntries();
	UE_LOG(LogTemp, Warning, TEXT("Successfuly binded delegates in host config"));
}

void URTSHostConfigWidget::NativeDestruct()
{
	if (CreateButton)
	{
		CreateButton->OnClicked.RemoveDynamic(this, &URTSHostConfigWidget::OnCreateClicked);
	}
	if (CloseButton)
	{
		CloseButton->OnClicked.RemoveDynamic(this, &URTSHostConfigWidget::OnCloseClicked);
	}

	if (MapEntryContainer)
	{
		for (UWidget* Child : MapEntryContainer->GetAllChildren())
		{
			if (URTSMapEntryWidget* Entry = Cast<URTSMapEntryWidget>(Child))
			{
				Entry->OnClickedDelegate.RemoveDynamic(this, &URTSHostConfigWidget::HandleMapEntrySelected);
			}
		}
	}

	Super::NativeDestruct();
}

void URTSHostConfigWidget::OnCreateClicked()
{
	if (SelectedMapPath.IsEmpty())
	{
		// No map picked yet — surface a recoverable error and keep the popup open.
		if (USteamRTSSubsystem* SteamRTS = GetSteamRTS())
		{
			SteamRTS->ReportError(ERTSSteamError::CreateLobbyFailed, TEXT("No map selected."));
		}
		OnError(ERTSSteamError::CreateLobbyFailed, TEXT("No map selected."));
		return;
	}

	FRTSLobbySettings Settings;
	Settings.LobbyName = LobbyNameInput ? LobbyNameInput->GetText().ToString() : FString(TEXT("Lobby"));
	Settings.MapName = SelectedMapPath;
	Settings.MaxPlayers = GetSelectedMaxPlayers();
	if (const USteamRTSSettings* Config = GetDefault<USteamRTSSettings>())
	{
		Settings.Visibility = Config->LobbyVisibility;
	}

	if (USteamLobbySubsystem* Lobby = GetLobby())
	{
		Lobby->CreateLobby(Settings);
	}
}

void URTSHostConfigWidget::OnCloseClicked()
{
	CloseConfig();
}

void URTSHostConfigWidget::CloseConfig()
{
	OnClosed.Broadcast();
	RemoveFromParent();
}

void URTSHostConfigWidget::HandleMapEntrySelected(URTSMapEntryWidget* Entry)
{
	if (Entry == nullptr)
	{
		return;
	}

	SelectedMapPath = Entry->MapPath;

	// Highlight the chosen tile and clear the rest.
	if (MapEntryContainer)
	{
		for (UWidget* Child : MapEntryContainer->GetAllChildren())
		{
			if (URTSMapEntryWidget* Other = Cast<URTSMapEntryWidget>(Child))
			{
				Other->OnSelectionChanged(Other == Entry);
			}
		}
	}
}

void URTSHostConfigWidget::SetSelectedMap(const FString& MapPath)
{
	SelectedMapPath = MapPath;
}

void URTSHostConfigWidget::BindMapEntries()
{
	if (MapEntryContainer == nullptr)
	{
		return;
	}

	for (UWidget* Child : MapEntryContainer->GetAllChildren())
	{
		if (URTSMapEntryWidget* Entry = Cast<URTSMapEntryWidget>(Child))
		{
			Entry->OnClickedDelegate.AddUniqueDynamic(this, &URTSHostConfigWidget::HandleMapEntrySelected);
		}
	}
}

int32 URTSHostConfigWidget::GetSelectedMaxPlayers() const
{
	if (MaxPlayersCombo)
	{
		const FString Selected = MaxPlayersCombo->GetSelectedOption();
		if (!Selected.IsEmpty())
		{
			const int32 Parsed = FCString::Atoi(*Selected);
			if (Parsed > 0)
			{
				return Parsed;
			}
		}
	}

	const USteamRTSSettings* Config = GetDefault<USteamRTSSettings>();
	return Config ? Config->MaxLobbyPlayers : 8;
}

USteamRTSSubsystem* URTSHostConfigWidget::GetSteamRTS() const
{
	UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<USteamRTSSubsystem>() : nullptr;
}

USteamLobbySubsystem* URTSHostConfigWidget::GetLobby() const
{
	UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<USteamLobbySubsystem>() : nullptr;
}
