// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/ReturnToMainMenuWidget.h"
#include "GameFramework/PlayerController.h"
#include "Components/Button.h"
#include "MultiplayerSessionsSubsystem.h"
#include "GameFramework/GameMode.h"
#include "Controller/MainPlayerController.h"
#include "PlayerCharacter.h"
#include "Components/TextBlock.h"
#include "Components/Slider.h"
#include "PlayerPrefsSave.h"
#include "Components/CanvasPanelSlot.h"
#include "Kismet/GameplayStatics.h"

#include "DebugMacros.h"


void UReturnToMainMenuWidget::MenuSetup()
{
	// should be over everything
	AddToViewport(2);
	SetVisibility(ESlateVisibility::Visible);

	if (!playerController) playerController = GetWorld()->GetFirstPlayerController<AMainPlayerController>();

	if (!GetWorld()) { return; }

	if(!playerController) playerController = Cast<AMainPlayerController>(GetWorld()->GetFirstPlayerController());
	if (!playerController) { return; }

	//FInputModeGameAndUI inputMode{};
	FInputModeUIOnly inputMode{};
	inputMode.SetWidgetToFocus(TakeWidget());
	playerController->SetInputMode(inputMode);
	playerController->SetShowMouseCursor(true);

	if (ReturnToMainMenuButton) //&& !ReturnToMainMenuButton->OnClicked.IsBound())
	{
		ReturnToMainMenuButton->OnClicked.AddDynamic(this, &ThisClass::ReturnToMenuPressed);

		APlayerCharacter* playerCharacter = playerController->GetPawn<APlayerCharacter>();
		if (!playerCharacter) { ReturnToMainMenuButton->SetIsEnabled(false); }
	}

	if (MouseSensitivityText && MouseSensitivitySlider)
	{
		MouseSensitivityText_StartPos = MouseSensitivityText->GetRenderTransform().Translation;

		

		UCanvasPanelSlot* CanvasSlot = Cast< UCanvasPanelSlot >(MouseSensitivitySlider->Slot);
		MouseSensitivitySlider_StartSize = CanvasSlot->GetSize();

	



		const float mouseSens = FMath::Clamp(playerController->
			GetMouseSensitivity(), 0.01f, MouseSensitivitySlider->GetMaxValue());

		const FString mouseSensStr = FString::Printf(TEXT("%.2f"), mouseSens);


		FVector2D newPos = MouseSensitivityText_StartPos + FVector2D{
		mouseSens *
		(MouseSensitivitySlider_StartSize.X / MouseSensitivitySlider->GetMaxValue()),
		0.f };
		MouseSensitivityText->SetRenderTranslation(newPos);


		MouseSensitivityText->SetText(FText::FromString(mouseSensStr));
		MouseSensitivitySlider->SetValue(mouseSens);

		MouseSensitivitySlider->OnValueChanged.AddDynamic(this, &ThisClass::MouseSensitivitySliderValueChanged);
	}


	TryGetMsSubsystem();
	if (ms_subsystem /*&&  !ms_subsystem->MultiplayerDestroySessionCompleteDelegate.IsBound()*/)
	{
		ms_subsystem->MultiplayerDestroySessionCompleteDelegate.AddDynamic(this, &ThisClass::OnDestroyedSession);
	}

	
}


void UReturnToMainMenuWidget::MouseSensitivitySliderValueChanged(float newValue)
{
	UPlayerPrefsSave::Save(newValue);

	if (playerController)
	{
		playerController->SetMouseSensitivity(newValue);
	}

	if (MouseSensitivityText && MouseSensitivitySlider)
	{

		FVector2D newPos = MouseSensitivityText_StartPos + FVector2D{
			newValue *
			(MouseSensitivitySlider_StartSize.X / MouseSensitivitySlider->GetMaxValue()),
			0.f };
		MouseSensitivityText->SetRenderTranslation(newPos);

		const FString mouseSensStr = FString::Printf(TEXT("%.2f"), newValue);
		MouseSensitivityText->SetText(FText::FromString(mouseSensStr));

		
	}
}



bool UReturnToMainMenuWidget::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}

	

	return true;
}

void UReturnToMainMenuWidget::MenuTeardown()
{
	RemoveFromParent();

	if (!GetWorld()) { return; }

	if (!playerController) playerController = Cast<AMainPlayerController>(GetWorld()->GetFirstPlayerController());
	if (!playerController) { return; }


	if (MouseSensitivityText)
	{
		MouseSensitivityText->SetRenderTranslation(MouseSensitivityText_StartPos);
	}

	FInputModeGameOnly inputMode{};
	playerController->SetInputMode(inputMode);
	playerController->SetShowMouseCursor(false);

	if (ReturnToMainMenuButton) //&& ReturnToMainMenuButton->OnClicked.IsBound())
	{
		ReturnToMainMenuButton->OnClicked.RemoveDynamic(this, &ThisClass::ReturnToMenuPressed);
	}


	
	//if (ms_subsystem /* && ms_subsystem->MultiplayerDestroySessionCompleteDelegate.IsBound()*/)
	//{
	//	ms_subsystem->MultiplayerDestroySessionCompleteDelegate.RemoveDynamic(this, &ThisClass::OnDestroyedSession);
	//}

}

void UReturnToMainMenuWidget::PlayerGotPawn()
{
	if (!ReturnToMainMenuButton) { return; }

	ReturnToMainMenuButton->SetIsEnabled(true);
}



void UReturnToMainMenuWidget::ReturnToMenuPressed()
{
	if (playerController && playerController->GetNetMode() == ENetMode::NM_Standalone)
	{
		UGameplayStatics::OpenLevel(this, FName{ TEXT("MainMenu") }, true);
	}

	if (!GetWorld()) { return; }
	if (!ReturnToMainMenuButton) { return; }

	ReturnToMainMenuButton->SetIsEnabled(false);

	if (!playerController) playerController = GetWorld()->GetFirstPlayerController<AMainPlayerController>();
	if (!playerController) { ReturnToMainMenuButton->SetIsEnabled(true); return; }

	APlayerCharacter* playerCharacter = playerController->GetPawn<APlayerCharacter>();
	if (!playerCharacter) { ReturnToMainMenuButton->SetIsEnabled(true); return; }

	//GEPRS_Y("RTM pressed");

	playerCharacter->OnPlayerLeftGameDelegate.AddDynamic(this, &ThisClass::OnPlayerLeftGame);
	playerCharacter->ServerLeaveGame();
}

void UReturnToMainMenuWidget::OnDestroyedSession(bool bWasSuccessful)
{
	if (ReturnToMainMenuButton && !bWasSuccessful)
	{
		ReturnToMainMenuButton->SetIsEnabled(true);
		return;
	}


	UWorld* world = GetWorld();
	if (!world) { return; }


	//Server
	AGameModeBase* gameMode = world->GetAuthGameMode();
	if (gameMode) 
	{
		gameMode->ReturnToMainMenuHost();
	}
	else // Client
	{
		if (!playerController) playerController = Cast<AMainPlayerController>(GetWorld()->GetFirstPlayerController());
		if (!playerController) { return; }

		playerController->ClientReturnToMainMenuWithTextReason(FText());
	}

}



void UReturnToMainMenuWidget::TryGetMsSubsystem()
{
	if (!ms_subsystem)
	{
		UGameInstance* gameInstance = GetGameInstance();
		if (!gameInstance) { return; }
		ms_subsystem = gameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
		if (!ms_subsystem) { return; }
	}
}


void UReturnToMainMenuWidget::OnPlayerLeftGame()
{
	TryGetMsSubsystem();

	if (ms_subsystem)
	{
		ms_subsystem->MultiplayerDestroySessionCompleteDelegate.AddDynamic(this, &ThisClass::OnDestroyedSession);
		ms_subsystem->DestroySession();
	}
}
