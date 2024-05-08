// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Weapon.h"
#include "ProjectileWeapon.generated.h"



/**
 * 
 */
UCLASS()
class MUTLIPLAYERSHOOTER_API AProjectileWeapon : public AWeapon
{
	GENERATED_BODY()

public:
	virtual void Fire(const FVector& shootFromPos, const FVector& targetPos) override;
	virtual void AutProxyFire(const FVector& shootFromPos, const FVector& targetPos, FVector& outActualHitLoc);


	

private:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class AProjectile> projectileToSpawnClass{};

	// projectile for local client shooting
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class AProjectile> projectileToSpawnClass_AutProxy{};

	UPROPERTY(EditAnywhere)
	float headshotDamage{};

public:
	FORCEINLINE TSubclassOf<class AProjectile> GetProjectileToSpawn() { return projectileToSpawnClass; }

	FORCEINLINE float GetBaseDamage() { return baseDamage; }
	FORCEINLINE float GetHeadshotDamage() { return headshotDamage; }




};
