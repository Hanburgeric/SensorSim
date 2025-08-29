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

	RayTracingStrategy::RayTracingStrategy()
	{
		// Register the delegate
		if (!PostOpaqueRenderDelegate.IsValid())
		{
			IRendererModule& Renderer{ FModuleManager::LoadModuleChecked<IRendererModule>("Renderer") };

			PostOpaqueRenderDelegate = Renderer.RegisterPostOpaqueRenderDelegate(
				FPostOpaqueRenderDelegate::CreateRaw(this, &RayTracingStrategy::PostOpaqueRender)
			);
		}
	}
	RayTracingStrategy::~RayTracingStrategy()
	{
		// Remove the delegate
		if (PostOpaqueRenderDelegate.IsValid())
		{
			IRendererModule& Renderer{ FModuleManager::LoadModuleChecked<IRendererModule>("Renderer") };

			Renderer.RemovePostOpaqueRenderDelegate(PostOpaqueRenderDelegate);
		}
	}

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

	// ???
	TUniquePtr<FRHIGPUBufferReadback> GPUBufferReadback{ MakeUnique<FRHIGPUBufferReadback>(TEXT("GPUBufferReadback")) };

	// Execute scan on render thread
	ENQUEUE_RENDER_COMMAND(LidarRTScan)(
		[this, RenderScene, SensorLocation, SensorRotation, SampleDirections, NumSamples, MinRange, MaxRange, GPUBufferReadbackPtr = GPUBufferReadback.Get()]
		(FRHICommandListImmediate& RHICmdList)
		{
			FRDGBuilder GraphBuilder(RHICmdList);

			// Allocate shader parameters
			FLidarRayGenShader::FParameters* Parameters{ GraphBuilder.AllocParameters<FLidarRayGenShader::FParameters>() };

			// Create buffer for sample directions
			FRDGBufferRef SampleDirectionsBuffer{
				GraphBuilder.CreateBuffer(
					FRDGBufferDesc::CreateStructuredDesc(sizeof(FVector3f), NumSamples),
					TEXT("SampleDirections")
				)
			};

			// Upload sample directions to the GPU
			GraphBuilder.QueueBufferUpload(SampleDirectionsBuffer, SampleDirections.GetData(), sizeof(FVector3f) * NumSamples);

			// Configure shader input parameters (except for the TLAS, which is configured later for reasons described below)
			Parameters->SensorLocation = FVector3f{ SensorLocation };
			Parameters->SensorRotation = FVector4f{ FVector4{ SensorRotation.X, SensorRotation.Y, SensorRotation.Z, SensorRotation.W } };
			Parameters->SampleDirections = GraphBuilder.CreateSRV(SampleDirectionsBuffer);
			Parameters->NumSamples = NumSamples;
			Parameters->MinRange = MinRange;
			Parameters->MaxRange = MaxRange;

			// Create buffer for GPU scan results
			FRDGBufferRef GPUScanResultsBuffer{
				GraphBuilder.CreateBuffer(
					FRDGBufferDesc::CreateStructuredDesc(sizeof(LidarPoint32Aligned), NumSamples),
					TEXT("GPUScanResults")
				)
			};

			// Configure shader output parameters
			Parameters->GPUScanResults = GraphBuilder.CreateUAV(GPUScanResultsBuffer);

			// Add ray trace pass
			GraphBuilder.AddPass(
				RDG_EVENT_NAME("LidarRTScan"), Parameters, ERDGPassFlags::Compute | ERDGPassFlags::NeverCull,
				[this, RenderScene, Parameters, NumSamples]
				(FRHIRayTracingCommandList& RHICmdList)
				{
					// Early return condition(s)
					if (!TLAS)
					{
						UE_LOG(LogLiDARSensor, Warning, TEXT("Ray tracing scene unavailable; unable to execute LiDAR scan."));
						return;
					}

					// Configure the TLAS shader input parameter; this must be done
					// inside the ray trace pass, otherwise the layer view will be invalid
					Parameters->TLAS = TLAS;

					// Get global shaders
					FGlobalShaderMap* GlobalShaderMap{ GetGlobalShaderMap(GMaxRHIFeatureLevel) };
					TShaderMapRef<FLidarRayGenShader> RayGenShader{ GlobalShaderMap };
					TShaderMapRef<FLidarMissShader> MissShader{ GlobalShaderMap };
					TShaderMapRef<FLidarClosestHitShader> ClosestHitShader{ GlobalShaderMap };

					// Configure and initialize pipeline
					FRayTracingPipelineStateInitializer PSInitializer{};
					PSInitializer.MaxPayloadSizeInBytes = sizeof(LidarPoint32Aligned);

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
					SBTInitializer.ShaderBindingMode = ERayTracingShaderBindingMode::RTPSO;
					SBTInitializer.HitGroupIndexingMode = ERayTracingHitGroupIndexingMode::Disallow;
					SBTInitializer.LocalBindingDataSize = 32U;

					FRHIShaderBindingTable* SBT{ RHICmdList.CreateRayTracingShaderBindingTable(SBTInitializer) };

					// Bind and commit shaders
					RHICmdList.SetRayTracingMissShader(SBT, 0U, Pipeline, 0U, 0U, nullptr, 0U);
					RHICmdList.SetRayTracingHitGroup(SBT, 0U, 0U, 0U, Pipeline, 0U, 0U, nullptr, 0U, nullptr, 0U);

					RHICmdList.CommitShaderBindingTable(SBT);

					// Configure and initialize batched shader parameters
					FRHIBatchedShaderParameters BSP{ RHICmdList.GetScratchShaderParameters() };

					// Dispatch rays
					RHICmdList.RayTraceDispatch(
						Pipeline, RayGenShader.GetRayTracingShader(), SBT, BSP, NumSamples, 1
					);
				}
			);

			//// Add GPU readback pass
			//AddReadbackBufferPass(
			//	GraphBuilder, RDG_EVENT_NAME("LidarGPUReadback"), GPUScanResultsBuffer,
			//	[GPUBufferReadbackPtr, GPUScanResultsBuffer, NumSamples](FRHICommandList& RHICmdList)
			//	{
			//		// Get the number of bytes to copy
			//		const uint32 NumBytes{ sizeof(LidarPoint32Aligned) * NumSamples };

			//		// Enqueue copying of the GPU scan results to the CPU
			//		GPUBufferReadbackPtr->EnqueueCopy(
			//			RHICmdList,
			//			GPUScanResultsBuffer->GetRHI(),
			//			NumBytes
			//		);
			//	}
			//);

			GraphBuilder.Execute();
		}
	);

	// Wait for GPU completion; note that this will block the game thread,
	// so a non-blocking mechanism may need to be implemented for better performance
	FlushRenderingCommands();

	//while (!GPUBufferReadback->IsReady()) { FPlatformProcess::Sleep(0.001F); }	// Absolutely terrible; will be removed later

	//// Readback GPU scan results and convert to CPU scan results
	//if (GPUBufferReadback->IsReady())
	//{
	//	// Get the total number of bytes to copy
	//	const uint32 NumBytes{ sizeof(LidarPoint32Aligned) * NumSamples };

	//	// Get the GPU scan results
	//	const LidarPoint32Aligned* GPUScanResults{
	//		static_cast<const LidarPoint32Aligned*>(GPUBufferReadback->Lock(NumBytes))
	//	};

	//	// Convert scan results to CPU struct
	//	for (int32 i{ 0 }; i < NumSamples; ++i)
	//	{
	//		const LidarPoint32Aligned& Src{ GPUScanResults[i] };
	//		FLidarPoint& Dst{ ScanResults[i] };

	//		Dst.XYZ = Src.XYZ;
	//		Dst.Intensity = 0.0F;
	//		Dst.RGB = FColor::Black;
	//		Dst.bHit = (Src.bHit != 0U);
	//	}

	//	GPUBufferReadback->Unlock();
	//}

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

void RayTracingStrategy::PostOpaqueRender(FPostOpaqueRenderParameters& Parameters)
{
	// Every frame, save the TLAS to be used when executing the scan
	const FViewInfo* View{ Parameters.View };
	const FSceneViewFamily* Family{ View ? View->Family : nullptr };
	const FSceneInterface* Scene{ Family ? Family->Scene : nullptr };
	const FScene* RenderScene{ Scene ? Scene->GetRenderScene() : nullptr };
	if (RenderScene && RenderScene->RayTracingScene.IsCreated())
	{
		TLAS = RenderScene->RayTracingScene.GetLayerView(ERayTracingSceneLayer::Base);
	}
}

} // namespace lidar
} // namespace uesensors
