// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Projectile.h"
#include "ProjectileGrenade.generated.h"

/**
 * 
 */
UCLASS()
class MUTLIPLAYERSHOOTER_API AProjectileGrenade : public AProjectile
{
	GENERATED_BODY()

public:
	AProjectileGrenade();
	virtual void Destroyed() override;
	void PlayBlowupSound();
	void SpawnBlowupEffect();
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;


	UPROPERTY(EditDefaultsOnly, Category = "ProjectileGrenade|Damage")
	float baseDamage{ 100.f };
	UPROPERTY(EditDefaultsOnly, Category = "ProjectileGrenade|Damage")
	float minimumDamage{ 5.f };
	UPROPERTY(EditDefaultsOnly, Category = "ProjectileGrenade|Damage")
	float damageInnerRadius{ 150.f };
	UPROPERTY(EditDefaultsOnly, Category = "ProjectileGrenade|Damage")
	float damageOuterRadius{ 800.f };

	void DoRadialDamage();

private:

	UPROPERTY(EditDefaultsOnly, Category = "ProjectileGrenade|LifeSpan")
	float lifeSpanAfterSpawn{ 6.f };
	UPROPERTY(EditDefaultsOnly, Category = "ProjectileGrenade|LifeSpan")
	bool bDecreaseLifeSpanOnBounce{ false };
	UPROPERTY(EditDefaultsOnly, Category = "ProjectileGrenade|LifeSpan")
	float lifeSpanDecreaseOnBounce{ 0.2f };

	float lifeSpanTimer{};

	UPROPERTY(VisibleAnywhere)
	class UProjectileMovementComponent* projectileMovement{};

	UFUNCTION()
	void OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity);

	


	void PlayBounceSound();

	UPROPERTY(EditDefaultsOnly, Category = "ProjectileGrenade|Effects")
	class USoundBase* bounceSound{};
	UPROPERTY(EditDefaultsOnly, Category = "ProjectileGrenade|Effects")
	float bounceVelocitySoundThreshold = 200.f;
	UPROPERTY(EditDefaultsOnly, Category = "ProjectileGrenade|Effects")
	class UParticleSystem* blowupEffect{};
	UPROPERTY(EditDefaultsOnly, Category = "ProjectileGrenade|Effects")
	class USoundBase* blowupSound{};
	
};
