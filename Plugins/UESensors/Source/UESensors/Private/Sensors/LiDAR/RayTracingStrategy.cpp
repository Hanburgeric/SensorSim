#include "Sensors/LiDAR/RayTracingStrategy.h"

// UE
#include "Runtime/Renderer/Private/ScenePrivate.h"
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

	// Early return condition(s)
	const UWorld* World{ LidarSensor.GetWorld() };
	const FSceneInterface* Scene{ World ? World->Scene : nullptr };
	const FScene* RenderScene{ Scene ? Scene->GetRenderScene() : nullptr };
	if (!RenderScene || !RenderScene->RayTracingScene.IsCreated())
	{
		UE_LOG(LogLiDARSensor, Warning, TEXT("Ray tracing scene unavailable; cannot execute LiDAR scan."));
		return ScanResults;
	}

	// Get the ray tracing scene
	const FRayTracingScene& RayTracingScene{ RenderScene->RayTracingScene };

	// Get the world location and rotation for the sensor
	const FVector SensorLocation{ LidarSensor.GetComponentLocation() };
	const FQuat SensorRotation{ LidarSensor.GetComponentQuat() };

	// Get the directions in which to perform the scan, but as a copy to ensure lifetime across threads
	const TArray<FVector3f> SampleDirections{ LidarSensor.GetSampleDirections() };

	// Initialize as many points as there are samples; this is an overestimation since samples can miss
	// (i.e. not hit anything), but this is preferable to multiple memory reallocations
	const int32 NumSamples{ SampleDirections.Num() };
	ScanResults.SetNum(NumSamples);

	// Get the range of the LiDAR
	const float MinRange{ LidarSensor.MinRange };
	const float MaxRange{ LidarSensor.MaxRange };

	// Create buffer for the GPU to fill
	TArray<LidarPoint32Aligned> GPUScanResults{};
	GPUScanResults.SetNum(NumSamples);

	// Execute scan on render thread
	ENQUEUE_RENDER_COMMAND(LidarRTScan)(
		[SensorLocation, SensorRotation, SampleDirections, NumSamples, MinRange, MaxRange, GPUScanResults](FRHICommandListImmediate& RHICmdList)
		{
			FRDGBuilder GraphBuilder(RHICmdList);

			// Allocate sample directions input buffer
			const FRDGBufferRef SampleDirectionsBuffer{
				CreateStructuredBuffer(GraphBuilder, TEXT("SampleDirections"), sizeof(FVector3f), NumSamples, SampleDirections.GetData(), sizeof(FVector3f) * NumSamples)
			};

			// Allocate GPU scan results output buffer
			const FRDGBufferRef GPUScanResultsBuffer{
				GraphBuilder.CreateBuffer(
					FRDGBufferDesc::CreateStructuredDesc(sizeof(LidarPoint32Aligned), NumSamples), TEXT("GPUScanResults")
				)
			};

			// Allocate shader parameters
			FLidarRayGenShader::FParameters* Parameters{ GraphBuilder.AllocParameters<FLidarRayGenShader::FParameters>() };

			// Configure shader input parameters
			Parameters->TLAS;
			Parameters->SensorLocation = FVector3f{ SensorLocation };
			Parameters->SensorRotation = FVector4f{ FVector4{ SensorRotation.X, SensorRotation.Y, SensorRotation.Z, SensorRotation.W } };
			Parameters->SampleDirections = GraphBuilder.CreateSRV(SampleDirectionsBuffer);
			Parameters->NumSamples = NumSamples;
			Parameters->MinRange = MinRange;
			Parameters->MaxRange = MaxRange;

			// Configure shader output parameters
			Parameters->GPUScanResults = GraphBuilder.CreateUAV(GPUScanResultsBuffer);

			// Add ray trace pass
			GraphBuilder.AddPass(
				RDG_EVENT_NAME("LidarRTScan"), Parameters, ERDGPassFlags::Compute,
				[&](FRHIRayTracingCommandList& RHIRTCmdList)
				{
					// ???
				}
			);

			GraphBuilder.Execute();
		}
	);

	// Wait for the GPU to completion to read back results
	FlushRenderingCommands();

	// Convert scan results from GPU to CPU
	check(GPUScanResults.Num() == ScanResults.Num());
	for (int32 i{ 0 }; i < GPUScanResults.Num(); ++i)
	{
		const LidarPoint32Aligned& Src{ GPUScanResults[i] };
		FLidarPoint& Dst{ ScanResults[i] };

		Dst.XYZ = Src.XYZ;
		Dst.Intensity = Src.Intensity;
		Dst.RGB = FColor{ (Src.RGB >> 16U) & 0xFFU, (Src.RGB >> 8U) & 0xFFU, Src.RGB & 0xFFU, 255U };
		Dst.bHit = (Src.bHit != 0U);
	}

	// Remove all points that failed to hit anything in a single pass
	// to create a clean set of results
	ScanResults.RemoveAll(
		[](const FLidarPoint& Point)
		{
			return !Point.bHit;
		}
	);
	
	return ScanResults;
}

} // namespace lidar
} // namespace uesensors
