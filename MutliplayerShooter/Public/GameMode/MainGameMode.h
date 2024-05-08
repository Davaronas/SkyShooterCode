// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "MainGameMode.generated.h"

namespace MatchState 
{
	extern MUTLIPLAYERSHOOTER_API const FName Cooldown; // Match ended, display winners
}

/**
 * 
 */
UCLASS()
class MUTLIPLAYERSHOOTER_API AMainGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AMainGameMode();

	virtual void PlayerEliminated(
		class APlayerCharacter* victimPlayer,
		class AMainPlayerController* victimController,
		class AMainPlayerController* killerController,
		AActor* damageCauser,
		bool bHeadshot);

	virtual bool GetTravelType() override;

	virtual void GetSeamlessTravelActorList(bool bToTransition, TArray< AActor* >& ActorList) override;
	
	virtual void RestartGame() override;

	virtual void RequestRespawn(
		class ACharacter* requesterCharacter,
		class AController* requesterController);

	virtual void Tick(float DeltaTime) override;

	void GameModeTimeMessages();

	void PlayerLeftGame(class AMainPlayerState* playerState);

	virtual void PostLogin(class APlayerController* joining) override;
	virtual void Logout(class AController* exiting) override;

	void BroadcastPlayerGainedLead(class AMainPlayerState* player, bool bTied);
	void BroadcastPlayerJoined(class APlayerState* joined);


protected:
	virtual void BeginPlay() override;
	virtual void OnMatchStateSet() override;

	bool bBroadcastedMatchIsOver{ false };
	virtual void BroadcastMatchIsOver();

	void SetMatchStateOnPlayerControllers();

	void BroadcastPlayerEliminated
	(class AMainPlayerState* victimPlayerState, class AMainPlayerState* killerPlayerState,
		AActor* damageCauser, bool bHeadshot);
	
	void BroadcastPlayerLeft(class AMainPlayerState* exiting);

	void BroadcastMessage(const FString& message);

	bool bBroadcastedHalfTime{ false };
	bool bBroadcastedOneMinute{ false };
	bool bBroadcastedTenSeconds{ false };


	UFUNCTION(NetMulticast, Reliable)
	void MulticastDebugMessage(const FString& message);

	UPROPERTY(VisibleAnywhere)
	bool bTeamMatch{ false };

private:
	UPROPERTY(EditDefaultsOnly, Category = "MainGameMode|Time")
	float warmupTime{10.f};

	UPROPERTY(EditDefaultsOnly, Category = "MainGameMode|Time")
	float matchTime{ 120.f };
	
	UPROPERTY(EditDefaultsOnly, Category = "MainGameMode|Time")
	float cooldownTime{ 10.f };

	float levelStartingTime{};
	float countdownTime{ 0.f };

	FTimerHandle notifyControllersTimer{};
	UPROPERTY(EditDefaultsOnly, Category = "MainGameMode|Time")
	float notifyControllersDelay{ 0.2f };

public:
	FORCEINLINE float GetWarmupTime() { return warmupTime; }
	FORCEINLINE float GetMatchTime() { return matchTime; }
	FORCEINLINE float GetLevelStartingTime() { return levelStartingTime; }
	FORCEINLINE float GetCooldownTime() { return cooldownTime; }

	FORCEINLINE bool IsTeamMatch() { return bTeamMatch; }
}; 

