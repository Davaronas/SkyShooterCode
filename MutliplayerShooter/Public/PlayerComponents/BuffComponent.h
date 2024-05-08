// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuffComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MUTLIPLAYERSHOOTER_API UBuffComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UBuffComponent();
	friend class APlayerCharacter;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void HealPlayer(float amount, float duration);
	FORCEINLINE void StopHealing() { bCurrentlyHealing = false; currentAmountToHeal = 0.f; currentHealingRate = 0.f;}

	void SpeedUpPlayer(float amount, float duration);
	void ResetSpeed();

	void AddDamageReduction(float amount, float duration);
	void ResetDamageReduction();

	void AddShield(float amount);

	void SpawnSpeedUp();
	void SpawnDamageReduction();


protected:
	virtual void BeginPlay() override;

	void HealRampup(float DeltaTime);

private:

	// Set from PostInitializeComponents on PlayerCharacter
	UPROPERTY()
	class APlayerCharacter* ownerPlayer{};

	/*
	* Health
	*/

	bool bCurrentlyHealing{ false };
	float currentHealingRate{ 0.f };
	float currentAmountToHeal{ 0.f };


	/*
	* Speed
	*/

	bool bCurrentlySpedUp{ false };
	float speedIncrease{ 0.f };
	float speedIncreaseDuration{ 0.f };
	FTimerHandle speedTimerHandle{};

	bool bCurrentlyHasDamageReduction{ false };
	float damageReduction{ 0.f };
	float damageReductionDuration{ 0.f };
	FTimerHandle damageReductionTimerHandle{};


	UPROPERTY(EditDefaultsOnly, Category = "BuffComponent|SpawnBuffs")
	float initialSpeedBuff = 200.f;
	UPROPERTY(EditDefaultsOnly, Category = "BuffComponent|SpawnBuffs")
	float initialSpeedBuffDurationIfNotInterrupted{ 20.f };


	// Meant as percentage: goes from 0 to 1 (0 - 100 %)
	// 0.5 amounts to 50 % damage reduction
	UPROPERTY(EditDefaultsOnly, Category = "BuffComponent|SpawnBuffs")
	float initialDamageReduction = 0.9f;
	UPROPERTY(EditDefaultsOnly, Category = "BuffComponent|SpawnBuffs")
	float initialDamageReductionDurationIfNotInterrupted{ 10.f };

public:	

	FORCEINLINE bool IsCurrentlySpedUp() { return bCurrentlySpedUp; }

	FORCEINLINE bool CurrenthyHasDamageReduction() { return bCurrentlyHasDamageReduction; }
	FORCEINLINE float GetDamageReduction() { return damageReduction; }
		
};
