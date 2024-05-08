// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerBoardElementWidget.generated.h"

/**
 * 
 */
UCLASS()
class MUTLIPLAYERSHOOTER_API UPlayerBoardElementWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void FillPlayerData(const FString& playerName,
		const float& score, const float& deaths, const float& ping, const FColor& displayColor);

protected:

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* PlayerName;
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* Score;
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* Deaths;
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* Ping;
};
