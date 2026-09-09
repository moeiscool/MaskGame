// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Core/MaskGameTypes.h"
#include "World/InteractableInterface.h"
#include "World/SongListenerInterface.h"

#include "TempleGate.generated.h"

class UStaticMeshComponent;

/**
 * The way into a temple.
 *
 * Sealed until the right song is played in front of it, and then only passable
 * in the form the temple was built for. Once opened it stays open for the rest
 * of the cycle but seals again at the next dawn, because the song is part of
 * the route, not a switch that has been thrown for good.
 */
UCLASS()
class MASKGAME_API ATempleGate : public AActor, public IInteractableInterface, public ISongListenerInterface
{
	GENERATED_BODY()

public:
	ATempleGate();

	virtual void BeginPlay() override;

	// ISongListenerInterface.
	virtual bool OnSongHeard_Implementation(ESongType Song, ESongPerformance Performance, AMaskCharacter* Performer) override;
	virtual float GetHearingRadius_Implementation() const override;

	// IInteractableInterface.
	virtual bool CanInteract_Implementation(AMaskCharacter* Character) const override;
	virtual void Interact_Implementation(AMaskCharacter* Character) override;
	virtual FText GetInteractionPrompt_Implementation() const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gate")
	FName DungeonId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gate")
	FText DisplayName;

	/** Song that opens this gate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gate")
	ESongType RequiredSong = ESongType::SonataOfRousing;

	/** Form the temple beyond is built around; the player is warned when they arrive as someone else. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gate")
	EMaskForm RequiredForm = EMaskForm::Traveller;

	UFUNCTION(BlueprintPure, Category = "Gate")
	bool IsOpen() const { return bOpen; }

	UFUNCTION(BlueprintCallable, Category = "Gate")
	void Open();

	UFUNCTION(BlueprintCallable, Category = "Gate")
	void Close();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

private:
	UPROPERTY(VisibleInstanceOnly, Category = "Gate")
	bool bOpen = false;
};
