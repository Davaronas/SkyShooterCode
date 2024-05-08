// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/MainGameMode.h"
#include "PlayerCharacter.h"
#include "Controller/MainPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "PlayerState/MainPlayerState.h"
#include "GameState/MainGameState.h"

#include "DebugMacros.h"

namespace MatchState
{
	const FName Cooldown = FName{ TEXT("Cooldown") }; // Match ended, display winners

}


AMainGameMode::AMainGameMode()
{
	// Game State will stay in Waiting to start state, instead of 
	// automatically going into InProgress
	// Blueprint overrides this, so set it there too
	// if created earlier than this change
	bDelayedStart = true;
}


void AMainGameMode::BeginPlay()
{
	if (GetNetMode() == ENetMode::NM_Standalone)
	{
		bDelayedStart = false;
		matchTime = 3600.f;
	}

	Super::BeginPlay();

	if (!GetWorld()) { return; }

	levelStartingTime = GetWorld()->GetTimeSeconds();
	//GEPR_ONE_VARIABLE("World time: %.2f", GetWorld()->GetTimeSeconds());

#if WITH_EDITOR
	warmupTime = 3.f;
#endif
}


void AMainGameMode::PostLogin(APlayerController* joining)
{
	Super::PostLogin(joining);

	AMainPlayerController* controller = Cast<AMainPlayerController>(joining);
	if (controller)
	{
		controller->OnMatchStateSet(MatchState);
		controller->BroadcastLoginOncePlayerStateIsInitialized();
	}
}

void AMainGameMode::Logout(AController* exiting)
{
	if (!exiting) { return; }

	AMainGameState* gameState = GetGameState<AMainGameState>();
	AMainPlayerState* exitingPlayerState = exiting->GetPlayerState<AMainPlayerState>();

	if (gameState && exitingPlayerState)
	{
		gameState->RemovePlayerFromTopScore(exitingPlayerState);
	}

	BroadcastPlayerLeft(exitingPlayerState);
}

// only called on server
void AMainGameMode::PlayerEliminated(
	APlayerCharacter* victimPlayer,
	AMainPlayerController* victimController,
	AMainPlayerController* killerController,
	AActor* damageCauser,
	bool bHeadshot)
{
	if (!GetWorld()) { return; }

	AMainPlayerState* killerPlayerState = 
		killerController != nullptr ? 
		Cast<AMainPlayerState>(killerController->PlayerState) : nullptr;
	AMainPlayerState* victimPlayerState =
		victimController != nullptr ?
		Cast<AMainPlayerState>(victimController->PlayerState) : nullptr;

	AMainGameState* gameState = GetGameState<AMainGameState>();


	BroadcastPlayerEliminated(victimPlayerState, killerPlayerState, damageCauser, bHeadshot);


	// last instigator gets the kill, if killed self
	if (killerPlayerState && victimPlayerState && killerPlayerState == victimPlayerState)
	{
		AController* lastInstigatorController
			= Cast<AController>(victimPlayerState->GetLastDamagingInstigatorController());
		if (lastInstigatorController)
		{
			AMainPlayerState* lastInstigatorPlayerState = lastInstigatorController->GetPlayerState<AMainPlayerState>();
			if (lastInstigatorPlayerState)
			{
				killerPlayerState = lastInstigatorPlayerState;
			}
		}
	}



	if (killerPlayerState && killerPlayerState != victimPlayerState && gameState)
	{

		TArray<AMainPlayerState*> currentTopScore{};
		for (auto topScorePlayer : gameState->topScoringPlayers)
		{
			currentTopScore.Add(topScorePlayer);
		}


		killerPlayerState->AddToScore();
		gameState->UpdateTopScore(killerPlayerState);

		if (gameState->topScoringPlayers.Contains(killerPlayerState))
		{
			APlayerCharacter* assocKillerPlayer = killerPlayerState->GetPawn<APlayerCharacter>();
			if (assocKillerPlayer)
			{
				assocKillerPlayer->MulticastGainedLead();
			}
		}

		for (uint8 i{}; i < currentTopScore.Num(); ++i)
		{
			if (!gameState->topScoringPlayers.Contains(currentTopScore[i]))
			{
				APlayerCharacter* assocPlayer = currentTopScore[i]->GetPawn<APlayerCharacter>();
				if (assocPlayer)
				{
					assocPlayer->MulticastLostLead();
				}
			}
		}
	}


	if (victimPlayerState)
	{
		victimPlayerState->AddToDeaths();
	}

	if (victimPlayer) victimPlayer->ServerPlayerEliminated();
}

bool AMainGameMode::GetTravelType()
{
	return true;
}

void AMainGameMode::GetSeamlessTravelActorList(bool bToTransition, TArray<AActor*>& ActorList)
{
	//ActorList.Empty();
	Super::GetSeamlessTravelActorList(bToTransition, ActorList);
}




void AMainGameMode::RestartGame()
{
	/*for (FConstPlayerControllerIterator it = GetWorld()->GetPlayerControllerIterator(); it; ++it)
	{
		AMainPlayerController* playerController = Cast<AMainPlayerController>(*it);
		if (playerController)
		{
			playerController->ClientGameRestarted();
		}
	}*/

	Super::RestartGame();
}

void AMainGameMode::RequestRespawn(ACharacter* requesterCharacter, AController* requesterController)
{
	if (requesterCharacter)
	{
		requesterCharacter->Reset();
		//requesterCharacter->Destroy();
	}

	if (requesterController)
	{
		if (bTeamMatch)
		{
			AActor* startPos = ChoosePlayerStart_Implementation(requesterController);
			RestartPlayerAtPlayerStart(requesterController, startPos);
		}
		else
		{
			TArray<AActor*> playerStarts{};
			UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), playerStarts);
			if (playerStarts.IsEmpty()) { return; }

			int32 selection = FMath::RandRange(0, playerStarts.Num() - 1);
			RestartPlayerAtPlayerStart(requesterController, playerStarts[selection]);
		}
		
	}
}


void AMainGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!GetWorld()) { return; }

	//if (GEngine) GEngine->AddOnScreenDebugMessage(2, 1.f, FColor::Red,
		//FString::Printf(TEXT("GameMode time: %.2f, starting time: %.2f"), GetWorld()->GetTimeSeconds(), levelStartingTime));

	if (MatchState == MatchState::WaitingToStart)
	{
		countdownTime = warmupTime - GetWorld()->GetTimeSeconds() + levelStartingTime;
		if (countdownTime <= 0.f)
		{
			// We need to call this, if bDelayedStart is true
			// Otherwise the MatchState will never go to InProgress
			StartMatch();
		}
	}
	else if (MatchState == MatchState::InProgress)
	{
		countdownTime = warmupTime + matchTime - GetWorld()->GetTimeSeconds() + levelStartingTime;

		GameModeTimeMessages();


		if (countdownTime <= 0.f)
		{
			SetMatchState(MatchState::Cooldown);
			BroadcastMatchIsOver();
		}
	}
	else if (MatchState == MatchState::Cooldown)
	{
		countdownTime = 
			warmupTime + matchTime + cooldownTime - GetWorld()->GetTimeSeconds() + levelStartingTime;
		if (countdownTime <= 0.f)
		{
			RestartGame();
		}
	}
}



void AMainGameMode::PlayerLeftGame(AMainPlayerState* playerState)
{
	if (!playerState) { return; }

	AMainGameState* gameState = GetGameState<AMainGameState>();
	if (!gameState) { return; }

	gameState->RemovePlayerFromTopScore(playerState);

	//GEPRS_Y("PlayerLeftGame");

	if (APlayerCharacter* pawn = Cast<APlayerCharacter>(playerState->GetPawn()))
	{
		pawn->ServerPlayerEliminated(true);
	}
}




void AMainGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();

	

	SetMatchStateOnPlayerControllers();

	/*
	for (FConstPlayerControllerIterator it = GetWorld()->GetPlayerControllerIterator();
		*it != nullptr; ++it)
	{

	}
	*/
}


void AMainGameMode::BroadcastMatchIsOver()
{
	if (bBroadcastedMatchIsOver) { return; }

	AMainGameState* gameState = GetGameState<AMainGameState>();
	if(gameState)
	{

		TArray<AMainPlayerState*> winners{ gameState->GetWinners(bTeamMatch) };
		for (FConstPlayerControllerIterator it = GetWorld()->GetPlayerControllerIterator(); it; ++it)
		{
			AMainPlayerController* playerController = Cast<AMainPlayerController>(*it);
			if (playerController)
			{
				playerController->ClientMatchIsOver(winners);
			}
		}

		bBroadcastedMatchIsOver = true;
	}
}

void AMainGameMode::SetMatchStateOnPlayerControllers()
{
	if (!GetWorld()) { return; }

	uint8 iterations{ 0 };
	for (FConstPlayerControllerIterator it = GetWorld()->GetPlayerControllerIterator(); it; ++it)
	{
		AMainPlayerController* playerController = Cast<AMainPlayerController>(*it);
		if (playerController)
		{
			playerController->OnMatchStateSet(MatchState, bTeamMatch);
			++iterations;
		}
	}

#if !WITH_EDITOR
	if (iterations <= 1)
	{
		GetWorldTimerManager().
			SetTimer(notifyControllersTimer, this, &ThisClass::SetMatchStateOnPlayerControllers, notifyControllersDelay);
	}
#endif
}



void AMainGameMode::MulticastDebugMessage_Implementation(const FString& message)
{
	GEPR_R(*message);
}

/*
* InGameMessages Broadcasts
*/

void AMainGameMode::BroadcastPlayerEliminated
(AMainPlayerState* victimPlayerState, AMainPlayerState* killerPlayerState,
	AActor* damageCauser, bool bHeadshot)
{
	if (victimPlayerState)
	{
		FConstPlayerControllerIterator it = GetWorld()->GetPlayerControllerIterator();
		for (; it; ++it)
		{
			AMainPlayerController* thisPlayerController = Cast<AMainPlayerController>(*it);
			if (thisPlayerController)
			{
				thisPlayerController->
					ClientHUD_InGameMessage_PlayerElimination
					(killerPlayerState, victimPlayerState, damageCauser, bHeadshot);
			}
		}
	}
}

void AMainGameMode::BroadcastPlayerJoined(APlayerState* joined)
{
	if (GetNetMode() == ENetMode::NM_Standalone) { return; }

	if (joined)
	{
		FConstPlayerControllerIterator it = GetWorld()->GetPlayerControllerIterator();
		for (; it; ++it)
		{
			AMainPlayerController* thisPlayerController = Cast<AMainPlayerController>(*it);
			if (thisPlayerController)
			{
				thisPlayerController->ClientHUD_InGameMessage_PlayerJoinedGame(joined);
			}
		}
	}
}

void AMainGameMode::BroadcastPlayerLeft(AMainPlayerState* exiting)
{
	if (GetNetMode() == ENetMode::NM_Standalone) { return; }

	if (exiting)
	{
		FConstPlayerControllerIterator it = GetWorld()->GetPlayerControllerIterator();
		for (; it; ++it)
		{
			AMainPlayerController* thisPlayerController = Cast<AMainPlayerController>(*it);
			if (thisPlayerController)
			{
				thisPlayerController->ClientHUD_InGameMessage_PlayerLeftGame(exiting);
			}
		}
	}
}

void AMainGameMode::BroadcastPlayerGainedLead(AMainPlayerState* player, bool bTied)
{
	if (GetNetMode() == ENetMode::NM_Standalone) { return; }

	if (player)
	{
		FConstPlayerControllerIterator it = GetWorld()->GetPlayerControllerIterator();
		for (; it; ++it)
		{
			AMainPlayerController* thisPlayerController = Cast<AMainPlayerController>(*it);
			if (thisPlayerController)
			{
				thisPlayerController->ClientHUD_InGameMessage_PlayerGainedLead(player, bTied);
			}
		}
	}
}





void AMainGameMode::GameModeTimeMessages()
{
	if (GetNetMode() == ENetMode::NM_Standalone) { return; }

	if (!bBroadcastedHalfTime && countdownTime <= (matchTime / 2.f))
	{
		BroadcastMessage(FString(TEXT("Match Half Time Reached!")));
		bBroadcastedHalfTime = true;
	}

	if (!bBroadcastedOneMinute && (matchTime >= 60.f) && countdownTime <= 60.f)
	{
		BroadcastMessage(FString(TEXT("One Minute Left!")));
		bBroadcastedOneMinute = true;
	}

	if (!bBroadcastedTenSeconds && (matchTime >= 10.f) && countdownTime <= 10.f)
	{
		BroadcastMessage(FString(TEXT("Ten Seconds Left!")));
		bBroadcastedTenSeconds = true;
	}
}

void AMainGameMode::BroadcastMessage(const FString& message)
{
	if (message.IsEmpty()) { return; }

	FConstPlayerControllerIterator it = GetWorld()->GetPlayerControllerIterator();
	for (; it; ++it)
	{
		AMainPlayerController* thisPlayerController = Cast<AMainPlayerController>(*it);
		if (thisPlayerController)
		{
			thisPlayerController->ClientHUD_InGameMessage_ServerAnnouncement(message, FColor::Silver);
		}
	}
}




