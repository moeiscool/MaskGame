// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

#include "World/SongListenerInterface.h"

bool ISongListenerInterface::OnSongHeard_Implementation(ESongType /*Song*/, ESongPerformance /*Performance*/, AMaskCharacter* /*Performer*/)
{
	return false;
}

float ISongListenerInterface::GetHearingRadius_Implementation() const
{
	// Roughly a courtyard: far enough to play to a whole room, short enough that
	// two puzzles in neighbouring rooms do not answer the same performance.
	return 2000.0f;
}
