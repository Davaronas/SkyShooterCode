// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HelperTypes/ServerSideRewindTypes.h"
#include "LagCompensationComponent.generated.h"




USTRUCT(BlueprintType)
struct FRewindBoxInformation
{
	GENERATED_BODY()

	UPROPERTY()
	FVector location{ FVector{} };
	UPROPERTY()
	FRotator rotation{ FRotator{} };
	UPROPERTY()
	FVector boxExtent{ FVector{} };
};

USTRUCT(BlueprintType)
struct FFramePackage
{
	GENERATED_BODY()

	UPROPERTY()
	float time{};

	UPROPERTY()
	TMap<FName, FRewindBoxInformation> hitBoxInfo{};
};

USTRUCT(BlueprintType)
struct FFrameAccuracy
{
	GENERATED_BODY()

	UPROPERTY()
	float time{};

	UPROPERTY()
	float accuracy;
};

USTRUCT(BlueprintType)
struct FServerSideRewindResult
{
	GENERATED_BODY()

	UPROPERTY()
	bool bHitConfirmed{false};

	UPROPERTY()
	bool bHeadshot{false};
};


UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MUTLIPLAYERSHOOTER_API ULagCompensationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULagCompensationComponent();
	friend class APlayerCharacter;



	/*void ServerHitRequest_HitScan(class APlayerCharacter* playerHit,
		const FVector_NetQuantize& startLoc, const FVector_NetQuantize& hitLoc,
		const float& timeOfHit, const float& singleTripTime,
		class AHitScanWeapon* damageCauserWeapon);*/

	UFUNCTION(Server, Reliable)
	void ServerHitRequest_HitScan(const FServerSideRewindData& debate, class AHitScanWeapon* damageCauserWeapon);

	UFUNCTION(Server, Reliable)
	void ServerHitRequests_HitScan(const TArray<FServerSideRewindData>& debates,
		class AHitScanWeapon* damageCauserWeapon);

	UFUNCTION(Server, Reliable)
	void ServerHitRequest_Projectile(const FServerSideRewindData& debate,
		class AProjectileWeapon* damageCauserWeapon);

	/*UFUNCTION(Server, Reliable)
	void ServerHitRequest_Projectile(class APlayerCharacter* playerHit,
		const FVector_NetQuantize& startLoc, const FVector_NetQuantize& hitLoc,
		const float& timeOfHit,
		class AProjectile* damageCauserProjectile);*/


	FFramePackage GetFrameToCheck(const APlayerCharacter* playerHit, const float& timeOfHit);
	FFrameAccuracy GetFrameToCheck_Accuracy(const float& timeOfHit);

protected:
	virtual void BeginPlay() override;

	

	TDoubleLinkedList<FFramePackage> frameLocationHistory{};
	TDoubleLinkedList<FFrameAccuracy> frameAccuracyHistory{};

	FServerSideRewindResult ServerSideRewind
	(class APlayerCharacter* playerHit,
		const FVector_NetQuantize& startLoc, const FVector_NetQuantize10& hitLoc,
		const float& timeOfHit, FVector& successfulHitLoc, bool bManualReset = false);

	
	FServerSideRewindResult ConfirmHit
	(const FFramePackage& frameToCheck,
		class APlayerCharacter* playerHit,
		const FVector_NetQuantize& startLoc,
		const FVector_NetQuantize10& hitLoc, FVector& successfulHitLoc, bool bManualReset = false);

	


	

	void CacheBoxPositions(class APlayerCharacter* playerHit, FFramePackage& outFramePackage);
	void MoveBoxes(class APlayerCharacter* playerHit, const FFramePackage& framePackage);
	void SetBoxesToFramePackage(APlayerCharacter* playerHit, const FFramePackage& framePackage, bool bReset = false);
	void ResetBoxes(class APlayerCharacter* playerHit, const FFramePackage& framePackage);


	void SaveFramePackage(FFramePackage& package);
	void SaveFrameAccuracy(FFrameAccuracy& accPackage);
	FFramePackage InterpolateFrames(const FFramePackage& older, const FFramePackage& younger, const float& timeOfHit);
	FFrameAccuracy InterpolateFrames(const FFrameAccuracy& older, const FFrameAccuracy& younger, const float& timeOfHit);
	void DeleteExpiredFrames();
	void SaveThisFrame();

	float GetLocationFrameHistoryLength();
	float GetAccuracyFrameHistoryLength();
	bool IsThisTimeExpired(const float& time);

	UPROPERTY()
	class APlayerCharacter* ownerPlayer{};
	UPROPERTY()
	class UCombatComponent* ownerCombatComponent{};

	UPROPERTY()
	class AMainPlayerController* ownerPlayerController{};

private:

	UPROPERTY(EditAnywhere)
	float frameHistoryMaxStoredLength = 2.f;

	class UWorld* world{};

public:
	virtual void TickComponent
	(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	FORCEINLINE TDoubleLinkedList<FFramePackage>& GetFrameHistory() { return frameLocationHistory; }



	

	

		
};
