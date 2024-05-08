// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerComponents/CombatComponent.h"
#include "PlayerCharacter.h"
#include "Weapon/Weapon.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

#include "Controller/MainPlayerController.h"
#include "Engine/GameViewportClient.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Camera/CameraComponent.h"
#include "TimerManager.h"
#include "Weapon/Projectile.h"
#include "Weapon/ProjectileWeapon.h"
#include "PlayerComponents/BuffComponent.h"
#include "HelperTypes/Teams.h"
#include "Weapon/HitScanWeapon.h"
#include "Weapon/Shotgun.h"
#include "Weapon/ProjectileWeapon.h"

#include "PlayerComponents/LagCompensationComponent.h"



#include "DebugMacros.h"
#include "DrawDebugHelpers.h"

#include "Kismet/KismetSystemLibrary.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	SetIsReplicatedByDefault(true);
}


void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();


	normalWalkSpeed = baseNormalWalkSpeed;
	aimWalkSpeed = baseAimWalkSpeed;
	crouchWalkSpeed = baseCrouchWalkSpeed;

	SetNormalWalkSpeed();


	if (!ownerPlayer) ownerPlayer = Cast<APlayerCharacter>(GetOwner());
	if (ownerPlayer)
	{
		playerController = Cast<AMainPlayerController>(ownerPlayer->GetController());
		

		if (ownerPlayer->GetPlayerCamera())
		{
			defaultFOV = ownerPlayer->GetPlayerCamera()->FieldOfView;
			currentFOV = defaultFOV;
		}

		if (ownerPlayer->HasAuthority())
		{
			InitializeCarriedAmmoTypes();
		}
	}

	if (playerController)
		playerHUD = Cast<AMainHUD>(playerController->GetHUD());

	

	//if (!playerController) { GEPRS_R("Player Controller Null"); }
	//if (!playerHUD) { GEPRS_R("HUD Null"); }


}

void UCombatComponent::InitializeCarriedAmmoTypes()
{
	carriedAmmoMap.Emplace(EWeaponType::EWT_AssaultRifle, default_AssaultRifeAmmo);
	carriedAmmoMap.Emplace(EWeaponType::EWT_RocketLauncher, default_RocketLauncherAmmo);
	carriedAmmoMap.Emplace(EWeaponType::EWT_Pistol, default_PistolAmmo);
	carriedAmmoMap.Emplace(EWeaponType::EWT_SubmachineGun, default_SMGAmmo);
	carriedAmmoMap.Emplace(EWeaponType::EWT_Shotgun, default_ShotgunAmmo);
	carriedAmmoMap.Emplace(EWeaponType::EWT_SniperRifle, default_SniperRifleAmmo);
	carriedAmmoMap.Emplace(EWeaponType::EWT_GrenadeLauncher, default_GrenadeLauncherAmmo);
	currentHandGrenades = default_HandGrenades;

#if WITH_EDITOR

	if (bDEBUG_InfiniteAmmo)
	{
		carriedAmmoMap[EWeaponType::EWT_AssaultRifle] += 9999;
		carriedAmmoMap[EWeaponType::EWT_RocketLauncher] += 9999;
		carriedAmmoMap[EWeaponType::EWT_Pistol] += 9999;
		carriedAmmoMap[EWeaponType::EWT_SubmachineGun] += 9999;
		carriedAmmoMap[EWeaponType::EWT_Shotgun] += 9999;
		carriedAmmoMap[EWeaponType::EWT_SniperRifle] += 9999;
		carriedAmmoMap[EWeaponType::EWT_GrenadeLauncher] += 9999;
		currentHandGrenades = 99;
	}
#endif
}


void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, equippedWeapon);
	DOREPLIFETIME(UCombatComponent, secondaryWeapon);
	DOREPLIFETIME(UCombatComponent, bIsAiming);
	// Other Clients need to know about this because shotgun will only ever load one shell
	// because they think the proxy has zero carried ammo
	DOREPLIFETIME(UCombatComponent, carriedAmmo) // COND_OwnerOnly);
	DOREPLIFETIME(UCombatComponent, combatState);
	DOREPLIFETIME_CONDITION(UCombatComponent, currentHandGrenades, COND_OwnerOnly);

	DOREPLIFETIME(UCombatComponent, bCanJump);
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);


	// Server needs this for actual inaccuracy, also for client crosshair
	CalculateInaccuracy(DeltaTime);

	if (ownerPlayer && ownerPlayer->IsLocallyControlled())
	{
		SetHUDCrosshair(DeltaTime);
		TraceUnderCrosshairs(tickTraceHitResult);
		SetCrosshairPosition();
		InterpFOV(DeltaTime);
	}


	// Stop Jump spam server side
	HandleJumpDelay(DeltaTime);
}

void UCombatComponent::HandleJumpDelay(float DeltaTime)
{
	if (ownerPlayer && playerMovementComp)
	{
		if (ownerPlayer->HasAuthority())
		{
			if (bCanJump && playerMovementComp->IsFalling())
			{
				bCanJump = false;
				timeBetweenJumpsRunningTime = 0;
			}

			if (!bCanJump && !playerMovementComp->IsFalling())
			{
				timeBetweenJumpsRunningTime += DeltaTime;
				//GEPR_ONE_VARIABLE("Time between jumps: %.2f", timeBetweenJumpsRunningTime);

				if (ownerPlayer->IsLocallyControlled())
				{
					bCanBufferJump = true;
				}
			}
			else
			{
				if (ownerPlayer->IsLocallyControlled())
				{
					bCanBufferJump = false;
				}
			}

			if (timeBetweenJumpsRunningTime >= timeBetweenJumps)
			{
				timeBetweenJumpsRunningTime = 0;
				bCanJump = true;
			}
		}
		else
		{
			if (!bCanJump && !playerMovementComp->IsFalling())
			{
				bCanBufferJump = true;
			}
			else
			{
				bCanBufferJump = false;
			}
		}


	}
}

void UCombatComponent::SetCrosshairPosition()
{
	if (!equippedWeapon) { return; }
	if (!equippedWeapon->GetWeaponMesh()) { return; }
	if (!ownerPlayer) { return; }
	UWorld* world = GetWorld();
	if (!world) { return; }


	FTransform barrelTransform = equippedWeapon->GetWeaponMesh()->
		GetSocketTransform(MUZZLE_FLASH_SOCKET, ERelativeTransformSpace::RTS_World);


	FVector hitPoint{};
	FHitResult barrelViewHitResult{};
	TArray<AActor*> actorsToIgnore{ equippedWeapon };
	FRotator orientation{ (tickTraceHitResult.ImpactPoint - barrelTransform.GetLocation()).Rotation() };
	if (UKismetSystemLibrary::BoxTraceSingle
	(this, barrelTransform.GetLocation(),
		tickTraceHitResult.ImpactPoint,
		FVector{ crosshairTraceCheckBoxSize }, orientation,
		ETraceTypeQuery::TraceTypeQuery1,
		false, actorsToIgnore,
		bDrawDebugCrosshairTrace ? EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None,
		barrelViewHitResult, true, FLinearColor::Red))
	{
		hitPoint = barrelViewHitResult.ImpactPoint;
	}
	else
	{
		hitPoint = tickTraceHitResult.ImpactPoint;
	}


		
	/*if (world->LineTraceSingleByChannel
	(barrelViewHitResult, barrelTransform.GetLocation(),
		tickTraceHitResult.ImpactPoint, ECollisionChannel::ECC_Visibility))
	{
		hitPoint = barrelViewHitResult.ImpactPoint;
	}
	else
	{
		hitPoint = tickTraceHitResult.ImpactPoint;
	}*/

	FVector2D screenPos{};
	UGameplayStatics::ProjectWorldToScreen(playerController, hitPoint, screenPos);
	ownerPlayer->SetCrosshairDrawLocation(screenPos);
}

void UCombatComponent::SetHUDCrosshair(float DeltaTime)
{
	if (!ownerPlayer) { return; }

	

	if (!playerController) playerController = Cast<AMainPlayerController>(ownerPlayer->GetController());
	if (!playerController) { return; }
	if (!playerHUD) playerHUD = Cast<AMainHUD>(playerController->GetHUD());
	if (!playerHUD) { return; }

	
	if (equippedWeapon && bShowCrosshair)
	{
		crosshairPackage.crosshairCenter = equippedWeapon->crosshairCenter;
		crosshairPackage.crosshairLeft = equippedWeapon->crosshairLeft;
		crosshairPackage.crosshairRight = equippedWeapon->crosshairRight;
		crosshairPackage.crosshairTop = equippedWeapon->crosshairTop;
		crosshairPackage.crosshairBottom = equippedWeapon->crosshairBottom;
	}
	else
	{
		crosshairPackage.crosshairCenter = nullptr;
		crosshairPackage.crosshairLeft = nullptr;
		crosshairPackage.crosshairRight = nullptr;
		crosshairPackage.crosshairTop = nullptr;
		crosshairPackage.crosshairBottom = nullptr;
	}

	//Calculate spread
	


	crosshairPackage.spread = inaccuracy;
	playerHUD->SetCrosshairPackage(crosshairPackage);
}

void UCombatComponent::CalculateInaccuracy(float DeltaTime)
{
	if (playerMovementComp)
	{
		inaccuracy = (playerMovementComp->Velocity.Size() / normalWalkSpeed) * movementInaccuracyMultiplier;
		inaccuracy += baseInaccuracy;
		inaccuracy += shootingInaccuracy;

		if (bIsAiming) inaccuracy -= aimingInaccuracyDecrease;

		if (playerMovementComp)
		{
			if (playerMovementComp->IsFalling()) inaccuracy += inAirInaccuracyIncrease;
			if (playerMovementComp->IsCrouching()) inaccuracy -= crouchingInaccuracyDecrease;
		}

		if (equippedWeapon) inaccuracy += equippedWeapon->GetBaseInaccuracyIncrease();

		shootingInaccuracy = FMath::FInterpTo(shootingInaccuracy, 0.f, DeltaTime, shootingInaccuracyDecreaseInterpSpeed);
	}
}

float UCombatComponent::GetCalculateInaccuracy()
{
	if (playerMovementComp)
	{
		float inaccuracy_{0.f};
		inaccuracy_ = (playerMovementComp->Velocity.Size() / normalWalkSpeed) * movementInaccuracyMultiplier;
		inaccuracy_ += baseInaccuracy;
		inaccuracy_ += shootingInaccuracy;

		if (bIsAiming) inaccuracy_ -= aimingInaccuracyDecrease;

		if (playerMovementComp)
		{
			if (playerMovementComp->IsFalling()) inaccuracy_ += inAirInaccuracyIncrease;
			if (playerMovementComp->IsCrouching()) inaccuracy_ -= crouchingInaccuracyDecrease;
		}

		if (equippedWeapon) inaccuracy_ += equippedWeapon->GetBaseInaccuracyIncrease();

		return FMath::Clamp(inaccuracy_, 0.f, 20.f);
	}

	return 1000.f;
}


void UCombatComponent::InterpFOV(float DeltaTime)
{
	if (!equippedWeapon) { return; }

	if (bIsAiming)
	{
		const float w_zoomedFOV = equippedWeapon->GetZoomedFOV();
		const float w_zoomInterpSpeed = equippedWeapon->GetZoomInterpSpeed();
		currentFOV = FMath::FInterpTo(currentFOV, w_zoomedFOV, DeltaTime, w_zoomInterpSpeed);
	}
	else
	{
		currentFOV = FMath::FInterpTo(currentFOV, defaultFOV, DeltaTime, zoomInterpSpeed);
	}

	if (ownerPlayer && ownerPlayer->GetPlayerCamera() && !ownerPlayer->IsGameplayDisabled())
	{
		ownerPlayer->GetPlayerCamera()->SetFieldOfView(currentFOV);
	}
}

void UCombatComponent::DisableGameplay()
{
	if (ownerPlayer && ownerPlayer->GetPlayerCamera())
	{
		ownerPlayer->GetPlayerCamera()->SetFieldOfView(defaultFOV);
	}

	bShowCrosshair = false;
	bFireButtonPressed = false;
}

void UCombatComponent::Pickup_Ammo(const EWeaponType& ammoType, const int32& ammoAmount)
{
	if (!ownerPlayer) ownerPlayer = Cast<APlayerCharacter>(GetOwner());
	if (ownerPlayer && ownerPlayer->HasAuthority())
	{
		if (carriedAmmoMap.Contains(ammoType))
		{
			carriedAmmoMap[ammoType] += ammoAmount;
		}
		else
		{
			carriedAmmoMap.Emplace(ammoType, ammoAmount);
		}

		if (equippedWeapon && equippedWeapon->GetWeaponType() == ammoType)
		{
			carriedAmmo = carriedAmmoMap[ammoType];
			HUD_UpdateCarriedAmmo();

			if(IsUnoccupied())
			ReloadIfEmpty();
		}
	}
}

void UCombatComponent::Pickup_Grenade(const int32& grenadeAmount)
{
	if (!ownerPlayer) ownerPlayer = Cast<APlayerCharacter>(GetOwner());
	if (ownerPlayer && ownerPlayer->HasAuthority())
	{
		currentHandGrenades = FMath::Clamp(currentHandGrenades + grenadeAmount, 0, 999);
		HUD_UpdateGrenades();
	}

}

void UCombatComponent::IncreaseSpeed(float amount)
{
	if (!ownerPlayer) ownerPlayer = Cast<APlayerCharacter>(GetOwner());
	if (ownerPlayer && ownerPlayer->HasAuthority())
	{
		MulticastIncreaseSpeed(amount);
	}
}


void UCombatComponent::ResetSpeed()
{
	if (!ownerPlayer) ownerPlayer = Cast<APlayerCharacter>(GetOwner());
	if (ownerPlayer && ownerPlayer->HasAuthority())
	{
		MulticastResetSpeed();
	}
}

void UCombatComponent::MulticastIncreaseSpeed_Implementation(float amount)
{
	if (!playerMovementComp) { return; }

	// JUMP Z !

	normalWalkSpeed = baseNormalWalkSpeed + amount;
	playerMovementComp->MaxWalkSpeed = normalWalkSpeed;

	aimWalkSpeed = baseAimWalkSpeed + amount;

	crouchWalkSpeed = baseCrouchWalkSpeed + amount;
	playerMovementComp->MaxWalkSpeedCrouched = crouchWalkSpeed;

}

void UCombatComponent::MulticastResetSpeed_Implementation()
{
	if (!playerMovementComp) { return; }

	playerMovementComp->MaxWalkSpeed = baseNormalWalkSpeed;
	normalWalkSpeed = baseNormalWalkSpeed;

	aimWalkSpeed = baseAimWalkSpeed;

	playerMovementComp->MaxWalkSpeedCrouched = baseCrouchWalkSpeed;
	crouchWalkSpeed = baseCrouchWalkSpeed;
}

void UCombatComponent::FireButtonPressed(bool state)
{
	bFireButtonPressed = state;
	TryFire();
}

void UCombatComponent::TryFire()
{
	if (!ownerPlayer) ownerPlayer = Cast<APlayerCharacter>(GetOwner());
	if (!ownerPlayer) { return; }
	if (!equippedWeapon) { return; }
	UWorld* world = GetWorld();
	if (!world) { return; }
	if (!playerController) { playerController = ownerPlayer->GetController<AMainPlayerController>(); }
	if (!playerController) { return; }

	if (bFireButtonPressed)
	{
		if (CanFire())
		{
			if (IsPlayerAutonomousProxy())
			{
				shootingInaccuracy += equippedWeapon->GetOneShotInaccuracyIncrease();

				if (equippedWeapon->UsesServerSideRewind())
				{
					if (equippedWeapon->IsHitScan())
					{
					
						if (!equippedWeapon->IsShotgun())
						{
							FFireData fireData{};
							FServerSideRewindData rewindData{};
							rewindData.startLoc = equippedWeapon->GetMuzzleFlashLocation();
							rewindData.hitLoc = tickTraceHitResult.ImpactPoint;
							rewindData.timeOfHit = ownerPlayer->GetServerTime() + ownerPlayer->GetSingleTripTime();
							FVector actualHitLocation{};
							LocalFire(fireData, rewindData, actualHitLocation);

							if (rewindData.playerHit)
							{
								ServerRecreateHitScanShot(rewindData, fireData,
									Cast<AHitScanWeapon>(equippedWeapon), GetWorld()->GetTimeSeconds());
							}
							else
							{
								ServerFire(rewindData.startLoc, actualHitLocation,  //rewindData.hitLoc,
									equippedWeapon, world->GetTimeSeconds());
							}
						}
						// I'm not touching the shotgun for hit location syncing, it's probably not very noticable
						else
						{
							// set start loc  to muzzle flash to all rewind dats, and hit lock to tick trace
							// and time of hit to all as well

							AShotgun* wepAsShotgun = Cast<AShotgun>(equippedWeapon);
							if (!wepAsShotgun) { return; }
							const uint8 pellets = wepAsShotgun->GetPelletCount();

							TArray<FFireData> fireDatas{};
							TArray<FServerSideRewindData> rewindDatas{};

							const FVector muzzleLoc = equippedWeapon->GetMuzzleFlashLocation();
							const FVector calculatePoint = tickTraceHitResult.ImpactPoint;
							const float timeOfHit = ownerPlayer->GetServerTime() + ownerPlayer->GetSingleTripTime();

							for (uint8 i{}; i < pellets; ++i)
							{
								FServerSideRewindData rewd{};
								rewd.startLoc = muzzleLoc;
								rewd.hitLoc = calculatePoint;
								rewd.timeOfHit = timeOfHit;
								rewindDatas.Add(rewd);

								FFireData fireData{};
								fireDatas.Add(fireData);
							}






							// this checks if shot hit a player, and fills fire data
							// because it will be used to recreate the shot in time
							LocalShotgunFire(fireDatas, rewindDatas);




							ServerRecreateShotgunHitScanShots(rewindDatas, fireDatas,
								Cast<AShotgun>(equippedWeapon), world->GetTimeSeconds());
						}
					}
					else // projectile weapon
					{
						FVector actualHitLoc{};
						LocalFire(actualHitLoc);
						ServerFire(equippedWeapon->GetMuzzleFlashLocation(), actualHitLoc /*tickTraceHitResult.ImpactPoint*/,
							equippedWeapon, world->GetTimeSeconds());
					}
				}
				else // Rocket Launcher and Grenade Launcher does not use SSR
				{ 
					FireEffect(equippedWeapon->GetMuzzleFlashLocation(), tickTraceHitResult.ImpactPoint);
					ServerFire(equippedWeapon->GetMuzzleFlashLocation(), tickTraceHitResult.ImpactPoint,
						equippedWeapon, world->GetTimeSeconds());
				}
			}
			else
			{
				ServerFire(equippedWeapon->GetMuzzleFlashLocation(), tickTraceHitResult.ImpactPoint,
					equippedWeapon, world->GetTimeSeconds());
			}
			
			bCanFire = false;

			/*
			* CHEATING TEST
			* 
			* if(IsPlayerAutonomousProxy())
				bCanFire = true;
			* 
			* 
			*/

			

			StartFireTimer();

			
		}
		else if(equippedWeapon && !equippedWeapon->HasCurrentAmmo())
		{
			ReloadButtonPressed();
		}
	}
}

void UCombatComponent::ServerRecreateHitScanShot_Implementation
(const FServerSideRewindData& rewindData, const FFireData& fireData,
	AHitScanWeapon* hitScanWeapon, const float& currentClientTime)
{
	if (!lagComp) { return; }
	if (!hitScanWeapon) { return; }
	if (!rewindData.playerHit) { return; }

	FFrameAccuracy timeOfHitAccuracyFrame = lagComp->GetFrameToCheck_Accuracy(rewindData.timeOfHit);
	FVector scatteredEnd = hitScanWeapon->AddScatterToTargetVector
		(rewindData.startLoc, rewindData.hitLoc, timeOfHitAccuracyFrame.accuracy, fireData);

	

	FServerSideRewindData scatteredShotData{rewindData};
	scatteredShotData.hitLoc = scatteredShotData.startLoc + scatteredEnd;
	
	lagComp->ServerHitRequest_HitScan(scatteredShotData, hitScanWeapon);

	// replace second arg with scattered shot data, once locally controlled fire data is override on weapon fire
	ServerFire(rewindData.startLoc, scatteredShotData.hitLoc /*rewindData.hitLoc*/, hitScanWeapon, currentClientTime);
}

void UCombatComponent::RequestCombatComponentToRecreateProjectileShot
(const FServerSideProjectileRewindData& projectileData, const FFireData& fireData,
	class AProjectileWeapon* projectileWeapon)
{
	ServerRecreateProjectileShot(projectileData, fireData, projectileWeapon);
}



void UCombatComponent::ServerRecreateProjectileShot_Implementation
(const FServerSideProjectileRewindData& projectileData,
	const FFireData& fireData, AProjectileWeapon* projectileWeapon)
{
	if (!lagComp) { return; }
	if (!projectileWeapon) { return; }
	if (!projectileData.playerHit) { return; }


	

	//DRAW_LINE_COMP(ownerPlayer, projectileData.startLoc, projectileData.hitLoc);

	FFrameAccuracy timeOfFiringAccuracyFrame = lagComp->GetFrameToCheck_Accuracy(projectileData.timeOfFiring);
	//GEPR_ONE_VARIABLE("Time of firing accuracy: %.2f", timeOfFiringAccuracyFrame.accuracy);

	FVector scatteredEnd = projectileWeapon->AddScatterToTargetVector
	(projectileData.startLoc, projectileData.hitLoc, timeOfFiringAccuracyFrame.accuracy, fireData);

	//DRAW_LINE_COMP_O(ownerPlayer, projectileData.startLoc, scatteredEnd);



	FServerSideRewindData scatteredShotData{ };
	scatteredShotData.playerHit = projectileData.playerHit;
	scatteredShotData.startLoc = projectileData.startLoc;
	scatteredShotData.timeOfHit = projectileData.timeOfHit;

	scatteredShotData.hitLoc = scatteredShotData.startLoc + scatteredEnd;

	// If we want gravity for projectile weapons, 
	// we will need projectile predict path, since
	// we're not at the moment, hit scan server rewind works fine

	lagComp->ServerHitRequest_Projectile(scatteredShotData, projectileWeapon);

}

void UCombatComponent::LocalFire(FVector& outActualHitLoc)
{
	if (!ownerPlayer) ownerPlayer = Cast<APlayerCharacter>(GetOwner());
	if (!ownerPlayer) { return; }
	if (!equippedWeapon) { return; }

	AProjectileWeapon* wepAsProjWep = Cast<AProjectileWeapon>(equippedWeapon);
	if (!wepAsProjWep) { return; }

	ownerPlayer->PlayFireMontage(bIsAimingLocally);



	wepAsProjWep->AutProxyFire(equippedWeapon->
		GetMuzzleFlashLocation(), tickTraceHitResult.ImpactPoint, outActualHitLoc);
}

void UCombatComponent::LocalFire(FFireData& outFireData, FServerSideRewindData& outSSRData, FVector& outActualHitLoc)
{
	if (!ownerPlayer) ownerPlayer = Cast<APlayerCharacter>(GetOwner());
	if (!ownerPlayer) { return; }
	if (!equippedWeapon) { return; }

	ownerPlayer->PlayFireMontage(bIsAimingLocally);
	
	equippedWeapon->Fire(equippedWeapon->
		GetMuzzleFlashLocation(), tickTraceHitResult.ImpactPoint, outFireData, outSSRData, outActualHitLoc);
}

bool UCombatComponent::ServerRecreateShotgunHitScanShots_Validate
(const TArray<FServerSideRewindData>& rewindDatas,
	const TArray<FFireData>& fireDatas, AShotgun* shotgun, const float& currentClientTime)
{
	if (!shotgun) { return true; }

	if (fireDatas.Num() > shotgun->GetPelletCount() || rewindDatas.Num() > shotgun->GetPelletCount()) { return false; }

	return true;
}

void UCombatComponent::ServerRecreateShotgunHitScanShots_Implementation
(const TArray<FServerSideRewindData>& rewindDatas,
	const TArray<FFireData>& fireDatas, AShotgun* shotgun, const float& currentClientTime)
{
	if (!lagComp) { return; }
	if (!shotgun) { return; }
	if (rewindDatas.Num() <= 0 || fireDatas.Num() <= 0 || rewindDatas.Num() != fireDatas.Num()) { return; }


	const float timeOfHit{ rewindDatas[0].timeOfHit };
	const FVector_NetQuantize startPos{ rewindDatas[0].startLoc };

	TArray<FServerSideRewindData> pelletsThatHitPlayer{};
	FFrameAccuracy timeOfHitAccuracyFrame = lagComp->GetFrameToCheck_Accuracy(timeOfHit);
	uint8 index{ 0 };
	for (auto& rewindData : rewindDatas)
	{
		if (rewindData.playerHit)
		{
			FServerSideRewindData thisPellet{rewindData};
			FVector scatteredEnd = shotgun->AddScatterToTargetVector
			(rewindData.startLoc, rewindData.hitLoc, timeOfHitAccuracyFrame.accuracy, fireDatas[index]);
			thisPellet.hitLoc = thisPellet.startLoc + scatteredEnd;
			pelletsThatHitPlayer.Add(thisPellet);
		}

		++index;
	}

	if (!pelletsThatHitPlayer.IsEmpty())
	{
		lagComp->ServerHitRequests_HitScan(pelletsThatHitPlayer, shotgun);
	}

	// temporary
	ServerFire(rewindDatas[0].startLoc, rewindDatas[0].hitLoc, shotgun, currentClientTime);

	// Multiple location Multicast fire

	// It locally controlled multicastfire call weapon:: fire with FireData override
	// to sync hit locations
	// currently everyone has random scatter on shots, but aut proxy hits are calculated accurately
}


void UCombatComponent::LocalShotgunFire(TArray<FFireData>& outFireDatas, TArray<FServerSideRewindData>& outSSRDatas)
{
	if (!ownerPlayer) ownerPlayer = Cast<APlayerCharacter>(GetOwner());
	if (!ownerPlayer) { return; }
	if (!equippedWeapon) { return; }

	ownerPlayer->PlayFireMontage(bIsAimingLocally);

	equippedWeapon->Fire(equippedWeapon->
		GetMuzzleFlashLocation(), tickTraceHitResult.ImpactPoint, outFireDatas, outSSRDatas);
}


bool UCombatComponent::CanFire()
{
	return (equippedWeapon && equippedWeapon->HasCurrentAmmo() &&
		bCanFire && (IsUnoccupied() || IsReloadingShotgun()) &&
		ownerPlayer && !bIsLocallySwappingWeapons && (!bIsLocallyReloading || HasShotgunEquipped())
		&& !bDisallowFireWeaponTooCloseToObject);
}



void UCombatComponent::ServerFire_Implementation
(const FVector_NetQuantize& shootFromPos, const FVector_NetQuantize& hitTarget,
	 AWeapon* firedWeapon, /*const float& clientTimeBetweenShots,*/ const float& currentClientTime)
{
	if (!ownerPlayer || (firedWeapon && !firedWeapon->HasCurrentAmmo())
		|| (!IsUnoccupied() && !IsReloadingShotgun())) { return; }

	firedWeapon->SetLastClientPacketTime(currentClientTime);

	/*
	* HITSCAN
	*/
	// if fire weapon uses server rewind AND hitscan weapon
	// do server side rewind
	// do NOT server rewind later after firing, and do not deal damage 
	// if hit on server
	// hitscan server side rewind do not need single trip time added anymore

	// if client, do server side rewind, hit scan, shotguns as well

	/*
	* PROJECTILE
	*/
	// projectiles will be spawned locally on clients, and if they hit something with the projectile
	// call server side rewind, this does not change, except the spawn time

	MulticastFire(shootFromPos, hitTarget);
}


// VALIDATE
bool UCombatComponent::ServerFire_Validate
(const FVector_NetQuantize& shootFromPos, const FVector_NetQuantize& hitTarget,
	AWeapon* firedWeapon, /*const float& clientTimeBetweenShots,*/ const float& currentClientTime)
{
	if (!firedWeapon) { return true; }
	if (!ownerPlayer) { return true; }

	// weapon must have dropped, because player died, and this packet arrived later
	if (firedWeapon->GetOwner() != ownerPlayer) { return true; }

	const float lastFiredThisWeapon = firedWeapon->GetLastClientPacketTime();
	const float timeBetweenShots = equippedWeapon->GetTimeBetweenShots();


	// We are checking the packing time, so this does the same thing basically, 
	// removed it to reduce bandwidth load

	// equippedWeapon->GetTimeBetweenShots() is on the server, and is accurate
	// if this does not match, the client is cheating
	//if(!FMath::IsNearlyEqual(clientTimeBetweenShots, timeBetweenShots, 0.0025f)) {return false;}

	const float lastClientPacketTimeWithThisWeapon = firedWeapon->GetLastClientPacketTime();

	// no fire packets came in so far from the client
	// let it through
	if(FMath::IsNearlyEqual(-1.f, lastClientPacketTimeWithThisWeapon, 0.0025f)) { return true; }


	// packet time difference check
	// packets can be arriving in different orders, so we are just comparing
	// every one of them to eachother, and all must have at least an absolute difference of delayBetweenShots.


	// this is supposed to be the later one, but arrive order may vary
	// if neither is true, then the two packets arrived at the same time, which means cheating as well.
	float timeDifference = { 0.f };
	if (currentClientTime > lastClientPacketTimeWithThisWeapon)
	{
		timeDifference = currentClientTime - lastClientPacketTimeWithThisWeapon;
	}
	else if (currentClientTime < lastClientPacketTimeWithThisWeapon)
	{
		timeDifference = lastClientPacketTimeWithThisWeapon - currentClientTime;
	}


	// time difference should be at least timeBetweenShots, whatever packet order the packets arrived in
	// if using the same weapon
	if (timeDifference < timeBetweenShots - 0.0025f) { return false; }
	

	return true;
}


void UCombatComponent::MulticastFire_Implementation
(const FVector_NetQuantize& shootFromPos, const FVector_NetQuantize& hitTarget)
{
	// Cannot check unoccupied here because combat state update might reach here first

	if (!ownerPlayer) ownerPlayer = Cast<APlayerCharacter>(GetOwner());
	if (!ownerPlayer) { return; }
	if (!equippedWeapon) { return; }


	if (IsReloadingShotgun())
	{
		if (ownerPlayer->HasAuthority())
		{
			combatState = ECombatState::ECS_Unoccupied;
		}
		if (IsPlayerLocallyControlled())
		{
			if (bIsLocallyReloading) bIsLocallyReloading = false;
		}
	}


	

	if (!IsPlayerAutonomousProxy())
	{
		FireEffect(shootFromPos, hitTarget);
		shootingInaccuracy += equippedWeapon->GetOneShotInaccuracyIncrease();
	}
	

	//StartFireTimer();
}

void UCombatComponent::FireEffect(const FVector_NetQuantize& shootFromPos, const FVector_NetQuantize& hitTarget)
{
	if (!ownerPlayer) ownerPlayer = Cast<APlayerCharacter>(GetOwner());
	if (!ownerPlayer) { return; }
	if (!equippedWeapon) { return; }

	ownerPlayer->PlayFireMontage(bIsAiming);
	equippedWeapon->Fire(shootFromPos, hitTarget);
}

bool UCombatComponent::IsPlayerAutonomousProxy()
{
	return ownerPlayer && ownerPlayer->GetLocalRole() == ENetRole::ROLE_AutonomousProxy;
}

bool UCombatComponent::IsPlayerLocallyControlled()
{
	return ownerPlayer && ownerPlayer->IsLocallyControlled();
}

bool UCombatComponent::IsPlayerServer()
{
	return ownerPlayer && ownerPlayer->HasAuthority();
}



void UCombatComponent::StartFireTimer()
{
	if (!equippedWeapon) { return; }
	if (!ownerPlayer) { return; }

	ownerPlayer->GetWorldTimerManager()
		.SetTimer(fireTimerHandle, this, &UCombatComponent::FireTimerFinished, delayBetweenShots);


	// bCanFire = false;
}

void UCombatComponent::FireTimerFinished()
{
	bCanFire = true;
	AutomaticFireCheck();
}

void UCombatComponent::AutomaticFireCheck()
{
	if (bUsingAutomatic)
	{
		TryFire();
	}
}




void UCombatComponent::SetNormalWalkSpeed()
{
	if (playerMovementComp){playerMovementComp->MaxWalkSpeed = normalWalkSpeed;}
}

void UCombatComponent::SetAimWalkSpeed()
{
	if (playerMovementComp){playerMovementComp->MaxWalkSpeed = aimWalkSpeed;}
}

void UCombatComponent::ServerFireBlockedChanged_Implementation(bool newState)
{
	MulticastFireBlockedChanged(newState);
}

void UCombatComponent::MulticastFireBlockedChanged_Implementation(bool newState)
{
	if (!ownerPlayer) { return; }
	if (ownerPlayer->IsLocallyControlled()) { return; }

	bDisallowFireWeaponTooCloseToObject = newState;
}




void UCombatComponent::ReloadButtonPressed()
{
	if (!ownerPlayer) ownerPlayer = Cast<APlayerCharacter>(GetOwner());

	if (!equippedWeapon) { return; }
	if (!HasCarriedAmmo()) { return; }
	if (!IsUnoccupied()) { return; }
	if (IsWeaponFull()) { return; }
	if (bIsLocallyReloading) { return; }
	if (bIsLocallySwappingWeapons) { return; }

	ServerReload();


	if (IsPlayerAutonomousProxy())
	{
		HandleReload();
	}
}

void UCombatComponent::ServerReload_Implementation()
{
	if (!equippedWeapon) { return; }
	if (!HasCarriedAmmo()) { return; }
	if (!IsUnoccupied()) { return; }
	if (IsWeaponFull()) { return; }

	combatState = ECombatState::ECS_Reloading;
	HandleReload();
}

void UCombatComponent::OnRep_combatState()
{
	switch (combatState)
	{
	case ECombatState::ECS_Unoccupied:
		AutomaticFireCheck();
		break;
	case ECombatState::ECS_Reloading:
		if (!IsPlayerAutonomousProxy())
		{
			HandleReload();
		}
		break;
	case ECombatState::ECS_ThrowingGrenade:
		HandleThrowGrenade();
		break;
	case ECombatState::ECS_SwappingWeapons:
		if (!IsPlayerAutonomousProxy())
		{
			HandleSwapWeaponState();
		}
	default:

		break;
	}
}

void UCombatComponent::EnableWeaponEquipSoundGuardClient()
{
	if (!ownerPlayer) { return; }
	if (ownerPlayer->HasAuthority()) { return; }

	ownerPlayer->GetWorldTimerManager().SetTimer
	(weaponEquipSoundTimer, this, &ThisClass::DisableWeaponEquipSoundGuardClient, weaponEquipSoundGuardTime);

	bWeaponEquipSoundGuard = true;
}

void UCombatComponent::DisableWeaponEquipSoundGuardClient()
{
	if (!ownerPlayer) { return; }
	if (ownerPlayer->HasAuthority()) { return; }

	bWeaponEquipSoundGuard = false;
}

void UCombatComponent::HandleThrowGrenade()
{
	if (!ownerPlayer) ownerPlayer = Cast<APlayerCharacter>(GetOwner());
	if (!ownerPlayer) { return; }
	if (!equippedWeapon) { return; }

	// attach weapon to left Hand

	AttachWeaponToLeftHand();

	ownerPlayer->PlayThrowGrenadeMontage();
	ownerPlayer->ShowGrenade(true);

	if (IsPlayerLocallyControlled())
	{
		if (bIsLocallyReloading) { bIsLocallyReloading = false; }
		if (bIsLocallySwappingWeapons) { bIsLocallySwappingWeapons = false; }
	}

	
}





void UCombatComponent::AttachWeaponToLeftHand()
{
	if (!ownerPlayer) { return; }
	if (!equippedWeapon) { return; }

	if (equippedWeapon->GetWeaponType() == EWeaponType::EWT_Pistol ||
		equippedWeapon->GetWeaponType() == EWeaponType::EWT_SubmachineGun)
	{
		AttachWeaponToLeftHand_S();
	}
	else
	{
		AttachWeaponToLeftHand_L();
	}



	
}

void UCombatComponent::AttachWeaponToLeftHand_L()
{
	if (!ownerPlayer)  return; 
	if (!ownerPlayer->GetMesh()) return;
	if (!equippedWeapon) { return; }

	if (equippedWeapon->UseRotatedRightHandSocket())
	{
		if (!leftHandSocket_LongWeapon_Rotated)
		{
			leftHandSocket_LongWeapon_Rotated = ownerPlayer->GetMesh()->
				GetSocketByName(LEFT_HAND_SOCKET_LONG_WEAPON_ROTATED);
		}
		if (leftHandSocket_LongWeapon_Rotated)
		{
			leftHandSocket_LongWeapon_Rotated->AttachActor(equippedWeapon, ownerPlayer->GetMesh());
		}
	}
	else
	{
		if (!leftHandSocket_LongWeapon)
		{
			leftHandSocket_LongWeapon = ownerPlayer->GetMesh()->GetSocketByName(LEFT_HAND_SOCKET_LONG_WEAPON);
		}
		if (leftHandSocket_LongWeapon)
		{
			leftHandSocket_LongWeapon->AttachActor(equippedWeapon, ownerPlayer->GetMesh());
		}
	}

	
}

void UCombatComponent::AttachWeaponToLeftHand_S()
{
	if (!leftHandSocket_ShortWeapon)
	{
		if (ownerPlayer->GetMesh())
			leftHandSocket_ShortWeapon = ownerPlayer->GetMesh()->GetSocketByName(LEFT_HAND_SOCKET_SHORT_WEAPON);
	}
	if (leftHandSocket_ShortWeapon)
	{
		leftHandSocket_ShortWeapon->AttachActor(equippedWeapon, ownerPlayer->GetMesh());
	}
}

void UCombatComponent::SwapWeaponsButtonPressed()
{
	if (!ownerPlayer) ownerPlayer = Cast<APlayerCharacter>(GetOwner());
	if (!equippedWeapon || !secondaryWeapon) { return; }
	if (!IsUnoccupied() && !IsReloading()) { return; }
	if (bIsLocallySwappingWeapons) { return; }

	if (ownerPlayer && ownerPlayer->IsLocallyControlled())
	{
		bIsLocallySwappingWeapons = true;
		if (bIsLocallyReloading) { bIsLocallyReloading = false; }
	}

	if (IsPlayerAutonomousProxy())
	{
		HandleSwapWeaponState();
	}

	ServerSwapWeapons();
}

void UCombatComponent::ServerSwapWeapons_Implementation()
{
	if (!equippedWeapon || !secondaryWeapon) { return; }
	if (!IsUnoccupied() && !IsReloading()) { return; }

	combatState = ECombatState::ECS_SwappingWeapons;
	HandleSwapWeaponState();
}

void UCombatComponent::SwapEquippedAndSecondaryWeapons()
{
	if (!equippedWeapon || !secondaryWeapon) { return; }
	if (!ownerPlayer) ownerPlayer = Cast<APlayerCharacter>(GetOwner());
	if (!ownerPlayer) { return; }

	if (IsPlayerServer())
	{
		MulticastSwapEquippedAndSecondaryWeapons();
	}
}

void UCombatComponent::FinishSwapWeapons()
{
	if (!ownerPlayer) ownerPlayer = Cast<APlayerCharacter>(GetOwner());

	if (IsPlayerLocallyControlled())
	{
		bIsLocallySwappingWeapons = false;
	}

	if (IsPlayerServer())
	{
		combatState = ECombatState::ECS_Unoccupied;
		ReloadIfEmpty();
	}
}

void UCombatComponent::MulticastSwapEquippedAndSecondaryWeapons_Implementation()
{
	if (!equippedWeapon || !secondaryWeapon) { return; }

	AWeapon* main = equippedWeapon;
	AWeapon* sec = secondaryWeapon;
	SetPrimaryWeapon(sec);
	SetSecondaryWeapon(main);
	PlayEquippedWeaponEquipSound();
}



bool UCombatComponent::HasProjectileWeaponEquipped()
{
	return equippedWeapon && Cast<AProjectileWeapon>(equippedWeapon);
}

bool UCombatComponent::HasProjectileWeaponEquipped(AProjectileWeapon*& outWeapon)
{
	outWeapon = nullptr;
	if (equippedWeapon)
	{
		outWeapon = Cast<AProjectileWeapon>(equippedWeapon);
	}
	return outWeapon != nullptr;
}

void UCombatComponent::HandleSwapWeaponState()
{
	if (!ownerPlayer) { return; }
	ownerPlayer->PlaySwapWeaponsMontage();

	if (IsPlayerLocallyControlled())
	{
		if (bIsLocallyReloading) { bIsLocallyReloading = false; }
		bIsLocallySwappingWeapons = true;
	}
}


void UCombatComponent::ThrowGrenadeButtonPressed()
{
	if (!IsUnoccupied() && !IsReloading()) { return; }
	if (!equippedWeapon) { return; }
	if (!HasHandGrenades()) { return; }

	ServerThrowGrenade();
}



void UCombatComponent::ServerThrowGrenade_Implementation()
{
	if (!IsUnoccupied() && !IsReloading()) { return; }
	if (!equippedWeapon) { return; }

	

	combatState = ECombatState::ECS_ThrowingGrenade;
	HandleThrowGrenade();
}


void UCombatComponent::FinishThrowGrenade()
{
	if (!ownerPlayer) { return; }
	if (!ownerPlayer->HasAuthority()) { return; }

	AttachWeaponToRightHand();
	combatState = ECombatState::ECS_Unoccupied;
	ReloadIfEmpty();
}

void UCombatComponent::ReleaseGrenadeFromHand()
{
	if (!ownerPlayer) { return; }
	ownerPlayer->ShowGrenade(false);

	// only the owner raycasts the tickTraceHitResult
	if(ownerPlayer->IsLocallyControlled())
	ServerSpawnGrenade(tickTraceHitResult.ImpactPoint);
}

void UCombatComponent::ServerSpawnGrenade_Implementation(const FVector_NetQuantize& hitTarget)
{
	if (!HasHandGrenades()) { return; }

	SpendHandGrenade();

	if (UWorld* world = GetWorld())
	{
		const FVector startLoc = ownerPlayer->GetGrenadeInHandPosition();
		const FVector direction = hitTarget - startLoc;
		const FVector spawnLoc = startLoc + ((direction).GetSafeNormal() * 50.f);
		FActorSpawnParameters spawnParams{};
		spawnParams.Owner = ownerPlayer;
		spawnParams.Instigator = ownerPlayer;
		spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		world->SpawnActor<AProjectile>(grenadeClass, spawnLoc, direction.Rotation(), spawnParams);
	}
}

void UCombatComponent::SpendHandGrenade()
{
	if (!ownerPlayer) { return; }
	if (!ownerPlayer->HasAuthority()) { return; }

	currentHandGrenades = FMath::Clamp(currentHandGrenades - 1, 0, 999);

	HUD_UpdateGrenades();
}







void UCombatComponent::HandleReload()
{
	if (!ownerPlayer) { return; }
	ownerPlayer->PlayReloadMontage();

	if (IsPlayerLocallyControlled())
	{
		bIsLocallyReloading = true;
		if (bIsLocallySwappingWeapons) { bIsLocallySwappingWeapons = false; }
	}

//	if (!bCanFire) { bCanFire = true; }
}

void UCombatComponent::ClientLocalPlayReloadAnim_Implementation()
{
	HandleReload();
	
}

// Server
void UCombatComponent::FinishReloading()
{
	if (ownerPlayer && ownerPlayer->IsLocallyControlled())
	{
		bIsLocallyReloading = false;
	}

	if (ownerPlayer && ownerPlayer->HasAuthority() && equippedWeapon)
	{
		combatState = ECombatState::ECS_Unoccupied;

		if (!bCanFire) { bCanFire = true; }

		const int32 missingAmmoFromMag = equippedWeapon->GetMagSize() - equippedWeapon->GetCurrentAmmo();
		const int32 minOfCarriedAmmoAndMissingAmmoFromMag = FMath::Min(missingAmmoFromMag, carriedAmmo);


		if (carriedAmmoMap.Contains(equippedWeapon->GetWeaponType()))
		{
			carriedAmmoMap[equippedWeapon->GetWeaponType()] -= minOfCarriedAmmoAndMissingAmmoFromMag;
			carriedAmmo = carriedAmmoMap[equippedWeapon->GetWeaponType()];
			equippedWeapon->AddToCurrentAmmo(minOfCarriedAmmoAndMissingAmmoFromMag);
		}

		HUD_UpdateCarriedAmmo();
		AutomaticFireCheck();
	}
}

void UCombatComponent::LoadShell()
{
	if (ownerPlayer && ownerPlayer->HasAuthority() && equippedWeapon)
	{
		if(IsWeaponFull() || !equippedWeapon->IsShotgun()) {return;}

		if (carriedAmmoMap.Contains(equippedWeapon->GetWeaponType()))
		{
			carriedAmmoMap[equippedWeapon->GetWeaponType()] -= 1;
			carriedAmmo = carriedAmmoMap[equippedWeapon->GetWeaponType()];
			equippedWeapon->AddToCurrentAmmo(1);
		}

		if (IsWeaponFull() || !HasCarriedAmmo())
		{
			ownerPlayer->JumpToShotgunMontageEnd();
		}

		HUD_UpdateCarriedAmmo();
	}
}

bool UCombatComponent::HasShotgunEquipped()
{
	return equippedWeapon && equippedWeapon->IsShotgun();
}









void UCombatComponent::TraceUnderCrosshairs(FHitResult& hitResult)
{
	if (!ownerPlayer) { return; }

	FVector2D viewportSize{};
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(viewportSize);
	}

	FVector2D crosshairLocation{ viewportSize.X / 2.f, viewportSize.Y / 2.f };
	FVector crosshairWorldPos{};
	FVector crosshairWorldDir{};
	if (UGameplayStatics::DeprojectScreenToWorld(UGameplayStatics::GetPlayerController(this, 0), // Always gets local player
		crosshairLocation, crosshairWorldPos, crosshairWorldDir))
	{
		FVector start = crosshairWorldPos;

		
		const float cameraDistanceToPlayer = (ownerPlayer->GetActorLocation() - start).Size();
		start += crosshairWorldDir * (cameraDistanceToPlayer + 100.f);
		//DRAW_SPHERE_SHORT_B(start);
		ownerPlayer->HideCharacterAndWeapon(cameraDistanceToPlayer < hideCharacterDistanceToCamera);
		

		FVector end = start + crosshairWorldDir * screenTraceRange;
		if (GetWorld())
		{
			GetWorld()->LineTraceSingleByChannel(hitResult, start, end,
				ECollisionChannel::ECC_Visibility);
		}

		if (!hitResult.bBlockingHit)
		{
			hitResult.ImpactPoint = start + crosshairWorldDir * (lastTraceDistance * 1.25f);
			
		}
		else
		{
			lastTraceDistance = (start + hitResult.ImpactPoint).Size();
		}

	//	DRAW_POINT_ONE_FRAME(hitResult.ImpactPoint);

		// This is also a good way, this uses the I version, the other uses the U version
		//Cast<ICrosshairInteractInterface>(hitResult.GetActor());

		bDisallowFireWeaponTooCloseToObject_Calculation = false;
		//FVector rightHandUpLocation{};
		if (equippedWeapon && rightHandSocket && ownerPlayer->GetMesh())
		{
			
			FCollisionQueryParams queryParams{};
			queryParams.AddIgnoredActor(equippedWeapon);
			queryParams.AddIgnoredActor(ownerPlayer);
			const float blockDistance = equippedWeapon->GetDisAllowShootingIfTooCloseDistance();
			const FVector boneLoc = ownerPlayer->GetMesh()->GetBoneLocation(RIGHT_HAND_BONE);
			//	// rightHandSocket->GetSocketLocation(equippedWeapon->GetWeaponMesh());

			if (GetWorld()->LineTraceSingleByChannel(tickTraceWeaponBlockHitResult,
				boneLoc, hitResult.ImpactPoint,
				ECollisionChannel::ECC_Visibility, queryParams))
			{
				if ((!bDisallowFireWeaponTooCloseToObject &&
					tickTraceWeaponBlockHitResult.Distance <= blockDistance)
					||
					(bDisallowFireWeaponTooCloseToObject &&
						tickTraceWeaponBlockHitResult.Distance <= blockDistance + 20.f))
				{
					bDisallowFireWeaponTooCloseToObject_Calculation = true;
				}
			}
		}

		if(bDisallowFireWeaponTooCloseToObject != bDisallowFireWeaponTooCloseToObject_Calculation)
		bDisallowFireWeaponTooCloseToObject = bDisallowFireWeaponTooCloseToObject_Calculation;

		if (bDisallowFireWeaponTooCloseToObject != bDisallowFireWeaponTooCloseToObject_LastFrame)
		{
			ServerFireBlockedChanged(bDisallowFireWeaponTooCloseToObject);
			bDisallowFireWeaponTooCloseToObject_LastFrame = bDisallowFireWeaponTooCloseToObject;

			if (!bDisallowFireWeaponTooCloseToObject)
				AutomaticFireCheck();
		}

		SetCrosshairColour(hitResult, bDisallowFireWeaponTooCloseToObject);
		SetRightHandLookRotator(hitResult.ImpactPoint);
	}
}

void UCombatComponent::SetCrosshairColour(const FHitResult& hitResult, bool bBlocked)
{
	if (!hitResult.GetActor()) {
		crosshairPackage.crosshairColour = bBlocked ? FLinearColor::Transparent : FLinearColor::White;
		if (playerController)
		{
			playerController->SetHUDPlayerInCrosshairName(FString{}, FColor::Transparent);
		}
		return; 
	}


	ICrosshairInteractInterface* target = Cast<ICrosshairInteractInterface>(hitResult.GetActor());

	if (target)
	{
		ETeams targetTeam = target->GetTeam();
		FColor displayColor{};
		if (targetTeam == ETeams::ET_NoTeam || targetTeam != ownerPlayer->GetTeam())
		{
			crosshairPackage.crosshairColour = FLinearColor::Red;
			displayColor = FColor::Red;
		}
		else // Same Team
		{
			crosshairPackage.crosshairColour = FLinearColor::Green;
			displayColor = FColor::Green;
		}


		if (playerController)
		{
			playerController->SetHUDPlayerInCrosshairName(target->GetDisplayName(), displayColor);
		}
	}
	else
	{
		crosshairPackage.crosshairColour = FLinearColor::White;
		if (playerController)
		{
			playerController->SetHUDPlayerInCrosshairName(FString{}, FColor::Transparent);
		}
	}

	if (bBlocked)
	{
		crosshairPackage.crosshairColour = FLinearColor::Transparent;
	}
}

void UCombatComponent::SetRightHandLookRotator(const FVector& lookPoint)
{
	if (!GetWorld()) { return; }
	if (!ownerPlayer) { return; }
	if (!ownerPlayer->GetMesh()) { return; }
	if (!equippedWeapon) { return; }
	if (!rightHandSocket) rightHandSocket = ownerPlayer->GetMesh()->GetSocketByName(RIGHT_HAND_SOCKET);
	if (rightHandSocket)
	{
		//GEPRS_R("Setting Right Hand Rot");

		//FTransform weaponTransform = equippedWeapon->GetWeaponMesh()->
			//GetSocketTransform(MUZZLE_FLASH_SOCKET, ERelativeTransformSpace::RTS_World);

		FTransform handTransform =
			ownerPlayer->GetMesh()->GetSocketTransform(RIGHT_HAND_BONE, ERelativeTransformSpace::RTS_World);
		FVector handLoc = handTransform.GetLocation();
		//FRotator handRot = handTransform.GetRotation().Rotator();

		FRotator currentHandRotator = UKismetMathLibrary::FindLookAtRotation
		(handLoc, handLoc + (handLoc - lookPoint));



		rightHandToTargetRotation = FMath::RInterpTo(
			rightHandToTargetRotation, currentHandRotator,
			GetWorld()->GetDeltaSeconds(), 35.f);


		
		//DrawDebugLine(GetWorld(), handLoc, handLoc + 
		//	(FRotationMatrix(handToTargetRotation).GetUnitAxis(EAxis::X) * 200.f), FColor::Orange, false, 0.05f);
		//DrawDebugLine(GetWorld(), handLoc, handLoc + (handLoc - hitResult.ImpactPoint), FColor::White, false, 0.05f);
		//DrawDebugLine(GetWorld(), handLoc, handLoc +
		//	(FRotationMatrix(handRot).GetUnitAxis(EAxis::X) * 200.f), FColor::Blue, false, 0.05f);

		//GEPR_R(FString::Printf(TEXT("%f %f %f"), handToTargetRotation.Pitch, handToTargetRotation.Yaw, handToTargetRotation.Roll));


		//DrawDebugSphere(GetWorld(), handLoc, 6.f, 12, FColor::Red, false, 0.1f);
		//DrawDebugSphere(GetWorld(), 
		//	hitResult.ImpactPoint, // hitResult.ImpactPoint,
		//	12.f, 12, FColor::Black, false, 0.1f);
		
	}
}








void UCombatComponent::OnRep_secondaryWeapon()
{
	if (secondaryWeapon)
		secondaryWeapon->SetWeaponState(EWeaponState::EWS_Stored);
	AttachSecondaryWeaponToSecondarySocket();
	PlaySecondaryWeaponEquipSound();
}

void UCombatComponent::SetWeaponAttributes()
{
	if (!equippedWeapon) { return; }
	if (!playerMovementComp) { return; }

	bUsingAutomatic = equippedWeapon->IsAutomatic();
	delayBetweenShots = equippedWeapon->GetTimeBetweenShots();
	shootingInaccuracyDecreaseInterpSpeed = equippedWeapon->GetInaccuracyDecreaseSpeed();
	movementInaccuracyMultiplier = equippedWeapon->GetMovementInaccuracyMultiplier();
	crouchingInaccuracyDecrease = equippedWeapon->GetCrouchingInaccuracyDecrease();
	aimingInaccuracyDecrease = equippedWeapon->GetAimingInaccuracyDecrease();
	crosshairTraceCheckBoxSize = equippedWeapon->GetCrosshairTraceCheckBoxSize();

	float w_normalWalkSpeed{};
	float w_aimWalkSpeed{};
	float w_crouchWalkSpeed{};
	equippedWeapon->GetWeaponMovementSpeed(w_normalWalkSpeed, w_aimWalkSpeed, w_crouchWalkSpeed);

	normalWalkSpeed = w_normalWalkSpeed;
	aimWalkSpeed = w_aimWalkSpeed;
	crouchWalkSpeed = w_crouchWalkSpeed;

	playerMovementComp->MaxWalkSpeed = normalWalkSpeed;
	playerMovementComp->MaxWalkSpeedCrouched = crouchWalkSpeed;
	DecideWalkSpeedOnAiming(bIsAiming);

	/*
	* 
	* CHEATING TEST
	* 
	* 	if (IsPlayerAutonomousProxy())
		{
			delayBetweenShots = 0.05f;
		}
	*/

	StartFireTimer();
}

void UCombatComponent::OnRep_bIsAiming()
{
	if (ownerPlayer && ownerPlayer->IsLocallyControlled())
	{
		bIsAiming = bIsAimingLocally;

		if(EquippedWeaponHasScope())
		ShowSniperScopeWidget(bIsAimingLocally);
	}
}





bool UCombatComponent::HasCurrentAmmo()
{
	return equippedWeapon && equippedWeapon->HasCurrentAmmo();
}

bool UCombatComponent::IsWeaponFull()
{
	return equippedWeapon && equippedWeapon->IsFull();
}



void UCombatComponent::SetAiming(bool state)
{
	bIsAiming = state;
	ServerSetAiming(state);
	DecideWalkSpeedOnAiming(state);
	ShowSniperScopeWidget(state);

	if (ownerPlayer && ownerPlayer->IsLocallyControlled())
	{
		bIsAimingLocally = state;
	}
}

void UCombatComponent::ShowSniperScopeWidget(bool state, bool bForce)
{
	// Disable scope when equipping another weapon!!!

	if (!ownerPlayer) { return; }
	if (!ownerPlayer->IsLocallyControlled()) { return; }
	if (!equippedWeapon) { return; }

	if (bForce)
	{
		ownerPlayer->ShowSniperScopeWidget(state, false);
		return;
	}

	if (!EquippedWeaponHasScope()) { return; }

	ownerPlayer->ShowSniperScopeWidget(state, true);
}

void UCombatComponent::ServerSetAiming_Implementation(bool state)
{
	bIsAiming = state;
	DecideWalkSpeedOnAiming(state);
}

void UCombatComponent::DecideWalkSpeedOnAiming(bool state)
{
	if (state) SetAimWalkSpeed(); else SetNormalWalkSpeed();
}



void UCombatComponent::EquipWeapon(AWeapon* weapon, bool bButtonHeld)
{
	if (!weapon) { return; }
	if (!ownerPlayer) { return; }
	if (!IsUnoccupied()) { return; }

	// Short: P and S -> Switch S
	// Held: P and S -> Switch P
	// Short: P and not S -> Set S
	// Held: P and not S -> Switch P old P Set to S

	// Short: not P -> Set P
	// Held: not P -> Set P

	if (UBuffComponent* buffComp = ownerPlayer->GetBuffComponent())
	{
		 if (buffComp->IsCurrentlySpedUp())
		 {
			 buffComp->ResetSpeed();
		 }

		 if (buffComp->CurrenthyHasDamageReduction())
		 {
			 buffComp->ResetDamageReduction();
		 }
	}

	if (equippedWeapon && secondaryWeapon)
	{
		if (bButtonHeld)
		{
			DropSecondaryWeapon();
			SetSecondaryWeapon(weapon);
			PlaySecondaryWeaponEquipSound();
		}
		else
		{
			DropEquippedWeapon();
			SetPrimaryWeapon(weapon);
			PlayEquippedWeaponEquipSound();
		}
	}
	else if (equippedWeapon && !secondaryWeapon)
	{
		if (bButtonHeld)
		{
			SetSecondaryWeapon(weapon);
			PlaySecondaryWeaponEquipSound();
		}
		else
		{
			MovePrimaryWeaponToSecondary();
			SetPrimaryWeapon(weapon);
			PlayEquippedWeaponEquipSound();
		}
	}
	else if (!equippedWeapon)
	{
		SetPrimaryWeapon(weapon);
		PlayEquippedWeaponEquipSound();
	}

	SetMovementCompToHandleWeapon();
	
}

void UCombatComponent::MovePrimaryWeaponToSecondary()
{
	SetSecondaryWeapon(equippedWeapon);
	equippedWeapon = nullptr;
}

void UCombatComponent::SetPrimaryWeapon(AWeapon* weapon)
{
	if (!weapon) { return; }

	if (IsPlayerServer())
	{
		SetEquippedWeapon(weapon);
		SetCarriedAmmoToEquippedWeaponType();
		ReloadIfEmpty();
	}

	HUD_UpdateCarriedAmmo();
	HUD_UpdateGrenades();
	AttachWeaponToRightHand();
	SetWeaponAttributes();
	HandleScopeOnEquip();
}

bool UCombatComponent::EquippedWeaponHasScope()
{
	return equippedWeapon && equippedWeapon->HasScope();
}

void UCombatComponent::AttachSecondaryWeaponToSecondarySocket()
{
	if (!ownerPlayer) { return; }
	if (!ownerPlayer->GetMesh()) { return; }
	if (!secondaryWeapon) { return; }

	if (secondaryWeapon->GetWeaponType() == EWeaponType::EWT_Pistol ||
		secondaryWeapon->GetWeaponType() == EWeaponType::EWT_SubmachineGun)
	{

		if (!secondarySocket_ShortWeapon)
		{
			secondarySocket_ShortWeapon =
				ownerPlayer->GetMesh()->GetSocketByName(SECONDARY_WEAPON_SOCKET_SHORT_WEAPON);
		}
		if (secondarySocket_ShortWeapon)
		{
			secondarySocket_ShortWeapon->AttachActor(secondaryWeapon, ownerPlayer->GetMesh());
		}
	}
	else
	{
		if (secondaryWeapon->UseRotatedRightHandSocket())
		{
			if (!secondarySocket_LongWeapon_Rotated)
			{
				secondarySocket_LongWeapon_Rotated = 
					ownerPlayer->GetMesh()->GetSocketByName(SECONDARY_WEAPON_SOCKET_LONG_WEAPON_ROTATED);
			}
			if (secondarySocket_LongWeapon_Rotated)
			{
				secondarySocket_LongWeapon_Rotated->AttachActor(secondaryWeapon, ownerPlayer->GetMesh());
			}
		}
		else
		{
			if (!secondarySocket_LongWeapon)
			{
				secondarySocket_LongWeapon =
					ownerPlayer->GetMesh()->GetSocketByName(SECONDARY_WEAPON_SOCKET_LONG_WEAPON);
			}
			if (secondarySocket_LongWeapon)
			{
				secondarySocket_LongWeapon->AttachActor(secondaryWeapon, ownerPlayer->GetMesh());
			}
		}

		
	}
}

void UCombatComponent::SetSecondaryWeapon(AWeapon* weapon)
{
	if (!ownerPlayer) { return; }
	if (!ownerPlayer->HasAuthority()) { return; }
	if (!weapon) { return; }

	secondaryWeapon = weapon;
	secondaryWeapon->SetOwner(ownerPlayer);
	secondaryWeapon->SetWeaponState(EWeaponState::EWS_Stored);
	AttachSecondaryWeaponToSecondarySocket();
}

void UCombatComponent::SetMovementCompToHandleWeapon()
{
	if (playerMovementComp)
	{
		playerMovementComp->bOrientRotationToMovement = false;
		ownerPlayer->bUseControllerRotationYaw = true;
	}
}

void UCombatComponent::ReloadIfEmpty()
{
	if (!IsPlayerServer()) { return; }
	if (!equippedWeapon) { return; }
	if (!equippedWeapon->HasCurrentAmmo() && HasCarriedAmmo())
	{
		ReloadButtonPressed();

		if (IsPlayerServer() && !IsPlayerLocallyControlled())
		{
			ClientLocalPlayReloadAnim();
		}
	}
}

void UCombatComponent::SetCarriedAmmoToEquippedWeaponType()
{
	if (!ownerPlayer) { return; }
	if (!ownerPlayer->HasAuthority()) { return; }

	if (carriedAmmoMap.Contains(equippedWeapon->GetWeaponType()))
	{
		carriedAmmo = carriedAmmoMap[equippedWeapon->GetWeaponType()];
	}
	else
	{
		carriedAmmo = 0;
	}
}

void UCombatComponent::SetEquippedWeapon(AWeapon* weapon)
{
	if (!ownerPlayer) { return; }
	if (!ownerPlayer->HasAuthority()) { return; }
	if (!weapon) { return; }

	equippedWeapon = weapon;
	equippedWeapon->SetOwner(ownerPlayer);
	equippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
}

void UCombatComponent::DropEquippedWeapon()
{
	if (!ownerPlayer) { return; }
	if (!ownerPlayer->HasAuthority()) { return; }

	if (equippedWeapon)
	{
		equippedWeapon->Drop();
		equippedWeapon = nullptr;
	}
}

void UCombatComponent::DropSecondaryWeapon()
{
	if (!ownerPlayer) { return; }
	if (!ownerPlayer->HasAuthority()) { return; }

	if (secondaryWeapon)
	{
		secondaryWeapon->Drop();
		secondaryWeapon = nullptr;
	}
}

void UCombatComponent::AttachWeaponToRightHand()
{
	if (!ownerPlayer) { return; }
	if (!equippedWeapon) { return; }
	if (!ownerPlayer->GetMesh()) { return; }

	if (equippedWeapon->UseRotatedRightHandSocket())
	{
		if (!rightHandSocket_Rotated)
		{
			rightHandSocket_Rotated = ownerPlayer->GetMesh()->GetSocketByName(RIGHT_HAND_SOCKET_ROTATED);
		}
		if (rightHandSocket_Rotated)
		{
			rightHandSocket_Rotated->AttachActor(equippedWeapon, ownerPlayer->GetMesh());
		}
	}
	else
	{
		if (!rightHandSocket)
		{
			rightHandSocket = ownerPlayer->GetMesh()->GetSocketByName(RIGHT_HAND_SOCKET);
		}
		if (rightHandSocket)
		{
			rightHandSocket->AttachActor(equippedWeapon, ownerPlayer->GetMesh());
		}
	}
}

void UCombatComponent::ClientAutonomousProxyProjectileEarlyDestroyed()
{
	if (!equippedWeapon) { return; }
	AProjectileWeapon* asProjWeapon = Cast<AProjectileWeapon>(equippedWeapon);
	if (!asProjWeapon) { return; }
	TSubclassOf<AProjectile> projectile = asProjWeapon->GetProjectileToSpawn();
	if (!projectile) { return; }
	AProjectile* projectileToSpawn = Cast<AProjectile>(projectile.Get());
	


}



void UCombatComponent::PlayEquippedWeaponEquipSound()
{
	if (bWeaponEquipSoundGuard) { return; }

	if (equippedWeapon && equippedWeapon->GetEquippedWeaponSound())
	{
		UGameplayStatics::PlaySoundAtLocation
		(this, equippedWeapon->GetEquippedWeaponSound(), ownerPlayer->GetActorLocation());

		EnableWeaponEquipSoundGuardClient();
	}
}

void UCombatComponent::PlaySecondaryWeaponEquipSound()
{
	if (bWeaponEquipSoundGuard) { return; }

	if (secondaryWeapon && secondaryWeapon->GetEquippedWeaponSound())
	{
		UGameplayStatics::PlaySoundAtLocation
		(this, secondaryWeapon->GetEquippedWeaponSound(), ownerPlayer->GetActorLocation());

		EnableWeaponEquipSoundGuardClient();
	}
}

void UCombatComponent::HUD_UpdateCarriedAmmo()
{
	if (ownerPlayer && ownerPlayer->IsLocallyControlled())
	{
		if (!playerController) playerController = Cast<AMainPlayerController>(ownerPlayer->GetController());
		if (playerController)
		{
			playerController->SetHUDCarriedAmmo(carriedAmmo);
		}
	}
}

void UCombatComponent::OnRep_carriedAmmo()
{
	HUD_UpdateCarriedAmmo();
}

void UCombatComponent::OnRep_currentHandGrenades()
{
	HUD_UpdateGrenades();
}

void UCombatComponent::HUD_UpdateGrenades()
{
	if (!equippedWeapon) { return; }

	if (!playerController) playerController = Cast<AMainPlayerController>(ownerPlayer->GetController());
	if (playerController)
	{
		playerController->SetHUDGrenades(currentHandGrenades);
	}
}





void UCombatComponent::OnRep_equippedWeapon()
{
	if (!equippedWeapon) { return; }
	if (!ownerPlayer) { return; }

	equippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);

	AttachWeaponToRightHand();
	SetMovementCompToHandleWeapon();
	SetWeaponAttributes();
	

	ReloadIfEmpty();

	equippedWeapon->HUD_UpdateCurrentAmmo();
	HUD_UpdateGrenades();
	HUD_UpdateCarriedAmmo();

	PlayEquippedWeaponEquipSound();

	HandleScopeOnEquip();
}

void UCombatComponent::HandleScopeOnEquip()
{
	if (EquippedWeaponHasScope() && bIsAiming && bIsAimingLocally)
	{
		ShowSniperScopeWidget(true);
	}
	else
	{
		ShowSniperScopeWidget(false, true);
	}
}



//
//float UCombatComponent::GetCurrentInaccuracy()
//{
//	if (!GetWorld()) { return 1000.f; }
//
//	return FMath::Clamp(GetCalculateInaccuracy(), 0.f, 1000.f);
//}




