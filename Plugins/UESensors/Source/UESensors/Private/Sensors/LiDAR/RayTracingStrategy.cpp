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
	if (!RenderScene)
	{
		UE_LOG(LogLiDARSensor, Warning, TEXT("Render scene unavailable; cannot execute LiDAR scan."));
		return ScanResults;
	}

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
		[SensorLocation, SensorRotation, NumSamples, MinRange, MaxRange](FRHICommandListImmediate& RHICmdList)
		{
			FRDGBuilder GraphBuilder(RHICmdList);

			// Allocate shader parameters
			FLidarRayGenShader::FParameters* Parameters{ GraphBuilder.AllocParameters<FLidarRayGenShader::FParameters>() };

			// Configure shader input parameters
			Parameters->TLAS;
			Parameters->SensorLocation = FVector3f{ SensorLocation };
			Parameters->SensorRotation = FVector4f{ FVector4{ SensorRotation.X, SensorRotation.Y, SensorRotation.Z, SensorRotation.W } };
			Parameters->SampleDirections;
			Parameters->NumSamples = NumSamples;
			Parameters->MinRange = MinRange;
			Parameters->MaxRange = MaxRange;

			// Configure shader output parameters
			Parameters->GPUScanResults;

			// Add ray trace pass
			GraphBuilder.AddPass(
				RDG_EVENT_NAME("LidarRTScan"), Parameters, ERDGPassFlags::Compute,
				[NumSamples](FRHIRayTracingCommandList& RHICmdList)
				{
					// Get global shaders
					FGlobalShaderMap* GlobalShaderMap{ GetGlobalShaderMap(GMaxRHIFeatureLevel) };
					TShaderMapRef<FLidarRayGenShader> RayGenShader{ GlobalShaderMap };
					TShaderMapRef<FLidarMissShader> MissShader{ GlobalShaderMap };
					TShaderMapRef<FLidarClosestHitShader> ClosestHitShader{ GlobalShaderMap };

					// Configure and initializer pipeline
					FRayTracingPipelineStateInitializer PSInitializer{};
					PSInitializer.MaxPayloadSizeInBytes = GetRayTracingPayloadTypeMaxSize(FLidarRayGenShader::GetRayTracingPayloadType(0));
					
					TArray<FRHIRayTracingShader*> RayGenShaderTable{};
					RayGenShaderTable.Emplace(RayGenShader.GetRayTracingShader());
					PSInitializer.SetRayGenShaderTable(RayGenShaderTable);

					TArray<FRHIRayTracingShader*> MissShaderTable{};
					MissShaderTable.Emplace(MissShader.GetRayTracingShader());
					PSInitializer.SetMissShaderTable(MissShaderTable);

					TArray<FRHIRayTracingShader*> HitGroupTable{};
					HitGroupTable.Emplace(ClosestHitShader.GetRayTracingShader());
					PSInitializer.SetHitGroupTable(HitGroupTable);

					FRayTracingPipelineState* Pipeline{ PipelineStateCache::GetAndOrCreateRayTracingPipelineState(RHICmdList, PSInitializer) };

					// Configure and initialize shader binding table
					FRayTracingShaderBindingTableInitializer SBTInitializer{};

					FRHIShaderBindingTable* SBT{ RHICmdList.CreateRayTracingShaderBindingTable(SBTInitializer) };

					// Configure and initialize batched shader parameters
					FRHIBatchedShaderParameters BSP{ RHICmdList.GetScratchShaderParameters() };

					// Dispatch rays
					RHICmdList.RayTraceDispatch(
						Pipeline, RayGenShader.GetRayTracingShader(), SBT, BSP, NumSamples, 1
					);
				}
			);

			GraphBuilder.Execute();
		}
	);

	// Wait for GPU completion to read back results; note that this blocks the game thread, so
	// it may be advisable to design a system that doesn't reqire waiting for performance
	FlushRenderingCommands();

	// Convert scan results from GPU to CPU
	check(GPUScanResults.Num() == ScanResults.Num());
	for (int32 i{ 0 }; i < GPUScanResults.Num(); ++i)
	{
		const LidarPoint32Aligned& Src{ GPUScanResults[i] };
		FLidarPoint& Dst{ ScanResults[i] };

		Dst.XYZ = Src.XYZ;
		Dst.Intensity = 0.0F;
		Dst.RGB = FColor::Black;
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
