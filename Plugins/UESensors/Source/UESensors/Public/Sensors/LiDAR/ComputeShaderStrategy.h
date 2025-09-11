#pragma once

#include "CoreMinimal.h"
#include "Sensors/LiDAR/LidarStrategy.h"

namespace uesensors {
namespace lidar {
	
class ComputeShaderStrategy final : public Strategy
{
public:
	TArray<FLidarPoint> ExecuteScan(const UWorld* World, const FLidarScanParameters& InScanParameters) override;
};

} // namespace lidar
} // namespace uesensors
