// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ProjectileBullet.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/ActorComponent.h"
#include "PlayerCharacter.h"
#include "PlayerComponents/LagCompensationComponent.h"
#include "Controller/MainPlayerController.h"
#include "Weapon/HeadshotDamageType.h"
#include "PlayerComponents/CombatComponent.h"
#include "HelperTypes/ServerSideRewindTypes.h"

#include "PlayerCharacter.h"
#include "Weapon/ProjectileWeapon.h"

#include "DebugMacros.h"


AProjectileBullet::AProjectileBullet()
{
	projectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Projectile Movement"));
	projectileMovement->bRotationFollowsVelocity = true;
	projectileMovement->InitialSpeed = 8000.f;
	projectileMovement->MaxSpeed = 8000.f;
	projectileMovement->SetIsReplicated(true);
}

void AProjectileBullet::OnHit(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	FVector NormalImpulse,
	const FHitResult& Hit)
{
	if (!ownerPlayer) ownerPlayer = Cast<APlayerCharacter>(GetOwner());
	if (!ownerPlayer) { return; }
	if (!firingWeapon) { return; }

	

	APlayerCharacter* hitPlayerCharacter = Cast<APlayerCharacter>(OtherActor);

	// aut proxy
	if (!bReplicates && IsOwnerLocallyControlled())
	{
		if (hitPlayerCharacter)
		{
			projectileRewindData.playerHit = hitPlayerCharacter;
			projectileRewindData.timeOfHit = ownerPlayer->GetServerTime() + ownerPlayer->GetSingleTripTime();
			//projectileRewindData.hitLoc = Hit.ImpactPoint;
			UCombatComponent* combatComp = ownerPlayer->GetCombatComponent();
			if (combatComp)
			{
				combatComp->RequestCombatComponentToRecreateProjectileShot
				(projectileRewindData, projectileFireData, firingWeapon);
			}
		}

		DisableProjectile();
	}



	if (IsOwnerServer() && IsOwnerLocallyControlled())
	{
		if (!GetInstigator()) { return; }
		const bool bHeadshot = Hit.BoneName == FString(TEXT("head"));
		const float actualDamage = bHeadshot ? firingWeapon->GetHeadshotDamage() : firingWeapon->GetBaseDamage();

		UClass* damageType = bHeadshot ? UHeadshotDamageType::StaticClass() : UDamageType::StaticClass();

		UGameplayStatics::ApplyDamage(
			OtherActor,
			actualDamage,
			GetInstigator()->Controller,
			firingWeapon, damageType);
	}
	else if (!IsOwnerServer())
	{
		/*APlayerCharacter* playerHit = Cast<APlayerCharacter>(OtherActor);
		AMainPlayerController* playerController = ownerPlayer->GetController<AMainPlayerController>();
		ULagCompensationComponent* lagComp = ownerPlayer->GetLagCompensationComponent();

		if (lagComp && playerHit && playerController)
		{
			lagComp->ServerHitRequest_Projectile(playerHit, GetActorLocation(), Hit.ImpactPoint,
				playerController->GetServerTime() - playerController->GetSingleTripTime(), this);
		}*/
	}
	
	



	Super::OnHit(HitComponent, OtherActor, OtherComp, NormalImpulse, Hit);
}

bool AProjectileBullet::DisableProjectileInTickOnClientCondition()
{
	return projectileMovement && projectileMovement->Velocity.Size() <= 100.f;
}
