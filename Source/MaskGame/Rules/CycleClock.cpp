// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

#include "CycleClock.h"

#include <algorithm>
#include <cmath>

namespace MaskGame
{
	namespace
	{
		/**
		 * The cycle opens at 06:00, so elapsed minute zero is 06:00 and the clock
		 * face is found by shifting into "minutes since midnight" before decoding.
		 * The day counter, by contrast, rolls at 06:00 rather than at midnight:
		 * the small hours of a night belong to the day that began the evening
		 * before, which is why 23:59 on day three comes *before* 00:00 on day
		 * three and the final hours begin at that midnight.
		 */
		constexpr int32_t StartOffsetMinutes = CycleStartHour * MinutesPerHour;

		/** Dawn and dusk are 12 hours apart, and elapsed minute zero is a dawn. */
		constexpr int32_t PhaseStrideMinutes = (DuskHour - DawnHour) * MinutesPerHour;
	}

	void FCycleClock::Reset()
	{
		ElapsedMinutes = 0;
		FractionalMinutes = 0.0;
		Flow = ETimeFlow::Normal;
	}

	int32_t FCycleClock::Advance(double RealDeltaSeconds)
	{
		if (Flow == ETimeFlow::Paused || RealDeltaSeconds <= 0.0 || IsMoonFallen())
		{
			return 0;
		}

		const double Scale = (Flow == ETimeFlow::Slowed) ? SlowedFlowScale : 1.0;
		const double MinutesPerRealSecond = static_cast<double>(MinutesPerHour) / RealSecondsPerHourNormal;

		FractionalMinutes += RealDeltaSeconds * MinutesPerRealSecond * Scale;

		// Only whole minutes are committed to the clock; the remainder is carried
		// into the next tick so that a long run of small deltas does not lose time.
		const double WholeMinutes = std::floor(FractionalMinutes);
		if (WholeMinutes <= 0.0)
		{
			return 0;
		}
		FractionalMinutes -= WholeMinutes;

		const int32_t Before = ElapsedMinutes;
		ElapsedMinutes = std::min(MinutesPerCycle, ElapsedMinutes + static_cast<int32_t>(WholeMinutes));
		return ElapsedMinutes - Before;
	}

	void FCycleClock::SetElapsedMinutes(int32_t InMinutes)
	{
		ElapsedMinutes = std::clamp(InMinutes, 0, MinutesPerCycle);
		FractionalMinutes = 0.0;
	}

	int32_t FCycleClock::SkipToNextPhaseBoundary()
	{
		if (IsMoonFallen())
		{
			return 0;
		}

		// Elapsed minute zero is a dawn, so every dawn and dusk falls on a multiple
		// of the 12-hour stride and the next boundary is a plain ceiling.
		const int32_t NextBoundary = ((ElapsedMinutes / PhaseStrideMinutes) + 1) * PhaseStrideMinutes;

		const int32_t Target = std::min(MinutesPerCycle, NextBoundary);
		const int32_t Skipped = Target - ElapsedMinutes;
		SetElapsedMinutes(Target);
		return Skipped;
	}

	FCycleTime FCycleClock::GetTime() const
	{
		return FromElapsedMinutes(ElapsedMinutes);
	}

	EDayPhase FCycleClock::GetPhase() const
	{
		const FCycleTime Now = GetTime();
		return (Now.Hour >= DawnHour && Now.Hour < DuskHour) ? EDayPhase::Day : EDayPhase::Night;
	}

	int32_t FCycleClock::ToElapsedMinutes(const FCycleTime& Time)
	{
		const int32_t DayIndex = std::max(0, Time.Day - 1);
		// Hours before 06:00 belong to the tail of their own day, not the head of it.
		const int32_t HourInDay = ((Time.Hour - CycleStartHour) % HoursPerDay + HoursPerDay) % HoursPerDay;
		const int32_t Minutes = DayIndex * MinutesPerDay + HourInDay * MinutesPerHour + Time.Minute;
		return std::clamp(Minutes, 0, MinutesPerCycle);
	}

	FCycleTime FCycleClock::FromElapsedMinutes(int32_t InMinutes)
	{
		const int32_t Clamped = std::clamp(InMinutes, 0, MinutesPerCycle);
		const int32_t ClockMinutes = (Clamped + StartOffsetMinutes) % MinutesPerDay;

		FCycleTime Out;
		// The instant of impact is 06:00 on a fourth day the game does not have;
		// report it as day three so no UI ever has to render one.
		Out.Day = std::min(DaysPerCycle, Clamped / MinutesPerDay + 1);
		Out.Hour = ClockMinutes / MinutesPerHour;
		Out.Minute = ClockMinutes % MinutesPerHour;
		return Out;
	}
}
