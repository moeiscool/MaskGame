// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

#include "Character/MaskCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"

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
	OnDied.Broadcast();
}

void AMaskCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	check(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AMaskCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AMaskCharacter::MoveRight);
	PlayerInputComponent->BindAxis(TEXT("Turn"), this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &APawn::AddControllerPitchInput);

	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Released, this, &ACharacter::StopJumping);
	PlayerInputComponent->BindAction(TEXT("Interact"), IE_Pressed, this, &AMaskCharacter::OnInteractPressed);
	PlayerInputComponent->BindAction(TEXT("Attack"), IE_Pressed, this, &AMaskCharacter::OnAttackPressed);
	PlayerInputComponent->BindAction(TEXT("FormAction"), IE_Pressed, this, &AMaskCharacter::StartFormAction);
	PlayerInputComponent->BindAction(TEXT("FormAction"), IE_Released, this, &AMaskCharacter::StopFormAction);
	PlayerInputComponent->BindAction(TEXT("RemoveMask"), IE_Pressed, this, &AMaskCharacter::OnRemoveMaskPressed);

	PlayerInputComponent->BindAction(TEXT("MaskSlot1"), IE_Pressed, this, &AMaskCharacter::OnQuickSlot1);
	PlayerInputComponent->BindAction(TEXT("MaskSlot2"), IE_Pressed, this, &AMaskCharacter::OnQuickSlot2);
	PlayerInputComponent->BindAction(TEXT("MaskSlot3"), IE_Pressed, this, &AMaskCharacter::OnQuickSlot3);
	PlayerInputComponent->BindAction(TEXT("MaskSlot4"), IE_Pressed, this, &AMaskCharacter::OnQuickSlot4);

	if (Ocarina != nullptr)
	{
		Ocarina->BindInput(*PlayerInputComponent);
	}
}

void AMaskCharacter::MoveForward(float Value)
{
	if (Controller == nullptr || FMath::IsNearlyZero(Value))
	{
		return;
	}

	const FRotator YawOnly(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
	AddMovementInput(FRotationMatrix(YawOnly).GetUnitAxis(EAxis::X), Value);
}

void AMaskCharacter::MoveRight(float Value)
{
	if (Controller == nullptr || FMath::IsNearlyZero(Value))
	{
		return;
	}

	const FRotator YawOnly(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
	AddMovementInput(FRotationMatrix(YawOnly).GetUnitAxis(EAxis::Y), Value);
}

void AMaskCharacter::OnInteractPressed()
{
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

void AMaskCharacter::OnAttackPressed()
{
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

void AMaskCharacter::OnRemoveMaskPressed()
{
	if (Masks != nullptr)
	{
		Masks->RemoveMask();
	}
}

void AMaskCharacter::EquipSlot(int32 SlotIndex)
{
	if (Masks != nullptr)
	{
		Masks->EquipQuickSlot(SlotIndex);
	}
}
