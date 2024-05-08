// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/OverheadWidget.h"
#include "Components/TextBlock.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "DebugMacros.h"

void UOverheadWidget::NativeDestruct()
{
	Super::NativeDestruct();
	RemoveFromParent();
}

void UOverheadWidget::SetDisplayText(FString text)
{
	if (!DisplayText) { return; }

	DisplayText->SetText(FText::FromString(text));
}

void UOverheadWidget::ShowPlayerNetRole(APawn* pawn)
{
	if (!pawn) { return; }

	FString playerName = FString{ "" };

	APlayerState* playerState = pawn->GetPlayerState();
	if (playerState)
	playerName = playerState->GetPlayerName();

	ENetRole localNetRole = pawn->GetLocalRole();
	FString playerRole{};

	switch (localNetRole)
	{
	case ENetRole::ROLE_Authority:
		playerRole = TEXT("(Authority)");
		break;
	case ENetRole::ROLE_AutonomousProxy:
		playerRole = TEXT("(Autonomous Proxy)");
		break;
	case ENetRole::ROLE_SimulatedProxy:
		playerRole = TEXT("(Simulated Proxy)");
		break;
	case ENetRole::ROLE_None:
		playerRole = TEXT("(No Role)");
		break;
	}
	FString nameAndRole{ FString::Printf(TEXT("%s %s"), *playerName, *playerRole) };
	GEPR_Y(playerName);
	GEPR_Y(playerRole);
	
	SetDisplayText(nameAndRole);
}
