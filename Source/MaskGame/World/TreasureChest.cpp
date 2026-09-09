// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

#include "World/TreasureChest.h"

#include "Components/StaticMeshComponent.h"

#include "Character/MaskCharacter.h"
#include "Core/MaskGameInstance.h"
#include "MaskGame.h"

ATreasureChest::ATreasureChest()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
}

void ATreasureChest::BeginPlay()
{
	Super::BeginPlay();

	// A chest whose contents are already banked is not worth walking to; hide it
	// rather than letting the player cross a room for an empty box.
	if (IsOpened())
	{
		SetActorHiddenInGame(true);
		SetActorEnableCollision(false);
	}
}

FName ATreasureChest::GetOpenedFlag() const
{
	if (ChestId.IsNone())
	{
		return NAME_None;
	}

	// Contents the rewind keeps write a permanent flag, so the chest stays open;
	// everything else writes a cycle-scoped one and refills at the next dawn.
	const bool bPermanent = Contents == EChestContents::Mask
		|| Contents == EChestContents::HeartFragment
		|| Contents == EChestContents::MagicMeter;
	return FName(*FString::Printf(TEXT("%schest.%s"), bPermanent ? TEXT("perm.") : TEXT(""), *ChestId.ToString()));
}

bool ATreasureChest::IsOpened() const
{
	const UMaskGameInstance* Instance = UMaskGameInstance::Get(this);
	const FName Flag = GetOpenedFlag();
	return Instance != nullptr && !Flag.IsNone() && Instance->HasFlag(Flag);
}

bool ATreasureChest::CanInteract_Implementation(AMaskCharacter* Character) const
{
	return Character != nullptr && !ChestId.IsNone() && !IsOpened();
}

void ATreasureChest::Interact_Implementation(AMaskCharacter* Character)
{
	UMaskGameInstance* Instance = UMaskGameInstance::Get(this);
	if (Instance == nullptr || Character == nullptr || IsOpened())
	{
		return;
	}

	switch (Contents)
	{
	case EChestContents::Rupees:
		Instance->AddRupees(Amount);
		break;

	case EChestContents::Mask:
		Instance->GiveMask(Mask);
		break;

	case EChestContents::HeartFragment:
		Instance->GiveHeartFragment();
		break;

	case EChestContents::DungeonKey:
		Instance->GetProgression().Carried().DungeonKeys += Amount;
		break;

	case EChestContents::Arrows:
	{
		MaskGame::FConsumables& Carried = Instance->GetProgression().Carried();
		Carried.Arrows = FMath::Min(Carried.Arrows + Amount, Instance->GetProgression().Equipment().GetMaxArrows());
		break;
	}

	case EChestContents::Bombs:
	{
		MaskGame::FConsumables& Carried = Instance->GetProgression().Carried();
		Carried.Bombs = FMath::Min(Carried.Bombs + Amount, Instance->GetProgression().Equipment().GetMaxBombs());
		break;
	}

	case EChestContents::MagicMeter:
	{
		MaskGame::FEquipmentTiers& Tiers = Instance->GetProgression().Equipment();
		Tiers.MagicTier = FMath::Max(Tiers.MagicTier, Amount);
		Character->GrantMagicMeter(static_cast<float>(Tiers.GetMaxMagic()));
		break;
	}
	}

	Instance->SetFlag(GetOpenedFlag(), 1);
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);

	UE_LOG(LogMaskGame, Log, TEXT("Opened chest '%s'."), *ChestId.ToString());
}

FText ATreasureChest::GetInteractionPrompt_Implementation() const
{
	return NSLOCTEXT("MaskGame", "ChestOpen", "Open the chest");
}
