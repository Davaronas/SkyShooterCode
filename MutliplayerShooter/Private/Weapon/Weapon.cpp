// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Weapon.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/PrimitiveComponent.h"
#include "PlayerCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Animation/AnimationAsset.h"
#include "Weapon/Casing.h"
#include "PlayerCharacter.h"
#include "Controller/MainPlayerController.h"
#include "PlayerComponents/CombatComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Pickup/PickupSpawn.h"

#include "DebugMacros.h"



AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	// Physics forces may be different on server and client, and it might result in
	// the weapon not being in the same place on server and client
	SetReplicateMovement(true);

	weaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Skeletal Weapon Mesh"));
	weaponMesh->SetupAttachment(RootComponent);
	SetRootComponent(weaponMesh);
	weaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	weaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECR_Ignore);
	weaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECR_Ignore);
	weaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);


	SetCustomDepthAndEnable();

	sphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere Overlap Mesh"));
	sphereComp->SetupAttachment(RootComponent);
	sphereComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	sphereComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	pickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("Pickup Widget"));
	pickupWidget->SetupAttachment(RootComponent);
	ShowPickupWidget(false);
	
}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWeapon, weaponState);
	//DOREPLIFETIME(AWeapon, currentAmmo);
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	SetCustomDepthAndEnable();

	currentAmmo = magazineCapacity;
	
	//if (GetLocalRole() == ENetRole::ROLE_Authority)
	if(HasAuthority())
	{
		sphereComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		sphereComp->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECR_Overlap);

		sphereComp->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnSphereOverlap);
		sphereComp->OnComponentEndOverlap.AddDynamic(this, &ThisClass::OnSphereOverlapExit);
	}

	
}

void AWeapon::SetCustomDepthAndEnable()
{
	if (weaponMesh)
	{
		weaponMesh->SetCustomDepthStencilValue((int32)weaponDepthType);
		weaponMesh->MarkRenderStateDirty();
		EnableCustomDepth(true);
	}
}


void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	

}

void AWeapon::OnSphereOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (APlayerCharacter* player = Cast<APlayerCharacter>(OtherActor))
	{
		player->SetOverlappingWeapon(this);
	}
}

void AWeapon::OnSphereOverlapExit(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	if (APlayerCharacter* player = Cast<APlayerCharacter>(OtherActor))
	{
		if(player->EqualsOverlappingWeapon(this))
		player->SetOverlappingWeapon(nullptr);
	}
}

void AWeapon::TryGetMuzzleFlashSocket()
{
	if (!muzzleFlashSocket) muzzleFlashSocket = GetWeaponMesh()->GetSocketByName(MUZZLE_FLASH_SOCKET);
	if(muzzleFlashSocket)
	muzzleFlashSocketTransform =
		muzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
}

FVector AWeapon::AddScatterToTargetVector(const FVector& start, const FVector& target)
{
	if (!ownerPlayer) { return FVector(); }

	const float currentPlayerInaccuracy = ownerPlayer->GetCurrentInaccuracy();

	const FVector targetVector = target - start;
	const float distance = targetVector.Size();
	FRotator targetVectorRotator = targetVector.Rotation();

	const float one_Y = FMath::RandBool() ? -1 : 1;
	const float one_P = FMath::RandBool() ? -1 : 1;

	const float randomScatter_Y = FMath::RandRange
	(-randomScatterAdditionalRange, randomScatterAdditionalRange);
	const float randomScatter_P = FMath::RandRange
	(-randomScatterAdditionalRange, randomScatterAdditionalRange);

	const float scatterYaw = (baseScatter * currentPlayerInaccuracy * one_Y) +
		(randomScatter_Y * currentPlayerInaccuracy);

	const float scatterPitch = (baseScatter * currentPlayerInaccuracy * one_P) +
		(randomScatter_P * currentPlayerInaccuracy);

	targetVectorRotator.Yaw += scatterYaw;
	targetVectorRotator.Pitch += scatterPitch;

	FVector targetVectorRotator_X =
		FRotationMatrix(targetVectorRotator).GetUnitAxis(EAxis::X) * distance;

	return targetVectorRotator_X;
}

FVector AWeapon::AddScatterToTargetVector
(const FVector& start, const FVector& target, FFireData& outRandomNumbers)
{
	if (!ownerPlayer) { return FVector(); }

	const float currentPlayerInaccuracy = ownerPlayer->GetCurrentInaccuracy();

	const FVector targetVector = target - start;
	const float distance = targetVector.Size();
	FRotator targetVectorRotator = targetVector.Rotation();

	const float one_Y = FMath::RandBool() ? -1 : 1;
	const float one_P = FMath::RandBool() ? -1 : 1;

	outRandomNumbers.one_Y = one_Y > 0.f ? true : false;
	outRandomNumbers.one_P = one_P > 0.f ? true : false;


	const float randomScatter_Y = FMath::RandRange
	(-randomScatterAdditionalRange, randomScatterAdditionalRange);
	const float randomScatter_P = FMath::RandRange
	(-randomScatterAdditionalRange, randomScatterAdditionalRange);

	outRandomNumbers.randomScatter_Y = randomScatter_Y;
	outRandomNumbers.randomScatter_P = randomScatter_P;


	const float scatterYaw = (baseScatter * currentPlayerInaccuracy * one_Y) +
		(randomScatter_Y * currentPlayerInaccuracy);

	const float scatterPitch = (baseScatter * currentPlayerInaccuracy * one_P) +
		(randomScatter_P * currentPlayerInaccuracy);

	targetVectorRotator.Yaw += scatterYaw;
	targetVectorRotator.Pitch += scatterPitch;

	FVector targetVectorRotator_X =
		FRotationMatrix(targetVectorRotator).GetUnitAxis(EAxis::X) * distance;

	return targetVectorRotator_X;
}

FVector AWeapon::AddScatterToTargetVector
(const FVector& start, const FVector& target, float accuracy, const FFireData& numbersOverride)
{
	const FVector targetVector = target - start;
	const float distance = targetVector.Size();
	FRotator targetVectorRotator = targetVector.Rotation();

	const float one_Y = numbersOverride.one_Y ? 1.f : -1.f;
	const float one_P = numbersOverride.one_P ? 1.f : -1.f;

	const float scatterYaw = (baseScatter * accuracy * one_Y) +
		(numbersOverride.randomScatter_Y * accuracy);

	const float scatterPitch = (baseScatter * accuracy * one_P) +
		(numbersOverride.randomScatter_P * accuracy);

	targetVectorRotator.Yaw += scatterYaw;
	targetVectorRotator.Pitch += scatterPitch;

	FVector targetVectorRotator_X =
		FRotationMatrix(targetVectorRotator).GetUnitAxis(EAxis::X) * distance;

	return targetVectorRotator_X;
}

void AWeapon::EnableCustomDepth(bool bState)
{
	if (weaponMesh)
	{
		weaponMesh->SetRenderCustomDepth(bState);
	}
}







// Client
//void AWeapon::OnRep_currentAmmo()
//{
//	HUD_UpdateCurrentAmmo();
//
//	if (IsShotgun() && ownerPlayer && ownerPlayer->GetCombatComponent() &&
//		(IsFull() || !ownerPlayer->GetCombatComponent()->HasCarriedAmmo()))
//	{
//		ownerPlayer->JumpToShotgunMontageEnd();
//	}
//	//TryReloadIfEmpty();
//}

void AWeapon::TryReloadIfEmpty()
{
	if (currentAmmo <= 0 && ownerPlayer)
	{
		ownerPlayer->TryReload();
	}
}

void AWeapon::HUD_UpdateCurrentAmmo()
{
	TryGetPlayer();

	if (ownerPlayer && ownerPlayer->IsLocallyControlled() && ownerPlayer->EqualsEquippedWeapon(this))
	{
		if (ownerMainPlayerController)
		{
			ownerMainPlayerController->SetHUDAmmo(currentAmmo);
		}
	}
}

void AWeapon::OnRep_Owner()
{
	Super::OnRep_Owner();

	// Dropped
	if (!GetOwner())
	{
		ownerPlayer = nullptr;
		ownerMainPlayerController = nullptr;
		return;
	}

	HUD_UpdateCurrentAmmo();
}



void AWeapon::TryGetPlayer()
{
	if (!ownerPlayer && GetOwner())
	{
		ownerPlayer = Cast<APlayerCharacter>(GetOwner());

		if (!ownerMainPlayerController && ownerPlayer)
		{
			ownerMainPlayerController = Cast<AMainPlayerController>(ownerPlayer->GetController());
		}
	}
}






void AWeapon::SetWeaponState(EWeaponState state)
{
	if (!sphereComp) { return; }

	weaponState = state;

	switch (weaponState)
	{
	case EWeaponState::EWS_Equipped:
		if (HasAuthority())
			ClientSetAmmo(currentAmmo);
			

		ShowPickupWidget(false);
		sphereComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		DisableMeshCollisionAndPhysics();
		HUD_UpdateCurrentAmmo();
		EnableCustomDepth(false);
		CheckEnableCosmeticPhysics();

		if (spawner)
		{
			spawner->PickupDestroyed();
			spawner = nullptr;
		}

		break;
	case EWeaponState::EWS_Dropped:

		if (HasAuthority())
		{
			sphereComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		}
		SetCustomDepthAndEnable();
		EnableMeshCollisionAndPhysics();

		break;
	case EWeaponState::EWS_Stored:
		ShowPickupWidget(false);
		sphereComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		DisableMeshCollisionAndPhysics();
		HUD_UpdateCurrentAmmo();
		EnableCustomDepth(false);
		CheckEnableCosmeticPhysics();
		break;
	case EWeaponState::EWS_Initial:

		break;
	default:

		break;
	}
}

void AWeapon::CheckEnableCosmeticPhysics()
{
	if (bSimulateCosmeticPhysicsOnWeaponMesh)
	{
		weaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		weaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		weaponMesh->SetEnableGravity(true);
	}
}


// Cannot call Update HUD here, because Owner might not be set before weapon state
void AWeapon::OnRep_weaponState()
{
	switch (weaponState)
	{
	case EWeaponState::EWS_Equipped:
		ShowPickupWidget(false);
		DisableMeshCollisionAndPhysics();
		CheckEnableCosmeticPhysics();
		EnableCustomDepth(false);
		break;
	case EWeaponState::EWS_Dropped:
		SetCustomDepthAndEnable();
		EnableMeshCollisionAndPhysics();

		break;
	case EWeaponState::EWS_Stored:
		ShowPickupWidget(false);
		DisableMeshCollisionAndPhysics();
		CheckEnableCosmeticPhysics();
		EnableCustomDepth(false);
		break;
	case EWeaponState::EWS_Initial:

		break;
	default:

		break;
	}
}






void AWeapon::EnableMeshCollisionAndPhysics()
{
	if (weaponMesh)
	{
		weaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		weaponMesh->SetSimulatePhysics(true);
		weaponMesh->SetEnableGravity(true);

		HandleCosmeticPhysicsOnPhysicsSimulated();
	}


}

void AWeapon::HandleCosmeticPhysicsOnPhysicsSimulated()
{
	if (bSimulateCosmeticPhysicsOnWeaponMesh)
	{
		weaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
		weaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECR_Ignore);
		weaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECR_Ignore);
	}
}

void AWeapon::DisableMeshCollisionAndPhysics()
{
	if (weaponMesh)
	{
		weaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		weaponMesh->SetSimulatePhysics(false);
		weaponMesh->SetEnableGravity(false);
	}
}






void AWeapon::ShowPickupWidget(bool bShow)
{
	if (pickupWidget) pickupWidget->SetVisibility(bShow);
}



void AWeapon::Fire(const FVector& shootFromPos, const FVector& targetPos)
{
	FireBase();
}

// Overriden in HitScanWeapon and ProjectileWeapon
void AWeapon::Fire
(const FVector& shootFromPos, const FVector& targetPos,
	FFireData& outFireData, FServerSideRewindData& outSSRData, FVector& outActualHitLoc)
{
	FireBase();
}

// Overriden in Shotgun
void AWeapon::Fire(const FVector& shootFromPos, const FVector& targetPos,
	TArray<FFireData>& outFireDatas, TArray<FServerSideRewindData>& outSSRDatas)
{
	FireBase();
}


// Sync aut proxy with others and 
void AWeapon::Fire(const FVector& shootFromPos, const FVector& targetPos, const FFireData& fireData)
{

}

void AWeapon::Fire(const FVector& shootFromPos, const FVector& targetPos, const TArray<FFireData>& fireDatas)
{

}

void AWeapon::FireBase()
{
	SpendRound();
	PlayWeaponAnimation();
	TryGetMuzzleFlashSocket();
	SpawnMuzzleFlashParticles();
	PlayFireSound();
	SpawnCasing();
}




void AWeapon::SpendRound()
{
	currentAmmo = FMath::Clamp(currentAmmo - 1, 0, magazineCapacity);
	HUD_UpdateCurrentAmmo();

	if (IsOwnerNotLocallyControlledServer())
	{
		ClientSpendAmmo(currentAmmo);
	}
	else if(IsOwnerLocallyControlled() && !ownerPlayer->HasAuthority())
	{
		++ammoRequestsSequence;
	}

}



void AWeapon::ClientSpendAmmo_Implementation(const int32& ammo)
{
	currentAmmo = FMath::Clamp(ammo, 0, magazineCapacity);
	--ammoRequestsSequence;
	currentAmmo = FMath::Clamp(currentAmmo - ammoRequestsSequence, 0, magazineCapacity);

	HUD_UpdateCurrentAmmo();
}



void AWeapon::SetCurrentAmmo(const int32& ammo)
{
	currentAmmo = FMath::Clamp(ammo, 0, magazineCapacity);
	HUD_UpdateCurrentAmmo();

	if (IsOwnerNotLocallyControlledServer())
	{
		ClientSetAmmo(currentAmmo);
	}
}

void AWeapon::ClientSetAmmo_Implementation(const int32& ammo)
{
	ammoRequestsSequence = 0;
	currentAmmo = FMath::Clamp(ammo, 0, magazineCapacity);
	HUD_UpdateCurrentAmmo();
}



void AWeapon::AddToCurrentAmmo(const int32& ammo)
{
	currentAmmo = FMath::Clamp(currentAmmo + ammo, 0, magazineCapacity);
	HUD_UpdateCurrentAmmo();

	if (IsShotgun() && ownerPlayer && ownerPlayer->GetCombatComponent() &&
		(IsFull() || !ownerPlayer->GetCombatComponent()->HasCarriedAmmo()))
	{
		ownerPlayer->JumpToShotgunMontageEnd();
	}

	if (IsOwnerNotLocallyControlledServer())
	{
		ClientAddToAmmo(ammo);
	}

}



void AWeapon::ClientAddToAmmo_Implementation(const int32& ammo)
{
	currentAmmo = FMath::Clamp(currentAmmo + ammo, 0, magazineCapacity);
	HUD_UpdateCurrentAmmo();

	if (IsShotgun() && ownerPlayer && ownerPlayer->GetCombatComponent() &&
		(IsFull() || !ownerPlayer->GetCombatComponent()->HasCarriedAmmo()))
	{
		ownerPlayer->JumpToShotgunMontageEnd();
	}
}


void AWeapon::PlayWeaponAnimation()
{
	if (weaponMesh && fireAnimAsset)
		weaponMesh->PlayAnimation(fireAnimAsset, false);
}

void AWeapon::PlayFireSound()
{
	if (fireSound)
	{
		if (muzzleFlashSocket)
		{
			UGameplayStatics::PlaySoundAtLocation(this, fireSound, muzzleFlashSocketTransform.GetLocation());
		}
	}
}

void AWeapon::SpawnMuzzleFlashParticles()
{
	if (muzzleFlashParticles)
	{

		if (muzzleFlashSocket)
		{
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(),
				muzzleFlashParticles, muzzleFlashSocketTransform, true);
		}
	}
}

void AWeapon::SpawnCasing()
{
	if (!casingToSpawn) { return; }
	if (!ammoEjectSocket) ammoEjectSocket = weaponMesh->GetSocketByName(AMMO_EJECT_SOCKET);
	if (!ammoEjectSocket) { return; }
	if (!GetWorld()) { return; }

	FTransform socketTransform = ammoEjectSocket->GetSocketTransform(weaponMesh);

	GetWorld()->SpawnActor<ACasing>
		(casingToSpawn, socketTransform.GetLocation(), socketTransform.GetRotation().Rotator());
}

void AWeapon::Drop()
{
	SetWeaponState(EWeaponState::EWS_Dropped);

	if (weaponMesh)
	{
		FDetachmentTransformRules detachRules{ EDetachmentRule::KeepWorld, true};
		
		weaponMesh->DetachFromComponent(detachRules);
		SetOwner(nullptr);
		SetInstigator(nullptr);
		ownerPlayer = nullptr;
		ownerMainPlayerController = nullptr;


	}
}



FVector AWeapon::GetMuzzleFlashLocation()
{
	TryGetMuzzleFlashSocket();
	if (muzzleFlashSocket)
	{
		return muzzleFlashSocket->GetSocketLocation(weaponMesh);
	}

	return FVector{};
}


bool AWeapon::IsOwnerNotLocallyControlledServer() const
{
	return IsOwnerServer() && !ownerPlayer->IsLocallyControlled();
}

bool AWeapon::IsOwnerLocallyControlled() const
{
	return ownerPlayer && ownerPlayer->IsLocallyControlled();
}

bool AWeapon::IsOwnerServer() const
{
	return ownerPlayer && ownerPlayer->HasAuthority();
}

bool AWeapon::IsOwnerAutonomousProxy() const
{
	return ownerPlayer && ownerPlayer->GetLocalRole() == ENetRole::ROLE_AutonomousProxy;
}

const FName AWeapon::WeaponReloadTypeAsFName(EWeaponReloadAnimationType type)
{
	switch (type)
	{
	case EWeaponReloadAnimationType::EWRAT_NormalReload:
		return WeaponReloadTypeName::NORMAL_RELOAD;
		break;
	case EWeaponReloadAnimationType::EWRAT_GripReload:
		return WeaponReloadTypeName::GRIP_RELOAD;
		break;
	case EWeaponReloadAnimationType::EWRAT_ShotgunReload:
		return WeaponReloadTypeName::SHOTGUN_RELOAD;
		break;
	case EWeaponReloadAnimationType::EWRAT_RocketLauncherReload:
		return WeaponReloadTypeName::ROCKET_LAUNCHER_RELOAD;
		break;
	default:
		return FName{};
		break;
	}
}

