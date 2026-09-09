// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

#include "Songs/OcarinaComponent.h"

#include "Components/InputComponent.h"

#include "Core/MaskGameInstance.h"
#include "MaskGame.h"
#include "Time/CycleSubsystem.h"

UOcarinaComponent::UOcarinaComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// The matcher only needs enough resolution to time a performance, so it runs
	// well below frame rate.
	PrimaryComponentTick.TickInterval = 0.05f;
}

void UOcarinaComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// The performance is timed on unpaused real seconds so that holding the clock
	// still while the ocarina is out does not also stop the note timeout.
	PerformanceClock += static_cast<double>(DeltaTime);
	Matcher.Tick(PerformanceClock);
}

void UOcarinaComponent::BindInput(UInputComponent& InputComponent)
{
	InputComponent.BindAction(TEXT("Ocarina"), IE_Pressed, this, &UOcarinaComponent::ToggleDrawn);
	InputComponent.BindAction(TEXT("NoteLeft"), IE_Pressed, this, &UOcarinaComponent::OnNoteLeft);
	InputComponent.BindAction(TEXT("NoteRight"), IE_Pressed, this, &UOcarinaComponent::OnNoteRight);
	InputComponent.BindAction(TEXT("NoteUp"), IE_Pressed, this, &UOcarinaComponent::OnNoteUp);
	InputComponent.BindAction(TEXT("NoteDown"), IE_Pressed, this, &UOcarinaComponent::OnNoteDown);
	InputComponent.BindAction(TEXT("NoteA"), IE_Pressed, this, &UOcarinaComponent::OnNoteA);
}

UCycleSubsystem* UOcarinaComponent::GetCycle() const
{
	const UWorld* World = GetWorld();
	return World != nullptr ? World->GetSubsystem<UCycleSubsystem>() : nullptr;
}

UMaskGameInstance* UOcarinaComponent::GetProgression() const
{
	return UMaskGameInstance::Get(this);
}

void UOcarinaComponent::ToggleDrawn()
{
	SetDrawn(!bDrawn);
}

void UOcarinaComponent::SetDrawn(bool bNewDrawn)
{
	if (bDrawn == bNewDrawn)
	{
		return;
	}

	bDrawn = bNewDrawn;
	ClearPerformance();

	// Holding the clock while the ocarina is out is what makes the instrument
	// safe to use in the final hours.
	if (UCycleSubsystem* Cycle = GetCycle())
	{
		Cycle->SetFlowRate(bDrawn ? ETimeFlowRate::Paused : ETimeFlowRate::Normal);
	}

	OnOcarinaToggled.Broadcast(bDrawn);
}

void UOcarinaComponent::ClearPerformance()
{
	Matcher.Clear();
}

void UOcarinaComponent::PlayNote(EOcarinaNote Note)
{
	if (!bDrawn)
	{
		return;
	}

	OnNotePlayed.Broadcast(Note);

	const MaskGame::FSongMatch Match = Matcher.PushNote(static_cast<MaskGame::ENote>(Note), PerformanceClock);
	if (!Match.IsValid())
	{
		return;
	}

	const ESongType Song = static_cast<ESongType>(Match.Id);
	const ESongPerformance Performance = static_cast<ESongPerformance>(Match.Performance);

	// Getting the notes right is not the same as knowing the song. A player who
	// works the sequence out early hears it played back and nothing happens.
	const UMaskGameInstance* Progression = GetProgression();
	if (Progression == nullptr || !Progression->HasSong(Song))
	{
		UE_LOG(LogMaskGame, Verbose, TEXT("Played the notes of %s, but has not been taught it."),
			*UEnum::GetDisplayValueAsText(Song).ToString());
		return;
	}

	UE_LOG(LogMaskGame, Log, TEXT("Played %s (%s)."),
		*UEnum::GetDisplayValueAsText(Song).ToString(),
		*UEnum::GetDisplayValueAsText(Performance).ToString());

	// Putting the instrument away before the effect lands means the clock is
	// running again by the time the world reacts.
	SetDrawn(false);

	if (!ApplyTimeSong(Song, Performance))
	{
		OnSongPlayed.Broadcast(Song, Performance, GetOwner());
	}
}

bool UOcarinaComponent::ApplyTimeSong(ESongType Song, ESongPerformance Performance)
{
	UCycleSubsystem* Cycle = GetCycle();
	if (Cycle == nullptr || Song != ESongType::HymnOfReturn)
	{
		return false;
	}

	switch (Performance)
	{
	case ESongPerformance::Reversed:
		// Time crawls, but the cycle is not rewound: this is the song that buys
		// an afternoon, not the one that gives back the three days.
		Cycle->SetFlowRate(ETimeFlowRate::Slowed);
		UE_LOG(LogMaskGame, Log, TEXT("Time slows to a crawl."));
		return true;

	case ESongPerformance::Doubled:
		Cycle->SkipToNextPhase();
		UE_LOG(LogMaskGame, Log, TEXT("The hours run together; it is suddenly %s."),
			*Cycle->GetTime().ToText().ToString());
		return true;

	case ESongPerformance::Forward:
	default:
		// The full rewind is the game mode's business: progression has to be
		// banked and the region reloaded, neither of which belongs to a
		// component on the player's back.
		OnSongPlayed.Broadcast(Song, Performance, GetOwner());
		return true;
	}
}
