// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.
//
// Console commands, all prefixed "MaskGame." so that typing that prefix into
// the console lists them.
//
// These exist because the game is long by design: the mask that answers a
// question is often two temples and a rewind away, and nobody testing a change
// to the mask rules should have to play there to see it. They are registered
// unconditionally rather than behind a shipping check, since this project has
// no shipping configuration yet; the place to gate them is here, in one file.

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"

#include "Character/MaskCharacter.h"
#include "Core/MaskGameInstance.h"
#include "Core/MaskGameMode.h"
#include "Core/MaskGameTypes.h"
#include "MaskGame.h"
#include "Masks/MaskInventoryComponent.h"
#include "Quests/QuestSubsystem.h"
#include "Time/CycleSubsystem.h"

namespace MaskGameDebug
{
	UMaskGameInstance* GetProgression(const UWorld* World)
	{
		UMaskGameInstance* Instance = World != nullptr ? UMaskGameInstance::Get(World) : nullptr;
		if (Instance == nullptr)
		{
			UE_LOG(LogMaskGame, Error, TEXT("No MaskGame game instance. Is the project's Game Instance class set?"));
		}
		return Instance;
	}

	UCycleSubsystem* GetCycle(const UWorld* World)
	{
		UCycleSubsystem* Cycle = World != nullptr ? World->GetSubsystem<UCycleSubsystem>() : nullptr;
		if (Cycle == nullptr)
		{
			UE_LOG(LogMaskGame, Error, TEXT("No cycle subsystem; are you in a running game world?"));
		}
		return Cycle;
	}

	/** Resolve a mask by its enum entry name, case-insensitively. */
	EMaskType ParseMask(const FString& Text)
	{
		const UEnum* Enum = StaticEnum<EMaskType>();
		for (int32 Index = 1; Index < static_cast<int32>(EMaskType::Count); ++Index)
		{
			const EMaskType Mask = static_cast<EMaskType>(Index);
			if (Enum->GetNameStringByValue(Index).Equals(Text, ESearchCase::IgnoreCase)
				|| Enum->GetDisplayNameTextByValue(Index).ToString().Equals(Text, ESearchCase::IgnoreCase))
			{
				return Mask;
			}
		}
		return EMaskType::None;
	}

	// ---- MaskGame.GiveAllMasks ----
	FAutoConsoleCommandWithWorld GiveAllMasks(
		TEXT("MaskGame.GiveAllMasks"),
		TEXT("Grant every mask in the game, including the wrath mask."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			UMaskGameInstance* Instance = GetProgression(World);
			if (Instance == nullptr)
			{
				return;
			}

			int32 Granted = 0;
			for (int32 Index = 1; Index < static_cast<int32>(EMaskType::Count); ++Index)
			{
				Granted += Instance->GiveMask(static_cast<EMaskType>(Index)) ? 1 : 0;
			}
			UE_LOG(LogMaskGame, Display, TEXT("Granted %d masks; the collection now holds %d."),
				Granted, Instance->GetMaskCount());
		}));

	// ---- MaskGame.GiveMask <name> ----
	FAutoConsoleCommandWithWorldAndArgs GiveMask(
		TEXT("MaskGame.GiveMask"),
		TEXT("Grant one mask by name, e.g. MaskGame.GiveMask Boulderkin"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UMaskGameInstance* Instance = GetProgression(World);
			if (Instance == nullptr)
			{
				return;
			}
			if (Args.Num() < 1)
			{
				UE_LOG(LogMaskGame, Warning, TEXT("Usage: MaskGame.GiveMask <name>, e.g. Tideborn or HareHood."));
				return;
			}

			const EMaskType Mask = ParseMask(Args[0]);
			if (Mask == EMaskType::None)
			{
				UE_LOG(LogMaskGame, Warning, TEXT("No mask called '%s'."), *Args[0]);
				return;
			}

			UE_LOG(LogMaskGame, Display, TEXT("%s %s."),
				Instance->GiveMask(Mask) ? TEXT("Granted") : TEXT("Already had"),
				*UEnum::GetDisplayValueAsText(Mask).ToString());
		}));

	// ---- MaskGame.LearnAllSongs ----
	FAutoConsoleCommandWithWorld LearnAllSongs(
		TEXT("MaskGame.LearnAllSongs"),
		TEXT("Learn every song, so the ocarina answers to all of them."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			UMaskGameInstance* Instance = GetProgression(World);
			if (Instance == nullptr)
			{
				return;
			}

			int32 Learned = 0;
			for (int32 Index = 1; Index <= static_cast<int32>(ESongType::SongOfStorms); ++Index)
			{
				Learned += Instance->LearnSong(static_cast<ESongType>(Index)) ? 1 : 0;
			}
			UE_LOG(LogMaskGame, Display, TEXT("Learned %d songs."), Learned);
		}));

	// ---- MaskGame.SetTime <day> <hour> [minute] ----
	FAutoConsoleCommandWithWorldAndArgs SetTime(
		TEXT("MaskGame.SetTime"),
		TEXT("Move the clock, e.g. MaskGame.SetTime 3 0 puts you in the final hours."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UCycleSubsystem* Cycle = GetCycle(World);
			if (Cycle == nullptr)
			{
				return;
			}
			if (Args.Num() < 2)
			{
				UE_LOG(LogMaskGame, Warning, TEXT("Usage: MaskGame.SetTime <day 1-3> <hour 0-23> [minute]"));
				return;
			}

			const FMaskCycleTime Target(
				FCString::Atoi(*Args[0]),
				FCString::Atoi(*Args[1]),
				Args.Num() > 2 ? FCString::Atoi(*Args[2]) : 0);

			Cycle->SetElapsedMinutes(MaskGame::FCycleClock::ToElapsedMinutes(Target.ToRules()));
			UE_LOG(LogMaskGame, Display, TEXT("It is now %s.%s"),
				*Cycle->GetTime().ToText().ToString(),
				Cycle->IsFinalHours() ? TEXT(" The final hours have begun.") : TEXT(""));
		}));

	// ---- MaskGame.Rewind ----
	FAutoConsoleCommandWithWorld Rewind(
		TEXT("MaskGame.Rewind"),
		TEXT("Play the Hymn of Return: back to the first dawn, keeping what is permanent."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			AMaskGameMode* GameMode = World != nullptr ? Cast<AMaskGameMode>(UGameplayStatics::GetGameMode(World)) : nullptr;
			if (GameMode == nullptr)
			{
				UE_LOG(LogMaskGame, Error, TEXT("No MaskGameMode in this level."));
				return;
			}
			GameMode->RestartCycle(/*bBankProgress=*/true);
		}));

	// ---- MaskGame.Where ----
	FAutoConsoleCommandWithWorld Where(
		TEXT("MaskGame.Where"),
		TEXT("Print the time, the player's form, and who is standing in each region right now."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			const UCycleSubsystem* Cycle = GetCycle(World);
			const UQuestSubsystem* Quests = UQuestSubsystem::Get(World);
			if (Cycle == nullptr || Quests == nullptr)
			{
				return;
			}

			const AMaskCharacter* Player = Cast<AMaskCharacter>(UGameplayStatics::GetPlayerPawn(World, 0));
			UE_LOG(LogMaskGame, Display, TEXT("%s  |  %s  |  form: %s"),
				*Cycle->GetTime().ToText().ToString(),
				Cycle->IsNight() ? TEXT("night") : TEXT("day"),
				Player != nullptr ? *UEnum::GetDisplayValueAsText(Player->GetForm()).ToString() : TEXT("no pawn"));

			const int32 Now = Cycle->GetElapsedMinutes();
			for (const FRegionTableRow& Region : Quests->GetAllRegions())
			{
				const TArray<FName> Occupants = Quests->FindOccupants(Region.RegionId, Now);
				if (Occupants.IsEmpty())
				{
					continue;
				}

				TArray<FString> Names;
				for (const FName Occupant : Occupants)
				{
					Names.Add(Occupant.ToString());
				}
				UE_LOG(LogMaskGame, Display, TEXT("  %-22s %s"),
					*Region.RegionId.ToString(), *FString::Join(Names, TEXT(", ")));
			}
		}));
}
