// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.
//
// The bridge between the engine-free rules layer and UObject land.
//
// Every enum here mirrors one in Source/MaskGame/Rules and is checked against it
// with static_asserts, so reordering a rules enum without updating the reflected
// copy is a compile error rather than a silent mis-mapping in a data table.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"

#include "CycleClock.h"
#include "MaskRules.h"
#include "ProgressionState.h"
#include "SongMatcher.h"

#include "MaskGameTypes.generated.h"

/** The bodies the player can wear. Mirrors MaskGame::EForm. */
UENUM(BlueprintType)
enum class EMaskForm : uint8
{
	Traveller  UMETA(DisplayName = "Traveller"),
	Sapling    UMETA(DisplayName = "Sapling"),
	Boulderkin UMETA(DisplayName = "Boulderkin"),
	Tideborn   UMETA(DisplayName = "Tideborn"),
	Wrath      UMETA(DisplayName = "Wrath"),
	Count      UMETA(Hidden),
};

/** Every mask in the game. Mirrors MaskGame::EMaskId. */
UENUM(BlueprintType)
enum class EMaskType : uint8
{
	None = 0        UMETA(Hidden),

	Sapling         UMETA(DisplayName = "Sapling Mask"),
	Boulderkin      UMETA(DisplayName = "Boulderkin Mask"),
	Tideborn        UMETA(DisplayName = "Tideborn Mask"),
	Wrath           UMETA(DisplayName = "Wrath Mask"),

	CourierCap      UMETA(DisplayName = "Courier's Cap"),
	SleeplessMask   UMETA(DisplayName = "Sleepless Mask"),
	BlastMask       UMETA(DisplayName = "Blast Mask"),
	UnseenMask      UMETA(DisplayName = "Unseen Mask"),
	GraceMask       UMETA(DisplayName = "Mask of Grace"),
	FoxwardenMask   UMETA(DisplayName = "Foxwarden Mask"),
	MarchMask       UMETA(DisplayName = "March Mask"),
	HareHood        UMETA(DisplayName = "Hare Hood"),
	ChorusMask      UMETA(DisplayName = "Chorus Mask"),
	KeenNoseMask    UMETA(DisplayName = "Keen Nose Mask"),
	RanchMask       UMETA(DisplayName = "Ranch Mask"),
	RingleaderMask  UMETA(DisplayName = "Ringleader's Mask"),
	LostSonMask     UMETA(DisplayName = "Lost Son's Mask"),
	BetrothalMask   UMETA(DisplayName = "Betrothal Mask"),
	TruthMask       UMETA(DisplayName = "Mask of Truth"),
	DancersMask     UMETA(DisplayName = "Dancer's Mask"),
	WrappedMask     UMETA(DisplayName = "Wrapped Mask"),
	SpectreMask     UMETA(DisplayName = "Spectre Mask"),
	CaptainsHelm    UMETA(DisplayName = "Captain's Helm"),
	TitanMask       UMETA(DisplayName = "Titan Mask"),

	Count           UMETA(Hidden),
};

/** The four guardian echoes, one per temple. Mirrors MaskGame::EEchoId. */
UENUM(BlueprintType)
enum class EEchoType : uint8
{
	None = 0    UMETA(Hidden),
	Marshwood   UMETA(DisplayName = "Marshwood Echo"),
	Frostcrown  UMETA(DisplayName = "Frostcrown Echo"),
	Tidebreak   UMETA(DisplayName = "Tidebreak Echo"),
	SunkenSpire UMETA(DisplayName = "Sunken Spire Echo"),
	Count       UMETA(Hidden),
};

/** Every learnable song. Mirrors MaskGame::ESongId. */
UENUM(BlueprintType)
enum class ESongType : uint8
{
	None = 0          UMETA(Hidden),
	HymnOfReturn      UMETA(DisplayName = "Hymn of Return"),
	HymnOfMending     UMETA(DisplayName = "Hymn of Mending"),
	MaresCall         UMETA(DisplayName = "Mare's Call"),
	SonataOfRousing   UMETA(DisplayName = "Sonata of Rousing"),
	StoneheartLullaby UMETA(DisplayName = "Stoneheart Lullaby"),
	TidecallAria      UMETA(DisplayName = "Tidecall Aria"),
	ElegyOfHollows    UMETA(DisplayName = "Elegy of Hollows"),
	OathOfConcord     UMETA(DisplayName = "Oath of Concord"),
	WindfeatherSong   UMETA(DisplayName = "Windfeather Song"),
	SongOfStorms      UMETA(DisplayName = "Song of Storms"),
};

/** The five ocarina notes. Mirrors MaskGame::ENote. */
UENUM(BlueprintType)
enum class EOcarinaNote : uint8
{
	Left  UMETA(DisplayName = "Left"),
	Right UMETA(DisplayName = "Right"),
	Up    UMETA(DisplayName = "Up"),
	Down  UMETA(DisplayName = "Down"),
	A     UMETA(DisplayName = "A"),
};

/** How a recognised song was performed. Mirrors MaskGame::EPerformance. */
UENUM(BlueprintType)
enum class ESongPerformance : uint8
{
	Forward  UMETA(DisplayName = "Forward"),
	Reversed UMETA(DisplayName = "Reversed"),
	Doubled  UMETA(DisplayName = "Doubled"),
};

/** Rate at which the clock runs. Mirrors MaskGame::ETimeFlow. */
UENUM(BlueprintType)
enum class ETimeFlowRate : uint8
{
	Normal UMETA(DisplayName = "Normal"),
	Slowed UMETA(DisplayName = "Slowed"),
	Paused UMETA(DisplayName = "Paused"),
};

/** Day or night. Mirrors MaskGame::EDayPhase. */
UENUM(BlueprintType)
enum class EDayPhaseType : uint8
{
	Day   UMETA(DisplayName = "Day"),
	Night UMETA(DisplayName = "Night"),
};

// The reflected enums above are cast straight to and from their rules-layer
// counterparts, so their underlying values have to agree. A reordering that
// breaks the mapping fails here instead of quietly handing the player the wrong
// mask.
static_assert(static_cast<uint8>(EMaskForm::Count) == static_cast<uint8>(MaskGame::EForm::Count),
	"EMaskForm and MaskGame::EForm have drifted apart");
static_assert(static_cast<uint8>(EMaskType::Count) == static_cast<uint8>(MaskGame::EMaskId::Count),
	"EMaskType and MaskGame::EMaskId have drifted apart");
static_assert(static_cast<uint8>(EMaskType::Wrath) == static_cast<uint8>(MaskGame::EMaskId::Wrath),
	"the transformation masks must occupy the same slots in both enums");
static_assert(static_cast<uint8>(EMaskType::TitanMask) == static_cast<uint8>(MaskGame::EMaskId::TitanMask),
	"the last mask must occupy the same slot in both enums");
static_assert(static_cast<uint8>(EEchoType::Count) == static_cast<uint8>(MaskGame::EEchoId::Count),
	"EEchoType and MaskGame::EEchoId have drifted apart");
static_assert(static_cast<uint8>(ESongType::SongOfStorms) == static_cast<uint8>(MaskGame::ESongId::SongOfStorms),
	"ESongType and MaskGame::ESongId have drifted apart");
static_assert(static_cast<uint8>(EOcarinaNote::A) == static_cast<uint8>(MaskGame::ENote::A),
	"EOcarinaNote and MaskGame::ENote have drifted apart");
static_assert(static_cast<uint8>(ESongPerformance::Doubled) == static_cast<uint8>(MaskGame::EPerformance::Doubled),
	"ESongPerformance and MaskGame::EPerformance have drifted apart");
static_assert(static_cast<uint8>(ETimeFlowRate::Paused) == static_cast<uint8>(MaskGame::ETimeFlow::Paused),
	"ETimeFlowRate and MaskGame::ETimeFlow have drifted apart");
static_assert(static_cast<uint8>(EDayPhaseType::Night) == static_cast<uint8>(MaskGame::EDayPhase::Night),
	"EDayPhaseType and MaskGame::EDayPhase have drifted apart");

/** A point in the three-day cycle, exposed to Blueprints and the HUD. */
USTRUCT(BlueprintType)
struct FMaskCycleTime
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Cycle", meta = (ClampMin = "1", ClampMax = "3"))
	int32 Day = 1;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Cycle", meta = (ClampMin = "0", ClampMax = "23"))
	int32 Hour = 6;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Cycle", meta = (ClampMin = "0", ClampMax = "59"))
	int32 Minute = 0;

	FMaskCycleTime() = default;
	FMaskCycleTime(int32 InDay, int32 InHour, int32 InMinute)
		: Day(InDay), Hour(InHour), Minute(InMinute) {}

	explicit FMaskCycleTime(const MaskGame::FCycleTime& In)
		: Day(In.Day), Hour(In.Hour), Minute(In.Minute) {}

	MaskGame::FCycleTime ToRules() const
	{
		MaskGame::FCycleTime Out;
		Out.Day = Day;
		Out.Hour = Hour;
		Out.Minute = Minute;
		return Out;
	}

	/** "Day 2, 14:30" for the clock HUD. */
	FText ToText() const
	{
		// The clock face is zero-padded digits in every locale we ship, so the
		// numbers are formatted directly rather than through FText::AsNumber.
		return FText::Format(NSLOCTEXT("MaskGame", "CycleTimeFormat", "Day {0}, {1}"),
			FText::AsNumber(Day),
			FText::FromString(FString::Printf(TEXT("%02d:%02d"), Hour, Minute)));
	}
};

/** One row of DT_Masks: where a mask comes from and what it does. */
USTRUCT(BlueprintType)
struct FMaskTableRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mask")
	EMaskType Mask = EMaskType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mask")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mask", meta = (MultiLine = true))
	FText Description;

	/** Walkthrough chapter in which this mask first becomes obtainable. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mask", meta = (ClampMin = "1", ClampMax = "13"))
	int32 Chapter = 1;

	/** Region id where the mask is earned. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mask")
	FName RegionId;

	/** One line on how it is earned, shown in the collection screen once owned. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mask", meta = (MultiLine = true))
	FText HowToEarn;

	/** Flag set when the mask is granted; prefix it with "perm." to survive a rewind. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mask")
	FName GrantFlag;
};

/** One row of DT_Songs. */
USTRUCT(BlueprintType)
struct FSongTableRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Song")
	ESongType Song = ESongType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Song")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Song", meta = (MultiLine = true))
	FText Effect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Song", meta = (ClampMin = "1", ClampMax = "13"))
	int32 Chapter = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Song")
	FName TaughtInRegion;
};

/** One row of DT_Regions: a chapter of the walkthrough as a place. */
USTRUCT(BlueprintType)
struct FRegionTableRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region")
	FName RegionId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region", meta = (ClampMin = "1", ClampMax = "13"))
	int32 Chapter = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region", meta = (MultiLine = true))
	FText Summary;

	/** Owl statue id in this region, empty when the region has none. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region")
	FName StatueId;

	/** Form the region cannot be entered without, or Traveller when it is open to all. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region")
	EMaskForm RequiredForm = EMaskForm::Traveller;

	/** Flag that must be set before the region opens. Empty means always open. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region")
	FName RequiredFlag;

	/** Where the greybox generator places this region's origin, in metres. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region")
	FVector WorldOrigin = FVector::ZeroVector;

	/** Rough extent of the greybox for this region, in metres. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region")
	FVector2D Extent = FVector2D(60.0, 60.0);
};

/** One row of DT_Dungeons: a temple, its boss, and what it yields. */
USTRUCT(BlueprintType)
struct FDungeonTableRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dungeon")
	FName DungeonId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dungeon")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dungeon", meta = (ClampMin = "1", ClampMax = "13"))
	int32 Chapter = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dungeon")
	FName RegionId;

	/** Form the temple is designed around. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dungeon")
	EMaskForm PrimaryForm = EMaskForm::Traveller;

	/** Echo freed by defeating the boss. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dungeon")
	EEchoType Echo = EEchoType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dungeon")
	FText BossName;

	/** Item found inside that the temple is built around. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dungeon")
	FText KeyItem;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dungeon", meta = (ClampMin = "0"))
	int32 RoomCount = 8;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dungeon", meta = (ClampMin = "0"))
	int32 SmallKeys = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dungeon", meta = (ClampMin = "0"))
	int32 StrayFairies = 15;
};

/** One row of DT_Quests: an entry in the notebook. */
USTRUCT(BlueprintType)
struct FQuestTableRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	FName QuestId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest", meta = (ClampMin = "1", ClampMax = "13"))
	int32 Chapter = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	FName RegionId;

	/** NPC whose notebook page this quest lives on. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	FName ActorId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest", meta = (MultiLine = true))
	FText Summary;

	/** Earliest point in the cycle the quest can be started. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	FMaskCycleTime WindowStart = FMaskCycleTime(1, 6, 0);

	/** Point in the cycle after which the chance is gone until the next rewind. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	FMaskCycleTime WindowEnd = FMaskCycleTime(3, 5, 59);

	/** Flag set on completion. Prefix with "perm." for progress a rewind keeps. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	FName CompletionFlag;

	/** Mask awarded on completion, if any. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	EMaskType RewardMask = EMaskType::None;

	/** Heart fragment awarded on completion. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	bool bRewardsHeartFragment = false;
};

/** One row of DT_Schedules: where somebody is, and when. */
USTRUCT(BlueprintType)
struct FScheduleTableRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Schedule")
	FName ActorId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Schedule")
	FName RegionId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Schedule")
	FText Activity;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Schedule")
	FMaskCycleTime Start = FMaskCycleTime(1, 6, 0);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Schedule")
	FMaskCycleTime End = FMaskCycleTime(1, 18, 0);

	/** Flag required for this entry to apply; empty means unconditional. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Schedule")
	FName RequiredFlag;

	/** Higher wins when two entries overlap. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Schedule")
	int32 Priority = 0;
};
