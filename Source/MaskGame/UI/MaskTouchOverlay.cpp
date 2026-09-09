// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

#include "UI/MaskTouchOverlay.h"

#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

namespace
{
	/** Radius of the movement stick, as a fraction of the smaller screen dimension. */
	constexpr float StickRadiusScale = 0.16f;
	/** Deflection beyond this fraction of the stick radius reads as full tilt. */
	constexpr float StickFullTiltScale = 0.85f;
	/** Fingers landing left of this fraction of the screen width drive movement. */
	constexpr float MoveZoneWidthFraction = 0.42f;

	const FLinearColor IdleColour(1.0f, 1.0f, 1.0f, 0.18f);
	const FLinearColor PressedColour(1.0f, 0.86f, 0.45f, 0.55f);
	const FLinearColor StickRingColour(1.0f, 1.0f, 1.0f, 0.14f);
	const FLinearColor StickKnobColour(1.0f, 1.0f, 1.0f, 0.35f);
	const FLinearColor LabelColour(1.0f, 1.0f, 1.0f, 0.85f);
}

void SMaskTouchOverlay::Construct(const FArguments& InArgs)
{
	OnAction = InArgs._OnAction;
	BuildButtons();

	// The overlay must never swallow a touch it has no button under, or the game
	// beneath it would stop receiving anything at all.
	SetVisibility(EVisibility::Visible);
	SetCanTick(false);
}

void SMaskTouchOverlay::BuildButtons()
{
	Buttons.Reset();

	const auto Add = [this](EMaskTouchAction Action, float X, float Y, const FText& Label,
		float RadiusScale = 0.075f, bool bOcarinaOnly = false, bool bHiddenInOcarina = false)
	{
		FTouchButton Button;
		Button.Action = Action;
		Button.Centre = FVector2D(X, Y);
		Button.Label = Label;
		Button.RadiusScale = RadiusScale;
		Button.bOcarinaOnly = bOcarinaOnly;
		Button.bHiddenInOcarinaMode = bHiddenInOcarina;
		Buttons.Add(MoveTemp(Button));
	};

	// The action cluster, laid out as a gamepad diamond under the right thumb.
	Add(EMaskTouchAction::Jump,       0.855f, 0.82f, NSLOCTEXT("MaskGame", "TouchJump", "Jump"),   0.085f, false, true);
	Add(EMaskTouchAction::Interact,   0.955f, 0.62f, NSLOCTEXT("MaskGame", "TouchUse", "Use"),     0.075f, false, true);
	Add(EMaskTouchAction::Attack,     0.755f, 0.62f, NSLOCTEXT("MaskGame", "TouchHit", "Hit"),     0.085f, false, true);
	Add(EMaskTouchAction::FormAction, 0.855f, 0.42f, NSLOCTEXT("MaskGame", "TouchForm", "Form"),   0.075f, false, true);
	Add(EMaskTouchAction::LockOn,     0.955f, 0.30f, NSLOCTEXT("MaskGame", "TouchLock", "Lock"),   0.060f, false, true);

	// Always available: the ocarina is how you leave ocarina mode again.
	Add(EMaskTouchAction::Ocarina,    0.060f, 0.14f, NSLOCTEXT("MaskGame", "TouchSong", "Song"),   0.065f);

	// Mask slots along the top, out of the way of both thumbs.
	Add(EMaskTouchAction::MaskSlot1,  0.62f, 0.10f, NSLOCTEXT("MaskGame", "TouchMask1", "1"), 0.050f, false, true);
	Add(EMaskTouchAction::MaskSlot2,  0.70f, 0.10f, NSLOCTEXT("MaskGame", "TouchMask2", "2"), 0.050f, false, true);
	Add(EMaskTouchAction::MaskSlot3,  0.78f, 0.10f, NSLOCTEXT("MaskGame", "TouchMask3", "3"), 0.050f, false, true);
	Add(EMaskTouchAction::MaskSlot4,  0.86f, 0.10f, NSLOCTEXT("MaskGame", "TouchMask4", "4"), 0.050f, false, true);
	Add(EMaskTouchAction::RemoveMask, 0.94f, 0.10f, NSLOCTEXT("MaskGame", "TouchMaskOff", "Off"), 0.050f, false, true);

	// The five notes, in the same diamond the action buttons occupied, so the
	// thumb does not have to learn a second position.
	Add(EMaskTouchAction::NoteUp,    0.855f, 0.42f, NSLOCTEXT("MaskGame", "TouchNoteUp", "Up"),       0.075f, true);
	Add(EMaskTouchAction::NoteLeft,  0.755f, 0.62f, NSLOCTEXT("MaskGame", "TouchNoteLeft", "Left"),   0.075f, true);
	Add(EMaskTouchAction::NoteRight, 0.955f, 0.62f, NSLOCTEXT("MaskGame", "TouchNoteRight", "Right"), 0.075f, true);
	Add(EMaskTouchAction::NoteDown,  0.855f, 0.82f, NSLOCTEXT("MaskGame", "TouchNoteDown", "Down"),   0.075f, true);
	Add(EMaskTouchAction::NoteA,     0.660f, 0.82f, NSLOCTEXT("MaskGame", "TouchNoteA", "A"),         0.085f, true);
}

void SMaskTouchOverlay::SetOcarinaMode(bool bInOcarinaMode)
{
	if (bOcarinaMode == bInOcarinaMode)
	{
		return;
	}

	bOcarinaMode = bInOcarinaMode;

	// Release anything held across the switch, or a button that has just been
	// hidden would stay pressed forever.
	for (const TPair<int32, FActiveTouch>& Pair : ActiveTouches)
	{
		if (Pair.Value.Kind == FActiveTouch::EKind::Button && OnAction.IsBound())
		{
			OnAction.Execute(Pair.Value.Action, false);
		}
	}
	ActiveTouches.Reset();
	MoveAxis = FVector2D::ZeroVector;
}

bool SMaskTouchOverlay::IsButtonVisible(const FTouchButton& Button) const
{
	if (bOcarinaMode)
	{
		return !Button.bHiddenInOcarinaMode;
	}
	return !Button.bOcarinaOnly;
}

void SMaskTouchOverlay::GetButtonGeometry(const FTouchButton& Button, const FVector2D& Size,
	FVector2D& OutCentre, float& OutRadius) const
{
	OutCentre = FVector2D(Button.Centre.X * Size.X, Button.Centre.Y * Size.Y);

	// Sizing from the smaller dimension keeps buttons thumb-sized on a tall
	// phone and a wide tablet alike.
	OutRadius = Button.RadiusScale * FMath::Min(Size.X, Size.Y);
}

FVector2D SMaskTouchOverlay::ConsumeLookDelta()
{
	const FVector2D Delta = PendingLookDelta;
	PendingLookDelta = FVector2D::ZeroVector;
	return Delta;
}

bool SMaskTouchOverlay::BeginTouch(int32 PointerIndex, const FVector2D& Local, const FVector2D& Size)
{
	// Buttons win over the stick and the look area: they are drawn on top, so
	// they should be hit first.
	for (const FTouchButton& Button : Buttons)
	{
		if (!IsButtonVisible(Button))
		{
			continue;
		}

		FVector2D Centre;
		float Radius = 0.0f;
		GetButtonGeometry(Button, Size, Centre, Radius);

		if (FVector2D::DistSquared(Local, Centre) <= Radius * Radius)
		{
			FActiveTouch Touch;
			Touch.Kind = FActiveTouch::EKind::Button;
			Touch.Action = Button.Action;
			Touch.Origin = Local;
			Touch.Current = Local;
			ActiveTouches.Add(PointerIndex, Touch);

			if (OnAction.IsBound())
			{
				OnAction.Execute(Button.Action, true);
			}
			return true;
		}
	}

	// The notes are the only thing worth touching while the ocarina is out;
	// walking and looking are both suspended.
	if (bOcarinaMode)
	{
		return false;
	}

	FActiveTouch Touch;
	Touch.Origin = Local;
	Touch.Current = Local;

	// A floating stick: it appears wherever the left thumb lands rather than at
	// a fixed spot, which is what stops a player having to look down to find it.
	Touch.Kind = (Local.X <= Size.X * MoveZoneWidthFraction)
		? FActiveTouch::EKind::MoveStick
		: FActiveTouch::EKind::Look;

	ActiveTouches.Add(PointerIndex, Touch);
	RefreshMoveAxis(Size);
	return true;
}

void SMaskTouchOverlay::UpdateTouch(int32 PointerIndex, const FVector2D& Local, const FVector2D& Size)
{
	FActiveTouch* Touch = ActiveTouches.Find(PointerIndex);
	if (Touch == nullptr)
	{
		return;
	}

	const FVector2D Previous = Touch->Current;
	Touch->Current = Local;

	switch (Touch->Kind)
	{
	case FActiveTouch::EKind::MoveStick:
		RefreshMoveAxis(Size);
		break;

	case FActiveTouch::EKind::Look:
	{
		// Look is a swipe, not a stick: the camera moves by how far the finger
		// travelled, scaled so that a swipe across the screen is a fixed number
		// of degrees whatever the device resolution.
		const FVector2D Travel = Local - Previous;
		const float Scale = LookSensitivityDegrees / FMath::Max(1.0f, static_cast<float>(Size.Y));
		PendingLookDelta += FVector2D(Travel.X * Scale, -Travel.Y * Scale);
		break;
	}

	case FActiveTouch::EKind::Button:
		// A finger that slides off a button keeps holding it. Re-triggering on
		// re-entry would make the form action stutter during a drag.
		break;
	}
}

void SMaskTouchOverlay::EndTouch(int32 PointerIndex)
{
	FActiveTouch Touch;
	if (!ActiveTouches.RemoveAndCopyValue(PointerIndex, Touch))
	{
		return;
	}

	if (Touch.Kind == FActiveTouch::EKind::Button && OnAction.IsBound())
	{
		OnAction.Execute(Touch.Action, false);
	}

	RefreshMoveAxis(LastKnownSize);
}

void SMaskTouchOverlay::RefreshMoveAxis(const FVector2D& Size)
{
	MoveAxis = FVector2D::ZeroVector;

	for (const TPair<int32, FActiveTouch>& Pair : ActiveTouches)
	{
		if (Pair.Value.Kind != FActiveTouch::EKind::MoveStick)
		{
			continue;
		}

		const float Radius = StickRadiusScale * FMath::Min(Size.X, Size.Y);
		const FVector2D Offset = Pair.Value.Current - Pair.Value.Origin;

		// Full tilt slightly before the edge of the ring, so a player does not
		// have to reach the exact boundary to run at full speed.
		const float FullTilt = FMath::Max(1.0f, Radius * StickFullTiltScale);
		const FVector2D Normalised = Offset / FullTilt;
		const float Length = Normalised.Size();

		// Screen Y grows downwards, so forward is negative Y.
		MoveAxis = (Length > 1.0f) ? (Normalised / Length) : Normalised;
		MoveAxis.Y = -MoveAxis.Y;
		break;
	}
}

FReply SMaskTouchOverlay::OnTouchStarted(const FGeometry& Geometry, const FPointerEvent& Event)
{
	LastKnownSize = Geometry.GetLocalSize();
	const FVector2D Local = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
	return BeginTouch(Event.GetPointerIndex(), Local, LastKnownSize)
		? FReply::Handled().CaptureMouse(SharedThis(this))
		: FReply::Unhandled();
}

FReply SMaskTouchOverlay::OnTouchMoved(const FGeometry& Geometry, const FPointerEvent& Event)
{
	LastKnownSize = Geometry.GetLocalSize();
	UpdateTouch(Event.GetPointerIndex(), Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition()), LastKnownSize);
	return FReply::Handled();
}

FReply SMaskTouchOverlay::OnTouchEnded(const FGeometry& Geometry, const FPointerEvent& Event)
{
	LastKnownSize = Geometry.GetLocalSize();
	EndTouch(Event.GetPointerIndex());
	return FReply::Handled().ReleaseMouseCapture();
}

FReply SMaskTouchOverlay::OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event)
{
	return OnTouchStarted(Geometry, Event);
}

FReply SMaskTouchOverlay::OnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event)
{
	// Only forward a drag; a mouse moving with no button down is not a finger.
	if (ActiveTouches.IsEmpty())
	{
		return FReply::Unhandled();
	}
	return OnTouchMoved(Geometry, Event);
}

FReply SMaskTouchOverlay::OnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& Event)
{
	return OnTouchEnded(Geometry, Event);
}

int32 SMaskTouchOverlay::OnPaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const
{
	const FVector2D Size = Geometry.GetLocalSize();
	LastKnownSize = Size;

	const FSlateBrush* Brush = FCoreStyle::Get().GetDefaultBrush();
	const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Bold", 16);

	// Squares rather than circles: the engine's default brush is a plain white
	// box and this project ships no art. It reads fine as a greybox control and
	// is one line to swap for a rounded brush later.
	const auto DrawSquare = [&](const FVector2D& Centre, float Radius, const FLinearColor& Colour, int32 Layer)
	{
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			Layer,
			Geometry.ToPaintGeometry(FVector2f(Radius * 2.0f, Radius * 2.0f),
				FSlateLayoutTransform(FVector2f(Centre.X - Radius, Centre.Y - Radius))),
			Brush,
			ESlateDrawEffect::None,
			Colour);
	};

	// The movement stick, drawn where the finger actually put it.
	for (const TPair<int32, FActiveTouch>& Pair : ActiveTouches)
	{
		if (Pair.Value.Kind != FActiveTouch::EKind::MoveStick)
		{
			continue;
		}

		const float Radius = StickRadiusScale * FMath::Min(Size.X, Size.Y);
		DrawSquare(Pair.Value.Origin, Radius, StickRingColour, LayerId);

		const FVector2D Offset = Pair.Value.Current - Pair.Value.Origin;
		const FVector2D Clamped = Offset.SizeSquared() > Radius * Radius
			? Offset.GetSafeNormal() * Radius
			: Offset;
		DrawSquare(Pair.Value.Origin + Clamped, Radius * 0.38f, StickKnobColour, LayerId + 1);
	}

	// Buttons, and their labels on top.
	for (const FTouchButton& Button : Buttons)
	{
		if (!IsButtonVisible(Button))
		{
			continue;
		}

		FVector2D Centre;
		float Radius = 0.0f;
		GetButtonGeometry(Button, Size, Centre, Radius);

		bool bPressed = false;
		for (const TPair<int32, FActiveTouch>& Pair : ActiveTouches)
		{
			if (Pair.Value.Kind == FActiveTouch::EKind::Button && Pair.Value.Action == Button.Action)
			{
				bPressed = true;
				break;
			}
		}

		DrawSquare(Centre, Radius, bPressed ? PressedColour : IdleColour, LayerId + 2);

		FSlateDrawElement::MakeText(
			OutDrawElements,
			LayerId + 3,
			Geometry.ToPaintGeometry(FVector2f(Radius * 2.0f, Radius),
				FSlateLayoutTransform(FVector2f(Centre.X - Radius * 0.8f, Centre.Y - Radius * 0.25f))),
			Button.Label,
			Font,
			ESlateDrawEffect::None,
			LabelColour);
	}

	return SCompoundWidget::OnPaint(Args, Geometry, CullingRect, OutDrawElements, LayerId + 4, Style, bParentEnabled);
}
