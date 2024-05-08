// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Casing.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Sound/SoundBase.h"

// Sets default values
ACasing::ACasing()
{
	PrimaryActorTick.bCanEverTick = false;

	casingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Casing Mesh"));
	casingMesh->SetSimulatePhysics(true);
	casingMesh->SetEnableGravity(true);
	casingMesh->SetNotifyRigidBodyCollision(true);

	casingMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	casingMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	casingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	casingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	//casingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Ignore);
	SetRootComponent(casingMesh);
}

void ACasing::BeginPlay()
{
	Super::BeginPlay();
	
	float ejectSpeed = FMath::RandRange(ejectSpeedMin, ejectSpeedMax);
	FVector ejectVector{ GetActorForwardVector() * ejectSpeed };
	casingMesh->AddImpulse(ejectVector);
	casingMesh->AddAngularImpulseInDegrees(GetActorUpVector() * ejectSpeed);

	casingMesh->OnComponentHit.AddDynamic(this, &ACasing::OnHit);
	SetLifeSpan(lifeSpan);
}

void ACasing::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ACasing::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!bSoundPlayed && impactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, impactSound, GetActorLocation());
		bSoundPlayed = true;
	}
}

