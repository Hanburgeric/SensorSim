#include "Sensors/LiDAR/RayTracingStrategy.h"

// UE
#include "RenderGraphBuilder.h"

// UESensors
#include "Sensors/LiDAR/LidarSensor.h"

// UESensorShaders
#include "LiDAR/LidarShaders.h"

namespace uesensors {
namespace lidar {

TArray<FLidarPoint> RayTracingStrategy::ExecuteScan(const ULidarSensor& LidarSensor)
{
    TArray<FLidarPoint> ScanResults;

	if (const UWorld* World{ LidarSensor.GetWorld() })
	{
		// Get the world location and rotation for the sensor
		const FVector SensorLocation{ LidarSensor.GetComponentLocation() };
		const FQuat SensorRotation{ LidarSensor.GetComponentQuat() };

		// Get the directions in which to perform the scan
		const TArray<FVector3f>& SampleDirections{ LidarSensor.GetSampleDirections() };

		// Initialize as many points as there are samples; this is an overestimation since samples can miss
		// (i.e. not hit anything), but this is preferable to multiple memory reallocations
		const int32 NumSamples{ SampleDirections.Num() };
		ScanResults.SetNum(NumSamples);

		// Get the range of the LiDAR
		const float MinRange{ LidarSensor.MinRange };
		const float MaxRange{ LidarSensor.MaxRange };
		
		// Execute scan on render thread
		ENQUEUE_RENDER_COMMAND(LidarScan)(
			[&](FRHICommandListImmediate& RHICmdList)
			{
				FRDGBuilder GraphBuilder(RHICmdList);

				// Allocate shader parameters
				FLidarShaderParameters* Parameters{ GraphBuilder.AllocParameters<FLidarShaderParameters>() };
				
				// Configure shader input parameters
				Parameters->TLAS;
				Parameters->SensorLocation = FVector3f{ SensorLocation };
				Parameters->SensorRotation = FVector4f{ FVector4{ SensorRotation.X, SensorRotation.Y, SensorRotation.Z, SensorRotation.W } };
				Parameters->SampleDirections;
				Parameters->NumSamples = static_cast<uint32>(NumSamples);
				Parameters->MinRange = MinRange;
				Parameters->MaxRange = MaxRange;

				// Configure shader output parameters
				Parameters->RTScanResults;

				// Get LiDAR shaders
				TShaderMapRef<FLidarRayGenShader> RayGenShader{ GetGlobalShaderMap(GMaxRHIFeatureLevel) };
				TShaderMapRef<FLidarMissShader> MissShader{ GetGlobalShaderMap(GMaxRHIFeatureLevel) };
				TShaderMapRef<FLidarClosestHitShader> ClosestHitShader{ GetGlobalShaderMap(GMaxRHIFeatureLevel) };
				
				GraphBuilder.Execute();
			}
		);

		// Wait for GPU completion to read back results
		FlushRenderingCommands();

		// TODO: Read back results from GPU

		// Remove all points that did failed to hit anything in a single pass
		// to create a clean set of results
		ScanResults.RemoveAll(
			[](const FLidarPoint& Point)
			{
				return !Point.bHit;
			}
		);
	}
	
	return ScanResults;
}

} // namespace lidar
} // namespace uesensors
