#include "Sensors/LiDAR/ParallelForStrategy.h"

// UESensors
#include "Sensors/LiDAR/LidarSensor.h"

namespace uesensors {
namespace lidar {

TArray<FLidarPoint> ParallelForStrategy::ExecuteScan(const ULidarSensor& LidarSensor)
{
	TArray<FLidarPoint> ScanResults{};

	// Early return condition(s)
	const UWorld* World{ LidarSensor.GetWorld() };
	if (!World)
	{
		UE_LOG(LogLiDARSensor, Warning, TEXT("Invalid world; cannot execute LiDAR scan."));
		return ScanResults;
	}

	// Get the world location and rotation for the sensor
	const FVector SensorLocation{ LidarSensor.GetComponentLocation() };
	const FQuat SensorRotation{ LidarSensor.GetComponentQuat() };

	// Get the directions in which to perform the scan
	const TArray<FVector3f>& SampleDirections{ LidarSensor.GetSampleDirections() };

	// Initialize as many points as there are samples; this is an overestimation since samples can miss
	// (i.e. not hit anything), but this is preferable to multiple memory reallocations
	const int32 NumSamples{ SampleDirections.Num() };
	ScanResults.SetNum(NumSamples);

	// Get the range of the LiDAR
	const float MinRange{ LidarSensor.MinRange };
	const float MaxRange{ LidarSensor.MaxRange };
		
	// Configure line trace parameters
	const ECollisionChannel LineTraceChannel{ ECollisionChannel::ECC_Visibility };

	FCollisionQueryParams LineTraceQueryParams{};
	LineTraceQueryParams.bTraceComplex = true;
	LineTraceQueryParams.bReturnPhysicalMaterial = true;

	FCollisionResponseParams LineTraceResponseParams{};

	// Process all samples in parallel for improved throughput
	ParallelFor(
		NumSamples, [&](int32 Index)
		{
			// Get the point whose data to populate
			FLidarPoint& Point{ ScanResults[Index] };

			// Calculate the direction in which to cast the line trace
			const FVector SampleDirection{ SampleDirections[Index] };
			const FVector LineTraceDirection{ SensorRotation.RotateVector(SampleDirection) };

			// Calculate the start and end points for the line trace
			const FVector LineTraceStart{ SensorLocation + MinRange * LineTraceDirection };
			const FVector LineTraceEnd{ SensorLocation + MaxRange * LineTraceDirection };

			// Perform the line trace and store the result
			FHitResult HitResult{};
			Point.bHit = World->LineTraceSingleByChannel(
				HitResult,
				LineTraceStart, LineTraceEnd,
				LineTraceChannel, LineTraceQueryParams, LineTraceResponseParams
			);

			// If the line trace hit, populate the point with relevant data
			if (Point.bHit)
			{
				// Store the world location of the hit
				Point.XYZ = FVector3f{ HitResult.ImpactPoint };

				// TODO: calculate proper intensity based on distance, angle, and material properties
				Point.Intensity = 0.0F;

				// TODO: extract semantic segmentation color from custom depth stencil value
				Point.RGB = FColor::Red;
			}
		}
	);

	// Remove all points that did failed to hit anything in a single pass
	// to create a clean set of results
	ScanResults.RemoveAll(
		[](const FLidarPoint& Point)
		{
			return !Point.bHit;
		}
	);

	return ScanResults;
}

} // namespace lidar
} // namespace uesensors
