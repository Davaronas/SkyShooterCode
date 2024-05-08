// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Casing.generated.h"

UCLASS()
class MUTLIPLAYERSHOOTER_API ACasing : public AActor
{
	GENERATED_BODY()
	
public:	
	ACasing();
	virtual void Tick(float DeltaTime) override;


	UFUNCTION()
	virtual void OnHit
	(class UPrimitiveComponent* HitComponent,
		class AActor* OtherActor,
		class UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit);

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere)
	class UStaticMeshComponent* casingMesh{};

	UPROPERTY(EditDefaultsOnly)
	float ejectSpeedMin{ 15.f };
	UPROPERTY(EditDefaultsOnly)
	float ejectSpeedMax{ 35.f };

	UPROPERTY(EditDefaultsOnly)
	float lifeSpan{ 10.f };


	UPROPERTY(EditDefaultsOnly)
	class USoundBase* impactSound{};
	bool bSoundPlayed{ false };


public:	
	

};
