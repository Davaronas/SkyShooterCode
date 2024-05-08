// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReturnToMainMenuWidget.generated.h"

/**
 * 
 */
UCLASS()
class MUTLIPLAYERSHOOTER_API UReturnToMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void MenuSetup();
	void TryGetMsSubsystem();
	virtual bool Initialize() override;


	UFUNCTION(BlueprintCallable)
	void MenuTeardown();

	void PlayerGotPawn();

protected:

	UFUNCTION()
	void OnPlayerLeftGame();

private:

	UPROPERTY(meta = (BindWidget))
	class UButton* ReturnToMainMenuButton;

	UFUNCTION()
	void ReturnToMenuPressed();

	UPROPERTY()
	class UMultiplayerSessionsSubsystem* ms_subsystem{};


	UFUNCTION()
	void OnDestroyedSession(bool bWasSuccessful);





	UPROPERTY(meta = (BindWidget))
	class USlider* MouseSensitivitySlider;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* MouseSensitivityText;

	FVector2D MouseSensitivityText_StartPos{};
	FVector2D MouseSensitivitySlider_StartSize{};

	
	UFUNCTION()
	void MouseSensitivitySliderValueChanged(float newValue);

	

	UPROPERTY()
	class AMainPlayerController* playerController{};



};
	

