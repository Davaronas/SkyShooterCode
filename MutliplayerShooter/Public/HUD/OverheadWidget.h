// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OverheadWidget.generated.h"

/**
 * 
 */
UCLASS()
class MUTLIPLAYERSHOOTER_API UOverheadWidget : public UUserWidget
{
	GENERATED_BODY()



public:
	virtual void NativeDestruct() override;
	
	void SetDisplayText(FString text);

	UFUNCTION(BlueprintCallable)
	void ShowPlayerNetRole(class APawn* pawn);

private:
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* DisplayText;
};
