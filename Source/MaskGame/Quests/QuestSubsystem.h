// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "Core/MaskGameTypes.h"
#include "ScheduleTable.h"

#include "QuestSubsystem.generated.h"

class UDataTable;
class UMaskGameInstance;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQuestDiscovered, FName, QuestId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQuestCompleted, FName, QuestId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNotebookChanged);

/** One page of the notebook: what the player has learned about somebody's three days. */
USTRUCT(BlueprintType)
struct FNotebookEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Notebook")
	FName QuestId;

	UPROPERTY(BlueprintReadOnly, Category = "Notebook")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Notebook")
	FName ActorId;

	UPROPERTY(BlueprintReadOnly, Category = "Notebook")
	FText Summary;

	UPROPERTY(BlueprintReadOnly, Category = "Notebook")
	FMaskCycleTime WindowStart;

	UPROPERTY(BlueprintReadOnly, Category = "Notebook")
	FMaskCycleTime WindowEnd;

	/** True once the quest has been finished in some cycle. */
	UPROPERTY(BlueprintReadOnly, Category = "Notebook")
	bool bCompleted = false;

	/** True while the cycle is inside the quest's window right now. */
	UPROPERTY(BlueprintReadOnly, Category = "Notebook")
	bool bAvailableNow = false;
};

/**
 * The content of the world, and the notebook the player keeps about it.
 *
 * Loads the six data tables on startup, turns the schedule rows into a
 * MaskGame::FScheduleTable, and answers the two questions the rest of the game
 * asks: where is this person right now, and what can still be done today.
 *
 * Lives on the game instance because the notebook, like a mask, is something a
 * rewind does not take away.
 */
UCLASS()
class MASKGAME_API UQuestSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	static UQuestSubsystem* Get(const UObject* WorldContextObject);

	// ---- Content lookup. ----

	UFUNCTION(BlueprintPure, Category = "Quests")
	bool GetRegion(FName RegionId, FRegionTableRow& OutRow) const;

	UFUNCTION(BlueprintPure, Category = "Quests")
	bool GetDungeon(FName DungeonId, FDungeonTableRow& OutRow) const;

	UFUNCTION(BlueprintPure, Category = "Quests")
	bool GetQuest(FName QuestId, FQuestTableRow& OutRow) const;

	UFUNCTION(BlueprintPure, Category = "Quests")
	bool GetMaskInfo(EMaskType Mask, FMaskTableRow& OutRow) const;

	UFUNCTION(BlueprintPure, Category = "Quests")
	TArray<FRegionTableRow> GetRegionsForChapter(int32 Chapter) const;

	UFUNCTION(BlueprintPure, Category = "Quests")
	TArray<FRegionTableRow> GetAllRegions() const;

	UFUNCTION(BlueprintPure, Category = "Quests")
	TArray<FDungeonTableRow> GetAllDungeons() const;

	// ---- Schedules. ----

	/** Where an NPC is at this point in the cycle. Returns false when off-stage. */
	UFUNCTION(BlueprintPure, Category = "Quests|Schedules")
	bool FindActorNow(FName ActorId, int32 ElapsedMinutes, FName& OutRegionId, FText& OutActivity) const;

	/** Everyone standing in a region at this point in the cycle. */
	UFUNCTION(BlueprintPure, Category = "Quests|Schedules")
	TArray<FName> FindOccupants(FName RegionId, int32 ElapsedMinutes) const;

	// ---- The notebook. ----

	/** Note somebody's business down. Returns false when it was already known. */
	UFUNCTION(BlueprintCallable, Category = "Quests|Notebook")
	bool DiscoverQuest(FName QuestId);

	UFUNCTION(BlueprintPure, Category = "Quests|Notebook")
	bool IsQuestKnown(FName QuestId) const { return KnownQuests.Contains(QuestId); }

	UFUNCTION(BlueprintPure, Category = "Quests|Notebook")
	bool IsQuestCompleted(FName QuestId) const;

	/**
	 * Finish a quest: set its flag and hand over its reward.
	 * Returns false when the quest is unknown, already done, or out of its window.
	 */
	UFUNCTION(BlueprintCallable, Category = "Quests|Notebook")
	bool CompleteQuest(FName QuestId, int32 ElapsedMinutes);

	/** True while the cycle sits inside the quest's window. */
	UFUNCTION(BlueprintPure, Category = "Quests|Notebook")
	bool IsQuestAvailable(FName QuestId, int32 ElapsedMinutes) const;

	/** Every page of the notebook, newest chapter last. */
	UFUNCTION(BlueprintPure, Category = "Quests|Notebook")
	TArray<FNotebookEntry> BuildNotebook(int32 ElapsedMinutes) const;

	UPROPERTY(BlueprintAssignable, Category = "Quests")
	FOnQuestDiscovered OnQuestDiscovered;

	UPROPERTY(BlueprintAssignable, Category = "Quests")
	FOnQuestCompleted OnQuestCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Quests")
	FOnNotebookChanged OnNotebookChanged;

private:
	void LoadTables();
	void BuildScheduleTable();

	UMaskGameInstance* GetProgression() const;

	/** Resolves gated schedule entries against the player's flags. */
	const MaskGame::IFlagSource* GetFlagSource() const;

	UPROPERTY()
	TObjectPtr<UDataTable> MaskTable;

	UPROPERTY()
	TObjectPtr<UDataTable> SongTable;

	UPROPERTY()
	TObjectPtr<UDataTable> RegionTable;

	UPROPERTY()
	TObjectPtr<UDataTable> DungeonTable;

	UPROPERTY()
	TObjectPtr<UDataTable> QuestTable;

	UPROPERTY()
	TObjectPtr<UDataTable> ScheduleDataTable;

	/** Quests the player has written down. Survives a rewind, like the notebook does. */
	UPROPERTY()
	TSet<FName> KnownQuests;

	MaskGame::FScheduleTable Schedules;
};
