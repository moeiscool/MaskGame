// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

#include "TestHarness.h"

#include "SongMatcher.h"

#include <algorithm>

using namespace MaskGame;

namespace
{
	/** Play a whole song at a steady tempo and return whatever the matcher recognised. */
	FSongMatch Play(FSongMatcher& Matcher, const std::vector<ENote>& Notes, double Interval = 0.5, double StartTime = 10.0)
	{
		FSongMatch Result;
		double Time = StartTime;
		for (const ENote Note : Notes)
		{
			Result = Matcher.PushNote(Note, Time);
			Time += Interval;
		}
		return Result;
	}

	std::vector<ENote> Reversed(const std::vector<ENote>& Notes)
	{
		return std::vector<ENote>(Notes.rbegin(), Notes.rend());
	}

	void TestLibraryIsUnambiguous()
	{
		// The matcher fires the instant the tail of its buffer completes a song, so
		// no song may be a prefix of another (the shorter one would fire first and
		// swallow the buffer) and none may be a suffix of another (the longer one
		// could never be reached). This test is the guard on that invariant: adding
		// a song with a colliding note sequence fails here rather than in playtest.
		TEST_CASE("no song's notes occur inside another song's notes");
		const std::vector<FSongDefinition>& Library = GetSongLibrary();

		// "A is a suffix of some prefix of B" is exactly "A occurs somewhere inside
		// B", so substring containment is the whole invariant in one check.
		const auto Contains = [](const std::vector<ENote>& Haystack, const std::vector<ENote>& Needle)
		{
			return !Needle.empty() && Needle.size() <= Haystack.size()
				&& std::search(Haystack.begin(), Haystack.end(), Needle.begin(), Needle.end()) != Haystack.end();
		};

		for (const FSongDefinition& A : Library)
		{
			CHECK(!A.Notes.empty());
			CHECK(A.Id != ESongId::None);

			// The reversed hymn must not collide with anything either, since the
			// matcher will happily recognise it out of the same buffer.
			const std::vector<ENote> AReversed = Reversed(A.Notes);

			for (const FSongDefinition& B : Library)
			{
				if (A.Id == B.Id)
				{
					continue;
				}
				++::MaskGameTest::Ctx().Checks;
				if (Contains(B.Notes, A.Notes))
				{
					::MaskGameTest::Fail(__FILE__, __LINE__, A.DisplayName + " occurs inside " + B.DisplayName);
				}
				if (A.bHasReversedForm && Contains(B.Notes, AReversed))
				{
					::MaskGameTest::Fail(__FILE__, __LINE__, A.DisplayName + " reversed occurs inside " + B.DisplayName);
				}
			}
		}

		TEST_CASE("every song has a definition and a name");
		for (const FSongDefinition& Song : Library)
		{
			const FSongDefinition* Found = FindSong(Song.Id);
			CHECK(Found != nullptr);
			CHECK(!Song.DisplayName.empty());
		}
		CHECK(FindSong(ESongId::None) == nullptr);
	}

	void TestRecognisesEverySong()
	{
		TEST_CASE("every song in the library is recognised when played");
		for (const FSongDefinition& Song : GetSongLibrary())
		{
			FSongMatcher Matcher;
			const FSongMatch Match = Play(Matcher, Song.Notes);
			CHECK(Match.IsValid());
			CHECK(Match.Id == Song.Id);
			CHECK(Match.Performance == EPerformance::Forward);
			// A recognised song consumes the buffer so the next note starts clean.
			CHECK(Matcher.GetBuffer().empty());
		}
	}

	void TestFumbledNotesStillResolve()
	{
		TEST_CASE("a song still resolves after a few wrong notes");
		FSongMatcher Matcher;
		const FSongDefinition* Hymn = FindSong(ESongId::HymnOfReturn);
		CHECK(Hymn != nullptr);

		double Time = 1.0;
		for (const ENote Note : { ENote::Down, ENote::Down, ENote::A })
		{
			CHECK(!Matcher.PushNote(Note, Time).IsValid());
			Time += 0.5;
		}

		FSongMatch Match;
		for (const ENote Note : Hymn->Notes)
		{
			Match = Matcher.PushNote(Note, Time);
			Time += 0.5;
		}
		CHECK(Match.Id == ESongId::HymnOfReturn);
	}

	void TestSilenceAbandonsThePerformance()
	{
		TEST_CASE("a long pause abandons a half-played song");
		FSongMatcher Matcher;
		const FSongDefinition* Hymn = FindSong(ESongId::HymnOfReturn);

		double Time = 1.0;
		for (size_t i = 0; i + 1 < Hymn->Notes.size(); ++i)
		{
			Matcher.PushNote(Hymn->Notes[i], Time);
			Time += 0.5;
		}
		CHECK(!Matcher.GetBuffer().empty());

		// Wait out the timeout, then play the final note: nothing should match.
		Time += FSongMatcher::NoteTimeoutSeconds + 0.5;
		const FSongMatch Match = Matcher.PushNote(Hymn->Notes.back(), Time);
		CHECK(!Match.IsValid());
		CHECK_EQ(static_cast<int>(Matcher.GetBuffer().size()), 1);

		TEST_CASE("Tick clears a stale buffer on its own");
		Matcher.Tick(Time + FSongMatcher::NoteTimeoutSeconds + 1.0);
		CHECK(Matcher.GetBuffer().empty());

		TEST_CASE("Clear drops a performance in progress");
		Matcher.PushNote(ENote::A, 100.0);
		Matcher.Clear();
		CHECK(Matcher.GetBuffer().empty());
	}

	void TestReversedAndDoubledPerformances()
	{
		const FSongDefinition* Hymn = FindSong(ESongId::HymnOfReturn);
		CHECK(Hymn != nullptr);
		CHECK(Hymn->bHasReversedForm);
		CHECK(Hymn->bHasDoubledForm);

		TEST_CASE("the hymn played backwards is the reversed performance");
		{
			FSongMatcher Matcher;
			const FSongMatch Match = Play(Matcher, Reversed(Hymn->Notes));
			CHECK(Match.Id == ESongId::HymnOfReturn);
			CHECK(Match.Performance == EPerformance::Reversed);
		}

		TEST_CASE("the hymn played briskly is the doubled performance");
		{
			FSongMatcher Matcher;
			const FSongMatch Match = Play(Matcher, Hymn->Notes, FSongMatcher::DoubledNoteIntervalSeconds * 0.5);
			CHECK(Match.Id == ESongId::HymnOfReturn);
			CHECK(Match.Performance == EPerformance::Doubled);
		}

		TEST_CASE("a song without a reversed form is not matched backwards");
		{
			const FSongDefinition* Elegy = FindSong(ESongId::ElegyOfHollows);
			CHECK(Elegy != nullptr);
			CHECK(!Elegy->bHasReversedForm);

			FSongMatcher Matcher;
			const FSongMatch Match = Play(Matcher, Reversed(Elegy->Notes));
			CHECK(!Match.IsValid());
		}

		TEST_CASE("a song without a doubled form stays a forward performance");
		{
			const FSongDefinition* Sonata = FindSong(ESongId::SonataOfRousing);
			FSongMatcher Matcher;
			const FSongMatch Match = Play(Matcher, Sonata->Notes, 0.01);
			CHECK(Match.Id == ESongId::SonataOfRousing);
			CHECK(Match.Performance == EPerformance::Forward);
		}
	}

	void TestBufferIsBounded()
	{
		TEST_CASE("the note buffer never grows without bound");
		FSongMatcher Matcher;
		double Time = 0.0;
		for (int i = 0; i < 500; ++i)
		{
			// Down/Down never completes any song, so nothing ever clears the buffer.
			Matcher.PushNote(ENote::Down, Time);
			Time += 0.1;
		}
		CHECK(Matcher.GetBuffer().size() <= FSongMatcher::MaxBufferedNotes);
	}
}

int main()
{
	std::printf("SongMatcher\n");
	TestLibraryIsUnambiguous();
	TestRecognisesEverySong();
	TestFumbledNotesStillResolve();
	TestSilenceAbandonsThePerformance();
	TestReversedAndDoubledPerformances();
	TestBufferIsBounded();
	return MaskGameTest::Report("SongMatcher");
}
