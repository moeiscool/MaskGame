// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"

#include "Core/MaskGameTypes.h"
#include "ProgressionState.h"
#include "ScheduleTable.h"

#include "MaskGameInstance.generated.h"

class UMaskSaveGame;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMaskAcquired, EMaskType, Mask);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSongLearned, ESongType, Song);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEchoFreed, EEchoType, Echo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFlagChanged, FName, Flag, int32, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHeartsChanged, int32, MaxHearts);

/**
 * Owner of the player's progression for the whole session.
 *
 * The progression outlives both the cycle and the level, so it belongs here
 * rather than on the game state: rewinding to the first dawn or travelling to
 * another region must not lose a mask. Every mutation goes through this class
 * so that there is one place that broadcasts, one place that decides what a
 * rewind keeps, and one place that writes the save.
 */
UCLASS()
class MASKGAME_API UMaskGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;

	/** Convenience accessor. Returns null outside a game world. */
	static UMaskGameInstance* Get(const UObject* WorldContextObject);

	// ---- Masks, songs and echoes. ----

	UFUNCTION(BlueprintPure, Category = "Progression|Masks")
	bool HasMask(EMaskType Mask) const;

	/** Grants a mask. Returns false when it was already owned. */
	UFUNCTION(BlueprintCallable, Category = "Progression|Masks")
	bool GiveMask(EMaskType Mask);

	UFUNCTION(BlueprintPure, Category = "Progression|Masks")
	int32 GetMaskCount() const { return static_cast<int32>(Progression.GetMaskCount()); }

	UFUNCTION(BlueprintPure, Category = "Progression|Masks")
	bool HasEveryMask() const { return Progression.HasEveryMask(); }

	UFUNCTION(BlueprintPure, Category = "Progression|Songs")
	bool HasSong(ESongType Song) const;

	UFUNCTION(BlueprintCallable, Category = "Progression|Songs")
	bool LearnSong(ESongType Song);

	UFUNCTION(BlueprintPure, Category = "Progression|Echoes")
	bool HasEcho(EEchoType Echo) const;

	UFUNCTION(BlueprintCallable, Category = "Progression|Echoes")
	bool FreeEcho(EEchoType Echo);

	/** True once all four temples have given up their guardian. */
	UFUNCTION(BlueprintPure, Category = "Progression|Echoes")
	bool CanCallTheGuardians() const { return Progression.CanCallTheGuardians(); }

	// ---- Owl statues and warping. ----

	UFUNCTION(BlueprintPure, Category = "Progression|Travel")
	bool HasTouchedStatue(FName StatueId) const;

	UFUNCTION(BlueprintCallable, Category = "Progression|Travel")
	void TouchStatue(FName StatueId);

	// ---- Hearts, fairies and equipment. ----

	UFUNCTION(BlueprintCallable, Category = "Progression|Hearts")
	void GiveHeartFragment();

	UFUNCTION(BlueprintPure, Category = "Progression|Hearts")
	int32 GetMaxHearts() const { return Progression.GetMaxHearts(); }

	UFUNCTION(BlueprintPure, Category = "Progression|Hearts")
	int32 GetHeartFragments() const { return Progression.GetHeartFragments(); }

	UFUNCTION(BlueprintCallable, Category = "Progression|Fairies")
	void ReturnStrayFairy(EEchoType Temple);

	UFUNCTION(BlueprintPure, Category = "Progression|Fairies")
	int32 GetStrayFairies(EEchoType Temple) const;

	// ---- Money. ----

	UFUNCTION(BlueprintPure, Category = "Progression|Money")
	int32 GetRupees() const { return Progression.Carried().Rupees; }

	/** Adds rupees up to the wallet's capacity. Returns how many actually fit. */
	UFUNCTION(BlueprintCallable, Category = "Progression|Money")
	int32 AddRupees(int32 Amount);

	/** Spends rupees. Returns false and changes nothing when the purse is short. */
	UFUNCTION(BlueprintCallable, Category = "Progression|Money")
	bool SpendRupees(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Progression|Money")
	int32 DepositRupees(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Progression|Money")
	int32 WithdrawRupees(int32 Amount);

	UFUNCTION(BlueprintPure, Category = "Progression|Money")
	int32 GetBankedRupees() const { return Progression.GetBankedRupees(); }

	// ---- Flags. ----

	UFUNCTION(BlueprintCallable, Category = "Progression|Flags")
	void SetFlag(FName Flag, int32 Value = 1);

	UFUNCTION(BlueprintPure, Category = "Progression|Flags")
	int32 GetFlag(FName Flag) const;

	UFUNCTION(BlueprintPure, Category = "Progression|Flags")
	bool HasFlag(FName Flag) const { return GetFlag(Flag) != 0; }

	/**
	 * Flag source for the schedule table's gated entries.
	 *
	 * Kept as a separate adapter rather than making this class implement
	 * MaskGame::IFlagSource directly, so that the reflected type keeps a single
	 * base and the rules layer stays free of any UObject involvement.
	 */
	const MaskGame::IFlagSource& GetFlagSource() const { return FlagSource; }

	// ---- The rewind. ----

	/**
	 * Bank what is permanent and clear what is not.
	 *
	 * Callers are expected to reset the clock through UCycleSubsystem and reload
	 * the region afterwards; this function only touches progression.
	 */
	UFUNCTION(BlueprintCallable, Category = "Progression")
	void RewindProgression();

	UFUNCTION(BlueprintPure, Category = "Progression")
	int32 GetCycleCount() const { return CycleCount; }

	// ---- Saving. ----

	UFUNCTION(BlueprintCallable, Category = "Progression|Save")
	bool SaveProgress();

	UFUNCTION(BlueprintCallable, Category = "Progression|Save")
	bool LoadProgress();

	UFUNCTION(BlueprintCallable, Category = "Progression|Save")
	bool HasSaveFile() const;

	/** Direct access for systems that need the rules type. */
	MaskGame::FProgressionState& GetProgression() { return Progression; }
	const MaskGame::FProgressionState& GetProgression() const { return Progression; }

	UPROPERTY(BlueprintAssignable, Category = "Progression")
	FOnMaskAcquired OnMaskAcquired;

	UPROPERTY(BlueprintAssignable, Category = "Progression")
	FOnSongLearned OnSongLearned;

	UPROPERTY(BlueprintAssignable, Category = "Progression")
	FOnEchoFreed OnEchoFreed;

	UPROPERTY(BlueprintAssignable, Category = "Progression")
	FOnFlagChanged OnFlagChanged;

	UPROPERTY(BlueprintAssignable, Category = "Progression")
	FOnHeartsChanged OnHeartsChanged;

	/** Region the player is currently in; written into the save. */
	UPROPERTY(BlueprintReadWrite, Category = "Progression")
	FName CurrentRegionId;

private:
	void WriteTo(UMaskSaveGame& Save) const;
	void ReadFrom(const UMaskSaveGame& Save);

	/** Adapter that lets the rules layer read this instance's flags. */
	class FFlagSource final : public MaskGame::IFlagSource
	{
	public:
		explicit FFlagSource(const UMaskGameInstance* InOwner) : Owner(InOwner) {}
		virtual bool IsFlagSet(const std::string& Name) const override;

	private:
		const UMaskGameInstance* Owner = nullptr;
	};

	MaskGame::FProgressionState Progression;
	FFlagSource FlagSource{ this };
	int32 CycleCount = 1;
};
