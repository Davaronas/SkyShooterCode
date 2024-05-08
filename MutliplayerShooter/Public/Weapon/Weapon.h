// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon/WeaponTypes.h"
#include "HelperTypes/ServerSideRewindTypes.h"
#include "Interface/InstigatorTypeInterface.h"

#include "Weapon.generated.h"



UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	EWS_Initial UMETA(DisplayName = "Initial"),
	EWS_Equipped UMETA(DisplayName = "Equipped"),
	EWS_Stored UMETA(DisplayName = "Stored"),
	EWS_Dropped UMETA(DisplayName = "Dropped"),
	EWS_MAX UMETA(DisplayName = "DefaultMax")
};





#define MUZZLE_FLASH_SOCKET FName{"MuzzleFlash"}
#define AMMO_EJECT_SOCKET FName{"AmmoEject"}

UCLASS()
class MUTLIPLAYERSHOOTER_API AWeapon : public AActor, public IInstigatorTypeInterface
{
	GENERATED_BODY()
	
public:	
	AWeapon();
	virtual void Tick(float DeltaTime) override;
	void ShowPickupWidget(bool bShow);
	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override;

	virtual void Fire(const FVector& shootFromPos, const FVector& targetPos);
	void FireBase();
	virtual void Fire(const FVector& shootFromPos, const FVector& targetPos,
		FFireData& outFireData, FServerSideRewindData& outSSRData,FVector& outActualHitLoc);
	virtual void Fire(const FVector& shootFromPos, const FVector& targetPos,
		TArray<FFireData>& outFireDatas, TArray<FServerSideRewindData>& outSSRDatas);

	virtual void Fire(const FVector& shootFromPos, const FVector& targetPos,
		const FFireData& fireData);
	virtual void Fire(const FVector& shootFromPos, const FVector& targetPos,
		const TArray<FFireData>& fireDatas);


	virtual const FName& GetInstigatorTypeName() {return instigatorTypeName;};

	void PlayWeaponAnimation();
	void PlayFireSound();
	void SpawnMuzzleFlashParticles();
	void SpawnCasing();
	void Drop();
	void HUD_UpdateCurrentAmmo();
	virtual void OnRep_Owner() override;

	//Server
	FVector AddScatterToTargetVector
	(const FVector& start, const FVector& target);
	FVector AddScatterToTargetVector
	(const FVector& start, const FVector& target, FFireData& outFireData);
	FVector AddScatterToTargetVector
	(const FVector& start, const FVector& target, float accuracy, const FFireData& overrideFireData);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly)
	FName instigatorTypeName{ TEXT("Unknown Instigator Type") };


	UPROPERTY(EditDefaultsOnly)
	float disAllowShootingIfTooCloseDistance{ 100.f };

	UFUNCTION()
	virtual void OnSphereOverlap(
		class UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		class UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	virtual void OnSphereOverlapExit(
		class UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		class UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	const class USkeletalMeshSocket* muzzleFlashSocket{};
	FTransform muzzleFlashSocketTransform{};

	void TryGetMuzzleFlashSocket();

	UPROPERTY()
	class APlayerCharacter* ownerPlayer{};

	/*
	* Scatter - Inaccuracy
	*/
	
	const float baseScatter{ 1.2f };
	const float randomScatterAdditionalRange{ 0.8f };


	// PROJECT SETTINGS CUSTOM DEPTH: ENABLED WITH STENCIL
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Effects")
	EWeaponOutlineDepthType weaponDepthType{};
	void EnableCustomDepth(bool bState);
	void SetCustomDepthAndEnable();

	UPROPERTY(EditAnywhere, Category = "Weapon|WeaponAttributes")
	bool bUseServerSideRewind{ false };


	UPROPERTY(EditAnywhere, Category = "Weapon|WeaponAttributes")
	float baseDamage{ 22.f };

	UPROPERTY(EditAnywhere, Category = "Weapon|WeaponAttributes|Speed")
	float normalWalkSpeed{450.f};
	UPROPERTY(EditAnywhere, Category = "Weapon|WeaponAttributes|Speed")
	float aimWalkSpeed{ 350.f };
	UPROPERTY(EditAnywhere, Category = "Weapon|WeaponAttributes|Speed")
	float crouchWalkSpeed{ 300.f };

private:
	UPROPERTY(VisibleAnywhere)
	class USkeletalMeshComponent* weaponMesh{};

	UPROPERTY(VisibleAnywhere)
	class USphereComponent* sphereComp{};

	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_weaponState)
	EWeaponState weaponState{EWeaponState::EWS_Initial};
	UFUNCTION()
	void OnRep_weaponState();
	
	UPROPERTY(EditDefaultsOnly)
	class UWidgetComponent* pickupWidget{};

	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Effects")
	class UAnimationAsset* fireAnimAsset{};

	// This doesn't need to be set if the animation plays this effect
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Effects")
	class UParticleSystem* muzzleFlashParticles{};
	// This doesn't need to be set if the animation plays this effect
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Effects")
	class USoundBase* fireSound{};

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class ACasing> casingToSpawn{};

	const class USkeletalMeshSocket* ammoEjectSocket {};

	
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|WeaponAttributes|Zoom")
	bool bUseUnrotatedSocket{ false };
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|WeaponAttributes|Zoom")
	bool bUseScope{ false };
	UPROPERTY(EditAnywhere, Category = "Weapon|WeaponAttributes|Zoom")
	float zoomFOV{ 60.f };
	UPROPERTY(EditAnywhere, Category = "Weapon|WeaponAttributes|Zoom")
	float zoomInterpSpeed{ 20.f };

	UPROPERTY(EditAnywhere, Category = "Weapon|WeaponAttributes")
	float oneShotInaccuracyIncrease = 0.08f;
	UPROPERTY(EditAnywhere, Category = "Weapon|WeaponAttributes")
	EWeaponType weaponType{};
	UPROPERTY(EditAnywhere, Category = "Weapon|WeaponAttributes")
	EWeaponReloadAnimationType weaponReloadType{};
	UPROPERTY(EditAnywhere, Category = "Weapon|WeaponAttributes")
	bool bAutomatic{};
	UPROPERTY(EditAnywhere, Category = "Weapon|WeaponAttributes")
	float timeBetweenShots{ 0.1f };
	UPROPERTY(EditAnywhere, Category = "Weapon|WeaponAttributes")
	float baseInaccuracyIncrease{ 0.f };
	UPROPERTY(EditAnywhere, Category = "Weapon|WeaponAttributes")
	float movementInaccuracyMultiplier{ 1.f };
	UPROPERTY(EditAnywhere, Category = "Weapon|WeaponAttributes")
	float shootingInaccuracyDecreaseInterpSpeed{ 5.f };
	UPROPERTY(EditAnywhere, Category = "Weapon|WeaponAttributes")
	float aimingInaccuracyDecrease = 0.25f;
	UPROPERTY(EditAnywhere, Category = "Weapon|WeaponAttributes")
	float crouchingInaccuracyDecrease = 0.25f;
	
	UPROPERTY(EditAnywhere, Category = "Weapon|WeaponAttributes")
	int32 magazineCapacity{20};

	//UPROPERTY(ReplicatedUsing = OnRep_currentAmmo, VisibleAnywhere, Category = "Weapon|WeaponAttributes")
	UPROPERTY(VisibleAnywhere, Category = "Weapon|WeaponAttributes")
	int32 currentAmmo{};
	/*UFUNCTION()
	void OnRep_currentAmmo();*/

	void SpendRound();

	int32 ammoRequestsSequence = 0;

	UFUNCTION(Client, Reliable)
	void ClientSpendAmmo(const int32& ammo);

	UFUNCTION(Client, Reliable)
	void ClientSetAmmo(const int32& ammo);

	UFUNCTION(Client, Reliable)
	void ClientAddToAmmo(const int32& ammo);





	UPROPERTY(EditAnywhere, Category = "Weapon|Effects")
	bool bSimulateCosmeticPhysicsOnWeaponMesh{ false };
	UPROPERTY(EditAnywhere, Category = "Weapon|Effects")
	class USoundBase* equipWeaponSound{};

	void TryReloadIfEmpty();



	void TryGetPlayer();

	UPROPERTY()
	class AMainPlayerController* ownerMainPlayerController{};

	UPROPERTY()
	class APickupSpawn* spawner{};

	float server_LastFirePacketClientTime{ -1.f };

public:
	FORCEINLINE void SetSpawner(APickupSpawn* spawn) { spawner = spawn; }

	FORCEINLINE float GetLastClientPacketTime() 
	{
		if (!HasAuthority()) { return -1; }
		return server_LastFirePacketClientTime;
	}
	FORCEINLINE void SetLastClientPacketTime(const float& packetTime)
	{
		if (!HasAuthority()) { return; }
		server_LastFirePacketClientTime = packetTime; }


	void SetWeaponState(EWeaponState state);
	void CheckEnableCosmeticPhysics();
	void EnableMeshCollisionAndPhysics();
	void HandleCosmeticPhysicsOnPhysicsSimulated();
	void DisableMeshCollisionAndPhysics();

	




	void SetCurrentAmmo(const int32& ammo);
	void AddToCurrentAmmo(const int32& ammo);
	FORCEINLINE class USphereComponent* GetSphereComponent() const { return sphereComp; }
	FORCEINLINE class USkeletalMeshComponent* GetWeaponMesh() const { return weaponMesh; }
	FORCEINLINE float GetZoomedFOV() const { return zoomFOV; }
	FORCEINLINE float GetZoomInterpSpeed() const { return zoomInterpSpeed; }
	FORCEINLINE float GetOneShotInaccuracyIncrease() const { return oneShotInaccuracyIncrease; }
	FORCEINLINE float GetMovementInaccuracyMultiplier() const { return movementInaccuracyMultiplier; }
	FORCEINLINE float GetCrouchingInaccuracyDecrease() const { return crouchingInaccuracyDecrease; }
	FORCEINLINE float GetAimingInaccuracyDecrease() const { return aimingInaccuracyDecrease; }

	FORCEINLINE bool IsAutomatic() const { return bAutomatic; }
	FORCEINLINE float GetTimeBetweenShots() const { return timeBetweenShots; }
	FORCEINLINE float GetBaseInaccuracyIncrease() const { return baseInaccuracyIncrease; }
	FORCEINLINE float GetInaccuracyDecreaseSpeed() const { return shootingInaccuracyDecreaseInterpSpeed; }
	FORCEINLINE int32 GetCurrentAmmo() const { return currentAmmo; }
	FORCEINLINE int32 GetMagSize() const { return magazineCapacity; }
	FORCEINLINE bool HasCurrentAmmo() const { return currentAmmo > 0; }
	FORCEINLINE bool IsEmpty() const { return !HasCurrentAmmo(); }
	FORCEINLINE bool IsFull() const { return currentAmmo >= magazineCapacity; }
	FORCEINLINE bool IsShotgun() const { return weaponType == EWeaponType::EWT_Shotgun; }
	FORCEINLINE bool UsesServerSideRewind()const { return bUseServerSideRewind; }

	FORCEINLINE void GetWeaponMovementSpeed(float& normalWalkSpeed_, float& aimWalkSpeed_, float& crouchWalkSpeed_)
	{
		normalWalkSpeed_ = normalWalkSpeed;
		aimWalkSpeed_ = aimWalkSpeed;
		crouchWalkSpeed_ = crouchWalkSpeed;
	}

	virtual bool IsHitScan(){ return false; }

	FVector GetMuzzleFlashLocation();


	FORCEINLINE float GetDisAllowShootingIfTooCloseDistance() { return disAllowShootingIfTooCloseDistance; }

	FORCEINLINE EWeaponType GetWeaponType() const { return weaponType; }
	FORCEINLINE EWeaponReloadAnimationType GetWeaponReloadAnimationType() const { return weaponReloadType; }

	FORCEINLINE class USoundBase* GetEquippedWeaponSound() const { return equipWeaponSound; }

	// HasAuthority() && !IsLocallyControlled()
	bool IsOwnerNotLocallyControlledServer() const;
	bool IsOwnerLocallyControlled() const;
	bool IsOwnerServer() const;
	bool IsOwnerAutonomousProxy() const;

	FORCEINLINE bool HasScope() const { return bUseScope; }
	FORCEINLINE bool UseRotatedRightHandSocket() const { return bUseUnrotatedSocket; }

//	FORCEINLINE class USkeletalMeshComponent* GetWeaponMesh() {return weaponMesh;}


	// Textures for crosshair
	UPROPERTY(EditAnywhere, Category = "Crosshair")
	class UTexture2D* crosshairCenter;
	UPROPERTY(EditAnywhere, Category = "Crosshair")
	UTexture2D* crosshairLeft;
	UPROPERTY(EditAnywhere, Category = "Crosshair")
	UTexture2D* crosshairRight;
	UPROPERTY(EditAnywhere, Category = "Crosshair")
	UTexture2D* crosshairTop;
	UPROPERTY(EditAnywhere, Category = "Crosshair")
	UTexture2D* crosshairBottom;
	UPROPERTY(EditAnywhere, Category = "Crosshair")
	float crosshairTraceCheckBoxSize{ 2.f };

	FORCEINLINE float GetCrosshairTraceCheckBoxSize() const { return crosshairTraceCheckBoxSize; }



	static const FName WeaponReloadTypeAsFName(EWeaponReloadAnimationType type);
};
