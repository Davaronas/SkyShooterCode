// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickup/PickupSpawn.h"
#include "WeaponSpawn.generated.h"

/**
 * 
 */
UCLASS()
class MUTLIPLAYERSHOOTER_API AWeaponSpawn : public APickupSpawn
{
	GENERATED_BODY()
protected:
	AWeaponSpawn();
	virtual void SpawnPickup() override;
	virtual void PickupDestroyed() override;

	UPROPERTY(EditDefaultsOnly)
	TArray<TSubclassOf<class AWeapon>> weaponsToSpawn{};

	UPROPERTY(EditDefaultsOnly)
	float Zoffset{ 85.f };

	

private:
	UPROPERTY(VisibleAnywhere)
	class UBillboardComponent* billboard {};
	

	UFUNCTION(BlueprintPure, Category = Whatever)
	bool bIsWithEditor() const
	{
#if WITH_EDITOR
		return true;
#else
		return false;
#endif
	}
};
