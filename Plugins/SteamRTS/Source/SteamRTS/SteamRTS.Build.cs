// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SteamRTS : ModuleRules
{
	public SteamRTS(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"OnlineSubsystem",
				"OnlineSubsystemUtils",
			}
			);


		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Slate",
				"SlateCore",
				"UMG",
				"Sockets",
				"Networking",
				"DeveloperSettings",
			}
			);


		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				"OnlineSubsystemSteam",
			}
			);
	}
}
