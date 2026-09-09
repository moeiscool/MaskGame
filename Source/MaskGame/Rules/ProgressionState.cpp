// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

#include "ProgressionState.h"

#include <algorithm>

namespace MaskGame
{
	namespace
	{
		/** Fragments needed to forge one more heart container. */
		constexpr int32_t FragmentsPerHeart = 4;
		/** Stray fairies hidden in each temple. */
		constexpr int32_t StrayFairiesPerTemple = 15;
	}

	int32_t FEquipmentTiers::GetMaxRupees() const
	{
		switch (std::clamp(WalletTier, 1, 3))
		{
		case 1:  return 200;
		case 2:  return 500;
		default: return 999;
		}
	}

	void FProgressionState::GiveMask(EMaskId Mask)
	{
		if (Mask != EMaskId::None && Mask != EMaskId::Count)
		{
			OwnedMasks.insert(Mask);
		}
	}

	void FProgressionState::FreeEcho(EEchoId Echo)
	{
		if (Echo != EEchoId::None && Echo != EEchoId::Count)
		{
			FreedEchoes.insert(Echo);
		}
	}

	void FProgressionState::GiveHeartFragment()
	{
		++HeartFragments;
		if (HeartFragments >= FragmentsPerHeart)
		{
			HeartFragments -= FragmentsPerHeart;
			++MaxHearts;
		}
	}

	void FProgressionState::ReturnStrayFairy(EEchoId Temple)
	{
		if (Temple == EEchoId::None || Temple == EEchoId::Count)
		{
			return;
		}
		int32_t& Count = StrayFairies[Temple];
		Count = std::min(StrayFairiesPerTemple, Count + 1);
	}

	int32_t FProgressionState::GetStrayFairies(EEchoId Temple) const
	{
		const auto It = StrayFairies.find(Temple);
		return It == StrayFairies.end() ? 0 : It->second;
	}

	int32_t FProgressionState::Deposit(int32_t Amount)
	{
		const int32_t Moved = std::clamp(Amount, 0, Consumables.Rupees);
		Consumables.Rupees -= Moved;
		BankedRupees += Moved;
		return Moved;
	}

	int32_t FProgressionState::Withdraw(int32_t Amount)
	{
		const int32_t Space = Tiers.GetMaxRupees() - Consumables.Rupees;
		const int32_t Moved = std::clamp(Amount, 0, std::min(BankedRupees, std::max(0, Space)));
		BankedRupees -= Moved;
		Consumables.Rupees += Moved;
		return Moved;
	}

	int32_t FProgressionState::GetFlag(const std::string& Name) const
	{
		const auto It = Flags.find(Name);
		return It == Flags.end() ? 0 : It->second;
	}

	void FProgressionState::ResetForNewCycle()
	{
		// The purse empties, the pouches empty, the temple forgets its keys.
		Consumables.Clear();

		// Flags marked permanent are the record of what the rewind cannot undo:
		// a mask already earned, a vow already made, a wall already blown open.
		const std::string Prefix = PermanentFlagPrefix;
		for (auto It = Flags.begin(); It != Flags.end(); )
		{
			const bool bPermanent = It->first.compare(0, Prefix.size(), Prefix) == 0;
			It = bPermanent ? std::next(It) : Flags.erase(It);
		}

		// Everything else on this object is permanent by construction and is
		// deliberately left untouched: masks, songs, echoes, statues, fairies,
		// hearts, equipment tiers and banked rupees.
	}
}
