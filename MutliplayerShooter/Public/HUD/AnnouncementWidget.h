// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AnnouncementWidget.generated.h"

/**
 * 
 */
UCLASS()
class MUTLIPLAYERSHOOTER_API UAnnouncementWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetTimer(const float& time);
	void SetAnnouncementText(const FString& announcement);
	void SetInfoText(const FString& info);

private:

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* AnnouncementText;
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* TimerText;
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* InfoText;
	
};
