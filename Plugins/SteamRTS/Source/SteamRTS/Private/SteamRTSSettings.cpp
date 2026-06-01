// Copyright Epic Games, Inc. All Rights Reserved.

#include "SteamRTSSettings.h"

USteamRTSSettings::USteamRTSSettings()
	: MaxLobbyPlayers(8)
	, LobbyVisibility(ERTSLobbyVisibility::Public)
	, bUseSteamNetworking(true)
	, bEnablePresence(true)
	, bEnableInvites(true)
	, LobbyMap(TEXT("/Game/Maps/L_Lobby"))
	, MainMenuMap(TEXT("/Game/Maps/M_MainMenu"))
{
}

FName USteamRTSSettings::GetCategoryName() const
{
	return TEXT("Game");
}
