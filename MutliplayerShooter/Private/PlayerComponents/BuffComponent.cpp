// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerComponents/BuffComponent.h"
#include "PlayerCharacter.h"
#include "TimerManager.h"
#include "PlayerComponents/CombatComponent.h"


UBuffComponent::UBuffComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	SetIsReplicatedByDefault(true);
}


void UBuffComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!ownerPlayer) ownerPlayer = Cast<APlayerCharacter>(GetOwner());
}


void UBuffComponent::HealPlayer(float amount, float duration)
{
	if (!ownerPlayer) ownerPlayer = Cast<APlayerCharacter>(GetOwner());

	if (ownerPlayer && ownerPlayer->HasAuthority())
	{

		bCurrentlyHealing = true;

		currentHealingRate = amount / duration;
		currentAmountToHeal += amount;
	}
}


// Does not work at the moment with weapon movement speeds, equipping a weapon
// cancels this, so this may only be used for an initial speed buff
void UBuffComponent::SpeedUpPlayer(float amount, float duration)
{
	if (!ownerPlayer) ownerPlayer = Cast<APlayerCharacter>(GetOwner());
	if (!ownerPlayer) { return; }
	if (!ownerPlayer->HasAuthority()) { return; }
	if (!ownerPlayer->GetCombatComponent()) { return; }


	UWorld* world = GetWorld();
	if (!world) { return; }
	FTimerManager& timerManager = world->GetTimerManager();

	bCurrentlySpedUp = true;
	speedIncrease = amount;
	speedIncreaseDuration = duration;

	ownerPlayer->GetCombatComponent()->IncreaseSpeed(amount);

	timerManager.SetTimer(speedTimerHandle, this, &ThisClass::ResetSpeed, speedIncreaseDuration);
}

void UBuffComponent::ResetSpeed()
{
	if (!ownerPlayer) ownerPlayer = Cast<APlayerCharacter>(GetOwner());
	if (!ownerPlayer) { return; }
	if (!ownerPlayer->HasAuthority()) { return; }
	if (!ownerPlayer->GetCombatComponent()) { return; }


	UWorld* world = GetWorld();
	if (!world) { return; }
	FTimerManager& timerManager = world->GetTimerManager();


	bCurrentlySpedUp = false;
	speedIncrease = 0.f;
	speedIncreaseDuration = 0.f;

	ownerPlayer->GetCombatComponent()->ResetSpeed();
	timerManager.ClearTimer(speedTimerHandle);
}




void UBuffComponent::AddDamageReduction(float amount, float duration)
{
	if (!ownerPlayer) ownerPlayer = Cast<APlayerCharacter>(GetOwner());
	if (!ownerPlayer) { return; }
	if (!ownerPlayer->HasAuthority()) { return; }


	UWorld* world = GetWorld();
	if (!world) { return; }
	FTimerManager& timerManager = world->GetTimerManager();


	bCurrentlyHasDamageReduction = true;
	damageReduction = FMath::Clamp(amount, 0.f, 1.f);
	damageReductionDuration = duration;

	timerManager.SetTimer(damageReductionTimerHandle, this, &ThisClass::ResetDamageReduction, damageReductionDuration);
}

void UBuffComponent::ResetDamageReduction()
{
	if (!ownerPlayer) ownerPlayer = Cast<APlayerCharacter>(GetOwner());
	if (!ownerPlayer) { return; }
	if (!ownerPlayer->HasAuthority()) { return; }

	UWorld* world = GetWorld();
	if (!world) { return; }
	FTimerManager& timerManager = world->GetTimerManager();

	bCurrentlyHasDamageReduction = false;
	damageReduction = 0.f;
	damageReductionDuration = 0.f;

	timerManager.ClearTimer(damageReductionTimerHandle);
}

void UBuffComponent::AddShield(float amount)
{
	if (!ownerPlayer) ownerPlayer = Cast<APlayerCharacter>(GetOwner());
	if (!ownerPlayer) { return; }
	if (!ownerPlayer->HasAuthority()) { return; }

	ownerPlayer->AddShield(amount);
}

void UBuffComponent::SpawnSpeedUp()
{
	ownerPlayer->GetBuffComponent()->SpeedUpPlayer(initialSpeedBuff, initialSpeedBuffDurationIfNotInterrupted);
}

void UBuffComponent::SpawnDamageReduction()
{
	#if !WITH_EDITOR
	ownerPlayer->GetBuffComponent()->AddDamageReduction(initialDamageReduction, initialDamageReductionDurationIfNotInterrupted);
	#endif
}




void UBuffComponent::HealRampup(float DeltaTime)
{
	if (!ownerPlayer) { return; }
	if (!ownerPlayer->HasAuthority()) { return; }
	if (!bCurrentlyHealing || ownerPlayer->IsDeadOrGameplayDisabled()) { return; }

	const float healThisFrame = currentHealingRate * DeltaTime;
	ownerPlayer->AddToHealth(healThisFrame);
	currentAmountToHeal -= healThisFrame;

	if (currentAmountToHeal <= 0.f || ownerPlayer->IsPlayerHealthFull())
	{
		StopHealing();
	}
}


// Called every frame
void UBuffComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	HealRampup(DeltaTime);
}

