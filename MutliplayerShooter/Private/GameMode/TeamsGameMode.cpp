// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/TeamsGameMode.h"
#include "GameState/MainGameState.h"
#include "PlayerState/MainPlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "HelperTypes/Teams.h"
#include "Controller/MainPlayerController.h"
#include "HelperTypes/Teams.h"
#include "GameFramework/PlayerStart.h"

#include "Controller/MainPlayerController.h"


#include "PlayerState/MainPlayerState.h"
#include "GameState/MainGameState.h"

#include "DebugMacros.h"

ATeamsGameMode::ATeamsGameMode()
{
	bTeamMatch = true;
}


void ATeamsGameMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	SetTeamForAllPlayers();
}

void ATeamsGameMode::SetTeamForAllPlayers()
{
	AMainGameState* gameState = Cast<AMainGameState>(UGameplayStatics::GetGameInstance(this));
	if (gameState)
	{
		for (auto player : gameState->PlayerArray)
		{
			PutPlayerIntoATeam(Cast<AMainPlayerState>(player), gameState);
		}
	}
}

void ATeamsGameMode::BroadcastTeamGainedLead(ETeams team)
{
	if (team == ETeams::ET_NoTeam) { return; }

	FConstPlayerControllerIterator it = GetWorld()->GetPlayerControllerIterator();
	for (; it; ++it)
	{
		AMainPlayerController* thisPlayerController = Cast<AMainPlayerController>(*it);
		if (thisPlayerController)
		{
			thisPlayerController->ClientHUD_InGameMessage_TeamGainedLead(team);
		}
	}
}



void ATeamsGameMode::PostLogin(APlayerController* joining)
{
	Super::PostLogin(joining);

	SetTeamForPlayer(joining);
}

void ATeamsGameMode::SetTeamForPlayer(APlayerController* joining)
{
	if (!joining) { return; }

	AMainGameState* gameState = Cast<AMainGameState>(UGameplayStatics::GetGameState(this));
	if (!gameState) { return; }

	PutPlayerIntoATeam(joining, gameState);
}

void ATeamsGameMode::PutPlayerIntoATeam(APlayerController* player, AMainGameState* gameState)
{
	if (!gameState) { return; }
	if (!player) { return; }

	AMainPlayerState* castPlayer = player->GetPlayerState<AMainPlayerState>();
	SetPlayerStateTeam(castPlayer, gameState);
}



void ATeamsGameMode::PutPlayerIntoATeam(AMainPlayerState* player, AMainGameState* gameState)
{
	if (!gameState) { return; }
	if (!player) { return; }

	SetPlayerStateTeam(player, gameState);
}

void ATeamsGameMode::SetPlayerStateTeam(AMainPlayerState*& castPlayer, AMainGameState* gameState)
{
	if (castPlayer && castPlayer->GetTeam() == ETeams::ET_NoTeam)
	{

		if (gameState->readTeamPlayers.Num() < gameState->blueTeamPlayers.Num())
		{
			SetPlayerStateToRedTeam(castPlayer, gameState);
		}
		else if(gameState->readTeamPlayers.Num() > gameState->blueTeamPlayers.Num())
		{
			SetPlayerStateToBlueTeam(castPlayer, gameState);
		}
		else
		{
			const bool bSetToBlueTeam = FMath::RandBool();

			if (bSetToBlueTeam)
			{
				SetPlayerStateToBlueTeam(castPlayer, gameState);
			}
			else
			{
				SetPlayerStateToRedTeam(castPlayer, gameState);
			}
		}
	}
}

void ATeamsGameMode::SetPlayerStateToBlueTeam(AMainPlayerState*& castPlayer, AMainGameState* gameState)
{
	if (castPlayer && gameState)
	{
		castPlayer->SetTeam(ETeams::ET_BlueTeam);
		gameState->blueTeamPlayers.AddUnique(castPlayer);
	}
}

void ATeamsGameMode::SetPlayerStateToRedTeam(AMainPlayerState*& castPlayer, AMainGameState* gameState)
{
	if (castPlayer && gameState)
	{
		castPlayer->SetTeam(ETeams::ET_RedTeam);
		gameState->readTeamPlayers.AddUnique(castPlayer);
	}
}







void ATeamsGameMode::Logout(AController* exiting)
{
	Super::Logout(exiting);

	AMainGameState* gameState = Cast<AMainGameState>(UGameplayStatics::GetGameInstance(this));
	AMainPlayerState* castPlayer = exiting->GetPlayerState<AMainPlayerState>();
	if (castPlayer && gameState)
	{
		if (gameState->readTeamPlayers.Contains(castPlayer))
		{
			gameState->readTeamPlayers.Remove(castPlayer);
		}
		else if(gameState->blueTeamPlayers.Contains(castPlayer))
		{
			gameState->readTeamPlayers.Remove(castPlayer);
		}
	}
}

void ATeamsGameMode::PlayerEliminated
(APlayerCharacter* victimPlayer, AMainPlayerController* victimController, AMainPlayerController* killerController,
	class AActor* damageCauser, bool bHeadshot)
{

	Super::PlayerEliminated(victimPlayer, victimController, killerController, damageCauser, bHeadshot);

	if (!killerController) { return; }
	if (!victimController) { return; }

	AMainGameState* gameState = Cast<AMainGameState>(UGameplayStatics::GetGameState(this));
	if (!gameState) { return; }


	AMainPlayerState* killerPlayerState = killerController->GetPlayerState<AMainPlayerState>();
	if (!killerPlayerState) { return; }
	ETeams killerTeam = killerPlayerState->GetTeam();
	if (victimController == killerController)
	{
		AMainPlayerController* lastInstigator = 
			Cast<AMainPlayerController>(victimController->GetLastDamagingInstigatorController());
		if (!lastInstigator) { return; }
	}

	if (killerTeam == ETeams::ET_BlueTeam)
	{
		gameState->AddToBlueTeamScore();
	}
	else if (killerTeam == ETeams::ET_RedTeam)
	{
		gameState->AddToRedTeamScore();
	}


}

AActor* ATeamsGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	if (!Player) { return (AActor*)nullptr; }

	TArray<AActor*> allPlayerStarts{};

	UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), allPlayerStarts);
	if (allPlayerStarts.Num() <= 0) { return (AActor*)nullptr; }

	TArray<APlayerStart*> validPlayerStarts{};

	AMainPlayerState* spawnerPlayerState = Player->GetPlayerState<AMainPlayerState>();
	if (!spawnerPlayerState) {  return (AActor*)nullptr; }
	ETeams spawnerTeam = spawnerPlayerState->GetTeam();



	if (spawnerTeam == ETeams::ET_NoTeam)
	{
		AMainPlayerController* playerController = Cast<AMainPlayerController>(Player);
		if (playerController)
		{
			SetTeamForPlayer(playerController);
			playerController->OnMatchStateSet(MatchState, true);
			spawnerTeam = spawnerPlayerState->GetTeam();
		}
	}


	if (spawnerTeam == ETeams::ET_BlueTeam)
	{
		for (auto start : allPlayerStarts)
		{
			APlayerStart* playerStart = Cast<APlayerStart>(start);
			if (playerStart)
			{
				if (playerStart->PlayerStartTag == FName{ TEXT("Blue") })
				{
					validPlayerStarts.Add(playerStart);
				}
			}
		}
	}
	else if (spawnerTeam == ETeams::ET_RedTeam)
	{
		for (auto start : allPlayerStarts)
		{
			APlayerStart* playerStart = Cast<APlayerStart>(start);
			if (playerStart)
			{
				if (playerStart->PlayerStartTag == FName{ TEXT("Red") })
				{
					validPlayerStarts.Add(playerStart);
				}
			}
		}
	}

	

	if (validPlayerStarts.Num() > 0)
	{
		return validPlayerStarts[FMath::RandRange(0, validPlayerStarts.Num() - 1)];
	}

	return (AActor*)nullptr;
}
