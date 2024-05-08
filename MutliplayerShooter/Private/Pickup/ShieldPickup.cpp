// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickup/ShieldPickup.h"
#include "PlayerCharacter.h"
#include "PlayerComponents/BuffComponent.h"

AShieldPickup::AShieldPickup()
{
	bReplicates = true;
}

void AShieldPickup::OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	APlayerCharacter* player = Cast<APlayerCharacter>(OtherActor);
	if (!player) { return; }
	UBuffComponent* buffComp = player->GetBuffComponent();
	if (!buffComp) { return; }

	buffComp->AddShield(shieldAmount);

	Destroy();
}
