// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

#include "World/MaskWorldGenerator.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

#include "AI/MaskEnemy.h"
#include "MaskGame.h"
#include "Quests/QuestSubsystem.h"
#include "World/OwlStatue.h"
#include "World/TempleGate.h"
#include "World/TreasureChest.h"

namespace
{
	/** Table positions are authored in metres; the engine works in centimetres. */
	constexpr float MetresToCentimetres = 100.0f;

	/** The engine's basic cube is a 100cm box centred on its origin. */
	constexpr float BasicCubeExtent = 100.0f;
}

AMaskWorldGenerator::AMaskWorldGenerator()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// One instanced component per material role keeps the whole greybox down to
	// three draw calls no matter how many chapters the tables grow to.
	GroundMeshes = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("GroundMeshes"));
	WallMeshes = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("WallMeshes"));
	PropMeshes = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("PropMeshes"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	for (UInstancedStaticMeshComponent* Component : { GroundMeshes.Get(), WallMeshes.Get(), PropMeshes.Get() })
	{
		Component->SetupAttachment(Root);
		Component->SetMobility(EComponentMobility::Static);
		Component->SetCollisionProfileName(TEXT("BlockAll"));
		if (CubeFinder.Succeeded())
		{
			Component->SetStaticMesh(CubeFinder.Object);
		}
	}

	OwlStatueClass = AOwlStatue::StaticClass();
	TempleGateClass = ATempleGate::StaticClass();
	ChestClass = ATreasureChest::StaticClass();
	EnemyClass = AMaskEnemy::StaticClass();
}

void AMaskWorldGenerator::BeginPlay()
{
	Super::BeginPlay();
	RebuildWorld();
}

UQuestSubsystem* AMaskWorldGenerator::GetQuests() const
{
	return UQuestSubsystem::Get(this);
}

void AMaskWorldGenerator::ClearWorld()
{
	for (AActor* Actor : SpawnedActors)
	{
		if (IsValid(Actor))
		{
			Actor->Destroy();
		}
	}
	SpawnedActors.Reset();
	RegionCentres.Reset();

	GroundMeshes->ClearInstances();
	WallMeshes->ClearInstances();
	PropMeshes->ClearInstances();
}

void AMaskWorldGenerator::RebuildWorld()
{
	ClearWorld();

	UQuestSubsystem* Quests = GetQuests();
	if (Quests == nullptr)
	{
		UE_LOG(LogMaskGame, Error, TEXT("No quest subsystem; the greybox world cannot be built."));
		return;
	}

	TArray<FRegionTableRow> Regions = Quests->GetAllRegions();
	if (Regions.IsEmpty())
	{
		UE_LOG(LogMaskGame, Warning, TEXT("The region table is empty; nothing to build."));
		return;
	}

	// Chapter order is the order the player walks the world in, so building in it
	// keeps the log readable and puts region one under the player start.
	Regions.Sort([](const FRegionTableRow& A, const FRegionTableRow& B)
	{
		return A.Chapter != B.Chapter ? A.Chapter < B.Chapter : A.RegionId.LexicalLess(B.RegionId);
	});

	for (const FRegionTableRow& Region : Regions)
	{
		BuildRegion(Region);
	}

	for (const FDungeonTableRow& Dungeon : Quests->GetAllDungeons())
	{
		const FVector* Origin = RegionCentres.Find(Dungeon.RegionId);
		if (Origin == nullptr)
		{
			UE_LOG(LogMaskGame, Warning, TEXT("Dungeon '%s' names region '%s', which is not in the region table."),
				*Dungeon.DungeonId.ToString(), *Dungeon.RegionId.ToString());
			continue;
		}
		BuildDungeon(Dungeon, *Origin);
	}

	UE_LOG(LogMaskGame, Log, TEXT("Greybox world built: %d regions, %d dungeons, %d actors."),
		Regions.Num(), Quests->GetAllDungeons().Num(), SpawnedActors.Num());
}

void AMaskWorldGenerator::AddBox(UInstancedStaticMeshComponent& Target, const FVector& Centre, const FVector& Size)
{
	const FVector Scale = Size / BasicCubeExtent;
	Target.AddInstance(FTransform(FRotator::ZeroRotator, Centre, Scale), /*bWorldSpace=*/false);
}

void AMaskWorldGenerator::AddPlatform(const FVector& Centre, const FVector2D& SizeCm, bool bWalled)
{
	// The plate's top surface sits at the platform's nominal height, so actors
	// placed at Centre.Z stand on it rather than inside it.
	const FVector GroundCentre(Centre.X, Centre.Y, Centre.Z - GroundThickness * 0.5f);
	AddBox(*GroundMeshes, GroundCentre, FVector(SizeCm.X, SizeCm.Y, GroundThickness));

	if (!bWalled)
	{
		return;
	}

	const float HalfX = SizeCm.X * 0.5f;
	const float HalfY = SizeCm.Y * 0.5f;
	const float WallThickness = GroundThickness;
	const float WallZ = Centre.Z + WallHeight * 0.5f;

	// Three walls, not four: the side facing +X is left open so neighbouring
	// regions stay connected on foot.
	AddBox(*WallMeshes, FVector(Centre.X - HalfX, Centre.Y, WallZ), FVector(WallThickness, SizeCm.Y, WallHeight));
	AddBox(*WallMeshes, FVector(Centre.X, Centre.Y - HalfY, WallZ), FVector(SizeCm.X, WallThickness, WallHeight));
	AddBox(*WallMeshes, FVector(Centre.X, Centre.Y + HalfY, WallZ), FVector(SizeCm.X, WallThickness, WallHeight));
}

AActor* AMaskWorldGenerator::SpawnTracked(UClass* Class, const FVector& Location)
{
	if (Class == nullptr)
	{
		return nullptr;
	}

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* Spawned = GetWorld()->SpawnActor<AActor>(Class, FTransform(Location), Params);
	if (Spawned != nullptr)
	{
		SpawnedActors.Add(Spawned);
	}
	return Spawned;
}

void AMaskWorldGenerator::BuildRegion(const FRegionTableRow& Region)
{
	const FVector Centre = Region.WorldOrigin * MetresToCentimetres;
	const FVector2D SizeCm = Region.Extent * MetresToCentimetres;

	RegionCentres.Add(Region.RegionId, Centre);
	AddPlatform(Centre, SizeCm, /*bWalled=*/true);

	// A marker obelisk per region, tall enough to sight across the world and get
	// your bearings while greyboxing. Deliberately off-centre: the middle of a
	// region is where a player start goes, and spawning inside a solid block is
	// a confusing first thirty seconds.
	AddBox(*PropMeshes,
		Centre + FVector(0.0f, SizeCm.Y * 0.4f, 300.0f),
		FVector(200.0f, 200.0f, 600.0f));

	if (!Region.StatueId.IsNone())
	{
		const FVector StatueLocation = Centre + FVector(-SizeCm.X * 0.3f, -SizeCm.Y * 0.3f, 60.0f);
		if (AOwlStatue* Statue = Cast<AOwlStatue>(SpawnTracked(OwlStatueClass, StatueLocation)))
		{
			Statue->StatueId = Region.StatueId;
			Statue->DisplayName = Region.DisplayName;
		}
	}

	// A chest and a couple of enemies per region so that the greybox has
	// something in it to do rather than being an empty plate.
	if (ATreasureChest* Chest = Cast<ATreasureChest>(SpawnTracked(ChestClass, Centre + FVector(SizeCm.X * 0.25f, 0.0f, 50.0f))))
	{
		Chest->ChestId = FName(*FString::Printf(TEXT("%s.field"), *Region.RegionId.ToString()));

		// The first chapter's chest is the fairy's gift: without a magic meter the
		// sapling cannot glide, the boulderkin cannot roll and the tideborn has no
		// barrier, so the greybox would have three forms and no abilities.
		if (Region.Chapter == 1)
		{
			Chest->Contents = EChestContents::MagicMeter;
			Chest->Amount = 1;
		}
		else
		{
			Chest->Contents = EChestContents::Rupees;
			Chest->Amount = 20;
		}
	}

	for (int32 Index = 0; Index < 2; ++Index)
	{
		const float Angle = 90.0f * static_cast<float>(Index);
		const FVector Offset = FRotator(0.0f, Angle, 0.0f).Vector() * (SizeCm.X * 0.35f);
		SpawnTracked(EnemyClass, Centre + Offset + FVector(0.0f, 0.0f, 100.0f));
	}
}

void AMaskWorldGenerator::BuildDungeon(const FDungeonTableRow& Dungeon, const FVector& Origin)
{
	// Temples run north from their region's plate as a corridor of rooms, ending
	// in a wider arena. It is a placeholder shape, but it is a real one: rooms to
	// walk, keys to find, a door at the end.
	const FVector Entrance = Origin + FVector(0.0f, RoomSize * 2.0f, 0.0f);

	if (ATempleGate* Gate = Cast<ATempleGate>(SpawnTracked(TempleGateClass, Entrance)))
	{
		Gate->DungeonId = Dungeon.DungeonId;
		Gate->DisplayName = Dungeon.DisplayName;
		Gate->RequiredSong = ESongType::SonataOfRousing;
		Gate->RequiredForm = Dungeon.PrimaryForm;
	}

	const int32 RoomCount = FMath::Max(1, Dungeon.RoomCount);
	for (int32 RoomIndex = 0; RoomIndex < RoomCount; ++RoomIndex)
	{
		const FVector RoomCentre = Entrance + FVector(0.0f, RoomSize * (RoomIndex + 1), 0.0f);
		AddPlatform(RoomCentre, FVector2D(RoomSize, RoomSize) * 0.9f, /*bWalled=*/true);

		// Small keys are spread through the first rooms, one per chest.
		if (RoomIndex < Dungeon.SmallKeys)
		{
			if (ATreasureChest* Chest = Cast<ATreasureChest>(SpawnTracked(ChestClass, RoomCentre + FVector(0.0f, 0.0f, 50.0f))))
			{
				Chest->ChestId = FName(*FString::Printf(TEXT("%s.key%d"), *Dungeon.DungeonId.ToString(), RoomIndex));
				Chest->Contents = EChestContents::DungeonKey;
				Chest->Amount = 1;
			}
		}

		SpawnTracked(EnemyClass, RoomCentre + FVector(RoomSize * 0.25f, 0.0f, 100.0f));
	}

	// The arena: twice as wide, no chest, and the echo waiting at the far end.
	const FVector ArenaCentre = Entrance + FVector(0.0f, RoomSize * (RoomCount + 2), 0.0f);
	AddPlatform(ArenaCentre, FVector2D(RoomSize * 2.0f, RoomSize * 2.0f), /*bWalled=*/true);

	if (AMaskEnemy* Boss = Cast<AMaskEnemy>(SpawnTracked(EnemyClass, ArenaCentre + FVector(0.0f, 0.0f, 150.0f))))
	{
		Boss->MakeBoss(Dungeon.Echo, Dungeon.BossName);
	}
}
