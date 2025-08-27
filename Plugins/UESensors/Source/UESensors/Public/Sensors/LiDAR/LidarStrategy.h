#pragma once

#include "CoreMinimal.h"
#include "LidarStrategy.generated.h"

UENUM(BlueprintType)
enum class ELidarScanMode : uint8
{
	ParallelFor			UMETA(DisplayName = "(CPU) ParallelFor"),
	ComputeShader		UMETA(DisplayName = "(GPU) Compute Shader"),
	RayTracing			UMETA(DisplayName = "(GPU) Ray Tracing")
};

USTRUCT(BlueprintType)
struct FLidarPoint
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector3f XYZ{ FVector3f::ZeroVector };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Intensity{ 0.0F };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FColor RGB{ FColor::Black };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bHit{ false };
};

// Forward declarations
class ULidarSensor;

namespace uesensors {
namespace lidar {
	
class Strategy
{
public:
	virtual ~Strategy() = default;

	virtual TArray<FLidarPoint> ExecuteScan(const ULidarSensor& LidarSensor) = 0;
};

} // namespace lidar
} // namespace uesensors
