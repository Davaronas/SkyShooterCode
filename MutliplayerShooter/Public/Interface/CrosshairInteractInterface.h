// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "HelperTypes/Teams.h"
#include "CrosshairInteractInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UCrosshairInteractInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class MUTLIPLAYERSHOOTER_API ICrosshairInteractInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	FORCEINLINE void SetDisplayName(FString display) { displayName = display; }
	FORCEINLINE FString GetDisplayName() { return displayName; }

	FORCEINLINE void SetTeamForCrosshair(ETeams newTeam) { belongingTeam = newTeam; }
	FORCEINLINE ETeams GetTeam() { return belongingTeam; }

private:
	FString displayName{};
	ETeams belongingTeam{};

};
