// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class GDTSurvivor : ModuleRules
{
	public GDTSurvivor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "UMG", "Niagara", "NavigationSystem" });


		// private -> only build in .cpp
		// Slate/SlateCore: self-drawn HUD widgets (UI/SegmentedBar, UI/HUDRing)
		PrivateDependencyModuleNames.AddRange(new string[] { "EnhancedInput", "CommonUI", "DeveloperSettings", "Slate", "SlateCore" });

		// Tile baking in EndlessTileAuthoring (editor only)
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "AssetRegistry" });
		}

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
