// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Maedan : ModuleRules
{
	public Maedan(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "GameplayAbilities", "GameplayTasks",
			"GameplayTags", "UMG", "OnlineSubsystem", "OnlineSubsystemSteam", "NavigationSystem"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[]
		{
			"Maedan/Public",
			"Maedan/Public/Player",
			"Maedan/Public/Player/Components",
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}