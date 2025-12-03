using System.IO;
using UnrealBuildTool;

public class CustomWebBrowser : ModuleRules
{
	public CustomWebBrowser(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[]
			{ "Engine", "Core", "CoreUObject", "UMG", "Slate", "SlateCore", "WebBrowser" }
		);

		var pluginPath = Utils.MakePathRelativeTo(ModuleDirectory, Target.RelativeEnginePath);

		if (Target.Platform == UnrealTargetPlatform.Android)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "Launch" });

			AdditionalPropertiesForReceipt.Add(
				"AndroidPlugin",
				Path.Combine(pluginPath, "CustomWebBrowser_UPL_Android.xml")
			);
		}
		else if (Target.Platform == UnrealTargetPlatform.IOS)
		{
			AdditionalPropertiesForReceipt.Add(
				"IOSPlugin",
				Path.Combine(pluginPath, "CustomWebBrowser_UPL_iOS.xml")
			);
		}
	}
}