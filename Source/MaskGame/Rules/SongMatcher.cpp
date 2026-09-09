// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

#include "SongMatcher.h"

#include <algorithm>

namespace MaskGame
{
	const std::vector<FSongDefinition>& GetSongLibrary()
	{
		using N = ENote;
		static const std::vector<FSongDefinition> Library = {
			{ ESongId::HymnOfReturn,      "Hymn of Return",      { N::Up, N::Left, N::Right, N::Up, N::Left, N::Right }, true, true },
			{ ESongId::HymnOfMending,     "Hymn of Mending",     { N::Left, N::Right, N::Down, N::Left, N::Right, N::Down }, false, false },
			{ ESongId::MaresCall,         "Mare's Call",         { N::A, N::Up, N::Left, N::A, N::Up, N::Left }, false, false },
			{ ESongId::SonataOfRousing,   "Sonata of Rousing",   { N::Up, N::A, N::Left, N::Right, N::Down, N::A }, false, false },
			{ ESongId::StoneheartLullaby, "Stoneheart Lullaby",  { N::A, N::Right, N::Left, N::A, N::Right, N::Left, N::Down }, false, false },
			{ ESongId::TidecallAria,      "Tidecall Aria",       { N::Left, N::Up, N::Left, N::Right, N::Down, N::Left, N::Right }, false, false },
			{ ESongId::ElegyOfHollows,    "Elegy of Hollows",    { N::Right, N::Left, N::Right, N::Down, N::Right, N::Up, N::Left }, false, false },
			{ ESongId::OathOfConcord,     "Oath of Concord",     { N::Down, N::A, N::Right, N::Down, N::A, N::Right, N::Up }, false, false },
			{ ESongId::WindfeatherSong,   "Windfeather Song",    { N::Down, N::Left, N::Up, N::Right, N::Down, N::Left, N::Up }, false, false },
			{ ESongId::SongOfStorms,      "Song of Storms",      { N::A, N::Down, N::Up, N::A, N::Down, N::Up }, false, false },
		};
		return Library;
	}

	const FSongDefinition* FindSong(ESongId Id)
	{
		for (const FSongDefinition& Song : GetSongLibrary())
		{
			if (Song.Id == Id)
			{
				return &Song;
			}
		}
		return nullptr;
	}

	namespace
	{
		/** True when Notes ends with Pattern. */
		bool EndsWith(const std::vector<ENote>& Notes, const std::vector<ENote>& Pattern)
		{
			if (Pattern.empty() || Pattern.size() > Notes.size())
			{
				return false;
			}
			return std::equal(Pattern.rbegin(), Pattern.rend(), Notes.rbegin());
		}

		/** True when Notes ends with Pattern played backwards. */
		bool EndsWithReversed(const std::vector<ENote>& Notes, const std::vector<ENote>& Pattern)
		{
			if (Pattern.empty() || Pattern.size() > Notes.size())
			{
				return false;
			}
			return std::equal(Pattern.begin(), Pattern.end(), Notes.rbegin());
		}
	}

	void FSongMatcher::Clear()
	{
		Buffer.clear();
		NoteTimes.clear();
		bHasNote = false;
	}

	void FSongMatcher::Tick(double TimeSeconds)
	{
		if (bHasNote && TimeSeconds - LastNoteTime > NoteTimeoutSeconds)
		{
			Clear();
		}
	}

	FSongMatch FSongMatcher::PushNote(ENote Note, double TimeSeconds)
	{
		// A long enough pause abandons whatever was being played.
		if (bHasNote && TimeSeconds - LastNoteTime > NoteTimeoutSeconds)
		{
			Clear();
		}

		Buffer.push_back(Note);
		NoteTimes.push_back(TimeSeconds);
		LastNoteTime = TimeSeconds;
		bHasNote = true;

		if (Buffer.size() > MaxBufferedNotes)
		{
			Buffer.erase(Buffer.begin());
			NoteTimes.erase(NoteTimes.begin());
		}

		// Longest match wins, so that a song ending in another song's notes still
		// resolves to the longer of the two.
		FSongMatch Best;
		size_t BestLength = 0;

		for (const FSongDefinition& Song : GetSongLibrary())
		{
			if (Song.Notes.size() <= BestLength)
			{
				continue;
			}

			if (EndsWith(Buffer, Song.Notes))
			{
				Best.Id = Song.Id;
				Best.Performance = EPerformance::Forward;
				BestLength = Song.Notes.size();
			}
			else if (Song.bHasReversedForm && EndsWithReversed(Buffer, Song.Notes))
			{
				Best.Id = Song.Id;
				Best.Performance = EPerformance::Reversed;
				BestLength = Song.Notes.size();
			}
		}

		if (!Best.IsValid())
		{
			return Best;
		}

		// A forward performance played briskly is the doubled variant. Measure across
		// the notes that actually formed the match rather than the whole buffer.
		const FSongDefinition* Definition = FindSong(Best.Id);
		if (Definition && Definition->bHasDoubledForm && Best.Performance == EPerformance::Forward && BestLength >= 2)
		{
			const size_t FirstIndex = NoteTimes.size() - BestLength;
			const double Span = NoteTimes.back() - NoteTimes[FirstIndex];
			const double AverageInterval = Span / static_cast<double>(BestLength - 1);
			if (AverageInterval <= DoubledNoteIntervalSeconds)
			{
				Best.Performance = EPerformance::Doubled;
			}
		}

		Clear();
		return Best;
	}
}
