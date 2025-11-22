// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Asymptomagickal : ModuleRules
{
	public Asymptomagickal(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"GameplayAbilities"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { 
			"EsotericUser",
			"GameplayTags",
			"GameplayTasks",
			"UMG",
			"OnlineSubsystem",
			"OnlineSubsystemUtils",
			"Iris",
			"NetCore",
		});
		
		SetupIrisSupport(Target);

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
