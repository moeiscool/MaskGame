// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "InteractableInterface.generated.h"

class AMaskCharacter;

UINTERFACE(MinimalAPI, Blueprintable)
class UInteractableInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Anything the player can walk up to and press Interact on: a person to talk to,
 * a chest to open, an owl statue to touch, a sign to read.
 *
 * CanInteract is asked first so that an actor can refuse based on the form the
 * player is wearing or the hour of the cycle - a shopkeeper is not behind the
 * counter at three in the morning, and a boulderkin cannot read fine print.
 */
class MASKGAME_API IInteractableInterface
{
	GENERATED_BODY()

public:
	/** Whether this actor will respond to the player right now. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	bool CanInteract(AMaskCharacter* Character) const;

	/** Do the thing. Only called when CanInteract returned true. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void Interact(AMaskCharacter* Character);

	/** Short prompt shown when the player is in range, e.g. "Touch the statue". */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	FText GetInteractionPrompt() const;
};
