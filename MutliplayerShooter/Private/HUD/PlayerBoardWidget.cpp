// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/PlayerBoardWidget.h"
#include "Components/VerticalBox.h"
#include "HUD/PlayerBoardElementWidget.h"

void UPlayerBoardWidget::AddBoardElement(UPlayerBoardElementWidget* element)
{
	if (ElementHolder)
	{
		ElementHolder->AddChild(element);
	}
}
