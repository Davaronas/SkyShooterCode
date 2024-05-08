// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerPrefsSave.h"
#include "Kismet/GameplayStatics.h"




void UPlayerPrefsSave::SetMouseSensitivity(const float& newSensitivity)
{
	mouseSensitivity = newSensitivity;
}

void UPlayerPrefsSave::Save(const float& newSensitivity)
{
	UPROPERTY()
	UPlayerPrefsSave* newSaveGame = Cast<UPlayerPrefsSave>
		(UGameplayStatics::CreateSaveGameObject(UPlayerPrefsSave::StaticClass()));

	if (!newSaveGame) { return; }

	newSaveGame->SetMouseSensitivity(newSensitivity);

	UGameplayStatics::SaveGameToSlot(newSaveGame, SAVE_GAME_SLOT, 0);
}

UPlayerPrefsSave* UPlayerPrefsSave::Load()
{
	if (UPROPERTY() UPlayerPrefsSave* loadedGame =
		Cast<UPlayerPrefsSave>(UGameplayStatics::LoadGameFromSlot(SAVE_GAME_SLOT, 0)))
	{
		return loadedGame;
	}

	return nullptr;
}
