// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickup/Pickup.h"
#include "HealthPickup.generated.h"

/**
 * 
 */
UCLASS()
class MUTLIPLAYERSHOOTER_API AHealthPickup : public APickup
{
	GENERATED_BODY()
public:
	AHealthPickup();

protected:
	virtual void OnOverlap
	(class UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult) override;

private:
	// You will heal this amount over the duration
	UPROPERTY(EditDefaultsOnly, Category = "Pickup")
	float healAmount{ 100.f };
	// The HealAmount will be received under this amount of time
	// Shorter duration will result in a quicker heal.
	UPROPERTY(EditDefaultsOnly, Category = "Pickup")
	float healDuration{2.f};
};
