// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/MainHUD.h"
#include "GameFramework/PlayerController.h"
#include "HUD/CharacterOverlayWidget.h"
#include "HUD/AnnouncementWidget.h"
#include "HelperTypes/HitDirection.h"
#include "HUD/InGameMessagesWidget.h"
#include "Controller/MainPlayerController.h"
#include "HelperTypes/CombatState.h"
#include "HUD/PlayerBoardElementWidget.h"
#include "HUD/PlayerBoardWidget.h"
#include "HelperTypes/Teams.h"

#include "DebugMacros.h"

#define OVERLAY_WIDGET_CHECK \
if (!overlayWidget) AddOverlayWidget(); \
if (!overlayWidget) {GEPRS_R("Overlay Widget failed to be created"); return;}

#define OVERLAY_WIDGET_CHECK_RETURN(return_) \
if (!overlayWidget) AddOverlayWidget(); \
if (!overlayWidget) return return_;

#define ANNOUNCEMENT_WIDGET_CHECK \
if (!announcementWidget) AddAnnouncementWidget(); \
if (!announcementWidget) { return; }

#define INGAME_MESSAGES_CHECK \
if (!inGameMessagesWidget) AddInGameMessagesWidget(); \
if (!inGameMessagesWidget)  return;



void AMainHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!GEngine) { return; }
	if (!GEngine->GameViewport) { return; }

	FVector2D viewportSize{};
	GEngine->GameViewport->GetViewportSize(viewportSize);
	FVector2D drawLocation = FVector2D{ viewportSize.X / 2.f, viewportSize.Y / 2.f };

	if (bOverrideCenterCrosshairPosition)
	{
		if (crosshairDrawLocation != FVector2D::ZeroVector)
			drawLocation = crosshairDrawLocation;
	}

	const float currentCrosshairSpread = crosshairSpreadMax * crosshairPackage.spread;

	// All of these can be null
	//    FVector2D CrosshairSpread = FVector2D{ };
	FVector2D centerSpread = FVector2D::Zero(); 
	DrawCrosshair(crosshairPackage.crosshairCenter, drawLocation, centerSpread, crosshairPackage.crosshairColour);


	FVector2D leftCrosshairSpread = FVector2D{-currentCrosshairSpread, 0.f };
	DrawCrosshair(crosshairPackage.crosshairLeft, drawLocation, leftCrosshairSpread, crosshairPackage.crosshairColour);


	FVector2D rightCrosshairSpread = FVector2D{currentCrosshairSpread, 0.f };
	DrawCrosshair(crosshairPackage.crosshairRight, drawLocation, rightCrosshairSpread, crosshairPackage.crosshairColour);


	FVector2D topCrosshairSpread = FVector2D{0.f, -currentCrosshairSpread };
	DrawCrosshair(crosshairPackage.crosshairTop, drawLocation, topCrosshairSpread, crosshairPackage.crosshairColour);


	FVector2D bottomCrosshairSpread = FVector2D{0.f, currentCrosshairSpread };
	DrawCrosshair(crosshairPackage.crosshairBottom, drawLocation, bottomCrosshairSpread, crosshairPackage.crosshairColour);


}

void AMainHUD::DrawCrosshair(UTexture2D* texture, const FVector2D& location,
	const FVector2D& spread, const FLinearColor& colour)
{
	if (!texture) { return; }

	const float textureWidth = texture->GetSizeX();
	const float textureHeight = texture->GetSizeY();
	// Will be drawn at the middle of the passed in location
	const FVector2D drawLocation{
		location.X - (textureWidth / 2.f) + spread.X, 
		location.Y - (textureHeight / 2.f) + spread.Y };

	DrawTexture(texture, drawLocation.X, drawLocation.Y, textureWidth, textureHeight,
		0.f, 0.f, 1.f, 1.f, colour);
}

void AMainHUD::SetHealth(float health, float maxHealth)
{
	OVERLAY_WIDGET_CHECK

	overlayWidget->SetHealth(health, maxHealth);
}

void AMainHUD::SetScore(float score)
{
	OVERLAY_WIDGET_CHECK

	overlayWidget->SetScore(score);
}

void AMainHUD::SetDeaths(const int32& deaths)
{
	OVERLAY_WIDGET_CHECK

	overlayWidget->SetDeaths(deaths);
}

void AMainHUD::SetGrenades(const int32& grenades)
{
	OVERLAY_WIDGET_CHECK

	overlayWidget->SetGrenades(grenades);
}

void AMainHUD::SetAmmo(const int32& ammo)
{
	OVERLAY_WIDGET_CHECK

	overlayWidget->SetAmmo(ammo);
}

void AMainHUD::SetAmmo(const int32& ammo, const int32& carriedAmmo)
{
	SetAmmo(ammo);

	if(ammo != HIDE_AMMO_AMOUNT)
	SetCarriedAmmo(carriedAmmo);
}

void AMainHUD::SetCarriedAmmo(const int32& ammo)
{
	OVERLAY_WIDGET_CHECK

	overlayWidget->SetCarriedAmmo(ammo);
}

void AMainHUD::SetShield(const float& shield, const float& maxShield)
{
	OVERLAY_WIDGET_CHECK

	overlayWidget->SetShield(shield, maxShield);
}

void AMainHUD::SetTimer(const float& countdownTimer)
{
	OVERLAY_WIDGET_CHECK

	overlayWidget->SetTimer(countdownTimer);
}

void AMainHUD::SetPlayerInCrosshairName(const FString& player, const FColor& displayColor)
{
	OVERLAY_WIDGET_CHECK

	overlayWidget->SetPlayerInCrosshairName(player, displayColor);
}

void AMainHUD::SetAnnouncementTimer(const float& countdownTimer)
{
	ANNOUNCEMENT_WIDGET_CHECK


	announcementWidget->SetTimer(countdownTimer);
}

void AMainHUD::SetAnnouncementText(const FString& announcement)
{
	ANNOUNCEMENT_WIDGET_CHECK

	announcementWidget->SetAnnouncementText(announcement);
}

void AMainHUD::SetAnnouncementInfoText(const FString& info)
{
	ANNOUNCEMENT_WIDGET_CHECK

	announcementWidget->SetInfoText(info);
}





void AMainHUD::HideTeamScore()
{
	OVERLAY_WIDGET_CHECK;

	overlayWidget->HideTeamScore();
}

void AMainHUD::InitializeTeamScores()
{
	OVERLAY_WIDGET_CHECK;

	//GEPRS_G("InitializeTeamScores");
	overlayWidget->InitializeTeamScores();
}

void AMainHUD::SetBlueTeamScore(const float& score)
{
	OVERLAY_WIDGET_CHECK;

	overlayWidget->SetBlueTeamScore(score);
}

void AMainHUD::SetRedTeamScore(const float& score)
{
	OVERLAY_WIDGET_CHECK;

	overlayWidget->SetRedTeamScore(score);
}






void AMainHUD::InGameMessage_PlayerElimination(const FString& attackerName, const FString& victimName,
	const FString& tool, bool bHeadshot ,const FColor textColor)
{
	INGAME_MESSAGES_CHECK;

	inGameMessagesWidget->Announcement_PlayerEliminated(attackerName, victimName, tool, bHeadshot, textColor);
}

void AMainHUD::InGameMessage_PlayerJoinedGame(const FString& joiningName)
{
	INGAME_MESSAGES_CHECK;

	inGameMessagesWidget->Announcement_PlayerJoined(joiningName);

}

void AMainHUD::InGameMessage_PlayerLeftGame(const FString& exitingName)
{
	INGAME_MESSAGES_CHECK;

	inGameMessagesWidget->Announcement_PlayerLeft(exitingName);
}

void AMainHUD::InGameMessage_ServerMessage(const FString& message, const FColor& color)
{
	INGAME_MESSAGES_CHECK;

	inGameMessagesWidget->Announcement_ServerMessage(message, color);
}

void AMainHUD::InGameMessage_PlayerGainedLead(const FString& gainedLeadPlayer, bool bIsSelf, bool bTied, const FColor& displayColor)
{
	INGAME_MESSAGES_CHECK;

	inGameMessagesWidget->Announcement_PlayerGainedLead(gainedLeadPlayer, bIsSelf, bTied, displayColor);
}

void AMainHUD::InGameMessage_TeamGainedLead(const FString& team, const FColor& displayColor)
{
	INGAME_MESSAGES_CHECK;

	inGameMessagesWidget->Announcement_TeamGainedLead(team, displayColor);
}

void AMainHUD::PlayHitEffect(bool bHitShield, bool bWillKill, const float& actualDamageDone)
{
	OVERLAY_WIDGET_CHECK

	overlayWidget->PlayHitEffect(bHitShield, bWillKill, actualDamageDone);
}

void AMainHUD::PlayVictimHitEffect(const EHitDirection& hitDir)
{
	OVERLAY_WIDGET_CHECK

	overlayWidget->PlayVictimHitEffect(hitDir);
}

void AMainHUD::HighPingWarning()
{
	OVERLAY_WIDGET_CHECK

	overlayWidget->EnableHighPingWarning();
}

void AMainHUD::StopHighPingWarning()
{
	OVERLAY_WIDGET_CHECK

	overlayWidget->StopHighPingWarning();
}

bool AMainHUD::IsHighPingAnimationPlaying()
{
	OVERLAY_WIDGET_CHECK_RETURN(false)

	return overlayWidget->IsHighPingAnimationPlaying();
}

void AMainHUD::SetPingText(const int32& ping)
{
	OVERLAY_WIDGET_CHECK

	overlayWidget->SetPingText(ping);
}



void AMainHUD::DisplayPlayerBoard(const TArray<FPlayerBoardInfo>& playerInfos)
{

	if(!owningPlayer) owningPlayer = Cast<AMainPlayerController>(GetOwningPlayerController());

	if (!playerBoard && playerBoardClass)
	{
		playerBoard = CreateWidget<UPlayerBoardWidget>(owningPlayer, playerBoardClass);
		playerBoard->AddToViewport();
	}

	if (playerBoardElementClass && playerBoard)
	{
		for (auto& playerInfo : playerInfos)
		{
			UPlayerBoardElementWidget* newPlayerBoardElement =
				CreateWidget< UPlayerBoardElementWidget>(owningPlayer, playerBoardElementClass);
			if (!newPlayerBoardElement) { return; }

			FColor displayColor{};
			switch (playerInfo.team)
			{
			case ETeams::ET_RedTeam:
				displayColor = redTeamDisplayColor;
				break;
			case ETeams::ET_BlueTeam:
				displayColor = blueTeamDisplayColor;
				break;
			default:
				displayColor = normalDisplayColor;
				break;
			}

			newPlayerBoardElement->FillPlayerData(playerInfo.playerName,
				playerInfo.score, playerInfo.deaths, playerInfo.ping, displayColor);

			playerBoardElements.Add(newPlayerBoardElement);
			playerBoard->AddBoardElement(newPlayerBoardElement);
		}

		playerBoard->SetVisibility(ESlateVisibility::Visible);
	}
}

void AMainHUD::HidePlayerBoard()
{
	if (!playerBoard) { return; }

	for (auto& playerBoardElement : playerBoardElements)
	{
		playerBoardElement->RemoveFromParent();
	}

	playerBoardElements.Empty();
	playerBoard->SetVisibility(ESlateVisibility::Hidden);
}

void AMainHUD::BeginPlay()
{
	Super::BeginPlay();

	owningPlayer = Cast<AMainPlayerController>(GetOwningPlayerController());
	if (owningPlayer)
	{
		//GEPRS_R("MainHUD: PC->OnHudInit");
		owningPlayer->OnHudInitialized();

		if (fpsCounterWidgetType && !fpsCounterWidget)
		{
			fpsCounterWidget = CreateWidget<UUserWidget>(owningPlayer, fpsCounterWidgetType);
			fpsCounterWidget->AddToViewport();
		}
	}

	
}

void AMainHUD::AddOverlayWidget(bool bForceVisible)
{

	if (!overlayWidgetType) { return; }

	if(!owningPlayer)
	owningPlayer = Cast<AMainPlayerController>(GetOwningPlayerController());
	
	if (owningPlayer)
	{
		if (!owningPlayer->IsLocalController()) return;
		if (overlayWidget) 
		{
			if (overlayWidget->GetVisibility() != ESlateVisibility::Visible && bForceVisible)
			{
				overlayWidget->SetVisibility(ESlateVisibility::Visible);
			}
			return;
		}
		else
		{
			overlayWidget = CreateWidget<UCharacterOverlayWidget>(owningPlayer, overlayWidgetType);
			overlayWidget->AddToViewport();
		}
	}

	if (announcementWidget)
	{
		announcementWidget->SetVisibility(ESlateVisibility::Hidden);
	}
}

void AMainHUD::AddAnnouncementWidget()
{
	if (!announcementWidgetType) { return; }

	if (!owningPlayer)
		owningPlayer = Cast<AMainPlayerController>(GetOwningPlayerController());

	if (owningPlayer)
	{
		if (!owningPlayer->IsLocalController()) return;
		if (announcementWidget)
		{
			if (announcementWidget->GetVisibility() != ESlateVisibility::Visible)
			{
				announcementWidget->SetVisibility(ESlateVisibility::Visible);
			}
		}
		else
		{
			announcementWidget = CreateWidget<UAnnouncementWidget>(owningPlayer, announcementWidgetType);
			announcementWidget->AddToViewport();
		}

		if (overlayWidget)
		{
			overlayWidget->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void AMainHUD::AddInGameMessagesWidget()
{
	if (!inGameMessagesWidgetType) { return; }


	if (!owningPlayer)
		owningPlayer = Cast<AMainPlayerController>(GetOwningPlayerController());

	if (owningPlayer)
	{
		if (!owningPlayer->IsLocalController()) return;
		if (inGameMessagesWidget)
		{
			if (inGameMessagesWidget->GetVisibility() != ESlateVisibility::HitTestInvisible)
			{
				inGameMessagesWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
		}
		else
		{
			inGameMessagesWidget = CreateWidget<UInGameMessagesWidget>(owningPlayer, inGameMessagesWidgetType);
			inGameMessagesWidget->AddToViewport();
		}
	}
}

