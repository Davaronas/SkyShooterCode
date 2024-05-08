// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ProjectileWeapon.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Weapon/Projectile.h"
#include "HelperTypes/ServerSideRewindTypes.h"
#include "PlayerCharacter.h"

#include "DebugMacros.h"
#include "PlayerComponents/CombatComponent.h"



// Server and sim proxies
void AProjectileWeapon::Fire(const FVector& shootFromPos, const FVector& targetPos)
{
	Super::Fire(shootFromPos, targetPos);

	if (!ownerPlayer) { return; }
	if (!ownerPlayer->HasAuthority()) { return; }
	if (!projectileToSpawnClass) { return; }
	if (!GetWeaponMesh()) { return; }
	TryGetMuzzleFlashSocket();
	if (!muzzleFlashSocket) { return; }
	if (!GetWorld()) { return; }


	FVector start = shootFromPos;
	FVector scatteredEnd{ targetPos };
	if (ownerPlayer->IsLocallyControlled() || !bUseServerSideRewind)
		scatteredEnd = start + AddScatterToTargetVector(start, targetPos);
	FRotator spawnRot { (scatteredEnd - start).Rotation() };

	
	FActorSpawnParameters spawnParams{};
	spawnParams.Owner = GetOwner();
	spawnParams.Instigator = Cast<APawn>(GetOwner());

	AProjectile* newProjectile = GetWorld()->SpawnActor<AProjectile>
		(projectileToSpawnClass,
			shootFromPos,
			spawnRot,
			spawnParams);

	if (newProjectile)
	{
		newProjectile->SetProjectileWeapon(this);
	}
}

// aut proxy
void AProjectileWeapon::AutProxyFire(const FVector& shootFromPos, const FVector& targetPos, FVector& outActualHitLoc)
{
	Super::Fire(shootFromPos, targetPos);

	if (!ownerPlayer) { return; }
	if (!projectileToSpawnClass) { return; }
	if (!GetWeaponMesh()) { return; }
	TryGetMuzzleFlashSocket();
	if (!muzzleFlashSocket) { return; }
	if (!GetWorld()) { return; }

	FFireData fireData{};
	FVector start = shootFromPos;
	FVector scatteredEnd = start + AddScatterToTargetVector(start, targetPos, fireData);
	FRotator spawnRot{ (scatteredEnd - start).Rotation() };

	outActualHitLoc = scatteredEnd;

	//DRAW_LINE_COMP_G(ownerPlayer, start, scatteredEnd);

	FActorSpawnParameters spawnParams{};
	spawnParams.Owner = GetOwner();
	spawnParams.Instigator = Cast<APawn>(GetOwner());

	AProjectile* newProjectile =
		GetWorld()->SpawnActor<AProjectile>
		(projectileToSpawnClass_AutProxy,
			shootFromPos,
			spawnRot,
			spawnParams);

	if (!newProjectile) { return; }


	FServerSideProjectileRewindData projectileData{};
	projectileData.startLoc = GetMuzzleFlashLocation(); 
	projectileData.hitLoc = targetPos;
	//projectileData.timeOfFiring = ownerPlayer->GetServerTime(); 
	projectileData.timeOfFiring = ownerPlayer->GetServerTime() + ownerPlayer->GetSingleTripTime();
	//projectileData.timeOfFiring = ownerPlayer->GetServerTime() - ownerPlayer->GetSingleTripTime();

	if (ownerPlayer)
	{
		/*if (ownerPlayer->GetCombatComponent())
		{
			GEPR_ONE_VARIABLE("Client side accuracy: %.2f",
				ownerPlayer->GetCombatComponent()->GetCalculateInaccuracy());
		}*/
	}
	newProjectile->SetProjectileRewindData(fireData, projectileData);
	newProjectile->SetProjectileWeapon(this);
}


