// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Weapon.h"

#include "HitScanWeapon.generated.h"

UENUM()
enum class EHitScanResult : uint8
{
	EHSR_NoHit,
	EHRS_NonPlayerHit,
	EHRS_PlayerHit,
	EHRS_MAX
};

UENUM()
enum class EClientSideHitDebate : uint8
{
	ECSHD_NotDetermined,
	ECSHD_AcceptServerResult,
	ECSHD_DebateServerResult,

	ECSHD_MAX
};


USTRUCT()
struct FClientSideHitDebateResult
{
	GENERATED_BODY()

	UPROPERTY()
	EClientSideHitDebate debateResult{ EClientSideHitDebate::ECSHD_NotDetermined };

	UPROPERTY()
	class APlayerCharacter* playerHit{ nullptr };

	UPROPERTY()
	FVector_NetQuantize startLoc{};
	UPROPERTY()
	FVector_NetQuantize hitLoc{};

	UPROPERTY()
	float timeOfHit{ 0.f };
	UPROPERTY()
	float singleTripTime{ 0.f };

	FORCEINLINE bool DebatesServer() const { return debateResult == EClientSideHitDebate::ECSHD_DebateServerResult; }
};



/**
 * 
 */
UCLASS()
class MUTLIPLAYERSHOOTER_API AHitScanWeapon : public AWeapon
{
	GENERATED_BODY()

public:
	virtual void Fire(const FVector& shootFromPos, const FVector& targetPos) override;
	virtual void Fire(const FVector& shootFromPos, const FVector& targetPos,
		FFireData& outFireData, FServerSideRewindData& outSSRData, FVector& outActualHitLoc) override;


	

protected:

	

	EHitScanResult hitRes{};

	FVector FireHitScanWeapon(const FVector& shootFromPos, const FVector& targetPos, bool bSendServer = true);
	// Local Fire
	void FireHitScanWeapon(const FVector& shootFromPos, const FVector& targetPos,
		FFireData& outFireData, FServerSideRewindData& outSSRData, FVector& outActualHitLoc);
	FVector FireHitScanWeapon
	(const FVector& shootFromPos, const FVector& targetPos, EHitScanResult& hitScanRes,
		class APlayerCharacter*& outPlayerHit, float& actualDamage, bool bSendEffectsServer = false);

	void HandleHitTarget
	(FHitResult& fireHitRes, FVector& beamEnd, EHitScanResult& hitScanRes,
		class APlayerCharacter*& outPlayerHit, float& actualDamage, bool bSendServer = true);

	void HandleHitTarget
	(FHitResult& fireHitRes, FVector& beamEnd, EHitScanResult& hitScanRes);

//	class APlayerCharacter* HandleHitTarget
//	(FHitResult& fireHitRes, FVector& beamEnd, EHitScanResult& hitScanRes,
//  float& actualDamage, bool bSendDamageServer = false);


	void SpawnImpactParticles(const FVector& fireHitRes);
	void PlayImpactSound(const FVector& fireHitRes);
	void SpawnBeamParticle(const FVector& end);

	UFUNCTION(Server, Reliable)
	void ServerHitScanEffects(const FVector_NetQuantize& hitPosition, const EHitScanResult hitScanRes);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastHitScanEffects(const FVector_NetQuantize& hitPosition, const EHitScanResult hitScanRes);

	void HandleHitImpact
	(const EHitScanResult& hitScanRes, const FVector_NetQuantize& hitPosition);

	void HandleImpactEffects(const EHitScanResult& hitScanRes, const FVector_NetQuantize& hitPosition);

	void DetermineImpactEffects(const EHitScanResult& hitScanRes);

	

private:
	
	UPROPERTY(EditAnywhere, Category = "HitScanWeapon|WeaponAttributes")
	float minDamage{ 8.f };
	UPROPERTY(EditAnywhere, Category = "HitScanWeapon|WeaponAttributes")
	float headShotDamageMultiplier{ 1.f };
	UPROPERTY(EditAnywhere, Category = "HitScanWeapon|WeaponAttributes")
	float hitScanRange{ 8000.f };
	

	UPROPERTY(EditDefaultsOnly, Category = "HitScanWeapon|Effects")
	class UParticleSystem* beamParticles{};

	UPROPERTY(EditDefaultsOnly, Category = "HitScanWeapon|Effects")
	class UParticleSystem* normalImpactParticles{};
	UPROPERTY(EditDefaultsOnly, Category = "HitScanWeapon|Effects")
	class UParticleSystem* playerImpactParticles{};



	UPROPERTY(EditDefaultsOnly, Category = "HitScanWeapon|Effects")
	class USoundBase* normalImpactSound{};
	UPROPERTY(EditDefaultsOnly, Category = "HitScanWeapon|Effects")
	class USoundBase* playerImpactSound{};


	class UParticleSystem* impactParticles{};
	class USoundBase* impactSound{};
	

public:
	float CalculateBaseDamage(const float& distance) const;
	FORCEINLINE float GetHeadshotDamageMultiplier() const { return headShotDamageMultiplier; }

	virtual bool IsHitScan() override { return true; } 

};
