#pragma once

#include "CoreMinimal.h"
#include "LidarStrategy.generated.h"

UENUM(BlueprintType)
enum class ELidarStrategy : uint8
{
	ParallelForLineTrace	UMETA(DisplayName = "(CPU) ParallelFor + Line Trace"),
	ComputeShader			UMETA(DisplayName = "(GPU) Compute Shader")
};

// Forward declarations
class FLidarPoint;
class ULidarSensor;

namespace uesensors {
namespace lidar {
	
class IStrategy
{
public:
	virtual ~IStrategy() = default;

	virtual void PerformScan() = 0;
};

} // namespace lidar
} // namespace uesensors
