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

	// Create the GPU buffer readback
	ScanResultsReadback = MakeUnique<FRHIGPUBufferReadback>(TEXT("ScanResultsReadback"));
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
	// Update the world location and rotation for the sensor
	SensorLocation = LidarSensor.GetComponentLocation();
	SensorRotation = LidarSensor.GetComponentQuat();

	// Update the directions in which to perform the scan
	SampleDirections = LidarSensor.GetSampleDirections();

	// Update the number of samples
	NumSamples = SampleDirections.Num();

	// Update the range of the LiDAR
	MinRange = LidarSensor.MinRange;
	MaxRange = LidarSensor.MaxRange;
	
	// Remove all points that did failed to hit anything in a single pass
	// to create a clean set of results
	ScanResults.RemoveAll(
		[](const FLidarPoint& Point)
		{
			return !Point.bHit;
		}
	);
	
	// Return the scan results; this will be updated in PostOpaqueRender
	return ScanResults;
}

void RayTracingStrategy::PostOpaqueRender(FPostOpaqueRenderParameters& Parameters)
{
	// Since PostOpaqueRender is likely to be called far more frequently than is ExecuteScan,
	// it is possible for the LiDAR parameters to be in an uninitialized state, in which case
	// the scan should not be performed
	if (SampleDirections.IsEmpty() || NumSamples <= 0)
	{
		return;
	}
	
	FRDGBuilder* GraphBuilder{ Parameters.GraphBuilder };

	// Allocate shader parameters
	FLidarRayGenShader::FParameters* ShaderParameters{
		GraphBuilder->AllocParameters<FLidarRayGenShader::FParameters>()
	};

	// Create buffer for sample directions
	FRDGBufferRef SampleDirectionsBuffer{
		GraphBuilder->CreateBuffer(
			FRDGBufferDesc::CreateStructuredDesc(sizeof(FVector3f), NumSamples),
			TEXT("SampleDirections")
		)
	};

	// Upload sample directions to the GPU
	GraphBuilder->QueueBufferUpload(
		SampleDirectionsBuffer, SampleDirections.GetData(), sizeof(FVector3f) * NumSamples
	);

	// Configure shader input parameters (except for the TLAS, which must be configured in the actual ray tracing pass)
	ShaderParameters->SensorLocation = FVector3f{ SensorLocation };
	ShaderParameters->SensorRotation = FVector4f{ FVector4{
		SensorRotation.X, SensorRotation.Y, SensorRotation.Z, SensorRotation.W
	} };
	ShaderParameters->SampleDirections = GraphBuilder->CreateSRV(SampleDirectionsBuffer);
	ShaderParameters->NumSamples = NumSamples;
	ShaderParameters->MinRange = MinRange;
	ShaderParameters->MaxRange = MaxRange;

	// Create buffer for GPU scan results
	FRDGBufferRef GPUScanResultsBuffer{
		GraphBuilder->CreateBuffer(
			FRDGBufferDesc::CreateStructuredDesc(sizeof(LidarPoint32Aligned), NumSamples),
			TEXT("GPUScanResults")
		)
	};

	// Configure shader output parameters
	ShaderParameters->GPUScanResults = GraphBuilder->CreateUAV(GPUScanResultsBuffer);

	// Add ray trace dispatch pass
	GraphBuilder->AddPass(
		RDG_EVENT_NAME("LidarRTScan"), ShaderParameters, ERDGPassFlags::Compute | ERDGPassFlags::NeverCull,
		[this, Parameters, ShaderParameters](FRHIRayTracingCommandList& RHICmdList)
		{
			// Only proceed if the TLAS has been built
			const FViewInfo* View{ Parameters.View };
			const FSceneViewFamily* Family{ View ? View->Family : nullptr };
			const FSceneInterface* Scene{ Family ? Family->Scene : nullptr };
			const FScene* RenderScene{ Scene ? Scene->GetRenderScene() : nullptr };
			if (!RenderScene || !RenderScene->RayTracingScene.IsCreated())
			{
				UE_LOG(
					LogLiDARSensor, Warning,
					TEXT("Invalid ray tracing scene; cannot execute LiDAR ray tracing strategy.")
				);
				return;
			}

			// Configure the ray tracing shader input parameters
			ShaderParameters->PreViewTranslation = FVector3f{ View->ViewMatrices.GetPreViewTranslation() };
			ShaderParameters->TLAS = RenderScene->RayTracingScene.GetLayerView(ERayTracingSceneLayer::Base);

			// Get global ray tracing shaders
			const FGlobalShaderMap* GlobalShaderMap{ GetGlobalShaderMap(GMaxRHIFeatureLevel) };
			const TShaderMapRef<FLidarRayGenShader> RayGenShader{ GlobalShaderMap };
			const TShaderMapRef<FLidarMissShader> MissShader{ GlobalShaderMap };
			const TShaderMapRef<FLidarClosestHitShader> ClosestHitShader{ GlobalShaderMap };

			// Configure and initialize ray tracing pipeline
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

			FRayTracingPipelineState* Pipeline{
				PipelineStateCache::GetAndOrCreateRayTracingPipelineState(RHICmdList, PSInitializer)
			};

			// Configure and initialize ray tracing shader binding table
			FRayTracingShaderBindingTableInitializer SBTInitializer{};
			SBTInitializer.ShaderBindingMode = ERayTracingShaderBindingMode::RTPSO;
			SBTInitializer.HitGroupIndexingMode = ERayTracingHitGroupIndexingMode::Disallow;
			SBTInitializer.LocalBindingDataSize = 32U;

			FRHIShaderBindingTable* SBT{ RHICmdList.CreateRayTracingShaderBindingTable(SBTInitializer) };

			// Bind and commit ray tracing shaders
			RHICmdList.SetRayTracingMissShader(SBT, 0U, Pipeline, 0U, 0U, nullptr, 0U);
			RHICmdList.SetRayTracingHitGroup(SBT, 0U, nullptr, 0U, Pipeline, 0U, 0U, nullptr, 0U, nullptr, 0U);

			RHICmdList.CommitShaderBindingTable(SBT);

			// Configure and initialize batched shader parameters
			FRHIBatchedShaderParameters BSP{ RHICmdList.GetScratchShaderParameters() };
			SetShaderParameters(BSP, RayGenShader, *ShaderParameters);

			// Dispatch rays
			RHICmdList.RayTraceDispatch(
				Pipeline, RayGenShader.GetRayTracingShader(), SBT, BSP, ShaderParameters->NumSamples, 1U
			);
		}
	);

	// Add GPU buffer readback pass
	AddReadbackBufferPass(
		*GraphBuilder, RDG_EVENT_NAME("LidarRTReadback"), GPUScanResultsBuffer,
		[this, GPUScanResultsBuffer, ShaderParameters](FRHICommandListImmediate& RHICmdList)
		{
			// Only request a readback if one is not already in flight
			if (!bReadbackInFlight)
			{
				// Request a readback
				const uint32 NumBytes{ static_cast<uint32>(sizeof(LidarPoint32Aligned)) * ShaderParameters->NumSamples };
				ScanResultsReadback->EnqueueCopy(RHICmdList, GPUScanResultsBuffer->GetRHI(), NumBytes);

				// Mark that a readback is now in flight
				bReadbackInFlight = true;
			}
		}
	);

	// Add readback copy pass
	GraphBuilder->AddPass(
		RDG_EVENT_NAME("LidarRTReadbackCopy"), ERDGPassFlags::NeverCull,
		[this, ShaderParameters](FRHICommandListImmediate& RHICmdList)
		{
			// Only attempt to copy from the readback if it is ready
			if (bReadbackInFlight && ScanResultsReadback->IsReady())
			{
				// Get the raw data from the readback
				const uint32 NumBytes{ static_cast<uint32>(ScanResultsReadback->GetGPUSizeBytes()) };
				const LidarPoint32Aligned* Src{
					static_cast<const LidarPoint32Aligned*>(ScanResultsReadback->Lock(NumBytes))
				};

				// Initialize as many points as there are samples; this is an overestimation since samples can miss
				// (i.e. not hit anything), but this is preferable to multiple memory reallocations
				ScanResults.SetNum(ShaderParameters->NumSamples);

				// Fill scan results with readback data
				for (int32 i{ 0 }; i < ScanResults.Num(); ++i)
				{
					ScanResults[i].XYZ = Src[i].XYZ;
					ScanResults[i].Intensity = Src[i].Intensity;
					ScanResults[i].RGB =  FColor{ Src[i].RGB };
					ScanResults[i].bHit = (Src[i].bHit != 0);
				}

				// Mark that the readback has been completed and is ready for another iteration
				ScanResultsReadback->Unlock();
				bReadbackInFlight = false;
			}
		}
	);
}

} // namespace lidar
} // namespace uesensors
