#pragma once

#include "CoreMinimal.h"
#include "Sensors/LiDAR/LidarStrategy.h"

// Forward declarations
class ULidarSensor;

namespace uesensors {
namespace lidar {

class RayTracingStrategy final : public Strategy
{
public:
	RayTracingStrategy();
	~RayTracingStrategy();
	
	TArray<FLidarPoint> ExecuteScan(const ULidarSensor& LidarSensor) override;

private:
	TArray<FLidarPoint> RTResults{};
	FCriticalSection RTResultsCS{};
	
	TUniquePtr<FRHIGPUBufferReadback> RTResultsReadback{ nullptr };
	bool bReadbackInFlight{ false };
};

} // namespace lidar
} // namespace uesensors
