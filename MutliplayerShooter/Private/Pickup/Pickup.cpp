// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickup/Pickup.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Components/StaticMeshComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Pickup/PickupSpawn.h"
#include "TimerManager.h"


APickup::APickup()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	overlapSphere = CreateDefaultSubobject<USphereComponent>(TEXT("Overlap Sphere"));
	overlapSphere->SetupAttachment(RootComponent);
	overlapSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	overlapSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	overlapSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	overlapSphere->AddLocalOffset(FVector{ 0.f, 0.f, 85.f });
	overlapSphere->SetSphereRadius(50.f);

	pickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Pickup Mesh"));
	pickupMesh->SetupAttachment(RootComponent);
	pickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	pickupMesh->AddLocalOffset(FVector{ 0.f, 0.f, 85.f });

	niagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("Niagara Component"));
	niagaraComponent->SetupAttachment(RootComponent);
	
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

}


void APickup::BeginPlay()
{
	Super::BeginPlay();

	if (pickupMesh && pickupMesh->GetStaticMesh() && bUseOutline)
	{
		pickupMesh->SetCustomDepthStencilValue((int32)outlineColor);
		pickupMesh->MarkRenderStateDirty();
		pickupMesh->SetRenderCustomDepth(true);
	}


	if(HasAuthority())
	GetWorldTimerManager().SetTimer(bindOverlapTimer, this, &ThisClass::BindOverlapTimerFinished, bindOverlapTime);

	
	
}

void APickup::OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{

}

void APickup::BindOverlapTimerFinished()
{
	if (HasAuthority())
	{
		overlapSphere->OnComponentBeginOverlap.AddDynamic(this, &APickup::OnOverlap);
	}
}


void APickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (pickupMesh && bShouldRotate)
	{
		RootComponent->AddLocalRotation
		(FVector{ 0.f, rotationSpeed, 0.f }.Rotation() * DeltaTime);
	}
}

void APickup::Destroyed()
{

	if (spawner && HasAuthority())
	{
		spawner->PickupDestroyed();
	}

	if (pickupSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, pickupSound, GetActorLocation());
	}

	if (pickupEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation
		(this, pickupEffect, GetActorLocation() + effectOffset, GetActorRotation(), FVector{effectScale});
	}

	Super::Destroyed();
}



