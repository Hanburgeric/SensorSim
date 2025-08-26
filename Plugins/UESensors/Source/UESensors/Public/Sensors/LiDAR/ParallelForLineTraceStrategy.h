#pragma once

#include "CoreMinimal.h"
#include "Sensors/LiDAR/LidarStrategy.h"

// Forward declarations
class ULidarSensor;

namespace uesensors {
namespace lidar {
	
class ParallelForLineTraceStrategy final : public IStrategy
{
public:
	void PerformScan() override;
};

} // namespace lidar
} // namespace uesensors
