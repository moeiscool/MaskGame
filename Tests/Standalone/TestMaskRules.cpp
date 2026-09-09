// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

#include "TestHarness.h"

#include "MaskRules.h"

#include <set>
#include <string>

using namespace MaskGame;

namespace
{
	void TestTablesAreComplete()
	{
		TEST_CASE("the mask table is indexed by mask id");
		// GetMaskTraits indexes the table directly, so a row inserted out of order
		// would silently hand out the wrong mask's traits.
		for (int i = 0; i < static_cast<int>(EMaskId::Count); ++i)
		{
			const EMaskId Id = static_cast<EMaskId>(i);
			const FMaskTraits& Traits = GetMaskTraits(Id);
			CHECK(Traits.Id == Id);
			CHECK(std::string(Traits.DisplayName).size() > 0);
		}

		TEST_CASE("every mask but None has a description and a chapter");
		for (int i = 1; i < static_cast<int>(EMaskId::Count); ++i)
		{
			const FMaskTraits& Traits = GetMaskTraits(static_cast<EMaskId>(i));
			CHECK(std::string(Traits.Description).size() > 0);
			CHECK(Traits.FirstAvailableChapter >= 1 && Traits.FirstAvailableChapter <= 13);
		}

		TEST_CASE("mask names are unique");
		std::set<std::string> Names;
		for (int i = 1; i < static_cast<int>(EMaskId::Count); ++i)
		{
			const std::string Name = GetMaskName(static_cast<EMaskId>(i));
			CHECK(Names.insert(Name).second);
		}

		TEST_CASE("every form has traits");
		for (int i = 0; i < static_cast<int>(EForm::Count); ++i)
		{
			const FFormTraits& Traits = GetFormTraits(static_cast<EForm>(i));
			CHECK(std::string(Traits.DisplayName).size() > 0);
			CHECK(Traits.WalkSpeed > 0.0f);
			CHECK(Traits.JumpVelocity > 0.0f);
		}
	}

	void TestTransformationMasksMapToForms()
	{
		TEST_CASE("exactly four masks are transformation masks");
		int TransformationCount = 0;
		for (int i = 1; i < static_cast<int>(EMaskId::Count); ++i)
		{
			if (IsTransformationMask(static_cast<EMaskId>(i)))
			{
				++TransformationCount;
			}
		}
		CHECK_EQ(TransformationCount, 4);

		TEST_CASE("mask and form map onto each other both ways");
		const EForm Forms[] = { EForm::Sapling, EForm::Boulderkin, EForm::Tideborn, EForm::Wrath };
		for (const EForm Form : Forms)
		{
			const EMaskId Mask = GetMaskForForm(Form);
			CHECK(Mask != EMaskId::None);
			CHECK(GetFormForMask(Mask) == Form);
		}
		CHECK(GetMaskForForm(EForm::Traveller) == EMaskId::None);
		CHECK(!IsTransformationMask(EMaskId::HareHood));
		CHECK(GetFormForMask(EMaskId::HareHood) == EForm::Count);
	}

	void TestFormTraitsMatchTheirFantasy()
	{
		TEST_CASE("the sapling is fragile and skips water but cannot swim");
		const FFormTraits& Sapling = GetFormTraits(EForm::Sapling);
		CHECK(!Sapling.bCanSwim);
		CHECK(Sapling.bSkipsOnWater);
		CHECK(Sapling.DamageTakenScale > 1.0f);

		TEST_CASE("the boulderkin sinks, ignores heat and rolls fast");
		const FFormTraits& Boulderkin = GetFormTraits(EForm::Boulderkin);
		CHECK(Boulderkin.bSinksInWater);
		CHECK(!Boulderkin.bCanSwim);
		CHECK(Boulderkin.bImmuneToHeat);
		CHECK(Boulderkin.SprintSpeed > GetFormTraits(EForm::Traveller).SprintSpeed);

		TEST_CASE("only the tideborn breathes underwater");
		for (int i = 0; i < static_cast<int>(EForm::Count); ++i)
		{
			const EForm Form = static_cast<EForm>(i);
			if (GetFormTraits(Form).bBreathesUnderwater)
			{
				CHECK(Form == EForm::Tideborn);
			}
		}

		TEST_CASE("only the traveller and wrath carry a blade");
		CHECK(GetFormTraits(EForm::Traveller).bCanUseSword);
		CHECK(GetFormTraits(EForm::Wrath).bCanUseSword);
		CHECK(!GetFormTraits(EForm::Sapling).bCanUseSword);
		CHECK(!GetFormTraits(EForm::Boulderkin).bCanUseSword);
		CHECK(!GetFormTraits(EForm::Tideborn).bCanUseSword);

		TEST_CASE("wrath hits hardest and only the traveller draws a bow");
		CHECK(GetFormTraits(EForm::Wrath).DamageDealtScale > GetFormTraits(EForm::Boulderkin).DamageDealtScale);
		CHECK(GetFormTraits(EForm::Traveller).bCanUseBow);
		CHECK(!GetFormTraits(EForm::Wrath).bCanUseBow);
	}

	void TestEquipRules()
	{
		FProgressionState State;

		TEST_CASE("a mask you do not own can never be worn");
		CHECK(!CanEquipMask(State, EMaskId::HareHood, EForm::Traveller, false));
		CHECK(!CanEquipMask(State, EMaskId::Sapling, EForm::Traveller, false));
		CHECK(!CanEquipMask(State, EMaskId::None, EForm::Traveller, false));

		State.GiveMask(EMaskId::HareHood);
		State.GiveMask(EMaskId::Sapling);
		State.GiveMask(EMaskId::Boulderkin);
		State.GiveMask(EMaskId::Wrath);

		TEST_CASE("regular masks need the traveller's own face");
		CHECK(CanEquipMask(State, EMaskId::HareHood, EForm::Traveller, false));
		CHECK(!CanEquipMask(State, EMaskId::HareHood, EForm::Sapling, false));
		CHECK(!CanEquipMask(State, EMaskId::HareHood, EForm::Boulderkin, false));
		CHECK(!CanEquipMask(State, EMaskId::HareHood, EForm::Tideborn, false));

		TEST_CASE("transformation masks can be worn from any form");
		CHECK(CanEquipMask(State, EMaskId::Sapling, EForm::Traveller, false));
		CHECK(CanEquipMask(State, EMaskId::Sapling, EForm::Boulderkin, false));
		CHECK(CanEquipMask(State, EMaskId::Boulderkin, EForm::Sapling, false));

		TEST_CASE("the wrath mask only comes out in a boss's arena");
		CHECK(!CanEquipMask(State, EMaskId::Wrath, EForm::Traveller, false));
		CHECK(CanEquipMask(State, EMaskId::Wrath, EForm::Traveller, true));
		CHECK(CanEquipMask(State, EMaskId::Wrath, EForm::Sapling, true));

		TEST_CASE("owning the wrath mask does not bypass the ownership check");
		FProgressionState Empty;
		CHECK(!CanEquipMask(Empty, EMaskId::Wrath, EForm::Traveller, true));
	}
}

int main()
{
	std::printf("MaskRules\n");
	TestTablesAreComplete();
	TestTransformationMasksMapToForms();
	TestFormTraitsMatchTheirFantasy();
	TestEquipRules();
	return MaskGameTest::Report("MaskRules");
}
