// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Demo : ModuleRules
{
	public Demo(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore" });

		// Using Slate UI
		PublicDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Using Custom Web Browser
		//PublicDependencyModuleNames.AddRange(new string[] { "CustomWebBrowser" });
	}
}
