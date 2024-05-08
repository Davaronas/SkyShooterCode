// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "LobbyGameMode.generated.h"

/**
 * 
 */
UCLASS()
class MUTLIPLAYERSHOOTER_API ALobbyGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

protected:
	void BeginPlay();

	UPROPERTY(EditDefaultsOnly)
	float playersUpdateInterval{ 1.f };
	FTimerHandle playerUpdateTimerHandle{};

	void UpdatePlayers();

	UPROPERTY()
	TArray<class UWidget*> playerNameWidgets{};

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class UWidget> playerNameWidgetClass{};


	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class UUserWidget> lobbyWidgetClass{};
	UPROPERTY()
	class ULobbyWidget* lobbyWidget{};
	
};
