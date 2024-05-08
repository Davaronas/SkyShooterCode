// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interface/InstigatorTypeInterface.h"
#include "HelperTypes/ServerSideRewindTypes.h"
#include "Projectile.generated.h"

UCLASS()
class MUTLIPLAYERSHOOTER_API AProjectile : public AActor, public IInstigatorTypeInterface
{
	GENERATED_BODY()
	
public:	
	AProjectile();
	virtual void Tick(float DeltaTime) override;

	FORCEINLINE virtual const FName& GetInstigatorTypeName() override { return instigatorTypeName; }

	// This only gets called when explicity destroy by Destroy(), not automatically on ending game
	virtual void Destroyed() override;

protected:
	virtual void BeginPlay() override;


	UPROPERTY()
	FFireData projectileFireData{};
	UPROPERTY()
	FServerSideProjectileRewindData projectileRewindData{};
	UPROPERTY()
	class AProjectileWeapon* firingWeapon{};


	UPROPERTY(EditDefaultsOnly)
	FName instigatorTypeName{ TEXT("Unknown Instigator Type") };

	UFUNCTION()
	virtual void OnHit
	(class UPrimitiveComponent* HitComponent,
		class AActor* OtherActor,
		class UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit);

	bool IsOwnerServer();

	void PlayImpactEffects();

	virtual void OnClientTick_VelocityStopped();

	virtual void MulticastPlayImpactEffects_Override(bool bPlayer);

	virtual bool DisableProjectileInTickOnClientCondition() { return false; }
	void DisableProjectile();

	UPROPERTY(EditDefaultsOnly, Category = "Projectile|Effects")
	class UNiagaraSystem* projectileTrailNiagaraSystem;
	UPROPERTY()
	class UNiagaraComponent* projectileTrailNiagaraSystemComponent;

	void CreateProjectileTrail();
	void DeactivateTrail();

	class APlayerCharacter* ownerPlayer{};

private:


	

	UPROPERTY(VisibleAnywhere)
	class UBoxComponent* collisionBox{};
	UPROPERTY(VisibleAnywhere)
	class UStaticMeshComponent* projectileMesh{};

	

	UPROPERTY(EditDefaultsOnly, Category = "Projectile|Effects")
	class UParticleSystem* projectileTracer{};
	class UParticleSystemComponent* projectileTracerComponent{};

	UPROPERTY(EditDefaultsOnly, Category = "Projectile|Effects")
	class UParticleSystem* normalImpactParticles{};
	UPROPERTY(EditDefaultsOnly, Category = "Projectile|Effects")
	class USoundCue* normalImpactSoundEffect{};
	UPROPERTY(EditDefaultsOnly, Category = "Projectile|Effects")
	class UParticleSystem* playerImpactParticles{};
	UPROPERTY(EditDefaultsOnly, Category = "Projectile|Effects")
	class USoundCue* playerImpactSoundEffect{};
	UPROPERTY(EditDefaultsOnly, Category = "Projectile|LifeSpan")
	bool bSetLifeSpanAfterImpact{ true };
	UPROPERTY(EditDefaultsOnly, Category = "Projectile|LifeSpan")
	float lifeSpanAfterImpact{ 1.f };

	class UParticleSystem* impactParticles{};
	class USoundCue* impactSoundEffect{};
	
	UFUNCTION(Server, Unreliable)
	void ServerPlayImpactEffects(bool bPlayer);
	UFUNCTION(NetMulticast, Unreliable)
	virtual void MulticastPlayImpactEffects(bool bPlayer);
	


	bool bPlayerHit{ false };

	bool bHit{false};

	bool bTurnedOffOnClient{ false };

public:	



	bool IsOwnerLocallyControlled();
	bool IsOwnerServerAndLocallyControlled();



	FORCEINLINE void SetProjectileRewindData
	(const FFireData& fireData, const FServerSideProjectileRewindData& rewindData) 
	{ 
		projectileRewindData = rewindData;
		projectileFireData = fireData;
	}

	FORCEINLINE void SetProjectileWeapon(class AProjectileWeapon* firingWeapon_)
	{
		firingWeapon = firingWeapon_;
	}
	

};
