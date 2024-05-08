// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/AnnouncementWidget.h"
#include "Components/TextBlock.h"

void UAnnouncementWidget::SetTimer(const float& time)
{
	if (!TimerText) { return; }

	if (time < 0.f)
	{
		TimerText->SetText(FText::FromString(FString(TEXT("00:00"))));
		return;
	}

	const int32 minutes = FMath::RoundToInt32(time / 60.f);
	const int32 seconds = FMath::RoundToInt32(time - (minutes * 60.f));

	FString timeStr = FString::Printf(TEXT("%02d:%02d"), minutes, seconds);

	TimerText->SetText(FText::FromString(timeStr));
}

void UAnnouncementWidget::SetAnnouncementText(const FString& announcement)
{
	if (!AnnouncementText) { return; }

	AnnouncementText->SetText(FText::FromString(announcement));
}

void UAnnouncementWidget::SetInfoText(const FString& info)
{
	if (!InfoText) { return; }

	InfoText->SetText(FText::FromString(info));
}
