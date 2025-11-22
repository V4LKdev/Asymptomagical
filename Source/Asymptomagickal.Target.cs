// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class AsymptomagickalTarget : TargetRules
{
	public AsymptomagickalTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		
		bUsesSteam = true;
		bUseIris = true;
		
		ExtraModuleNames.Add("Asymptomagickal");
	}
}
