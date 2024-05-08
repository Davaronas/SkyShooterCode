// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameMode/MainGameMode.h"
#include "TeamsGameMode.generated.h"

/**
 * 
 */
UCLASS()
class MUTLIPLAYERSHOOTER_API ATeamsGameMode : public AMainGameMode
{
	GENERATED_BODY()

public:
	ATeamsGameMode();
	virtual void PostLogin(class APlayerController* joining) override;
	void SetTeamForPlayer(class APlayerController* player);
	
	virtual void Logout(class AController* exiting) override;

	virtual void PlayerEliminated(
		class APlayerCharacter* victimPlayer,
		class AMainPlayerController* victimController,
		class AMainPlayerController* killerController,
		class AActor* damageCauser, bool bHeadshot) override;

	virtual AActor* ChoosePlayerStart_Implementation(class AController* Player) override;

	void BroadcastTeamGainedLead(ETeams team);


protected:

	void PutPlayerIntoATeam(class APlayerController* player, class AMainGameState* gameState);
	void PutPlayerIntoATeam(class AMainPlayerState* player, class AMainGameState* gameState);
	void SetPlayerStateTeam(AMainPlayerState*& castPlayer, AMainGameState* gameState);

	void SetPlayerStateToBlueTeam(AMainPlayerState*& castPlayer, AMainGameState* gameState);

	void SetPlayerStateToRedTeam(AMainPlayerState*& castPlayer, AMainGameState* gameState);

	virtual void HandleMatchHasStarted() override;

	void SetTeamForAllPlayers();


	

	
	
};
