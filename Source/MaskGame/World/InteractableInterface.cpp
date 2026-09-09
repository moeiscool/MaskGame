// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

#include "World/InteractableInterface.h"

bool IInteractableInterface::CanInteract_Implementation(AMaskCharacter* /*Character*/) const
{
	return true;
}

void IInteractableInterface::Interact_Implementation(AMaskCharacter* /*Character*/)
{
}

FText IInteractableInterface::GetInteractionPrompt_Implementation() const
{
	return NSLOCTEXT("MaskGame", "DefaultInteractPrompt", "Examine");
}
