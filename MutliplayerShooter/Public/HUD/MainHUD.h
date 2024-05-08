// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "HelperTypes/HitDirection.h"
#include "Math/Vector2D.h"
#include "HelperTypes/Teams.h"
#include "MainHUD.generated.h"

USTRUCT(BlueprintType)
struct FPlayerBoardInfo
{
	GENERATED_BODY()

public:
	FString playerName{};
	float score{};
	float deaths{};
	float ping{};
	ETeams team{};

};


USTRUCT(BlueprintType)
struct FCrosshairPackage
{
	GENERATED_BODY()

public:

	class UTexture2D* crosshairCenter;
	UTexture2D* crosshairLeft;
	UTexture2D* crosshairRight;
	UTexture2D* crosshairTop;
	UTexture2D* crosshairBottom;
	float spread;
	FLinearColor crosshairColour;


};

/**
 * 
 */
UCLASS()
class MUTLIPLAYERSHOOTER_API AMainHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;
//	virtual void BeginPlay() override;

	FORCEINLINE void SetCrosshairPackage(const FCrosshairPackage& crosshair) { crosshairPackage = crosshair; }

	UPROPERTY(EditDefaultsOnly, Category = "Overlay Widget")
	TSubclassOf<class UUserWidget> overlayWidgetType{};
	UPROPERTY(VisibleAnywhere, Category = "Overlay Widget")
	class UCharacterOverlayWidget* overlayWidget{};

	UPROPERTY(EditDefaultsOnly, Category = "Announcement Widget")
	TSubclassOf<class UUserWidget> announcementWidgetType{};
	UPROPERTY(VisibleAnywhere, Category = "Announcement Widget")
	class UAnnouncementWidget* announcementWidget{};

	UPROPERTY(EditDefaultsOnly, Category = "Announcement Widget")
	TSubclassOf<class UUserWidget> fpsCounterWidgetType{};
	UPROPERTY(VisibleAnywhere, Category = "Announcement Widget")
	class UUserWidget* fpsCounterWidget{};

	


	UPROPERTY(EditDefaultsOnly, Category = "Ingame Messages Widget")
	TSubclassOf<class UUserWidget> inGameMessagesWidgetType{};
	UPROPERTY(VisibleAnywhere, Category = "Ingame Messages Widget")
	class UInGameMessagesWidget* inGameMessagesWidget{};

	void SetHealth(float health, float maxHealth);
	void SetScore(float score);
	void SetDeaths(const int32& deaths);
	void SetGrenades(const int32& grenades);
	void SetAmmo(const int32& ammo);
	void SetAmmo(const int32& ammo, const int32& carriedAmmo);
	void SetCarriedAmmo(const int32& ammo);
	void SetShield(const float& shield, const float& maxShield);
	void SetTimer(const float& countdownTimer);
	void SetPlayerInCrosshairName(const FString& player, const FColor& displayColor);

	void SetAnnouncementTimer(const float& countdownTimer);
	void SetAnnouncementText(const FString& announcement);
	void SetAnnouncementInfoText(const FString& announcement);


	void HideTeamScore();
	void InitializeTeamScores();
	void SetBlueTeamScore(const float& score);
	void SetRedTeamScore(const float& score);


	void InGameMessage_PlayerElimination(const FString& attackerName, const FString& victimName,
		const FString& tool, bool bHeadshot, const FColor textColor);
	void InGameMessage_PlayerJoinedGame(const FString& joiningName);
	void InGameMessage_PlayerLeftGame(const FString& exitingName);
	void InGameMessage_ServerMessage(const FString& message, const FColor& color);
	void InGameMessage_PlayerGainedLead
	(const FString& gainedLeadPlayer, bool bIsSelf, bool bTied, const FColor& displayColor);
	void InGameMessage_TeamGainedLead(const FString& team, const FColor& displayColor);

	void PlayHitEffect(bool bHitShield, bool bWillKill, const float& actualDamageDone);

	void PlayVictimHitEffect(const EHitDirection& hitDir);

	void HighPingWarning();
	void StopHighPingWarning();

	bool IsHighPingAnimationPlaying();

	void SetPingText(const int32& ping);

	void AddOverlayWidget(bool bForceVisible = true);
	void AddAnnouncementWidget();

	FORCEINLINE bool IsOverlayWidgetCreated() { return overlayWidget != nullptr; }

	void AddInGameMessagesWidget();

	FORCEINLINE bool IsInGameMessagesWidgetCreated() { return inGameMessagesWidget != nullptr; }

	void SetCrosshairPosition(const FVector2D& crosshairPos) { crosshairDrawLocation = crosshairPos; }


	void DisplayPlayerBoard(const TArray<FPlayerBoardInfo>& playerInfos);
	void HidePlayerBoard();

protected:
	virtual void BeginPlay() override;


	UPROPERTY(EditDefaultsOnly, Category = "PlayerBoard")
	TSubclassOf<class UUserWidget> playerBoardClass{};
	
	UPROPERTY()
	class UPlayerBoardWidget* playerBoard{};

	UPROPERTY(EditDefaultsOnly, Category = "PlayerBoard")
	TSubclassOf<class UUserWidget> playerBoardElementClass{};

	UPROPERTY(EditDefaultsOnly, Category = "PlayerBoard")
	FColor normalDisplayColor{};
	UPROPERTY(EditDefaultsOnly, Category = "PlayerBoard")
	FColor redTeamDisplayColor{};
	UPROPERTY(EditDefaultsOnly, Category = "PlayerBoard")
	FColor blueTeamDisplayColor{};

	UPROPERTY()
	TArray<class UPlayerBoardElementWidget*> playerBoardElements{};

	FVector2D crosshairDrawLocation{};
	UPROPERTY(EditAnywhere)
	bool bOverrideCenterCrosshairPosition = true;

	UPROPERTY()
	class AMainPlayerController* owningPlayer{};

private:
	FCrosshairPackage crosshairPackage{};

	UPROPERTY(EditAnywhere)
	float crosshairSpreadMax = 16.f;

	void DrawCrosshair(class UTexture2D* texture, const FVector2D& location,
		const FVector2D& spread, const FLinearColor& colour);
};
