// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/RocketMovementComponent.h"

URocketMovementComponent::EHandleBlockingHitResult URocketMovementComponent::HandleBlockingHit
(const FHitResult& Hit, float TimeTick, const FVector& MoveDelta, float& SubTickTimeRemaining)
{
	Super::HandleBlockingHit(Hit, TimeTick, MoveDelta, SubTickTimeRemaining);

	return URocketMovementComponent::EHandleBlockingHitResult::AdvanceNextSubstep;
}

void URocketMovementComponent::HandleImpact
(const FHitResult& Hit, float TimeSlice, const FVector& MoveDelta)
{
	//Super::HandleImpact(Hit, TimeSlice, MoveDelta);

	// Rockets should not stop

}
