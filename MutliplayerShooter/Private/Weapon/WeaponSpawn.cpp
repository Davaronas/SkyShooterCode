// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/WeaponSpawn.h"
#include "Weapon/Weapon.h"
#include "Components/BillboardComponent.h"
#include "Components/SceneComponent.h"
#include "Components/BillboardComponent.h"
#include "DebugMacros.h"

AWeaponSpawn::AWeaponSpawn()
{ 
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	billboard = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
	billboard->SetupAttachment(RootComponent);

	FVector offsetedLoc = GetActorLocation();
	offsetedLoc.Z += 20.f;
	billboard->SetWorldLocation(offsetedLoc);



}

void AWeaponSpawn::SpawnPickup()
{
	if (!HasAuthority()) { return; }

	const int32 numPickupClasses = weaponsToSpawn.Num();
	if (numPickupClasses <= 0) { return; }
	UWorld* world = GetWorld();
	if (!world) { return; }

	const int32 spawnIndex = FMath::RandRange(0, numPickupClasses - 1);
	//GEPR_Y(*FString::Printf(TEXT("Spawn index: %d"), spawnIndex));

	FTransform offsetedTransform = GetActorTransform();
	FVector offsetedLoc = GetActorLocation();
	offsetedLoc.Z += Zoffset;
	offsetedTransform.SetLocation(offsetedLoc);

	FRotator rot = GetActorRotation();
	rot.Yaw = FMath::RandRange(0.f, 360.f);
	offsetedTransform.SetRotation(rot.Quaternion());

	AWeapon* newPickup = world->SpawnActor<AWeapon>(weaponsToSpawn[spawnIndex], offsetedTransform);

	if (newPickup)
		newPickup->SetSpawner(this);
}

void AWeaponSpawn::PickupDestroyed()
{
	if (bSpawnOnce)
	{
		Destroy();
	}
	else
	{
		Super::PickupDestroyed();
	}
}
