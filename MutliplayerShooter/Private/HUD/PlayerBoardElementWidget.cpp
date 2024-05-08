// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/PlayerBoardElementWidget.h"
#include "Components/TextBlock.h"

void UPlayerBoardElementWidget::FillPlayerData
(const FString& playerName, const float& score, const float& deaths, const float& ping, const FColor& displayColor)
{
	if (!PlayerName) { return; }
	if (!Score) { return; }
	if (!Deaths) { return; }
	if (!Ping) { return; }

	PlayerName->SetText(FText::FromString(playerName));
	PlayerName->SetColorAndOpacity(displayColor);
	Score->SetText(FText::AsNumber(FMath::RoundToInt32(score)));
	Deaths->SetText(FText::AsNumber(FMath::RoundToInt32(deaths)));
	Ping->SetText(FText::AsNumber(FMath::RoundToInt32(ping)));
}
