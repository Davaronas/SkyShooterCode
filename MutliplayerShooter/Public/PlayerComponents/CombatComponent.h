// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HUD/MainHUD.h"
#include "Weapon/WeaponTypes.h"
#include "HelperTypes/CombatState.h"
#include "HelperTypes/ServerSideRewindTypes.h"


#include "CombatComponent.generated.h"




UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MUTLIPLAYERSHOOTER_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	

#if WITH_EDITORONLY_DATA

	UPROPERTY(EditAnywhere, Category = "DEBUG")
	bool bDEBUG_InfiniteAmmo{ false };

#endif

	UCombatComponent();
	virtual void TickComponent
	(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void HandleJumpDelay(float DeltaTime);
	void SetCrosshairPosition();
	void EquipWeapon(class AWeapon* weapon, bool bButtonHeld = false);
	void MovePrimaryWeaponToSecondary();
	void SetPrimaryWeapon(AWeapon* weapon);
	bool EquippedWeaponHasScope();
	void AttachSecondaryWeaponToSecondarySocket();
	void SetSecondaryWeapon(AWeapon* weapon);
	void SetMovementCompToHandleWeapon();
	void ReloadIfEmpty();
	void SetCarriedAmmoToEquippedWeaponType();
	void SetEquippedWeapon(AWeapon* weapon);
	void DropEquippedWeapon();
	void DropSecondaryWeapon();
	void AttachWeaponToRightHand();

	void ClientAutonomousProxyProjectileEarlyDestroyed();


	UPROPERTY(VisibleAnywhere, Replicated)
	bool bCanJump{ true };

	bool bCanBufferJump{ false };


	
	void PlayEquippedWeaponEquipSound();
	void PlaySecondaryWeaponEquipSound();

	void HUD_UpdateCarriedAmmo();
	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override;


	friend class APlayerCharacter;

	void DisableGameplay();

	void Pickup_Ammo(const EWeaponType& ammoType, const int32& ammoAmount);
	void Pickup_Grenade(const int32& grenadeAmount);

	void IncreaseSpeed(float amount);
	void ResetSpeed();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastIncreaseSpeed(float amount);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastResetSpeed();

	float GetCalculateInaccuracy();


	// Call server fire when Local Firing, client time will be not checked here
	void RequestCombatComponentToRecreateProjectileShot(const FServerSideProjectileRewindData& projectileData, const FFireData& fireData,
		class AProjectileWeapon* projectileWeapon);

	UFUNCTION(Server, Reliable)
	void ServerRecreateProjectileShot(const FServerSideProjectileRewindData& projectileData, const FFireData& fireData,
		class AProjectileWeapon* projectileWeapon);


protected:
	virtual void BeginPlay() override;
	void SetNormalWalkSpeed();
	void SetAiming(bool state);
	void ShowSniperScopeWidget(bool state, bool bForce = false);

	void DecideWalkSpeedOnAiming(bool state);
	void SetAimWalkSpeed();

	UPROPERTY(VisibleAnywhere)
	bool bDisallowFireWeaponTooCloseToObject{ false };
	bool bDisallowFireWeaponTooCloseToObject_Calculation{ false };
	bool bDisallowFireWeaponTooCloseToObject_LastFrame{ false };
	UFUNCTION(Server, Reliable)
	void ServerFireBlockedChanged(bool newState);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastFireBlockedChanged(bool newState);


	UPROPERTY(EditDefaultsOnly)
	float timeBetweenJumps = 0.2f;
	float timeBetweenJumpsRunningTime{};
	



	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool state);

	void FireButtonPressed(bool state);
	void TryFire();

	/*
	* @param weaponFireDelay	Needed for server side validation, this is not checked server side, so we need to validate
	* @param currentClientTime	Needed for server side rewind validation, if client sent out packets faster than weapon delay, then the client is cheating
	*/
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerFire
	(const FVector_NetQuantize& shootFromPos, const FVector_NetQuantize& hitTarget,
		class AWeapon* firedWeapon, /*const float& clientTimeBetweenShots,*/ const float& currentClientTime);


	UFUNCTION(Server, Reliable)
	void ServerRecreateHitScanShot
	(const FServerSideRewindData& rewindData, const FFireData& fireData,
		class AHitScanWeapon* hitScanWeapon, const float& currentClientTime);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerRecreateShotgunHitScanShots
	(const TArray<FServerSideRewindData>& rewindDatas, const TArray<FFireData>& fireDatas,
		class AShotgun* shotgun, const float& currentClientTime);


	
	


	// Projectile Weapons
	void LocalFire(FVector& outActualHitLoc);

	// HitScan Weapons
	void LocalFire(FFireData& outFireData, FServerSideRewindData& outSSRData, FVector& outActualHitLoc);
	void LocalShotgunFire(TArray<FFireData>& outFireDatas, TArray<FServerSideRewindData>& outSSRDatas);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastFire(const FVector_NetQuantize& shootFromPos, const FVector_NetQuantize& hitTarget);

	void ReloadButtonPressed();
	UFUNCTION(Server, Reliable)
	void ServerReload();
	void HandleReload();

	UFUNCTION(Client, Reliable)
	void ClientLocalPlayReloadAnim();

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class AProjectile> grenadeClass{};
	void ThrowGrenadeButtonPressed();
	void HandleThrowGrenade();
	UFUNCTION(Server, Reliable)
	void ServerSpawnGrenade(const FVector_NetQuantize& hitTarget);

	UFUNCTION(Server, Reliable)
	void ServerThrowGrenade();
	UFUNCTION(BlueprintCallable)
	void FinishThrowGrenade();
	UFUNCTION(BlueprintCallable)
	void ReleaseGrenadeFromHand();

	void AttachWeaponToLeftHand();
	void AttachWeaponToLeftHand_L();
	void AttachWeaponToLeftHand_S();


	void SwapWeaponsButtonPressed();

	UFUNCTION(Server, Reliable)
	void ServerSwapWeapons();

	UFUNCTION(BlueprintCallable)
	void SwapEquippedAndSecondaryWeapons();
	UFUNCTION(BlueprintCallable)
	void FinishSwapWeapons();
	UFUNCTION(NetMulticast, Reliable)
	void MulticastSwapEquippedAndSecondaryWeapons();

	

	float lastTraceDistance{};
	void TraceUnderCrosshairs(struct FHitResult& hitResult);
	void SetCrosshairColour(const FHitResult& hitResult, bool bBlocked = false);
	void SetRightHandLookRotator(const FVector& lookPoint);
	void SetHUDCrosshair(float DeltaTime);
	void CalculateInaccuracy(float DeltaTime);
	
	void InterpFOV(float DeltaTime);


	FTimerHandle fireTimerHandle{};
	void StartFireTimer();
	void FireTimerFinished();

	void AutomaticFireCheck();

	void SpendHandGrenade();

private:
	/* This is set in PlayerCharacter PostInitializeComponents*/
	class APlayerCharacter* ownerPlayer{};
	class UCharacterMovementComponent* playerMovementComp{};
	class AMainPlayerController* playerController{};
	class AMainHUD* playerHUD{};

	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_combatState)
	ECombatState combatState{ ECombatState::ECS_Unoccupied };
	UFUNCTION()
	void OnRep_combatState();

	UPROPERTY(VisibleAnywhere)
	bool bIsLocallyReloading{ false }; 
	UPROPERTY(VisibleAnywhere)
	bool bIsLocallySwappingWeapons{ false };


	class ULagCompensationComponent* lagComp{};




	UPROPERTY(EditAnywhere)
	float screenTraceRange = 99999.f;
	UPROPERTY(EditAnywhere)
	float screenTraceFailEndDistance = 5000.f;


	void EnableWeaponEquipSoundGuardClient();
	void DisableWeaponEquipSoundGuardClient();
	FTimerHandle weaponEquipSoundTimer{};
	UPROPERTY(EditDefaultsOnly)
	float weaponEquipSoundGuardTime{ 0.2f };
	bool bWeaponEquipSoundGuard{ false };


	UPROPERTY(EditAnywhere, Category = "CombatComponent|Crosshair")
	float crosshairTraceCheckBoxSize{ 2.f };
	UPROPERTY(EditAnywhere, Category = "CombatComponent|Crosshair")
	bool bDrawDebugCrosshairTrace{ false };


	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_equippedWeapon)
	class AWeapon* equippedWeapon{};
	UFUNCTION()
	void OnRep_equippedWeapon();

	void HandleScopeOnEquip();

	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_secondaryWeapon)
	class AWeapon* secondaryWeapon{};
	UFUNCTION()
	void OnRep_secondaryWeapon();

	void SetWeaponAttributes();

	UPROPERTY(VisibleAnywhere, Replicated)
	bool bIsAiming{false};
	bool bIsAimingLocally{ false };
	UFUNCTION()
	void OnRep_bIsAiming();

	UPROPERTY(VisibleAnywhere)
	bool bFireButtonPressed{};


	UPROPERTY(VisibleAnywhere)
	float normalWalkSpeed{ 0.f };
	UPROPERTY(EditAnywhere)
	float baseNormalWalkSpeed{ 650.f };
	UPROPERTY(VisibleAnywhere)
	float aimWalkSpeed{ 0.f };
	UPROPERTY(EditAnywhere)
	float baseAimWalkSpeed{ 450.f };
	UPROPERTY(VisibleAnywhere)
	float crouchWalkSpeed{ 0.f };
	UPROPERTY(EditAnywhere)
	float baseCrouchWalkSpeed{ 300.f };




	UPROPERTY(EditAnywhere, Category = "CombatComponent|Accuracy")
	float baseInaccuracy = 0.25f;
	UPROPERTY(VisibleAnywhere, Category = "CombatComponent|Accuracy")
	float aimingInaccuracyDecrease = 0.25f;
	UPROPERTY(VisibleAnywhere, Category = "CombatComponent|Accuracy")
	float crouchingInaccuracyDecrease = 0.25f;
	UPROPERTY(EditAnywhere, Category = "CombatComponent|Accuracy")
	float inAirInaccuracyIncrease = 0.3f;
	UPROPERTY(VisibleAnywhere, Category = "CombatComponent|Accuracy")
	float inaccuracy{};
	UPROPERTY(VisibleAnywhere, Category = "CombatComponent|Accuracy")
	float shootingInaccuracy{};
	UPROPERTY(VisibleAnywhere, Category = "CombatComponent|Accuracy")
	float shootingInaccuracyDecreaseInterpSpeed{ 5.f };
	UPROPERTY(VisibleAnywhere, Category = "CombatComponent|Accuracy")
	float movementInaccuracyMultiplier{ 1.f };

//	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FRotator rightHandToTargetRotation{};
	const class USkeletalMeshSocket* rightHandSocket{};
	const class USkeletalMeshSocket* rightHandSocket_Rotated{};
	const USkeletalMeshSocket* leftHandSocket_LongWeapon{};
	const USkeletalMeshSocket* leftHandSocket_LongWeapon_Rotated{};
	const USkeletalMeshSocket* leftHandSocket_ShortWeapon{};
	const USkeletalMeshSocket* secondarySocket_LongWeapon{};
	const USkeletalMeshSocket* secondarySocket_LongWeapon_Rotated{};
	const USkeletalMeshSocket* secondarySocket_ShortWeapon{};
	struct FHitResult tickTraceHitResult {};
	struct FHitResult tickTraceWeaponBlockHitResult {};

	UPROPERTY(VisibleAnywhere, Category = "CombatComponent|WeaponZoom")
	float defaultFOV{};
	UPROPERTY(EditAnywhere, Category = "CombatComponent|WeaponZoom")
	float zoomedFOV{30.f};
	UPROPERTY(EditAnywhere, Category = "CombatComponent|WeaponZoom")
	float zoomInterpSpeed{ 20.f };
	float currentFOV{};

	UPROPERTY(EditAnywhere)
	float hideCharacterDistanceToCamera = 150.f;

	UPROPERTY(VisibleAnywhere, Category = "CombatComponent|WeaponHandling")
	bool bUsingAutomatic{};
	UPROPERTY(VisibleAnywhere, Category = "CombatComponent|WeaponHandling")
	float delayBetweenShots{0.1f};
	UPROPERTY(VisibleAnywhere, Category = "CombatComponent|WeaponHandling")
	bool bCanFire{false};

	// For the currently equipped weapon
	UPROPERTY(ReplicatedUsing = OnRep_carriedAmmo, EditAnywhere, Category = "CombatComponent|Ammo")
	int32 carriedAmmo{};
	UFUNCTION()
	void OnRep_carriedAmmo();
	TMap<EWeaponType, int32> carriedAmmoMap{};

	UPROPERTY(ReplicatedUsing = OnRep_currentHandGrenades)
	int32 currentHandGrenades{ 0 };
	UFUNCTION()
	void OnRep_currentHandGrenades();

	void HUD_UpdateGrenades();

	UPROPERTY(EditAnywhere, Category = "CombatComponent|Ammo")
	int32 default_AssaultRifeAmmo{ 40 };
	UPROPERTY(EditAnywhere, Category = "CombatComponent|Ammo")
	int32 default_RocketLauncherAmmo{ 2 };
	UPROPERTY(EditAnywhere, Category = "CombatComponent|Ammo")
	int32 default_PistolAmmo{ 36 };
	UPROPERTY(EditAnywhere, Category = "CombatComponent|Ammo")
	int32 default_SMGAmmo{ 60 };
	UPROPERTY(EditAnywhere, Category = "CombatComponent|Ammo")
	int32 default_ShotgunAmmo{ 12 };
	UPROPERTY(EditAnywhere, Category = "CombatComponent|Ammo")
	int32 default_SniperRifleAmmo{ 5 };
	UPROPERTY(EditAnywhere, Category = "CombatComponent|Ammo")
	int32 default_GrenadeLauncherAmmo{ 0 };
	UPROPERTY(EditAnywhere, Category = "CombatComponent|Ammo")
	int32 default_HandGrenades{ 1 };
	void InitializeCarriedAmmoTypes();

	


	bool bShowCrosshair{ true };
	FCrosshairPackage crosshairPackage {};


	FORCEINLINE bool IsUnoccupied(){ return combatState == ECombatState::ECS_Unoccupied; }
	//FORCEINLINE bool IsReloading() { return combatState == ECombatState::ECS_Reloading; }

public:

	bool CanFire();
	
	FORCEINLINE FRotator GetRightHandTargetRotation() { return rightHandToTargetRotation; }
	bool HasCurrentAmmo();
	FORCEINLINE bool HasCarriedAmmo() { return carriedAmmo > 0; }
	bool IsWeaponFull();

	UFUNCTION(BlueprintCallable)
	void FinishReloading();
	UFUNCTION(BlueprintCallable)
	void LoadShell();
	bool HasShotgunEquipped();
	FORCEINLINE bool IsReloadingShotgun() { return HasShotgunEquipped() && IsReloading(); }
	FORCEINLINE bool IsReloading() { return combatState == ECombatState::ECS_Reloading; }

	FORCEINLINE float GetCurrentInaccuracy() { return FMath::Clamp(inaccuracy, 0, 1000.f); };
	FORCEINLINE bool HasHandGrenades() { return currentHandGrenades > 0; }

	FORCEINLINE bool IsLocallyReloading() { return bIsLocallyReloading; }
	FORCEINLINE bool IsLocallySwappingWeapons() { return bIsLocallySwappingWeapons; }
	FORCEINLINE bool IsLocallyAiming() { return bIsAimingLocally; }
	bool HasProjectileWeaponEquipped();
	bool HasProjectileWeaponEquipped(class AProjectileWeapon*& outWeapon);

	FORCEINLINE bool GetDisallowFireWeaponTooCloseToObject() { return bDisallowFireWeaponTooCloseToObject; }
	
		
	void HandleSwapWeaponState();


	// Client that is controlled locally.
	// IsLocallyControlled() && !HasAuthority()
	bool IsPlayerAutonomousProxy();
	bool IsPlayerLocallyControlled();
	bool IsPlayerServer();

	void FireEffect(const FVector_NetQuantize& shootFromPos, const FVector_NetQuantize& hitTarget);

};
