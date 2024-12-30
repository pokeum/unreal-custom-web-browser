using UnrealBuildTool;

public class CustomWebBrowser : ModuleRules
{
	public CustomWebBrowser(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[]
			{ "Engine", "Core", "CoreUObject", "UMG", "Slate", "SlateCore", "WebBrowser" }
		);
	}
}