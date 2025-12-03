using UnrealBuildTool;

public class Demo : ModuleRules
{
	public Demo(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
			{ "Core", "CoreUObject", "Engine", "InputCore", "CustomWebBrowser" });

		// Using Slate UI
		PublicDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
	}
}