#pragma once

#include "CoreMinimal.h"
#include "Sensors/LiDAR/LidarStrategy.h"

namespace uesensors {
namespace lidar {

class ParallelForStrategy final : public Strategy
{
public:
	ParallelForStrategy();

	TArray<FLidarPoint> ExecuteScan(const UWorld* World, const FLidarScanParameters& InScanParameters) override;

public:
	ECollisionChannel LineTraceChannel{};
	FCollisionQueryParams LineTraceQueryParams{};
	FCollisionResponseParams LineTraceResponseParams{};
};

} // namespace lidar
} // namespace uesensors
