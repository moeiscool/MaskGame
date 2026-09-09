// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.

using System.IO;
using UnrealBuildTool;

public class MaskGame : ModuleRules
{
	public MaskGame(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"UMG",
			"GameplayTags",
			"AIModule",
			"NavigationSystem",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate",
			"SlateCore",
		});

		// The Rules folder is plain C++ with no engine dependency, shared verbatim
		// with the standalone test build under Tests/Standalone. Exposing it as a
		// public include path lets game code say #include "CycleClock.h".
		PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Rules"));
		PublicIncludePaths.Add(ModuleDirectory);
	}
}
