// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.
//
// Engine-free NPC scheduling. Every townsperson lives on a timetable that
// repeats identically each cycle, which is what makes the three days learnable
// and the notebook worth keeping.

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "CycleClock.h"

namespace MaskGame
{
	/** One appointment: an actor is somewhere, doing something, over a window of the cycle. */
	struct FScheduleEntry
	{
		/** Stable id of the NPC this entry belongs to. */
		std::string ActorId;
		/** Region the NPC occupies during the window. */
		std::string RegionId;
		/** Short verb phrase for the notebook, e.g. "waiting by the west gate". */
		std::string Activity;
		/** Inclusive start of the window, as minutes elapsed since the cycle opened. */
		int32_t StartMinute = 0;
		/** Exclusive end of the window. */
		int32_t EndMinute = 0;
		/**
		 * Flag that must be set for this entry to apply. Empty means unconditional.
		 * This is how a quest reroutes someone's day: give the branch a higher
		 * priority and gate it on the flag the player set.
		 */
		std::string RequiredFlag;
		/** Higher priority wins when two entries overlap. */
		int32_t Priority = 0;

		bool Contains(int32_t ElapsedMinute) const
		{
			return ElapsedMinute >= StartMinute && ElapsedMinute < EndMinute;
		}
	};

	/** Interface used to resolve an entry's RequiredFlag without depending on the progression type. */
	class IFlagSource
	{
	public:
		virtual ~IFlagSource() = default;
		virtual bool IsFlagSet(const std::string& Name) const = 0;
	};

	/** A queryable set of appointments for the whole cast. */
	class FScheduleTable
	{
	public:
		/** Add an entry using day/hour/minute bounds, which is how the CSV data is authored. */
		void Add(const std::string& ActorId,
		         const std::string& RegionId,
		         const std::string& Activity,
		         const FCycleTime& Start,
		         const FCycleTime& End,
		         const std::string& RequiredFlag = std::string(),
		         int32_t Priority = 0);

		void AddEntry(const FScheduleEntry& Entry) { Entries.push_back(Entry); }

		/**
		 * Where is this actor at this moment?
		 * @return The winning entry, or nullptr when the actor is off-stage.
		 */
		const FScheduleEntry* FindEntry(const std::string& ActorId,
		                                int32_t ElapsedMinute,
		                                const IFlagSource* Flags = nullptr) const;

		/** Every actor present in a region at a moment, one entry each. */
		std::vector<const FScheduleEntry*> FindOccupants(const std::string& RegionId,
		                                                 int32_t ElapsedMinute,
		                                                 const IFlagSource* Flags = nullptr) const;

		/** Every entry for one actor across the whole cycle, in chronological order. */
		std::vector<const FScheduleEntry*> GetDay(const std::string& ActorId) const;

		const std::vector<FScheduleEntry>& GetEntries() const { return Entries; }
		void Clear() { Entries.clear(); }
		size_t Num() const { return Entries.size(); }

	private:
		bool IsEligible(const FScheduleEntry& Entry, const IFlagSource* Flags) const;

		std::vector<FScheduleEntry> Entries;
	};
}
