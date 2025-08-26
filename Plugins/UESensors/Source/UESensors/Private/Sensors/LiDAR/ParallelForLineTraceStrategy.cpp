#include "Sensors/LiDAR/ParallelForLineTraceStrategy.h"

// UESensors
#include "Sensors/LiDAR/LidarSensor.h"

namespace uesensors {
namespace lidar {

void ParallelForLineTraceStrategy::PerformScan()
{
	UE_LOG(LogTemp, Warning, TEXT("Performing scan with ParallelFor + Line Trace..."));
}

} // namespace lidar
} // namespace uesensors
