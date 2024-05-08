// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickup/Pickup.h"
#include "SpeedPickup.generated.h"

/**
 * 
 */
UCLASS()
class MUTLIPLAYERSHOOTER_API ASpeedPickup : public APickup
{
	GENERATED_BODY()

protected:
	virtual void OnOverlap
	(class UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult) override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Pickup")
	float speedIncrease{ 250.f };
	UPROPERTY(EditDefaultsOnly, Category = "Pickup")
	float speedIncreaseDuration{ 15.f };
	
};
