#pragma once

#include "CoreMinimal.h"
#include "Sensors/LiDAR/LidarStrategy.h"

// Forward declarations
class ULidarSensor;

namespace uesensors {
namespace lidar {
	
class ComputeShaderStrategy final : public Strategy
{
public:
	TArray<FLidarPoint> PerformScan(const ULidarSensor& LidarSensor) override;
};

} // namespace lidar
} // namespace uesensors
