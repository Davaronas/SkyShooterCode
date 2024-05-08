// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "HelperTypes/TurningInPlace.h"
#include "Interface/CrosshairInteractInterface.h"
#include "Components/TimelineComponent.h"
#include "HelperTypes/CombatState.h"
#include "HelperTypes/Teams.h"


#include "PlayerCharacter.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerLeaveGameDelegate);


#define RIGHT_HAND_SOCKET FName{TEXT("RightHandSocket")}
#define RIGHT_HAND_SOCKET_ROTATED FName{TEXT("RightHandSocket_Rotated")}
#define LEFT_HAND_SOCKET_LONG_WEAPON FName{TEXT("LeftHandSocket_LongWeapon")}
#define LEFT_HAND_SOCKET_SHORT_WEAPON FName{TEXT("LeftHandSocket_ShortWeapon")}
#define LEFT_HAND_SOCKET_LONG_WEAPON_ROTATED FName{TEXT("LeftHandSocket_LongWeapon_Rotated")}
#define SECONDARY_WEAPON_SOCKET_LONG_WEAPON FName{TEXT("SecondaryWeaponSocket_LongWeapon")}
#define SECONDARY_WEAPON_SOCKET_LONG_WEAPON_ROTATED FName{TEXT("SecondaryWeaponSocket_LongWeapon_Rotated")}

#define SECONDARY_WEAPON_SOCKET_SHORT_WEAPON FName{TEXT("SecondaryWeaponSocket_ShortWeapon")}
#define GRENADE_SOCKET FName{TEXT("GrenadeSocket")}
#define LEFT_HAND_IK_SOCKET_ON_WEAPONS FName{TEXT("LeftHandIK")}
#define RIGHT_HAND_BONE FName{TEXT("hand_r")}
#define HEAD_BONE_AND_SSRB FName{TEXT("head")}


#define HIDE_AMMO_AMOUNT -99

// [] means incluse brackets in comments
// () means exclusive brackets
// [0 - 501) means from 0 to 500, excluding 501

UCLASS()
class MUTLIPLAYERSHOOTER_API APlayerCharacter : public ACharacter, public ICrosshairInteractInterface
{
	GENERATED_BODY()

public:

#if WITH_EDITORONLY_DATA
	UPROPERTY(BlueprintReadWrite, Category = "DEBUG")
	bool bDEBUG_KeepJumping{ false };

	UPROPERTY(BlueprintReadWrite, Category = "DEBUG")
	bool bDEBUG_KeepStrafing{ false };

	UPROPERTY(EditAnywhere, Category = "DEBUG")
	bool bDEBUG_Invincible{ false };
	


	bool bStrafesRight{ true };
	bool bStrafesLeft{ false };
#endif

	APlayerCharacter();

	virtual void Landed(const FHitResult& Hit) override;
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode) override;

	

	void SetTeamColor(ETeams team);


	UFUNCTION(Server, Reliable)
	void ServerLeaveGame();
	FOnPlayerLeaveGameDelegate OnPlayerLeftGameDelegate{};


	bool bLanded = false;
	bool bPerchingEnabled = false;
	void Perching();
	void GroundCheckPerching();
	void CheckEnablePerching(const FHitResult& Hit, bool bReversed = false);
	void DisablePerching();
	void EnablePerching();
	FVector GetCapsuleBottomPosition();
	FHitResult lastLandedHit{};
	UFUNCTION(Server, Reliable)
	void Server_ClientChangedPerching(bool bEnabled);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_ClientChangedPerching(bool bEnabled);

	virtual void Tick(float DeltaTime) override;

	

	UFUNCTION(NetMulticast, Reliable)
	void MulticastGainedLead();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastLostLead();
	
	void RotateInPlace(float DeltaTime);
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;

	void AddToHealth(float health);

	void PlayFireMontage(bool bAiming);
	void PlayHitReactMontage();
	void PlayDeathMontage();
	void PlayReloadMontage();
	void PlayThrowGrenadeMontage();
	void PlaySwapWeaponsMontage();
	void JumpToShotgunMontageEnd();

	UFUNCTION(BlueprintImplementableEvent)
	void ShowSniperScopeWidget(bool state, bool bPlaySound);

	void HideCharacterAndWeapon(const bool& bHide);

	// when actor movement gets updated
	virtual void OnRep_ReplicatedMovement() override;

	virtual void Destroyed() override;

	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor,
		float Damage, const UDamageType* DamageType,
		class AController* InstigatorController, AActor* DamageCauser);

	void OnScreenDebugMessages(AActor* DamageCauser, AController* InstigatorController, float Damage);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastOnScreenDebugMessage(const FString& message);

	void InstigatorHUDHitFeedback
	(AController* InstigatorController, float newHealth, float shieldBeforeHit, const float& actualDamageDone);

	void VictimHUDHitFeedback(AActor* DamagedActor, AActor* InstigatorActor);

	// No need to set UFUNCTION here, because this gets called only on the game mode
	// which only exists on the server, so only the server calls this
	void ServerPlayerEliminated(bool bLeftGame_ = false);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayerEliminated(bool bLeftGame_ = false);

	void TryReload() { ReloadButtonPressed(); }

	void DisableGameplay();

	void ShowGrenade(bool bShow);

	void SetCrosshairDrawLocation(const FVector2D& location);

	void AddShield(float amount);


	virtual void OnJumped_Implementation() override;


	UFUNCTION(BlueprintCallable)
	void FootstepSound();
	void HandleFootstepSound(const float& vol_);
	void PlayCurrentFootstepSound(const float& vol_);
	bool FootstepSphereTrace();
	UFUNCTION(BlueprintCallable)
	void FootstepSoundLight();
	UFUNCTION(BlueprintCallable)
	void FootstepLand();

protected:
	virtual void BeginPlay() override;

	/*
	* Footstep
	*/

	UPROPERTY(EditDefaultsOnly, Category = "Footstep")
	float footstepSphereTraceRadius{ 50.f };
	UPROPERTY(EditDefaultsOnly, Category = "Footstep")
	FVector footstepSphereTraceOffset{ FVector{0.f, 0.f, 30.f} };
	UPROPERTY()
	TArray<AActor*> ignoreSelfArray{};
	FHitResult footstepHitResult{};
	UPROPERTY(EditDefaultsOnly, Category = "Footstep")
	class USoundBase* grassFootstepSound{};
	UPROPERTY(EditDefaultsOnly, Category = "Footstep")
	class USoundBase* stoneFootstepSound{};
	UPROPERTY(EditDefaultsOnly, Category = "Footstep")
	class USoundBase* metalFootstepSound{};
	UPROPERTY(EditDefaultsOnly, Category = "Footstep|Land")
	class USoundBase* grassLandSound{};
	UPROPERTY(EditDefaultsOnly, Category = "Footstep|Land")
	class USoundBase* stoneLandSound{};
	UPROPERTY(EditDefaultsOnly, Category = "Footstep|Land")
	class USoundBase* metalLandSound{};
	UPROPERTY()
	class USoundBase* currentFootstepSound{};
	

	UPROPERTY(VisibleAnywhere, meta = (ClampMin = "0.01"))
	float mouseSensitivity{1.f};

	UPROPERTY()
	class AMainGameMode* mainGameMode{};

//	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	UPROPERTY(EditDefaultsOnly)
	float perchWhileNotInAir{ 60.f };

	void MoveForward(float axisValue);
	void MoveRight(float axisValue);
	void Turn(float axisValue);
	void LookUp(float axisValue);


	void CrouchButtonPressed();
	void CrouchButtonReleased();
	void AimButtonPressed();
	void AimButtonReleased();
	virtual void Jump() override;
	bool bJumpBuffered{ false };
	void FireButtonPressed();
	void FireButtonReleased();
	void ReloadButtonPressed();
	void ThrowGrenadeButtonPressed();
	void SwapWeaponsButtonPressed();


	void AimOffset(float DeltaTime);
	void CalculateAO_Pitch();
	void TurnInPlace(float DeltaTime);

	void SwitchCameraSide();
	void InterpSpringArm(float DeltaTime);
	FVector springArmInterpTo_{};

	void SimulatedProxiesTurn();

	void CalculateSpeed();


	void InteractButtonPressed();
	void InteractButtonShortPressed();
	void InteractButtonHeld();
	void InteractButtonCheckHold();
	void InteractButtonReleased();
	bool bInteractPressed{ false };
	bool bInteractHeld{ false };
	FTimerHandle interactButtonPressTimer{};
	UPROPERTY(EditAnywhere)
	float interactButtonPressTimeConsideredHeld{ 0.5f };
	// Remote Procedure Call
	// Create definition for _Implementation
	UFUNCTION(Server, Reliable)
	void ServerInteractButtonPressed(bool bButtonHeld = false);


	void RespawnTimerEnded();
	//void ResetServer();

	
	bool TeamsEqual(AActor* DamagedActor, class AController* InstigatorController);


	/*
	* Dev Map Debug Methods
	*/

	UFUNCTION(BlueprintCallable, Category = "Debug")
	void DEBUG_PlacePlayerAt_TwentyFiveMeters();

	UFUNCTION(BlueprintCallable, Category = "Debug")
	void DEBUG_PlacePlayerAt_FiftyMeters();

	UFUNCTION(BlueprintCallable, Category = "Debug")
	void DEBUG_PlacePlayerAt_SeventyFiveMeters();

	UFUNCTION(BlueprintCallable, Category = "Debug")
	void DEBUG_PlacePlayerAt_HundredMeters();
	UFUNCTION(BlueprintCallable, Category = "Debug")
	void DEBUG_PlacePlayerAt_HundredAndFiftyMeters();

	UFUNCTION(Server, Reliable)
	void DEBUG_ServerPlacePlayerAt_TwentyFiveMeters();

	UFUNCTION(Server, Reliable)
	void DEBUG_ServerPlacePlayerAt_FiftyMeters();

	UFUNCTION(Server, Reliable)
	void DEBUG_ServerPlacePlayerAt_SeventyFiveMeters();

	UFUNCTION(Server, Reliable)
	void DEBUG_ServerPlacePlayerAt_HundredMeters();
	UFUNCTION(Server, Reliable)
	void DEBUG_ServerPlacePlayerAt_HundredAndFiftyMeters();



private:

	UPROPERTY(VisibleAnywhere)
	bool bLeftGame{};

	
	UPROPERTY(VisibleAnywhere, Category = "PlayerCharacter|LastInstigator")
	class AController* lastInstigator{nullptr};
	float lastInstigatorRunningTime{0.f};
	UPROPERTY(EditAnywhere, Category = "PlayerCharacter|LastInstigator")
	float lastInstigatorKeepTime{ 20.f };



	UPROPERTY(EditAnywhere, Category = "PlayerCharacter|Camera")
	FVector springArmOffset_Right = FVector{ 0, 75.f, 35.f };
	UPROPERTY(EditAnywhere, Category = "PlayerCharacter|Camera")
	FVector springArmOffset_Left = FVector{ 0, -75.f, 35.f };
	UPROPERTY(EditAnywhere, Category = "PlayerCharacter|Camera")
	float springArmStandingZ = 35.f;
	UPROPERTY(EditAnywhere, Category = "PlayerCharacter|Camera")
	float springArmCrouchingZ = 10.f;
	UPROPERTY(EditAnywhere, Category = "PlayerCharacter|Camera")
	float springArmInterpSeed = 30.f;
	bool bSpringArmIsRight = true;

	class UCharacterMovementComponent* characterMovement{};

	UPROPERTY(VisibleAnywhere, Category = "PlayerCharacter|Camera")
	class USpringArmComponent* springArmComponent{};
	UPROPERTY(VisibleAnywhere, Category = "PlayerCharacter|Camera")
	class UCameraComponent* cameraComponent{};

	UPROPERTY(EditAnywhere, Category = "PlayerCharacter|HUD", BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent* overheadWidget{};

	// OnRep only gets called on machines that got this updated from others, not on the one that set it first
	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_overlappingWeapon)
	class AWeapon* overlappingWeapon{};
	UFUNCTION()
	void OnRep_overlappingWeapon(class AWeapon* lastWeapon);

	/*
	* Player Components
	*/

	UPROPERTY(VisibleAnywhere)
	class UCombatComponent* combatComponent{};

	UPROPERTY(VisibleAnywhere)
	class UBuffComponent* buffComponent{};

	UPROPERTY(VisibleAnywhere)
	class ULagCompensationComponent* lagCompensationComponent{};

	/*
	* 
	*/


	UPROPERTY(VisibleAnywhere)
	class UStaticMeshComponent* grenadeInHand{};

	UPROPERTY()
	class AMainPlayerState* mainPlayerState{};

	// Tries to get relevant classes, and then initializes them
	void PollInit();

	/*
	* Calculations for Turning and Animations
	*/

	float AO_Yaw{};
	float InterpAO_Yaw{};
	float AO_Pitch{};
	float speed_{};
	FVector velocity_{};
	bool bIsInAir_{};
	FRotator baseAimRotation_{};
	FRotator currentAimRotation_{};
	FRotator deltaAimRotation_{};

	ETurningInPlace turningInPlaceState{ ETurningInPlace::ETIP_NotTurning };


	/*
	* Animation
	*/

	class UAnimInstance* animInstance{};
	UPROPERTY(EditDefaultsOnly, Category = "PlayerCharacter|Montages");
	class UAnimMontage* fireWeaponMontage{};
	UPROPERTY(EditDefaultsOnly, Category = "PlayerCharacter|Montages");
	class UAnimMontage* hitReactMontage{};
	UPROPERTY(EditDefaultsOnly, Category = "PlayerCharacter|Montages");
	class UAnimMontage* deathMontage{};
	UPROPERTY(EditDefaultsOnly, Category = "PlayerCharacter|Montages");
	class UAnimMontage* reloadMontage{};
	UPROPERTY(EditDefaultsOnly, Category = "PlayerCharacter|Montages");
	class UAnimMontage* throwGrenadeMontage{};
	UPROPERTY(EditDefaultsOnly, Category = "PlayerCharacter|Montages");
	class UAnimMontage* swapWeaponsMontage{};

	const FName FireWeaponMontageSection_RifleAim{ "RifleAim" };
	const FName FireWeaponMontageSection_RifleHip{ "RifleHip" };


	bool bRotateRootBone{};

	/*
	* For Simulated Proxies
	*/
	UPROPERTY(EditAnywhere, Category = "PlayerCharacter|SimProxy")
	float proxyTurnThreshold{ 0.5f };
	float proxyYawDifference{};
	FRotator proxyThisFrameRotation{};
	FRotator proxyLastFrameRotation{};
	float timeSinceLastSimuledProxyTurn{};



	/*
	*  Health
	*/
	UPROPERTY(EditDefaultsOnly, Category = "PlayerCharacter|Health")
	float maxHealth{ 100.f };
	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_currentHealth, Category = "PlayerCharacter|Health")
	float currentHealth{ 100.f };
	UFUNCTION()
	void OnRep_currentHealth(float lastHealth);
	UPROPERTY(VisibleAnywhere, Category = "PlayerCharacter|Health")
	bool bIsDead{ false };

	UPROPERTY(EditDefaultsOnly, Category = "PlayerCharacter|Health")
	float maxShield{ 100.f };
	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_currentShield, Category = "PlayerCharacter|Health")
	float currentShield{ 0.f };
	UFUNCTION()
	void OnRep_currentShield();
	UPROPERTY(EditDefaultsOnly, Category = "PlayerCharacter|Health")
	float shieldDamageAbsorbRatio{ 0.75f };

	void UpdateHUD_Shield();

	UPROPERTY()
	class AMainPlayerController* playerController{};

	void SetHealth(float health,
		class AController* instigatorController = (AController*)nullptr,
		AActor* damageCauser = (AActor*)nullptr,
		bool bHeadshot = false);

	void SetShield(float shield);
	void CalculateDamage(float& damage, float& outNewHealth, float& outNewShield);

	void UpdateHUD_Health();



	UPROPERTY(EditAnywhere, Category = "PlayerCharacter|Respawn")
	float respawnDelayTime{ 5.f };
	FTimerHandle respawnTimerhandle{};

	/*
	* Dissolve Effect
	*/

	UPROPERTY(VisibleAnywhere)
	UTimelineComponent* dissolveTimelineComponent{};

	FOnTimelineFloat DissolveTrack{};

	UPROPERTY(EditAnywhere, Category = "PlayerCharacter|Cosmetic|Dissolve")
	UCurveFloat* dissolveCurve{};

	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue);
	void StartDissolve();

	/*	
	* Team specific
	*/

	UPROPERTY
	(EditAnywhere, Category = "PlayerCharacter|Team|Cosmetic", meta = (DisplayName = "Blue Team Base Material"))
	class UMaterialInstance* baseMaterialInstance_BlueTeam{};
	UPROPERTY
	(EditAnywhere, Category = "PlayerCharacter|Team|Cosmetic", meta = (DisplayName = "Red Team Base Material"))
	class UMaterialInstance* baseMaterialInstance_RedTeam{};
	UPROPERTY
	(EditAnywhere, Category = "PlayerCharacter|Team|Cosmetic", meta = (DisplayName = "Free For All Base Material"))
	class UMaterialInstance* baseMaterialInstance_NoTeam{};



	UPROPERTY
	(EditAnywhere, Category = "PlayerCharacter|Team|Cosmetic|Dissolve", meta = (DisplayName = "Blue Team Dissolve Material"))
	class UMaterialInstance* dissolveMaterialInstance_BlueTeam{};
	UPROPERTY
	(EditAnywhere, Category = "PlayerCharacter|Team|Cosmetic|Dissolve", meta = (DisplayName = "Red Team Dissolve Material"))
	class UMaterialInstance* dissolveMaterialInstance_RedTeam{};
	UPROPERTY
	(EditAnywhere, Category = "PlayerCharacter|Team|Cosmetic|Dissolve", meta = (DisplayName = "Free For All Dissolve Material"))
	class UMaterialInstance* dissolveMaterialInstance_NoTeam{};



	/*
	* 
	*/


	
	UPROPERTY(VisibleAnywhere, Category = "PlayerCharacter|Cosmetic")
	class UMaterialInstanceDynamic* baseMaterialInstanceDynamic{};

	UPROPERTY(VisibleAnywhere, Category = "PlayerCharacter|Cosmetic|Dissolve")
	class UMaterialInstanceDynamic* dissolveMaterialInstanceDynamic{};
	UPROPERTY(VisibleAnywhere, Category = "PlayerCharacter|Team|Cosmetic|Dissolve")
	class UMaterialInstance* dissolveMaterialInstance{};

	UPROPERTY(EditAnywhere, Category = "PlayerCharacter|Cosmetic|Dissolve")
	class UParticleSystem* dissolveBotParticleSystem{};
	UPROPERTY()
	class UParticleSystemComponent* dissolveBotParticleSystemComponent{};

	UPROPERTY(EditAnywhere, Category = "PlayerCharacter|Cosmetic|Dissolve")
	class USoundBase* dissolveBotSoundEffect{};
	UPROPERTY(EditAnywhere, Category = "PlayerCharacter|Cosmetic|Dissolve")
	float botZ_Offset = 200.f;


	UPROPERTY(EditDefaultsOnly, Category = "PlayerCharacter|Cosmetic|TopScoreCrown")
	class UNiagaraSystem* crownNiagaraSystem{};

	UPROPERTY()
	class UNiagaraComponent* crownNiagaraSystemComponent{};


	UPROPERTY(ReplicatedUsing = OnRep_gameplayDisabled)
	bool bGameplayDisabled{ false };
	UFUNCTION()
	void OnRep_gameplayDisabled();


	UPROPERTY(ReplicatedUsing = OnRep_playerDisplayName)
	FString playerDisplayName{};
	UFUNCTION()
	void OnRep_playerDisplayName();


	/*
	* Box components for Server-side rewind
	* SSRB -> Server-Side Rewind Box
	*/
	UPROPERTY(VisibleAnywhere)
	class UBoxComponent* SSRB_head{};
	UPROPERTY(VisibleAnywhere)
	class UBoxComponent* SSRB_pelvis{};
	UPROPERTY(VisibleAnywhere)
	class UBoxComponent* SSRB_spine_02{};
	UPROPERTY(VisibleAnywhere)
	class UBoxComponent* SSRB_spine_03{};
	UPROPERTY(VisibleAnywhere)
	class UBoxComponent* SSRB_upperarm_l{};
	UPROPERTY(VisibleAnywhere)
	class UBoxComponent* SSRB_upperarm_r{};
	UPROPERTY(VisibleAnywhere)
	class UBoxComponent* SSRB_lowerarm_l{};
	UPROPERTY(VisibleAnywhere)
	class UBoxComponent* SSRB_lowerarm_r{};
	UPROPERTY(VisibleAnywhere)
	class UBoxComponent* SSRB_hand_l{};
	UPROPERTY(VisibleAnywhere)
	class UBoxComponent* SSRB_hand_r{};
	UPROPERTY(VisibleAnywhere)
	class UBoxComponent* SSRB_thigh_l{};
	UPROPERTY(VisibleAnywhere)
	class UBoxComponent* SSRB_thigh_r{};
	UPROPERTY(VisibleAnywhere)
	class UBoxComponent* SSRB_calf_l{};
	UPROPERTY(VisibleAnywhere)
	class UBoxComponent* SSRB_calf_r{};
	UPROPERTY(VisibleAnywhere)
	class UBoxComponent* SSRB_foot_l{};
	UPROPERTY(VisibleAnywhere)
	class UBoxComponent* SSRB_foot_r{};

	UPROPERTY()
	TMap<FName, UBoxComponent*> SSRBoxes{};

	void CreateAllSSRB();
	


	void FallDamage();

	UPROPERTY(EditAnywhere, Category = "PlayerCharacter|FallDamage")
	float fallDamage_Start_Z{ 1000.f };
	UPROPERTY(EditAnywhere, Category = "PlayerCharacter|FallDamage")
	float fallDamage_SecondDamagePhase_Z{ 1800.f };
	UPROPERTY(EditAnywhere, Category = "PlayerCharacter|FallDamage")
	float fallDamage_ThirdDamagePhase_Z{ 2700.f };
	UPROPERTY(EditAnywhere, Category = "PlayerCharacter|FallDamage")
	float fallDamageDivider_FirstPhase{ 15.f };
	UPROPERTY(EditAnywhere, Category = "PlayerCharacter|FallDamage")
	float fallDamageDivider_SecondPhase{ 35.f };
	UPROPERTY(EditAnywhere, Category = "PlayerCharacter|FallDamage")
	float fallDamageDivider_ThirdPhase{ 60.f };


	UPROPERTY(EditDefaultsOnly, Category = "PlayerCharacter|FallDamage|Sound")
	class USoundBase* fallDamageSound_Light{};
	UPROPERTY(EditDefaultsOnly, Category = "PlayerCharacter|FallDamage|Sound")
	class USoundBase* fallDamageSound_Hard{};
	UPROPERTY(EditDefaultsOnly, Category = "PlayerCharacter|FallDamage|Sound")
	float fallDamageHardSoundThresholdDamage{ 30.f };


	


	void PlayFallDamageSound(bool bHard);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayFallDamageSound(bool bHard);

	bool bInAirLastFrame{ false };
	bool fallDamage_CountingZ{ false };
	float fallDamage_WorldZWhenStartedFalling{};


public:
	void SetOverlappingWeapon(class AWeapon* weapon);
	bool EqualsOverlappingWeapon(class AWeapon* weapon);
	bool HasWeaponEquipped() const;
	bool HasSecondaryWeapon() const;
	
	bool HasShotgunEquipped() const;
	bool IsAiming() const;


	FORCEINLINE bool HasLeftGame() { return bLeftGame; }

	FORCEINLINE TMap<FName, UBoxComponent*>& Get_SSRBoxes() { return SSRBoxes; }

	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
	class AWeapon* GetEquippedWeapon() const;
	class AWeapon* GetSecondaryWeapon() const;
	bool EqualsEquippedWeapon(AWeapon* weapon) const;

	FORCEINLINE ETurningInPlace GetTurningInPlaceState() const { return turningInPlaceState; }

	FRotator GetRightHandTargetRotation() const;

	FORCEINLINE class UCameraComponent* GetPlayerCamera() const { return cameraComponent; }

	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }

	FORCEINLINE bool IsDead() const { return currentHealth<=0 || bIsDead; }
	FORCEINLINE bool IsDeadOrGameplayDisabled() const { return IsDead() || IsGameplayDisabled(); }

	FORCEINLINE void SetMouseSensitivity(const float& newSensitivity) { mouseSensitivity = newSensitivity; }

	void DissolveEffect();

	void DisableInputAndCollision();

	

	FORCEINLINE float GetCurrentHealth() const { return currentHealth; }
	FORCEINLINE float GetMaxHealth() const { return maxHealth; }
	FORCEINLINE float GetCurrentShield() const { return currentShield; }
	FORCEINLINE float GetMaxShield() const { return maxShield; }
	ECombatState GetCombatState();




	
	bool IsReloading() const;
	bool IsUnoccupied() const;
	bool CanReload() const;

	FORCEINLINE class UCombatComponent* GetCombatComponent() const { return combatComponent; }
	FORCEINLINE class UBuffComponent* GetBuffComponent() const { return buffComponent; }
	FORCEINLINE class ULagCompensationComponent* GetLagCompensationComponent() const { return lagCompensationComponent; }


	FORCEINLINE bool IsPlayerHealthFull() const { return currentHealth >= maxHealth; }


	float GetServerTime();
	float GetSingleTripTime();

	FORCEINLINE bool IsGameplayDisabled() const { return bGameplayDisabled; }


	float GetCurrentInaccuracy();

	FVector GetGrenadeInHandPosition();

	FORCEINLINE class AController* GetLastInstigatorController() { return lastInstigator; }
	
};
