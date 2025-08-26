#include "Sensors/LiDAR/ComputeShaderStrategy.h"

// UESensors
#include "Sensors/LiDAR/LidarSensor.h"

namespace uesensors {
namespace lidar {

TArray<FLidarPoint> ComputeShaderStrategy::PerformScan(const ULidarSensor& LidarSensor)
{
	TArray<FLidarPoint> ScanResult{};

	// Get the world location and rotation for the sensor
	const FVector SensorLocation{ LidarSensor.GetComponentLocation() };
	const FRotator SensorRotation{ LidarSensor.GetComponentRotation() };

	// TEMPORARY
	FLidarPoint TempLidarPoint{};
	TempLidarPoint.XYZ = SensorLocation + LidarSensor.GetForwardVector() * 1000.0;
	TempLidarPoint.Intensity = 0.0F;
	TempLidarPoint.RGB = FColor::Blue;

	ScanResult.Emplace(TempLidarPoint);

	return ScanResult;
}

} // namespace lidar
} // namespace uesensors
