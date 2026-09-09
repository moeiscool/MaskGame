// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

using UnrealBuildTool;

public class MaskGameTarget : TargetRules
{
	public MaskGameTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_5;
		ExtraModuleNames.Add("MaskGame");
	}
}
