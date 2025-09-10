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
	// Create the GPU buffer readback
	RTResultsReadback = MakeUnique<FRHIGPUBufferReadback>(TEXT("RTResultsReadback"));
}

RayTracingStrategy::~RayTracingStrategy()
{
	// Flush all rendering commands before destruction such that
	// any render commands in flight do not reference a dangling this pointer
	FlushRenderingCommands();
}

TArray<FLidarPoint> RayTracingStrategy::ExecuteScan(const ULidarSensor& LidarSensor)
{
	TArray<FLidarPoint> ScanResults{};
	
	// Get the world location and rotation for the sensor
	const FVector SensorLocation{ LidarSensor.GetComponentLocation() };
	const FQuat SensorRotation{ LidarSensor.GetComponentQuat() };

	// Get the directions in which to perform the scan
	const TArray<FVector3f>& SampleDirections{ LidarSensor.GetSampleDirections() };

	// Get the number of samples
	const int32 NumSamples{ SampleDirections.Num() };

	// Get the range of the LiDAR
	const float MinRange{ LidarSensor.MinRange };
	const float MaxRange{ LidarSensor.MaxRange };
	
	ENQUEUE_RENDER_COMMAND(LidarRTScan)(
		[this, SensorLocation, SensorRotation, SampleDirectionsCopy = SampleDirections, NumSamples, MinRange, MaxRange]
		(FRHICommandListImmediate& RHICmdList)
		{
			FRDGBuilder GraphBuilder{ RHICmdList };

			// Allocate shader parameters
			FLidarRayGenShader::FParameters* ShaderParameters{
				GraphBuilder.AllocParameters<FLidarRayGenShader::FParameters>()
			};

			// Create buffer for sample directions
			const FRDGBufferRef SampleDirectionsBuffer{
				GraphBuilder.CreateBuffer(
					FRDGBufferDesc::CreateStructuredDesc(sizeof(FVector3f), NumSamples),
					TEXT("SampleDirections")
				)
			};

			// Upload sample directions to the GPU
			GraphBuilder.QueueBufferUpload(
				SampleDirectionsBuffer, SampleDirectionsCopy.GetData(), sizeof(FVector3f) * NumSamples
			);

			// Configure shader input parameters (except for the TLAS,
			// which must be configured in the ray trace dispatch pass)
			ShaderParameters->SensorLocation = FVector3f{ SensorLocation };
			ShaderParameters->SensorRotation = FVector4f{ FVector4{
				SensorRotation.X, SensorRotation.Y, SensorRotation.Z, SensorRotation.W
			} };
			ShaderParameters->SampleDirections = GraphBuilder.CreateSRV(SampleDirectionsBuffer);
			ShaderParameters->NumSamples = NumSamples;
			ShaderParameters->MinRange = MinRange;
			ShaderParameters->MaxRange = MaxRange;

			// Create buffer for GPU scan results
			const FRDGBufferRef GPUScanResultsBuffer{
				GraphBuilder.CreateBuffer(
					FRDGBufferDesc::CreateStructuredDesc(sizeof(LidarPoint32Aligned), NumSamples),
					TEXT("GPUScanResults")
				)
			};

			// Configure shader output parameters
			ShaderParameters->GPUScanResults = GraphBuilder.CreateUAV(GPUScanResultsBuffer);

			// Add ray trace dispatch pass
			GraphBuilder.AddPass(
				RDG_EVENT_NAME("LidarRTDispatch"), ShaderParameters, ERDGPassFlags::Compute | ERDGPassFlags::NeverCull,
				[ShaderParameters](FRHIRayTracingCommandList& RHICmdList)
				{
					// TODO: Create the TLAS
					return;
					ShaderParameters->TLAS;
					
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
				GraphBuilder, RDG_EVENT_NAME("LidarRTReadback"), GPUScanResultsBuffer,
				[this, GPUScanResultsBuffer, NumSamples](FRHICommandListImmediate& RHICmdList)
				{
					// Only request a readback if one is not already in flight
					if (!bReadbackInFlight)
					{
						const uint32 NumBytes{ static_cast<uint32>(sizeof(LidarPoint32Aligned)) * NumSamples };
						RTResultsReadback->EnqueueCopy(RHICmdList, GPUScanResultsBuffer->GetRHI(), NumBytes);

						// Mark the readback as being in flight
						bReadbackInFlight = true;
					}
				}
			);

			// Add readback copy pass
			GraphBuilder.AddPass(
				RDG_EVENT_NAME("LidarRTReadbackCopy"), ERDGPassFlags::NeverCull,
				[this, NumSamples](FRHICommandListImmediate& RHICmdList)
				{
					// Only attempt to copy from the readback if it is ready
					if (bReadbackInFlight && RTResultsReadback->IsReady())
					{
						// Get the raw data from the readback
						const uint32 NumBytes{ static_cast<uint32>(RTResultsReadback->GetGPUSizeBytes()) };
						const LidarPoint32Aligned* Src{
							static_cast<const LidarPoint32Aligned*>(RTResultsReadback->Lock(NumBytes))
						};

						// Initialize as many points as there are samples; this is an overestimation since
						// samples can miss (i.e. not hit anything),
						// but this is preferable to multiple memory reallocations
						RTResults.SetNum(NumSamples);

						// Lock scan results to prevent data race
						{
							FScopeLock Lock(&RTResultsCS);
							
							// Fill scan results with readback data
							for (int32 i{ 0 }; i < RTResults.Num(); ++i)
							{
								RTResults[i].XYZ = Src[i].XYZ;
								RTResults[i].Intensity = Src[i].Intensity;
								RTResults[i].RGB =  FColor{ Src[i].RGB };
								RTResults[i].bHit = (Src[i].bHit != 0);
							}

							// Remove all points that did failed to hit anything in a single pass
							// to create a clean set of results
							RTResults.RemoveAll(
								[](const FLidarPoint& Point)
								{
									return !Point.bHit;
								}
							);
						}

						// Mark the readback as complete for another iteration
						RTResultsReadback->Unlock();
						bReadbackInFlight = false;
					}
				}
			);

			GraphBuilder.Execute();
		}
	);
	
	// Lock and copy scan results to prevent data race
	{
		FScopeLock Lock(&RTResultsCS);
		ScanResults = RTResults;
	}
	
	return ScanResults;
}

} // namespace lidar
} // namespace uesensors
