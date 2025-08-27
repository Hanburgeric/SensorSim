#pragma once

#include "CoreMinimal.h"
#include "LiDAR/LidarShaderParameters.h"

class FLidarRayGenShader : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FLidarRayGenShader)
	SHADER_USE_PARAMETER_STRUCT(FLidarRayGenShader, FGlobalShader)

	using FParameters = FLidarShaderParameters;

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsRayTracingEnabledForProject(Parameters.Platform);
	}
};

IMPLEMENT_GLOBAL_SHADER(FLidarRayGenShader, "/Plugin/UESensorShaders/Shaders/LidarRayGen.usf", "LidarRayGen", SF_RayGen);
