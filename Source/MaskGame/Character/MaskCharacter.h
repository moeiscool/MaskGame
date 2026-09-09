// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

#include "Core/MaskGameTypes.h"

#include "MaskCharacter.generated.h"

class UCameraComponent;
class UMaskInventoryComponent;
class UOcarinaComponent;
class USpringArmComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChanged, float, Health, float, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMagicChanged, float, Magic, float, MaxMagic);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCharacterDied);

/**
 * The player.
 *
 * Wears five bodies over the course of the game and takes all of its movement
 * numbers from MaskGame::FFormTraits, so a form's feel is a data change in
 * MaskRules.cpp rather than a rewrite here. Health is measured in hearts, one
 * unit per container.
 */
UCLASS()
class MASKGAME_API AMaskCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AMaskCharacter();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual float TakeDamage(float Damage, const FDamageEvent& DamageEvent, AController* Instigator, AActor* Causer) override;

	UFUNCTION(BlueprintPure, Category = "Character")
	UMaskInventoryComponent* GetMasks() const { return Masks; }

	UFUNCTION(BlueprintPure, Category = "Character")
	UOcarinaComponent* GetOcarina() const { return Ocarina; }

	UFUNCTION(BlueprintPure, Category = "Character")
	EMaskForm GetForm() const;

	// ---- Health. ----

	UFUNCTION(BlueprintPure, Category = "Character|Health")
	float GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category = "Character|Health")
	float GetMaxHealth() const;

	UFUNCTION(BlueprintCallable, Category = "Character|Health")
	void Heal(float Amount);

	/** Refill hearts and magic, as a fairy fountain or a new cycle would. */
	UFUNCTION(BlueprintCallable, Category = "Character|Health")
	void RestoreFully();

	UFUNCTION(BlueprintPure, Category = "Character|Health")
	bool IsAlive() const { return Health > 0.0f; }

	// ---- Magic. ----

	UFUNCTION(BlueprintPure, Category = "Character|Magic")
	float GetMagic() const { return Magic; }

	UFUNCTION(BlueprintPure, Category = "Character|Magic")
	float GetMaxMagic() const { return MaxMagic; }

	/** Spend magic. Returns false and spends nothing when the meter is short. */
	UFUNCTION(BlueprintCallable, Category = "Character|Magic")
	bool ConsumeMagic(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Character|Magic")
	void RestoreMagic(float Amount);

	/** Unlocks the meter; the traveller has none until a fairy grants it. */
	UFUNCTION(BlueprintCallable, Category = "Character|Magic")
	void GrantMagicMeter(float NewMaxMagic);

	// ---- Actions. ----

	/** The form's special: a flower launch, a rolling charge, a barrier, a sword swing. */
	UFUNCTION(BlueprintCallable, Category = "Character|Actions")
	void StartFormAction();

	UFUNCTION(BlueprintCallable, Category = "Character|Actions")
	void StopFormAction();

	UFUNCTION(BlueprintPure, Category = "Character|Actions")
	bool IsFormActionActive() const { return bFormActionActive; }

	// ---- Lock-on. ----

	/**
	 * Fix the camera on the nearest enemy, or let go of the one already held.
	 *
	 * While locked the character faces its target and strafes around it instead
	 * of turning to face where it is walking, which is what makes a stick usable
	 * in a fight: the right thumb is free and the left one circles.
	 */
	UFUNCTION(BlueprintCallable, Category = "Character|Lock-On")
	void ToggleLockOn();

	UFUNCTION(BlueprintCallable, Category = "Character|Lock-On")
	void ClearLockOn();

	UFUNCTION(BlueprintPure, Category = "Character|Lock-On")
	AActor* GetLockOnTarget() const { return LockOnTarget.Get(); }

	UFUNCTION(BlueprintPure, Category = "Character|Lock-On")
	bool IsLockedOn() const { return LockOnTarget.IsValid(); }

	/** True while the ocarina is out, when the character's own controls are put away. */
	UFUNCTION(BlueprintPure, Category = "Character")
	bool IsOcarinaDrawn() const;

	UPROPERTY(BlueprintAssignable, Category = "Character")
	FOnHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Character")
	FOnMagicChanged OnMagicChanged;

	UPROPERTY(BlueprintAssignable, Category = "Character")
	FOnCharacterDied OnDied;

protected:
	/** Re-tunes movement whenever the body changes. */
	UFUNCTION()
	void HandleFormChanged(EMaskForm OldForm, EMaskForm NewForm);

	/** Speed changes when a mask like the hare hood goes on or comes off. */
	UFUNCTION()
	void HandleWornMaskChanged(EMaskType Mask);

	/** Pushes the current form's traits into the movement component. */
	void ApplyFormTraits();

	void Die();

	// Input handlers, bound to the fallback axis and action mappings in
	// Config/DefaultInput.ini so the greybox is playable before any Enhanced
	// Input assets exist.
	void MoveForward(float Value);
	void MoveRight(float Value);

	/**
	 * Stick look.
	 *
	 * A mouse reports how far it moved since the last frame, so its value is
	 * already a delta and goes straight to the controller. A stick reports how
	 * far it is being held, so it has to be multiplied by a rate and by the
	 * frame time or the camera speed would depend on the frame rate.
	 */
	void TurnAtRate(float Value);
	void LookUpAtRate(float Value);

	void OnJumpPressed();
	void OnJumpReleased();
	void OnInteractPressed();
	void OnAttackPressed();
	void OnRemoveMaskPressed();
	void OnQuickSlot1() { EquipSlot(1); }
	void OnQuickSlot2() { EquipSlot(2); }
	void OnQuickSlot3() { EquipSlot(3); }
	void OnQuickSlot4() { EquipSlot(4); }
	void EquipSlot(int32 SlotIndex);

	/** Nearest living enemy in front of the player within LockOnRange, or null. */
	AActor* FindLockOnTarget() const;

	/** Drops the target when it dies, is destroyed, or walks out of range. */
	void UpdateLockOn(float DeltaSeconds);

	/** Switches between facing-where-you-walk and facing-the-target. */
	void ApplyLockOnRotationMode();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UMaskInventoryComponent> Masks;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UOcarinaComponent> Ocarina;

	/** How far the player can reach to talk, read or pick something up. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character", meta = (ClampMin = "0.0"))
	float InteractRange = 220.0f;

	/** Magic refilled per second while the form's action is not being held. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Magic", meta = (ClampMin = "0.0"))
	float MagicRegenPerSecond = 2.0f;

	/** Degrees per second the right stick turns the camera at full deflection. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Input", meta = (ClampMin = "0.0"))
	float GamepadTurnRate = 150.0f;

	/** Degrees per second the right stick pitches the camera at full deflection. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Input", meta = (ClampMin = "0.0"))
	float GamepadLookUpRate = 110.0f;

	/** Push the right stick up to look up, rather than down. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Character|Input")
	bool bInvertGamepadLookY = false;

	/** How far away an enemy can be and still be locked onto. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Lock-On", meta = (ClampMin = "0.0"))
	float LockOnRange = 1600.0f;

	/** How far a locked target may get before the lock breaks. Wider than LockOnRange so it does not flicker. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Lock-On", meta = (ClampMin = "0.0"))
	float LockOnBreakRange = 2400.0f;

	/** Degrees per second the camera swings round to hold a locked target. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Lock-On", meta = (ClampMin = "0.0"))
	float LockOnCameraSpeed = 360.0f;

private:
	UPROPERTY(VisibleInstanceOnly, Category = "Character")
	float Health = 3.0f;

	UPROPERTY(VisibleInstanceOnly, Category = "Character")
	float Magic = 0.0f;

	UPROPERTY(VisibleInstanceOnly, Category = "Character")
	float MaxMagic = 0.0f;

	UPROPERTY(VisibleInstanceOnly, Category = "Character")
	bool bFormActionActive = false;

	/** Weak, so a target destroyed mid-fight simply reads as gone rather than dangling. */
	TWeakObjectPtr<AActor> LockOnTarget;
};
