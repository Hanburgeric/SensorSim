using UnrealBuildTool;

public class SensorSim : ModuleRules
{
	public SensorSim(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput",
				"ChaosVehicles", "PhysicsCore",
				"UESensors"
			}
		);
	}
}
