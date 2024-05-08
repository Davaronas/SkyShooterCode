// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Projectile.h"
#include "ProjectileBullet.generated.h"


/**
 * 
 */
UCLASS()
class MUTLIPLAYERSHOOTER_API AProjectileBullet : public AProjectile
{
	GENERATED_BODY()

public:
	AProjectileBullet();


protected:
	virtual void OnHit
	(class UPrimitiveComponent* HitComponent,
		class AActor* OtherActor,
		class UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit) override;

	virtual bool DisableProjectileInTickOnClientCondition() override;

private:

	UPROPERTY(VisibleAnywhere)
	class UProjectileMovementComponent* projectileMovement{};
	
};
