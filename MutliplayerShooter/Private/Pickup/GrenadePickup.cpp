// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickup/GrenadePickup.h"
#include "PlayerCharacter.h"
#include "PlayerComponents/CombatComponent.h"

void AGrenadePickup::OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	APlayerCharacter* player = Cast<APlayerCharacter>(OtherActor);
	if (!player) { return; }
	UCombatComponent* combatComp = player->GetCombatComponent();
	if (!combatComp) { return; }

	combatComp->Pickup_Grenade(grenadesToGive);

	Destroy();
}
