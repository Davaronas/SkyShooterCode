// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickup/AmmoPickup.h"
#include "PlayerCharacter.h"
#include "PlayerComponents/CombatComponent.h"

void AAmmoPickup::OnOverlap
(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	APlayerCharacter* player = Cast<APlayerCharacter>(OtherActor);
	if (!player) { return; }
	UCombatComponent* combatComp = player->GetCombatComponent();
	if (!combatComp) { return; }

	if (ammoAmount > 0)
	{
		combatComp->Pickup_Ammo(ammoType, ammoAmount);
	}

	Destroy();
}

