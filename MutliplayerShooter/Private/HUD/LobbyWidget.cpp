// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/LobbyWidget.h"
#include "Components/VerticalBox.h"
#include "Components/Widget.h"

void ULobbyWidget::AddWidgetToVerticalBox(UWidget* widget)
{
	if (widget && PlayerNameVerticalBox)
	{
		PlayerNameVerticalBox->AddChild(widget);
	}
}
