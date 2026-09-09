// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "Core/MaskGameTypes.h"
#include "SongMatcher.h"

#include "OcarinaComponent.generated.h"

class UCycleSubsystem;
class UMaskGameInstance;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNotePlayed, EOcarinaNote, Note);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSongPlayed, ESongType, Song, ESongPerformance, Performance);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOcarinaToggled, bool, bDrawn);

/**
 * The instrument.
 *
 * Notes go in one at a time and MaskGame::FSongMatcher decides when they have
 * become a song. The three songs that act on time itself are applied here,
 * because they are global and have no actor to ask; every other song is
 * broadcast for the world to answer - a temple to rise, a guard to sleep, a
 * statue of the current form to be left behind.
 *
 * A song the player has not learned is heard but does nothing, which is how a
 * player who works out the notes early still has to be taught the song.
 */
UCLASS(ClassGroup = (MaskGame), meta = (BlueprintSpawnableComponent))
class MASKGAME_API UOcarinaComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UOcarinaComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Binds the note keys and the draw/put-away key. */
	void BindInput(UInputComponent& InputComponent);

	/** Take the ocarina out or put it away. Time holds still while it is drawn. */
	UFUNCTION(BlueprintCallable, Category = "Ocarina")
	void ToggleDrawn();

	UFUNCTION(BlueprintCallable, Category = "Ocarina")
	void SetDrawn(bool bNewDrawn);

	UFUNCTION(BlueprintPure, Category = "Ocarina")
	bool IsDrawn() const { return bDrawn; }

	/** Play one note. Ignored while the ocarina is put away. */
	UFUNCTION(BlueprintCallable, Category = "Ocarina")
	void PlayNote(EOcarinaNote Note);

	/** Abandon a half-played song. */
	UFUNCTION(BlueprintCallable, Category = "Ocarina")
	void ClearPerformance();

	/** How many notes are sitting in the buffer, for the on-screen stave. */
	UFUNCTION(BlueprintPure, Category = "Ocarina")
	int32 GetBufferedNoteCount() const { return static_cast<int32>(Matcher.GetBuffer().size()); }

	UPROPERTY(BlueprintAssignable, Category = "Ocarina")
	FOnNotePlayed OnNotePlayed;

	/** Fired for a recognised song the player has learned. */
	UPROPERTY(BlueprintAssignable, Category = "Ocarina")
	FOnSongPlayed OnSongPlayed;

	UPROPERTY(BlueprintAssignable, Category = "Ocarina")
	FOnOcarinaToggled OnOcarinaToggled;

private:
	/** Applies the songs that act on the clock. Returns true when one was handled. */
	bool ApplyTimeSong(ESongType Song, ESongPerformance Performance);

	UCycleSubsystem* GetCycle() const;
	UMaskGameInstance* GetProgression() const;

	// Note handlers for the fallback input bindings.
	void OnNoteLeft()  { PlayNote(EOcarinaNote::Left); }
	void OnNoteRight() { PlayNote(EOcarinaNote::Right); }
	void OnNoteUp()    { PlayNote(EOcarinaNote::Up); }
	void OnNoteDown()  { PlayNote(EOcarinaNote::Down); }
	void OnNoteA()     { PlayNote(EOcarinaNote::A); }

	MaskGame::FSongMatcher Matcher;

	UPROPERTY(VisibleInstanceOnly, Category = "Ocarina")
	bool bDrawn = false;

	/** Seconds since the component began ticking, used to time the performance. */
	double PerformanceClock = 0.0;
};
