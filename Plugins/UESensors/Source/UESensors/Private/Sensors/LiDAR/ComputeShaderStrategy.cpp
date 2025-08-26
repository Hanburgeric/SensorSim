#include "Sensors/LiDAR/ComputeShaderStrategy.h"

// UESensors
#include "Sensors/LiDAR/LidarSensor.h"

namespace uesensors {
namespace lidar {

void ComputeShaderStrategy::PerformScan()
{
	UE_LOG(LogTemp, Warning, TEXT("Performing scan with Compute Shader..."));
}

} // namespace lidar
} // namespace uesensors
