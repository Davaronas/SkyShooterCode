// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ProjectileGrenade.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"

#define LINEAR_DAMAGE_DECREASE 1.f

AProjectileGrenade::AProjectileGrenade()
{
	if (HasAuthority())
	SetActorTickEnabled(true);

	SetReplicatingMovement(true);

	projectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Projectile Movement"));
	projectileMovement->bRotationFollowsVelocity = true;
	projectileMovement->bShouldBounce = true;

	projectileMovement->InitialSpeed = 1600.f;
	projectileMovement->MaxSpeed = 1600.f;
	projectileMovement->SetIsReplicated(true);
}



void AProjectileGrenade::BeginPlay()
{
	// We don't need tracers on grenade
	// and we don't need the OnHit event
	// from Projectile
	AActor::BeginPlay();

	// Cosmetic
	projectileMovement->OnProjectileBounce.AddDynamic(this, &AProjectileGrenade::OnBounce);

	CreateProjectileTrail();

	if(HasAuthority())
	lifeSpanTimer = lifeSpanAfterSpawn;

//	SetLifeSpan(lifeSpanAfterSpawn);
}

void AProjectileGrenade::DoRadialDamage()
{
	if (!HasAuthority()) { return; }

	APawn* firingPawn = GetInstigator();
	if (firingPawn)
	{
		APlayerController* firingPawnController = GetInstigatorController<APlayerController>();

		if (firingPawnController)
		{
			UGameplayStatics::ApplyRadialDamageWithFalloff
			(this, baseDamage, minimumDamage, GetActorLocation(),
				damageInnerRadius, damageOuterRadius, LINEAR_DAMAGE_DECREASE,
				UDamageType::StaticClass(), TArray<AActor*>(), this, firingPawnController);
		}
	}
}


void AProjectileGrenade::Destroyed()
{
	DoRadialDamage();
	SpawnBlowupEffect();
	PlayBlowupSound();
	Super::Destroyed();
}

void AProjectileGrenade::OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	if (ImpactVelocity.Size() > bounceVelocitySoundThreshold)
	{
		if (bDecreaseLifeSpanOnBounce)
		{
			lifeSpanTimer -= lifeSpanDecreaseOnBounce;
		}
		
		PlayBounceSound();
	}
}

void AProjectileGrenade::PlayBounceSound()
{
	if (bounceSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, bounceSound, GetActorLocation());
	}
}


void AProjectileGrenade::PlayBlowupSound()
{
	if (blowupSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, blowupSound, GetActorLocation());
	}
}

void AProjectileGrenade::SpawnBlowupEffect()
{
	if (blowupEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), blowupEffect, GetActorTransform());
	}
}

void AProjectileGrenade::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HasAuthority())
	{
		lifeSpanTimer -= DeltaTime;

		if (lifeSpanTimer <= 0.f)
		{
			Destroy();
		}
	}
}
