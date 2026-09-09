// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

#include "TestHarness.h"

#include "MaskRules.h"
#include "SongMatcher.h"
#include "ProgressionState.h"

using namespace MaskGame;

namespace
{
	void TestRewindKeepsTheRightThings()
	{
		TEST_CASE("the hymn keeps masks, songs, echoes, hearts and the bank");
		FProgressionState State;

		State.GiveMask(EMaskId::Sapling);
		State.GiveMask(EMaskId::HareHood);
		State.LearnSong(static_cast<int32_t>(ESongId::HymnOfReturn));
		State.FreeEcho(EEchoId::Marshwood);
		State.TouchStatue("statue.mirefen");
		State.ReturnStrayFairy(EEchoId::Marshwood);
		State.Equipment().SwordTier = 2;
		State.Equipment().BombBagTier = 1;
		State.GiveHeartFragment();

		State.Carried().Rupees = 150;
		State.Deposit(100);

		State.SetFlag("perm.swamp.poison.cleared");
		State.SetFlag("mirefen.guard.bribed");
		State.Carried().Bombs = 12;
		State.Carried().Arrows = 20;
		State.Carried().DungeonKeys = 3;

		State.ResetForNewCycle();

		CHECK(State.HasMask(EMaskId::Sapling));
		CHECK(State.HasMask(EMaskId::HareHood));
		CHECK(State.HasSong(static_cast<int32_t>(ESongId::HymnOfReturn)));
		CHECK(State.HasEcho(EEchoId::Marshwood));
		CHECK(State.HasTouchedStatue("statue.mirefen"));
		CHECK_EQ(State.GetStrayFairies(EEchoId::Marshwood), 1);
		CHECK_EQ(State.Equipment().SwordTier, 2);
		CHECK_EQ(State.Equipment().BombBagTier, 1);
		CHECK_EQ(State.GetHeartFragments(), 1);
		CHECK_EQ(State.GetBankedRupees(), 100);
		CHECK(State.HasFlag("perm.swamp.poison.cleared"));

		TEST_CASE("the hymn takes the purse, the pouches and the temple keys");
		CHECK_EQ(State.Carried().Rupees, 0);
		CHECK_EQ(State.Carried().Bombs, 0);
		CHECK_EQ(State.Carried().Arrows, 0);
		CHECK_EQ(State.Carried().DungeonKeys, 0);
		CHECK(!State.HasFlag("mirefen.guard.bribed"));
		CHECK_EQ(static_cast<int>(State.GetFlags().size()), 1);
	}

	void TestHeartFragments()
	{
		TEST_CASE("four fragments make a heart container");
		FProgressionState State;
		CHECK_EQ(State.GetMaxHearts(), 3);

		for (int i = 0; i < 3; ++i)
		{
			State.GiveHeartFragment();
		}
		CHECK_EQ(State.GetMaxHearts(), 3);
		CHECK_EQ(State.GetHeartFragments(), 3);

		State.GiveHeartFragment();
		CHECK_EQ(State.GetMaxHearts(), 4);
		CHECK_EQ(State.GetHeartFragments(), 0);

		for (int i = 0; i < 9; ++i)
		{
			State.GiveHeartFragment();
		}
		CHECK_EQ(State.GetMaxHearts(), 6);
		CHECK_EQ(State.GetHeartFragments(), 1);
	}

	void TestBanking()
	{
		TEST_CASE("the wallet tier caps what can be withdrawn");
		FProgressionState State;
		CHECK_EQ(State.Equipment().GetMaxRupees(), 200);

		State.Carried().Rupees = 90;
		CHECK_EQ(State.Deposit(500), 90); // Can only bank what is actually carried.
		CHECK_EQ(State.Carried().Rupees, 0);
		CHECK_EQ(State.GetBankedRupees(), 90);

		State.Carried().Rupees = 195;
		CHECK_EQ(State.Withdraw(50), 5);  // Only five rupees of room left in the wallet.
		CHECK_EQ(State.Carried().Rupees, 200);
		CHECK_EQ(State.GetBankedRupees(), 85);

		TEST_CASE("a bigger wallet makes room for more");
		State.Equipment().WalletTier = 3;
		CHECK_EQ(State.Equipment().GetMaxRupees(), 999);
		CHECK_EQ(State.Withdraw(85), 85);
		CHECK_EQ(State.GetBankedRupees(), 0);

		TEST_CASE("negative and oversized amounts are ignored");
		CHECK_EQ(State.Deposit(-40), 0);
		CHECK_EQ(State.Withdraw(-40), 0);
		CHECK_EQ(State.Withdraw(1000), 0);
	}

	void TestEquipmentTiers()
	{
		TEST_CASE("pouch capacities follow their tiers");
		FEquipmentTiers Tiers;
		CHECK_EQ(Tiers.GetMaxArrows(), 0);
		CHECK_EQ(Tiers.GetMaxBombs(), 0);

		Tiers.QuiverTier = 1; CHECK_EQ(Tiers.GetMaxArrows(), 30);
		Tiers.QuiverTier = 3; CHECK_EQ(Tiers.GetMaxArrows(), 50);
		Tiers.BombBagTier = 1; CHECK_EQ(Tiers.GetMaxBombs(), 20);
		Tiers.BombBagTier = 3; CHECK_EQ(Tiers.GetMaxBombs(), 40);
		Tiers.WalletTier = 2; CHECK_EQ(Tiers.GetMaxRupees(), 500);
	}

	void TestEchoesAndFairies()
	{
		TEST_CASE("all four echoes are needed to answer the oath");
		FProgressionState State;
		CHECK(!State.CanCallTheGuardians());

		State.FreeEcho(EEchoId::Marshwood);
		State.FreeEcho(EEchoId::Frostcrown);
		State.FreeEcho(EEchoId::Tidebreak);
		CHECK(!State.CanCallTheGuardians());

		State.FreeEcho(EEchoId::SunkenSpire);
		CHECK(State.CanCallTheGuardians());
		CHECK_EQ(static_cast<int>(State.GetEchoCount()), 4);

		TEST_CASE("freeing an echo twice does not count twice");
		State.FreeEcho(EEchoId::Marshwood);
		CHECK_EQ(static_cast<int>(State.GetEchoCount()), 4);

		TEST_CASE("stray fairies cap at the temple's count");
		for (int i = 0; i < 50; ++i)
		{
			State.ReturnStrayFairy(EEchoId::Tidebreak);
		}
		CHECK_EQ(State.GetStrayFairies(EEchoId::Tidebreak), 15);
		CHECK_EQ(State.GetStrayFairies(EEchoId::Frostcrown), 0);

		TEST_CASE("invalid ids are rejected rather than stored");
		State.FreeEcho(EEchoId::None);
		State.ReturnStrayFairy(EEchoId::None);
		State.GiveMask(EMaskId::None);
		CHECK_EQ(static_cast<int>(State.GetEchoCount()), 4);
		CHECK_EQ(static_cast<int>(State.GetMaskCount()), 0);
	}

	void TestCollectingEveryMask()
	{
		TEST_CASE("the full set is every id except None");
		FProgressionState State;
		CHECK(!State.HasEveryMask());

		for (int i = 1; i < static_cast<int>(EMaskId::Count); ++i)
		{
			State.GiveMask(static_cast<EMaskId>(i));
		}
		CHECK(State.HasEveryMask());
		CHECK_EQ(static_cast<int>(State.GetMaskCount()), static_cast<int>(EMaskId::Count) - 1);
	}
}

int main()
{
	std::printf("ProgressionState\n");
	TestRewindKeepsTheRightThings();
	TestHeartFragments();
	TestBanking();
	TestEquipmentTiers();
	TestEchoesAndFairies();
	TestCollectingEveryMask();
	return MaskGameTest::Report("ProgressionState");
}
