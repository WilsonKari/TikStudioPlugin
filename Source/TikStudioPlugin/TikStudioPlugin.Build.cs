// Wilson Karl 2025

using UnrealBuildTool;

public class TikStudioPlugin : ModuleRules
{
	public TikStudioPlugin(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// Add public include paths required here
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// Add other private include paths required here
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"TikFinityPlugin" // Needed to reference TikFinity public structs in conversion nodes
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"UMG",
				"UnrealEd",
				"ToolMenus",
				"EditorStyle",
				"EditorWidgets",
				"PropertyEditor",
				"Kismet",
				"KismetCompiler",
				"BlueprintGraph",
				"ToolMenus"
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// Add any modules that your module loads dynamically here
			}
			);

		// Enable RTTI for this module (required for some Blueprint features)
		bUseRTTI = true;
	}
}
