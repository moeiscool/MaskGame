// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Core/MaskGameTypes.h"
#include "World/InteractableInterface.h"

#include "TreasureChest.generated.h"

class UStaticMeshComponent;

/** What a chest can hold. */
UENUM(BlueprintType)
enum class EChestContents : uint8
{
	Rupees        UMETA(DisplayName = "Rupees"),
	Mask          UMETA(DisplayName = "Mask"),
	HeartFragment UMETA(DisplayName = "Heart Fragment"),
	DungeonKey    UMETA(DisplayName = "Small Key"),
	Arrows        UMETA(DisplayName = "Arrows"),
	Bombs         UMETA(DisplayName = "Bombs"),
	MagicMeter    UMETA(DisplayName = "Magic Meter"),
};

/**
 * A chest.
 *
 * Whether it stays open through a rewind is a property of what is inside: a
 * mask or a heart fragment is permanent progression and the chest stays open
 * forever, while rupees and keys come back with the cycle. That decision is
 * expressed by the flag the chest writes, not by a separate switch, so the two
 * can never disagree.
 */
UCLASS()
class MASKGAME_API ATreasureChest : public AActor, public IInteractableInterface
{
	GENERATED_BODY()

public:
	ATreasureChest();

	virtual void BeginPlay() override;

	// IInteractableInterface.
	virtual bool CanInteract_Implementation(AMaskCharacter* Character) const override;
	virtual void Interact_Implementation(AMaskCharacter* Character) override;
	virtual FText GetInteractionPrompt_Implementation() const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest")
	FName ChestId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest")
	EChestContents Contents = EChestContents::Rupees;

	/** Amount for the countable contents; ignored for a mask or a fragment. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest", meta = (ClampMin = "1"))
	int32 Amount = 20;

	/** Which mask, when Contents is Mask. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest")
	EMaskType Mask = EMaskType::None;

	UFUNCTION(BlueprintPure, Category = "Chest")
	bool IsOpened() const;

	/** The flag this chest writes when opened. Permanent contents get a "perm." flag. */
	UFUNCTION(BlueprintPure, Category = "Chest")
	FName GetOpenedFlag() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;
};
