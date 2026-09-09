// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

#include "Core/MaskGameTypes.h"

#include "MaskEnemy.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemyDefeated, AMaskEnemy*, Enemy);

/**
 * A thing that fights back.
 *
 * One class covers both the field enemies and the temple guardians: a boss is
 * an enemy that has been told which echo it is holding, has more health, and
 * lets the wrath mask come out while the player is in its arena. Splitting them
 * into two hierarchies would duplicate all of the damage handling to express a
 * difference that is really just data.
 */
UCLASS()
class MASKGAME_API AMaskEnemy : public ACharacter
{
	GENERATED_BODY()

public:
	AMaskEnemy();

	virtual void BeginPlay() override;
	virtual float TakeDamage(float Damage, const FDamageEvent& DamageEvent, AController* Instigator, AActor* Causer) override;

	/** Promote this enemy to the guardian of a temple. */
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void MakeBoss(EEchoType InEcho, const FText& InDisplayName);

	UFUNCTION(BlueprintPure, Category = "Enemy")
	bool IsBoss() const { return Echo != EEchoType::None; }

	UFUNCTION(BlueprintPure, Category = "Enemy")
	bool IsAlive() const { return Health > 0.0f; }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (ClampMin = "1.0"))
	float MaxHealth = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
	FText DisplayName;

	/** Echo freed when this enemy dies. None for anything that is not a guardian. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
	EEchoType Echo = EEchoType::None;

	/** Radius within which a boss lets the wrath mask be worn. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (ClampMin = "0.0"))
	float ArenaRadius = 2000.0f;

	UPROPERTY(BlueprintAssignable, Category = "Enemy")
	FOnEnemyDefeated OnDefeated;

protected:
	/** Tells the player's mask inventory whether they are standing in this arena. */
	UFUNCTION()
	void HandleArenaOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Sweep);

	UFUNCTION()
	void HandleArenaOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex);

	void Die();

	/** Overlap volume that marks out a boss's arena. Disabled for field enemies. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class USphereComponent> ArenaVolume;

private:
	UPROPERTY(VisibleInstanceOnly, Category = "Enemy")
	float Health = 3.0f;
};
