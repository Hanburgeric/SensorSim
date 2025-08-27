#pragma once

#include "CoreMinimal.h"
#include "LiDAR/LidarShaderParameters.h"

class FLidarMissShader : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FLidarMissShader)
	SHADER_USE_PARAMETER_STRUCT(FLidarMissShader, FGlobalShader)

	using FParameters = FLidarShaderParameters;

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsRayTracingEnabledForProject(Parameters.Platform);
	}
};

IMPLEMENT_GLOBAL_SHADER(FLidarMissShader, "/Plugin/UESensorShaders/Shaders/LidarMiss.usf", "LidarMiss", SF_RayMiss);
