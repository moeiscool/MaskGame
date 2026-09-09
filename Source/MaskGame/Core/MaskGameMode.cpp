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
#include "World/MaskSkyDirector.h"
#include "World/MaskWorldGenerator.h"
#include "World/SongListenerInterface.h"

AMaskGameMode::AMaskGameMode()
{
	DefaultPawnClass = AMaskCharacter::StaticClass();
	PlayerControllerClass = AMaskPlayerController::StaticClass();
	WorldGeneratorClass = AMaskWorldGenerator::StaticClass();
	SkyDirectorClass = AMaskSkyDirector::StaticClass();
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

	if (SkyDirectorClass != nullptr && SkyDirector == nullptr)
	{
		FActorSpawnParameters SkyParams;
		SkyParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SkyDirector = GetWorld()->SpawnActor<AMaskSkyDirector>(SkyDirectorClass, FTransform::Identity, SkyParams);
	}

	for (AMaskCharacter* Character : GetLocalCharacters())
	{
		BindToPlayer(Character);
	}
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

TArray<AMaskCharacter*> AMaskGameMode::GetLocalCharacters() const
{
	TArray<AMaskCharacter*> Characters;

	const UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return Characters;
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* Controller = It->Get();
		if (AMaskCharacter* Character = Controller != nullptr ? Cast<AMaskCharacter>(Controller->GetPawn()) : nullptr)
		{
			Characters.Add(Character);
		}
	}
	return Characters;
}

APawn* AMaskGameMode::SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& Transform)
{
	// Everyone shares one player start, so fan additional players out along it
	// rather than letting the engine's collision nudging decide where they land.
	const int32 ExistingPlayers = GetLocalCharacters().Num();

	FTransform Spawn = Transform;
	if (ExistingPlayers > 0)
	{
		// Alternate right and left of the start so a four-player game stays centred.
		const int32 Step = (ExistingPlayers + 1) / 2;
		const float Side = (ExistingPlayers % 2 == 1) ? 1.0f : -1.0f;
		const FVector Offset = Transform.GetRotation().GetRightVector() * (Side * Step * LocalPlayerSpawnSpacing);
		Spawn.SetLocation(Transform.GetLocation() + Offset);
	}

	return Super::SpawnDefaultPawnAtTransform_Implementation(NewPlayer, Spawn);
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
	// In split-screen one player going down is not the end of the cycle; the
	// others can still finish what they were doing. The rewind waits until
	// nobody is left standing, which in single player is the same rule.
	for (const AMaskCharacter* Character : GetLocalCharacters())
	{
		if (Character->IsAlive())
		{
			UE_LOG(LogMaskGame, Log, TEXT("A traveller fell, but the cycle goes on."));
			return;
		}
	}

	UE_LOG(LogMaskGame, Log, TEXT("Every traveller has fallen. Returning to the first dawn."));
	RestartCycle(/*bBankProgress=*/true);
}

void AMaskGameMode::HandleSongPlayed(ESongType Song, ESongPerformance Performance, AActor* Performer)
{
	// The full Hymn of Return is the one song the world does not get a say in.
	// In split-screen either player can play it, and it takes everyone back:
	// there is one cycle, and it belongs to all of them.
	if (Song == ESongType::HymnOfReturn && Performance == ESongPerformance::Forward)
	{
		RestartCycle(/*bBankProgress=*/true);
		return;
	}

	// The song is heard from wherever it was actually played, which in
	// split-screen is not necessarily where player one is standing.
	AMaskCharacter* PerformingCharacter = Cast<AMaskCharacter>(Performer);
	const int32 Answered = BroadcastSongToListeners(Song, Performance, PerformingCharacter);
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

	const TArray<AMaskCharacter*> Characters = GetLocalCharacters();
	for (int32 Index = 0; Index < Characters.Num(); ++Index)
	{
		AMaskCharacter* Character = Characters[Index];
		Character->ClearLockOn();
		Character->RestoreFully();

		if (const AActor* Start = FindPlayerStart(Character->GetController()))
		{
			// Same fan-out as the initial spawn, so a rewind does not stack
			// split-screen players on top of one another.
			const FVector Offset = Index > 0
				? Start->GetActorRightVector() * (((Index % 2 == 1) ? 1.0f : -1.0f) * ((Index + 1) / 2) * LocalPlayerSpawnSpacing)
				: FVector::ZeroVector;
			Character->SetActorLocationAndRotation(Start->GetActorLocation() + Offset, Start->GetActorRotation());
		}
		BindToPlayer(Character);
	}

	OnCycleRestarted.Broadcast();
	bRestartInProgress = false;
}
