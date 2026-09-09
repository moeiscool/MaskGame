// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

#include "Core/MaskGameMode.h"

#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

#include "Character/MaskCharacter.h"
#include "Core/MaskGameInstance.h"
#include "Core/MaskGameSettings.h"
#include "Core/MaskPlayerController.h"
#include "MaskGame.h"
#include "Songs/OcarinaComponent.h"
#include "Time/CycleSubsystem.h"
#include "World/MaskWorldGenerator.h"
#include "World/SongListenerInterface.h"

AMaskGameMode::AMaskGameMode()
{
	DefaultPawnClass = AMaskCharacter::StaticClass();
	PlayerControllerClass = AMaskPlayerController::StaticClass();
	WorldGeneratorClass = AMaskWorldGenerator::StaticClass();
}

void AMaskGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (UCycleSubsystem* Cycle = GetCycle())
	{
		Cycle->OnMoonFell.AddDynamic(this, &AMaskGameMode::HandleMoonFell);
	}

	// While there are no authored levels, the world is built from the data tables
	// at startup so that an empty map is still a playable game.
	if (UMaskGameSettings::Get().bGenerateGreyboxWorld && WorldGeneratorClass != nullptr && WorldGenerator == nullptr)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		WorldGenerator = GetWorld()->SpawnActor<AMaskWorldGenerator>(WorldGeneratorClass, FTransform::Identity, Params);
	}

	BindToPlayer(GetPlayerCharacter());
}

void AMaskGameMode::RestartPlayer(AController* NewPlayer)
{
	Super::RestartPlayer(NewPlayer);

	// A pawn respawned mid-session is a different object, so its delegates have
	// to be hooked up again.
	if (NewPlayer != nullptr)
	{
		BindToPlayer(Cast<AMaskCharacter>(NewPlayer->GetPawn()));
	}
}

UCycleSubsystem* AMaskGameMode::GetCycle() const
{
	const UWorld* World = GetWorld();
	return World != nullptr ? World->GetSubsystem<UCycleSubsystem>() : nullptr;
}

AMaskCharacter* AMaskGameMode::GetPlayerCharacter() const
{
	return Cast<AMaskCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
}

void AMaskGameMode::BindToPlayer(AMaskCharacter* Character)
{
	if (Character == nullptr)
	{
		return;
	}

	if (UOcarinaComponent* Ocarina = Character->GetOcarina())
	{
		// AddUniqueDynamic keeps a second call from double-firing every song.
		Ocarina->OnSongPlayed.AddUniqueDynamic(this, &AMaskGameMode::HandleSongPlayed);
	}
	Character->OnDied.AddUniqueDynamic(this, &AMaskGameMode::HandlePlayerDied);
}

void AMaskGameMode::HandleMoonFell()
{
	UE_LOG(LogMaskGame, Warning, TEXT("The moon has landed. Returning to the first dawn."));
	RestartCycle(/*bBankProgress=*/true);
}

void AMaskGameMode::HandlePlayerDied()
{
	UE_LOG(LogMaskGame, Log, TEXT("The traveller fell. Returning to the first dawn."));
	RestartCycle(/*bBankProgress=*/true);
}

void AMaskGameMode::HandleSongPlayed(ESongType Song, ESongPerformance Performance)
{
	AMaskCharacter* Performer = GetPlayerCharacter();

	// The full Hymn of Return is the one song the world does not get a say in.
	if (Song == ESongType::HymnOfReturn && Performance == ESongPerformance::Forward)
	{
		RestartCycle(/*bBankProgress=*/true);
		return;
	}

	const int32 Answered = BroadcastSongToListeners(Song, Performance, Performer);
	if (Answered == 0)
	{
		UE_LOG(LogMaskGame, Verbose, TEXT("%s went unanswered here."),
			*UEnum::GetDisplayValueAsText(Song).ToString());
	}
}

int32 AMaskGameMode::BroadcastSongToListeners(ESongType Song, ESongPerformance Performance, AMaskCharacter* Performer)
{
	UWorld* World = GetWorld();
	if (World == nullptr || Performer == nullptr)
	{
		return 0;
	}

	const FVector Origin = Performer->GetActorLocation();
	int32 Answered = 0;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor == nullptr || !Actor->Implements<USongListenerInterface>())
		{
			continue;
		}

		const float Radius = ISongListenerInterface::Execute_GetHearingRadius(Actor);
		if (FVector::DistSquared(Origin, Actor->GetActorLocation()) > FMath::Square(Radius))
		{
			continue;
		}

		if (ISongListenerInterface::Execute_OnSongHeard(Actor, Song, Performance, Performer))
		{
			++Answered;
		}
	}

	return Answered;
}

void AMaskGameMode::RestartCycle(bool bBankProgress)
{
	if (bRestartInProgress)
	{
		return;
	}
	bRestartInProgress = true;

	if (UMaskGameInstance* Instance = UMaskGameInstance::Get(this); Instance != nullptr && bBankProgress)
	{
		Instance->RewindProgression();
	}

	if (UCycleSubsystem* Cycle = GetCycle())
	{
		Cycle->SetFlowRate(ETimeFlowRate::Normal);
		Cycle->RewindCycle();
	}

	// The world is rebuilt rather than patched: every chest closed, every door
	// locked, every enemy back where it started. That is the whole promise of the
	// rewind, and doing it wholesale is what keeps it honest.
	if (WorldGenerator != nullptr)
	{
		WorldGenerator->RebuildWorld();
	}

	if (AMaskCharacter* Character = GetPlayerCharacter())
	{
		Character->RestoreFully();
		if (const AActor* Start = FindPlayerStart(Character->GetController()))
		{
			Character->SetActorLocationAndRotation(Start->GetActorLocation(), Start->GetActorRotation());
		}
		BindToPlayer(Character);
	}

	OnCycleRestarted.Broadcast();
	bRestartInProgress = false;
}
