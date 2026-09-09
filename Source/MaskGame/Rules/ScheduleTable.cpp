// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

#include "ScheduleTable.h"

#include <algorithm>

namespace MaskGame
{
	void FScheduleTable::Add(const std::string& ActorId,
	                         const std::string& RegionId,
	                         const std::string& Activity,
	                         const FCycleTime& Start,
	                         const FCycleTime& End,
	                         const std::string& RequiredFlag,
	                         int32_t Priority)
	{
		FScheduleEntry Entry;
		Entry.ActorId = ActorId;
		Entry.RegionId = RegionId;
		Entry.Activity = Activity;
		Entry.StartMinute = FCycleClock::ToElapsedMinutes(Start);
		Entry.EndMinute = FCycleClock::ToElapsedMinutes(End);
		Entry.RequiredFlag = RequiredFlag;
		Entry.Priority = Priority;

		// An entry authored as ending "before" it starts runs to the end of the cycle;
		// that is how an all-night window past the day boundary is expressed.
		if (Entry.EndMinute <= Entry.StartMinute)
		{
			Entry.EndMinute = MinutesPerCycle;
		}

		Entries.push_back(std::move(Entry));
	}

	bool FScheduleTable::IsEligible(const FScheduleEntry& Entry, const IFlagSource* Flags) const
	{
		if (Entry.RequiredFlag.empty())
		{
			return true;
		}
		return Flags != nullptr && Flags->IsFlagSet(Entry.RequiredFlag);
	}

	const FScheduleEntry* FScheduleTable::FindEntry(const std::string& ActorId,
	                                                int32_t ElapsedMinute,
	                                                const IFlagSource* Flags) const
	{
		const FScheduleEntry* Best = nullptr;
		for (const FScheduleEntry& Entry : Entries)
		{
			if (Entry.ActorId != ActorId || !Entry.Contains(ElapsedMinute) || !IsEligible(Entry, Flags))
			{
				continue;
			}
			if (Best == nullptr || Entry.Priority > Best->Priority)
			{
				Best = &Entry;
			}
		}
		return Best;
	}

	std::vector<const FScheduleEntry*> FScheduleTable::FindOccupants(const std::string& RegionId,
	                                                                 int32_t ElapsedMinute,
	                                                                 const IFlagSource* Flags) const
	{
		// Resolve per actor first so that a shadowed low-priority entry in this
		// region does not appear alongside the branch that replaced it.
		std::vector<std::string> Seen;
		std::vector<const FScheduleEntry*> Out;

		for (const FScheduleEntry& Entry : Entries)
		{
			if (std::find(Seen.begin(), Seen.end(), Entry.ActorId) != Seen.end())
			{
				continue;
			}
			Seen.push_back(Entry.ActorId);

			if (const FScheduleEntry* Winner = FindEntry(Entry.ActorId, ElapsedMinute, Flags))
			{
				if (Winner->RegionId == RegionId)
				{
					Out.push_back(Winner);
				}
			}
		}
		return Out;
	}

	std::vector<const FScheduleEntry*> FScheduleTable::GetDay(const std::string& ActorId) const
	{
		std::vector<const FScheduleEntry*> Out;
		for (const FScheduleEntry& Entry : Entries)
		{
			if (Entry.ActorId == ActorId)
			{
				Out.push_back(&Entry);
			}
		}
		std::sort(Out.begin(), Out.end(), [](const FScheduleEntry* A, const FScheduleEntry* B)
		{
			return A->StartMinute < B->StartMinute;
		});
		return Out;
	}
}
