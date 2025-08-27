#pragma once

#include "CoreMinimal.h"
#include "ShaderParameterStruct.h"

// Parameter structure for LiDAR shaders
BEGIN_SHADER_PARAMETER_STRUCT(FLidarShaderParameters, )
	SHADER_PARAMETER_SRV(RaytracingAccelerationStructure, TLAS)
	SHADER_PARAMETER(FVector3f, SensorLocation)
	SHADER_PARAMETER(FVector4f, SensorRotation)
	SHADER_PARAMETER_SRV(StructuredBuffer<FVector3f>, SampleDirections)
	SHADER_PARAMETER(uint32, NumSamples)
	SHADER_PARAMETER(float, MinRange)
	SHADER_PARAMETER(float, MaxRange)

	// TODO: output buffer
END_SHADER_PARAMETER_STRUCT()

// LiDAR ray generation shader
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
IMPLEMENT_GLOBAL_SHADER(FLidarRayGenShader, "/Plugin/UESensorShaders/Shaders/LidarShaders.usf", "LidarRayGen", SF_RayGen);

// LiDAR miss shader
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
IMPLEMENT_GLOBAL_SHADER(FLidarMissShader, "/Plugin/UESensorShaders/Shaders/LidarShaders.usf", "LidarMiss", SF_RayMiss);

// LiDAR closest hit shader
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
IMPLEMENT_GLOBAL_SHADER(FLidarClosestHitShader, "/Plugin/UESensorShaders/Shaders/LidarShaders.usf", "LidarClosestHit", SF_RayHitGroup);
