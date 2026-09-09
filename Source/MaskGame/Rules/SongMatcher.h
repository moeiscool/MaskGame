// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.
//
// Engine-free ocarina song recognition. See CycleClock.h for why this layer
// avoids Unreal types.

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace MaskGame
{
	/** The five ocarina notes, mapped to the face and shoulder inputs at the UE layer. */
	enum class ENote : uint8_t
	{
		Left,
		Right,
		Up,
		Down,
		A,
	};

	/** Identifiers for every song the player can learn. Kept in walkthrough order. */
	enum class ESongId : uint8_t
	{
		None,
		/** Rewinds the cycle to dawn of the first day, banking permanent progress. */
		HymnOfReturn,
		/** Draws the spirit out of a cursed soul and leaves a mask behind. */
		HymnOfMending,
		/** Calls the mare across the fields. */
		MaresCall,
		/** Wakes the sleeping temple out of the mire. */
		SonataOfRousing,
		/** Quiets a raging heart; puts the mountainfolk to sleep. */
		StoneheartLullaby,
		/** Summons the tide and the creatures that swim it. */
		TidecallAria,
		/** Leaves a hollow statue of the current form behind to hold a switch. */
		ElegyOfHollows,
		/** Binds the four guardians and unmakes the moon. */
		OathOfConcord,
		/** Warps to any owl statue already touched. */
		WindfeatherSong,
		/** Calls rain, and with it the hidden springs. */
		SongOfStorms,
	};

	/** A learnable song: an identifier, a display name and the notes that play it. */
	struct FSongDefinition
	{
		ESongId Id = ESongId::None;
		std::string DisplayName;
		std::vector<ENote> Notes;
		/** True if playing the notes in reverse produces a distinct effect. */
		bool bHasReversedForm = false;
		/** True if playing the notes at double tempo produces a distinct effect. */
		bool bHasDoubledForm = false;
	};

	/** How a recognised song was performed. Drives the three variants of the Hymn of Return. */
	enum class EPerformance : uint8_t
	{
		Forward,
		Reversed,
		Doubled,
	};

	/** The outcome of feeding a note into the matcher. */
	struct FSongMatch
	{
		ESongId Id = ESongId::None;
		EPerformance Performance = EPerformance::Forward;
		bool IsValid() const { return Id != ESongId::None; }
	};

	/** The canonical song list, used by both the runtime and the tests. */
	const std::vector<FSongDefinition>& GetSongLibrary();

	/** Look up a definition by id. Returns nullptr when the id is unknown. */
	const FSongDefinition* FindSong(ESongId Id);

	/**
	 * Rolling note-sequence recogniser.
	 *
	 * Notes are pushed one at a time as the player performs them. The matcher
	 * compares the tail of its buffer against every known song and reports the
	 * longest match, so that a song whose notes end with a shorter song's notes
	 * still resolves to the longer one. A gap longer than NoteTimeoutSeconds
	 * between notes clears the buffer, which is what lets a player abandon a
	 * half-played song by simply stopping.
	 */
	class FSongMatcher
	{
	public:
		/** Silence longer than this ends the current performance. */
		static constexpr double NoteTimeoutSeconds = 2.0;
		/** Notes closer together than this are a "doubled" performance. */
		static constexpr double DoubledNoteIntervalSeconds = 0.28;
		/** Longest song the buffer needs to hold. */
		static constexpr size_t MaxBufferedNotes = 16;

		/**
		 * Push a note.
		 * @param Note              The note performed.
		 * @param TimeSeconds       Monotonic timestamp of the performance, in real seconds.
		 * @return The song recognised by this note, if any.
		 */
		FSongMatch PushNote(ENote Note, double TimeSeconds);

		/** Drop any partially played song. Called when the ocarina is put away. */
		void Clear();

		/** Clear the buffer if too long has passed since the last note. */
		void Tick(double TimeSeconds);

		const std::vector<ENote>& GetBuffer() const { return Buffer; }

	private:
		std::vector<ENote> Buffer;
		std::vector<double> NoteTimes;
		double LastNoteTime = 0.0;
		bool bHasNote = false;
	};
}
