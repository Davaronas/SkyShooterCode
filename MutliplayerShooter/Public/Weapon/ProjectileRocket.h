// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Projectile.h"
#include "ProjectileRocket.generated.h"



/**
 * 
 */
UCLASS()
class MUTLIPLAYERSHOOTER_API AProjectileRocket : public AProjectile
{
	GENERATED_BODY()

public:
	AProjectileRocket();
	virtual void Tick(float DeltaTime) override;

protected:

	virtual void BeginPlay() override;

	void CreateRocketSound();



	virtual void OnHit
	(class UPrimitiveComponent* HitComponent,
		class AActor* OtherActor,
		class UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit);

	void DoRadialDamage();

	UPROPERTY(EditDefaultsOnly, Category = "ProjectileRocket|Damage")
	float baseDamage{ 200.f };
	UPROPERTY(EditDefaultsOnly, Category = "ProjectileRocket|Damage")
	float minimumDamage{ 15.f };
	UPROPERTY(EditDefaultsOnly, Category = "ProjectileRocket|Damage")
	float damageInnerRadius{ 100.f };
	UPROPERTY(EditDefaultsOnly, Category = "ProjectileRocket|Damage")
	float damageOuterRadius{ 600.f };
	UPROPERTY(EditDefaultsOnly, Category = "ProjectileRocket|Damage")
	float damagefFalloff{ 1.2f };


	virtual void OnClientTick_VelocityStopped() override;

	

	virtual void MulticastPlayImpactEffects_Override(bool bPlayer) override;

	void DestroyRocketSoundComponent();

	virtual bool DisableProjectileInTickOnClientCondition() override;

private:

	UPROPERTY(VisibleAnywhere)
	class URocketMovementComponent* rocketMovementComponent{};



	UPROPERTY(EditDefaultsOnly, Category = "Projectile|Effects")
	class USoundBase* rocketSoundLoop{};
	UPROPERTY()
	class UAudioComponent* rocketSoundAudioComponent{};



	// Click PlayWhenSilent on Cue as well, otherwise it won't play if too far
	// Need to set attenuation when creating a component like this
	UPROPERTY(EditDefaultsOnly, Category = "Projectile|Effects")
	class USoundAttenuation* rocketSoundAttenuation{};
	
	
};
