#pragma once

#include "CoreMinimal.h"
#include "ShaderParameterStruct.h"

// Output data structure for the GPU, aligned to 32-bytes
struct LidarPointAligned
{
	FVector3f XYZ{ 0.0F, 0.0F, 0.0F };
	float Intensity{ 0.0F };
	uint32 RGB{ 0x00000000 };
	uint32 bHit{ 0U };
	FUintVector2 Padding{ 0U, 0U };
};

// Parameter structure for LiDAR shaders
BEGIN_SHADER_PARAMETER_STRUCT(FLidarShaderParameters, )
	SHADER_PARAMETER_SRV(RaytracingAccelerationStructure, TLAS)
	SHADER_PARAMETER(FVector3f, SensorLocation)
	SHADER_PARAMETER(FVector4f, SensorRotation)
	SHADER_PARAMETER_SRV(StructuredBuffer<FVector3f>, SampleDirections)
	SHADER_PARAMETER(uint32, NumSamples)
	SHADER_PARAMETER(float, MinRange)
	SHADER_PARAMETER(float, MaxRange)

	SHADER_PARAMETER_UAV(RWStructuredBuffer<LidarPointAligned>, RTScanResults)
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
