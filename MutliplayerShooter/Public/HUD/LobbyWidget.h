// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LobbyWidget.generated.h"

/**
 * 
 */
UCLASS()
class MUTLIPLAYERSHOOTER_API ULobbyWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void AddWidgetToVerticalBox(class UWidget* widget);

protected:
	UPROPERTY(meta = (BindWidget))
	class UVerticalBox* PlayerNameVerticalBox;
	
};
