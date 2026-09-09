// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.
//
// Engine-free model of what the player keeps and what they lose when the cycle
// rewinds. This is the single source of truth for that rule; the Unreal save
// game and HUD both read it rather than reimplementing it.

#pragma once

#include <cstdint>
#include <map>
#include <set>
#include <string>

namespace MaskGame
{
	/**
	 * Every mask in the game.
	 *
	 * The first four are transformation masks, which change the player's form.
	 * The rest are worn over the current form and change how the world reacts.
	 */
	enum class EMaskId : uint8_t
	{
		None = 0,

		// Transformation masks.
		Sapling,      // Light, glides on flower-spun updrafts, skips over water.
		Boulderkin,   // Heavy, rolls, punches through cracked stone, sinks in lava-proof rock.
		Tideborn,     // Swims and casts a barrier; the only form that breathes underwater.
		Wrath,        // The final mask. Immense damage, no magic, boss arenas only.

		// Regular masks, roughly in the order the walkthrough hands them out.
		CourierCap,
		SleeplessMask,
		BlastMask,
		UnseenMask,
		GraceMask,
		FoxwardenMask,
		MarchMask,
		HareHood,
		ChorusMask,
		KeenNoseMask,
		RanchMask,
		RingleaderMask,
		LostSonMask,
		BetrothalMask,
		TruthMask,
		DancersMask,
		WrappedMask,
		SpectreMask,
		CaptainsHelm,
		TitanMask,

		Count,
	};

	/** The four guardian echoes, one per temple, that must be freed to unmake the moon. */
	enum class EEchoId : uint8_t
	{
		None = 0,
		Marshwood,   // Chapter 3  - Marshwood Temple
		Frostcrown,  // Chapter 6  - Frostcrown Temple
		Tidebreak,   // Chapter 9  - Tidebreak Temple
		SunkenSpire, // Chapter 12 - Sunken Spire Temple
		Count,
	};

	/** Equipment tiers that survive a rewind once earned. */
	struct FEquipmentTiers
	{
		int32_t SwordTier = 1;   // 1 Kokiri-scale blade, 2 razor, 3 gilded.
		int32_t ShieldTier = 1;  // 1 hero's, 2 mirror.
		int32_t QuiverTier = 0;  // 0 none, 1..3 = 30/40/50 arrows.
		int32_t BombBagTier = 0; // 0 none, 1..3 = 20/30/40 bombs.
		int32_t WalletTier = 1;  // 1..3 = 200/500/999 rupees.
		int32_t BottleCount = 0; // 0..6.

		int32_t GetMaxArrows() const { return QuiverTier <= 0 ? 0 : 20 + QuiverTier * 10; }
		int32_t GetMaxBombs() const { return BombBagTier <= 0 ? 0 : 10 + BombBagTier * 10; }
		int32_t GetMaxRupees() const;
	};

	/** Everything the player is carrying right now. All of it is lost on a rewind. */
	struct FConsumables
	{
		int32_t Rupees = 0;
		int32_t Arrows = 0;
		int32_t Bombs = 0;
		int32_t Nuts = 0;
		int32_t Sticks = 0;
		/** Small keys for the temple currently being explored. */
		int32_t DungeonKeys = 0;

		void Clear() { *this = FConsumables{}; }
	};

	/**
	 * The player's whole progression.
	 *
	 * Split deliberately into two halves: fields above ResetForNewCycle's line
	 * persist across the rewind, fields below it do not. Adding a new field means
	 * deciding which half it belongs to, and ResetForNewCycle is where that
	 * decision is written down.
	 */
	class FProgressionState
	{
	public:
		// ---- Permanent progress: survives the Hymn of Return. ----

		bool HasMask(EMaskId Mask) const { return OwnedMasks.count(Mask) > 0; }
		void GiveMask(EMaskId Mask);
		size_t GetMaskCount() const { return OwnedMasks.size(); }
		const std::set<EMaskId>& GetMasks() const { return OwnedMasks; }

		bool HasSong(int32_t SongId) const { return LearnedSongs.count(SongId) > 0; }
		void LearnSong(int32_t SongId) { LearnedSongs.insert(SongId); }
		size_t GetSongCount() const { return LearnedSongs.size(); }

		bool HasEcho(EEchoId Echo) const { return FreedEchoes.count(Echo) > 0; }
		void FreeEcho(EEchoId Echo);
		size_t GetEchoCount() const { return FreedEchoes.size(); }
		/** All four echoes freed: the Oath of Concord can be answered. */
		bool CanCallTheGuardians() const { return FreedEchoes.size() >= 4; }

		bool HasTouchedStatue(const std::string& StatueId) const { return TouchedStatues.count(StatueId) > 0; }
		void TouchStatue(const std::string& StatueId) { TouchedStatues.insert(StatueId); }

		/** Four fragments make a heart container. */
		void GiveHeartFragment();
		int32_t GetHeartFragments() const { return HeartFragments; }
		int32_t GetMaxHearts() const { return MaxHearts; }

		/** Stray fairies returned, keyed by temple. Twenty per temple earns a reward. */
		void ReturnStrayFairy(EEchoId Temple);
		int32_t GetStrayFairies(EEchoId Temple) const;

		FEquipmentTiers& Equipment() { return Tiers; }
		const FEquipmentTiers& Equipment() const { return Tiers; }

		/** Rupees deposited with the banker. The only money that survives a rewind. */
		int32_t GetBankedRupees() const { return BankedRupees; }
		/** Move rupees from the purse into the bank. Returns the amount actually deposited. */
		int32_t Deposit(int32_t Amount);
		/** Withdraw from the bank into the purse, respecting the wallet cap. */
		int32_t Withdraw(int32_t Amount);

		// ---- Cycle-scoped progress: cleared by the Hymn of Return. ----

		FConsumables& Carried() { return Consumables; }
		const FConsumables& Carried() const { return Consumables; }

		/**
		 * Quest and world flags.
		 *
		 * A flag whose name begins with PermanentFlagPrefix survives the rewind;
		 * every other flag is cleared. That prefix is how a designer marks
		 * "this happened for good" without touching this class.
		 */
		static constexpr const char* PermanentFlagPrefix = "perm.";

		void SetFlag(const std::string& Name, int32_t Value = 1) { Flags[Name] = Value; }
		int32_t GetFlag(const std::string& Name) const;
		bool HasFlag(const std::string& Name) const { return GetFlag(Name) != 0; }
		const std::map<std::string, int32_t>& GetFlags() const { return Flags; }

		/**
		 * Rewind the cycle.
		 *
		 * Keeps masks, songs, echoes, hearts, statues, equipment tiers, banked
		 * rupees and permanent flags. Clears the purse, all ammunition, temple
		 * keys and every non-permanent flag.
		 */
		void ResetForNewCycle();

		/** How many of the twenty-four masks have been collected. */
		bool HasEveryMask() const { return OwnedMasks.size() >= static_cast<size_t>(EMaskId::Count) - 1; }

	private:
		// Permanent.
		std::set<EMaskId> OwnedMasks;
		std::set<int32_t> LearnedSongs;
		std::set<EEchoId> FreedEchoes;
		std::set<std::string> TouchedStatues;
		std::map<EEchoId, int32_t> StrayFairies;
		FEquipmentTiers Tiers;
		int32_t HeartFragments = 0;
		int32_t MaxHearts = 3;
		int32_t BankedRupees = 0;

		// Cycle-scoped.
		FConsumables Consumables;
		std::map<std::string, int32_t> Flags;
	};
}
