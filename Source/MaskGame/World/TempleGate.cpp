// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

#include "World/TempleGate.h"

#include "Components/StaticMeshComponent.h"

#include "Character/MaskCharacter.h"
#include "MaskGame.h"
#include "Masks/MaskInventoryComponent.h"

ATempleGate::ATempleGate()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionProfileName(TEXT("BlockAll"));
}

void ATempleGate::BeginPlay()
{
	Super::BeginPlay();
	Close();
}

float ATempleGate::GetHearingRadius_Implementation() const
{
	// A temple gate answers only someone standing in front of it, not a song
	// played from the far side of the swamp.
	return 1200.0f;
}

bool ATempleGate::OnSongHeard_Implementation(ESongType Song, ESongPerformance /*Performance*/, AMaskCharacter* /*Performer*/)
{
	if (bOpen || Song != RequiredSong)
	{
		return false;
	}

	Open();
	return true;
}

void ATempleGate::Open()
{
	if (bOpen)
	{
		return;
	}

	bOpen = true;
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	UE_LOG(LogMaskGame, Log, TEXT("The way into %s stands open."), *DisplayName.ToString());
}

void ATempleGate::Close()
{
	bOpen = false;
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
}

bool ATempleGate::CanInteract_Implementation(AMaskCharacter* Character) const
{
	return Character != nullptr && !bOpen;
}

void ATempleGate::Interact_Implementation(AMaskCharacter* /*Character*/)
{
	// Examining a sealed gate is how the player is told what it wants; the gate
	// itself only ever opens to the song.
	UE_LOG(LogMaskGame, Log, TEXT("%s is sealed. It is waiting for a song."), *DisplayName.ToString());
}

FText ATempleGate::GetInteractionPrompt_Implementation() const
{
	return FText::Format(NSLOCTEXT("MaskGame", "GateSealed", "{0} - sealed"), DisplayName);
}
