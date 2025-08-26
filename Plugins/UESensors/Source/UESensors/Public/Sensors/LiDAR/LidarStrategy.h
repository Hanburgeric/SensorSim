#pragma once

#include "CoreMinimal.h"
#include "LidarStrategy.generated.h"

UENUM(BlueprintType)
enum class ELidarScanMode : uint8
{
	ParallelForLineTrace	UMETA(DisplayName = "(CPU) ParallelFor + Line Trace"),
	ComputeShader			UMETA(DisplayName = "(GPU) Compute Shader")
};

USTRUCT(BlueprintType)
struct FLidarPoint
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector XYZ{ FVector::ZeroVector };

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

	virtual TArray<FLidarPoint> PerformScan(const ULidarSensor& LidarSensor) = 0;
};

} // namespace lidar
} // namespace uesensors
