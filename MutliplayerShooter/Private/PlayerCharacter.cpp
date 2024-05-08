// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HUD/OverheadWidget.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "Weapon/Weapon.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "PlayerComponents/CombatComponent.h"
#include "PlayerComponents/BuffComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "PlayerAnimInstance.h"
#include "../Source/MutliplayerShooter/MutliplayerShooter.h"
#include "Controller/MainPlayerController.h"
#include "GameMode/MainGameMode.h"
#include "TimerManager.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"
#include "PlayerState/MainPlayerState.h"
#include "Weapon/WeaponTypes.h"
#include "Components/BoxComponent.h"
#include "PlayerComponents/LagCompensationComponent.h"
#include "Interface/InstigatorTypeInterface.h"
#include "Components/CapsuleComponent.h"
#include "GameState/MainGameState.h"
#include "HelperTypes/Teams.h"
#include "Weapon/HeadshotDamageType.h"

#include "NiagaraSystem.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

#include "DebugMacros.h"


/*
* 
* 
* 
* 
*/

//ABlasterPlayerController::PawnLeavingGame() and ABlasterGameMode::Logout(AController* Exiting)

/*
*
*
*
*
*/

APlayerCharacter::APlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = true;
	

	springArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("Spring Arm"));
	if (GetMesh()) springArmComponent->SetupAttachment(GetMesh());
	springArmComponent->TargetArmLength = 600.f;
	springArmComponent->bUsePawnControlRotation = true;
	//springArmComponent->SocketOffset = 

	cameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Player Camera"));
	cameraComponent->SetupAttachment(springArmComponent, USpringArmComponent::SocketName);
	cameraComponent->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->bOrientRotationToMovement = true;
		GetCharacterMovement()->RotationRate = FRotator{ 0.f, 850.f, 0.f };
	}

	overheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("Overhead Widget"));
	overheadWidget->SetupAttachment(RootComponent);

	combatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("Combat Component"));
	combatComponent->SetIsReplicated(true);

	buffComponent = CreateDefaultSubobject<UBuffComponent>(TEXT("Buff Component"));
	buffComponent->SetIsReplicated(true);

	// Will only be used by the server, no need to be replicated
	lagCompensationComponent = CreateDefaultSubobject<ULagCompensationComponent>(TEXT("Lagg Compensation Component"));



	if (GetMovementComponent()) GetMovementComponent()->GetNavAgentPropertiesRef().bCanCrouch = true;
	//if (GetMovementComponent()) GetMovementComponent()->NavAgentProps.bCanCrouch = true;

	if (GetCapsuleComponent()) GetCapsuleComponent()->SetCollisionResponseToChannel
	(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

	if (GetMesh())
	{
		GetMesh()->SetCollisionObjectType(ECollisionChannel_SkeletalMesh);

		GetMesh()->SetCollisionResponseToChannel
		(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
		GetMesh()->SetCollisionResponseToChannel
		(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

		grenadeInHand = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Grenade Cosmetic"));
		grenadeInHand->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		grenadeInHand->SetupAttachment(GetMesh(), GRENADE_SOCKET);
	}
 
	
	dissolveTimelineComponent = CreateDefaultSubobject<UTimelineComponent>(TEXT("Dissolve Timeline"));


	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;

	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;


	/*
	* Server Side Rewind Boxes
	* 
	*/

	CreateAllSSRB();
}





void APlayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(APlayerCharacter, overlappingWeapon, COND_OwnerOnly);
	//DOREPLIFETIME(APlayerCharacter, overlappingWeapon);
	DOREPLIFETIME(APlayerCharacter, currentHealth);
	DOREPLIFETIME(APlayerCharacter, currentShield);
	DOREPLIFETIME(APlayerCharacter, playerDisplayName);

	// cannot be owner only because proxies would still turn to controller rotation
	// because they don't know about this variable
	DOREPLIFETIME(APlayerCharacter, bGameplayDisabled);
}

void APlayerCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (combatComponent) 
	{
		combatComponent->ownerPlayer = this;
		combatComponent->playerMovementComp = GetCharacterMovement();

		if (lagCompensationComponent)
		{
			combatComponent->lagComp = lagCompensationComponent;
		}
	}

	if (buffComponent)
	{
		buffComponent->ownerPlayer = this;
		buffComponent->SpawnSpeedUp();
		buffComponent->SpawnDamageReduction();
	}

	if (lagCompensationComponent)
	{
		lagCompensationComponent->ownerPlayer = this;

		if (combatComponent)
		{
			lagCompensationComponent->ownerCombatComponent = combatComponent;
		}

		if (playerController)
		{
			lagCompensationComponent->ownerPlayerController = playerController;
		}
	}
}



void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	if(GetMesh()) animInstance = GetMesh()->GetAnimInstance();

	ShowGrenade(false);

	playerController = Cast<AMainPlayerController>(Controller);

	characterMovement = GetCharacterMovement();

	ignoreSelfArray.Add(this);


	if (HasAuthority())
	{
		// does not update health for clients in begin play for some reason
		SetHealth(maxHealth);

		if (buffComponent)
		{
			buffComponent->ownerPlayer = this;
			buffComponent->SpawnSpeedUp();
			buffComponent->SpawnDamageReduction();
		}

#if WITH_EDITOR
		if (bDEBUG_Invincible)
		{
			maxHealth = 1000000.f;
			SetHealth(1000000.f);
		}
#endif
		
		OnTakeAnyDamage.AddDynamic(this, &APlayerCharacter::ReceiveDamage);

		if (!playerController) playerController = Cast<AMainPlayerController>(Controller);
		if (IsLocallyControlled() && playerController)
		{
			playerController->SetHUDAmmo(HIDE_AMMO_AMOUNT);
		}

		
	}
	else
	{
		if (!playerController) playerController = Cast<AMainPlayerController>(Controller);
		if (IsLocallyControlled() && playerController)
		{
			playerController->SetHUDHealth(maxHealth, maxHealth);
			playerController->SetHUDAmmo(HIDE_AMMO_AMOUNT);
		}
	}


	

}


void APlayerCharacter::OnRep_playerDisplayName()
{
	SetDisplayName(playerDisplayName);
}

void APlayerCharacter::ShowGrenade(bool bShow)
{
	if (!grenadeInHand) { return; }
	if (!GetMesh()) { return; }

	if (bShow)
	{
		if (GetMesh()->GetVisibleFlag())
		{
			grenadeInHand->SetVisibility(true);
		}
	}
	else
	{
		grenadeInHand->SetVisibility(false);
	}

	
}

void APlayerCharacter::SetCrosshairDrawLocation(const FVector2D& location)
{
	if (!playerController) playerController = Cast<AMainPlayerController>(Controller);
	if (playerController)
	{
		playerController->SetHUDCrosshairDrawLocation(location);
	}
}

void APlayerCharacter::AddShield(float amount)
{
	if(!HasAuthority())
	{
		return;
	}

	SetShield(currentShield + amount);
}

void APlayerCharacter::OnJumped_Implementation()
{
	combatComponent->bCanJump = false;
	Super::OnJumped_Implementation();
}

void APlayerCharacter::FootstepSound()
{
	HandleFootstepSound(1.f);
}



// BP
void APlayerCharacter::FootstepSoundLight()
{
	HandleFootstepSound(0.5f);
}

void APlayerCharacter::FootstepLand()
{
	if (FootstepSphereTrace())
	{
		if (footstepHitResult.PhysMaterial.IsValid())
		{
			switch (footstepHitResult.PhysMaterial.Get()->SurfaceType)
			{
				// grass
			case EPhysicalSurface::SurfaceType1:
				currentFootstepSound = grassLandSound;
				break;
				// stone
			case EPhysicalSurface::SurfaceType2:
				currentFootstepSound = stoneLandSound;
				break;
				// metal
			case EPhysicalSurface::SurfaceType3:
				currentFootstepSound = metalLandSound;
				break;
			default:
				currentFootstepSound = stoneLandSound;
				break;
			}

			PlayCurrentFootstepSound(1.f);
		}
	}
}



// BP
bool APlayerCharacter::FootstepSphereTrace()
{
	return UKismetSystemLibrary::SphereTraceSingle(this, GetCapsuleBottomPosition() - footstepSphereTraceOffset,
		GetCapsuleBottomPosition() - footstepSphereTraceOffset,
		footstepSphereTraceRadius, ETraceTypeQuery::TraceTypeQuery1,
		false, ignoreSelfArray, EDrawDebugTrace::None,
		footstepHitResult, true, FLinearColor::Red, FLinearColor::Green, 1.f);
}



void APlayerCharacter::HandleFootstepSound(const float& vol_)
{
	// 1 grass
	// 2 stone
	// 3 metal

	if (FootstepSphereTrace())
	{
		if (footstepHitResult.PhysMaterial.IsValid())
		{
			switch (footstepHitResult.PhysMaterial.Get()->SurfaceType)
			{
				// grass
			case EPhysicalSurface::SurfaceType1:
				currentFootstepSound = grassFootstepSound;
				break;
				// stone
			case EPhysicalSurface::SurfaceType2:
				currentFootstepSound = stoneFootstepSound;
				break;
				// metal
			case EPhysicalSurface::SurfaceType3:
				currentFootstepSound = metalFootstepSound;
				break;
			default:
				currentFootstepSound = stoneFootstepSound;
				break;
			}

			PlayCurrentFootstepSound(vol_);
		}
	}
}

void APlayerCharacter::PlayCurrentFootstepSound(const float& vol_)
{
	if (currentFootstepSound)
		UGameplayStatics::PlaySoundAtLocation(this, currentFootstepSound, GetCapsuleBottomPosition(), vol_);
}







void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bGameplayDisabled) { return; }

	RotateInPlace(DeltaTime);
	PollInit();

	if (IsLocallyControlled())
	{
		InterpSpringArm(DeltaTime);
		Perching();

		if (bJumpBuffered)
		{
			bJumpBuffered = false;
			Jump();
		}
	}

	if (HasAuthority())
	{
		if (lastInstigator != nullptr)
		{
			lastInstigatorRunningTime += DeltaTime;

			if (lastInstigatorRunningTime > lastInstigatorKeepTime)
			{
				lastInstigator = (AController*)nullptr;
				lastInstigatorRunningTime = 0.f;
			}
		}
		else // just in case a player exits, and lastInstigator becomes a nullptr without resetting timer
		{
			lastInstigatorRunningTime = 0.f;
		}

		
	}
	
	FallDamage();

#if WITH_EDITOR
	// FOR DEBUG



	if(IsLocallyControlled())
	{
		if(bDEBUG_KeepJumping)
		Jump();

		if (bDEBUG_KeepStrafing)
		{
			const FRotator controllerYawRot = FRotator{ 0.f, GetControlRotation().Yaw, 0.f };
			const FVector controllerRightDirection = FRotationMatrix(controllerYawRot).GetUnitAxis(EAxis::Y);

			const  float currentX = GetActorLocation().X;
			if (bStrafesRight)
			{
				if (currentX > -500.f)
				{
					AddMovementInput(-FVector::ForwardVector, 1.f);
				}
				else
				{
					bStrafesRight = false;
					bStrafesLeft = true;
				}
			}
			else if (bStrafesLeft)
			{
				if (currentX < 500.f)
				{
					AddMovementInput(FVector::ForwardVector, 1.f);
				}
				else
				{
					bStrafesLeft = false;
					bStrafesRight = true;
				}
			}
		}
	}


#endif

}


void APlayerCharacter::Perching()
{
	if (!characterMovement) {return;}
	
	if (characterMovement->IsFalling() || !characterMovement->IsCrouching())
	{
		DisablePerching();
	}
	else
	{
		GroundCheckPerching();
	}
}



void APlayerCharacter::GroundCheckPerching()
{
	if (GetWorld() && !bPerchingEnabled)
	{
	//	GEPR_ONE_VARIABLE("Enabled: %d", bPerchingEnabled);

		FHitResult perchHit{};
		FVector capsuleBottom = GetCapsuleBottomPosition();
		if (GetWorld()->LineTraceSingleByChannel(perchHit, capsuleBottom,
			capsuleBottom + (FVector::DownVector * 100.f), ECollisionChannel::ECC_Visibility))
		{
			CheckEnablePerching(perchHit);
		}


		//DRAW_POINT(capsuleBottom + (FVector::DownVector * 30.f));

	}
}

void APlayerCharacter::Landed(const FHitResult& Hit)
{
	bLanded = true;
	lastLandedHit = Hit;
	Super::Landed(Hit);
}

void APlayerCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);

	if (!bLanded) { return; }
	if (!characterMovement) { return; }
	if (characterMovement->IsCrouching() && !characterMovement->IsFalling())
	{
		CheckEnablePerching(lastLandedHit);
		bLanded = false;
	}
	
}

void APlayerCharacter::SetTeamColor(ETeams team)
{

	SetTeamForCrosshair(team);

	if (!baseMaterialInstance_NoTeam) { return; }
	if (!baseMaterialInstance_BlueTeam) { return; }
	if (!baseMaterialInstance_RedTeam) { return; }

	if (!dissolveMaterialInstance_NoTeam) { return; }
	if (!dissolveMaterialInstance_BlueTeam) { return; }
	if (!dissolveMaterialInstance_RedTeam) { return; }

	if (!GetMesh()) { return; }

	switch (team)
	{
	case ETeams::ET_NoTeam:
		baseMaterialInstanceDynamic = UMaterialInstanceDynamic::Create(baseMaterialInstance_NoTeam, this);


		GetMesh()->SetMaterial(0, baseMaterialInstanceDynamic);
		dissolveMaterialInstance = dissolveMaterialInstance_NoTeam;
		break;
	case ETeams::ET_BlueTeam:
		baseMaterialInstanceDynamic = UMaterialInstanceDynamic::Create(baseMaterialInstance_BlueTeam, this);


		GetMesh()->SetMaterial(0, baseMaterialInstanceDynamic);
		dissolveMaterialInstance = dissolveMaterialInstance_BlueTeam;
		break;
	case ETeams::ET_RedTeam:
		baseMaterialInstanceDynamic = UMaterialInstanceDynamic::Create(baseMaterialInstance_RedTeam, this);


		GetMesh()->SetMaterial(0, baseMaterialInstanceDynamic);
		dissolveMaterialInstance = dissolveMaterialInstance_RedTeam;
		break;
	default:
		baseMaterialInstanceDynamic = UMaterialInstanceDynamic::Create(baseMaterialInstance_NoTeam, this);


		GetMesh()->SetMaterial(0, baseMaterialInstanceDynamic);
		dissolveMaterialInstance = dissolveMaterialInstance_NoTeam;
		break;
	}
}

void APlayerCharacter::CheckEnablePerching(const FHitResult& Hit, bool bReversed)
{
	if (characterMovement)
	{
		if (Hit.ImpactPoint.Z > GetCapsuleBottomPosition().Z + 0.5f)
		{
			DisablePerching();
			return;
		}

		FFindFloorResult findPerch = {};
		characterMovement->ComputePerchResult(perchWhileNotInAir, Hit, 200.f, findPerch);



		if (!findPerch.bWalkableFloor)
		{
			if (bReversed) DisablePerching();
			else EnablePerching();
			
		}
		else
		{
			if(bReversed) EnablePerching();
			else DisablePerching();
		}
	}
}

void APlayerCharacter::EnablePerching()
{
	characterMovement->PerchRadiusThreshold = perchWhileNotInAir;
	bPerchingEnabled = true;

	Server_ClientChangedPerching(bPerchingEnabled);
}

void APlayerCharacter::DisablePerching()
{
	characterMovement->PerchRadiusThreshold = 0;
	bPerchingEnabled = false;

	Server_ClientChangedPerching(bPerchingEnabled);
}

void APlayerCharacter::Server_ClientChangedPerching_Implementation(bool bEnabled)
{
	Multicast_ClientChangedPerching(bEnabled);
}

void APlayerCharacter::Multicast_ClientChangedPerching_Implementation(bool bEnabled)
{
	if (!IsLocallyControlled())
	{
		if (!characterMovement) characterMovement = GetCharacterMovement();
		if (!characterMovement) { return; }

		if (bEnabled)
		{
			characterMovement->PerchRadiusThreshold = perchWhileNotInAir;
		}
		else
		{
			characterMovement->PerchRadiusThreshold = 0;
		}
	}
}

FVector APlayerCharacter::GetCapsuleBottomPosition()
{
	FVector feetPos{};
	if (GetCapsuleComponent())
	{
		feetPos = GetActorLocation() - FVector{ 0.f, 0.f,
			GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - 5.f};
	}

	return feetPos;
}



void APlayerCharacter::RotateInPlace(float DeltaTime)
{
	if (GetLocalRole() > ENetRole::ROLE_SimulatedProxy && IsLocallyControlled())
	{
		AimOffset(DeltaTime);
	}
	else
	{
		//SimulatedProxiesTurn();
		timeSinceLastSimuledProxyTurn += DeltaTime;
		if (timeSinceLastSimuledProxyTurn > 0.25f)
		{
			OnRep_ReplicatedMovement();
		}
		CalculateAO_Pitch();
	}
}


void APlayerCharacter::OnRep_currentShield()
{
	UpdateHUD_Shield();
}

void APlayerCharacter::UpdateHUD_Shield()
{
	if (!playerController) playerController = Cast<AMainPlayerController>(Controller);
	if (playerController)
	{
		playerController->SetHUDShield(currentShield, maxShield);
	}
}


void APlayerCharacter::SetShield(float shield)
{
	if (IsDeadOrGameplayDisabled()) { return; }

	shield = FMath::Clamp(shield, 0, maxShield);
	currentShield = shield;

	UpdateHUD_Shield();
}



void APlayerCharacter::UpdateHUD_Health()
{
	if (!playerController) playerController = Cast<AMainPlayerController>(Controller);
	if (playerController)
	{
		playerController->SetHUDHealth(currentHealth, maxHealth);
	}
}

void APlayerCharacter::AddToHealth(float health)
{
	if (IsDeadOrGameplayDisabled()) { return; }

	currentHealth = FMath::Clamp(currentHealth + health, 0, maxHealth);

	UpdateHUD_Health();
}

// Client
void APlayerCharacter::OnRep_currentHealth(float lastHealth)
{
	if (!playerController) playerController = Cast<AMainPlayerController>(Controller);
	if (IsLocallyControlled() && playerController)
	{
		playerController->SetHUDHealth(currentHealth, maxHealth);
	}

	if (currentHealth < lastHealth)
	{
		PlayHitReactMontage();
	}

}





bool APlayerCharacter::TeamsEqual(AActor* DamagedActor, AController* InstigatorController)
{
	if(!DamagedActor) return false;
	if (!InstigatorController) return false;


	APlayerCharacter* damagedPlayer = Cast<APlayerCharacter>(DamagedActor);
	if (!damagedPlayer) { return false; }

	AMainPlayerState* damagedPlayerState = damagedPlayer->GetPlayerState<AMainPlayerState>();
	if (!damagedPlayerState) { return false; }
	if (damagedPlayerState->GetTeam() == ETeams::ET_NoTeam) { return false; }

	AMainPlayerState* instigatorPlayerState = InstigatorController->GetPlayerState<AMainPlayerState>();
	if (!instigatorPlayerState) { return false; }
	if (instigatorPlayerState->GetTeam() == ETeams::ET_NoTeam) { return false; }


	if (damagedPlayerState->GetTeam() == instigatorPlayerState->GetTeam())
	{
		return true;
	}

	return false;
}




void APlayerCharacter::CalculateDamage(float& damage, float& outNewHealth, float& outNewShield)
{
	if (buffComponent && buffComponent->CurrenthyHasDamageReduction())
	{
		damage *= (1 - buffComponent->GetDamageReduction());
	}

	if (currentShield <= 0)
	{
		// will be clamped in set health
		outNewHealth = currentHealth - damage;
		outNewShield = 0;
		return;
	}

	float shieldTryAbsorb = damage * shieldDamageAbsorbRatio;
	float damageToHealth = damage * (1 - shieldDamageAbsorbRatio);

	if (shieldTryAbsorb > currentShield)
	{
		outNewShield = 0;
		shieldTryAbsorb -= currentShield;
		damageToHealth += shieldTryAbsorb;
	}
	else
	{
		outNewShield = currentShield - shieldTryAbsorb;
	}

	outNewHealth = currentHealth - damageToHealth;
}


// Only runs on server
void APlayerCharacter::ReceiveDamage(
	AActor* DamagedActor, float Damage,
	const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser)
{
	if (TeamsEqual(DamagedActor, InstigatorController)
		&& InstigatorController && InstigatorController != GetController())
	{
		return;
	}


	// last instigator shouldn't be self
	if (InstigatorController && InstigatorController != GetController())
	{
		lastInstigator = InstigatorController;
	}
	else if (lastInstigator)
	{
		InstigatorController = lastInstigator;
	}



	const UHeadshotDamageType* damageType = Cast<UHeadshotDamageType>(DamageType);
	const bool bHeadshot = damageType != nullptr;

	float newHealth{};
	float newShield{};
	CalculateDamage(Damage, newHealth, newShield);
	//OnScreenDebugMessages(DamageCauser, InstigatorController, Damage);

	// currentShield is not changed at this point, it's just calculated into newShield

	if (DamagedActor)
	{
		if (DamagedActor->GetInstigatorController() != InstigatorController)
		{
			float actualDamageDone = Damage;
			if (newHealth <= 0.f)
			{
				actualDamageDone -= FMath::Abs(newHealth);
			}

			InstigatorHUDHitFeedback(InstigatorController, newHealth, currentShield, actualDamageDone);
		}

		VictimHUDHitFeedback(DamagedActor, DamageCauser);
	}

	
	SetShield(newShield);
	SetHealth(newHealth, InstigatorController, DamageCauser, bHeadshot);
}




// Server
void APlayerCharacter::SetHealth
(float health, AController* instigatorController, AActor* damageCauser, bool bHeadshot)
{

	if (IsDeadOrGameplayDisabled()) { return; }

	health = FMath::Clamp(health, 0, maxHealth);
	currentHealth = health;
	UpdateHUD_Health();

	// we are probably not DEALT damage, maybe the environment?
	//if (!causer) { return; }

	PlayHitReactMontage();

	if (buffComponent)
	{
		buffComponent->StopHealing();
	}

	if (currentHealth > 0.f) { return; }



	// Handle Death
	if (!GetWorld()) { return; }
	// Can only get this on server, we only call this from server
	if (!mainGameMode) mainGameMode = Cast<AMainGameMode>(GetWorld()->GetAuthGameMode());
	if (mainGameMode)
	{
		if (!playerController) playerController = Cast<AMainPlayerController>(Controller);

		mainGameMode->PlayerEliminated(
			this,
			playerController,
			Cast<AMainPlayerController>(instigatorController), damageCauser, bHeadshot);
	}
}


void APlayerCharacter::OnScreenDebugMessages(AActor* DamageCauser, AController* InstigatorController, float Damage)
{
	if (HasAuthority())
	{
		AMainPlayerState* playerState = GetPlayerState<AMainPlayerState>();
		if (playerState)
		{
			FString damageMessage
			{ FString::Printf(TEXT("%s received %.2f damage caused by "), *playerState->GetPlayerName(), Damage)};

			if (InstigatorController)
			{
				AMainPlayerState* causerPlayerState = InstigatorController->GetPlayerState<AMainPlayerState>();
				if (causerPlayerState)
				{
					damageMessage.Append(FString::Printf(TEXT("%s"), *causerPlayerState->GetPlayerName()));
				}
			}
			else
			{
				damageMessage.Append(FString{ TEXT("the environment") });
			}

			if (DamageCauser)
			{
				if (IInstigatorTypeInterface* instigatorType = Cast< IInstigatorTypeInterface>(DamageCauser))
				{
					damageMessage.Append(FString::Printf
					(TEXT(" using a(n) %s"), *instigatorType->GetInstigatorTypeName().ToString()));
				}
			}

#if WITH_EDITOR
			GEPR_Y(*damageMessage);
#else
			MulticastOnScreenDebugMessage(damageMessage);
#endif
		}
	}
}

void APlayerCharacter::MulticastOnScreenDebugMessage_Implementation(const FString& message)
{
	GEPR_Y(*message);
}

void APlayerCharacter::InstigatorHUDHitFeedback
(AController* InstigatorController, float newHealth, float shieldBeforeHit, const float& actualDamageDone)
{
	//FString message = FString::Printf(TEXT("New health: %.2f"), newHealth);
	//GEPR_R(*message);

	AMainPlayerController* enemyController = Cast<AMainPlayerController>(InstigatorController);
	if (enemyController && InstigatorController != Controller)
	{
		bool bHitShield{false};
		bool bWillKill{false};

		if (newHealth <= 0.f)
		{
			// will kill
			bWillKill = true;
		}
		if (shieldBeforeHit > 0.f)
		{
			// hit shield
			bHitShield = true;
		}


		enemyController->ClientHUDPlayHitEffect(bHitShield, bWillKill, actualDamageDone);
	}
}

void APlayerCharacter::VictimHUDHitFeedback(AActor* DamagedActor, AActor* InstigatorActor)
{
	if (!DamagedActor) { return; }

	if (AMainPlayerController* damagedActorController =
		DamagedActor->GetInstigatorController<AMainPlayerController>())
	{
		damagedActorController->ClientHUDPlayVictimHitEffect(InstigatorActor);
	}

}



// Called from GameMode after we notified it about our death
void APlayerCharacter::ServerPlayerEliminated(bool bLeftGame_)
{

	if (HasWeaponEquipped())
	{
		combatComponent->DropEquippedWeapon();
	}

	if (HasSecondaryWeapon())
	{
		combatComponent->DropSecondaryWeapon();
	}

	

	MulticastPlayerEliminated(bLeftGame_);
	
}


void APlayerCharacter::MulticastPlayerEliminated_Implementation(bool bLeftGame_)
{
	bLeftGame = bLeftGame_;

	bIsDead = true;
	PlayDeathMontage();
	DisableInputAndCollision();
	DissolveEffect();
	ShowSniperScopeWidget(false, false);

	if (grenadeInHand)
	{
		grenadeInHand->SetVisibility(false);
	}

	if (crownNiagaraSystemComponent)
	{
		crownNiagaraSystemComponent->DestroyComponent();
	}

	if (!playerController) playerController = Cast<AMainPlayerController>(Controller);
	if (playerController) { playerController->SetHUDAmmo(HIDE_AMMO_AMOUNT); }

	GetWorldTimerManager().SetTimer(respawnTimerhandle, this, &APlayerCharacter::RespawnTimerEnded, respawnDelayTime);
}

void APlayerCharacter::DisableInputAndCollision()
{
	// Disable Movement
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->DisableMovement();
		GetCharacterMovement()->StopMovementImmediately();
	}

	if (combatComponent)
	{
		combatComponent->DisableGameplay();
	}

	// Disable Input
	if (playerController)
	{
		//DisableInput(playerController);
		bGameplayDisabled = true;
	}

	// Disable Collision On Capsule
	if (GetCapsuleComponent())
	{
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// Disable Collision On Mesh
	if (GetMesh())
	{
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}





void APlayerCharacter::RespawnTimerEnded()
{
	if (!GetWorld()) { return; }
	if (!mainGameMode) { Cast<AMainGameMode>(GetWorld()->GetAuthGameMode()); }
	if (mainGameMode)
	{
		if(Controller && !bLeftGame)
		mainGameMode->RequestRespawn(this, Controller);
	}

	if (bLeftGame && IsLocallyControlled())
	{
		OnPlayerLeftGameDelegate.Broadcast();
	}
}




void APlayerCharacter::ServerLeaveGame_Implementation()
{
	//GEPRS_Y("ServerLeaveGame");
	if (!GetWorld()) { return; }
	if (!mainGameMode) { mainGameMode = Cast<AMainGameMode>(GetWorld()->GetAuthGameMode()); }
	if (mainGameMode)
	{
		//GEPRS_Y("ServerLeaveGame: GameModeExists");

		if (AMainPlayerState* playerState = GetPlayerState<AMainPlayerState>())
		{
			mainGameMode->PlayerLeftGame(playerState);
		}
	}
}









void APlayerCharacter::AimOffset(float DeltaTime)
{
	if (!HasWeaponEquipped()) { return; }

	

	CalculateSpeed();
	if(GetCharacterMovement()) bIsInAir_ = GetCharacterMovement()->IsFalling();
	
	if (speed_ == 0.f && !bIsInAir_) // standing still, not jumping
	{
		bRotateRootBone = true;
		currentAimRotation_ = FRotator{ 0.f, GetBaseAimRotation().Yaw, 0.f };
		// Need to change up the two rotators, because this results in a reversed number (left is right, right is left)
		// deltaAimRotation_ = UKismetMathLibrary::NormalizedDeltaRotator(baseAimRotation_, currentAimRotation_);
		deltaAimRotation_ = UKismetMathLibrary::NormalizedDeltaRotator(currentAimRotation_, baseAimRotation_);
		AO_Yaw = deltaAimRotation_.Yaw;
		if (turningInPlaceState == ETurningInPlace::ETIP_NotTurning)
		{
			InterpAO_Yaw = AO_Yaw;
		}
		//bUseControllerRotationYaw = false;
		bUseControllerRotationYaw = true;
		TurnInPlace(DeltaTime);

		
	}
	else
	{
		bRotateRootBone = false;
		baseAimRotation_ = FRotator{ 0.f, GetBaseAimRotation().Yaw, 0.f };
		AO_Yaw = 0.f; // We don't want yaw in the aim while we are not standing still
		bUseControllerRotationYaw = true;
		turningInPlaceState = ETurningInPlace::ETIP_NotTurning;
	}

	/* This needs to be corrected because unreal engine compresses to unsigned
	   Rotation values before sending on the network so they take less bandwidth 
	*/
	CalculateAO_Pitch();

}

void APlayerCharacter::CalculateAO_Pitch()
{
	AO_Pitch = GetBaseAimRotation().Pitch;
	if (AO_Pitch > 90.f && !IsLocallyControlled()) // This data is from another machine
	{
		// map pitch from [270 - 360) to [-90, 0)
		FVector2D pitchInRange{ 270.f, 360.f };
		FVector2D pitchOutRange{ -90.f, 0.f };
		AO_Pitch = FMath::GetMappedRangeValueClamped(pitchInRange, pitchOutRange, AO_Pitch);
	}
}

void APlayerCharacter::TurnInPlace(float DeltaTime)
{
	if (AO_Yaw > 90.f)
	{
		turningInPlaceState = ETurningInPlace::ETIP_Right;
	}
	else if (AO_Yaw < -90.f)
	{
		turningInPlaceState = ETurningInPlace::ETIP_Left;
	}
	
	if (turningInPlaceState != ETurningInPlace::ETIP_NotTurning) // We are turning
	{
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0, DeltaTime, 14.f);
		AO_Yaw = InterpAO_Yaw;
		if (FMath::Abs(AO_Yaw) < 5.f)
		{
			turningInPlaceState = ETurningInPlace::ETIP_NotTurning;
			baseAimRotation_ = FRotator{ 0.f, GetBaseAimRotation().Yaw, 0.f };
		}
	}
}

void APlayerCharacter::SwitchCameraSide()
{
	bSpringArmIsRight = !bSpringArmIsRight;
}

void APlayerCharacter::InterpSpringArm(float DeltaTime)
{
	if (!characterMovement) characterMovement = GetCharacterMovement();
	if (!characterMovement) { return; }

	springArmInterpTo_ = bSpringArmIsRight ? springArmOffset_Right : springArmOffset_Left;
	springArmInterpTo_.Z = !characterMovement->IsCrouching() ? springArmStandingZ : springArmCrouchingZ;

	springArmComponent->SocketOffset = 
		FMath::VInterpTo(springArmComponent->SocketOffset, springArmInterpTo_, DeltaTime, springArmInterpSeed);
}

void APlayerCharacter::SimulatedProxiesTurn()
{
	// prevent calls that are not created here
	if (GetLocalRole() > ENetRole::ROLE_SimulatedProxy && IsLocallyControlled()) { return; }

	bRotateRootBone = false;

	if (!HasWeaponEquipped()) { return; }

	CalculateSpeed();

	if (speed_ > 0.f) 
	{
		turningInPlaceState = ETurningInPlace::ETIP_NotTurning;
		return;
	}

	
	

	proxyLastFrameRotation = proxyThisFrameRotation;
	proxyThisFrameRotation = GetActorRotation();

	proxyYawDifference = UKismetMathLibrary::NormalizedDeltaRotator(proxyLastFrameRotation, proxyThisFrameRotation).Yaw;
	
	if (FMath::Abs(proxyYawDifference) > proxyTurnThreshold)
	{
		if (proxyYawDifference > proxyTurnThreshold)
		{
			turningInPlaceState = ETurningInPlace::ETIP_Right;
		}
		else if(proxyYawDifference < -proxyTurnThreshold)
		{
			turningInPlaceState = ETurningInPlace::ETIP_Left;
		}
		else // should never reach this
		{
			turningInPlaceState = ETurningInPlace::ETIP_NotTurning;
		}
		return;
	}

	turningInPlaceState = ETurningInPlace::ETIP_NotTurning;

}

void APlayerCharacter::CalculateSpeed()
{
	velocity_ = GetVelocity();
	velocity_.Z = 0.f;
	speed_ = velocity_.Size();
}


void APlayerCharacter::OnRep_ReplicatedMovement()
{
	Super::OnRep_ReplicatedMovement();

	SimulatedProxiesTurn();
	timeSinceLastSimuledProxyTurn = 0.f;
}






void APlayerCharacter::PlayFireMontage(bool bAiming)
{
	if (!fireWeaponMontage) { return; }
	if (!HasWeaponEquipped()) { return; }
	if (!animInstance) { return; }

	animInstance->Montage_Play(fireWeaponMontage);

	if (bAiming) animInstance->Montage_JumpToSection(FireWeaponMontageSection_RifleAim);
	else animInstance->Montage_JumpToSection(FireWeaponMontageSection_RifleHip);
}

void APlayerCharacter::PlayHitReactMontage()
{
	if (!IsUnoccupied()) { return; }
	if (!hitReactMontage) { return; }
	if (!HasWeaponEquipped()) { return; }
	if (!animInstance) { return; }



	int32 randomHitReactNum = FMath::RandRange(0, 3);
	FName sectionName{};
	switch (randomHitReactNum)
	{
	case 0:
		sectionName = FName{ "FromFront" };
		break;
	case 1:
		sectionName = FName{ "FromBack" };
		break;
	case 2:
		sectionName = FName{ "FromLeft" };
		break;
	case 3:
		sectionName = FName{ "FromRight" };
		break;
	default:
		sectionName = FName{ "FromFront" };
		break;
	}



	animInstance->Montage_Play(hitReactMontage);
	animInstance->Montage_JumpToSection(sectionName, hitReactMontage);
}

void APlayerCharacter::PlayDeathMontage()
{
	if (!deathMontage) { return; }
	if (!animInstance) { return; }

	animInstance->Montage_Play(deathMontage);
}

void APlayerCharacter::PlayReloadMontage()
{
	if (!reloadMontage) { return; }
	if (!animInstance) { return; }
	if (!HasWeaponEquipped()) { return; }

	FName sectionName{AWeapon::WeaponReloadTypeAsFName(combatComponent->equippedWeapon->GetWeaponReloadAnimationType())};

	animInstance->Montage_Play(reloadMontage);
	animInstance->Montage_JumpToSection(sectionName, reloadMontage);
}

void APlayerCharacter::PlayThrowGrenadeMontage()
{
	if (!throwGrenadeMontage) { return; }
	if (!animInstance) { return; }

	animInstance->Montage_Play(throwGrenadeMontage);

}

void APlayerCharacter::PlaySwapWeaponsMontage()
{
	if (!swapWeaponsMontage) { return; }
	if (!animInstance) { return; }

	animInstance->Montage_Play(swapWeaponsMontage);
}

void APlayerCharacter::JumpToShotgunMontageEnd()
{
	if (!reloadMontage) { return; }
	if (!animInstance) { return; }
	if (!HasWeaponEquipped()) { return; }


	animInstance->Montage_JumpToSection(WeaponReloadTypeName::SHOTGUN_RELOAD_END, reloadMontage);
}

void APlayerCharacter::HideCharacterAndWeapon(const bool& bHide)
{
	if (!GetMesh()) { return; }
	if (!HasWeaponEquipped()) { return; }
	if (!GetEquippedWeapon()->GetWeaponMesh()) { return; }


	if (bHide)
	{
		GetMesh()->SetVisibility(false);
		GetEquippedWeapon()->GetWeaponMesh()->bOwnerNoSee = true;
		ShowGrenade(false);

		if (GetSecondaryWeapon() && GetSecondaryWeapon()->GetWeaponMesh())
		{
			GetSecondaryWeapon()->GetWeaponMesh()->bOwnerNoSee = true;
		}
	}
	else
	{
		GetMesh()->SetVisibility(true);
		GetEquippedWeapon()->GetWeaponMesh()->bOwnerNoSee = false;

		if (GetSecondaryWeapon() && GetSecondaryWeapon()->GetWeaponMesh())
		{
			GetSecondaryWeapon()->GetWeaponMesh()->bOwnerNoSee = false;
		}
	}
}







void APlayerCharacter::MoveForward(float axisValue)
{
	if (bGameplayDisabled) { return; }

	if (!Controller || axisValue == 0.f) { return; }

	const FRotator controllerYawRot = FRotator{ 0.f, GetControlRotation().Yaw, 0.f };
	const FVector controllerForwardDirection = FRotationMatrix(controllerYawRot).GetUnitAxis(EAxis::X);
	AddMovementInput(controllerForwardDirection, axisValue);
}

void APlayerCharacter::MoveRight(float axisValue)
{
	if (bGameplayDisabled) { return; }

	if (!Controller || axisValue == 0.f) { return; }

	const FRotator controllerYawRot = FRotator{ 0.f, GetControlRotation().Yaw, 0.f };
	const FVector controllerRightDirection = FRotationMatrix(controllerYawRot).GetUnitAxis(EAxis::Y);
	AddMovementInput(controllerRightDirection, axisValue);
}

void APlayerCharacter::Turn(float axisValue)
{
	if (!Controller || axisValue == 0.f) { return; }

	AddControllerYawInput(axisValue * mouseSensitivity);
}

void APlayerCharacter::LookUp(float axisValue)
{
	if (!Controller || axisValue == 0.f) { return; }

	AddControllerPitchInput(axisValue * mouseSensitivity);
}

void APlayerCharacter::InteractButtonPressed()
{
	if (bGameplayDisabled) { return; }

	bInteractPressed = true;
	GetWorldTimerManager().SetTimer
	(interactButtonPressTimer, this, &ThisClass::InteractButtonCheckHold, interactButtonPressTimeConsideredHeld);
}

void APlayerCharacter::InteractButtonShortPressed()
{
	if (IsDeadOrGameplayDisabled()) { return; }

	ServerInteractButtonPressed();
}

void APlayerCharacter::InteractButtonHeld()
{
	if (IsDeadOrGameplayDisabled()) { return; }

	ServerInteractButtonPressed(true);
}

void APlayerCharacter::InteractButtonCheckHold()
{
	if (IsDeadOrGameplayDisabled()) { return; }

	if (bInteractPressed)
	{
		InteractButtonHeld();
	}

	bInteractPressed = false;
	bInteractHeld = false;
}

void APlayerCharacter::InteractButtonReleased()
{
	if (IsDeadOrGameplayDisabled() || !bInteractPressed) { return; }

	InteractButtonShortPressed();

	bInteractPressed = false;
	bInteractHeld = false;
}



void APlayerCharacter::ServerInteractButtonPressed_Implementation(bool bButtonHeld)
{
	if (combatComponent)
	{
		combatComponent->EquipWeapon(overlappingWeapon, bButtonHeld);
	}
}


// These functions are already set up for multiplayer
void APlayerCharacter::CrouchButtonPressed()
{
	if (bGameplayDisabled) { return; }

	if (!bIsCrouched)
	{
		Crouch();
	}
	
}

void APlayerCharacter::CrouchButtonReleased()
{
	if (bGameplayDisabled) { return; }

	if (bIsCrouched)
	{
		UnCrouch();
	}
}

void APlayerCharacter::AimButtonPressed()
{
	if (bGameplayDisabled) { return; }

	if (!combatComponent) { return; }


	combatComponent->SetAiming(true);
}

void APlayerCharacter::AimButtonReleased()
{
	if (bGameplayDisabled) { return; }

	if (!combatComponent) { return; }

	combatComponent->SetAiming(false);
}

void APlayerCharacter::Jump()
{
	if (bGameplayDisabled) { return; }

	if (bIsCrouched)
	{
		Super::UnCrouch();
	}
	// Stop jump spam
	else if(combatComponent)
	{
		if (combatComponent->bCanJump)
		{
			Super::Jump();
			
		}
		else
		{
			if (IsLocallyControlled() && combatComponent->bCanBufferJump)
			{
				bJumpBuffered = true;
			}
		}
	}
}



void APlayerCharacter::FireButtonPressed()
{
	if (bGameplayDisabled) { return; }

	if (!combatComponent) { return; }

	combatComponent->FireButtonPressed(true);
}

void APlayerCharacter::FireButtonReleased()
{
	if (bGameplayDisabled) { return; }

	if (!combatComponent) { return; }

	combatComponent->FireButtonPressed(false);
}

void APlayerCharacter::ReloadButtonPressed()
{
	if (bGameplayDisabled) { return; }

	if (!combatComponent) { return; }
	combatComponent->ReloadButtonPressed();
}

void APlayerCharacter::ThrowGrenadeButtonPressed()
{
	if (bGameplayDisabled) { return; }

	if (!combatComponent) { return; }
	combatComponent->ThrowGrenadeButtonPressed();
}

void APlayerCharacter::SwapWeaponsButtonPressed()
{
	if (bGameplayDisabled) { return; }

	if (!combatComponent) { return; }
	combatComponent->SwapWeaponsButtonPressed();
}







void APlayerCharacter::DissolveEffect()
{
	// Dissolve Material
	if (!dissolveMaterialInstance) { return; }

	dissolveMaterialInstanceDynamic = UMaterialInstanceDynamic::Create(dissolveMaterialInstance, this);
	if (GetMesh() && dissolveMaterialInstanceDynamic)
	{
		dissolveMaterialInstanceDynamic->SetScalarParameterValue(TEXT("Dissolve"), 0.55f);
		dissolveMaterialInstanceDynamic->SetScalarParameterValue(TEXT("Glow"), 1000.f);

		GetMesh()->SetMaterial(0, dissolveMaterialInstanceDynamic);

		StartDissolve();
	}



	// Dissolve Bot (Paragon Dekker)
	if (!dissolveBotParticleSystem) { return; }

	const FVector botSpawnPoint
	{ GetActorLocation().X, GetActorLocation().Y,GetActorLocation().Z + botZ_Offset };
	dissolveBotParticleSystemComponent = UGameplayStatics::SpawnEmitterAtLocation
	(GetWorld(), dissolveBotParticleSystem, botSpawnPoint, GetActorRotation());


	//Dissolve Bot Sound Effect
	if (!dissolveBotSoundEffect) { return; }

	UGameplayStatics::PlaySoundAtLocation(this, dissolveBotSoundEffect, GetActorLocation());

}

void APlayerCharacter::Destroyed()
{
	Super::Destroyed();

	if (dissolveBotParticleSystemComponent)
	{
		dissolveBotParticleSystemComponent->DestroyComponent();
	}

	
}



void APlayerCharacter::StartDissolve()
{
	if (!dissolveCurve) { return; }

	DissolveTrack.BindDynamic(this, &ThisClass::UpdateDissolveMaterial);
	dissolveTimelineComponent->AddInterpFloat(dissolveCurve, DissolveTrack);
	dissolveTimelineComponent->Play();
}

void APlayerCharacter::UpdateDissolveMaterial(float DissolveValue)
{
	if (!dissolveMaterialInstanceDynamic) { return; }

	dissolveMaterialInstanceDynamic->SetScalarParameterValue(TEXT("Dissolve"), DissolveValue);
}






void APlayerCharacter::OnRep_overlappingWeapon(AWeapon* lastWeapon /* Value before it was replicated*/)
{
	if(lastWeapon){ lastWeapon->ShowPickupWidget(false); }
	if (overlappingWeapon) { overlappingWeapon->ShowPickupWidget(true); }
}




void APlayerCharacter::PollInit()
{
	// Player state is initialized a few frames after begin play, so we can't
	// initialize it there, instead we keep checking in tick
	if (!mainPlayerState)
	{
		mainPlayerState = GetPlayerState<AMainPlayerState>();
		//check if succesfull
		if (mainPlayerState)
		{
			mainPlayerState->UpdateHUD();
			SetTeamColor(mainPlayerState->GetTeam());
			if (HasAuthority())
			{
				playerDisplayName = mainPlayerState->GetPlayerName();
				SetDisplayName(playerDisplayName);

				AMainGameState* gameState = Cast<AMainGameState>(UGameplayStatics::GetGameState(this));
				if (mainPlayerState && gameState)
				{
					if (gameState->topScoringPlayers.Contains(mainPlayerState))
					{
						MulticastGainedLead();
					}
				}
			}
		}

		
	}
}



void APlayerCharacter::OnRep_gameplayDisabled()
{
	if(bGameplayDisabled)
	DisableGameplay();
}



void APlayerCharacter::SetOverlappingWeapon(AWeapon* weapon)
{
	// If we have an overlapping weapon at this time we either set this to another weapon
	// While we already overlap with another weapon
	// Or we exited from a weapon
	// Either way, pickup widget visibility should be set to false
	if (overlappingWeapon) { overlappingWeapon->ShowPickupWidget(false); }
	
	overlappingWeapon = weapon;

	// whether this pawn is controlled by us
	// this only runs on the server, because the delegate was only binded if we were the server
	if (IsLocallyControlled())
	{
		if (overlappingWeapon) { overlappingWeapon->ShowPickupWidget(true); }
	}
}

bool APlayerCharacter::EqualsOverlappingWeapon(AWeapon* weapon)
{
	return overlappingWeapon == weapon;
}


bool APlayerCharacter::HasWeaponEquipped() const
{
	return (combatComponent && combatComponent->equippedWeapon != nullptr);
}

bool APlayerCharacter::HasSecondaryWeapon() const
{
	return (combatComponent && combatComponent->secondaryWeapon != nullptr);
}

bool APlayerCharacter::HasShotgunEquipped() const
{
	return (combatComponent && combatComponent->equippedWeapon != nullptr && combatComponent->equippedWeapon->IsShotgun());
}

bool APlayerCharacter::IsAiming() const
{
	return (combatComponent && combatComponent->bIsAiming);
}

AWeapon* APlayerCharacter::GetEquippedWeapon() const
{
	if (!combatComponent) { return nullptr; }

	return combatComponent->equippedWeapon;
}

AWeapon* APlayerCharacter::GetSecondaryWeapon() const
{
	if (!combatComponent) { return nullptr; }
	
	return combatComponent->secondaryWeapon;
}

bool APlayerCharacter::EqualsEquippedWeapon(AWeapon* weapon) const
{
	return GetEquippedWeapon() == weapon;
}

FRotator APlayerCharacter::GetRightHandTargetRotation() const
{
	if (!combatComponent) { return FRotator(); }
		
	return combatComponent->GetRightHandTargetRotation();
	
}


ECombatState APlayerCharacter::GetCombatState()
{
	return combatComponent ? combatComponent->combatState : ECombatState::ECS_Unoccupied;
}

bool APlayerCharacter::IsReloading() const
{
	return combatComponent && combatComponent->IsReloading();
}

bool APlayerCharacter::IsUnoccupied() const
{
	return combatComponent && combatComponent->IsUnoccupied();
}

bool APlayerCharacter::CanReload() const
{
	return combatComponent && combatComponent->HasCarriedAmmo() && combatComponent->IsUnoccupied();
}

float APlayerCharacter::GetServerTime()
{
	if (playerController)
	{
		return playerController->GetServerTime();
	}

	return 0.0f;
}

float APlayerCharacter::GetSingleTripTime()
{
	if (playerController)
	{
		return playerController->GetSingleTripTime();
	}

	return 0.0f;
}

float APlayerCharacter::GetCurrentInaccuracy()
{
	if (!combatComponent) { return 1000.f; }

	return combatComponent->GetCurrentInaccuracy();
}

FVector APlayerCharacter::GetGrenadeInHandPosition()
{
	if (grenadeInHand)
	{
		return grenadeInHand->GetComponentLocation();
	}

	
	return FVector();
}

// Called from playerController
void APlayerCharacter::DisableGameplay()
{
	if (combatComponent)
	{
		combatComponent->DisableGameplay();
	}

	bGameplayDisabled = true;
	bUseControllerRotationYaw = false;
	turningInPlaceState = ETurningInPlace::ETIP_NotTurning;
	ShowSniperScopeWidget(false, false);
}



/*
* Server-Side Rewind Boxes
*/

#define CREATE_SSRB(outBox, componentName, bone)\
	if(!GetMesh()){return;}\
	outBox = CreateDefaultSubobject<class UBoxComponent>(TEXT(componentName));\
	outBox->SetupAttachment(GetMesh(), FName{TEXT(bone)}); \
	outBox->SetCollisionEnabled(ECollisionEnabled::NoCollision); \
	outBox->SetCollisionObjectType(ECollisionChannel_SSR);


#define ADD_SSRB(bone, box) SSRBoxes.Add(FName{ TEXT(bone) }, box);


void APlayerCharacter::CreateAllSSRB()
{
	CREATE_SSRB(SSRB_head, "HeadBox", "head");
	CREATE_SSRB(SSRB_pelvis, "PelvisBox", "pelvis");
	CREATE_SSRB(SSRB_spine_02, "Spine02Box", "spine_02");
	CREATE_SSRB(SSRB_spine_03, "Spine03Box", "spine_03");
	CREATE_SSRB(SSRB_upperarm_l, "UpperarmLBox", "upperarm_l");
	CREATE_SSRB(SSRB_upperarm_r, "UpperarmRBox", "upperarm_r");
	CREATE_SSRB(SSRB_lowerarm_l, "LowerarmLBox", "lowerarm_l");
	CREATE_SSRB(SSRB_lowerarm_r, "LowerarmRBox", "lowerarm_r");
	CREATE_SSRB(SSRB_hand_l, "HandLBox", "hand_l");
	CREATE_SSRB(SSRB_hand_r, "HandRBox", "hand_r");
	CREATE_SSRB(SSRB_thigh_l, "ThighLBox", "thigh_l");
	CREATE_SSRB(SSRB_thigh_r, "ThighRBox", "thigh_r");
	CREATE_SSRB(SSRB_calf_l, "CalfLBox", "calf_l");
	CREATE_SSRB(SSRB_calf_r, "CalfRBox", "calf_r");
	CREATE_SSRB(SSRB_foot_l, "FootLBox", "foot_l");
	CREATE_SSRB(SSRB_foot_r, "FootRBox", "foot_r");

	ADD_SSRB("head", SSRB_head);
	ADD_SSRB("pelvis", SSRB_pelvis);
	ADD_SSRB("spine_02", SSRB_spine_02);
	ADD_SSRB("spine_03", SSRB_spine_03);
	ADD_SSRB("upperarm_l", SSRB_upperarm_l);
	ADD_SSRB("upperarm_r", SSRB_upperarm_r);
	ADD_SSRB("lowerarm_l", SSRB_lowerarm_l);
	ADD_SSRB("lowerarm_r", SSRB_lowerarm_r);
	ADD_SSRB("hand_l", SSRB_hand_l);
	ADD_SSRB("hand_r", SSRB_hand_r);
	ADD_SSRB("thigh_l", SSRB_thigh_l);
	ADD_SSRB("thigh_r", SSRB_thigh_r);
	ADD_SSRB("calf_l", SSRB_calf_l);
	ADD_SSRB("calf_r", SSRB_calf_r);
	ADD_SSRB("foot_l", SSRB_foot_l);
	ADD_SSRB("foot_r", SSRB_foot_r);
}




void APlayerCharacter::FallDamage()
{
	if (!HasAuthority()) { return; }
	UCharacterMovementComponent* movement = GetCharacterMovement();
	if (!movement) { return; }


	if (!bInAirLastFrame)
	{
		if (!movement->IsMovingOnGround())
		{
			bInAirLastFrame = true;
		}
	}
	else
	{
		if (movement->IsMovingOnGround())
		{
			bInAirLastFrame = false;
			fallDamage_CountingZ = false;

			const float currentZ = GetActorLocation().Z;

			if (currentZ < fallDamage_WorldZWhenStartedFalling)
			{
				const float zDiff = fallDamage_WorldZWhenStartedFalling - currentZ;


				if (zDiff > fallDamage_Start_Z)
				{
					const float zDiff_subtracted = (zDiff - fallDamage_Start_Z);
					bool bNearLethalFall{};

					float fallDamageDivider{};
						
					if (zDiff_subtracted > fallDamage_ThirdDamagePhase_Z)
					{
						fallDamageDivider = fallDamageDivider_ThirdPhase;
						bNearLethalFall = true;
					}
					else if (zDiff_subtracted > fallDamage_SecondDamagePhase_Z)
					{
						fallDamageDivider = fallDamageDivider_SecondPhase;
					}
					else
					{
						fallDamageDivider = fallDamageDivider_FirstPhase;
					}
					


					float fallDamage = zDiff_subtracted / fallDamageDivider;

					if (bNearLethalFall && buffComponent && buffComponent->CurrenthyHasDamageReduction())
					{
						fallDamage *= 10.f;
					}
					
					
					UGameplayStatics::ApplyDamage(this, fallDamage, nullptr, nullptr, UDamageType::StaticClass());
					MulticastPlayFallDamageSound(fallDamage >= fallDamageHardSoundThresholdDamage ? true : false);
				}
			}
		}
	}



	if (bInAirLastFrame)
	{
		const float velZ = movement->Velocity.Z;
		if (velZ <= 0 && !fallDamage_CountingZ)
		{
			fallDamage_CountingZ = true;
			fallDamage_WorldZWhenStartedFalling = GetActorLocation().Z;
		}
	}
}


void APlayerCharacter::PlayFallDamageSound(bool bHard)
{
	if (bHard)
	{
		if (fallDamageSound_Hard)
		{
			UGameplayStatics::PlaySoundAtLocation(this, fallDamageSound_Hard, GetActorLocation());
		}
	}
	else
	{
		if (fallDamageSound_Light)
		{
			UGameplayStatics::PlaySoundAtLocation(this, fallDamageSound_Light, GetActorLocation());
		}
	}
}

void APlayerCharacter::MulticastPlayFallDamageSound_Implementation(bool bHard)
{
	PlayFallDamageSound(bHard);
}





void APlayerCharacter::MulticastGainedLead_Implementation()
{
	if (GetNetMode() == ENetMode::NM_Standalone) { return; }

	if (!crownNiagaraSystem) { return; }

	if (!crownNiagaraSystemComponent)
	{
		FVector loc = GetCapsuleBottomPosition() + FVector{0.f, 0.f, 195.f};

		crownNiagaraSystemComponent =
			UNiagaraFunctionLibrary::SpawnSystemAttached
			(crownNiagaraSystem, GetMesh(), FName{}, loc,
				GetActorRotation(), EAttachLocation::KeepWorldPosition, false);
	}

	if (crownNiagaraSystemComponent)
	{
		crownNiagaraSystemComponent->Activate();
	}
}

void APlayerCharacter::MulticastLostLead_Implementation()
{
	if (crownNiagaraSystemComponent)
	{
		crownNiagaraSystemComponent->DestroyComponent();
	}
}



void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (!PlayerInputComponent) { return; }

	PlayerInputComponent->BindAxis(FName{ "MoveForward" }, this, &APlayerCharacter::MoveForward);
	PlayerInputComponent->BindAxis(FName{ "MoveRight" }, this, &APlayerCharacter::MoveRight);
	PlayerInputComponent->BindAxis(FName{ "Turn" }, this, &APlayerCharacter::Turn);
	PlayerInputComponent->BindAxis(FName{ "LookUp" }, this, &APlayerCharacter::LookUp);

	PlayerInputComponent->BindAction(FName{ "Jump" }, IE_Pressed, this, &APlayerCharacter::Jump);


	PlayerInputComponent->BindAction(FName{ "Interact" }, IE_Pressed, this, &ThisClass::InteractButtonPressed);
	PlayerInputComponent->BindAction(FName{ "Interact" }, IE_Released, this, &ThisClass::InteractButtonReleased);


	PlayerInputComponent->BindAction(FName{ "Crouch" }, IE_Pressed, this, &ThisClass::CrouchButtonPressed);
	PlayerInputComponent->BindAction(FName{ "Crouch" }, IE_Released, this, &ThisClass::CrouchButtonReleased);

	PlayerInputComponent->BindAction(FName{ "Aim" }, IE_Pressed, this, &ThisClass::AimButtonPressed);
	PlayerInputComponent->BindAction(FName{ "Aim" }, IE_Released, this, &ThisClass::AimButtonReleased);

	PlayerInputComponent->BindAction(FName{ "Fire" }, IE_Pressed, this, &ThisClass::FireButtonPressed);
	PlayerInputComponent->BindAction(FName{ "Fire" }, IE_Released, this, &ThisClass::FireButtonReleased);

	PlayerInputComponent->BindAction(FName{ "Reload" }, IE_Pressed, this, &ThisClass::ReloadButtonPressed);

	PlayerInputComponent->BindAction(FName{ "ThrowGrenade" }, IE_Pressed, this, &ThisClass::ThrowGrenadeButtonPressed);

	PlayerInputComponent->BindAction(FName{ "SwitchCameraSide" }, IE_Pressed, this, &ThisClass::SwitchCameraSide);

	PlayerInputComponent->BindAction(FName{ "SwapWeapon" }, IE_Pressed, this, &ThisClass::SwapWeaponsButtonPressed);





	//	PlayerInputComponent->BindAction(FName{ "" }, , this, &ThisClass:);

}



/*
* Dev Map Debug Methods
*/

void APlayerCharacter::DEBUG_PlacePlayerAt_TwentyFiveMeters()
{
	if (IsLocallyControlled())
	{
		DEBUG_ServerPlacePlayerAt_TwentyFiveMeters();
	}

}

void APlayerCharacter::DEBUG_PlacePlayerAt_FiftyMeters()
{
	if (IsLocallyControlled())
	{
		DEBUG_ServerPlacePlayerAt_FiftyMeters();
	}
}

void APlayerCharacter::DEBUG_PlacePlayerAt_SeventyFiveMeters()
{
	if (IsLocallyControlled())
	{
		DEBUG_ServerPlacePlayerAt_SeventyFiveMeters();
	}
}

void APlayerCharacter::DEBUG_PlacePlayerAt_HundredMeters()
{
	if (IsLocallyControlled())
	{
		DEBUG_ServerPlacePlayerAt_HundredMeters();
	}
}

void APlayerCharacter::DEBUG_PlacePlayerAt_HundredAndFiftyMeters()
{
	if (IsLocallyControlled())
	{
		DEBUG_ServerPlacePlayerAt_HundredAndFiftyMeters();
	}
}



void APlayerCharacter::DEBUG_ServerPlacePlayerAt_TwentyFiveMeters_Implementation()
{
#if WITH_EDITOR
	SetActorLocation(FVector{ 0.f, -2500.f, 100.f });
#endif
}

void APlayerCharacter::DEBUG_ServerPlacePlayerAt_FiftyMeters_Implementation()
{
#if WITH_EDITOR
	SetActorLocation(FVector{ 0.f, -5000.f, 100.f });
#endif
}

void APlayerCharacter::DEBUG_ServerPlacePlayerAt_SeventyFiveMeters_Implementation()
{
#if WITH_EDITOR
	SetActorLocation(FVector{ 0.f, -7500.f, 100.f });
#endif
}

void APlayerCharacter::DEBUG_ServerPlacePlayerAt_HundredMeters_Implementation()
{
#if WITH_EDITOR
	SetActorLocation(FVector{ 0.f, -10000.f, 100.f });
#endif
}

void APlayerCharacter::DEBUG_ServerPlacePlayerAt_HundredAndFiftyMeters_Implementation()
{
#if WITH_EDITOR
	SetActorLocation(FVector{ 0.f, -15000.f, 100.f });
#endif
}


