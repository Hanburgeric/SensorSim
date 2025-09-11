#include "Sensors/LiDAR/ParallelForStrategy.h"

DECLARE_LOG_CATEGORY_EXTERN(LogLiDARSensor, Log, All);

namespace uesensors {
namespace lidar {

ParallelForStrategy::ParallelForStrategy()
{
	// Initialize default line trace parameters
	LineTraceChannel = ECC_Visibility;

	LineTraceQueryParams.bTraceComplex = true;
	LineTraceQueryParams.bReturnPhysicalMaterial = true;
}

TArray<FLidarPoint> ParallelForStrategy::ExecuteScan(const UWorld* World, const FLidarScanParameters& InScanParameters)
{
	TArray<FLidarPoint> ScanResults{};

	// Initialize as many points as there are samples; this is an overestimation since samples can miss
	// (i.e. not hit anything), but this is preferable to multiple memory reallocations
	const int32 NumSamples{ InScanParameters.SampleDirections.Num() };
	ScanResults.SetNum(NumSamples);

	// Process all samples in parallel for improved throughput
	ParallelFor(NumSamples,
		[this, &ScanResults, World,
		 SensorLocation = InScanParameters.SensorLocation,
		 SensorRotation = InScanParameters.SensorRotation,
		 SampleDirections = MakeArrayView(InScanParameters.SampleDirections),
		 MinRange = InScanParameters.MinRange,
		 MaxRange = InScanParameters.MaxRange]
		(int32 Index)
		{
			// Get the point whose data to populate
			FLidarPoint& Point{ ScanResults[Index] };

			// Calculate the direction in which to cast the line trace
			const FVector3f& SampleDirection{ SampleDirections[Index] };
			const FVector3f LineTraceDirection{
				FQuat4f{ SensorRotation.X, SensorRotation.Y, SensorRotation.Z, SensorRotation.W }.RotateVector(SampleDirection)
			};

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
				Point.XYZ = FVector3f{ HitResult.ImpactPoint };

				// TODO: calculate intensity based on distance, angle, and material properties
				Point.Intensity = 0.0F;

				// TODO: extract semantic segmentation color from custom depth stencil value
				Point.RGB = FColor::Black;
			}
		}
	);

	// Remove all points that did failed to hit anything in a single pass
	// to create a clean set of valid results
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
