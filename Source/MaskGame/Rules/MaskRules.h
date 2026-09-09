// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.
//
// Engine-free description of what each form and each mask does. The Unreal
// character reads these tables rather than hard-coding movement numbers, so the
// same rules drive gameplay, the tests and the tooltip text.

#pragma once

#include <cstdint>
#include <string>

#include "ProgressionState.h"

namespace MaskGame
{
	/** The bodies the player can wear. */
	enum class EForm : uint8_t
	{
		/** The traveller's own body: sword, shield, bow, hookshot. */
		Traveller,
		/** Light and hollow. Launches from flower pads, skitters across water. */
		Sapling,
		/** Heavy and armoured. Rolls, pounds, ignores heat, sinks like a stone. */
		Boulderkin,
		/** Built for water. Swims fast, breathes under it, throws finned blades. */
		Tideborn,
		/** The mask beneath the masks. Overwhelming, brief, and only in a boss's arena. */
		Wrath,
		Count,
	};

	/** Numbers that define how a form moves and what it may do. */
	struct FFormTraits
	{
		const char* DisplayName = "";
		float WalkSpeed = 600.0f;
		float SprintSpeed = 600.0f;
		float JumpVelocity = 420.0f;
		/** Scales incoming damage; the Sapling is fragile, Wrath nearly untouchable. */
		float DamageTakenScale = 1.0f;
		/** Scales outgoing damage. */
		float DamageDealtScale = 1.0f;
		bool bCanSwim = false;
		bool bBreathesUnderwater = false;
		/** Sinks to the floor of any water volume instead of swimming. */
		bool bSinksInWater = false;
		/** Walks on water surfaces for a short distance. */
		bool bSkipsOnWater = false;
		bool bImmuneToHeat = false;
		bool bCanUseSword = false;
		bool bCanUseBow = false;
		/** Magic drained per second while the form's special action is held. */
		float MagicDrainPerSecond = 0.0f;
	};

	/** What a non-transformation mask changes about the world's reaction to you. */
	enum class EMaskEffect : uint8_t
	{
		/** Purely a key: someone recognises it and opens a door. */
		Social,
		/** Alters movement. */
		Movement,
		/** Alters combat. */
		Combat,
		/** Reveals information otherwise hidden. */
		Revelation,
	};

	/** Static description of one mask. */
	struct FMaskTraits
	{
		EMaskId Id = EMaskId::None;
		const char* DisplayName = "";
		/** Non-None for the four transformation masks. */
		EForm GrantsForm = EForm::Count;
		EMaskEffect Effect = EMaskEffect::Social;
		/** Movement speed multiplier applied while worn. */
		float SpeedMultiplier = 1.0f;
		/** Chapter of the walkthrough where this mask first becomes obtainable. */
		int32_t FirstAvailableChapter = 1;
		/** One line for the inventory screen. */
		const char* Description = "";
	};

	/** Look up the movement and combat traits of a form. */
	const FFormTraits& GetFormTraits(EForm Form);

	/** Look up the static description of a mask. Never null for a valid id. */
	const FMaskTraits& GetMaskTraits(EMaskId Mask);

	/** True for the four masks that replace the body rather than sit on it. */
	bool IsTransformationMask(EMaskId Mask);

	/** The form a transformation mask grants, or EForm::Count for a regular mask. */
	EForm GetFormForMask(EMaskId Mask);

	/** The transformation mask that produces a form, or EMaskId::None for Traveller. */
	EMaskId GetMaskForForm(EForm Form);

	/**
	 * May this mask be put on right now?
	 *
	 * Transformation masks may be worn from any form. Regular masks may only be
	 * worn in the traveller's own body, because they sit on a face the other
	 * forms do not have. The Wrath mask is refused outside a boss's arena.
	 */
	bool CanEquipMask(const FProgressionState& State, EMaskId Mask, EForm CurrentForm, bool bInBossArena);

	/** Human-readable name, safe to call with any id. */
	std::string GetMaskName(EMaskId Mask);
}
