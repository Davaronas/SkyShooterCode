// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "MainGameState.generated.h"

/**
 * 
 */
UCLASS()
class MUTLIPLAYERSHOOTER_API AMainGameState : public AGameState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override;

	void UpdateTopScore(class AMainPlayerState* playerState);

	UPROPERTY(Replicated)
	TArray<class AMainPlayerState*> topScoringPlayers{};

	void RemovePlayerFromTopScore(class AMainPlayerState* playerState);


	void AddToBlueTeamScore(const float score = 1.f);
	void AddToRedTeamScore(const float score = 1.f);

	bool bBlueWinning{ false };
	bool bRedWinning{ false };

	/*
	* TEAMS
	*/

	TArray<class AMainPlayerState*> blueTeamPlayers{};
	TArray<class AMainPlayerState*> readTeamPlayers{};

	UPROPERTY(ReplicatedUsing = OnRep_blueTeamScore)
	float blueTeamScore{};
	

	UPROPERTY(ReplicatedUsing = OnRep_redTeamScore)
	float redTeamScore{};
	

protected:
	UFUNCTION()
	void OnRep_blueTeamScore();
	UFUNCTION()
	void OnRep_redTeamScore();

private:
	float topScore{};

public:
	FORCEINLINE const TArray<class AMainPlayerState*> GetTopScoringPlayers() { return topScoringPlayers; }
	TArray<class AMainPlayerState*> GetWinners(bool bTeamMatch);
	
};
