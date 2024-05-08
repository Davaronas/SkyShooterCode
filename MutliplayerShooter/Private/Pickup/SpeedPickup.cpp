// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickup/SpeedPickup.h"
#include "PlayerCharacter.h"
#include "PlayerComponents/BuffComponent.h"

void ASpeedPickup::OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	APlayerCharacter* player = Cast<APlayerCharacter>(OtherActor);
	if (!player) { return; }
	UBuffComponent* buffComp = player->GetBuffComponent();
	if (!buffComp) { return; }

	buffComp->SpeedUpPlayer(speedIncrease, speedIncreaseDuration);

	Destroy();
}
