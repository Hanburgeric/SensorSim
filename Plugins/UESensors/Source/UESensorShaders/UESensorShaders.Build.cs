using UnrealBuildTool;

public class UESensorShaders : ModuleRules
{
	public UESensorShaders(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
            {
				"Projects",
				"Core", "Engine",
				"RenderCore"
			}
		);
	}
}
