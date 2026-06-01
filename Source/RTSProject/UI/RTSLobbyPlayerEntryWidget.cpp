// Fill out your copyright notice in the Description page of Project Settings.

#include "RTSLobbyPlayerEntryWidget.h"

#include "RTSLobbyPlayerState.h"

void URTSLobbyPlayerEntryWidget::NativeDestruct()
{
	UnbindCurrentPlayer();
	Super::NativeDestruct();
}

void URTSLobbyPlayerEntryWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	// ListView recycles entry widgets across items, so drop the previous binding first.
	UnbindCurrentPlayer();

	BoundPlayer = Cast<ARTSLobbyPlayerState>(ListItemObject);

	if (IsValid(BoundPlayer))
	{
		BoundPlayer->OnLobbyStateChanged.AddDynamic(this, &URTSLobbyPlayerEntryWidget::HandlePlayerStateChanged);
	}

	// Keep the default behaviour (fires the BP "On List Item Object Set" event), then
	// run our own initial populate so the WBP only needs to implement OnPlayerStateUpdated.
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);
	OnPlayerStateUpdated(BoundPlayer);
}

void URTSLobbyPlayerEntryWidget::NativeOnEntryReleased()
{
	UnbindCurrentPlayer();
	IUserObjectListEntry::NativeOnEntryReleased();
}

void URTSLobbyPlayerEntryWidget::HandlePlayerStateChanged()
{
	// One of the bound player's replicated lobby fields changed; let the WBP refresh.
	OnPlayerStateUpdated(BoundPlayer);
}

void URTSLobbyPlayerEntryWidget::UnbindCurrentPlayer()
{
	if (IsValid(BoundPlayer))
	{
		BoundPlayer->OnLobbyStateChanged.RemoveDynamic(this, &URTSLobbyPlayerEntryWidget::HandlePlayerStateChanged);
	}

	BoundPlayer = nullptr;
}
