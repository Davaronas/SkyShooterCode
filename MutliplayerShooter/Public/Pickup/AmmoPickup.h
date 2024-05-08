// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickup/Pickup.h"


#include "AmmoPickup.generated.h"

/**
 * 
 */
UCLASS()
class MUTLIPLAYERSHOOTER_API AAmmoPickup : public APickup
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
	EWeaponType ammoType{};

	UPROPERTY(EditDefaultsOnly, Category = "Pickup")
	int32 ammoAmount{};

	
	
};
