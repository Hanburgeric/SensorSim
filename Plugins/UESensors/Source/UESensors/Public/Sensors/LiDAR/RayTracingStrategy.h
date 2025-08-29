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
	void PostOpaqueRender(FPostOpaqueRenderParameters& Parameters);

	FDelegateHandle PostOpaqueRenderDelegate{};

	FRDGBufferSRVRef TLAS{ nullptr };
};

} // namespace lidar
} // namespace uesensors
