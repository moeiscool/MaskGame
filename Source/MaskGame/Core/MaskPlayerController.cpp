// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

#include "Core/MaskPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "HAL/IConsoleManager.h"

#include "Character/MaskCharacter.h"
#include "MaskGame.h"

static TAutoConsoleVariable<int32> CVarForceTouchControls(
	TEXT("MaskGame.Touch.Force"),
	0,
	TEXT("Show the on-screen touch controls on a platform that would not normally have them.\n")
	TEXT("  0: only on touch devices (default)\n")
	TEXT("  1: always show them\n")
	TEXT("  2: never show them"),
	ECVF_Default);

AMaskPlayerController::AMaskPlayerController()
{
	bShowMouseCursor = false;
}

bool AMaskPlayerController::ShouldUseTouchControls() const
{
	const int32 Override = CVarForceTouchControls.GetValueOnGameThread();
	if (Override == 1)
	{
		return true;
	}
	if (Override == 2)
	{
		return false;
	}

	// Only the first local player gets touch controls. A phone has one screen
	// and one pair of thumbs; split-screen is a desktop idea.
	const ULocalPlayer* Player = GetLocalPlayer();
	if (Player == nullptr || Player->GetControllerId() != 0)
	{
		return false;
	}

#if PLATFORM_ANDROID || PLATFORM_IOS
	return true;
#else
	// Covers a desktop build running with -faketouches or on a touchscreen PC.
	return FPlatformMisc::GetUseVirtualJoysticks();
#endif
}

void AMaskPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (ShouldUseTouchControls())
	{
		CreateTouchOverlay();
	}
	else
	{
		SetInputMode(FInputModeGameOnly());
	}

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

void AMaskPlayerController::EndPlay(const EEndPlayReason::Type Reason)
{
	DestroyTouchOverlay();
	Super::EndPlay(Reason);
}

void AMaskPlayerController::CreateTouchOverlay()
{
	UGameViewportClient* Viewport = GetWorld() != nullptr ? GetWorld()->GetGameViewport() : nullptr;
	if (Viewport == nullptr || TouchOverlay.IsValid())
	{
		return;
	}

	TouchOverlay = SNew(SMaskTouchOverlay)
		.OnAction(FOnMaskTouchAction::CreateUObject(this, &AMaskPlayerController::HandleTouchAction));

	// Below any UMG the HUD adds, so a menu can still be tapped through.
	Viewport->AddViewportWidgetContent(TouchOverlay.ToSharedRef(), /*ZOrder=*/0);

	// GameAndUI, or the viewport would swallow every touch before the overlay
	// saw it. The cursor stays hidden; on a phone there is not one anyway.
	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(true);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
	SetInputMode(InputMode);
	bShowMouseCursor = false;

	UE_LOG(LogMaskGame, Log, TEXT("On-screen touch controls enabled."));
}

void AMaskPlayerController::DestroyTouchOverlay()
{
	if (!TouchOverlay.IsValid())
	{
		return;
	}

	if (UGameViewportClient* Viewport = GetWorld() != nullptr ? GetWorld()->GetGameViewport() : nullptr)
	{
		Viewport->RemoveViewportWidgetContent(TouchOverlay.ToSharedRef());
	}
	TouchOverlay.Reset();
}

AMaskCharacter* AMaskPlayerController::GetMaskCharacter() const
{
	return Cast<AMaskCharacter>(GetPawn());
}

void AMaskPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	if (TouchOverlay.IsValid())
	{
		ApplyTouchInput(DeltaTime);
	}
}

void AMaskPlayerController::ApplyTouchInput(float DeltaTime)
{
	AMaskCharacter* Character = GetMaskCharacter();
	if (Character == nullptr)
	{
		return;
	}

	// Keep the overlay's layout in step with the ocarina, so drawing it swaps
	// the action cluster for the five notes.
	const bool bDrawn = Character->IsOcarinaDrawn();
	if (bDrawn != bOverlayInOcarinaMode)
	{
		bOverlayInOcarinaMode = bDrawn;
		TouchOverlay->SetOcarinaMode(bDrawn);
	}

	const FVector2D Move = TouchOverlay->GetMoveAxis();
	if (!Move.IsNearlyZero())
	{
		Character->ApplyMoveInput(static_cast<float>(Move.Y), static_cast<float>(Move.X));
	}

	// The swipe is already in degrees, and is consumed rather than sampled so a
	// dropped frame does not throw away the movement that happened during it.
	const FVector2D Look = TouchOverlay->ConsumeLookDelta();
	if (!Look.IsNearlyZero())
	{
		Character->ApplyLookInput(static_cast<float>(Look.X), static_cast<float>(Look.Y));
	}
}

void AMaskPlayerController::HandleTouchAction(EMaskTouchAction Action, bool bPressed)
{
	AMaskCharacter* Character = GetMaskCharacter();
	if (Character == nullptr)
	{
		return;
	}

	// The form action is the only held button; everything else fires on press.
	if (Action == EMaskTouchAction::FormAction)
	{
		bPressed ? Character->StartFormAction() : Character->StopFormAction();
		return;
	}

	if (Action == EMaskTouchAction::Jump)
	{
		bPressed ? Character->RequestJump() : Character->RequestStopJump();
		return;
	}

	if (!bPressed)
	{
		return;
	}

	switch (Action)
	{
	case EMaskTouchAction::Interact:   Character->TryInteract(); break;
	case EMaskTouchAction::Attack:     Character->PerformAttack(); break;
	case EMaskTouchAction::LockOn:     Character->ToggleLockOn(); break;
	case EMaskTouchAction::Ocarina:    Character->ToggleOcarina(); break;
	case EMaskTouchAction::RemoveMask: Character->RemoveMask(); break;

	case EMaskTouchAction::MaskSlot1:  Character->EquipMaskSlot(1); break;
	case EMaskTouchAction::MaskSlot2:  Character->EquipMaskSlot(2); break;
	case EMaskTouchAction::MaskSlot3:  Character->EquipMaskSlot(3); break;
	case EMaskTouchAction::MaskSlot4:  Character->EquipMaskSlot(4); break;

	case EMaskTouchAction::NoteUp:     Character->PlayOcarinaNote(EOcarinaNote::Up); break;
	case EMaskTouchAction::NoteDown:   Character->PlayOcarinaNote(EOcarinaNote::Down); break;
	case EMaskTouchAction::NoteLeft:   Character->PlayOcarinaNote(EOcarinaNote::Left); break;
	case EMaskTouchAction::NoteRight:  Character->PlayOcarinaNote(EOcarinaNote::Right); break;
	case EMaskTouchAction::NoteA:      Character->PlayOcarinaNote(EOcarinaNote::A); break;

	default: break;
	}
}
