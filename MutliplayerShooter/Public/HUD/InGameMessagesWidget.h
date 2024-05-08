// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InGameMessagesWidget.generated.h"

/**
 * 
 */
UCLASS()
class MUTLIPLAYERSHOOTER_API UInGameMessagesWidget : public UUserWidget
{
	GENERATED_BODY()

public:


	void Announcement_PlayerEliminated
	(const FString& attackerName, const FString& victimName,
		const FString& tool, bool bHeadshot, const FColor textColor);
	void Announcement_PlayerJoined(const FString& joining);
	void Announcement_PlayerLeft(const FString& exitingName);
	void Announcement_ServerMessage(const FString& message, const FColor& color);
	void Announcement_PlayerGainedLead(const FString& gainedLeadPlayer, bool bIsSelf, bool bTied, const FColor& displayColor);
	void Announcement_TeamGainedLead(const FString& team, const FColor& displayColor);

	virtual void NativeTick(const FGeometry& MyGeometry, float DeltaTime) override;

	void DeleteExpiredMessages(float DeltaTime);

protected:


	void AddInGameMessage(const FString& message, FColor textColor, bool bImportant = false);

	void CreateMessage(class UTextBlock*& newMessage, const FColor& textColor, const FString& message, bool bImportant = false);


private:

	UPROPERTY(meta = (BindWidget))
	class UVerticalBox* AnnouncementVerticalBox;
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class UTextBlock> messageBaseBP{};
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class UTextBlock> messageBaseBP_Important{};

	UPROPERTY(EditDefaultsOnly)
	class USoundBase* importantMessageSound{};



	UPROPERTY(EditDefaultsOnly)
	float messageLifeSpan = 6.f;
	UPROPERTY(EditDefaultsOnly)
	float importantMessageLifeSpan = 10.f;

	UPROPERTY()
	TMap<UTextBlock*, float> messageTexts{};
	
};
