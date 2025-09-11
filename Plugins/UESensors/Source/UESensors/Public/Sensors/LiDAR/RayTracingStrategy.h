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

private:
	FDelegateHandle PostOpaqueRenderDelegate{};
	
	FVector SensorLocation{ 0.0, 0.0, 0.0 };
	FQuat SensorRotation{ 0.0, 0.0, 0.0, 0.0 };
	TArray<FVector3f> SampleDirections{};
	int32 NumSamples{ 0 };
	float MinRange{ 0.0F };
	float MaxRange{ 0.0F };

	TArray<FLidarPoint> ScanResults{};
	TUniquePtr<FRHIGPUBufferReadback> ScanResultsReadback{};
	bool bReadbackInFlight{ false };
};

} // namespace lidar
} // namespace uesensors
