// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

#include "Core/MaskGameTypes.h"
#include "UI/MaskTouchOverlay.h"

#include "MaskPlayerController.generated.h"

class AMaskCharacter;
class UUserWidget;

/**
 * The player's controller.
 *
 * Thin by design: movement and masks belong to the pawn, the clock belongs to
 * the world, and progression belongs to the game instance. What is left is the
 * HUD, the input mode, and - on phones and tablets - owning the on-screen
 * controls and feeding what they report into the pawn's action API.
 */
UCLASS()
class MASKGAME_API AMaskPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AMaskPlayerController();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void PlayerTick(float DeltaTime) override;

	/** Widget class for the clock, hearts, magic and mask slots. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> HudWidgetClass;

	UFUNCTION(BlueprintPure, Category = "UI")
	UUserWidget* GetHudWidget() const { return HudWidget; }

	/** True when this controller is driving the on-screen touch controls. */
	UFUNCTION(BlueprintPure, Category = "UI")
	bool IsUsingTouchControls() const { return TouchOverlay.IsValid(); }

private:
	/** Whether this controller should show touch controls at all. */
	bool ShouldUseTouchControls() const;

	void CreateTouchOverlay();
	void DestroyTouchOverlay();

	/** Reads the overlay's stick and swipe and applies them to the pawn. */
	void ApplyTouchInput(float DeltaTime);

	/** Turns one touch button into a call on the pawn's action API. */
	void HandleTouchAction(EMaskTouchAction Action, bool bPressed);

	AMaskCharacter* GetMaskCharacter() const;

	UPROPERTY()
	TObjectPtr<UUserWidget> HudWidget;

	TSharedPtr<SMaskTouchOverlay> TouchOverlay;

	/** Tracks the ocarina so the overlay can swap to its note layout. */
	bool bOverlayInOcarinaMode = false;
};
