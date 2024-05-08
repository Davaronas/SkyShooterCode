// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OnlineSessionSettings.h"
#include "SessionResultWidget.generated.h"


// UNUSED CLASS

/**
 * 
 */
UCLASS()
class MUTLIPLAYERSHOOTER_API USessionResultWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeSessionResult(FOnlineSessionSearchResult& searchResult,
		class UMultiplayerSessionsMenu* menuPointer);

	UFUNCTION()
	void OnJoinClicked();

protected:

	bool bValid{ false };

	FOnlineSessionSearchResult storedSearchResult;
	UPROPERTY()
	class UMultiplayerSessionsMenu* storedMenuPointer{};

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* PlayerName;
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* GameMode;
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* PlayerCount;
	UPROPERTY(meta = (BindWidget))
	class UButton* JoinButton;
};
