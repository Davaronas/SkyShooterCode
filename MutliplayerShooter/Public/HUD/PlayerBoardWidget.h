// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerBoardWidget.generated.h"

/**
 * 
 */
UCLASS()
class MUTLIPLAYERSHOOTER_API UPlayerBoardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void AddBoardElement(class UPlayerBoardElementWidget* element);

protected:

	UPROPERTY(meta = (BindWidget))
	class UVerticalBox* ElementHolder;
	
};
