// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon/WeaponTypes.h"

#include "Pickup.generated.h"

UCLASS()
class MUTLIPLAYERSHOOTER_API APickup : public AActor
{
	GENERATED_BODY()
	
public:	
	APickup();
	virtual void Tick(float DeltaTime) override;
	virtual void Destroyed() override;
	FORCEINLINE void SetSpawner(class APickupSpawn* spawn) {spawner = spawn;}

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnOverlap
	(class UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

private:

	UPROPERTY()
	class APickupSpawn* spawner {};


	UPROPERTY(VisibleAnywhere)
	class USphereComponent* overlapSphere{};

	UPROPERTY(VisibleAnywhere)
	class UStaticMeshComponent* pickupMesh{};

	UPROPERTY(VisibleAnywhere)
	class UNiagaraComponent* niagaraComponent{};
	



	UPROPERTY(EditDefaultsOnly)
	bool bUseOutline{ true };
	UPROPERTY(EditDefaultsOnly)
	EWeaponOutlineDepthType outlineColor{};

	UPROPERTY(EditDefaultsOnly)
	bool bShouldRotate{ true };
	UPROPERTY(EditDefaultsOnly)
	float rotationSpeed{ 0.3f };

	

	UPROPERTY(EditDefaultsOnly)
	class USoundBase* pickupSound{};

	UPROPERTY(EditAnywhere, Category = "Pickup|Effect")
	class UNiagaraSystem* pickupEffect{};
	UPROPERTY(EditAnywhere, Category = "Pickup|Effect")
	float effectScale{ 1.f };
	UPROPERTY(EditAnywhere, Category = "Pickup|Effect")
	FVector effectOffset{};


	FTimerHandle bindOverlapTimer{};
	float bindOverlapTime{ 0.25f };
	void BindOverlapTimerFinished();


public:	
	

};
