// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameViewportClient.h"

#include "MaskGameViewportClient.generated.h"

/**
 * Lets extra controllers join a local split-screen game.
 *
 * This has to be the viewport client rather than a player controller, because
 * input from a pad with no player assigned to it does not reach any player
 * controller - the viewport is the last place it is still visible. Pressing a
 * face button on an unassigned pad creates a local player for it, and the
 * engine splits the screen automatically once there is more than one.
 *
 * Joining is by button press rather than by plugging a pad in, so a controller
 * left connected on a desk does not silently halve somebody's screen.
 *
 * Desktop only. On phones and tablets there is one screen, one pair of thumbs
 * and no second pad, so the whole path is compiled out.
 */
UCLASS()
class MASKGAME_API UMaskGameViewportClient : public UGameViewportClient
{
	GENERATED_BODY()

public:
	virtual bool InputKey(const FInputKeyEventArgs& EventArgs) override;

	/** Most local players allowed at once. The engine's split layouts stop at four. */
	UPROPERTY(EditDefaultsOnly, Category = "Split Screen", meta = (ClampMin = "1", ClampMax = "4"))
	int32 MaxLocalPlayers = 4;

	/** Whether an unassigned pad may join by pressing a button. */
	UPROPERTY(EditDefaultsOnly, Category = "Split Screen")
	bool bAllowJoinInProgress = true;

	/** Add a local player for this controller. Returns null when it cannot. */
	UFUNCTION(BlueprintCallable, Category = "Split Screen")
	APlayerController* AddLocalPlayerForController(int32 ControllerId);

	/** Drop the most recently added local player. Never removes player one. */
	UFUNCTION(BlueprintCallable, Category = "Split Screen")
	bool RemoveLastLocalPlayer();

	/** True when some local player already owns this controller. */
	UFUNCTION(BlueprintPure, Category = "Split Screen")
	bool IsControllerAssigned(int32 ControllerId) const;

private:
	/** True for the buttons an unassigned pad may join with. */
	static bool IsJoinKey(const FKey& Key);

	/**
	 * Which controller an input event came from.
	 *
	 * Isolated here because this is the one piece of engine API in the file that
	 * has moved between versions: Unreal is midway through replacing the plain
	 * controller index with FInputDeviceId and FPlatformUserId. If a future
	 * engine drops FInputKeyEventArgs::ControllerId, this function is the only
	 * thing that needs rewriting.
	 */
	static int32 GetControllerId(const FInputKeyEventArgs& EventArgs);
};
