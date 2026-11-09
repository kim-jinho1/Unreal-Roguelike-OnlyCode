// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Unreal_Roguelike : ModuleRules
{
	public Unreal_Roguelike(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(new string[]
		{
            "Unreal_Roguelike"
        });

        PublicDependencyModuleNames.AddRange(new string[] 
		{ 
			"Core", 
			"CoreUObject",
			"Engine", 
			"InputCore", 
			"EnhancedInput",
            "GameplayTags",
            "UMG",
            "Slate",
            "SlateCore",
            "AIModule",
            "NavigationSystem",
            "GameplayTasks",
            "EngineCameras"
        });

		PrivateDependencyModuleNames.AddRange(new string[] {  });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
