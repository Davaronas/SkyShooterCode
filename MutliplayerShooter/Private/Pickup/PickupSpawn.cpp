// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickup/PickupSpawn.h"
#include "Pickup/Pickup.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "GameMode/MainGameMode.h"


// Sets default values
APickupSpawn::APickupSpawn()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void APickupSpawn::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		UWorld* world = GetWorld();
		if (!world) { return; }

		world->GetTimerManager().
			SetTimer(spawnTimerHandle, this, &ThisClass::StartSpawning, startSpawningFromBeginPlayDelay);
		
	}
}

void APickupSpawn::StartSpawning()
{
	UWorld* world = GetWorld();
	if (!world) { return; }
	AMainGameMode* gameMode = world->GetAuthGameMode<AMainGameMode>();

	if (gameMode)
	{
		warmupTime = gameMode->GetWarmupTime();
	}

	spawnMinTime += warmupTime;
	spawnMaxTime += warmupTime;
	StartSpawnPickupTimer();
}

void APickupSpawn::StartSpawnPickupTimer()
{
	if (!HasAuthority()) { return; }

	UWorld* world = GetWorld();
	if (!world) { return; }
	FTimerManager& timerManager = world->GetTimerManager();
	const float spawnTime = FMath::RandRange(spawnMinTime, spawnMaxTime);

	timerManager.SetTimer(spawnTimerHandle, this, &ThisClass::SpawnPickup, spawnTime);
}

void APickupSpawn::SpawnPickup()
{
	if (!HasAuthority()) { return; }

	const int32 numPickupClasses = pickupsToSpawn.Num();
	if (numPickupClasses <= 0) { return; }
	UWorld* world = GetWorld();
	if (!world) { return; }

	const int32 spawnIndex = FMath::RandRange(0, numPickupClasses - 1);

	APickup* newPickup = world->SpawnActor<APickup>(pickupsToSpawn[spawnIndex], GetActorTransform());

	if(newPickup)
	newPickup->SetSpawner(this);
}



// Called every frame
void APickupSpawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void APickupSpawn::PickupDestroyed()
{
	if (bSpawnOnce)
	{
		Destroy();
	}
	else
	{
		if (bFirstSpawn)
		{
			bFirstSpawn = false;
			spawnMinTime -= warmupTime;
			spawnMaxTime -= warmupTime;
		}
		
		StartSpawnPickupTimer();
	}
}

