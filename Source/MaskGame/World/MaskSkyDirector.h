// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Core/MaskGameTypes.h"

#include "MaskSkyDirector.generated.h"

class ADirectionalLight;
class AExponentialHeightFog;
class ASkyAtmosphere;
class ASkyLight;
class UCycleSubsystem;

/**
 * Drives the sky from the clock.
 *
 * The three-day cycle is the game's central mechanic and it is invisible unless
 * the light moves with it, so this actor takes the sun's elevation from
 * UCycleSubsystem and puts it on a directional light: dawn at 06:00, noon
 * overhead, dusk at 18:00, and a long cold night in between that turns
 * unmistakably wrong once the final hours begin.
 *
 * It adopts whatever sky actors the level already has and spawns the ones it is
 * missing, which is what lets an empty level be playable: without this, a level
 * with no light renders black and the day/night cycle is a number on the HUD.
 *
 * Updates run off the cycle's per-minute event rather than every frame - about
 * once every three quarters of a second at normal flow, which is far smoother
 * than the sun visibly needs.
 */
UCLASS()
class MASKGAME_API AMaskSkyDirector : public AActor
{
	GENERATED_BODY()

public:
	AMaskSkyDirector();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	/** Sun brightness at noon, in lux. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sky", meta = (ClampMin = "0.0"))
	float DaylightIntensity = 8.0f;

	/** Sun brightness once it is fully below the horizon. Moonlight, not darkness. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sky", meta = (ClampMin = "0.0"))
	float NightIntensity = 0.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sky")
	FLinearColor NoonColour = FLinearColor(1.0f, 0.98f, 0.92f);

	/** Warm low sun, blended in as the sun approaches the horizon. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sky")
	FLinearColor HorizonColour = FLinearColor(1.0f, 0.62f, 0.36f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sky")
	FLinearColor NightColour = FLinearColor(0.42f, 0.52f, 0.85f);

	/** The sky the final hours turn: everything goes red and the light drops. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sky")
	FLinearColor FinalHoursColour = FLinearColor(0.95f, 0.22f, 0.16f);

protected:
	/** Adopts the level's existing sky actors and spawns whatever is missing. */
	void EnsureSkyActors();

	UFUNCTION()
	void HandleMinuteChanged(FMaskCycleTime Time);

	UFUNCTION()
	void HandleCycleRewound();

	/** Pushes the current time onto the light. */
	void UpdateSky();

	UCycleSubsystem* GetCycle() const;

	UPROPERTY()
	TObjectPtr<ADirectionalLight> Sun;

	UPROPERTY()
	TObjectPtr<ASkyLight> Sky;

	UPROPERTY()
	TObjectPtr<ASkyAtmosphere> Atmosphere;

	UPROPERTY()
	TObjectPtr<AExponentialHeightFog> Fog;

	/** True for actors this director spawned, which are the ones it may destroy. */
	UPROPERTY()
	TArray<TObjectPtr<AActor>> SpawnedSkyActors;
};
