// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

#include "World/MaskSkyDirector.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/SkyLight.h"
#include "EngineUtils.h"

#include "MaskGame.h"
#include "Time/CycleSubsystem.h"

#include "Components/SkyAtmosphereComponent.h"

AMaskSkyDirector::AMaskSkyDirector()
{
	// Nothing to do per frame: the clock drives this, once an in-game minute.
	PrimaryActorTick.bCanEverTick = false;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

namespace
{
	/** The first actor of a type already placed in the level, or null. */
	template <typename ActorType>
	ActorType* FindExisting(UWorld* World)
	{
		if (World == nullptr)
		{
			return nullptr;
		}

		for (TActorIterator<ActorType> It(World); It; ++It)
		{
			return *It;
		}
		return nullptr;
	}
}

void AMaskSkyDirector::BeginPlay()
{
	Super::BeginPlay();

	EnsureSkyActors();

	if (UCycleSubsystem* Cycle = GetCycle())
	{
		Cycle->OnMinuteChanged.AddDynamic(this, &AMaskSkyDirector::HandleMinuteChanged);
		Cycle->OnCycleRewound.AddDynamic(this, &AMaskSkyDirector::HandleCycleRewound);
	}

	UpdateSky();
}

void AMaskSkyDirector::EndPlay(const EEndPlayReason::Type Reason)
{
	if (UCycleSubsystem* Cycle = GetCycle())
	{
		Cycle->OnMinuteChanged.RemoveDynamic(this, &AMaskSkyDirector::HandleMinuteChanged);
		Cycle->OnCycleRewound.RemoveDynamic(this, &AMaskSkyDirector::HandleCycleRewound);
	}

	// Only tidy up what this director put there; a sky the level author placed
	// by hand is theirs and outlives us.
	for (AActor* Actor : SpawnedSkyActors)
	{
		if (IsValid(Actor))
		{
			Actor->Destroy();
		}
	}
	SpawnedSkyActors.Reset();

	Super::EndPlay(Reason);
}

UCycleSubsystem* AMaskSkyDirector::GetCycle() const
{
	const UWorld* World = GetWorld();
	return World != nullptr ? World->GetSubsystem<UCycleSubsystem>() : nullptr;
}

void AMaskSkyDirector::EnsureSkyActors()
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	Sun = FindExisting<ADirectionalLight>(World);
	if (Sun == nullptr)
	{
		Sun = World->SpawnActor<ADirectionalLight>(FVector(0.0f, 0.0f, 10000.0f), FRotator(-45.0f, -35.0f, 0.0f), Params);
		if (Sun != nullptr)
		{
			SpawnedSkyActors.Add(Sun);
		}
	}

	// The sun has to be movable to be swung through seventy-two hours; a light
	// the level author left static would bake and then refuse to move.
	if (Sun != nullptr)
	{
		if (UDirectionalLightComponent* Component = Cast<UDirectionalLightComponent>(Sun->GetLightComponent()))
		{
			Component->SetMobility(EComponentMobility::Movable);
			Component->SetAtmosphereSunLight(true);
		}
	}

	Atmosphere = FindExisting<ASkyAtmosphere>(World);
	if (Atmosphere == nullptr)
	{
		Atmosphere = World->SpawnActor<ASkyAtmosphere>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
		if (Atmosphere != nullptr)
		{
			SpawnedSkyActors.Add(Atmosphere);
		}
	}

	Sky = FindExisting<ASkyLight>(World);
	if (Sky == nullptr)
	{
		Sky = World->SpawnActor<ASkyLight>(FVector(0.0f, 0.0f, 500.0f), FRotator::ZeroRotator, Params);
		if (Sky != nullptr)
		{
			SpawnedSkyActors.Add(Sky);
		}
	}
	if (Sky != nullptr)
	{
		if (USkyLightComponent* Component = Sky->GetLightComponent())
		{
			Component->SetMobility(EComponentMobility::Movable);
			// Real-time capture, or the ambient light stays at whatever the sky
			// looked like the moment the level loaded.
			Component->SourceType = ESkyLightSourceType::SLS_CapturedScene;
			Component->bRealTimeCapture = true;
			Component->SetIntensity(1.0f);
		}
	}

	Fog = FindExisting<AExponentialHeightFog>(World);
	if (Fog == nullptr)
	{
		Fog = World->SpawnActor<AExponentialHeightFog>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
		if (Fog != nullptr)
		{
			SpawnedSkyActors.Add(Fog);
		}
	}

	UE_LOG(LogMaskGame, Log, TEXT("Sky ready: %d actors spawned, the rest adopted from the level."),
		SpawnedSkyActors.Num());
}

void AMaskSkyDirector::HandleMinuteChanged(FMaskCycleTime /*Time*/)
{
	UpdateSky();
}

void AMaskSkyDirector::HandleCycleRewound()
{
	UpdateSky();
}

void AMaskSkyDirector::UpdateSky()
{
	const UCycleSubsystem* Cycle = GetCycle();
	if (Cycle == nullptr || Sun == nullptr)
	{
		return;
	}

	Sun->SetActorRotation(Cycle->GetSunLightRotation());

	ULightComponent* Light = Sun->GetLightComponent();
	if (Light == nullptr)
	{
		return;
	}

	// Elevation runs -90 at midnight to +90 at noon. Normalising it to 0..1 gives
	// one number that drives both brightness and colour.
	const float Elevation = Cycle->GetSunElevationDegrees();
	const float Daylight = FMath::Clamp((Elevation + 12.0f) / 30.0f, 0.0f, 1.0f);

	const FLinearColor DayColour = FMath::Lerp(HorizonColour, NoonColour, Daylight);
	FLinearColor Colour = FMath::Lerp(NightColour, DayColour, Daylight);
	float Intensity = FMath::Lerp(NightIntensity, DaylightIntensity, Daylight);

	// The last six hours are the game telling the player, without a word of
	// dialogue, that they have run out of time.
	if (Cycle->IsFinalHours())
	{
		const float Urgency = 1.0f - static_cast<float>(Cycle->GetRemainingMinutes()) / (6.0f * 60.0f);
		Colour = FMath::Lerp(Colour, FinalHoursColour, FMath::Clamp(Urgency, 0.0f, 1.0f));
		Intensity = FMath::Lerp(Intensity, NightIntensity * 0.5f, FMath::Clamp(Urgency, 0.0f, 1.0f));
	}

	Light->SetLightColor(Colour);
	Light->SetIntensity(Intensity);
}
