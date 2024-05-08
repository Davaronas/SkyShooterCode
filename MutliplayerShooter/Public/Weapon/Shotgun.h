// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/HitScanWeapon.h"
#include "Shotgun.generated.h"

/**
 * 
 */
UCLASS()
class MUTLIPLAYERSHOOTER_API AShotgun : public AHitScanWeapon
{
	GENERATED_BODY()

public:
	virtual void Fire(const FVector& shootFromPos, const FVector& targetPos) override;
	virtual void Fire(const FVector& shootFromPos, const FVector& targetPos,
		TArray<FFireData>& outFireDatas, TArray<FServerSideRewindData>& outSSRDatas);

protected:
	UFUNCTION(Server, Reliable)
	void ServerHitScansEffects(const TArray<FVector_NetQuantize>& hitPositions,
		const TArray<EHitScanResult>& hitScanResArr);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastHitScansEffects(const TArray<FVector_NetQuantize>& hitPositions,
		const TArray<EHitScanResult>& hitScanResArr);



private:
	UPROPERTY(EditAnywhere, Category = "Shotgun|WeaponAttributes")
	uint8 shotgunPellets{ 7 };

public:
	FORCEINLINE uint8 GetPelletCount() { return shotgunPellets; }
	
};
