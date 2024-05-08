// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HelperTypes/HitDirection.h"
#include "CharacterOverlayWidget.generated.h"

#define HIDE_AMMO_AMOUNT -99

/**
 * 
 */
UCLASS()
class MUTLIPLAYERSHOOTER_API UCharacterOverlayWidget : public UUserWidget
{
	GENERATED_BODY()


public:
	void SetHealth(float health, float maxHealth);
	void SetScore(float score);
	void SetDeaths(const int32& deaths);
	void SetGrenades(const int32& grenades);
	void SetAmmo(const int32& ammo);
	void SetCarriedAmmo(const int32& carriedAmmo);
	void SetShield(const float& shield, const float& maxShield);
	void SetTimer(const float& countdownTime);

	void SetPlayerInCrosshairName(const FString& playerName, const FColor& displayColor);

	void HideTeamScore();
	void InitializeTeamScores();
	void SetBlueTeamScore(const float& score);
	void SetRedTeamScore(const float& score);



	void PlayHitEffect(bool bHitShield, bool bWillKill, const float& actualDamageDone);
	FLinearColor CreateHitEffectColor(bool bHitShield, bool bWillKill);

	void EnableHighPingWarning();
	void StopHighPingWarning();
	bool IsHighPingAnimationPlaying();

	void SetPingText(const int32& ping);


	void PlayVictimHitEffect(const EHitDirection& hitDir);

	virtual void NativeTick(const FGeometry& MyGeometry, float DeltaTime) override;

private:
	UPROPERTY(meta = (BindWidget))
	class UProgressBar* HealthBar;
	UPROPERTY(meta = (BindWidget))
	class UProgressBar* ShieldBar;
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* HealthText;
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* ScoreText;
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* DeathsText;
	UPROPERTY(meta = (BindWidget))
	class UImage* GrenadeImage;
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* GrenadeText;
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* AmmoText;
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* CarriedAmmoText;
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* TimerText;

	FString currentlyDisplayedName{};
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* PlayerInCrosshair{};



	UPROPERTY(meta = (BindWidget))
	class UImage* HighPingWarning;

	// Will only work if UPROPERTY Transient
	UPROPERTY(meta = (BindWidgetAnim), Transient)
	class UWidgetAnimation* HighPingAnimation;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* PingText;

	const FString ms{ TEXT(" ms") };


	UPROPERTY(EditAnywhere)
	FLinearColor normalHitColor{FColor::White};
	UPROPERTY(EditAnywhere)
	FLinearColor shieldHitColor{ FColor::Blue };
	UPROPERTY(EditAnywhere)
	FLinearColor killHitColor{ FColor::Red };


	UPROPERTY(meta = (BindWidget))
	class UImage* HitEffect;
	UPROPERTY(meta = (BindWidgetAnim), Transient)
	class UWidgetAnimation* HitEffectAnimation;



	UPROPERTY(EditAnywhere)
	float timeTohideRecentDamage{ 2.f };

	float recentDamageRunningTime{};
	float recentDamage{};
	bool bRecentDamageShown{};

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* RecentDamage;
	UPROPERTY(meta = (BindWidgetAnim), Transient)
	class UWidgetAnimation* RecentDamageAddAnimation;
	UPROPERTY(meta = (BindWidgetAnim), Transient)
	class UWidgetAnimation* RecentDamageHideAnimation;




	UPROPERTY(meta = (BindWidget))
	class UImage* VictimHitEffectTop;
	UPROPERTY(meta = (BindWidgetAnim), Transient)
	class UWidgetAnimation* HitEffectVictimTopAnimation;



	UPROPERTY(meta = (BindWidget))
	class UImage* VictimHitEffectLeft;
	UPROPERTY(meta = (BindWidgetAnim), Transient)
	class UWidgetAnimation* HitEffectVictimLeftAnimation;

	

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* TeamScoreDivider;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* RedTeamScore;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* BlueTeamScore;


};
