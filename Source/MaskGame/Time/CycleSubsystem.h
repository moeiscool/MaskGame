// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "Core/MaskGameTypes.h"

#include "CycleSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCycleMinuteChanged, FMaskCycleTime, Time);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCyclePhaseChanged, EDayPhaseType, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCycleDayChanged, int32, NewDay);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFinalHoursBegan);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMoonFell);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCycleRewound);

/**
 * The clock the whole game is played against.
 *
 * Owns a MaskGame::FCycleClock and turns its movement into events: a tick every
 * in-game minute, an edge at each dawn and dusk, one when the final hours begin
 * and one at the moment of impact. Everything that cares about time - NPC
 * schedules, shop doors, the sky, the HUD - listens here rather than counting
 * seconds of its own.
 *
 * Lives on the world, so travelling to a new level starts a fresh clock; the
 * position within the cycle is carried across by UMaskGameInstance.
 */
UCLASS()
class MASKGAME_API UCycleSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	// USubsystem.
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	// FTickableGameObject.
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	/** Fired once per in-game minute. */
	UPROPERTY(BlueprintAssignable, Category = "Cycle")
	FOnCycleMinuteChanged OnMinuteChanged;

	/** Fired at 06:00 and 18:00. */
	UPROPERTY(BlueprintAssignable, Category = "Cycle")
	FOnCyclePhaseChanged OnPhaseChanged;

	/** Fired at each 06:00 that starts a new day. */
	UPROPERTY(BlueprintAssignable, Category = "Cycle")
	FOnCycleDayChanged OnDayChanged;

	/** Fired at midnight of the third night, six hours from impact. */
	UPROPERTY(BlueprintAssignable, Category = "Cycle")
	FOnFinalHoursBegan OnFinalHoursBegan;

	/** Fired when the clock runs out. */
	UPROPERTY(BlueprintAssignable, Category = "Cycle")
	FOnMoonFell OnMoonFell;

	/** Fired when the Hymn of Return sends the cycle back to the first dawn. */
	UPROPERTY(BlueprintAssignable, Category = "Cycle")
	FOnCycleRewound OnCycleRewound;

	UFUNCTION(BlueprintPure, Category = "Cycle")
	FMaskCycleTime GetTime() const;

	UFUNCTION(BlueprintPure, Category = "Cycle")
	int32 GetElapsedMinutes() const { return Clock.GetElapsedMinutes(); }

	UFUNCTION(BlueprintPure, Category = "Cycle")
	int32 GetRemainingMinutes() const { return Clock.GetRemainingMinutes(); }

	UFUNCTION(BlueprintPure, Category = "Cycle")
	bool IsNight() const { return Clock.IsNight(); }

	UFUNCTION(BlueprintPure, Category = "Cycle")
	bool IsFinalHours() const { return Clock.IsFinalHours(); }

	UFUNCTION(BlueprintPure, Category = "Cycle")
	bool IsMoonFallen() const { return Clock.IsMoonFallen(); }

	UFUNCTION(BlueprintPure, Category = "Cycle")
	EDayPhaseType GetPhase() const { return static_cast<EDayPhaseType>(Clock.GetPhase()); }

	/** Zero at the cycle's opening dawn, one at impact. Drives the moon's descent. */
	UFUNCTION(BlueprintPure, Category = "Cycle")
	float GetCycleProgress() const;

	/**
	 * How high the sun stands, in degrees: -90 at midnight, 0 at dawn and dusk,
	 * +90 at noon.
	 */
	UFUNCTION(BlueprintPure, Category = "Cycle")
	float GetSunElevationDegrees() const;

	/** Rotation to drive the level's directional light with, derived from the elevation. */
	UFUNCTION(BlueprintPure, Category = "Cycle")
	FRotator GetSunLightRotation() const;

	UFUNCTION(BlueprintCallable, Category = "Cycle")
	void SetFlowRate(ETimeFlowRate Rate);

	UFUNCTION(BlueprintPure, Category = "Cycle")
	ETimeFlowRate GetFlowRate() const { return static_cast<ETimeFlowRate>(Clock.GetFlow()); }

	/** Song of Double Time: jump to the next dawn or dusk. Returns minutes skipped. */
	UFUNCTION(BlueprintCallable, Category = "Cycle")
	int32 SkipToNextPhase();

	/** The Hymn of Return: back to the first dawn. Progress is banked by the caller. */
	UFUNCTION(BlueprintCallable, Category = "Cycle")
	void RewindCycle();

	/** Place the clock at a point in the cycle, e.g. after loading a save. */
	UFUNCTION(BlueprintCallable, Category = "Cycle")
	void SetElapsedMinutes(int32 Minutes);

	/** Direct access for systems that need the rules type, such as the schedule table. */
	const MaskGame::FCycleClock& GetClock() const { return Clock; }

private:
	/** Raise every edge crossed between the previous minute and the current one. */
	void BroadcastTransitions(int32 PreviousMinutes);

	MaskGame::FCycleClock Clock;

	/** Guards OnMoonFell against firing more than once per cycle. */
	bool bMoonFallBroadcast = false;
	/** Guards OnFinalHoursBegan the same way. */
	bool bFinalHoursBroadcast = false;
};
