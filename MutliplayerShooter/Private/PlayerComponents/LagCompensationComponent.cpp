// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerComponents/LagCompensationComponent.h"
#include "PlayerCharacter.h"
#include "Controller/MainPlayerController.h"
#include "Components/BoxComponent.h"
#include "Weapon/Weapon.h"
#include "Weapon/HitScanWeapon.h"
#include "Weapon/ProjectileWeapon.h"
#include "Weapon/Projectile.h"
#include "Kismet/GameplayStatics.h"
#include "DebugMacros.h"
#include "Weapon/HeadshotDamageType.h"
#include "../Source/MutliplayerShooter/MutliplayerShooter.h"
#include "PlayerComponents/CombatComponent.h"


ULagCompensationComponent::ULagCompensationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
	SetIsReplicated(true);
}


void ULagCompensationComponent::BeginPlay()
{
	Super::BeginPlay();
}

void ULagCompensationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!ownerPlayer) { return; }
	if (!ownerPlayer->HasAuthority()) { return; }

	if (frameLocationHistory.Num() <= 1)
	{
		SaveThisFrame();
	}
	else
	{
		DeleteExpiredFrames();
		SaveThisFrame();
	}
}




void ULagCompensationComponent::ServerHitRequest_HitScan_Implementation
(const FServerSideRewindData& debate, AHitScanWeapon* damageCauserWeapon)
{
	if (!damageCauserWeapon) { return; }

	if (!debate.playerHit) { return; }

	FVector actualHitLoc{};
	FServerSideRewindResult requestResult = ServerSideRewind
	(debate.playerHit, debate.startLoc, debate.hitLoc, debate.timeOfHit, actualHitLoc);
	if (!requestResult.bHitConfirmed) { return; }

	float damage = damageCauserWeapon->CalculateBaseDamage((debate.startLoc - actualHitLoc).Size());
	damage = requestResult.bHeadshot ? damage * damageCauserWeapon->GetHeadshotDamageMultiplier() : damage;


	//GEPR_TWO_VARIABLE("Damage: %.2f, Distance: %.2f", damage, (debate.startLoc - actualHitLoc).Size());

	UClass* damageType = requestResult.bHeadshot ? UHeadshotDamageType::StaticClass() : UDamageType::StaticClass();

	if (!ownerPlayer) ownerPlayer = Cast<APlayerCharacter>(GetOwner());
	if (ownerPlayer && !ownerPlayerController) ownerPlayerController = ownerPlayer->GetController<AMainPlayerController>();
	if (ownerPlayer && ownerPlayerController)
	{
		UGameplayStatics::ApplyDamage
		(debate.playerHit, damage, ownerPlayerController, damageCauserWeapon, damageType);
	}
}

void ULagCompensationComponent::ServerHitRequests_HitScan_Implementation
(const TArray<struct FServerSideRewindData>& rewindDatas, AHitScanWeapon* damageCauserWeapon)
{
	if (!damageCauserWeapon) { return; }
	if (rewindDatas.IsEmpty()) { return; }

	TMap<APlayerCharacter*, float> damagedPlayers{};

	for (auto& rewindData : rewindDatas)
	{
		if (!rewindData.playerHit) { continue; }

		FVector hitResultLoc{};
		FServerSideRewindResult requestResult = ServerSideRewind
		(rewindData.playerHit, rewindData.startLoc, rewindData.hitLoc, rewindData.timeOfHit, hitResultLoc);
		if (requestResult.bHitConfirmed)
		{
			//DRAW_LINE_COMP(ownerPlayer, rewindData.startLoc, hitResultLoc);

			//GEPR_ONE_VARIABLE("Server Side Rewind Distance: %.2f", (rewindData.startLoc - hitResultLoc).Size());
			float damage = damageCauserWeapon->CalculateBaseDamage((rewindData.startLoc - hitResultLoc).Size());
			damage = requestResult.bHeadshot ? damage * damageCauserWeapon->GetHeadshotDamageMultiplier() : damage;

			if (!damagedPlayers.Contains(rewindData.playerHit))
			{
				damagedPlayers.Emplace(rewindData.playerHit, damage);
			}
			else
			{
				damagedPlayers[rewindData.playerHit] += damage;
			}
		}
	}

	if (!ownerPlayer) ownerPlayer = Cast<APlayerCharacter>(GetOwner());
	if (ownerPlayer && !ownerPlayerController) ownerPlayerController = ownerPlayer->GetController<AMainPlayerController>();
	if (ownerPlayer && ownerPlayerController)
	{
		for (auto& damagedPlayerAndDamage : damagedPlayers)
		{
			UGameplayStatics::ApplyDamage
			(damagedPlayerAndDamage.Key, damagedPlayerAndDamage.Value,
				ownerPlayerController, damageCauserWeapon, UDamageType::StaticClass());
		}
	}
}





void ULagCompensationComponent::ServerHitRequest_Projectile_Implementation
(const FServerSideRewindData& projectileData,
	AProjectileWeapon* damageCauserWeapon)
{
	if (!damageCauserWeapon) { return; }

	FVector actualHitLoc{};
	FServerSideRewindResult requestResult = ServerSideRewind
	(projectileData.playerHit, projectileData.startLoc,
		projectileData.hitLoc, projectileData.timeOfHit, actualHitLoc);
	if (!requestResult.bHitConfirmed) { return; }

	float damage = requestResult.bHeadshot ?
		damageCauserWeapon->GetHeadshotDamage() : damageCauserWeapon->GetBaseDamage();

	UClass* damageType = requestResult.bHeadshot ? UHeadshotDamageType::StaticClass() : UDamageType::StaticClass();

	if (!ownerPlayer) ownerPlayer = Cast<APlayerCharacter>(GetOwner());
	if(ownerPlayer && !ownerPlayerController) ownerPlayerController = ownerPlayer->GetController<AMainPlayerController>();
	if (ownerPlayer && ownerPlayerController)
	{
		UGameplayStatics::ApplyDamage
		(projectileData.playerHit, damage, ownerPlayerController, damageCauserWeapon, damageType);
	}
}



FServerSideRewindResult ULagCompensationComponent::ServerSideRewind
(APlayerCharacter* playerHit,
	const FVector_NetQuantize& startLoc, const FVector_NetQuantize10& hitLoc,
	const float& timeOfHit, FVector& successfulHitLoc, bool bManualReset)
{
	FFramePackage selectedFrameToCheck = GetFrameToCheck(playerHit, timeOfHit);
	if (selectedFrameToCheck.time <= 0.f) { GEPRS_R("Server Rewind Hit: Time expired"); return FServerSideRewindResult{}; }

	
	FServerSideRewindResult rewindRes = ConfirmHit
	(selectedFrameToCheck, playerHit, startLoc, hitLoc, successfulHitLoc, bManualReset);

	if (rewindRes.bHitConfirmed)
	{
		//GEPRS_G("Server concluded it was a valid hit in the past");
		//DRAW_FRAME_PACKAGE_G(selectedFrameToCheck, ownerPlayer)
	}
	else
	{
		//GEPRS_R("Server concluded it was NOT a valid hit in the past");
		//DRAW_FRAME_PACKAGE_R(selectedFrameToCheck, ownerPlayer)
	}

	return rewindRes;
}



FServerSideRewindResult ULagCompensationComponent::ConfirmHit
(const FFramePackage& frameToCheck, APlayerCharacter* playerHit,
	const FVector_NetQuantize& startLoc, const FVector_NetQuantize10& hitLoc, 
	FVector& successfulHitLoc, bool bManualReset)
{
	if (!playerHit) { GEPRS_R("Server Rewind Hit: PlayerHit is null"); return FServerSideRewindResult{}; }
	FFramePackage currentFrameToMoveBackToAfterConfirmHit{};

	if(!bManualReset)
	CacheBoxPositions(playerHit, currentFrameToMoveBackToAfterConfirmHit);





	// Move the poxes to where the boxes were at the time of the hit
	MoveBoxes(playerHit, frameToCheck);

	//DRAW_FRAME_PACKAGE_COMP(frameToCheck, FColor::Black, ownerPlayer);
	//DRAW_FRAME_PACKAGE_G(currentFrameToMoveBackToAfterConfirmHit, ownerPlayer);

	const TMap<FName, UBoxComponent*>& boxes  = playerHit->Get_SSRBoxes();
	if (boxes.IsEmpty()) { return FServerSideRewindResult{}; }

	for (auto& boxPair : boxes)
	{
		if (boxPair.Value != nullptr)
		{
			boxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			boxPair.Value->SetCollisionResponseToChannel(ECollisionChannel_SSR, ECR_Block);
		}
	}

	const FVector traceEnd = startLoc + ((hitLoc - startLoc) * 1.25f);


//	DRAW_LINE_COMP(ownerPlayer, startLoc, hitLoc);

	FServerSideRewindResult checkResult{};
	FHitResult hitRes{};
	if(!world) world = GetWorld();
	
	if (world)
	{
		if (world->LineTraceSingleByChannel(hitRes, startLoc, traceEnd, ECollisionChannel_SSR))
		{
			//DRAW_POINT(hitRes.ImpactPoint);
			//DRAW_POINT_O(traceEnd);

			//DRAW_LINE_COMP_G(ownerPlayer, startLoc, traceEnd);

			if (hitRes.GetComponent())
			{

				//GEPR_ONE_VARIABLE("Component Hit: %s", *hitRes.GetComponent()->GetName());

				for (auto& boxPair : boxes)
				{
					if (hitRes.GetComponent() == boxPair.Value)
					{
						checkResult.bHitConfirmed = true;
						if (boxPair.Key == HEAD_BONE_AND_SSRB)
						{
							checkResult.bHeadshot = true;
						}

						if(!bManualReset)
						ResetBoxes(playerHit, currentFrameToMoveBackToAfterConfirmHit);

						//DRAW_POINT_O(hitRes.ImpactPoint);

						successfulHitLoc = hitRes.ImpactPoint;
						//DRAW_LINE_COMP_BLUE(ownerPlayer, startLoc, successfulHitLoc);
						return checkResult;
					}
				}
			}
		}
		else
		{
			//DRAW_LINE_COMP_BLACK(ownerPlayer, startLoc, traceEnd);
			//DRAW_POINT(startLoc);
			//DRAW_POINT_O(hitLoc);
		}
	}

	ResetBoxes(playerHit, currentFrameToMoveBackToAfterConfirmHit);
	return checkResult;
}




void ULagCompensationComponent::CacheBoxPositions
(APlayerCharacter* playerHit, FFramePackage& outFramePackage)
{
	if (!playerHit) { return; }
	if (!GetWorld()) { return; }

	if (!outFramePackage.hitBoxInfo.IsEmpty())
	{
		outFramePackage.hitBoxInfo.Empty();
	}

	// probably won't be used
	outFramePackage.time = GetWorld()->GetTimeSeconds();

	const TMap<FName, UBoxComponent*>& boxes = playerHit->Get_SSRBoxes();
	for (auto& boxPair : boxes)
	{
		if (boxPair.Value)
		{
			FRewindBoxInformation boxInfo{};
			boxInfo.boxExtent = boxPair.Value->GetScaledBoxExtent();
			boxInfo.location = boxPair.Value->GetComponentLocation();
			boxInfo.rotation = boxPair.Value->GetComponentRotation();
			outFramePackage.hitBoxInfo.Add(boxPair.Key, boxInfo);
		}
	}
}

void ULagCompensationComponent::MoveBoxes(APlayerCharacter* playerHit, const FFramePackage& framePackage)
{
	SetBoxesToFramePackage(playerHit, framePackage);
}

void ULagCompensationComponent::ResetBoxes(APlayerCharacter* playerHit, const FFramePackage& framePackage)
{
	SetBoxesToFramePackage(playerHit, framePackage, true);
}

void ULagCompensationComponent::SetBoxesToFramePackage
(APlayerCharacter* playerHit, const FFramePackage& framePackage, bool bReset)
{
	if (!playerHit) { return; }


	const TMap<FName, UBoxComponent*>& boxes = playerHit->Get_SSRBoxes();
	for (auto& boxPair : boxes)
	{
		if (boxPair.Value)
		{
			boxPair.Value->SetWorldLocation(framePackage.hitBoxInfo[boxPair.Key].location);
			boxPair.Value->SetWorldRotation(framePackage.hitBoxInfo[boxPair.Key].rotation);
			boxPair.Value->SetBoxExtent(framePackage.hitBoxInfo[boxPair.Key].boxExtent);

			if (bReset)
			{
				boxPair.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				boxPair.Value->SetCollisionResponseToChannel(ECollisionChannel_SSR, ECR_Ignore);
			}
		}
	}
}





void ULagCompensationComponent::DeleteExpiredFrames()
{
	float frameHistoryLength = GetLocationFrameHistoryLength();
	while (frameHistoryLength > frameHistoryMaxStoredLength)
	{
		frameLocationHistory.RemoveNode(frameLocationHistory.GetTail());
		frameHistoryLength = GetLocationFrameHistoryLength();
	}

	float accHistoryLength = GetAccuracyFrameHistoryLength();
	while (accHistoryLength > frameHistoryMaxStoredLength)
	{
		frameAccuracyHistory.RemoveNode(frameAccuracyHistory.GetTail());
		accHistoryLength = GetAccuracyFrameHistoryLength();
	}
}

void ULagCompensationComponent::SaveThisFrame()
{
	FFramePackage framePackage{};
	SaveFramePackage(framePackage);
	frameLocationHistory.AddHead(framePackage);

	FFrameAccuracy frameAccuracy{};
	SaveFrameAccuracy(frameAccuracy);
	frameAccuracyHistory.AddHead(frameAccuracy);
}



void ULagCompensationComponent::SaveFramePackage(FFramePackage& package)
{
	if(!world){ world = GetWorld(); }
	if (!world) { return; }

	if (!ownerPlayer) ownerPlayer = Cast<APlayerCharacter>(GetOwner());
	if (ownerPlayer)
	{
		package.time = world->GetTimeSeconds();


		const TMap<FName, UBoxComponent*>& playerSSRBoxes = ownerPlayer->Get_SSRBoxes();
		for (auto& rewindBoxPair : playerSSRBoxes)
		{
			if (rewindBoxPair.Value)
			{
				FRewindBoxInformation boxInfo{};
				boxInfo.location = rewindBoxPair.Value->GetComponentLocation();
				boxInfo.rotation = rewindBoxPair.Value->GetComponentRotation();
				boxInfo.boxExtent = rewindBoxPair.Value->GetScaledBoxExtent();

				package.hitBoxInfo.Emplace(rewindBoxPair.Key, boxInfo);
			}
		}
	}
}

void ULagCompensationComponent::SaveFrameAccuracy(FFrameAccuracy& accPackage)
{
	if (!world) { world = GetWorld(); }
	if (!world) { return; }

	if (ownerCombatComponent)
	{
		accPackage.time = world->GetTimeSeconds();
		accPackage.accuracy = ownerCombatComponent->GetCalculateInaccuracy();
	}
}




FFramePackage ULagCompensationComponent::GetFrameToCheck(const APlayerCharacter* playerHit, const float& timeOfHit)
{
	if (!playerHit) { return FFramePackage{}; }
	if (!playerHit->GetLagCompensationComponent()) { return FFramePackage{}; }
	if (playerHit->GetLagCompensationComponent()->IsThisTimeExpired(timeOfHit))
	{
		GEPRS_R("Time of hit expired");
		return FFramePackage{};
	}

	TDoubleLinkedList<FFramePackage>& playerHitFrameHistory =
		playerHit->GetLagCompensationComponent()->GetFrameHistory();

	const float oldestHistoryTime{ playerHitFrameHistory.GetTail()->GetValue().time };
	const float newestHistoryTime{ playerHitFrameHistory.GetHead()->GetValue().time };

	bool bShouldInterpolate{ true };

	FFramePackage selectedFrameToCheck{};

	// very unlikely
	if (newestHistoryTime <= timeOfHit)
	{
		selectedFrameToCheck = playerHitFrameHistory.GetHead()->GetValue();
		bShouldInterpolate = false;
	}

	// very unlikely
	if (oldestHistoryTime == timeOfHit)
	{
		selectedFrameToCheck = playerHitFrameHistory.GetTail()->GetValue();
		bShouldInterpolate = false;
	}



	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* older = playerHitFrameHistory.GetHead();
	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* younger = older;
	if (!older || !younger) { return FFramePackage{}; }


	while (older->GetValue().time > timeOfHit)
	{
		if (!older->GetNextNode()) { break; }
		older = older->GetNextNode();
		if (older->GetValue().time > timeOfHit)
		{
			younger = older;
		}
	}

	// very unlikely
	if (older->GetValue().time == timeOfHit)
	{
		selectedFrameToCheck = older->GetValue();
		bShouldInterpolate = false;
	}

	// interpolate between older and younger
	if (bShouldInterpolate)
	{
		selectedFrameToCheck = InterpolateFrames(older->GetValue(), younger->GetValue(), timeOfHit);
	}

	return selectedFrameToCheck;
}

FFrameAccuracy ULagCompensationComponent::GetFrameToCheck_Accuracy(const float& timeOfHit)
{
	if (!frameAccuracyHistory.GetTail()) { return FFrameAccuracy{ 0.f, 99.f }; }
	if (!frameAccuracyHistory.GetHead()) { return FFrameAccuracy{ 0.f, 99.f }; }
	if (IsThisTimeExpired(timeOfHit)) { GEPRS_R("Time of hit expired (Accuracy)"); return FFrameAccuracy{0.f, 99.f}; }

	const float oldestHistoryTime{ frameAccuracyHistory.GetTail()->GetValue().time };
	const float newestHistoryTime{ frameAccuracyHistory.GetHead()->GetValue().time };
	
	bool bShouldInterpolate{ true };

	FFrameAccuracy selectedFrameToCheck{};

	// very unlikely
	if (newestHistoryTime <= timeOfHit)
	{
		selectedFrameToCheck = frameAccuracyHistory.GetHead()->GetValue();
		bShouldInterpolate = false;
	}

	// very unlikely
	if (oldestHistoryTime == timeOfHit)
	{
		selectedFrameToCheck = frameAccuracyHistory.GetTail()->GetValue();
		bShouldInterpolate = false;
	}



	TDoubleLinkedList<FFrameAccuracy>::TDoubleLinkedListNode* older = frameAccuracyHistory.GetHead();
	TDoubleLinkedList<FFrameAccuracy>::TDoubleLinkedListNode* younger = older;

	while (older->GetValue().time > timeOfHit)
	{
		if (!older->GetNextNode()) { break; }
		older = older->GetNextNode();
		if (older->GetValue().time > timeOfHit)
		{
			younger = older;
		}
	}


	if (older->GetValue().time == timeOfHit)
	{
		selectedFrameToCheck = older->GetValue();
		bShouldInterpolate = false;
	}

	// interpolate between older and younger
	if (bShouldInterpolate)
	{
		selectedFrameToCheck = InterpolateFrames(older->GetValue(), younger->GetValue(), timeOfHit);
	}

//	GEPR_ONE_VARIABLE("Accuracy in the past: %f", selectedFrameToCheck.accuracy)
	return selectedFrameToCheck;
}


#define IGNORE_DELTA_TIME 1.f
FFramePackage ULagCompensationComponent::InterpolateFrames
(const FFramePackage& older, const FFramePackage& younger, const float& timeOfHit)
{
	const float timeDistance = younger.time - older.time;
	const float interpFraction = FMath::Clamp((timeOfHit - older.time) / timeDistance, 0.f, 1.f);


	FFramePackage interpolatedFramePackage{};
	interpolatedFramePackage.time = timeOfHit;

	for (auto& youngerPair : older.hitBoxInfo)
	{
		const FName& boneName = youngerPair.Key;

		const FRewindBoxInformation& youngerBox = younger.hitBoxInfo[boneName];
		const FRewindBoxInformation& olderBox = older.hitBoxInfo[boneName];

		FRewindBoxInformation interpolatedRewindBox{};

		interpolatedRewindBox.location = FMath::VInterpTo
		(olderBox.location, youngerBox.location, IGNORE_DELTA_TIME, interpFraction);

		interpolatedRewindBox.rotation = FMath::RInterpTo
		(olderBox.rotation, youngerBox.rotation, IGNORE_DELTA_TIME, interpFraction);

		interpolatedRewindBox.boxExtent = youngerBox.boxExtent;

		interpolatedFramePackage.hitBoxInfo.Add(boneName, interpolatedRewindBox);
	}

	return interpolatedFramePackage;
}

FFrameAccuracy ULagCompensationComponent::InterpolateFrames
(const FFrameAccuracy& older, const FFrameAccuracy& younger, const float& timeOfHit)
{
	const float timeDistance = younger.time - older.time;
	const float interpFraction = FMath::Clamp((timeOfHit - older.time) / timeDistance, 0.f, 1.f);

	FFrameAccuracy interpolatedFrame{};
	interpolatedFrame.time = timeOfHit;

	interpolatedFrame.accuracy = FMath::FInterpTo(older.accuracy, younger.accuracy, IGNORE_DELTA_TIME, interpFraction);

	return interpolatedFrame;
}




float ULagCompensationComponent::GetLocationFrameHistoryLength()
{
	if (frameLocationHistory.GetHead() && frameLocationHistory.GetTail())
	{
		return frameLocationHistory.GetHead()->GetValue().time -
			frameLocationHistory.GetTail()->GetValue().time;
	}

	return 0.0f;
}

float ULagCompensationComponent::GetAccuracyFrameHistoryLength()
{
	if (frameAccuracyHistory.GetHead() && frameAccuracyHistory.GetTail())
	{
		return frameAccuracyHistory.GetHead()->GetValue().time -
			frameAccuracyHistory.GetTail()->GetValue().time;
	}

	return 0.0f;
}

bool ULagCompensationComponent::IsThisTimeExpired(const float& time)
{
	if (frameLocationHistory.Num() <= 1)
	{
		return true;
	}

	if(frameLocationHistory.GetTail() && frameLocationHistory.GetHead())
	return time < frameLocationHistory.GetTail()->GetValue().time;

	return true;
}















// IMPLEMENTATION!!
//void ULagCompensationComponent::ServerHitRequest_HitScan_Implementation
//(APlayerCharacter* playerHit,
//	const FVector_NetQuantize& startLoc, const FVector_NetQuantize& hitLoc,
//	const float& timeOfHit, const float& singleTripTime,
//	AHitScanWeapon* damageCauserWeapon)
//{
//	if (!damageCauserWeapon) { GEPRS_R("Hit Scan Weapon Null");  return; }
//
//
//	DRAW_POINT(startLoc);
//	DRAW_POINT(hitLoc);
//
//	DRAW_FRAME_PACKAGE(GetFrameToCheck(playerHit, timeOfHit - singleTripTime), FColor::White);
//	DRAW_FRAME_PACKAGE(GetFrameToCheck(playerHit, timeOfHit), FColor::Magenta);
//	DRAW_FRAME_PACKAGE(GetFrameToCheck(playerHit, timeOfHit + singleTripTime), FColor::Black);
//	DRAW_FRAME_PACKAGE(GetFrameToCheck(playerHit, timeOfHit + (singleTripTime*2)), FColor::Purple);
//	FFramePackage currentPos{};
//	CacheBoxPositions(playerHit, currentPos);
//	DRAW_FRAME_PACKAGE(currentPos, FColor::Green);
//
//
//	FServerSideRewindResult requestResult = ServerSideRewind(playerHit, startLoc, hitLoc, timeOfHit + singleTripTime);
//	if (!requestResult.bHitConfirmed) {  return; }
//
//	float damage = damageCauserWeapon->CalculateDamage((hitLoc - startLoc).Size());
//	damage = requestResult.bHeadshot ? damage * damageCauserWeapon->GetHeadshotDamageMultiplier() : damage;
//
//
//	if (!ownerPlayer) ownerPlayer = Cast<APlayerCharacter>(GetOwner());
//	if (ownerPlayer && !ownerPlayerController) ownerPlayerController = ownerPlayer->GetController<AMainPlayerController>();
//	if (ownerPlayer && ownerPlayerController)
//	{
//		UGameplayStatics::ApplyDamage
//		(playerHit, damage, ownerPlayerController, damageCauserWeapon, UDamageType::StaticClass());
//	}
//}
