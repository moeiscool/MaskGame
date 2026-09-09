// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

#include "Core/MaskPlayerController.h"

#include "Blueprint/UserWidget.h"

AMaskPlayerController::AMaskPlayerController()
{
	bShowMouseCursor = false;
}

void AMaskPlayerController::BeginPlay()
{
	Super::BeginPlay();

	SetInputMode(FInputModeGameOnly());

	// The HUD is optional: the greybox is playable without one, and the class is
	// left unset until a widget blueprint exists.
	if (HudWidgetClass != nullptr)
	{
		HudWidget = CreateWidget<UUserWidget>(this, HudWidgetClass);
		if (HudWidget != nullptr)
		{
			HudWidget->AddToViewport();
		}
	}
}
