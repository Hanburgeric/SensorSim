#pragma once

#include "CoreMinimal.h"
#include "Sensors/LiDAR/LidarStrategy.h"

namespace uesensors {
namespace lidar {

class RayTracingStrategy final : public Strategy
{
public:
	RayTracingStrategy();
	~RayTracingStrategy();

	TArray<FLidarPoint> ExecuteScan(const UWorld* World, const FLidarScanParameters& InScanParameters) override;

private:
	void PostOpaqueRender(FPostOpaqueRenderParameters& Parameters);

private:
	IRendererModule& Renderer;
	FDelegateHandle PostOpaqueRenderDelegate{};

	FCriticalSection ScanCS{};
	FLidarScanParameters ScanParameters{};
	TArray<FLidarPoint> RTScanResults{};
	TUniquePtr<FRHIGPUBufferReadback> RTScanResultsReadback{};
	bool bReadbackInFlight{ false };
};

} // namespace lidar
} // namespace uesensors
