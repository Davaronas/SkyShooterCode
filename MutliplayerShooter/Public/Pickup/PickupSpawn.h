// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PickupSpawn.generated.h"

UCLASS()
class MUTLIPLAYERSHOOTER_API APickupSpawn : public AActor
{
	GENERATED_BODY()
	
public:	
	APickupSpawn();
	virtual void Tick(float DeltaTime) override;

	virtual void PickupDestroyed();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly)
	TArray<TSubclassOf<class APickup>> pickupsToSpawn{};

	void StartSpawnPickupTimer();
	virtual void SpawnPickup();

	UPROPERTY(EditDefaultsOnly)
	bool bSpawnOnce{ false };

	bool bFirstSpawn{ true };
	float warmupTime{};

private:

	UPROPERTY(EditAnywhere)
	float spawnMinTime{10.f};
	UPROPERTY(EditAnywhere)
	float spawnMaxTime{ 120.f };
	UPROPERTY(EditAnywhere)
	float startSpawningFromBeginPlayDelay = 2.f;
	FTimerHandle spawnTimerHandle{};

	void StartSpawning();


public:	
	

};
