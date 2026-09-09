// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

#include "TestHarness.h"

#include "ScheduleTable.h"

#include <set>
#include <string>

using namespace MaskGame;

namespace
{
	/** Minimal flag source so schedule branches can be exercised without the whole progression. */
	class FTestFlags : public IFlagSource
	{
	public:
		std::set<std::string> Set;
		bool IsFlagSet(const std::string& Name) const override { return Set.count(Name) > 0; }
	};

	int32_t At(int32_t Day, int32_t Hour, int32_t Minute = 0)
	{
		return FCycleClock::ToElapsedMinutes({ Day, Hour, Minute });
	}

	FScheduleTable MakeTownSchedule()
	{
		FScheduleTable Table;
		// A courier who works the town by day and sleeps at the inn by night.
		Table.Add("courier", "bellwether.plaza", "sorting the morning post", { 1, 6, 0 }, { 1, 18, 0 });
		Table.Add("courier", "bellwether.inn", "asleep in the back room", { 1, 18, 0 }, { 2, 6, 0 });
		Table.Add("courier", "bellwether.plaza", "running his route", { 2, 6, 0 }, { 2, 18, 0 });

		// An innkeeper who never leaves the desk on the first day.
		Table.Add("innkeeper", "bellwether.inn", "behind the desk", { 1, 6, 0 }, { 2, 6, 0 });
		return Table;
	}

	void TestBasicLookup()
	{
		TEST_CASE("an actor is found at the right place and time");
		const FScheduleTable Table = MakeTownSchedule();

		const FScheduleEntry* Morning = Table.FindEntry("courier", At(1, 9));
		CHECK(Morning != nullptr);
		CHECK(Morning->RegionId == "bellwether.plaza");
		CHECK(Morning->Activity == "sorting the morning post");

		const FScheduleEntry* Night = Table.FindEntry("courier", At(1, 22));
		CHECK(Night != nullptr);
		CHECK(Night->RegionId == "bellwether.inn");

		TEST_CASE("window bounds are inclusive at the start and exclusive at the end");
		CHECK(Table.FindEntry("courier", At(1, 6, 0))->RegionId == "bellwether.plaza");
		CHECK(Table.FindEntry("courier", At(1, 17, 59))->RegionId == "bellwether.plaza");
		CHECK(Table.FindEntry("courier", At(1, 18, 0))->RegionId == "bellwether.inn");

		TEST_CASE("an actor with no entry is off-stage");
		CHECK(Table.FindEntry("courier", At(3, 12)) == nullptr);
		CHECK(Table.FindEntry("nobody", At(1, 9)) == nullptr);
	}

	void TestOccupants()
	{
		TEST_CASE("a region reports everyone standing in it");
		const FScheduleTable Table = MakeTownSchedule();

		const auto Plaza = Table.FindOccupants("bellwether.plaza", At(1, 9));
		CHECK_EQ(static_cast<int>(Plaza.size()), 1);
		CHECK(Plaza[0]->ActorId == "courier");

		const auto Inn = Table.FindOccupants("bellwether.inn", At(1, 22));
		CHECK_EQ(static_cast<int>(Inn.size()), 2); // The courier asleep, the innkeeper on shift.

		const auto Empty = Table.FindOccupants("mirefen.trail", At(1, 9));
		CHECK(Empty.empty());
	}

	void TestFlagGatedBranches()
	{
		TEST_CASE("a quest flag reroutes someone's day");
		FScheduleTable Table = MakeTownSchedule();
		// Once the player delivers the letter, the courier spends the afternoon
		// waiting at the west gate instead of working the plaza.
		Table.Add("courier", "bellwether.westgate", "waiting for a reply",
		          { 1, 12, 0 }, { 1, 18, 0 }, "courier.letter.delivered", /*Priority=*/10);

		FTestFlags Flags;

		const FScheduleEntry* Before = Table.FindEntry("courier", At(1, 14), &Flags);
		CHECK(Before != nullptr);
		CHECK(Before->RegionId == "bellwether.plaza");

		Flags.Set.insert("courier.letter.delivered");
		const FScheduleEntry* After = Table.FindEntry("courier", At(1, 14), &Flags);
		CHECK(After != nullptr);
		CHECK(After->RegionId == "bellwether.westgate");

		TEST_CASE("the branch does not shadow hours it does not cover");
		CHECK(Table.FindEntry("courier", At(1, 9), &Flags)->RegionId == "bellwether.plaza");

		TEST_CASE("a region query respects the winning branch only");
		const auto Plaza = Table.FindOccupants("bellwether.plaza", At(1, 14), &Flags);
		CHECK(Plaza.empty());
		const auto Gate = Table.FindOccupants("bellwether.westgate", At(1, 14), &Flags);
		CHECK_EQ(static_cast<int>(Gate.size()), 1);

		TEST_CASE("without a flag source a gated entry never applies");
		CHECK(Table.FindEntry("courier", At(1, 14), nullptr)->RegionId == "bellwether.plaza");
	}

	void TestOpenEndedWindowRunsToImpact()
	{
		TEST_CASE("an end at or before the start runs to the moment of impact");
		FScheduleTable Table;
		Table.Add("watchman", "bellwether.tower", "counting down", { 3, 18, 0 }, { 3, 6, 0 });

		CHECK(Table.FindEntry("watchman", At(3, 20)) != nullptr);
		CHECK(Table.FindEntry("watchman", At(3, 23, 59)) != nullptr);
		CHECK(Table.FindEntry("watchman", At(3, 17, 59)) == nullptr);
	}

	void TestDayListing()
	{
		TEST_CASE("an actor's whole day comes back in order");
		const FScheduleTable Table = MakeTownSchedule();
		const auto Day = Table.GetDay("courier");
		CHECK_EQ(static_cast<int>(Day.size()), 3);
		for (size_t i = 1; i < Day.size(); ++i)
		{
			CHECK(Day[i - 1]->StartMinute <= Day[i]->StartMinute);
		}
		CHECK(Table.GetDay("nobody").empty());

		TEST_CASE("clearing empties the table");
		FScheduleTable Scratch = MakeTownSchedule();
		CHECK(Scratch.Num() > 0);
		Scratch.Clear();
		CHECK_EQ(static_cast<int>(Scratch.Num()), 0);
	}
}

int main()
{
	std::printf("ScheduleTable\n");
	TestBasicLookup();
	TestOccupants();
	TestFlagGatedBranches();
	TestOpenEndedWindowRunsToImpact();
	TestDayListing();
	return MaskGameTest::Report("ScheduleTable");
}
