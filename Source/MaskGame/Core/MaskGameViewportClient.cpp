// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

#include "Core/MaskGameViewportClient.h"

#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

#include "MaskGame.h"

int32 UMaskGameViewportClient::GetControllerId(const FInputKeyEventArgs& EventArgs)
{
	return EventArgs.ControllerId;
}

bool UMaskGameViewportClient::IsJoinKey(const FKey& Key)
{
	// Start or the bottom face button, which is what a person picking up a pad
	// presses first without being told.
	return Key == EKeys::Gamepad_FaceButton_Bottom || Key == EKeys::Gamepad_Special_Right;
}

bool UMaskGameViewportClient::IsControllerAssigned(int32 ControllerId) const
{
	const UGameInstance* Instance = GetGameInstance();
	if (Instance == nullptr)
	{
		return false;
	}

	for (const ULocalPlayer* Player : Instance->GetLocalPlayers())
	{
		if (Player != nullptr && Player->GetControllerId() == ControllerId)
		{
			return true;
		}
	}
	return false;
}

APlayerController* UMaskGameViewportClient::AddLocalPlayerForController(int32 ControllerId)
{
	UGameInstance* Instance = GetGameInstance();
	if (Instance == nullptr || Instance->GetWorld() == nullptr)
	{
		return nullptr;
	}

	if (Instance->GetNumLocalPlayers() >= MaxLocalPlayers)
	{
		UE_LOG(LogMaskGame, Log, TEXT("A pad tried to join, but all %d seats are taken."), MaxLocalPlayers);
		return nullptr;
	}

	if (IsControllerAssigned(ControllerId))
	{
		return nullptr;
	}

	APlayerController* Controller = UGameplayStatics::CreatePlayer(Instance->GetWorld(), ControllerId, true);
	if (Controller == nullptr)
	{
		UE_LOG(LogMaskGame, Warning, TEXT("Could not create a local player for controller %d."), ControllerId);
		return nullptr;
	}

	UE_LOG(LogMaskGame, Log, TEXT("Controller %d joined; %d players are now sharing the screen and the cycle."),
		ControllerId, Instance->GetNumLocalPlayers());
	return Controller;
}

bool UMaskGameViewportClient::RemoveLastLocalPlayer()
{
	UGameInstance* Instance = GetGameInstance();
	if (Instance == nullptr || Instance->GetNumLocalPlayers() <= 1)
	{
		// Player one is the session; removing them would leave nobody holding it.
		return false;
	}

	ULocalPlayer* Last = Instance->GetLocalPlayers().Last();
	APlayerController* Controller = Last != nullptr ? Last->GetPlayerController(Instance->GetWorld()) : nullptr;
	if (Controller == nullptr)
	{
		return false;
	}

	UGameplayStatics::RemovePlayer(Controller, /*bDestroyPawn=*/true);
	UE_LOG(LogMaskGame, Log, TEXT("A player left; %d remain."), Instance->GetNumLocalPlayers());
	return true;
}

bool UMaskGameViewportClient::InputKey(const FInputKeyEventArgs& EventArgs)
{
#if PLATFORM_DESKTOP
	// A gamepad button from a pad nobody owns is a request to join. Everything
	// else falls through untouched, including every key from an assigned pad.
	if (bAllowJoinInProgress
		&& EventArgs.Event == IE_Pressed
		&& EventArgs.Key.IsGamepadKey()
		&& IsJoinKey(EventArgs.Key)
		&& !IsControllerAssigned(GetControllerId(EventArgs)))
	{
		if (AddLocalPlayerForController(GetControllerId(EventArgs)) != nullptr)
		{
			return true;
		}
	}
#endif

	return Super::InputKey(EventArgs);
}
