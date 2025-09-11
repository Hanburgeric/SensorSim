#include "Sensors/LiDAR/RayTracingStrategy.h"

// UE
#include "Runtime/Renderer/Private/ScenePrivate.h"
#include "RenderGraphBuilder.h"

// UESensorShaders
#include "LiDAR/LidarShaders.h"

DECLARE_LOG_CATEGORY_EXTERN(LogLiDARSensor, Log, All);

namespace uesensors {
namespace lidar {

RayTracingStrategy::RayTracingStrategy()
	: Renderer{ FModuleManager::LoadModuleChecked<IRendererModule>("Renderer") }
{
	// Register the delegate
	if (!PostOpaqueRenderDelegate.IsValid())
	{
		PostOpaqueRenderDelegate = Renderer.RegisterPostOpaqueRenderDelegate(
			FPostOpaqueRenderDelegate::CreateRaw(this, &RayTracingStrategy::PostOpaqueRender)
		);
	}

	// Create the GPU buffer readback
	RTScanResultsReadback = MakeUnique<FRHIGPUBufferReadback>(TEXT("RTScanResultsReadback"));
}

RayTracingStrategy::~RayTracingStrategy()
{
	// Remove the delegate
	if (PostOpaqueRenderDelegate.IsValid())
	{
		Renderer.RemovePostOpaqueRenderDelegate(PostOpaqueRenderDelegate);
		PostOpaqueRenderDelegate.Reset();
	}

	// Flush all rendering command before destruction such that
	// any render commands in flight do not reference a dangling this pointer
	FlushRenderingCommands();
}

TArray<FLidarPoint> RayTracingStrategy::ExecuteScan(const UWorld* World, const FLidarScanParameters& InScanParameters)
{
	TArray<FLidarPoint> ScanResults{};

	// Copy the scan parameters for later use in PostOpaqueRender
	ScanParameters = InScanParameters;

	// Lock the scan results such that PostOpaqueRender does not write to it
	// while it is being read from
	{
		FScopeLock Lock(&ScanCS);

		// Copy the scan results from PostOpaqueRender
		ScanResults = RTScanResults;
	}

	return ScanResults;
}

void RayTracingStrategy::PostOpaqueRender(FPostOpaqueRenderParameters& Parameters)
{
	// Create variables for the render thread to use safely
	FVector3f SensorLocation{ 0.0F, 0.0F, 0.0F };
	FVector4f SensorRotation{ 0.0F, 0.0F, 0.0F, 0.0F };
	TArrayView<FVector3f> SampleDirections{};
	int32 NumSamples{ 0 };
	float MinRange{ 0.0F };
	float MaxRange{ 0.0F };

	// Lock the cached scan parameters such that
	// they are not altered by ExecuteScan while being read from
	{
		FScopeLock Lock(&ScanCS);

		// Set the render thread variables
		SensorLocation = FVector3f{ ScanParameters.SensorLocation };
		SensorRotation = FVector4f{ ScanParameters.SensorRotation };
		SampleDirections = MakeArrayView(ScanParameters.SampleDirections);
		MinRange = ScanParameters.MinRange;
		MaxRange = ScanParameters.MaxRange;
	}

	// Since PostOpaqueRender is likely to be called far more frequently than is ExecuteScan,
	// it is entirely possible for the scan parameters to be in an uninitialized but valid state after
	// the strategy has just been constructed, in which case the scan should not be performed
	NumSamples = SampleDirections.Num();
	if (NumSamples <= 0) { return; }

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
	ShaderParameters->SensorLocation = SensorLocation;
	ShaderParameters->SensorRotation = SensorRotation;
	ShaderParameters->SampleDirections = GraphBuilder->CreateSRV(SampleDirectionsBuffer);
	ShaderParameters->NumSamples = NumSamples;
	ShaderParameters->MinRange = MinRange;
	ShaderParameters->MaxRange = MaxRange;

	// Create buffer for ray tracing scan results
	FRDGBufferRef RTScanResultsBuffer{
		GraphBuilder->CreateBuffer(
			FRDGBufferDesc::CreateStructuredDesc(sizeof(LidarPoint32Aligned), NumSamples),
			TEXT("RTScanResults")
		)
	};

	// Configure shader output parameters
	ShaderParameters->RTScanResults = GraphBuilder->CreateUAV(RTScanResultsBuffer);

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
		*GraphBuilder, RDG_EVENT_NAME("LidarRTReadback"), RTScanResultsBuffer,
		[this, RTScanResultsBuffer, ShaderParameters](FRHICommandListImmediate& RHICmdList)
		{
			// Only request a readback if one is not already in flight
			if (!bReadbackInFlight)
			{
				// Request a readback
				const uint32 NumBytes{ static_cast<uint32>(sizeof(LidarPoint32Aligned)) * ShaderParameters->NumSamples };
				RTScanResultsReadback->EnqueueCopy(RHICmdList, RTScanResultsBuffer->GetRHI(), NumBytes);

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
			if (bReadbackInFlight && RTScanResultsReadback->IsReady())
			{
				// Get the raw data from the readback
				const uint32 NumBytes{ static_cast<uint32>(RTScanResultsReadback->GetGPUSizeBytes()) };
				const LidarPoint32Aligned* Src{
					static_cast<const LidarPoint32Aligned*>(RTScanResultsReadback->Lock(NumBytes))
				};

				// Lock the scan results such that ExecuteScan does not attempt to read from it
				// while it is being written to
				{
					FScopeLock Lock(&ScanCS);

					// Initialize as many points as there are samples; this is an overestimation since samples can miss
					// (i.e. not hit anything), but this is preferable to multiple memory reallocations
					RTScanResults.SetNum(ShaderParameters->NumSamples);

					// Fill scan results with readback data
					for (int32 i{ 0 }; i < RTScanResults.Num(); ++i)
					{
						RTScanResults[i].XYZ = Src[i].XYZ;
						RTScanResults[i].Intensity = Src[i].Intensity;
						RTScanResults[i].RGB = FColor{ Src[i].RGB };
						RTScanResults[i].bHit = (Src[i].bHit != 0);
					}

					// Remove all points that did failed to hit anything in a single pass
					// to create a clean set of valid results
					RTScanResults.RemoveAll(
						[](const FLidarPoint& Point)
						{
							return !Point.bHit;
						}
					);
				}

				// Mark that the readback has been completed and is ready for another iteration
				RTScanResultsReadback->Unlock();
				bReadbackInFlight = false;
			}
		}
	);
}

} // namespace lidar
} // namespace uesensors
