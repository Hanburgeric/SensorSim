#pragma once

#include "CoreMinimal.h"
#include "LiDAR/LidarShaderParameters.h"

class FLidarClosestHitShader : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FLidarClosestHitShader)
	SHADER_USE_PARAMETER_STRUCT(FLidarClosestHitShader, FGlobalShader)

	using FParameters = FLidarShaderParameters;

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsRayTracingEnabledForProject(Parameters.Platform);
	}
};

IMPLEMENT_GLOBAL_SHADER(FLidarClosestHitShader, "/Plugin/UESensorShaders/Shaders/LidarClosestHit.usf", "LidarClosestHit", SF_RayHitGroup);
