// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

#include "Quests/QuestSubsystem.h"

#include "Engine/DataTable.h"
#include "Kismet/GameplayStatics.h"

#include "Core/MaskGameInstance.h"
#include "Core/MaskGameSettings.h"
#include "MaskGame.h"

namespace
{
	std::string ToStdString(FName Name)
	{
		return Name.IsNone() ? std::string() : std::string(TCHAR_TO_UTF8(*Name.ToString()));
	}

	/** Loads a soft-referenced table, logging rather than crashing when it is missing. */
	UDataTable* LoadTableChecked(const TSoftObjectPtr<UDataTable>& SoftTable, const TCHAR* Label)
	{
		if (SoftTable.IsNull())
		{
			UE_LOG(LogMaskGame, Warning, TEXT("No %s table configured; that content will be unavailable."), Label);
			return nullptr;
		}

		UDataTable* Table = SoftTable.LoadSynchronous();
		if (Table == nullptr)
		{
			UE_LOG(LogMaskGame, Error, TEXT("Failed to load the %s table at '%s'."), Label, *SoftTable.ToString());
		}
		return Table;
	}

	/** Reads every row of a table into an array, in table order. */
	template <typename RowType>
	TArray<RowType> ReadRows(const UDataTable* Table, const TCHAR* Context)
	{
		TArray<RowType> Out;
		if (Table == nullptr)
		{
			return Out;
		}

		TArray<RowType*> Rows;
		Table->GetAllRows<RowType>(Context, Rows);
		Out.Reserve(Rows.Num());
		for (const RowType* Row : Rows)
		{
			if (Row != nullptr)
			{
				Out.Add(*Row);
			}
		}
		return Out;
	}

	/** Finds a row whose named id field matches, since rows are keyed by content id rather than row name. */
	template <typename RowType, typename IdGetter>
	bool FindRowById(const UDataTable* Table, FName Id, IdGetter Getter, RowType& OutRow, const TCHAR* Context)
	{
		if (Table == nullptr || Id.IsNone())
		{
			return false;
		}

		TArray<RowType*> Rows;
		Table->GetAllRows<RowType>(Context, Rows);
		for (const RowType* Row : Rows)
		{
			if (Row != nullptr && Getter(*Row) == Id)
			{
				OutRow = *Row;
				return true;
			}
		}
		return false;
	}
}

void UQuestSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadTables();
	BuildScheduleTable();
}

void UQuestSubsystem::Deinitialize()
{
	Schedules.Clear();
	KnownQuests.Reset();
	Super::Deinitialize();
}

UQuestSubsystem* UQuestSubsystem::Get(const UObject* WorldContextObject)
{
	const UGameInstance* Instance = WorldContextObject != nullptr
		? UGameplayStatics::GetGameInstance(WorldContextObject)
		: nullptr;
	return Instance != nullptr ? Instance->GetSubsystem<UQuestSubsystem>() : nullptr;
}

UMaskGameInstance* UQuestSubsystem::GetProgression() const
{
	return Cast<UMaskGameInstance>(GetGameInstance());
}

const MaskGame::IFlagSource* UQuestSubsystem::GetFlagSource() const
{
	const UMaskGameInstance* Instance = GetProgression();
	return Instance != nullptr ? &Instance->GetFlagSource() : nullptr;
}

void UQuestSubsystem::LoadTables()
{
	const UMaskGameSettings& Settings = UMaskGameSettings::Get();

	MaskTable = LoadTableChecked(Settings.MaskTable, TEXT("mask"));
	SongTable = LoadTableChecked(Settings.SongTable, TEXT("song"));
	RegionTable = LoadTableChecked(Settings.RegionTable, TEXT("region"));
	DungeonTable = LoadTableChecked(Settings.DungeonTable, TEXT("dungeon"));
	QuestTable = LoadTableChecked(Settings.QuestTable, TEXT("quest"));
	ScheduleDataTable = LoadTableChecked(Settings.ScheduleTable, TEXT("schedule"));
}

void UQuestSubsystem::BuildScheduleTable()
{
	Schedules.Clear();

	for (const FScheduleTableRow& Row : ReadRows<FScheduleTableRow>(ScheduleDataTable, TEXT("BuildScheduleTable")))
	{
		if (Row.ActorId.IsNone() || Row.RegionId.IsNone())
		{
			UE_LOG(LogMaskGame, Warning, TEXT("Skipping a schedule row with no actor or region."));
			continue;
		}

		Schedules.Add(
			ToStdString(Row.ActorId),
			ToStdString(Row.RegionId),
			std::string(TCHAR_TO_UTF8(*Row.Activity.ToString())),
			Row.Start.ToRules(),
			Row.End.ToRules(),
			ToStdString(Row.RequiredFlag),
			Row.Priority);
	}

	UE_LOG(LogMaskGame, Log, TEXT("Loaded %d schedule entries."), static_cast<int32>(Schedules.Num()));
}

bool UQuestSubsystem::GetRegion(FName RegionId, FRegionTableRow& OutRow) const
{
	return FindRowById<FRegionTableRow>(RegionTable, RegionId,
		[](const FRegionTableRow& Row) { return Row.RegionId; }, OutRow, TEXT("GetRegion"));
}

bool UQuestSubsystem::GetDungeon(FName DungeonId, FDungeonTableRow& OutRow) const
{
	return FindRowById<FDungeonTableRow>(DungeonTable, DungeonId,
		[](const FDungeonTableRow& Row) { return Row.DungeonId; }, OutRow, TEXT("GetDungeon"));
}

bool UQuestSubsystem::GetQuest(FName QuestId, FQuestTableRow& OutRow) const
{
	return FindRowById<FQuestTableRow>(QuestTable, QuestId,
		[](const FQuestTableRow& Row) { return Row.QuestId; }, OutRow, TEXT("GetQuest"));
}

bool UQuestSubsystem::GetMaskInfo(EMaskType Mask, FMaskTableRow& OutRow) const
{
	if (MaskTable == nullptr || Mask == EMaskType::None)
	{
		return false;
	}

	TArray<FMaskTableRow*> Rows;
	MaskTable->GetAllRows<FMaskTableRow>(TEXT("GetMaskInfo"), Rows);
	for (const FMaskTableRow* Row : Rows)
	{
		if (Row != nullptr && Row->Mask == Mask)
		{
			OutRow = *Row;
			return true;
		}
	}
	return false;
}

TArray<FRegionTableRow> UQuestSubsystem::GetAllRegions() const
{
	return ReadRows<FRegionTableRow>(RegionTable, TEXT("GetAllRegions"));
}

TArray<FDungeonTableRow> UQuestSubsystem::GetAllDungeons() const
{
	return ReadRows<FDungeonTableRow>(DungeonTable, TEXT("GetAllDungeons"));
}

TArray<FRegionTableRow> UQuestSubsystem::GetRegionsForChapter(int32 Chapter) const
{
	TArray<FRegionTableRow> Out;
	for (const FRegionTableRow& Row : GetAllRegions())
	{
		if (Row.Chapter == Chapter)
		{
			Out.Add(Row);
		}
	}
	return Out;
}

bool UQuestSubsystem::FindActorNow(FName ActorId, int32 ElapsedMinutes, FName& OutRegionId, FText& OutActivity) const
{
	const MaskGame::FScheduleEntry* Entry =
		Schedules.FindEntry(ToStdString(ActorId), ElapsedMinutes, GetFlagSource());
	if (Entry == nullptr)
	{
		return false;
	}

	OutRegionId = FName(UTF8_TO_TCHAR(Entry->RegionId.c_str()));
	OutActivity = FText::FromString(UTF8_TO_TCHAR(Entry->Activity.c_str()));
	return true;
}

TArray<FName> UQuestSubsystem::FindOccupants(FName RegionId, int32 ElapsedMinutes) const
{
	TArray<FName> Out;
	for (const MaskGame::FScheduleEntry* Entry :
		Schedules.FindOccupants(ToStdString(RegionId), ElapsedMinutes, GetFlagSource()))
	{
		Out.Add(FName(UTF8_TO_TCHAR(Entry->ActorId.c_str())));
	}
	return Out;
}

bool UQuestSubsystem::DiscoverQuest(FName QuestId)
{
	FQuestTableRow Row;
	if (!GetQuest(QuestId, Row) || KnownQuests.Contains(QuestId))
	{
		return false;
	}

	KnownQuests.Add(QuestId);
	UE_LOG(LogMaskGame, Log, TEXT("Noted down: %s."), *Row.DisplayName.ToString());

	OnQuestDiscovered.Broadcast(QuestId);
	OnNotebookChanged.Broadcast();
	return true;
}

bool UQuestSubsystem::IsQuestCompleted(FName QuestId) const
{
	FQuestTableRow Row;
	const UMaskGameInstance* Instance = GetProgression();
	if (Instance == nullptr || !GetQuest(QuestId, Row) || Row.CompletionFlag.IsNone())
	{
		return false;
	}
	return Instance->HasFlag(Row.CompletionFlag);
}

bool UQuestSubsystem::IsQuestAvailable(FName QuestId, int32 ElapsedMinutes) const
{
	FQuestTableRow Row;
	if (!GetQuest(QuestId, Row) || IsQuestCompleted(QuestId))
	{
		return false;
	}

	const int32 Start = MaskGame::FCycleClock::ToElapsedMinutes(Row.WindowStart.ToRules());
	const int32 End = MaskGame::FCycleClock::ToElapsedMinutes(Row.WindowEnd.ToRules());
	return ElapsedMinutes >= Start && ElapsedMinutes <= End;
}

bool UQuestSubsystem::CompleteQuest(FName QuestId, int32 ElapsedMinutes)
{
	FQuestTableRow Row;
	if (!GetQuest(QuestId, Row))
	{
		UE_LOG(LogMaskGame, Warning, TEXT("No quest '%s' in the table."), *QuestId.ToString());
		return false;
	}

	if (!IsQuestAvailable(QuestId, ElapsedMinutes))
	{
		// Either the chance has passed for this cycle or the work is already done.
		return false;
	}

	UMaskGameInstance* Instance = GetProgression();
	if (Instance == nullptr)
	{
		return false;
	}

	if (!Row.CompletionFlag.IsNone())
	{
		Instance->SetFlag(Row.CompletionFlag, 1);
	}
	if (Row.RewardMask != EMaskType::None)
	{
		Instance->GiveMask(Row.RewardMask);
	}
	if (Row.bRewardsHeartFragment)
	{
		Instance->GiveHeartFragment();
	}

	// Finishing something you never wrote down still fills in the page.
	KnownQuests.Add(QuestId);

	UE_LOG(LogMaskGame, Log, TEXT("Completed: %s."), *Row.DisplayName.ToString());
	OnQuestCompleted.Broadcast(QuestId);
	OnNotebookChanged.Broadcast();
	return true;
}

TArray<FNotebookEntry> UQuestSubsystem::BuildNotebook(int32 ElapsedMinutes) const
{
	TArray<FQuestTableRow> Rows = ReadRows<FQuestTableRow>(QuestTable, TEXT("BuildNotebook"));
	Rows.Sort([](const FQuestTableRow& A, const FQuestTableRow& B)
	{
		return A.Chapter != B.Chapter ? A.Chapter < B.Chapter : A.QuestId.LexicalLess(B.QuestId);
	});

	TArray<FNotebookEntry> Out;
	Out.Reserve(KnownQuests.Num());

	for (const FQuestTableRow& Row : Rows)
	{
		if (!KnownQuests.Contains(Row.QuestId))
		{
			continue;
		}

		FNotebookEntry Entry;
		Entry.QuestId = Row.QuestId;
		Entry.DisplayName = Row.DisplayName;
		Entry.ActorId = Row.ActorId;
		Entry.Summary = Row.Summary;
		Entry.WindowStart = Row.WindowStart;
		Entry.WindowEnd = Row.WindowEnd;
		Entry.bCompleted = IsQuestCompleted(Row.QuestId);
		Entry.bAvailableNow = IsQuestAvailable(Row.QuestId, ElapsedMinutes);
		Out.Add(MoveTemp(Entry));
	}

	return Out;
}
