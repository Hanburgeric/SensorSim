#pragma once

#include "CoreMinimal.h"
#include "ShaderParameterStruct.h"

// Define parameter structure for LiDAR shaders
BEGIN_SHADER_PARAMETER_STRUCT(FLidarShaderParameters, )
	SHADER_PARAMETER_SRV(RaytracingAccelerationStructure, TLAS)

	SHADER_PARAMETER(FVector3f, SensorLocation)
	SHADER_PARAMETER(FVector4f, SensorRotation)
	SHADER_PARAMETER_SRV(StructuredBuffer<FVector3f>, SampleDirections)
	SHADER_PARAMETER(int32, NumSamples)
	SHADER_PARAMETER(float, MinRange)
	SHADER_PARAMETER(float, MaxRange)
END_SHADER_PARAMETER_STRUCT()
