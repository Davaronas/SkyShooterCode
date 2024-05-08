// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Projectile.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "PlayerCharacter.h"
#include "../Source/MutliplayerShooter/MutliplayerShooter.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraSystemInstance.h"
#include "NiagaraSystemInstanceController.h"


#include "DebugMacros.h"

//#define ECollisionChannel_SkeletalMesh ECC_GameTraceChannel1

AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	collisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision Box"));
	collisionBox->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	collisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	collisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	collisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Ignore);
	collisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
	collisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	collisionBox->SetCollisionResponseToChannel(ECollisionChannel_SkeletalMesh, ECollisionResponse::ECR_Block);
	collisionBox->SetNotifyRigidBodyCollision(true);

	SetRootComponent(collisionBox);

	projectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Projectile Mesh"));
	projectileMesh->SetupAttachment(RootComponent);
	projectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	projectileMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Ignore);

	
}



void AProjectile::BeginPlay()
{
	Super::BeginPlay();

	ownerPlayer = Cast<APlayerCharacter>(GetOwner());

	if(collisionBox)
	collisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Ignore);
	if(projectileMesh)
	projectileMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Ignore);

	if (collisionBox)
	{
		if (bReplicates && IsOwnerLocallyControlled())
		{
			collisionBox->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);
		}
		else if (HasAuthority())
		{
			collisionBox->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);
		}

		
	}
		

	if (!projectileTracer ||
		(bReplicates && ownerPlayer &&
			ownerPlayer->GetLocalRole() == ENetRole::ROLE_AutonomousProxy)) { return; }

	projectileTracerComponent = UGameplayStatics::SpawnEmitterAttached
	(projectileTracer, collisionBox, NAME_None, GetActorLocation(), GetActorRotation(), EAttachLocation::KeepWorldPosition);
}

void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!HasAuthority() && !bTurnedOffOnClient && DisableProjectileInTickOnClientCondition())
	{
		OnClientTick_VelocityStopped();
	}

}

//projectileMovement&& projectileMovement->Velocity.Size() <= 100.f

void AProjectile::OnClientTick_VelocityStopped()
{
	//DisableProjectile();
	bTurnedOffOnClient = true;
}

void AProjectile::OnHit(UPrimitiveComponent* HitComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	if (bHit) { return; }
	bHit = true;

	if (!ownerPlayer) ownerPlayer = Cast<APlayerCharacter>(GetOwner());
	if (!ownerPlayer) { return; }


	
	if (HasAuthority() && bReplicates)
	{
		APlayerCharacter* hitPlayerCharacter = Cast<APlayerCharacter>(OtherActor);

		if (hitPlayerCharacter)
		{
			bPlayerHit = true;
		}

		ServerPlayImpactEffects(bPlayerHit);

		if (bSetLifeSpanAfterImpact)
			SetLifeSpan(lifeSpanAfterImpact);
	}

	if (!IsOwnerServer() && IsOwnerLocallyControlled() && bReplicates)
	{
		DisableProjectile();
	}
}

bool AProjectile::IsOwnerServer()
{
	return GetOwner() && GetOwner()->HasAuthority();
}

void AProjectile::PlayImpactEffects()
{
	if (impactParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation
		(GetWorld(), impactParticles, GetActorLocation(), GetActorRotation(), true, EPSCPoolMethod::AutoRelease);
	}

	if (impactSoundEffect)
	{
		UGameplayStatics::PlaySoundAtLocation(this, impactSoundEffect, GetActorLocation());
	}
}

void AProjectile::ServerPlayImpactEffects_Implementation(bool bPlayer)
{
	MulticastPlayImpactEffects(bPlayer);
}


void AProjectile::MulticastPlayImpactEffects_Implementation(bool bPlayer)
{
	//DRAW_SPHERE_BIG_B(GetActorLocation())

	MulticastPlayImpactEffects_Override(bPlayer);

	bPlayerHit = bPlayer;
	if (bPlayerHit)
	{
		impactParticles = playerImpactParticles;
		impactSoundEffect = playerImpactSoundEffect;
	}
	else
	{
		impactParticles = normalImpactParticles;
		impactSoundEffect = normalImpactSoundEffect;
	}


	DisableProjectile();
	PlayImpactEffects();
	//SetLifeSpan(0.1f);
	//Destroy();
}

void AProjectile::DisableProjectile()
{
	if (IsOwnerLocallyControlled() && !IsOwnerServer())
	{
		if (bHit)
		{
			if (collisionBox) collisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
	else
	{
		if (collisionBox) collisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (projectileTracerComponent) projectileTracerComponent->SetVisibility(false);
	if (projectileMesh) projectileMesh->SetVisibility(false);
}

void AProjectile::CreateProjectileTrail()
{

	if (projectileTrailNiagaraSystem)
	{
		projectileTrailNiagaraSystemComponent =
			UNiagaraFunctionLibrary::SpawnSystemAttached
			(projectileTrailNiagaraSystem,
				GetRootComponent(),
				NAME_None,
				GetActorLocation(),
				GetActorRotation(),
				EAttachLocation::KeepWorldPosition,
				false);
	}
}

void AProjectile::DeactivateTrail()
{
	if (projectileTrailNiagaraSystemComponent)
	{
		if (auto system = projectileTrailNiagaraSystemComponent->GetSystemInstanceController())
		{
			system->Deactivate();
		}
	}
}



void AProjectile::MulticastPlayImpactEffects_Override(bool bPlayer)
{

}

bool AProjectile::IsOwnerLocallyControlled()
{
	if (APawn* pawn = Cast<APawn>(GetOwner()))
	{
		if (pawn->IsLocallyControlled())
		{
			return true;
		}
	}

	return false;
}

bool AProjectile::IsOwnerServerAndLocallyControlled()
{
	return ownerPlayer && ownerPlayer->HasAuthority() && ownerPlayer->IsLocallyControlled();
}





void AProjectile::Destroyed()
{
	Super::Destroyed();



}

