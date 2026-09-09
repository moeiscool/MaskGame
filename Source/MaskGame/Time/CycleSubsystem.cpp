// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

#include "Time/CycleSubsystem.h"

#include "MaskGame.h"

void UCycleSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Clock.Reset();
	bMoonFallBroadcast = false;
	bFinalHoursBroadcast = false;
}

bool UCycleSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// Editor previews and inactive worlds have no business running the clock.
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

TStatId UCycleSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCycleSubsystem, STATGROUP_Tickables);
}

void UCycleSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (Clock.IsMoonFallen())
	{
		return;
	}

	const int32 PreviousMinutes = Clock.GetElapsedMinutes();
	const int32 MinutesAdvanced = Clock.Advance(static_cast<double>(DeltaTime));
	if (MinutesAdvanced > 0)
	{
		BroadcastTransitions(PreviousMinutes);
	}
}

void UCycleSubsystem::BroadcastTransitions(int32 PreviousMinutes)
{
	using namespace MaskGame;

	const FCycleTime Before = FCycleClock::FromElapsedMinutes(PreviousMinutes);
	const FMaskCycleTime Now(Clock.GetTime());

	OnMinuteChanged.Broadcast(Now);

	// A single frame can cross more than one edge if the game hitched, so each
	// edge is tested against the span rather than against "did this exact minute
	// land on the boundary".
	const bool bWasNight = !(Before.Hour >= DawnHour && Before.Hour < DuskHour);
	const bool bIsNight = Clock.IsNight();
	if (bWasNight != bIsNight)
	{
		OnPhaseChanged.Broadcast(GetPhase());
	}

	if (Now.Day != Before.Day)
	{
		UE_LOG(LogMaskGame, Log, TEXT("Dawn of day %d."), Now.Day);
		OnDayChanged.Broadcast(Now.Day);
	}

	if (!bFinalHoursBroadcast && Clock.IsFinalHours())
	{
		bFinalHoursBroadcast = true;
		UE_LOG(LogMaskGame, Warning, TEXT("The final hours have begun."));
		OnFinalHoursBegan.Broadcast();
	}

	if (!bMoonFallBroadcast && Clock.IsMoonFallen())
	{
		bMoonFallBroadcast = true;
		UE_LOG(LogMaskGame, Warning, TEXT("The moon has fallen."));
		OnMoonFell.Broadcast();
	}
}

FMaskCycleTime UCycleSubsystem::GetTime() const
{
	return FMaskCycleTime(Clock.GetTime());
}

float UCycleSubsystem::GetCycleProgress() const
{
	return static_cast<float>(Clock.GetElapsedMinutes()) / static_cast<float>(MaskGame::MinutesPerCycle);
}

float UCycleSubsystem::GetSunElevationDegrees() const
{
	using namespace MaskGame;

	const FCycleTime Now = Clock.GetTime();
	const int32 MinuteOfDay = Now.Hour * MinutesPerHour + Now.Minute;

	// Zero at dawn and dusk, +90 at noon, -90 at midnight.
	const float Turns = static_cast<float>(MinuteOfDay - DawnHour * MinutesPerHour) / static_cast<float>(MinutesPerDay);
	return 90.0f * FMath::Sin(Turns * 2.0f * PI);
}

FRotator UCycleSubsystem::GetSunLightRotation() const
{
	// A directional light points along its forward vector, so a sun high in the
	// sky is a light pitched steeply downwards: the pitch is the negated elevation.
	// Yaw swings gently across the cycle so shadows are not identical each day.
	const float Yaw = -35.0f + 20.0f * GetCycleProgress();
	return FRotator(-GetSunElevationDegrees(), Yaw, 0.0f);
}

void UCycleSubsystem::SetFlowRate(ETimeFlowRate Rate)
{
	Clock.SetFlow(static_cast<MaskGame::ETimeFlow>(Rate));
}

int32 UCycleSubsystem::SkipToNextPhase()
{
	const int32 PreviousMinutes = Clock.GetElapsedMinutes();
	const int32 Skipped = Clock.SkipToNextPhaseBoundary();
	if (Skipped > 0)
	{
		BroadcastTransitions(PreviousMinutes);
	}
	return Skipped;
}

void UCycleSubsystem::RewindCycle()
{
	Clock.Reset();
	bMoonFallBroadcast = false;
	bFinalHoursBroadcast = false;

	UE_LOG(LogMaskGame, Log, TEXT("The cycle returns to the first dawn."));
	OnCycleRewound.Broadcast();
	OnMinuteChanged.Broadcast(GetTime());
	OnPhaseChanged.Broadcast(GetPhase());
	OnDayChanged.Broadcast(1);
}

void UCycleSubsystem::SetElapsedMinutes(int32 Minutes)
{
	const int32 PreviousMinutes = Clock.GetElapsedMinutes();
	Clock.SetElapsedMinutes(Minutes);

	// Re-arm the one-shot edges so that a load into the middle of day one does
	// not leave the final-hours flag stuck from a previous cycle.
	bFinalHoursBroadcast = Clock.IsFinalHours();
	bMoonFallBroadcast = Clock.IsMoonFallen();

	BroadcastTransitions(PreviousMinutes);
}
