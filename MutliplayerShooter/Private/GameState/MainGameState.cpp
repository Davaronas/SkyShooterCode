// Fill out your copyright notice in the Description page of Project Settings.


#include "GameState/MainGameState.h"
#include "Net/UnrealNetwork.h"
#include "PlayerState/MainPlayerState.h"
#include "Controller/MainPlayerController.h"
#include "GameMode/MainGameMode.h"
#include "GameMode/TeamsGameMode.h"

void AMainGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMainGameState, topScoringPlayers);

	DOREPLIFETIME(AMainGameState, blueTeamScore);
	DOREPLIFETIME(AMainGameState, redTeamScore);
}

void AMainGameState::UpdateTopScore(AMainPlayerState* playerState)
{
	if (!playerState) { return; }

	bool bGainedLead{};
	bool bTied{};
	bool bChanged{};

	if (topScoringPlayers.Num() == 0)
	{
		topScoringPlayers.Add(playerState);
		topScore = playerState->GetScore();

		bGainedLead = true;
		bTied = false;
		bChanged = true;
	}
	else if (playerState->GetScore() == topScore) // tie
	{
		topScoringPlayers.AddUnique(playerState);

		bGainedLead = true;
		bTied = true;
		bChanged = true;
	}
	else if (playerState->GetScore() > topScore) // gained absolute lead
	{
		if (topScoringPlayers.Num() == 1 && topScoringPlayers.Contains(playerState))
		{
			bChanged = false;
		}
		else
		{
			bChanged = true;

			topScoringPlayers.Empty();
			topScoringPlayers.Add(playerState);
		}

		
		topScore = playerState->GetScore();

		bGainedLead = true;
		bTied = false;
	}

	if (bGainedLead && bChanged)
	{
		if (!GetWorld()) { return; }
		AMainGameMode* gameMode = GetWorld()->GetAuthGameMode<AMainGameMode>();
		if (gameMode && !gameMode->IsTeamMatch())
		{
			gameMode->BroadcastPlayerGainedLead(playerState, bTied);
		}
	}
}

void AMainGameState::RemovePlayerFromTopScore(AMainPlayerState* playerState)
{
	if (topScoringPlayers.Contains(playerState))
	{
		topScoringPlayers.Remove(playerState);
	}
}

void AMainGameState::AddToBlueTeamScore(const float score)
{
	blueTeamScore += score;

	AMainPlayerController* localPlayerController = GetWorld()->GetFirstPlayerController<AMainPlayerController>();
	if (localPlayerController)
	{
		localPlayerController->SetHUDBlueTeamScore(blueTeamScore);
	}

	if (blueTeamScore > redTeamScore && !bBlueWinning)
	{
		if (!GetWorld()) { return; }
		ATeamsGameMode* gameMode = GetWorld()->GetAuthGameMode<ATeamsGameMode>();
		if (gameMode && gameMode->IsTeamMatch())
		{
			gameMode->BroadcastTeamGainedLead(ETeams::ET_BlueTeam);
		}

		bBlueWinning = true;
		bRedWinning = false;
	}
}

void AMainGameState::AddToRedTeamScore(const float score)
{
	redTeamScore += score;

	AMainPlayerController* localPlayerController = GetWorld()->GetFirstPlayerController<AMainPlayerController>();
	if (localPlayerController)
	{
		localPlayerController->SetHUDRedTeamScore(redTeamScore);
	}

	if (blueTeamScore < redTeamScore && !bRedWinning)
	{
		if (!GetWorld()) { return; }
		ATeamsGameMode* gameMode = GetWorld()->GetAuthGameMode<ATeamsGameMode>();
		if (gameMode && gameMode->IsTeamMatch())
		{
			gameMode->BroadcastTeamGainedLead(ETeams::ET_RedTeam);
		}

		bRedWinning = true;
		bBlueWinning = false;
	}
}



void AMainGameState::OnRep_blueTeamScore()
{
	AMainPlayerController* localPlayerController = GetWorld()->GetFirstPlayerController<AMainPlayerController>();
	if (localPlayerController)
	{
		localPlayerController->SetHUDBlueTeamScore(blueTeamScore);
	}
}

void AMainGameState::OnRep_redTeamScore()
{
	AMainPlayerController* localPlayerController = GetWorld()->GetFirstPlayerController<AMainPlayerController>();
	if (localPlayerController)
	{
		localPlayerController->SetHUDRedTeamScore(redTeamScore);
	}
}

TArray<AMainPlayerState*> AMainGameState::GetWinners(bool bTeamMatch)
{
	TArray<AMainPlayerState*> winners {};

	if (bTeamMatch)
	{
		if (bBlueWinning)
		{
			return blueTeamPlayers;
		}
		else if (bRedWinning)
		{
			return readTeamPlayers;
		}
	}
	else
	{
		if (topScoringPlayers.Num() == 1)
		{
			return topScoringPlayers;
		}
	}

	return winners;
}
