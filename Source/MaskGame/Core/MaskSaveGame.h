// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"

#include "Core/MaskGameTypes.h"

#include "MaskSaveGame.generated.h"

/**
 * A flat, versioned snapshot of MaskGame::FProgressionState.
 *
 * The rules layer uses std containers that USaveGame cannot serialise, so the
 * save holds reflected equivalents and UMaskGameInstance converts in both
 * directions. Only the permanent half of the progression is written: a save
 * taken mid-cycle restores you to the first dawn with everything you had
 * earned, which is the same guarantee the Hymn of Return makes.
 */
UCLASS()
class MASKGAME_API UMaskSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** Bumped whenever the layout below changes; older saves are migrated on load. */
	static constexpr int32 CurrentVersion = 1;

	UPROPERTY(SaveGame)
	int32 SaveVersion = CurrentVersion;

	UPROPERTY(SaveGame)
	TArray<EMaskType> OwnedMasks;

	UPROPERTY(SaveGame)
	TArray<ESongType> LearnedSongs;

	UPROPERTY(SaveGame)
	TArray<EEchoType> FreedEchoes;

	UPROPERTY(SaveGame)
	TArray<FName> TouchedStatues;

	/** Stray fairies returned, per temple. */
	UPROPERTY(SaveGame)
	TMap<EEchoType, int32> StrayFairies;

	/** Flags whose names begin with "perm."; nothing else is worth keeping. */
	UPROPERTY(SaveGame)
	TMap<FName, int32> PermanentFlags;

	UPROPERTY(SaveGame)
	int32 SwordTier = 1;

	UPROPERTY(SaveGame)
	int32 ShieldTier = 1;

	UPROPERTY(SaveGame)
	int32 QuiverTier = 0;

	UPROPERTY(SaveGame)
	int32 BombBagTier = 0;

	UPROPERTY(SaveGame)
	int32 WalletTier = 1;

	UPROPERTY(SaveGame)
	int32 BottleCount = 0;

	UPROPERTY(SaveGame)
	int32 MagicTier = 0;

	UPROPERTY(SaveGame)
	int32 HeartFragments = 0;

	UPROPERTY(SaveGame)
	int32 MaxHearts = 3;

	UPROPERTY(SaveGame)
	int32 BankedRupees = 0;

	/** Cycles begun, shown on the file select screen. */
	UPROPERTY(SaveGame)
	int32 CycleCount = 1;

	/** Region the player was last standing in, used to place them on load. */
	UPROPERTY(SaveGame)
	FName LastRegionId;
};
