// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "PlayerPrefsSave.generated.h"


#define SAVE_GAME_SLOT FString{TEXT("playerPrefs")}

/**
 * 
 */
UCLASS()
class MUTLIPLAYERSHOOTER_API UPlayerPrefsSave : public USaveGame
{
	GENERATED_BODY()

public:

	static void Save(const float& newSensitivity);
	static UPlayerPrefsSave* Load();


protected:
	

private:

	UPROPERTY(VisibleAnywhere, SaveGame)
	float mouseSensitivity{ 1.f };

public:

	FORCEINLINE void SetMouseSensitivity(const float& newSensitivity);
	FORCEINLINE float GetMouseSensitivity() {return mouseSensitivity;}



	
};
