// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

#include "Masks/MaskInventoryComponent.h"

#include "Core/MaskGameInstance.h"
#include "MaskGame.h"
#include "MaskRules.h"

UMaskInventoryComponent::UMaskInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// The three transformation masks and the hare hood are what a player reaches
	// for most; the slots are rebindable from the mask screen.
	QuickSlots = { EMaskType::Sapling, EMaskType::Boulderkin, EMaskType::Tideborn, EMaskType::HareHood };
}

void UMaskInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	// Announce the starting form so anything that dresses the pawn - mesh, camera
	// offset, movement tuning - runs through the same path a mask change does.
	OnFormChanged.Broadcast(EMaskForm::Traveller, CurrentForm);
}

UMaskGameInstance* UMaskInventoryComponent::GetProgressionOwner() const
{
	return UMaskGameInstance::Get(this);
}

bool UMaskInventoryComponent::CanEquipMask(EMaskType Mask) const
{
	const UMaskGameInstance* Instance = GetProgressionOwner();
	if (Instance == nullptr)
	{
		return false;
	}

	return MaskGame::CanEquipMask(
		Instance->GetProgression(),
		static_cast<MaskGame::EMaskId>(Mask),
		static_cast<MaskGame::EForm>(CurrentForm),
		bInBossArena);
}

FText UMaskInventoryComponent::BuildRefusalReason(EMaskType Mask) const
{
	const UMaskGameInstance* Instance = GetProgressionOwner();
	if (Instance == nullptr || !Instance->HasMask(Mask))
	{
		return NSLOCTEXT("MaskGame", "MaskNotOwned", "You do not have that mask.");
	}

	if (Mask == EMaskType::Wrath)
	{
		return NSLOCTEXT("MaskGame", "MaskNeedsArena", "Nothing here is worth what this mask costs.");
	}

	return NSLOCTEXT("MaskGame", "MaskNeedsOwnFace", "This mask needs a face of your own to sit on.");
}

bool UMaskInventoryComponent::EquipMask(EMaskType Mask)
{
	if (!CanEquipMask(Mask))
	{
		const FText Reason = BuildRefusalReason(Mask);
		UE_LOG(LogMaskGame, Verbose, TEXT("Refused mask %s: %s"),
			*UEnum::GetDisplayValueAsText(Mask).ToString(), *Reason.ToString());
		OnMaskRefused.Broadcast(Mask, Reason);
		return false;
	}

	const MaskGame::EMaskId RulesMask = static_cast<MaskGame::EMaskId>(Mask);

	if (MaskGame::IsTransformationMask(RulesMask))
	{
		const EMaskForm NewForm = static_cast<EMaskForm>(MaskGame::GetFormForMask(RulesMask));
		if (NewForm == CurrentForm)
		{
			return true;
		}

		// Changing body drops whatever regular mask was on the old face.
		if (WornMask != EMaskType::None)
		{
			WornMask = EMaskType::None;
			OnWornMaskChanged.Broadcast(WornMask);
		}

		const EMaskForm OldForm = CurrentForm;
		CurrentForm = NewForm;
		UE_LOG(LogMaskGame, Log, TEXT("Form changed to %s."), *UEnum::GetDisplayValueAsText(CurrentForm).ToString());
		OnFormChanged.Broadcast(OldForm, CurrentForm);
		return true;
	}

	if (WornMask == Mask)
	{
		return true;
	}

	WornMask = Mask;
	OnWornMaskChanged.Broadcast(WornMask);
	return true;
}

void UMaskInventoryComponent::RemoveMask()
{
	if (WornMask != EMaskType::None)
	{
		WornMask = EMaskType::None;
		OnWornMaskChanged.Broadcast(WornMask);
		return;
	}

	if (CurrentForm != EMaskForm::Traveller)
	{
		const EMaskForm OldForm = CurrentForm;
		CurrentForm = EMaskForm::Traveller;
		OnFormChanged.Broadcast(OldForm, CurrentForm);
	}
}

float UMaskInventoryComponent::GetWornSpeedMultiplier() const
{
	if (WornMask == EMaskType::None)
	{
		return 1.0f;
	}
	return MaskGame::GetMaskTraits(static_cast<MaskGame::EMaskId>(WornMask)).SpeedMultiplier;
}

void UMaskInventoryComponent::SetInBossArena(bool bInArena)
{
	if (bInBossArena == bInArena)
	{
		return;
	}

	bInBossArena = bInArena;

	// Leaving the arena takes the wrath mask off; it is not a form to walk around in.
	if (!bInBossArena && CurrentForm == EMaskForm::Wrath)
	{
		const EMaskForm OldForm = CurrentForm;
		CurrentForm = EMaskForm::Traveller;
		OnFormChanged.Broadcast(OldForm, CurrentForm);
	}
}

void UMaskInventoryComponent::AssignQuickSlot(int32 SlotIndex, EMaskType Mask)
{
	const int32 Index = SlotIndex - 1;
	if (Index < 0)
	{
		return;
	}

	if (!QuickSlots.IsValidIndex(Index))
	{
		QuickSlots.SetNum(Index + 1);
	}
	QuickSlots[Index] = Mask;
}

bool UMaskInventoryComponent::EquipQuickSlot(int32 SlotIndex)
{
	const int32 Index = SlotIndex - 1;
	return QuickSlots.IsValidIndex(Index) && EquipMask(QuickSlots[Index]);
}
