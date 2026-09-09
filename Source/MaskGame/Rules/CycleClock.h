// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.
//
// Engine-free implementation of the three-day cycle clock.
//
// This header intentionally depends on nothing but the C++ standard library so
// that it can be compiled both into the MaskGame Unreal module and into the
// standalone rules test binary under Tests/Standalone.

#pragma once

#include <cstdint>

namespace MaskGame
{
	/** Minutes of in-game time per in-game hour. */
	inline constexpr int32_t MinutesPerHour = 60;
	/** Hours per in-game day. */
	inline constexpr int32_t HoursPerDay = 24;
	/** Days in one cycle before the moon falls. */
	inline constexpr int32_t DaysPerCycle = 3;
	/** Minutes in a full day. */
	inline constexpr int32_t MinutesPerDay = MinutesPerHour * HoursPerDay;
	/** Total minutes in one cycle: 3 days of 24 hours. */
	inline constexpr int32_t MinutesPerCycle = MinutesPerDay * DaysPerCycle;

	/** The cycle opens at 06:00 on the first day. */
	inline constexpr int32_t CycleStartHour = 6;
	/** Dawn: night ends, most townsfolk wake. */
	inline constexpr int32_t DawnHour = 6;
	/** Dusk: night begins, shops close, night-only actors spawn. */
	inline constexpr int32_t DuskHour = 18;
	/** The Final Hours begin at midnight on the third day. */
	inline constexpr int32_t FinalHoursLengthInHours = 6;

	/** Rate at which in-game time advances, selectable by ocarina songs. */
	enum class ETimeFlow : uint8_t
	{
		/** Default flow: one in-game hour per 45 seconds of real time. */
		Normal,
		/** Hymn of Return played backwards: time crawls at one third speed. */
		Slowed,
		/** Cutscenes, menus and dialogue: the clock is held. */
		Paused,
	};

	/** Which half of the day it is; drives lighting, schedules and spawns. */
	enum class EDayPhase : uint8_t
	{
		Day,
		Night,
	};

	/**
	 * A decoded point in the cycle.
	 *
	 * The day counter turns at 06:00, not at midnight, so a night belongs to the
	 * day whose evening it began: 23:59 on day three is followed by 00:00 on day
	 * three, and only 06:00 would have started a day four the game does not have.
	 */
	struct FCycleTime
	{
		/** 1-based day index, clamped to [1, DaysPerCycle]. */
		int32_t Day = 1;
		/** 0-23. */
		int32_t Hour = CycleStartHour;
		/** 0-59. */
		int32_t Minute = 0;

		bool operator==(const FCycleTime& Other) const
		{
			return Day == Other.Day && Hour == Other.Hour && Minute == Other.Minute;
		}
		bool operator!=(const FCycleTime& Other) const { return !(*this == Other); }
	};

	/**
	 * The three-day clock.
	 *
	 * Time is stored as whole minutes elapsed since the cycle opened at 06:00 on
	 * day one, so the range [0, MinutesPerCycle] maps onto day 1 06:00 through
	 * the moment of impact 72 hours later. Fractional minutes accumulate in a
	 * separate accumulator so that Advance() can be driven directly from a
	 * variable frame delta without drifting.
	 */
	class FCycleClock
	{
	public:
		FCycleClock() = default;

		/** Real-time seconds that one in-game hour takes at ETimeFlow::Normal. */
		static constexpr double RealSecondsPerHourNormal = 45.0;
		/** Multiplier applied to elapsed real time when the flow is Slowed. */
		static constexpr double SlowedFlowScale = 1.0 / 3.0;

		/** Restart the cycle: day one, 06:00, normal flow. Used by the Hymn of Return. */
		void Reset();

		/**
		 * Advance the clock by a real-time delta.
		 * @param RealDeltaSeconds  Unscaled real seconds elapsed this frame.
		 * @return Whole in-game minutes that elapsed, for callers that tick schedules.
		 */
		int32_t Advance(double RealDeltaSeconds);

		/** Jump straight to a point in the cycle. Never moves the clock backwards within a cycle. */
		void SetElapsedMinutes(int32_t InMinutes);

		/**
		 * Song of Double Time: skip to the next dawn or dusk, whichever comes first.
		 * @return Minutes skipped.
		 */
		int32_t SkipToNextPhaseBoundary();

		void SetFlow(ETimeFlow InFlow) { Flow = InFlow; }
		ETimeFlow GetFlow() const { return Flow; }

		/** Minutes elapsed since the cycle opened, in [0, MinutesPerCycle]. */
		int32_t GetElapsedMinutes() const { return ElapsedMinutes; }

		/** Minutes remaining until the moon lands. Zero once it has. */
		int32_t GetRemainingMinutes() const { return MinutesPerCycle - ElapsedMinutes; }

		/** Decode the current position in the cycle into day/hour/minute. */
		FCycleTime GetTime() const;

		EDayPhase GetPhase() const;
		bool IsNight() const { return GetPhase() == EDayPhase::Night; }

		/** True once the clock has run out and the moon has struck. */
		bool IsMoonFallen() const { return ElapsedMinutes >= MinutesPerCycle; }

		/** True during the last six hours before impact: midnight to 06:00 on day three. */
		bool IsFinalHours() const
		{
			return GetRemainingMinutes() <= FinalHoursLengthInHours * MinutesPerHour && !IsMoonFallen();
		}

		/** Convert a day/hour/minute triple into minutes elapsed since the cycle opened. */
		static int32_t ToElapsedMinutes(const FCycleTime& Time);

		/** Decode minutes elapsed into a day/hour/minute triple. */
		static FCycleTime FromElapsedMinutes(int32_t InMinutes);

	private:
		int32_t ElapsedMinutes = 0;
		double FractionalMinutes = 0.0;
		ETimeFlow Flow = ETimeFlow::Normal;
	};
}
