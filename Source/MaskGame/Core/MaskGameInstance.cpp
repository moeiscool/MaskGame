// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

#include "Core/MaskGameInstance.h"

#include "Kismet/GameplayStatics.h"

#include "Core/MaskGameSettings.h"
#include "Core/MaskSaveGame.h"
#include "MaskGame.h"

namespace
{
	/** FName to the std::string the rules layer keys flags by. */
	std::string ToStdString(FName Name)
	{
		return std::string(TCHAR_TO_UTF8(*Name.ToString()));
	}

	MaskGame::EMaskId ToRules(EMaskType Mask) { return static_cast<MaskGame::EMaskId>(Mask); }
	MaskGame::EEchoId ToRules(EEchoType Echo) { return static_cast<MaskGame::EEchoId>(Echo); }
	int32 ToRulesSong(ESongType Song) { return static_cast<int32>(Song); }
}

bool UMaskGameInstance::FFlagSource::IsFlagSet(const std::string& Name) const
{
	return Owner != nullptr && Owner->Progression.GetFlag(Name) != 0;
}

void UMaskGameInstance::Init()
{
	Super::Init();

	// A file left over from a previous session is picked up automatically; a new
	// game simply starts with an empty progression.
	if (HasSaveFile())
	{
		LoadProgress();
	}
}

UMaskGameInstance* UMaskGameInstance::Get(const UObject* WorldContextObject)
{
	return WorldContextObject != nullptr
		? Cast<UMaskGameInstance>(UGameplayStatics::GetGameInstance(WorldContextObject))
		: nullptr;
}

bool UMaskGameInstance::HasMask(EMaskType Mask) const
{
	return Progression.HasMask(ToRules(Mask));
}

bool UMaskGameInstance::GiveMask(EMaskType Mask)
{
	if (Mask == EMaskType::None || Mask == EMaskType::Count || HasMask(Mask))
	{
		return false;
	}

	Progression.GiveMask(ToRules(Mask));
	UE_LOG(LogMaskGame, Log, TEXT("Mask acquired: %s (%d of %d)."),
		*UEnum::GetDisplayValueAsText(Mask).ToString(),
		GetMaskCount(),
		static_cast<int32>(EMaskType::Count) - 1);

	OnMaskAcquired.Broadcast(Mask);
	return true;
}

bool UMaskGameInstance::HasSong(ESongType Song) const
{
	return Progression.HasSong(ToRulesSong(Song));
}

bool UMaskGameInstance::LearnSong(ESongType Song)
{
	if (Song == ESongType::None || HasSong(Song))
	{
		return false;
	}

	Progression.LearnSong(ToRulesSong(Song));
	UE_LOG(LogMaskGame, Log, TEXT("Song learned: %s."), *UEnum::GetDisplayValueAsText(Song).ToString());
	OnSongLearned.Broadcast(Song);
	return true;
}

bool UMaskGameInstance::HasEcho(EEchoType Echo) const
{
	return Progression.HasEcho(ToRules(Echo));
}

bool UMaskGameInstance::FreeEcho(EEchoType Echo)
{
	if (Echo == EEchoType::None || Echo == EEchoType::Count || HasEcho(Echo))
	{
		return false;
	}

	Progression.FreeEcho(ToRules(Echo));
	UE_LOG(LogMaskGame, Log, TEXT("Echo freed: %s (%d of 4)."),
		*UEnum::GetDisplayValueAsText(Echo).ToString(),
		static_cast<int32>(Progression.GetEchoCount()));

	OnEchoFreed.Broadcast(Echo);

	// Freeing a guardian is the one thing a rewind must never undo, so it is
	// mirrored into a permanent flag for quests and dialogue to branch on.
	SetFlag(*FString::Printf(TEXT("perm.echo.%s"), *UEnum::GetValueAsString(Echo)), 1);
	return true;
}

bool UMaskGameInstance::HasTouchedStatue(FName StatueId) const
{
	return Progression.HasTouchedStatue(ToStdString(StatueId));
}

void UMaskGameInstance::TouchStatue(FName StatueId)
{
	if (!StatueId.IsNone())
	{
		Progression.TouchStatue(ToStdString(StatueId));
	}
}

void UMaskGameInstance::GiveHeartFragment()
{
	const int32 Before = Progression.GetMaxHearts();
	Progression.GiveHeartFragment();
	if (Progression.GetMaxHearts() != Before)
	{
		OnHeartsChanged.Broadcast(Progression.GetMaxHearts());
	}
}

void UMaskGameInstance::ReturnStrayFairy(EEchoType Temple)
{
	Progression.ReturnStrayFairy(ToRules(Temple));
}

int32 UMaskGameInstance::GetStrayFairies(EEchoType Temple) const
{
	return Progression.GetStrayFairies(ToRules(Temple));
}

int32 UMaskGameInstance::AddRupees(int32 Amount)
{
	if (Amount <= 0)
	{
		return 0;
	}

	const int32 Room = Progression.Equipment().GetMaxRupees() - Progression.Carried().Rupees;
	const int32 Added = FMath::Clamp(Amount, 0, FMath::Max(0, Room));
	Progression.Carried().Rupees += Added;
	return Added;
}

bool UMaskGameInstance::SpendRupees(int32 Amount)
{
	if (Amount <= 0 || Progression.Carried().Rupees < Amount)
	{
		return false;
	}

	Progression.Carried().Rupees -= Amount;
	return true;
}

int32 UMaskGameInstance::DepositRupees(int32 Amount)
{
	return Progression.Deposit(Amount);
}

int32 UMaskGameInstance::WithdrawRupees(int32 Amount)
{
	return Progression.Withdraw(Amount);
}

void UMaskGameInstance::SetFlag(FName Flag, int32 Value)
{
	if (Flag.IsNone())
	{
		return;
	}

	const std::string Key = ToStdString(Flag);
	if (Progression.GetFlag(Key) == Value)
	{
		return;
	}

	Progression.SetFlag(Key, Value);
	OnFlagChanged.Broadcast(Flag, Value);
}

int32 UMaskGameInstance::GetFlag(FName Flag) const
{
	return Flag.IsNone() ? 0 : Progression.GetFlag(ToStdString(Flag));
}

void UMaskGameInstance::RewindProgression()
{
	// Saving before the reset is deliberate: the file should describe what the
	// player has earned, not the half-finished cycle they are abandoning.
	SaveProgress();

	Progression.ResetForNewCycle();
	++CycleCount;

	UE_LOG(LogMaskGame, Log, TEXT("Progression rewound; beginning cycle %d with %d masks and %d echoes."),
		CycleCount, GetMaskCount(), static_cast<int32>(Progression.GetEchoCount()));
}

bool UMaskGameInstance::HasSaveFile() const
{
	return UGameplayStatics::DoesSaveGameExist(UMaskGameSettings::Get().SaveSlotName, 0);
}

bool UMaskGameInstance::SaveProgress()
{
	UMaskSaveGame* Save = Cast<UMaskSaveGame>(UGameplayStatics::CreateSaveGameObject(UMaskSaveGame::StaticClass()));
	if (Save == nullptr)
	{
		UE_LOG(LogMaskGame, Error, TEXT("Could not create a save game object."));
		return false;
	}

	WriteTo(*Save);

	const bool bSaved = UGameplayStatics::SaveGameToSlot(Save, UMaskGameSettings::Get().SaveSlotName, 0);
	UE_LOG(LogMaskGame, Log, TEXT("Save to slot '%s' %s."),
		*UMaskGameSettings::Get().SaveSlotName, bSaved ? TEXT("succeeded") : TEXT("failed"));
	return bSaved;
}

bool UMaskGameInstance::LoadProgress()
{
	const UMaskSaveGame* Save = Cast<UMaskSaveGame>(
		UGameplayStatics::LoadGameFromSlot(UMaskGameSettings::Get().SaveSlotName, 0));
	if (Save == nullptr)
	{
		UE_LOG(LogMaskGame, Warning, TEXT("No save found in slot '%s'."), *UMaskGameSettings::Get().SaveSlotName);
		return false;
	}

	if (Save->SaveVersion > UMaskSaveGame::CurrentVersion)
	{
		UE_LOG(LogMaskGame, Error, TEXT("Save is version %d but this build understands %d; refusing to load."),
			Save->SaveVersion, UMaskSaveGame::CurrentVersion);
		return false;
	}

	ReadFrom(*Save);
	UE_LOG(LogMaskGame, Log, TEXT("Loaded cycle %d with %d masks."), CycleCount, GetMaskCount());
	return true;
}

void UMaskGameInstance::WriteTo(UMaskSaveGame& Save) const
{
	Save.SaveVersion = UMaskSaveGame::CurrentVersion;

	for (const MaskGame::EMaskId Mask : Progression.GetMasks())
	{
		Save.OwnedMasks.Add(static_cast<EMaskType>(Mask));
	}

	for (int32 SongIndex = 1; SongIndex <= static_cast<int32>(ESongType::SongOfStorms); ++SongIndex)
	{
		if (Progression.HasSong(SongIndex))
		{
			Save.LearnedSongs.Add(static_cast<ESongType>(SongIndex));
		}
	}

	for (int32 EchoIndex = 1; EchoIndex < static_cast<int32>(EEchoType::Count); ++EchoIndex)
	{
		const EEchoType Echo = static_cast<EEchoType>(EchoIndex);
		if (HasEcho(Echo))
		{
			Save.FreedEchoes.Add(Echo);
		}
		if (const int32 Fairies = GetStrayFairies(Echo); Fairies > 0)
		{
			Save.StrayFairies.Add(Echo, Fairies);
		}
	}

	// Only permanent flags are written; the rest describe a cycle that will not
	// exist when the save is next opened.
	const std::string Prefix = MaskGame::FProgressionState::PermanentFlagPrefix;
	for (const auto& [Name, Value] : Progression.GetFlags())
	{
		if (Name.compare(0, Prefix.size(), Prefix) == 0)
		{
			Save.PermanentFlags.Add(FName(UTF8_TO_TCHAR(Name.c_str())), Value);
		}
	}

	const MaskGame::FEquipmentTiers& Tiers = Progression.Equipment();
	Save.SwordTier = Tiers.SwordTier;
	Save.ShieldTier = Tiers.ShieldTier;
	Save.QuiverTier = Tiers.QuiverTier;
	Save.BombBagTier = Tiers.BombBagTier;
	Save.WalletTier = Tiers.WalletTier;
	Save.BottleCount = Tiers.BottleCount;
	Save.MagicTier = Tiers.MagicTier;

	Save.HeartFragments = Progression.GetHeartFragments();
	Save.MaxHearts = Progression.GetMaxHearts();
	Save.BankedRupees = Progression.GetBankedRupees();
	Save.CycleCount = CycleCount;
	Save.LastRegionId = CurrentRegionId;

	for (const std::string& StatueId : Progression.GetTouchedStatues())
	{
		Save.TouchedStatues.Add(FName(UTF8_TO_TCHAR(StatueId.c_str())));
	}
}

void UMaskGameInstance::ReadFrom(const UMaskSaveGame& Save)
{
	Progression = MaskGame::FProgressionState();

	for (const EMaskType Mask : Save.OwnedMasks)
	{
		Progression.GiveMask(ToRules(Mask));
	}
	for (const ESongType Song : Save.LearnedSongs)
	{
		Progression.LearnSong(ToRulesSong(Song));
	}
	for (const EEchoType Echo : Save.FreedEchoes)
	{
		Progression.FreeEcho(ToRules(Echo));
	}
	for (const FName StatueId : Save.TouchedStatues)
	{
		Progression.TouchStatue(ToStdString(StatueId));
	}
	for (const auto& [Temple, Count] : Save.StrayFairies)
	{
		Progression.RestoreStrayFairies(ToRules(Temple), Count);
	}
	for (const auto& [Flag, Value] : Save.PermanentFlags)
	{
		Progression.SetFlag(ToStdString(Flag), Value);
	}

	MaskGame::FEquipmentTiers& Tiers = Progression.Equipment();
	Tiers.SwordTier = Save.SwordTier;
	Tiers.ShieldTier = Save.ShieldTier;
	Tiers.QuiverTier = Save.QuiverTier;
	Tiers.BombBagTier = Save.BombBagTier;
	Tiers.WalletTier = Save.WalletTier;
	Tiers.BottleCount = Save.BottleCount;
	Tiers.MagicTier = Save.MagicTier;

	Progression.RestoreHearts(Save.MaxHearts, Save.HeartFragments);
	Progression.RestoreBankedRupees(Save.BankedRupees);

	// A load lands the player at the first dawn of a fresh cycle, so the purse
	// and pouches start empty exactly as they would after a rewind.
	Progression.Carried().Clear();

	CycleCount = FMath::Max(1, Save.CycleCount);
	CurrentRegionId = Save.LastRegionId;
}
