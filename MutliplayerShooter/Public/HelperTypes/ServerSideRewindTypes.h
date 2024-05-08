#pragma once

#include "ServerSideRewindTypes.generated.h"

USTRUCT(BlueprintType)
struct FFireData
{
	GENERATED_BODY()

	UPROPERTY()
	bool one_Y{};
	UPROPERTY()
	bool one_P{};
	UPROPERTY()
	float randomScatter_Y{};
	UPROPERTY()
	float randomScatter_P{};
};

USTRUCT(BlueprintType)
struct FServerSideProjectileRewindData
{
	GENERATED_BODY()

	UPROPERTY()
	class APlayerCharacter* playerHit{ (APlayerCharacter*)nullptr };
	UPROPERTY()
	FVector_NetQuantize startLoc{};
	UPROPERTY()
	FVector_NetQuantize10 hitLoc{};


	// needed for checking player accuracy, 
	// most weapons hit almost instantly however
	UPROPERTY()
	float timeOfFiring{ 0.f };
	UPROPERTY()
	float timeOfHit{ 0.f };


};


// Could be optimized for shotgun, start loc and time of hit is always the same
USTRUCT(BlueprintType)
struct FServerSideRewindData
{
	GENERATED_BODY()

	UPROPERTY()
	class APlayerCharacter* playerHit{ (APlayerCharacter*)nullptr };
	UPROPERTY()
	FVector_NetQuantize startLoc{};
	UPROPERTY()
	FVector_NetQuantize10 hitLoc{};

	UPROPERTY()
	float timeOfHit{ 0.f };


	
};



