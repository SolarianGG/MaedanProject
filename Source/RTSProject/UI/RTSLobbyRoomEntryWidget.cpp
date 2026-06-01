// Fill out your copyright notice in the Description page of Project Settings.

#include "RTSLobbyRoomEntryWidget.h"

#include "RTSLobbyInfoObject.h"

void URTSLobbyRoomEntryWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	// ListView recycles entry widgets across items; just read the snapshot off the
	// bound wrapper object (or clear it if the entry is recycled with no item).
	if (const URTSLobbyInfoObject* LobbyObject = Cast<URTSLobbyInfoObject>(ListItemObject))
	{
		LobbyInfo = LobbyObject->Info;
	}
	else
	{
		LobbyInfo = FRTSLobbyInfo();
	}

	// Keep the default behaviour (fires the BP "On List Item Object Set" event), then
	// run our own populate so the WBP only needs to implement OnLobbyInfoUpdated.
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);
	OnLobbyInfoUpdated(LobbyInfo);
}
