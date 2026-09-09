// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"

#include "Core/MaskGameTypes.h"

#include "MaskGameMode.generated.h"

class AMaskCharacter;
class AMaskWorldGenerator;
class UCycleSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCycleRestarted);

/**
 * Runs the cycle.
 *
 * Three things end a cycle: the Hymn of Return, the moon landing, and the player
 * dying. All three come back here, and all three end the same way - progression
 * is banked, the clock goes back to the first dawn, and the region is rebuilt.
 * Keeping that in one place is what stops the three paths from drifting apart.
 */
UCLASS()
class MASKGAME_API AMaskGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMaskGameMode();

	virtual void BeginPlay() override;
	virtual void RestartPlayer(AController* NewPlayer) override;

	/**
	 * Send the world back to the first dawn.
	 *
	 * @param bBankProgress  True for the Hymn of Return and for the moon landing,
	 *                       which both keep what was earned. False is reserved
	 *                       for a debug restart.
	 */
	UFUNCTION(BlueprintCallable, Category = "Cycle")
	void RestartCycle(bool bBankProgress = true);

	UPROPERTY(BlueprintAssignable, Category = "Cycle")
	FOnCycleRestarted OnCycleRestarted;

	/** Greybox generator spawned at startup while there are no authored levels. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World")
	TSubclassOf<AMaskWorldGenerator> WorldGeneratorClass;

	/**
	 * Drives the sky from the clock.
	 *
	 * Always spawned, greybox or not: it adopts a level's existing sky actors
	 * rather than replacing them, and without it the day/night cycle is a number
	 * on the HUD instead of something the player can see out of a window.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World")
	TSubclassOf<class AMaskSkyDirector> SkyDirectorClass;

protected:
	UFUNCTION()
	void HandleMoonFell();

	UFUNCTION()
	void HandleSongPlayed(ESongType Song, ESongPerformance Performance);

	UFUNCTION()
	void HandlePlayerDied();

	/** Offers a song to every listener in the level that is close enough to hear it. */
	int32 BroadcastSongToListeners(ESongType Song, ESongPerformance Performance, AMaskCharacter* Performer);

	/** Hooks the player's ocarina and death delegates. Safe to call more than once. */
	void BindToPlayer(AMaskCharacter* Character);

	UCycleSubsystem* GetCycle() const;
	AMaskCharacter* GetPlayerCharacter() const;

private:
	UPROPERTY()
	TObjectPtr<AMaskWorldGenerator> WorldGenerator;

	UPROPERTY()
	TObjectPtr<class AMaskSkyDirector> SkyDirector;

	/** Guards against a rewind being started twice in the same frame. */
	bool bRestartInProgress = false;
};
