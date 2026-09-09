// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "Core/MaskGameTypes.h"

#include "MaskInventoryComponent.generated.h"

class UMaskGameInstance;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFormChanged, EMaskForm, OldForm, EMaskForm, NewForm);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWornMaskChanged, EMaskType, Mask);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMaskRefused, EMaskType, Mask, FText, Reason);

/**
 * The player's face.
 *
 * Holds which form is being worn and which regular mask sits on it, and is the
 * only place that decides whether a mask may go on right now. Ownership lives on
 * UMaskGameInstance because it survives the cycle; this component is about the
 * here and now.
 */
UCLASS(ClassGroup = (MaskGame), meta = (BlueprintSpawnableComponent))
class MASKGAME_API UMaskInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMaskInventoryComponent();

	virtual void BeginPlay() override;

	/**
	 * Put on a mask.
	 *
	 * Transformation masks change the body; regular masks sit on the traveller's
	 * own face and are refused while transformed. Returns false and broadcasts
	 * OnMaskRefused with a reason the HUD can show when the mask cannot go on.
	 */
	UFUNCTION(BlueprintCallable, Category = "Masks")
	bool EquipMask(EMaskType Mask);

	/** Take off whatever is being worn and return to the traveller's own body. */
	UFUNCTION(BlueprintCallable, Category = "Masks")
	void RemoveMask();

	/** True when this mask could be put on at this moment. */
	UFUNCTION(BlueprintPure, Category = "Masks")
	bool CanEquipMask(EMaskType Mask) const;

	UFUNCTION(BlueprintPure, Category = "Masks")
	EMaskForm GetCurrentForm() const { return CurrentForm; }

	/** The regular mask being worn, or None. Never a transformation mask. */
	UFUNCTION(BlueprintPure, Category = "Masks")
	EMaskType GetWornMask() const { return WornMask; }

	/** Movement speed multiplier from the worn mask, 1.0 when none applies. */
	UFUNCTION(BlueprintPure, Category = "Masks")
	float GetWornSpeedMultiplier() const;

	/**
	 * Whether the wrath mask may come out.
	 *
	 * Set by the boss arena the player is standing in; false everywhere else.
	 */
	UFUNCTION(BlueprintCallable, Category = "Masks")
	void SetInBossArena(bool bInArena);

	UFUNCTION(BlueprintPure, Category = "Masks")
	bool IsInBossArena() const { return bInBossArena; }

	/** Quick-slot assignment, driven by the number keys. */
	UFUNCTION(BlueprintCallable, Category = "Masks")
	void AssignQuickSlot(int32 SlotIndex, EMaskType Mask);

	/** Equip whatever is in a quick slot. Slots are one-based to match the keys. */
	UFUNCTION(BlueprintCallable, Category = "Masks")
	bool EquipQuickSlot(int32 SlotIndex);

	UPROPERTY(BlueprintAssignable, Category = "Masks")
	FOnFormChanged OnFormChanged;

	UPROPERTY(BlueprintAssignable, Category = "Masks")
	FOnWornMaskChanged OnWornMaskChanged;

	UPROPERTY(BlueprintAssignable, Category = "Masks")
	FOnMaskRefused OnMaskRefused;

	/** Masks bound to the number keys, in order. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Masks")
	TArray<EMaskType> QuickSlots;

private:
	/** Reason a refused mask could not go on, for the HUD. */
	FText BuildRefusalReason(EMaskType Mask) const;

	UMaskGameInstance* GetProgressionOwner() const;

	UPROPERTY(VisibleInstanceOnly, Category = "Masks")
	EMaskForm CurrentForm = EMaskForm::Traveller;

	UPROPERTY(VisibleInstanceOnly, Category = "Masks")
	EMaskType WornMask = EMaskType::None;

	UPROPERTY(VisibleInstanceOnly, Category = "Masks")
	bool bInBossArena = false;
};
