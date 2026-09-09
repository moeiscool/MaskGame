// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

#include "AI/MaskEnemy.h"

#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"

#include "Character/MaskCharacter.h"
#include "Core/MaskGameInstance.h"
#include "MaskGame.h"
#include "Masks/MaskInventoryComponent.h"

AMaskEnemy::AMaskEnemy()
{
	PrimaryActorTick.bCanEverTick = false;

	GetCapsuleComponent()->InitCapsuleSize(40.0f, 90.0f);

	ArenaVolume = CreateDefaultSubobject<USphereComponent>(TEXT("ArenaVolume"));
	ArenaVolume->SetupAttachment(RootComponent);
	ArenaVolume->SetSphereRadius(ArenaRadius);
	ArenaVolume->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	ArenaVolume->SetGenerateOverlapEvents(false);

	DisplayName = NSLOCTEXT("MaskGame", "EnemyDefaultName", "Wanderer");
}

void AMaskEnemy::BeginPlay()
{
	Super::BeginPlay();

	Health = MaxHealth;

	if (IsBoss())
	{
		ArenaVolume->SetSphereRadius(ArenaRadius);
		ArenaVolume->SetGenerateOverlapEvents(true);
		ArenaVolume->OnComponentBeginOverlap.AddDynamic(this, &AMaskEnemy::HandleArenaOverlapBegin);
		ArenaVolume->OnComponentEndOverlap.AddDynamic(this, &AMaskEnemy::HandleArenaOverlapEnd);
	}
}

void AMaskEnemy::MakeBoss(EEchoType InEcho, const FText& InDisplayName)
{
	Echo = InEcho;
	DisplayName = InDisplayName;

	// A guardian is worth the walk through its temple; field enemies fall in a
	// hit or two, this one should not.
	MaxHealth = 20.0f;
	Health = MaxHealth;

	if (ArenaVolume != nullptr)
	{
		ArenaVolume->SetGenerateOverlapEvents(true);
	}
}

float AMaskEnemy::TakeDamage(float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float Base = Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
	if (Base <= 0.0f || !IsAlive())
	{
		return 0.0f;
	}

	Health = FMath::Max(0.0f, Health - Base);
	if (!IsAlive())
	{
		Die();
	}
	return Base;
}

void AMaskEnemy::Die()
{
	UE_LOG(LogMaskGame, Log, TEXT("%s defeated."), *DisplayName.ToString());

	if (IsBoss())
	{
		if (UMaskGameInstance* Instance = UMaskGameInstance::Get(this))
		{
			Instance->FreeEcho(Echo);
		}

		// The arena dies with its owner: nobody should be able to keep the wrath
		// mask on by standing where a boss used to be.
		if (ArenaVolume != nullptr)
		{
			ArenaVolume->SetGenerateOverlapEvents(false);
		}
		const APlayerController* Controller = GetWorld()->GetFirstPlayerController();
		AMaskCharacter* Player = Controller != nullptr ? Cast<AMaskCharacter>(Controller->GetPawn()) : nullptr;
		if (Player != nullptr)
		{
			if (UMaskInventoryComponent* PlayerMasks = Player->GetMasks())
			{
				PlayerMasks->SetInBossArena(false);
			}
		}
	}

	OnDefeated.Broadcast(this);
	Destroy();
}

void AMaskEnemy::HandleArenaOverlapBegin(UPrimitiveComponent* /*OverlappedComponent*/, AActor* OtherActor,
	UPrimitiveComponent* /*OtherComponent*/, int32 /*OtherBodyIndex*/, bool /*bFromSweep*/, const FHitResult& /*Sweep*/)
{
	if (AMaskCharacter* Character = Cast<AMaskCharacter>(OtherActor))
	{
		if (UMaskInventoryComponent* Masks = Character->GetMasks())
		{
			Masks->SetInBossArena(true);
		}
	}
}

void AMaskEnemy::HandleArenaOverlapEnd(UPrimitiveComponent* /*OverlappedComponent*/, AActor* OtherActor,
	UPrimitiveComponent* /*OtherComponent*/, int32 /*OtherBodyIndex*/)
{
	if (AMaskCharacter* Character = Cast<AMaskCharacter>(OtherActor))
	{
		if (UMaskInventoryComponent* Masks = Character->GetMasks())
		{
			Masks->SetInBossArena(false);
		}
	}
}
