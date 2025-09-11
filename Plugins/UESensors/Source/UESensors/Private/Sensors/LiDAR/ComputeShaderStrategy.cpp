#include "Sensors/LiDAR/ComputeShaderStrategy.h"

DECLARE_LOG_CATEGORY_EXTERN(LogLiDARSensor, Log, All);

namespace uesensors {
namespace lidar {

TArray<FLidarPoint> ComputeShaderStrategy::ExecuteScan(const UWorld* World, const FLidarScanParameters& InScanParameters)
{
	TArray<FLidarPoint> ScanResults{};

	// TODO: oh boy

	return ScanResults;
}

} // namespace lidar
} // namespace uesensors
