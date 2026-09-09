// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

#include "Core/MaskGameTypes.h"

/** Every control the touch overlay can offer. */
enum class EMaskTouchAction : uint8
{
	None,
	Jump,
	Interact,
	Attack,
	FormAction,
	LockOn,
	Ocarina,
	RemoveMask,
	MaskSlot1,
	MaskSlot2,
	MaskSlot3,
	MaskSlot4,
	NoteUp,
	NoteDown,
	NoteLeft,
	NoteRight,
	NoteA,
};

/** Fired when a touch button goes down or comes up. */
DECLARE_DELEGATE_TwoParams(FOnMaskTouchAction, EMaskTouchAction /*Action*/, bool /*bPressed*/);

/**
 * The on-screen controls for phones and tablets.
 *
 * One full-screen widget owns every touch rather than composing a tree of
 * buttons, because multi-touch is the whole problem here: a player is holding
 * the movement stick with their left thumb while tapping attack with their
 * right, and both fingers have to be tracked independently. Slate hands each
 * finger to whichever widget it started on, so a single widget that dispatches
 * by position is far simpler than a dozen that each have to cope with being
 * interrupted.
 *
 * It draws itself rather than using any art, so it works in a project with no
 * authored content. Replacing it with a UMG layout later means keeping the
 * delegates and throwing away the paint code.
 *
 * The layout is modal in the same way the physical controls are: while the
 * ocarina is drawn the action cluster becomes the five notes, because a phone
 * screen has even less room than a gamepad has buttons.
 */
class MASKGAME_API SMaskTouchOverlay : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMaskTouchOverlay) {}
		/** Called when a button is pressed or released. */
		SLATE_EVENT(FOnMaskTouchAction, OnAction)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// SWidget.
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(1920.0f, 1080.0f); }
	virtual bool SupportsKeyboardFocus() const override { return false; }

	virtual FReply OnTouchStarted(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply OnTouchMoved(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply OnTouchEnded(const FGeometry& Geometry, const FPointerEvent& Event) override;

	// Mouse is handled as well so the overlay can be tested on a desktop build
	// with MaskGame.Touch.Force 1, without needing a touchscreen.
	virtual FReply OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply OnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply OnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& Event) override;

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& Style,
		bool bParentEnabled) const override;

	/** Movement axes, -1..1, read once per frame by the player controller. */
	FVector2D GetMoveAxis() const { return MoveAxis; }

	/**
	 * Look delta accumulated since the last call, in degrees, and cleared by it.
	 * Consuming rather than sampling means a frame that drops does not lose the
	 * swipe that happened during it.
	 */
	FVector2D ConsumeLookDelta();

	/** Swap the action cluster for the five ocarina notes, and back. */
	void SetOcarinaMode(bool bInOcarinaMode);

	/** Points per screen height that a full swipe of the look area turns the camera. */
	float LookSensitivityDegrees = 140.0f;

private:
	/** One round button, positioned in fractions of the screen. */
	struct FTouchButton
	{
		EMaskTouchAction Action = EMaskTouchAction::None;
		/** Centre, as a fraction of screen width and height. */
		FVector2D Centre = FVector2D::ZeroVector;
		FText Label;
		/** Radius as a fraction of the smaller screen dimension. */
		float RadiusScale = 1.0f;
		/** True for buttons that only appear while the ocarina is drawn. */
		bool bOcarinaOnly = false;
		/** True for buttons hidden while the ocarina is drawn. */
		bool bHiddenInOcarinaMode = false;
	};

	/** A finger currently down, and what it is doing. */
	struct FActiveTouch
	{
		enum class EKind : uint8 { Button, MoveStick, Look };

		EKind Kind = EKind::Look;
		EMaskTouchAction Action = EMaskTouchAction::None;
		/** Where the finger went down, in local space. */
		FVector2D Origin = FVector2D::ZeroVector;
		/** Where it is now. */
		FVector2D Current = FVector2D::ZeroVector;
	};

	void BuildButtons();

	/** Screen-space centre and radius of a button, in local coordinates. */
	void GetButtonGeometry(const FTouchButton& Button, const FVector2D& Size, FVector2D& OutCentre, float& OutRadius) const;

	bool IsButtonVisible(const FTouchButton& Button) const;

	/** Begin tracking a finger. Returns true when the overlay claimed it. */
	bool BeginTouch(int32 PointerIndex, const FVector2D& Local, const FVector2D& Size);
	void UpdateTouch(int32 PointerIndex, const FVector2D& Local, const FVector2D& Size);
	void EndTouch(int32 PointerIndex);

	/** Recompute MoveAxis from whichever finger owns the stick. */
	void RefreshMoveAxis(const FVector2D& Size);

	FOnMaskTouchAction OnAction;

	TArray<FTouchButton> Buttons;
	TMap<int32, FActiveTouch> ActiveTouches;

	FVector2D MoveAxis = FVector2D::ZeroVector;
	FVector2D PendingLookDelta = FVector2D::ZeroVector;

	bool bOcarinaMode = false;

	/** Cached each paint so touch handling and drawing agree on where things are. */
	mutable FVector2D LastKnownSize = FVector2D(1920.0f, 1080.0f);
};
