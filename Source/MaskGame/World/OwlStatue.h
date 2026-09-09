// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "World/InteractableInterface.h"

#include "OwlStatue.generated.h"

class UStaticMeshComponent;

/**
 * A weathered owl at the edge of each region.
 *
 * Touching one records the place and saves the game; once touched, the
 * Windfeather Song can bring the player back to it in any later cycle. The
 * record is permanent progression, so a statue stays touched through a rewind.
 */
UCLASS()
class MASKGAME_API AOwlStatue : public AActor, public IInteractableInterface
{
	GENERATED_BODY()

public:
	AOwlStatue();

	virtual void BeginPlay() override;

	// IInteractableInterface.
	virtual bool CanInteract_Implementation(AMaskCharacter* Character) const override;
	virtual void Interact_Implementation(AMaskCharacter* Character) override;
	virtual FText GetInteractionPrompt_Implementation() const override;

	/** Id recorded in progression. Must match the StatueId of a region row. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Statue")
	FName StatueId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Statue")
	FText DisplayName;

	UFUNCTION(BlueprintPure, Category = "Statue")
	bool IsAwakened() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;
};
