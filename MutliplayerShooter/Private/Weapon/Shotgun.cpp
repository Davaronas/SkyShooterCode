// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Shotgun.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Engine/SkeletalMesh.h"
#include "PlayerCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "PlayerComponents/LagCompensationComponent.h"

#include "DebugMacros.h"


// Server and sim proxy
void AShotgun::Fire(const FVector& shootFromPos, const FVector& targetPos)
{

	// Only call weapon ::fire 

	AWeapon::Fire(shootFromPos, targetPos);
	if (!HasAuthority()) { return; }
	TArray<FVector_NetQuantize> impactPositions{};
	TArray<EHitScanResult> scanResults{};
	TMap<APlayerCharacter*, float> damageToPlayers{};


	for (uint8 i{}; i < shotgunPellets; ++i)
	{
		APlayerCharacter* player{};
		float damage{};
		EHitScanResult scanRes{};

		impactPositions.Add(FireHitScanWeapon(shootFromPos, targetPos, scanRes, player, damage, false));
		scanResults.Add(scanRes);

		if (scanRes == EHitScanResult::EHRS_PlayerHit)
		{
			if (player)
			{
				if (!damageToPlayers.Contains(player))
				{
					damageToPlayers.Add(player, damage);
				}
				else
				{
					damageToPlayers[player] += damage;
				}
			}
		}
	}

	if (!bUseServerSideRewind || ownerPlayer->IsLocallyControlled())
	{
		if (!GetOwner()) { return; }
		AController* ownerController = GetOwner()->GetInstigatorController();

		for (const auto& damagedPlayer : damageToPlayers)
		{
			UGameplayStatics::ApplyDamage(
				damagedPlayer.Key, damagedPlayer.Value,
				ownerController, this, UDamageType::StaticClass());
		}
	}

	ServerHitScansEffects(impactPositions, scanResults);

	// Send server and all clients
}


// Aut proxy
void AShotgun::Fire(const FVector& shootFromPos, const FVector& targetPos,
	TArray<FFireData>& outFireDatas, TArray<FServerSideRewindData>& outSSRDatas)
{
	AWeapon::Fire(shootFromPos, targetPos);

	if (!GetWeaponMesh()) { return; }
	TryGetMuzzleFlashSocket();

	// Actual hit location is currently not used on shotguns
	FVector actualHitLoc{};
	for (uint8 i{}; i < outSSRDatas.Num(); ++i)
	{
		FireHitScanWeapon(shootFromPos, targetPos, outFireDatas[i], outSSRDatas[i], actualHitLoc);
	}
}

void AShotgun::ServerHitScansEffects_Implementation
(const TArray<FVector_NetQuantize>& hitPositions, const TArray<EHitScanResult>& hitScanResArr)
{
	MulticastHitScansEffects(hitPositions, hitScanResArr);
}

void AShotgun::MulticastHitScansEffects_Implementation
(const TArray<FVector_NetQuantize>& hitPositions, const TArray<EHitScanResult>& hitScanResArr)
{
	TArray<FServerSideRewindData> debatedHits{};

	for (uint8 i{}; i < hitPositions.Num(); ++i)
	{
		FServerSideRewindData debateRes{};
		HandleHitImpact(hitScanResArr[i], hitPositions[i]);

		
		if (!HasAuthority() && ownerPlayer && ownerPlayer->IsLocallyControlled())
		{
			debatedHits.Add(debateRes);
		}
	}

	if (HasAuthority() || (ownerPlayer && !ownerPlayer->IsLocallyControlled())) { return; }

	if (!debatedHits.IsEmpty())
	{
		if (!ownerPlayer) ownerPlayer = Cast<APlayerCharacter>(GetOwner());
		if (!ownerPlayer) { return; }

		ULagCompensationComponent* playerLagComp =
			ownerPlayer->GetLagCompensationComponent();

		if (playerLagComp)
		{
			playerLagComp->ServerHitRequests_HitScan(debatedHits, this);
		}
	}
}
