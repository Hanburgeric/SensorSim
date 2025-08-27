#include "Sensors/LiDAR/RayTracingStrategy.h"

// UESensors
#include "Sensors/LiDAR/LidarSensor.h"

// UESensorShaders
#include "LiDAR/LidarRayGenShader.h"
#include "LiDAR/LidarMissShader.h"
#include "LiDAR/LidarClosestHitShader.h"

namespace uesensors {
namespace lidar {

TArray<FLidarPoint> RayTracingStrategy::ExecuteScan(const ULidarSensor& LidarSensor)
{
    TArray<FLidarPoint> ScanResult;

    // TODO: oh boy

    return ScanResult;
}

} // namespace lidar
} // namespace uesensors
