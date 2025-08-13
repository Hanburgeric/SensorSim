#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MaterialUtils.generated.h"

UCLASS()
class UESENSORS_API UMaterialUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static bool DoesMaterialHaveParameter(
		const UMaterial* Material,
		EMaterialParameterType ParameterType,
		FName ParameterName
	);
};
