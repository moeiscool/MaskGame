// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/DeveloperSettings.h"

#include "MaskGameSettings.generated.h"

/**
 * Project settings for the game's content tables, edited under
 * Project Settings > Game > Mask Game and stored in Config/DefaultGame.ini.
 *
 * The tables are soft references so that a cooked build does not pull the whole
 * of Content/Data into memory before the first region streams in.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Mask Game"))
class MASKGAME_API UMaskGameSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UMaskGameSettings();

	/** Convenience accessor; never null. */
	static const UMaskGameSettings& Get();

	UPROPERTY(Config, EditAnywhere, Category = "Data Tables", meta = (RequiredAssetDataTags = "RowStructure=/Script/MaskGame.MaskTableRow"))
	TSoftObjectPtr<UDataTable> MaskTable;

	UPROPERTY(Config, EditAnywhere, Category = "Data Tables", meta = (RequiredAssetDataTags = "RowStructure=/Script/MaskGame.SongTableRow"))
	TSoftObjectPtr<UDataTable> SongTable;

	UPROPERTY(Config, EditAnywhere, Category = "Data Tables", meta = (RequiredAssetDataTags = "RowStructure=/Script/MaskGame.RegionTableRow"))
	TSoftObjectPtr<UDataTable> RegionTable;

	UPROPERTY(Config, EditAnywhere, Category = "Data Tables", meta = (RequiredAssetDataTags = "RowStructure=/Script/MaskGame.DungeonTableRow"))
	TSoftObjectPtr<UDataTable> DungeonTable;

	UPROPERTY(Config, EditAnywhere, Category = "Data Tables", meta = (RequiredAssetDataTags = "RowStructure=/Script/MaskGame.QuestTableRow"))
	TSoftObjectPtr<UDataTable> QuestTable;

	UPROPERTY(Config, EditAnywhere, Category = "Data Tables", meta = (RequiredAssetDataTags = "RowStructure=/Script/MaskGame.ScheduleTableRow"))
	TSoftObjectPtr<UDataTable> ScheduleTable;

	/** Slot the game autosaves into whenever the cycle rewinds. */
	UPROPERTY(Config, EditAnywhere, Category = "Save")
	FString SaveSlotName = TEXT("MaskGameSave");

	/**
	 * Build the greybox world from the region and dungeon tables at startup.
	 *
	 * Leave this on until authored levels exist: it is what makes the project
	 * playable from an empty map.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "World")
	bool bGenerateGreyboxWorld = true;
};
