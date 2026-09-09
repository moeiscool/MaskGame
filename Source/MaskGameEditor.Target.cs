// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

using UnrealBuildTool;

public class MaskGameEditorTarget : TargetRules
{
	public MaskGameEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_5;
		ExtraModuleNames.Add("MaskGame");
	}
}
