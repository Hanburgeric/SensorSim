#include "Sensors/LiDAR/RayTracingStrategy.h"

// UESensors
#include "Sensors/LiDAR/LidarSensor.h"

// UESensorShaders
#include "UESensorShaders/Public/TestShader.h"

namespace uesensors {
namespace lidar {

TArray<FLidarPoint> RayTracingStrategy::ExecuteScan(const ULidarSensor& LidarSensor)
{
    TArray<FLidarPoint> ScanResult;

    // Test the shader
    float TestInput = 5.0f;
    float TestMultiplier = 3.0f;
    float TestResult = FTestShaderRunner::Execute(TestInput, TestMultiplier);

    UE_LOG(LogTemp, Warning, TEXT("RayTracingStrategy: Shader test result = %.2f"), TestResult);

    return ScanResult;
}

} // namespace lidar
} // namespace uesensors
