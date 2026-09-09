// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

#include "TestHarness.h"

#include "CycleClock.h"

#include <cstdlib>

using namespace MaskGame;

namespace
{
	void TestOpensAtDawnOfTheFirstDay()
	{
		TEST_CASE("the cycle opens at 06:00 on day one");
		FCycleClock Clock;
		const FCycleTime Now = Clock.GetTime();
		CHECK_EQ(Now.Day, 1);
		CHECK_EQ(Now.Hour, 6);
		CHECK_EQ(Now.Minute, 0);
		CHECK(!Clock.IsNight());
		CHECK(!Clock.IsFinalHours());
		CHECK(!Clock.IsMoonFallen());
		CHECK_EQ(Clock.GetRemainingMinutes(), MinutesPerCycle);
	}

	void TestNormalFlowRate()
	{
		TEST_CASE("45 real seconds is one in-game hour at normal flow");
		FCycleClock Clock;
		const int32_t Elapsed = Clock.Advance(FCycleClock::RealSecondsPerHourNormal);
		CHECK_EQ(Elapsed, 60);
		CHECK_EQ(Clock.GetTime().Hour, 7);
	}

	void TestSlowedFlowRate()
	{
		TEST_CASE("the reversed hymn slows time to one third");
		FCycleClock Clock;
		Clock.SetFlow(ETimeFlow::Slowed);
		Clock.Advance(FCycleClock::RealSecondsPerHourNormal);
		CHECK_EQ(Clock.GetElapsedMinutes(), 20);

		TEST_CASE("a paused clock does not move");
		Clock.SetFlow(ETimeFlow::Paused);
		CHECK_EQ(Clock.Advance(600.0), 0);
		CHECK_EQ(Clock.GetElapsedMinutes(), 20);
	}

	void TestNoDriftAcrossManySmallTicks()
	{
		TEST_CASE("many small frame deltas accumulate without drift");
		// Half an hour of real time, once as a single delta and once as 60fps
		// frames. Dropping the sub-minute remainder each frame would lose roughly
		// half a minute per frame here, so any real drift shows up enormous; the
		// carried remainder should keep the two within the one minute of
		// quantisation that whole-minute ticks allow.
		constexpr double RealSeconds = 30.0 * 60.0;
		constexpr int FrameCount = 30 * 60 * 60;

		FCycleClock Coarse;
		FCycleClock Fine;

		Coarse.Advance(RealSeconds);
		for (int i = 0; i < FrameCount; ++i)
		{
			Fine.Advance(RealSeconds / FrameCount);
		}

		CHECK_EQ(Coarse.GetElapsedMinutes(), 2400);
		CHECK(std::abs(Fine.GetElapsedMinutes() - Coarse.GetElapsedMinutes()) <= 1);
	}

	void TestDayAndNight()
	{
		TEST_CASE("night runs from 18:00 to 06:00");
		FCycleClock Clock;

		Clock.SetElapsedMinutes(FCycleClock::ToElapsedMinutes({ 1, 17, 59 }));
		CHECK(!Clock.IsNight());

		Clock.SetElapsedMinutes(FCycleClock::ToElapsedMinutes({ 1, 18, 0 }));
		CHECK(Clock.IsNight());

		Clock.SetElapsedMinutes(FCycleClock::ToElapsedMinutes({ 2, 5, 59 }));
		CHECK(Clock.IsNight());

		Clock.SetElapsedMinutes(FCycleClock::ToElapsedMinutes({ 2, 6, 0 }));
		CHECK(!Clock.IsNight());
	}

	void TestDoubleTimeSkip()
	{
		TEST_CASE("double time skips to the next dawn or dusk");
		FCycleClock Clock;

		// 07:00 on day one -> dusk the same day.
		Clock.SetElapsedMinutes(FCycleClock::ToElapsedMinutes({ 1, 7, 0 }));
		const int32_t SkippedToDusk = Clock.SkipToNextPhaseBoundary();
		CHECK_EQ(SkippedToDusk, 11 * 60);
		CHECK_EQ(Clock.GetTime().Hour, 18);
		CHECK_EQ(Clock.GetTime().Day, 1);

		// 19:00 on day one -> dawn on day two.
		Clock.SetElapsedMinutes(FCycleClock::ToElapsedMinutes({ 1, 19, 0 }));
		Clock.SkipToNextPhaseBoundary();
		CHECK_EQ(Clock.GetTime().Day, 2);
		CHECK_EQ(Clock.GetTime().Hour, 6);

		// Skipping never runs past the moment of impact.
		Clock.SetElapsedMinutes(FCycleClock::ToElapsedMinutes({ 3, 23, 0 }));
		Clock.SkipToNextPhaseBoundary();
		CHECK(Clock.IsMoonFallen());
		CHECK_EQ(Clock.GetRemainingMinutes(), 0);
	}

	void TestDayRollsOverAtDawnNotMidnight()
	{
		TEST_CASE("the day counter turns at 06:00, so a night keeps its own day");
		FCycleClock Clock;

		// Midnight of the first night is still the first day, twelve hours after
		// the cycle opened at 06:00 rather than a fresh day.
		Clock.SetElapsedMinutes(FCycleClock::ToElapsedMinutes({ 1, 0, 0 }));
		CHECK_EQ(Clock.GetElapsedMinutes(), 18 * 60);
		CHECK_EQ(Clock.GetTime().Day, 1);
		CHECK_EQ(Clock.GetTime().Hour, 0);
		CHECK(Clock.IsNight());

		// 05:59 is the last minute of the first day; 06:00 begins the second.
		Clock.SetElapsedMinutes(FCycleClock::ToElapsedMinutes({ 1, 5, 59 }));
		CHECK_EQ(Clock.GetElapsedMinutes(), MinutesPerDay - 1);
		CHECK_EQ(Clock.GetTime().Day, 1);

		Clock.SetElapsedMinutes(MinutesPerDay);
		CHECK_EQ(Clock.GetTime().Day, 2);
		CHECK_EQ(Clock.GetTime().Hour, 6);
	}

	void TestFinalHours()
	{
		TEST_CASE("the final hours are the last six before impact");
		FCycleClock Clock;

		Clock.SetElapsedMinutes(FCycleClock::ToElapsedMinutes({ 3, 17, 59 }));
		CHECK(!Clock.IsFinalHours());

		// One minute before the third night's midnight: still not the final hours.
		Clock.SetElapsedMinutes(FCycleClock::ToElapsedMinutes({ 3, 23, 59 }));
		CHECK(!Clock.IsFinalHours());
		CHECK_EQ(Clock.GetRemainingMinutes(), 6 * 60 + 1);

		// Midnight of the third night, six hours out.
		Clock.SetElapsedMinutes(FCycleClock::ToElapsedMinutes({ 3, 0, 0 }));
		CHECK(Clock.IsFinalHours());
		CHECK_EQ(Clock.GetRemainingMinutes(), 6 * 60);

		Clock.SetElapsedMinutes(FCycleClock::ToElapsedMinutes({ 3, 5, 59 }));
		CHECK(Clock.IsFinalHours());
		CHECK_EQ(Clock.GetRemainingMinutes(), 1);
	}

	void TestMoonFallAndReset()
	{
		TEST_CASE("the clock stops at impact and the hymn rewinds it");
		FCycleClock Clock;
		Clock.Advance(FCycleClock::RealSecondsPerHourNormal * 100.0); // Far past 72 hours.

		CHECK(Clock.IsMoonFallen());
		CHECK(!Clock.IsFinalHours()); // Already fallen: no longer counting down.
		CHECK_EQ(Clock.GetElapsedMinutes(), MinutesPerCycle);
		CHECK_EQ(Clock.Advance(60.0), 0); // A fallen moon does not tick.

		Clock.Reset();
		CHECK(!Clock.IsMoonFallen());
		CHECK_EQ(Clock.GetElapsedMinutes(), 0);
		CHECK_EQ(Clock.GetTime().Day, 1);
		CHECK_EQ(Clock.GetTime().Hour, 6);
	}

	void TestTimeRoundTrip()
	{
		TEST_CASE("day/hour/minute round-trips through elapsed minutes");
		for (int32_t Minutes = 0; Minutes <= MinutesPerCycle; Minutes += 7)
		{
			const FCycleTime Decoded = FCycleClock::FromElapsedMinutes(Minutes);
			const int32_t Encoded = FCycleClock::ToElapsedMinutes(Decoded);
			// The very last minute decodes to day three 06:00, which also encodes to
			// the start of day three; every other point round-trips exactly.
			if (Minutes < MinutesPerCycle)
			{
				CHECK_EQ(Encoded, Minutes);
			}
			CHECK(Decoded.Day >= 1 && Decoded.Day <= DaysPerCycle);
			CHECK(Decoded.Hour >= 0 && Decoded.Hour < HoursPerDay);
			CHECK(Decoded.Minute >= 0 && Decoded.Minute < MinutesPerHour);
		}

		TEST_CASE("times past the end of the cycle clamp to the moment of impact");
		CHECK_EQ(FCycleClock::ToElapsedMinutes({ 9, 0, 0 }), MinutesPerCycle);
		CHECK_EQ(FCycleClock::ToElapsedMinutes({ 0, 6, 0 }), 0);
	}
}

int main()
{
	std::printf("CycleClock\n");
	TestOpensAtDawnOfTheFirstDay();
	TestNormalFlowRate();
	TestSlowedFlowRate();
	TestNoDriftAcrossManySmallTicks();
	TestDayAndNight();
	TestDayRollsOverAtDawnNotMidnight();
	TestDoubleTimeSkip();
	TestFinalHours();
	TestMoonFallAndReset();
	TestTimeRoundTrip();
	return MaskGameTest::Report("CycleClock");
}
