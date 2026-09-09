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
			// Public because MaskPlayerController.h exposes the touch overlay,
			// which is a Slate widget.
			"Slate",
			"SlateCore",
			"ApplicationCore",
		});

		// The Rules folder is plain C++ with no engine dependency, shared verbatim
		// with the standalone test build under Tests/Standalone. Exposing it as a
		// public include path lets game code say #include "CycleClock.h".
		PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Rules"));
		PublicIncludePaths.Add(ModuleDirectory);
	}
}
