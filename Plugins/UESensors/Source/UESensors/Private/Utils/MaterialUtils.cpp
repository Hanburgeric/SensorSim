#include "Utils/MaterialUtils.h"

bool UMaterialUtils::DoesMaterialHaveParameter(
	const UMaterial* Material,
	EMaterialParameterType ParameterType,
	FName ParameterName
)
{
	// Get all parameters from the material of the specified type
	TMap<FMaterialParameterInfo, FMaterialParameterMetadata> MaterialParameters{};
	Material->GetAllParametersOfType(ParameterType, MaterialParameters);

	// Attempt to find a parameter with a matching name
	for (const auto& Parameter : MaterialParameters)
	{
		// Matching parameter found
		if (Parameter.Key.Name == ParameterName)
		{
			return true;
		}
	}

	// No matching parameter found
	return false;
}
