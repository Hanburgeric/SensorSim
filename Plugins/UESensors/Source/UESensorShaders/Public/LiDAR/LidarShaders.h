#pragma once

#include "CoreMinimal.h"
#include "ShaderParameterStruct.h"
#include "RayTracingPayloadType.h"

// Output data structure for the GPU, aligned to 32-bytes
struct LidarPoint32Aligned
{
	FVector3f XYZ{ 0.0F, 0.0F, 0.0F };
	float Intensity{ 0.0F };
	uint32 RGB{ 0x00000000U };
	uint32 bHit{ 0U };
	FUint32Vector2 Padding{ 0U, 0U };
};

// LiDAR ray generation shader
class UESENSORSHADERS_API FLidarRayGenShader : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FLidarRayGenShader)
	SHADER_USE_ROOT_PARAMETER_STRUCT(FLidarRayGenShader, FGlobalShader)

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_BUFFER_SRV(RaytracingAccelerationStructure, TLAS)
		SHADER_PARAMETER(FVector3f, SensorLocation)
		SHADER_PARAMETER(FVector4f, SensorRotation)
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<FVector3f>, SampleDirections)
		SHADER_PARAMETER(uint32, NumSamples)
		SHADER_PARAMETER(float, MinRange)
		SHADER_PARAMETER(float, MaxRange)

		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<LidarPoint32Aligned>, GPUScanResults)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return ShouldCompileRayTracingShadersForProject(Parameters.Platform);
	}

	static ERayTracingPayloadType GetRayTracingPayloadType(const int32 PermutationId)
	{
		return ERayTracingPayloadType::Minimal;
	}
};

// LiDAR miss shader
class UESENSORSHADERS_API FLidarMissShader : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FLidarMissShader)
	SHADER_USE_ROOT_PARAMETER_STRUCT(FLidarMissShader, FGlobalShader)

	using FParameters = FEmptyShaderParameters;

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return ShouldCompileRayTracingShadersForProject(Parameters.Platform);
	}

	static ERayTracingPayloadType GetRayTracingPayloadType(const int32 PermutationId)
	{
		return FLidarRayGenShader::GetRayTracingPayloadType(PermutationId);
	}
};

// LiDAR closest hit shader
class UESENSORSHADERS_API FLidarClosestHitShader : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FLidarClosestHitShader)
	SHADER_USE_ROOT_PARAMETER_STRUCT(FLidarClosestHitShader, FGlobalShader)

	using FParameters = FEmptyShaderParameters;

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return ShouldCompileRayTracingShadersForProject(Parameters.Platform);
	}

	static ERayTracingPayloadType GetRayTracingPayloadType(const int32 PermutationId)
	{
		return FLidarRayGenShader::GetRayTracingPayloadType(PermutationId);
	}
};
