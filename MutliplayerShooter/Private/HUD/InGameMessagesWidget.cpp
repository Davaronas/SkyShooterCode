// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/InGameMessagesWidget.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Kismet/GameplayStatics.h"

#include "DebugMacros.h"


void UInGameMessagesWidget::AddInGameMessage(const FString& message, FColor textColor, bool bImportant)
{
	if (AnnouncementVerticalBox)
	{
		UPROPERTY()
		UTextBlock* newMessage{};

		if (!bImportant)
		{
			if(messageBaseBP)
			newMessage = NewObject<UTextBlock>(this, messageBaseBP);
		}
		else
		{
			if (messageBaseBP_Important)
			{
				newMessage = NewObject<UTextBlock>(this, messageBaseBP_Important);
				if (importantMessageSound)
				{
					UGameplayStatics::PlaySound2D(this, importantMessageSound);
				}
			}
		}

		CreateMessage(newMessage, textColor, message, bImportant);
	}

}

void UInGameMessagesWidget::CreateMessage(UTextBlock*& newMessage,
	const FColor& textColor, const FString& message, bool bImportant)
{
	if (newMessage)
	{
		newMessage->SetColorAndOpacity(textColor);
		newMessage->SetText(FText::FromString(message));
		AnnouncementVerticalBox->AddChild(newMessage);

		messageTexts.Emplace(newMessage, bImportant ? importantMessageLifeSpan : messageLifeSpan);
	}
}


void UInGameMessagesWidget::Announcement_PlayerEliminated(const FString& attackerName, const FString& victimName,
	const FString& tool, bool bHeadshot, const FColor textColor)
{
	if (!GetWorld()) { return; }

	FString message = FString::Printf(TEXT("%s eliminated %s!"), *attackerName, *victimName);

	if(!tool.IsEmpty())
	message.Append(FString::Printf(TEXT("   [%s]"), *tool));


	if (bHeadshot) message.Append(TEXT("   [Headshot]"));

	AddInGameMessage(message, textColor);
}

void UInGameMessagesWidget::Announcement_PlayerJoined(const FString& joining)
{
	FString message = FString::Printf(TEXT("%s has joined the session"), *joining);
	AddInGameMessage(message, FColor::Silver);
}

void UInGameMessagesWidget::Announcement_PlayerLeft(const FString& exitingName)
{
	FString message = FString::Printf(TEXT("%s has left the session"), *exitingName);
	AddInGameMessage(message, FColor::Silver);
}

void UInGameMessagesWidget::Announcement_ServerMessage(const FString& message, const FColor& color)
{
	AddInGameMessage(message, color, true);
}

void UInGameMessagesWidget::Announcement_PlayerGainedLead
(const FString& gainedLeadPlayer, bool bIsSelf, bool bTied, const FColor& displayColor)
{
	FString message{};

	message = gainedLeadPlayer;
	

	if (bTied)
	{
		message.Append(bIsSelf ? TEXT(" Are Tied!") : TEXT(" Is Tied!"));
	}
	else
	{
		message.Append(bIsSelf ? TEXT(" Have Gained The Lead!") : TEXT(" Has Gained The Lead!"));
	}

	AddInGameMessage(message, displayColor, true);
}

void UInGameMessagesWidget::Announcement_TeamGainedLead(const FString& team, const FColor& displayColor)
{
	FString message = team;
	message.Append(TEXT(" Has Gained The Lead!"));

	AddInGameMessage(message, displayColor, true);
}

void UInGameMessagesWidget::NativeTick(const FGeometry& MyGeometry, float DeltaTime)
{
	Super::NativeTick(MyGeometry, DeltaTime);

	DeleteExpiredMessages(DeltaTime);
}

void UInGameMessagesWidget::DeleteExpiredMessages(float DeltaTime)
{
	UTextBlock* expiredMessage{ nullptr };
	for (auto& messageTextPair : messageTexts)
	{
		messageTextPair.Value -= DeltaTime;

		if (messageTextPair.Value <= 0.f)
		{
			expiredMessage = messageTextPair.Key;
			break;
		}
	}

	if (expiredMessage)
	{
		if (messageTexts.Contains(expiredMessage))
		{
			messageTexts.Remove(expiredMessage);
		}
		expiredMessage->RemoveFromParent();
	}
}


