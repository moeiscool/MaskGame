// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "Core/MaskGameTypes.h"

#include "SongListenerInterface.generated.h"

class AMaskCharacter;

UINTERFACE(MinimalAPI, Blueprintable)
class USongListenerInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Anything in the world that answers the ocarina: a temple that rises out of the
 * mire, a guard who falls asleep, a hollow statue left behind where the player
 * stood.
 *
 * The game mode walks every listener in the level when a song is played rather
 * than each actor subscribing, so an actor spawned mid-cycle needs no wiring up.
 */
class MASKGAME_API ISongListenerInterface
{
	GENERATED_BODY()

public:
	/**
	 * A song was played nearby.
	 * @return true when this actor did something, so the game mode can report
	 *         that the song had an effect rather than falling on deaf ears.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Songs")
	bool OnSongHeard(ESongType Song, ESongPerformance Performance, AMaskCharacter* Performer);

	/** How far from the player this actor can hear, in centimetres. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Songs")
	float GetHearingRadius() const;
};
