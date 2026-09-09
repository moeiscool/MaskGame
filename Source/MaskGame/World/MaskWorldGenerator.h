// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Core/MaskGameTypes.h"

#include "MaskWorldGenerator.generated.h"

class AOwlStatue;
class ATempleGate;
class UInstancedStaticMeshComponent;
class UQuestSubsystem;

/**
 * Builds a playable greybox of the whole game out of the region and dungeon
 * tables, at runtime, into whatever level it is spawned in.
 *
 * This exists so the project is a game rather than a folder of systems while no
 * levels have been authored: drop the game mode into an empty map and all
 * thirteen chapters lay themselves out, connected, walkable and populated with
 * the statues, gates and chests the tables describe. Replacing it later means
 * turning bGenerateGreyboxWorld off in project settings; nothing else depends on
 * it existing.
 *
 * Everything it makes is either an instance on one of the three instanced mesh
 * components or an actor recorded in SpawnedActors, so RebuildWorld can put the
 * world back exactly as it started when the cycle rewinds.
 */
UCLASS()
class MASKGAME_API AMaskWorldGenerator : public AActor
{
	GENERATED_BODY()

public:
	AMaskWorldGenerator();

	virtual void BeginPlay() override;

	/** Tear the world down and lay it out again. Called on every cycle rewind. */
	UFUNCTION(BlueprintCallable, Category = "World")
	void RebuildWorld();

	/** Centre of a region in world space, for warping and for placing the player. */
	UFUNCTION(BlueprintPure, Category = "World")
	FVector GetRegionCentre(FName RegionId) const;

	/** Thickness of the ground plates and walls, in centimetres. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World", meta = (ClampMin = "10.0"))
	float GroundThickness = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World", meta = (ClampMin = "50.0"))
	float WallHeight = 400.0f;

	/** Side length of one greybox dungeon room, in centimetres. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World", meta = (ClampMin = "200.0"))
	float RoomSize = 1600.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World")
	TSubclassOf<AOwlStatue> OwlStatueClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World")
	TSubclassOf<ATempleGate> TempleGateClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World")
	TSubclassOf<class ATreasureChest> ChestClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World")
	TSubclassOf<class AMaskEnemy> EnemyClass;

protected:
	void BuildRegion(const FRegionTableRow& Region);
	void BuildDungeon(const FDungeonTableRow& Dungeon, const FVector& Origin);

	/** Adds a box instance given its centre and full size, both in centimetres. */
	void AddBox(UInstancedStaticMeshComponent& Target, const FVector& Centre, const FVector& Size);

	/** A floor plate with a low wall around it, open on the side facing the next region. */
	void AddPlatform(const FVector& Centre, const FVector2D& SizeCm, bool bWalled);

	/**
	 * Spawns an actor and records it so RebuildWorld can clean it up.
	 * Returns null when Class is unset, so callers can guard on the result.
	 */
	AActor* SpawnTracked(UClass* Class, const FVector& Location);

	void ClearWorld();

	UQuestSubsystem* GetQuests() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> GroundMeshes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> WallMeshes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> PropMeshes;

private:
	/** Everything this generator spawned, so a rebuild leaves nothing behind. */
	UPROPERTY()
	TArray<TObjectPtr<AActor>> SpawnedActors;

	/** Region centres in world space, filled in as regions are built. */
	UPROPERTY()
	TMap<FName, FVector> RegionCentres;
};
