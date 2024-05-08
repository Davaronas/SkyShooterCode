// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickup/Pickup.h"
#include "ShieldPickup.generated.h"

/**
 * 
 */
UCLASS()
class MUTLIPLAYERSHOOTER_API AShieldPickup : public APickup
{
	GENERATED_BODY()

public:
	AShieldPickup();

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
	float shieldAmount{ 100.f };
	
};
