// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class GDTSurvivor : ModuleRules
{
	public GDTSurvivor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "UMG", "Niagara", "NavigationSystem" });


		// private -> only build in .cpp
		PrivateDependencyModuleNames.AddRange(new string[] { "EnhancedInput" });

		// Tile baking in EndlessTileAuthoring (editor only)
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "AssetRegistry", "Slate", "SlateCore" });
		}

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
