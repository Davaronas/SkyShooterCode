// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/HitScanWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Engine/SkeletalMesh.h"
#include "PlayerCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "PlayerComponents/LagCompensationComponent.h"
#include "Controller/MainPlayerController.h"
#include "Weapon/HeadshotDamageType.h"

#include "DebugMacros.h"

void AHitScanWeapon::Fire(const FVector& shootFromPos, const FVector& targetPos)
{
	Super::Fire(shootFromPos, targetPos);

	if (!HasAuthority()) { return; }

	if (!GetWeaponMesh()) { return; }
	TryGetMuzzleFlashSocket();
	FireHitScanWeapon(shootFromPos, targetPos);
}


// Aut proxy
void AHitScanWeapon::Fire(const FVector& shootFromPos, const FVector& targetPos,
	FFireData& outFireData, FServerSideRewindData& outSSRData, FVector& outActualHitLoc)
{
	Super::Fire(shootFromPos, targetPos);

	if (!GetWeaponMesh()) { return; }
	TryGetMuzzleFlashSocket();

	FireHitScanWeapon(shootFromPos, targetPos, outFireData, outSSRData, outActualHitLoc);
}

// Server and Sim Proxies
// Target position is sent to not locally controlled characters
// Server calculates his own scatter
FVector AHitScanWeapon::FireHitScanWeapon
(const FVector& shootFromPos, const FVector& targetPos, bool bSendEffectsServer)
{
	if (muzzleFlashSocket)
	{
		FVector start = shootFromPos;
		// bit longer so it's guaranteed to hit the target pos
		FVector end = start + (((targetPos - start).GetSafeNormal() * hitScanRange));

		// Sync scatter ?
		FVector scatteredEnd{end};
		if((ownerPlayer && ownerPlayer->IsLocallyControlled()) || !bUseServerSideRewind)
		scatteredEnd = start + AddScatterToTargetVector(start, end);


		FHitResult fireHitRes{};
		UWorld* world = GetWorld();

		if (!world) { return FVector(); }

		EHitScanResult hsRes{ EHitScanResult::EHSR_NoHit };
		FVector beamEnd{ scatteredEnd };

		FCollisionQueryParams queryParams{ NAME_None, false, ownerPlayer };
		if (world->LineTraceSingleByChannel(fireHitRes, start, beamEnd, ECollisionChannel::ECC_Visibility, queryParams))
		{
		/*	if (fireHitRes.GetActor())
			{
				GEPR_ONE_VARIABLE("Actor Hit On Server: %s", *fireHitRes.GetActor()->GetActorNameOrLabel());
			}*/

			HandleHitTarget(fireHitRes, beamEnd, hsRes);
		}

		if(bSendEffectsServer)
		ServerHitScanEffects(beamEnd, hsRes);

		return beamEnd;
	}

	return FVector();
}

// Auto Proxy
void AHitScanWeapon::FireHitScanWeapon
(const FVector& shootFromPos, const FVector& targetPos,
	FFireData& outFireData, FServerSideRewindData& outSSRData, FVector& outActualHitLoc)
{
	if (muzzleFlashSocket)
	{
		FVector start = shootFromPos;
		// bit longer so it's guaranteed to hit the target pos
		FVector end = start + ((targetPos - start).GetSafeNormal() * hitScanRange); 
		//.GetClampedToMaxSize(hitScanRange)); 

		//DRAW_LINE_COMP(ownerPlayer, shootFromPos, targetPos);
		const FVector scatteredEnd = start + AddScatterToTargetVector(start, end, outFireData);
		FHitResult fireHitRes{};
		UWorld* world = GetWorld();

		if (!world) { return; }
		FVector beamEnd{ scatteredEnd };
		FCollisionQueryParams queryParams{ NAME_None, false, ownerPlayer };
		if (world->LineTraceSingleByChannel(fireHitRes, start, beamEnd, ECollisionChannel::ECC_Visibility, queryParams))
		{
			APlayerCharacter* playerHit = Cast<APlayerCharacter>(fireHitRes.GetActor());
			if(playerHit)
			outSSRData.playerHit = playerHit;
			outActualHitLoc = fireHitRes.ImpactPoint;
			//outSSRData.hitLoc = fireHitRes.ImpactPoint;
			//DRAW_LINE_COMP_B(ownerPlayer, start, beamEnd);
		}
		else
		{
			outActualHitLoc = beamEnd;
		}

		// make it instant for the local player
		// it might look confusing if the beam particles lag behind
		// and may make the player think he has to lead the shots
		SpawnBeamParticle(scatteredEnd);
	}
}




// Shotgun
// Hit Locations are not synced on shotguns
FVector AHitScanWeapon::FireHitScanWeapon
(const FVector& shootFromPos, const FVector& targetPos,
	EHitScanResult& hitScanRes, APlayerCharacter*& outPlayerHit, float& actualDamage_, bool bSendServer)
{
	if (muzzleFlashSocket)
	{
		FVector start = shootFromPos;
		// bit longer so it's guaranteed to hit the target pos
		FVector end = start + (((targetPos - start).GetSafeNormal() * hitScanRange)); //.GetClampedToMaxSize(hitScanRange)); 
		const FVector scatteredEnd = start + AddScatterToTargetVector(start, end);

		FHitResult fireHitRes{};
		UWorld* world = GetWorld();

		if (!world) { return FVector(); }

		FVector beamEnd{ scatteredEnd };
		FCollisionQueryParams queryParams{ NAME_None, false, ownerPlayer };
		if (world->LineTraceSingleByChannel(fireHitRes, start, beamEnd, ECollisionChannel::ECC_Visibility, queryParams))
		{
			HandleHitTarget(fireHitRes, beamEnd, hitScanRes, outPlayerHit, actualDamage_, bSendServer);
		}

		if (bSendServer)
			ServerHitScanEffects(beamEnd, hitScanRes);

		return beamEnd;
	}

	return FVector();
}



void AHitScanWeapon::HandleHitTarget(FHitResult& fireHitRes,
	FVector& beamEnd, EHitScanResult& hitScanRes, APlayerCharacter*& outPlayerHit, float& actualDamage_, bool bSendServer)
{
	beamEnd = fireHitRes.ImpactPoint;
	APlayerCharacter* playerHit = Cast<APlayerCharacter>(fireHitRes.GetActor());
	outPlayerHit = playerHit;
	if (playerHit && HasAuthority())
	{
		if (!GetOwner()) { return; }
		if (playerHit == ownerPlayer) { return; }

		const bool bHeadshot = fireHitRes.BoneName == FString{ TEXT("head") };
		float actualDamage = CalculateBaseDamage(fireHitRes.Distance);
		actualDamage = bHeadshot ? actualDamage * headShotDamageMultiplier : actualDamage;
		actualDamage_ = actualDamage;

		UClass* damageType = bHeadshot ? UHeadshotDamageType::StaticClass() : UDamageType::StaticClass();


		if (bSendServer && (!bUseServerSideRewind || ownerPlayer->IsLocallyControlled()))
		{
			UGameplayStatics::ApplyDamage(
				playerHit, actualDamage,
				GetOwner()->GetInstigatorController(), this, damageType);
		}


		hitScanRes = EHitScanResult::EHRS_PlayerHit;
	}
	else
	{
		hitScanRes = EHitScanResult::EHRS_NonPlayerHit;
	}

}

void AHitScanWeapon::HandleHitTarget(FHitResult& fireHitRes, FVector& beamEnd, EHitScanResult& hitScanRes)
{
	beamEnd = fireHitRes.ImpactPoint;
	APlayerCharacter* playerHit = Cast<APlayerCharacter>(fireHitRes.GetActor());
	if (playerHit && HasAuthority())
	{
		if (!GetOwner()) { return; }
		if (playerHit == ownerPlayer) { return; }

		const bool bHeadshot = fireHitRes.BoneName == FString{ TEXT("head") };
		float actualDamage = CalculateBaseDamage(fireHitRes.Distance);
		
		actualDamage = bHeadshot ? actualDamage * headShotDamageMultiplier : actualDamage;

		UClass* damageType = bHeadshot ? UHeadshotDamageType::StaticClass() : UDamageType::StaticClass();

		if(!bUseServerSideRewind || ownerPlayer->IsLocallyControlled())
		UGameplayStatics::ApplyDamage(
			playerHit, actualDamage,
			GetOwner()->GetInstigatorController(), this, damageType);

		hitScanRes = EHitScanResult::EHRS_PlayerHit;
	}
	else
	{
		hitScanRes = EHitScanResult::EHRS_NonPlayerHit;
	}

}


void AHitScanWeapon::ServerHitScanEffects_Implementation
(const FVector_NetQuantize& hitPosition, const EHitScanResult hitScanRes)
{
	MulticastHitScanEffects(hitPosition, hitScanRes);
}



void AHitScanWeapon::MulticastHitScanEffects_Implementation
(const FVector_NetQuantize& hitPosition, const EHitScanResult hitScanRes)
{
	HandleHitImpact(hitScanRes, hitPosition);
}

void AHitScanWeapon::HandleHitImpact
(const EHitScanResult& hitScanRes, const FVector_NetQuantize& hitPosition)
{

	HandleImpactEffects(hitScanRes, hitPosition);
}








void AHitScanWeapon::HandleImpactEffects(const EHitScanResult& hitScanRes, const FVector_NetQuantize& hitPosition)
{
	DetermineImpactEffects(hitScanRes);
	SpawnImpactParticles(hitPosition);
	PlayImpactSound(hitPosition);

	if (!IsOwnerAutonomousProxy())
	{
		SpawnBeamParticle(hitPosition);
	}
}


void AHitScanWeapon::DetermineImpactEffects(const EHitScanResult& hitScanRes)
{
	if (hitScanRes == EHitScanResult::EHRS_PlayerHit)
	{
		impactParticles = playerImpactParticles;
		impactSound = playerImpactSound;
	}
	else if (hitScanRes == EHitScanResult::EHRS_NonPlayerHit)
	{
		impactParticles = normalImpactParticles;
		impactSound = normalImpactSound;
	}
	else if (hitScanRes == EHitScanResult::EHSR_NoHit)
	{
		impactParticles = nullptr;
		impactSound = nullptr;
	}
}



void AHitScanWeapon::SpawnImpactParticles(const FVector& fireHitRes)
{
	UWorld* world = GetWorld();
	if (!world) { return; }

	if (impactParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation
		(world, impactParticles, fireHitRes, fireHitRes.Rotation());
	}
}

void AHitScanWeapon::PlayImpactSound(const FVector& fireHitRes)
{
	if (impactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, impactSound, fireHitRes);
	}
}

void AHitScanWeapon::SpawnBeamParticle(const FVector& end)
{
	UWorld* world = GetWorld();
	if (!world) { return; }

	if (beamParticles)
	{
		UParticleSystemComponent* beamParticlesComponent = UGameplayStatics::SpawnEmitterAtLocation(
			world, beamParticles, muzzleFlashSocketTransform, true);

		if (beamParticlesComponent)
		{
			beamParticlesComponent->SetVectorParameter(FName{ TEXT("Target") }, end);
		}
	}
}



float AHitScanWeapon::CalculateBaseDamage(const float& distance) const
{
	//GEPR_ONE_VARIABLE("Distance: %.2f", distance);
	
	float damage_ = FMath::Clamp
	((1 - (distance / hitScanRange)) * baseDamage, minDamage, baseDamage);

	//GEPR_ONE_VARIABLE("Damage: %.2f", damage_);

	return damage_;
}






//APlayerCharacter* AHitScanWeapon::HandleHitTarget(FHitResult& fireHitRes, FVector& beamEnd, EHitScanResult& hitScanRes, float& actualDamage, bool bSendDamageServer)
//{
//	beamEnd = fireHitRes.ImpactPoint;
//	APlayerCharacter* player = Cast<APlayerCharacter>(fireHitRes.GetActor());
//	if (player)
//	{
//		if (HasAuthority())
//		{
//			if (!GetOwner()) { return nullptr; }
//
//			actualDamage = FMath::Clamp
//			((1 - (fireHitRes.Distance / hitScanRange)) * baseDamage, minDamage, baseDamage);
//
//			if(bSendDamageServer)
//			UGameplayStatics::ApplyDamage(
//				player, actualDamage,
//				GetOwner()->GetInstigatorController(), this, UDamageType::StaticClass());
//		}
//		hitScanRes = EHitScanResult::EHRS_PlayerHit;
//	}
//	else
//	{
//		hitScanRes = EHitScanResult::EHRS_NonPlayerHit;
//	}
//
//	return player;
//}





//void AHitScanWeapon::CheckServerDisagree(const EHitScanResult& hitScanRes, const FVector_NetQuantize& hitPosition)
//{
//	if (bUseServerSideRewind && hitScanRes != EHitScanResult::EHRS_PlayerHit &&
//		ownerPlayer && !ownerPlayer->HasAuthority() && ownerPlayer->IsLocallyControlled())
//	{
//		if (UWorld* world = GetWorld())
//		{
//			FVector start = muzzleFlashSocketTransform.GetLocation();
//			FVector end = start + ((hitPosition - start).GetSafeNormal() * hitScanRange);
//
//			
//
//			FHitResult clientSideCheckHitResult{};
//			if (world->LineTraceSingleByChannel
//			(clientSideCheckHitResult, start,
//				end, ECollisionChannel::ECC_Visibility))
//			{
//				
//
//				if (clientSideCheckHitResult.GetActor() &&
//					clientSideCheckHitResult.GetActor() != ownerPlayer)
//				{
//					APlayerCharacter* hitPlayerCheck = 
//						Cast<APlayerCharacter>(clientSideCheckHitResult.GetActor());
//
//
//					// we hit a player on our screen, but not on the server side
//					// check server side rewind position
//					if (hitPlayerCheck)
//					{
//						AMainPlayerController* ownerPlayerController =
//							ownerPlayer->GetController<AMainPlayerController>();
//						ULagCompensationComponent* playerLagComp =
//							ownerPlayer->GetLagCompensationComponent();
//
//						if (ownerPlayerController && playerLagComp)
//						{
//							FServerSideRewindData hitDebate{};
//							hitDebate.debateResult = EClientSideHitDebate::ECSHD_DebateServerResult;
//							hitDebate.playerHit = hitPlayerCheck;
//							hitDebate.startLoc = start;
//							hitDebate.hitLoc = clientSideCheckHitResult.ImpactPoint;
//							hitDebate.timeOfHit = ownerPlayerController->GetServerTime();
//							hitDebate.singleTripTime = ownerPlayerController->GetSingleTripTime();
//
//							playerLagComp->ServerHitRequest_HitScan
//							(hitDebate, this);
//						}
//					}
//				}
//
//			}
//		}
//	}
//}

//void AHitScanWeapon::CheckServerDisagree(const EHitScanResult& hitScanRes, const FVector_NetQuantize& hitPosition,
//	FServerSideRewindData& outDebate)
//{
//	if (bUseServerSideRewind && hitScanRes != EHitScanResult::EHRS_PlayerHit &&
//		ownerPlayer && !ownerPlayer->HasAuthority() && ownerPlayer->IsLocallyControlled())
//	{
//		if (UWorld* world = GetWorld())
//		{
//			FVector start = muzzleFlashSocketTransform.GetLocation();
//			FVector end = start + ((hitPosition - start).GetSafeNormal() * hitScanRange);
//
//
//			FHitResult clientSideCheckHitResult{};
//			if (world->LineTraceSingleByChannel
//			(clientSideCheckHitResult, start,
//				end, ECollisionChannel::ECC_Visibility))
//			{
//				if (clientSideCheckHitResult.GetActor() &&
//					clientSideCheckHitResult.GetActor() != ownerPlayer)
//				{
//					APlayerCharacter* hitPlayerCheck =
//						Cast<APlayerCharacter>(clientSideCheckHitResult.GetActor());
//
//					
//
//					// we hit a player on our screen, but not on the server side
//					// check server side rewind position
//
//					if (hitPlayerCheck)
//					{
//						AMainPlayerController* ownerPlayerController =
//							ownerPlayer->GetController<AMainPlayerController>();
//
//
//
//						if (ownerPlayerController)
//						{
//							outDebate.debateResult = EClientSideHitDebate::ECSHD_DebateServerResult;
//							outDebate.playerHit = hitPlayerCheck;
//							outDebate.timeOfHit = ownerPlayerController->GetServerTime();
//							outDebate.singleTripTime = ownerPlayerController->GetSingleTripTime();
//							outDebate.startLoc = start;
//							outDebate.hitLoc = clientSideCheckHitResult.ImpactPoint;
//
//							DRAW_POINT_B_SMALL(outDebate.hitLoc);
//						}
//					}
//					else
//					{
//						outDebate.debateResult = EClientSideHitDebate::ECSHD_AcceptServerResult;
//					}
//				}
//
//			}
//		}
//	}
//}

