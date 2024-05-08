// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerAnimInstance.h"
#include "PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Weapon/Weapon.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "PlayerComponents/CombatComponent.h"


void UPlayerAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	InitializePlayerCharacterPointer();
	InitializePlayerCharacterMovementComponentPointer();
	InitializePlayerCombatComponentPointer();

}



void UPlayerAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!playerCharacter) { InitializePlayerCharacterPointer(); }
	if (!playerCharacter) { return; }
	if (!playerCharacterMovement) { InitializePlayerCharacterMovementComponentPointer(); }
	if (!playerCharacterMovement) { return; }
	if (!playerCombatComponent) { InitializePlayerCombatComponentPointer(); }
	if (!playerCombatComponent) { return; }

	FVector playerVelocity = playerCharacter->GetVelocity();
	playerVelocity.Z = 0.f;
	speed = playerVelocity.Size();

	//bInAir = !(playerCharacterMovement->IsMovingOnGround());
	bInAir = playerCharacterMovement->IsFalling();
	bIsAccelerating = playerCharacterMovement->GetCurrentAcceleration().Size() > 0.f ? true : false;
	bWeaponEquipped = playerCharacter->HasWeaponEquipped();
	equippedWeapon = playerCharacter->GetEquippedWeapon();
	bIsCrouched = playerCharacter->bIsCrouched;
	bIsAiming = playerCharacter->IsAiming() && combatState == ECombatState::ECS_Unoccupied 
		&& !bDisallowFireWeaponTooCloseToObject;

	if (playerCharacter->IsLocallyControlled())
	{
		bIsAiming = bIsLocallyAiming && !bIsLocallyReloading && !bIsLocallySwappingWeapons &&
			combatState != ECombatState::ECS_ThrowingGrenade && !bDisallowFireWeaponTooCloseToObject;
	}

	// Yaw Offset -> Strafing
	const FRotator aimRot = playerCharacter->GetBaseAimRotation();
	const FRotator playerVel = UKismetMathLibrary::MakeRotFromX(playerCharacter->GetVelocity());
	yawOffset = UKismetMathLibrary::NormalizedDeltaRotator(aimRot, playerVel).Yaw;

	lastFrameRotation = thisFrameRotation;
	thisFrameRotation = playerCharacter->GetActorRotation();
	const FRotator rotDifferenceBetweenFrames = UKismetMathLibrary::NormalizedDeltaRotator
	(lastFrameRotation, thisFrameRotation);
	const float rotYaw = rotDifferenceBetweenFrames.Yaw / DeltaSeconds;
	const float rotYawInterp = FMath::FInterpTo(lean, rotYaw, DeltaSeconds, 12.f);
	//lean = FMath::Clamp(rotYawInterp, -90.f, 90.f);
	lean = FMath::Clamp(rotYawInterp, -180.f, 180.f);

	AO_Yaw = playerCharacter->GetAO_Yaw();
	AO_Pitch = playerCharacter->GetAO_Pitch();

	if (CanDoLeftHandIK())
	{
		//const USkeletalMeshSocket* weaponLeftHandIK_Socket =
			//equippedWeapon->GetWeaponMesh()->GetSocketByName(LEFT_HAND_IK_SOCKET_ON_WEAPONS);

		leftHandTransform = equippedWeapon->GetWeaponMesh()->GetSocketTransform
		(LEFT_HAND_IK_SOCKET_ON_WEAPONS, ERelativeTransformSpace::RTS_World);

		FVector outToBonePos{};
		FRotator outToBoneRot{};
		playerCharacter->GetMesh()->TransformToBoneSpace(RIGHT_HAND_BONE,
			leftHandTransform.GetLocation(), leftHandTransform.GetRotation().Rotator(),
			outToBonePos, outToBoneRot);

		leftHandTransform.SetLocation(outToBonePos);
		leftHandTransform.SetRotation(outToBoneRot.Quaternion());
	}

	turningInPlaceState = playerCharacter->GetTurningInPlaceState();
	rightHandToTargetRotation = playerCharacter->GetRightHandTargetRotation();
	bLocallyControlled = playerCharacter->IsLocallyControlled();
	bRotateRootBone = playerCharacter->ShouldRotateRootBone();
	bIsDead = playerCharacter->IsDead();
	combatState = playerCharacter->GetCombatState();
	bUseFabrik = combatState == ECombatState::ECS_Unoccupied;

	bDisallowFireWeaponTooCloseToObject = playerCombatComponent->GetDisallowFireWeaponTooCloseToObject();

	if (playerCharacter->IsLocallyControlled())
	{
		bUseFabrik =
			!bIsLocallySwappingWeapons && 
			combatState != ECombatState::ECS_ThrowingGrenade && !bIsLocallyReloading;
	}

	bUseRightHandLookRot = combatState == ECombatState::ECS_Unoccupied &&
		!playerCharacter->IsGameplayDisabled();

	if (playerCharacter->IsLocallyControlled())
	{
		bUseRightHandLookRot = !bIsLocallySwappingWeapons &&
			combatState != ECombatState::ECS_ThrowingGrenade && !bIsLocallyReloading &&
			!playerCharacter->IsGameplayDisabled();
	}

	bIsLocallyReloading = playerCombatComponent->IsLocallyReloading();
	bIsLocallyAiming = playerCombatComponent->IsLocallyAiming();
	bIsLocallySwappingWeapons = playerCombatComponent->IsLocallySwappingWeapons();
}

bool UPlayerAnimInstance::CanDoLeftHandIK()
{
	return bWeaponEquipped && equippedWeapon && equippedWeapon->GetWeaponMesh() && playerCharacter->GetMesh();
}






void UPlayerAnimInstance::InitializePlayerCharacterPointer()
{
	if (APawn* pawn = TryGetPawnOwner())
	{
		playerCharacter = Cast<APlayerCharacter>(pawn);
	}
}

void UPlayerAnimInstance::InitializePlayerCombatComponentPointer()
{
	if (playerCharacter)
	{
		playerCombatComponent = playerCharacter->GetCombatComponent();
	}
}

void UPlayerAnimInstance::InitializePlayerCharacterMovementComponentPointer()
{
	if (playerCharacter)
	{
		playerCharacterMovement = playerCharacter->GetCharacterMovement();
	}
}