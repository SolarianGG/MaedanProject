// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class RTSProject : ModuleRules
{
	public RTSProject(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore",
			"RealTimeStrategy", "SteamRTS", "UMG", "OnlineSubsystem"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
	}
}
