// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/SessionResultWidget.h"
#include "MultiplayerSessionsMenu.h"

void USessionResultWidget::InitializeSessionResult
(FOnlineSessionSearchResult& searchResult, UMultiplayerSessionsMenu* menuPointer)
{
	if (!PlayerName) { return; }
	if (!PlayerCount) { return; }
	if (!GameMode) { return; }
	if (!JoinButton) { return; }

	storedSearchResult = searchResult;
	bValid = true;


}

void USessionResultWidget::OnJoinClicked()
{
	if (!bValid) { return; }
}
