using UnrealBuildTool;

public class UESensors : ModuleRules
{
	public UESensors(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core", "CoreUObject", "Engine",
				"RenderCore",
				"CinematicCamera", "CineCameraSceneCapture"
			}
		);
	}
}
