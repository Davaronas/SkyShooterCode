// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/CharacterOverlayWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "DebugMacros.h"
#include "HelperTypes/HitDirection.h"


#include "HelperTypes/CombatState.h"



void UCharacterOverlayWidget::SetHealth(float health, float maxHealth)
{
	if (!HealthBar) { return; }
	HealthBar->SetPercent(health / maxHealth);
	if (!HealthText) { return; }
	HealthText->SetText(FText::AsNumber(FMath::CeilToInt(health)));
}

void UCharacterOverlayWidget::SetScore(float score)
{
	if (!ScoreText) { return; }
	ScoreText->SetText(FText::AsNumber(FMath::CeilToInt(score)));
}

void UCharacterOverlayWidget::SetDeaths(const int32& deaths)
{
	if (!DeathsText) { return; }
	DeathsText->SetText(FText::AsNumber(deaths));
}

void UCharacterOverlayWidget::SetGrenades(const int32& grenades)
{
	if (!GrenadeText) { return; }
	if (!GrenadeImage) { return; }

	if (grenades <= 0)
	{
		GrenadeImage->SetVisibility(ESlateVisibility::Hidden);
		GrenadeText->SetText(FText());
	}
	else
	{
		GrenadeImage->SetVisibility(ESlateVisibility::Visible);
		GrenadeText->SetText(FText::AsNumber(grenades));
	}

}

void UCharacterOverlayWidget::SetAmmo(const int32& ammo)
{
	if (!AmmoText) { return; }

	if (ammo == HIDE_AMMO_AMOUNT)
	{
		AmmoText->SetText(FText::FromString(""));
		if(CarriedAmmoText) CarriedAmmoText->SetText(FText::FromString(""));
		return;
	}

	AmmoText->SetText(FText::AsNumber(ammo));
}

void UCharacterOverlayWidget::SetCarriedAmmo(const int32& carriedAmmo)
{
	if (!CarriedAmmoText) { return; }
	FString carriedAmmoStr{ FString::Printf(TEXT("/ %d"), carriedAmmo) };
	CarriedAmmoText->SetText(FText::FromString(carriedAmmoStr));
}

void UCharacterOverlayWidget::SetShield(const float& shield, const float& maxShield)
{
	if (!ShieldBar) { return; }
	ShieldBar->SetPercent(shield / maxShield);
}

void UCharacterOverlayWidget::SetTimer(const float& countdownTime)
{
	if (!TimerText) { return; }

	if (countdownTime < 0.f)
	{
		TimerText->SetText(FText());
		return;
	}

	const int32 minutes = FMath::FloorToInt(countdownTime / 60.f);
	const int32 seconds = FMath::RoundToInt32(countdownTime - (minutes * 60.f));
	const FString formattedTime = FString::Printf(TEXT("%02d:%02d"), minutes, seconds);

	TimerText->SetText(FText::FromString(formattedTime));
}

void UCharacterOverlayWidget::SetPlayerInCrosshairName(const FString& playerName, const FColor& displayColor)
{
	if (!PlayerInCrosshair) { return; }
	if (currentlyDisplayedName == playerName) { return; }

	PlayerInCrosshair->SetText(FText::FromString(playerName));
	PlayerInCrosshair->SetColorAndOpacity(displayColor);
	currentlyDisplayedName = playerName;
}




void UCharacterOverlayWidget::HideTeamScore()
{
	if (!TeamScoreDivider || !BlueTeamScore || !RedTeamScore) { return; }

	TeamScoreDivider->SetText(FText());
	BlueTeamScore->SetText(FText());
	RedTeamScore->SetText(FText());
}

void UCharacterOverlayWidget::InitializeTeamScores()
{
	if (!TeamScoreDivider || !BlueTeamScore || !RedTeamScore) { return; }

	FString zero_ = FString{ TEXT("0") };
	TeamScoreDivider->SetText(FText::FromString(TEXT("|")));
	BlueTeamScore->SetText(FText::FromString(zero_));
	RedTeamScore->SetText(FText::FromString(zero_));
}

void UCharacterOverlayWidget::SetBlueTeamScore(const float& score)
{
	if (!BlueTeamScore) { return; }

	BlueTeamScore->SetText(FText::AsNumber(FMath::CeilToInt(score)));
}

void UCharacterOverlayWidget::SetRedTeamScore(const float& score)
{
	if (!RedTeamScore) { return; }

	RedTeamScore->SetText(FText::AsNumber(FMath::CeilToInt(score)));
}



void UCharacterOverlayWidget::NativeTick(const FGeometry& MyGeometry, float DeltaTime)
{
	Super::NativeTick(MyGeometry, DeltaTime);

	if (bRecentDamageShown)
	{
		recentDamageRunningTime -= DeltaTime;
		if (recentDamageRunningTime <= 0.f)
		{
			recentDamageRunningTime = 0.f;

			if (RecentDamageHideAnimation)
			{
				PlayAnimation(RecentDamageHideAnimation);
				bRecentDamageShown = false;
				recentDamage = 0;
			}
		}
	}
	
}


void UCharacterOverlayWidget::PlayHitEffect(bool bHitShield, bool bWillKill, const float& actualDamageDone)
{
	if (!HitEffect) { return; }
	if (!HitEffectAnimation) { return; }
	
	
	HitEffect->SetRenderOpacity(1.f);
	HitEffect->SetColorAndOpacity(CreateHitEffectColor(bHitShield, bWillKill));
	PlayAnimation(HitEffectAnimation);

	if (!RecentDamage) { return; }
	if (!RecentDamageAddAnimation) { return; }
	if (!RecentDamageHideAnimation) { return; }

	recentDamage += actualDamageDone;
	RecentDamage->SetText(FText::AsNumber(FMath::RoundToInt32(recentDamage)));
	bRecentDamageShown = true;
	recentDamageRunningTime = timeTohideRecentDamage;
	if (IsAnimationPlaying(RecentDamageHideAnimation))
	{
		StopAnimation(RecentDamageHideAnimation);
	}
	PlayAnimation(RecentDamageAddAnimation);

}

FLinearColor UCharacterOverlayWidget::CreateHitEffectColor(bool bHitShield, bool bWillKill)
{
	if (bWillKill)
	{
		return killHitColor;
	}
	else if (bHitShield)
	{
		return shieldHitColor;
	}
	else
	{
		return normalHitColor;
	}
}

void UCharacterOverlayWidget::EnableHighPingWarning()
{
	if (HighPingWarning && HighPingAnimation)
	{
		HighPingWarning->SetRenderOpacity(1.f);
		PlayAnimation(HighPingAnimation, 0.f, 5);
	}
}

void UCharacterOverlayWidget::StopHighPingWarning()
{
	if (HighPingWarning && HighPingAnimation)
	{
		StopAnimation(HighPingAnimation);
		HighPingWarning->SetRenderOpacity(0.f);
	}
}

bool UCharacterOverlayWidget::IsHighPingAnimationPlaying()
{
	return HighPingWarning && HighPingAnimation && IsAnimationPlaying(HighPingAnimation);
}

void UCharacterOverlayWidget::SetPingText(const int32& ping)
{
	if (!PingText) { return; }

	if (ping <= 0)
	{
		PingText->SetText(FText());
	}
	else
	{
		PingText->SetText(FText::FromString(FString::Printf(TEXT("%d%s"), ping, *ms)));
	}
	
}

void UCharacterOverlayWidget::PlayVictimHitEffect(const EHitDirection& hitDir)
{
	if (!VictimHitEffectTop || !HitEffectVictimTopAnimation ||
		!VictimHitEffectLeft || !HitEffectVictimLeftAnimation) { return; }

	switch (hitDir)
	{
	case EHitDirection::EHD_Front:

		VictimHitEffectTop->SetRenderTransformAngle(0.f);
		PlayAnimation(HitEffectVictimTopAnimation);
		break;
	case EHitDirection::EHD_Back:
		VictimHitEffectTop->SetRenderTransformAngle(180.f);
		PlayAnimation(HitEffectVictimTopAnimation);
		break;
	case EHitDirection::EHD_Left:
		VictimHitEffectLeft->SetRenderTransformAngle(0.f);
		PlayAnimation(HitEffectVictimLeftAnimation);
		break;
	case EHitDirection::EHD_Right:
		VictimHitEffectLeft->SetRenderTransformAngle(180.f);
		PlayAnimation(HitEffectVictimLeftAnimation);
		break;

	default:
		break;
	}
}

