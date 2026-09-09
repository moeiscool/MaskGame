// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

#include "World/OwlStatue.h"

#include "Components/StaticMeshComponent.h"

#include "Character/MaskCharacter.h"
#include "Core/MaskGameInstance.h"
#include "MaskGame.h"

AOwlStatue::AOwlStatue()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	DisplayName = NSLOCTEXT("MaskGame", "OwlStatueDefaultName", "Owl Statue");
}

void AOwlStatue::BeginPlay()
{
	Super::BeginPlay();

	if (StatueId.IsNone())
	{
		UE_LOG(LogMaskGame, Warning, TEXT("Owl statue '%s' has no StatueId and cannot be recorded."), *GetName());
	}
}

bool AOwlStatue::IsAwakened() const
{
	const UMaskGameInstance* Instance = UMaskGameInstance::Get(this);
	return Instance != nullptr && Instance->HasTouchedStatue(StatueId);
}

bool AOwlStatue::CanInteract_Implementation(AMaskCharacter* Character) const
{
	return Character != nullptr && !StatueId.IsNone();
}

void AOwlStatue::Interact_Implementation(AMaskCharacter* Character)
{
	UMaskGameInstance* Instance = UMaskGameInstance::Get(this);
	if (Instance == nullptr || Character == nullptr)
	{
		return;
	}

	const bool bWasAwakened = IsAwakened();
	Instance->TouchStatue(StatueId);

	// Touching a statue is the game's save point, so the file is written even
	// when the statue was already known: the player came back here on purpose.
	Instance->SaveProgress();

	if (!bWasAwakened)
	{
		UE_LOG(LogMaskGame, Log, TEXT("Owl statue '%s' awakened."), *StatueId.ToString());
	}
}

FText AOwlStatue::GetInteractionPrompt_Implementation() const
{
	return IsAwakened()
		? NSLOCTEXT("MaskGame", "OwlStatueRest", "Rest at the statue")
		: NSLOCTEXT("MaskGame", "OwlStatueTouch", "Touch the statue");
}
