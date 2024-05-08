// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "HelperTypes/Teams.h"
#include "MainPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class MUTLIPLAYERSHOOTER_API AMainPlayerState : public APlayerState
{
	GENERATED_BODY()
public:
	virtual void OnRep_Score() override;
	
	void AddToScore(float amount = 1.f);
	void UpdateHUD();
	void HideAmmoHUD();
	void SetGrenadesOnHUD();
	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override;

	void AddToDeaths(float amount = 1.f);

	void SetDeathsOnHUD();

protected:
	void SetScoreOnHUD();
	void SetHealthOnHUD();
	void SetShieldOnHUD();
	void TrySetPlayer();
	void TrySetPlayerController();
	void TrySetPlayerCharacter();

	virtual void BeginPlay() override;

	UPROPERTY(ReplicatedUsing = OnRep_deaths, VisibleAnywhere)
	int32 deaths{};
	UFUNCTION()
	virtual void OnRep_deaths();

	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_playerTeam)
	ETeams playerTeam{ ETeams::ET_NoTeam };
	UFUNCTION()
	void OnRep_playerTeam();


private:
	// This needs UPROPERTY because when the character is destroyed, this pointer
	// still points to the same location, which is now garbage data
	// yet we will pass the null checks, because it's not a null pointer
	// we don't know when they get deleted, so we don't know where to set them to nullptr
	// UPROPERTY takes care of this
	UPROPERTY()
	class APlayerCharacter* currentPlayerCharacter{};
	UPROPERTY()
	class AMainPlayerController* currentPlayerController{};

public:
	FORCEINLINE ETeams GetTeam() { return playerTeam; }
	FORCEINLINE float GetDeaths() { return deaths; }
	void SetTeam(ETeams newTeam);

	void SetCharacterColor(ETeams newTeam);

	
	class AController* GetLastDamagingInstigatorController();
	
	
};


