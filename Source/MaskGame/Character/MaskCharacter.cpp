// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

#include "Character/MaskCharacter.h"

#include "Camera/CameraComponent.h"
#include "EngineUtils.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"

#include "AI/MaskEnemy.h"
#include "Core/MaskGameInstance.h"
#include "MaskGame.h"
#include "MaskRules.h"
#include "Masks/MaskInventoryComponent.h"
#include "Songs/OcarinaComponent.h"
#include "World/InteractableInterface.h"

AMaskCharacter::AMaskCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = true;
	Movement->RotationRate = FRotator(0.0f, 640.0f, 0.0f);
	Movement->AirControl = 0.4f;
	Movement->BrakingDecelerationWalking = 2200.0f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 480.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	Masks = CreateDefaultSubobject<UMaskInventoryComponent>(TEXT("Masks"));
	Ocarina = CreateDefaultSubobject<UOcarinaComponent>(TEXT("Ocarina"));
}

void AMaskCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (Masks != nullptr)
	{
		Masks->OnFormChanged.AddDynamic(this, &AMaskCharacter::HandleFormChanged);
		Masks->OnWornMaskChanged.AddDynamic(this, &AMaskCharacter::HandleWornMaskChanged);
	}

	Health = GetMaxHealth();

	// Magic capacity is permanent progression, so a pawn respawned into a later
	// cycle picks the meter back up rather than losing the forms' abilities.
	if (const UMaskGameInstance* Instance = UMaskGameInstance::Get(this))
	{
		MaxMagic = static_cast<float>(Instance->GetProgression().Equipment().GetMaxMagic());
		Magic = MaxMagic;
	}

	ApplyFormTraits();
	OnHealthChanged.Broadcast(Health, GetMaxHealth());
	OnMagicChanged.Broadcast(Magic, MaxMagic);
}

EMaskForm AMaskCharacter::GetForm() const
{
	return Masks != nullptr ? Masks->GetCurrentForm() : EMaskForm::Traveller;
}

float AMaskCharacter::GetMaxHealth() const
{
	const UMaskGameInstance* Instance = UMaskGameInstance::Get(this);
	return Instance != nullptr ? static_cast<float>(Instance->GetMaxHearts()) : 3.0f;
}

void AMaskCharacter::ApplyFormTraits()
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (Movement == nullptr)
	{
		return;
	}

	const MaskGame::FFormTraits& Traits = MaskGame::GetFormTraits(static_cast<MaskGame::EForm>(GetForm()));
	const float MaskMultiplier = Masks != nullptr ? Masks->GetWornSpeedMultiplier() : 1.0f;

	Movement->MaxWalkSpeed = Traits.WalkSpeed * MaskMultiplier;
	Movement->JumpZVelocity = Traits.JumpVelocity;

	// A form that cannot swim has no business floating: the boulderkin walks the
	// bottom, and the sapling skips across the surface until its hops run out.
	Movement->MaxSwimSpeed = Traits.bCanSwim ? Traits.WalkSpeed : 0.0f;
	Movement->Buoyancy = Traits.bSinksInWater ? 0.0f : 1.0f;
	Movement->NavAgentProps.bCanSwim = Traits.bCanSwim;
}

void AMaskCharacter::HandleFormChanged(EMaskForm OldForm, EMaskForm NewForm)
{
	ApplyFormTraits();

	// Hearts are a property of the traveller, not of the body worn over them, so
	// a transformation neither heals nor hurts; it only re-clamps against the cap.
	Health = FMath::Clamp(Health, 0.0f, GetMaxHealth());
	OnHealthChanged.Broadcast(Health, GetMaxHealth());

	UE_LOG(LogMaskGame, Verbose, TEXT("%s became %s."),
		*UEnum::GetDisplayValueAsText(OldForm).ToString(),
		*UEnum::GetDisplayValueAsText(NewForm).ToString());
}

void AMaskCharacter::HandleWornMaskChanged(EMaskType /*Mask*/)
{
	ApplyFormTraits();
}

void AMaskCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateLockOn(DeltaSeconds);

	const MaskGame::FFormTraits& Traits = MaskGame::GetFormTraits(static_cast<MaskGame::EForm>(GetForm()));

	if (bFormActionActive && Traits.MagicDrainPerSecond > 0.0f)
	{
		if (!ConsumeMagic(Traits.MagicDrainPerSecond * DeltaSeconds))
		{
			StopFormAction();
		}
	}
	else if (!bFormActionActive && MaxMagic > 0.0f && Magic < MaxMagic)
	{
		RestoreMagic(MagicRegenPerSecond * DeltaSeconds);
	}
}

float AMaskCharacter::TakeDamage(float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float Base = Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
	if (Base <= 0.0f || !IsAlive())
	{
		return 0.0f;
	}

	const MaskGame::FFormTraits& Traits = MaskGame::GetFormTraits(static_cast<MaskGame::EForm>(GetForm()));
	const float Applied = Base * Traits.DamageTakenScale;

	Health = FMath::Max(0.0f, Health - Applied);
	OnHealthChanged.Broadcast(Health, GetMaxHealth());

	if (!IsAlive())
	{
		Die();
	}
	return Applied;
}

void AMaskCharacter::Heal(float Amount)
{
	if (Amount <= 0.0f || !IsAlive())
	{
		return;
	}

	Health = FMath::Min(GetMaxHealth(), Health + Amount);
	OnHealthChanged.Broadcast(Health, GetMaxHealth());
}

void AMaskCharacter::RestoreFully()
{
	Health = GetMaxHealth();
	Magic = MaxMagic;
	OnHealthChanged.Broadcast(Health, GetMaxHealth());
	OnMagicChanged.Broadcast(Magic, MaxMagic);
}

bool AMaskCharacter::ConsumeMagic(float Amount)
{
	if (Amount <= 0.0f)
	{
		return true;
	}
	if (Magic < Amount)
	{
		return false;
	}

	Magic -= Amount;
	OnMagicChanged.Broadcast(Magic, MaxMagic);
	return true;
}

void AMaskCharacter::RestoreMagic(float Amount)
{
	if (Amount <= 0.0f || MaxMagic <= 0.0f)
	{
		return;
	}

	Magic = FMath::Min(MaxMagic, Magic + Amount);
	OnMagicChanged.Broadcast(Magic, MaxMagic);
}

void AMaskCharacter::GrantMagicMeter(float NewMaxMagic)
{
	MaxMagic = FMath::Max(MaxMagic, NewMaxMagic);
	Magic = MaxMagic;
	OnMagicChanged.Broadcast(Magic, MaxMagic);
}

void AMaskCharacter::StartFormAction()
{
	if (IsOcarinaDrawn())
	{
		return;
	}

	const MaskGame::FFormTraits& Traits = MaskGame::GetFormTraits(static_cast<MaskGame::EForm>(GetForm()));

	// A form whose action costs magic simply cannot start it on an empty meter.
	if (Traits.MagicDrainPerSecond > 0.0f && Magic <= 0.0f)
	{
		return;
	}

	bFormActionActive = true;

	// The boulderkin's roll is the one action that changes how the body moves
	// rather than spawning something, so it is applied here directly.
	if (GetForm() == EMaskForm::Boulderkin)
	{
		GetCharacterMovement()->MaxWalkSpeed = Traits.SprintSpeed;
	}
}

void AMaskCharacter::StopFormAction()
{
	if (!bFormActionActive)
	{
		return;
	}

	bFormActionActive = false;
	ApplyFormTraits();
}

void AMaskCharacter::Die()
{
	UE_LOG(LogMaskGame, Log, TEXT("The traveller has fallen."));
	bFormActionActive = false;
	ClearLockOn();
	OnDied.Broadcast();
}

void AMaskCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	check(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AMaskCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AMaskCharacter::MoveRight);
	PlayerInputComponent->BindAxis(TEXT("Turn"), this, &AMaskCharacter::MouseTurn);
	PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &AMaskCharacter::MouseLookUp);
	PlayerInputComponent->BindAxis(TEXT("TurnRate"), this, &AMaskCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis(TEXT("LookUpRate"), this, &AMaskCharacter::LookUpAtRate);

	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &AMaskCharacter::RequestJump);
	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Released, this, &AMaskCharacter::RequestStopJump);
	PlayerInputComponent->BindAction(TEXT("Interact"), IE_Pressed, this, &AMaskCharacter::TryInteract);
	PlayerInputComponent->BindAction(TEXT("Attack"), IE_Pressed, this, &AMaskCharacter::PerformAttack);
	PlayerInputComponent->BindAction(TEXT("FormAction"), IE_Pressed, this, &AMaskCharacter::StartFormAction);
	PlayerInputComponent->BindAction(TEXT("FormAction"), IE_Released, this, &AMaskCharacter::StopFormAction);
	PlayerInputComponent->BindAction(TEXT("LockOn"), IE_Pressed, this, &AMaskCharacter::ToggleLockOn);
	PlayerInputComponent->BindAction(TEXT("RemoveMask"), IE_Pressed, this, &AMaskCharacter::RemoveMask);

	PlayerInputComponent->BindAction(TEXT("MaskSlot1"), IE_Pressed, this, &AMaskCharacter::OnQuickSlot1);
	PlayerInputComponent->BindAction(TEXT("MaskSlot2"), IE_Pressed, this, &AMaskCharacter::OnQuickSlot2);
	PlayerInputComponent->BindAction(TEXT("MaskSlot3"), IE_Pressed, this, &AMaskCharacter::OnQuickSlot3);
	PlayerInputComponent->BindAction(TEXT("MaskSlot4"), IE_Pressed, this, &AMaskCharacter::OnQuickSlot4);

	if (Ocarina != nullptr)
	{
		Ocarina->BindInput(*PlayerInputComponent);
	}
}

bool AMaskCharacter::IsOcarinaDrawn() const
{
	return Ocarina != nullptr && Ocarina->IsDrawn();
}

void AMaskCharacter::ApplyMoveInput(float Forward, float Right)
{
	if (Controller == nullptr || IsOcarinaDrawn())
	{
		return;
	}

	// Movement is relative to where the camera is pointing, not where the
	// character is facing, so that walking left means left on the screen.
	const FRotator YawOnly(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
	const FRotationMatrix Frame(YawOnly);

	if (!FMath::IsNearlyZero(Forward))
	{
		AddMovementInput(Frame.GetUnitAxis(EAxis::X), Forward);
	}
	if (!FMath::IsNearlyZero(Right))
	{
		AddMovementInput(Frame.GetUnitAxis(EAxis::Y), Right);
	}
}

void AMaskCharacter::ApplyLookInput(float YawDelta, float PitchDelta)
{
	// A lock holds the camera itself, so manual look is ignored rather than
	// fighting the interpolation in UpdateLockOn.
	if (Controller == nullptr || IsOcarinaDrawn() || IsLockedOn())
	{
		return;
	}

	AddControllerYawInput(YawDelta);
	AddControllerPitchInput(PitchDelta);
}

void AMaskCharacter::RequestJump()
{
	// The jump button is also the A note, and the d-pad is also the mask slots.
	// Rather than have the two fight over a press, drawing the ocarina puts the
	// character's own controls away for as long as it is out.
	if (!IsOcarinaDrawn())
	{
		Jump();
	}
}

void AMaskCharacter::RequestStopJump()
{
	StopJumping();
}

void AMaskCharacter::RemoveMask()
{
	if (!IsOcarinaDrawn() && Masks != nullptr)
	{
		Masks->RemoveMask();
	}
}

bool AMaskCharacter::EquipMaskSlot(int32 SlotIndex)
{
	return !IsOcarinaDrawn() && Masks != nullptr && Masks->EquipQuickSlot(SlotIndex);
}

void AMaskCharacter::ToggleOcarina()
{
	if (Ocarina != nullptr)
	{
		Ocarina->ToggleDrawn();
	}
}

void AMaskCharacter::PlayOcarinaNote(EOcarinaNote Note)
{
	if (Ocarina != nullptr)
	{
		Ocarina->PlayNote(Note);
	}
}

void AMaskCharacter::TryInteract()
{
	if (IsOcarinaDrawn())
	{
		return;
	}

	// A short sphere sweep ahead of the player, taking the nearest thing that
	// says it can be interacted with in this form.
	const FVector Start = GetActorLocation();
	const FVector End = Start + GetActorForwardVector() * InteractRange;

	TArray<FHitResult> Hits;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MaskInteract), false, this);
	GetWorld()->SweepMultiByChannel(Hits, Start, End, FQuat::Identity, ECC_Visibility,
		FCollisionShape::MakeSphere(64.0f), Params);

	for (const FHitResult& Hit : Hits)
	{
		AActor* Actor = Hit.GetActor();
		if (Actor != nullptr && Actor->Implements<UInteractableInterface>())
		{
			if (IInteractableInterface::Execute_CanInteract(Actor, this))
			{
				IInteractableInterface::Execute_Interact(Actor, this);
				return;
			}
		}
	}
}

void AMaskCharacter::PerformAttack()
{
	if (IsOcarinaDrawn())
	{
		return;
	}

	const MaskGame::FFormTraits& Traits = MaskGame::GetFormTraits(static_cast<MaskGame::EForm>(GetForm()));
	if (!Traits.bCanUseSword)
	{
		// Formless bodies fall back on their own action rather than swinging air.
		StartFormAction();
		StopFormAction();
		return;
	}

	const FVector Start = GetActorLocation();
	const FVector End = Start + GetActorForwardVector() * 180.0f;

	TArray<FHitResult> Hits;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MaskAttack), false, this);
	GetWorld()->SweepMultiByChannel(Hits, Start, End, FQuat::Identity, ECC_Pawn,
		FCollisionShape::MakeSphere(90.0f), Params);

	const float Damage = 1.0f * Traits.DamageDealtScale;
	for (const FHitResult& Hit : Hits)
	{
		if (AActor* Actor = Hit.GetActor(); Actor != nullptr && Actor != this)
		{
			UGameplayStatics::ApplyDamage(Actor, Damage, GetController(), this, nullptr);
		}
	}
}

// ---------------------------------------------------------------------------
// Input translation
// ---------------------------------------------------------------------------

void AMaskCharacter::MoveForward(float Value)
{
	ApplyMoveInput(Value, 0.0f);
}

void AMaskCharacter::MoveRight(float Value)
{
	ApplyMoveInput(0.0f, Value);
}

void AMaskCharacter::TurnAtRate(float Value)
{
	if (FMath::IsNearlyZero(Value))
	{
		return;
	}

	// Scaled by the frame time so the camera turns at the same speed whatever
	// the frame rate, unlike a mouse delta which already accounts for it.
	ApplyLookInput(Value * GamepadTurnRate * GetWorld()->GetDeltaSeconds(), 0.0f);
}

void AMaskCharacter::MouseTurn(float Value)
{
	ApplyLookInput(Value, 0.0f);
}

void AMaskCharacter::MouseLookUp(float Value)
{
	ApplyLookInput(0.0f, Value);
}

void AMaskCharacter::LookUpAtRate(float Value)
{
	if (FMath::IsNearlyZero(Value))
	{
		return;
	}

	const float Direction = bInvertGamepadLookY ? 1.0f : -1.0f;
	ApplyLookInput(0.0f, Value * Direction * GamepadLookUpRate * GetWorld()->GetDeltaSeconds());
}

// ---------------------------------------------------------------------------
// Lock-on
// ---------------------------------------------------------------------------

void AMaskCharacter::ToggleLockOn()
{
	if (IsOcarinaDrawn())
	{
		return;
	}

	if (IsLockedOn())
	{
		ClearLockOn();
		return;
	}

	AActor* Target = FindLockOnTarget();
	if (Target == nullptr)
	{
		return;
	}

	LockOnTarget = Target;
	ApplyLockOnRotationMode();
	UE_LOG(LogMaskGame, Verbose, TEXT("Locked on to %s."), *Target->GetName());
}

void AMaskCharacter::ClearLockOn()
{
	LockOnTarget.Reset();
	ApplyLockOnRotationMode();
}

AActor* AMaskCharacter::FindLockOnTarget() const
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return nullptr;
	}

	const FVector Origin = GetActorLocation();
	const FVector Facing = GetActorForwardVector();

	AActor* Best = nullptr;
	float BestScore = -1.0f;

	for (TActorIterator<AMaskEnemy> It(World); It; ++It)
	{
		AMaskEnemy* Enemy = *It;
		if (Enemy == nullptr || !Enemy->IsAlive())
		{
			continue;
		}

		const FVector ToEnemy = Enemy->GetActorLocation() - Origin;
		const float Distance = ToEnemy.Size();
		if (Distance > LockOnRange || Distance < KINDA_SMALL_NUMBER)
		{
			continue;
		}

		// Prefer what the player is already looking at over what merely happens
		// to be closest, so a lock does not jump to something behind them.
		const float Alignment = FVector::DotProduct(Facing, ToEnemy / Distance);
		if (Alignment <= 0.0f)
		{
			continue;
		}

		const float Score = Alignment * (1.0f - Distance / LockOnRange);
		if (Score > BestScore)
		{
			BestScore = Score;
			Best = Enemy;
		}
	}

	return Best;
}

void AMaskCharacter::UpdateLockOn(float DeltaSeconds)
{
	if (!LockOnTarget.IsValid())
	{
		// The target was destroyed rather than released - a guardian died, or the
		// world was rebuilt by a rewind. bUseControllerRotationYaw is the tell
		// that the camera is still configured for a lock that no longer exists.
		if (bUseControllerRotationYaw)
		{
			ApplyLockOnRotationMode();
		}
		return;
	}

	AActor* Target = LockOnTarget.Get();

	// A guardian that dies mid-swing, or one the player has run away from.
	const AMaskEnemy* Enemy = Cast<AMaskEnemy>(Target);
	const float Distance = FVector::Dist(GetActorLocation(), Target->GetActorLocation());
	if ((Enemy != nullptr && !Enemy->IsAlive()) || Distance > LockOnBreakRange)
	{
		ClearLockOn();
		return;
	}

	AController* OwningController = GetController();
	if (OwningController == nullptr)
	{
		return;
	}

	// Swing the camera round to hold the target rather than snapping to it, so
	// the lock reads as the camera moving rather than the world jumping.
	const FRotator Desired = (Target->GetActorLocation() - GetActorLocation()).Rotation();
	const FRotator Current = OwningController->GetControlRotation();
	OwningController->SetControlRotation(
		FMath::RInterpConstantTo(Current, Desired, DeltaSeconds, LockOnCameraSpeed));
}

void AMaskCharacter::ApplyLockOnRotationMode()
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (Movement == nullptr)
	{
		return;
	}

	// Unlocked, the character turns to face wherever it is walking. Locked, it
	// keeps facing the target and circles it, which is what lets the left stick
	// strafe while the right thumb does nothing.
	const bool bLocked = IsLockedOn();
	Movement->bOrientRotationToMovement = !bLocked;
	bUseControllerRotationYaw = bLocked;
}
