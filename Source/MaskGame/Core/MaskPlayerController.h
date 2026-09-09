// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

#include "MaskPlayerController.generated.h"

/**
 * The player's controller.
 *
 * Thin by design: movement and masks belong to the pawn, the clock belongs to
 * the world, and progression belongs to the game instance. What is left is the
 * HUD and the input mode, which is all this class owns.
 */
UCLASS()
class MASKGAME_API AMaskPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AMaskPlayerController();

	virtual void BeginPlay() override;

	/** Widget class for the clock, hearts, magic and mask slots. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<class UUserWidget> HudWidgetClass;

	UFUNCTION(BlueprintPure, Category = "UI")
	UUserWidget* GetHudWidget() const { return HudWidget; }

private:
	UPROPERTY()
	TObjectPtr<UUserWidget> HudWidget;
};
