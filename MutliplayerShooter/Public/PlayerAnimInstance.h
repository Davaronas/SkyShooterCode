// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "HelperTypes/TurningInPlace.h"
#include "HelperTypes/CombatState.h"

#include "PlayerAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class MUTLIPLAYERSHOOTER_API UPlayerAnimInstance : public UAnimInstance
{
	GENERATED_BODY()



public:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	bool CanDoLeftHandIK();

protected:
	virtual void NativeInitializeAnimation() override;

	void InitializePlayerCharacterMovementComponentPointer();

	void InitializePlayerCharacterPointer();

	void InitializePlayerCombatComponentPointer();

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Movement")
	class APlayerCharacter* playerCharacter{};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Movement")
	class UCharacterMovementComponent* playerCharacterMovement{};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Movement")
	float speed{};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Movement")
	bool bInAir{};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Movement")
	bool bIsAccelerating{};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Movement")
	bool bIsCrouched{};

	

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Combat")
	class UCombatComponent* playerCombatComponent{};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Combat")
	bool bWeaponEquipped{};
	class AWeapon* equippedWeapon{};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Combat")
	bool bDisallowFireWeaponTooCloseToObject{};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Combat")
	bool bIsAiming{};
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Combat")
	bool bIsLocallyAiming{};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Combat")
	float yawOffset{};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Combat")
	float lean{};

	FRotator lastFrameRotation{};
	FRotator thisFrameRotation{};



	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Combat")
	float AO_Yaw{};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Combat")
	float AO_Pitch{};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Combat")
	FTransform leftHandTransform{};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Combat")
	ETurningInPlace turningInPlaceState{ ETurningInPlace::ETIP_NotTurning };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Combat")
	FRotator rightHandToTargetRotation{};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Combat")
	bool bIsDead{};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Combat")
	bool bUseFabrik{};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Combat")
	bool bIsLocallyReloading{};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Combat")
	bool bIsLocallySwappingWeapons{};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Combat")
	bool bUseRightHandLookRot{};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Combat")
	ECombatState combatState{};



	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Util")
	bool bLocallyControlled{};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Util")
	bool bRotateRootBone{};

};
