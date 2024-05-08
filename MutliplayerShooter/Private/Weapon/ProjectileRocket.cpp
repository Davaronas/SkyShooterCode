// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ProjectileRocket.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"
#include "NiagaraSystem.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Weapon/RocketMovementComponent.h"




AProjectileRocket::AProjectileRocket()
{
	rocketMovementComponent = CreateDefaultSubobject<URocketMovementComponent>(TEXT("Rocket Movement"));
	rocketMovementComponent->bRotationFollowsVelocity = true;
	rocketMovementComponent->InitialSpeed = 5000.f;
	rocketMovementComponent->MaxSpeed = 8000.f;
	rocketMovementComponent->SetIsReplicated(true);
}

void AProjectileRocket::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AProjectileRocket::BeginPlay()
{
	Super::BeginPlay();
	CreateProjectileTrail();
	CreateRocketSound();
}

void AProjectileRocket::CreateRocketSound()
{
	if (rocketSoundLoop && rocketSoundAttenuation)
	{
		rocketSoundAudioComponent = UGameplayStatics::SpawnSoundAttached(rocketSoundLoop,
			GetRootComponent(), NAME_None, GetActorLocation(), EAttachLocation::KeepWorldPosition, true,
			1.f, 1.f, 0.f, rocketSoundAttenuation);
	}
}


void AProjectileRocket::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{

	// Can hit self immediately upon firing
	if(OtherActor == GetOwner())
	{
		return;
	}

	DoRadialDamage();

	//DeactivateTrail();
	Super::OnHit(HitComponent, OtherActor, OtherComp, NormalImpulse, Hit);
}

void AProjectileRocket::DoRadialDamage()
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
				damageInnerRadius, damageOuterRadius, damagefFalloff,
				UDamageType::StaticClass(), TArray<AActor*>(), this, firingPawnController);
		}
	}
}


void AProjectileRocket::OnClientTick_VelocityStopped()
{
	DeactivateTrail();
	DestroyRocketSoundComponent();
	Super::OnClientTick_VelocityStopped();
}


void AProjectileRocket::MulticastPlayImpactEffects_Override(bool bPlayer)
{
	Super::MulticastPlayImpactEffects_Override(bPlayer);
	DeactivateTrail();
	DestroyRocketSoundComponent();
}

void AProjectileRocket::DestroyRocketSoundComponent()
{
	if (rocketSoundAudioComponent)
	{
		rocketSoundAudioComponent->DestroyComponent();
		rocketSoundAudioComponent = nullptr;
	}
}

bool AProjectileRocket::DisableProjectileInTickOnClientCondition()
{
	return rocketMovementComponent && rocketMovementComponent->Velocity.Size() <= 100.f;
}
