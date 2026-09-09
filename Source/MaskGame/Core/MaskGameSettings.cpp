// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

#include "Core/MaskGameSettings.h"

UMaskGameSettings::UMaskGameSettings()
{
	CategoryName = TEXT("Game");
	SectionName = TEXT("Mask Game");
}

const UMaskGameSettings& UMaskGameSettings::Get()
{
	const UMaskGameSettings* Settings = GetDefault<UMaskGameSettings>();
	check(Settings);
	return *Settings;
}
