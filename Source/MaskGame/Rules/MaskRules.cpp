// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

#include "MaskRules.h"

#include <array>

namespace MaskGame
{
	namespace
	{
		FFormTraits MakeTraveller()
		{
			FFormTraits T;
			T.DisplayName = "Traveller";
			T.WalkSpeed = 600.0f;
			T.SprintSpeed = 780.0f;
			T.JumpVelocity = 420.0f;
			T.bCanSwim = true;
			T.bCanUseSword = true;
			T.bCanUseBow = true;
			return T;
		}

		FFormTraits MakeSapling()
		{
			FFormTraits T;
			T.DisplayName = "Sapling";
			T.WalkSpeed = 480.0f;
			T.SprintSpeed = 520.0f;
			T.JumpVelocity = 300.0f;
			T.DamageTakenScale = 2.0f;   // Hollow wood; everything hurts twice as much.
			T.DamageDealtScale = 0.5f;
			T.bCanSwim = false;
			T.bSkipsOnWater = true;      // Five hops across a pond, then it sinks.
			T.MagicDrainPerSecond = 6.0f; // Spent gliding on a flower's updraft.
			return T;
		}

		FFormTraits MakeBoulderkin()
		{
			FFormTraits T;
			T.DisplayName = "Boulderkin";
			T.WalkSpeed = 420.0f;
			T.SprintSpeed = 1400.0f;     // Curled into a rolling wheel.
			T.JumpVelocity = 260.0f;
			T.DamageTakenScale = 0.5f;
			T.DamageDealtScale = 2.0f;
			T.bCanSwim = false;
			T.bSinksInWater = true;      // Walks the bottom instead of floating.
			T.bImmuneToHeat = true;
			T.MagicDrainPerSecond = 4.0f; // Spent on the spiked roll.
			return T;
		}

		FFormTraits MakeTideborn()
		{
			FFormTraits T;
			T.DisplayName = "Tideborn";
			T.WalkSpeed = 560.0f;
			T.SprintSpeed = 640.0f;
			T.JumpVelocity = 380.0f;
			T.bCanSwim = true;
			T.bBreathesUnderwater = true;
			T.MagicDrainPerSecond = 8.0f; // Spent on the crackling barrier.
			return T;
		}

		FFormTraits MakeWrath()
		{
			FFormTraits T;
			T.DisplayName = "Wrath";
			T.WalkSpeed = 620.0f;
			T.SprintSpeed = 700.0f;
			T.JumpVelocity = 440.0f;
			T.DamageTakenScale = 0.25f;
			T.DamageDealtScale = 8.0f;
			T.bCanSwim = true;
			T.bCanUseSword = true;
			return T;
		}

		const std::array<FFormTraits, static_cast<size_t>(EForm::Count)>& FormTable()
		{
			static const std::array<FFormTraits, static_cast<size_t>(EForm::Count)> Table = {
				MakeTraveller(), MakeSapling(), MakeBoulderkin(), MakeTideborn(), MakeWrath()
			};
			return Table;
		}

		constexpr EForm NoForm = EForm::Count;

		/**
		 * Every mask, in the order of EMaskId. Chapter numbers refer to the
		 * walkthrough structure documented in Docs/Walkthrough.md.
		 */
		const std::array<FMaskTraits, static_cast<size_t>(EMaskId::Count)>& MaskTable()
		{
			static const std::array<FMaskTraits, static_cast<size_t>(EMaskId::Count)> Table = {{
				{ EMaskId::None, "None", NoForm, EMaskEffect::Social, 1.0f, 1, "" },

				{ EMaskId::Sapling, "Sapling Mask", EForm::Sapling, EMaskEffect::Movement, 1.0f, 1,
				  "The face of a boy turned to wood. Launches from flower pads and skips over still water." },
				{ EMaskId::Boulderkin, "Boulderkin Mask", EForm::Boulderkin, EMaskEffect::Movement, 1.0f, 5,
				  "Heavy as the mountain that mourned him. Rolls, pounds, and walks through heat unharmed." },
				{ EMaskId::Tideborn, "Tideborn Mask", EForm::Tideborn, EMaskEffect::Movement, 1.0f, 8,
				  "A drowned musician's last performance. Swims the deep bay and throws finned blades." },
				{ EMaskId::Wrath, "Wrath Mask", EForm::Wrath, EMaskEffect::Combat, 1.0f, 13,
				  "Given by the children on the moon, if every other mask is offered first. Only a boss's arena will hold it." },

				{ EMaskId::CourierCap, "Courier's Cap", NoForm, EMaskEffect::Social, 1.0f, 1,
				  "The postman's route, worn on the head. Letterboxes give up what they hold." },
				{ EMaskId::SleeplessMask, "Sleepless Mask", NoForm, EMaskEffect::Revelation, 1.0f, 4,
				  "Eyes that never close. The drowsy will talk all night and tell you everything." },
				{ EMaskId::BlastMask, "Blast Mask", NoForm, EMaskEffect::Combat, 1.0f, 1,
				  "Detonates on command at the cost of your own skin. Bombs for the penniless." },
				{ EMaskId::UnseenMask, "Unseen Mask", NoForm, EMaskEffect::Social, 1.0f, 4,
				  "Sight slides off it. Guards, gatekeepers and shopkeepers look straight through you." },
				{ EMaskId::GraceMask, "Mask of Grace", NoForm, EMaskEffect::Revelation, 1.0f, 2,
				  "Draws scattered spirits back toward their fountain." },
				{ EMaskId::FoxwardenMask, "Foxwarden Mask", NoForm, EMaskEffect::Social, 1.0f, 2,
				  "Wear it in tall grass and something old and clever asks you riddles." },
				{ EMaskId::MarchMask, "March Mask", NoForm, EMaskEffect::Social, 1.0f, 4,
				  "Anything small enough will fall in behind you and keep step." },
				{ EMaskId::HareHood, "Hare Hood", NoForm, EMaskEffect::Movement, 1.6f, 2,
				  "Long ears, longer stride. The ground goes by faster and secrets sound louder." },
				{ EMaskId::ChorusMask, "Chorus Mask", NoForm, EMaskEffect::Social, 1.0f, 5,
				  "The mountain frogs answer it. Find all five and they will sing for you." },
				{ EMaskId::KeenNoseMask, "Keen Nose Mask", NoForm, EMaskEffect::Revelation, 1.0f, 2,
				  "Scent becomes colour. Mushrooms, trails and buried things stand out." },
				{ EMaskId::RanchMask, "Ranch Mask", NoForm, EMaskEffect::Social, 1.0f, 7,
				  "A little sister's face. The herd trusts it and so do the people who keep them." },
				{ EMaskId::RingleaderMask, "Ringleader's Mask", NoForm, EMaskEffect::Social, 1.0f, 7,
				  "The face of a brother who left. It stops a grown man mid-sentence." },
				{ EMaskId::LostSonMask, "Lost Son's Mask", NoForm, EMaskEffect::Social, 1.0f, 4,
				  "Shown to the right people it unwinds the town's longest, saddest errand." },
				{ EMaskId::BetrothalMask, "Betrothal Mask", NoForm, EMaskEffect::Social, 1.0f, 12,
				  "Two halves finally joined. Worn on the last night it moves people who cannot be moved." },
				{ EMaskId::TruthMask, "Mask of Truth", NoForm, EMaskEffect::Revelation, 1.0f, 2,
				  "Beasts speak plainly and the stones under the town remember out loud." },
				{ EMaskId::DancersMask, "Dancer's Mask", NoForm, EMaskEffect::Social, 1.0f, 7,
				  "Left by a ghost who only wanted his steps taught to someone." },
				{ EMaskId::WrappedMask, "Wrapped Mask", NoForm, EMaskEffect::Social, 1.0f, 11,
				  "The bandaged dead take you for one of their own and ask for what they are owed." },
				{ EMaskId::SpectreMask, "Spectre Mask", NoForm, EMaskEffect::Social, 1.0f, 10,
				  "Call the watchers of the ruined kingdom out of the air and make them answer." },
				{ EMaskId::CaptainsHelm, "Captain's Helm", NoForm, EMaskEffect::Social, 1.0f, 10,
				  "The bone soldiers still take orders from whoever wears the crest." },
				{ EMaskId::TitanMask, "Titan Mask", NoForm, EMaskEffect::Combat, 1.0f, 12,
				  "Found in the last temple's depths. Fills the arena and fights on the boss's own scale." },
			}};
			return Table;
		}
	}

	const FFormTraits& GetFormTraits(EForm Form)
	{
		const size_t Index = static_cast<size_t>(Form);
		const auto& Table = FormTable();
		return Index < Table.size() ? Table[Index] : Table[0];
	}

	const FMaskTraits& GetMaskTraits(EMaskId Mask)
	{
		const size_t Index = static_cast<size_t>(Mask);
		const auto& Table = MaskTable();
		return Index < Table.size() ? Table[Index] : Table[0];
	}

	bool IsTransformationMask(EMaskId Mask)
	{
		return GetMaskTraits(Mask).GrantsForm != EForm::Count;
	}

	EForm GetFormForMask(EMaskId Mask)
	{
		return GetMaskTraits(Mask).GrantsForm;
	}

	EMaskId GetMaskForForm(EForm Form)
	{
		switch (Form)
		{
		case EForm::Sapling:    return EMaskId::Sapling;
		case EForm::Boulderkin: return EMaskId::Boulderkin;
		case EForm::Tideborn:   return EMaskId::Tideborn;
		case EForm::Wrath:      return EMaskId::Wrath;
		default:                return EMaskId::None;
		}
	}

	bool CanEquipMask(const FProgressionState& State, EMaskId Mask, EForm CurrentForm, bool bInBossArena)
	{
		if (Mask == EMaskId::None || Mask == EMaskId::Count || !State.HasMask(Mask))
		{
			return false;
		}

		if (Mask == EMaskId::Wrath)
		{
			return bInBossArena;
		}

		if (IsTransformationMask(Mask))
		{
			// Putting on a transformation mask while already transformed simply
			// swaps bodies, so there is nothing further to check.
			return true;
		}

		// A regular mask needs a face to sit on, and only the traveller has one.
		return CurrentForm == EForm::Traveller;
	}

	std::string GetMaskName(EMaskId Mask)
	{
		return GetMaskTraits(Mask).DisplayName;
	}
}
