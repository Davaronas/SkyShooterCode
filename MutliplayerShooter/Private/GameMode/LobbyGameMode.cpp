// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/LobbyGameMode.h"
#include "GameFramework/GameState.h"
#include "DebugMacros.h"
#include "GameFramework/PlayerController.h"
#include "Controller/MainPlayerController.h"
#include "PlayerState/MainPlayerState.h"
#include "MultiplayerSessionsSubsystem.h"
#include "Components/Widget.h"
#include "Blueprint/UserWidget.h"
#include "HUD/LobbyWidget.h"
#include "Components/TextBlock.h"



void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (!NewPlayer) { return; }
	if (!GameState) { return; }

	FString playerName = NewPlayer->GetName();

	AMainPlayerController* mainController = Cast<AMainPlayerController>(NewPlayer);

	if (mainController)
	{
		AMainPlayerState* newPlayerState = mainController->GetPlayerState<AMainPlayerState>();

		if (newPlayerState)
		{
			playerName = newPlayerState->GetPlayerName();
		}
	}
	const int32 playerCount = GameState.Get()->PlayerArray.Num();

	GEPR_W(FString{ TEXT("A Player Has Joined The Session") });
	GEPR_W(FString::Printf(TEXT("Current Player Count: %i"), playerCount));



	FString targetMapUrl{};
	UGameInstance* gameInstance = GetGameInstance();
	if (!gameInstance) { return; }

	UMultiplayerSessionsSubsystem* ms_Subsystem = gameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
	check(ms_Subsystem != nullptr);

	int32 playersNeededToStartMatch{};
	FString matchType{};

	ms_Subsystem->GetDesiredMatchData(playersNeededToStartMatch, matchType);

	if (playerCount >= playersNeededToStartMatch)
	{
		if (UWorld* world = GetWorld())
		{
			FString travelToMapURL{};

			if (matchType == FString{ TEXT("FreeForAll") })
			{
				travelToMapURL = FString{ "/Game/Maps/Map1?listen" };
			}
			else if (matchType == FString{ TEXT("TeamDM") })
			{
				travelToMapURL = FString{ "/Game/Maps/Map1_Teams?listen" };
			}
			else
			{
				travelToMapURL = FString{ "/Game/Maps/Map1?listen" };
			}



			bUseSeamlessTravel = true;
			world->ServerTravel(travelToMapURL, true);
		}
	}
}

void ALobbyGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	if (!Exiting) { return; }
	if (!GameState) { return; }

	if (APlayerController* ExitingPlayer = Cast<APlayerController>(Exiting))
	{
		const FString playerName = ExitingPlayer->GetName();
		const int32 playerCount = GameState.Get()->PlayerArray.Num();

		GEPR_W(FString::Printf(TEXT("A Player Has Left The Session")));
		GEPR_W(FString::Printf(TEXT("Current Player Count: %i"), playerCount - 1));
	}


}

void ALobbyGameMode::BeginPlay()
{
	UWorld* world = GetWorld();
	if (!world) { return; }

	APlayerController* localPlayer = world->GetFirstPlayerController();
	if (!localPlayer) { return; }

	if (!lobbyWidgetClass) { return; }
	
	lobbyWidget = CreateWidget<ULobbyWidget>(localPlayer, lobbyWidgetClass);
	lobbyWidget->AddToViewport();

	FTimerManager& timerManager = world->GetTimerManager();


	timerManager.SetTimer(playerUpdateTimerHandle,
		this, &ThisClass::UpdatePlayers, playersUpdateInterval, true, 1.f);
}

void ALobbyGameMode::UpdatePlayers()
{
	if (!lobbyWidget) { return; }

	AGameState* gameState = GetGameState<AGameState>();
	if (!gameState) { return; }

	for (auto& playerWidget : playerNameWidgets)
	{
		playerWidget->RemoveFromParent();
	}

	playerNameWidgets.Empty();

	const TArray<APlayerState*>& playerArray = gameState->PlayerArray;

	for (auto player : playerArray)
	{
		if (!player) { continue; }

		UTextBlock* newMessage = NewObject<UTextBlock>(this, playerNameWidgetClass);
		if (!newMessage) { return; }

		newMessage->SetText(FText::FromString(player->GetPlayerName()));

		playerNameWidgets.Add(newMessage);
		lobbyWidget->AddWidgetToVerticalBox(newMessage);
	}
	


}

