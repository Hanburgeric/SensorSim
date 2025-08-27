#include "Sensors/LiDAR/LidarSensor.h"

// UESensors
#include "Sensors/LiDAR/LidarStrategy.h"
#include "Sensors/LiDAR/ParallelForStrategy.h"
#include "Sensors/LiDAR/ComputeShaderStrategy.h"
#include "Sensors/LiDAR/RayTracingStrategy.h"

DEFINE_LOG_CATEGORY(LogLiDARSensor);

void ULidarSensor::BeginPlay()
{
	Super::BeginPlay();

	// Initialize sample directions for the first time
	RebuildSampleDirections();

	// Initialize LiDAR scan strategy
	ReinitializeScanStrategy();
}

void ULidarSensor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Get the name of the property and the member property that changed
	const FName PropertyName{ PropertyChangedEvent.GetPropertyName() };

	// Changes to the LiDAR properties only matter after BeginPlay has been called
	// (i.e. after the sample directions have been initialized)
	if (HasBegunPlay())
	{
		// If the horizontal/vertical field of view or resolution is changed,
		// then the sample directions must be rebuilt
		if (PropertyName == GET_MEMBER_NAME_CHECKED(ULidarSensor, HorizontalFieldOfView)
			|| PropertyName != GET_MEMBER_NAME_CHECKED(ULidarSensor, HorizontalResolution)
			|| PropertyName != GET_MEMBER_NAME_CHECKED(ULidarSensor, VerticalFieldOfView)
			|| PropertyName != GET_MEMBER_NAME_CHECKED(ULidarSensor, VerticalResolution))
		{
			RebuildSampleDirections();
		}

		// By this point, the value of ScanMode has already changed;
		// since SetScanMode is not idempotent due to the early return, the callback is called directly
		if (PropertyName == GET_MEMBER_NAME_CHECKED(ULidarSensor, ScanMode))
		{
			ReinitializeScanStrategy();
		}
	}
}

int32 ULidarSensor::GetHorizontalSamples() const
{
	return FMath::RoundToInt(HorizontalFieldOfView / HorizontalResolution) + 1;
}

int32 ULidarSensor::GetVerticalSamples() const
{
	return FMath::RoundToInt(VerticalFieldOfView / VerticalResolution) + 1;
}

float ULidarSensor::GetHorizontalFieldOfView() const
{
	return HorizontalFieldOfView;
}

void ULidarSensor::SetHorizontalFieldOfView(float NewHorizontalFieldOfView)
{
	// Avoid doing unnecessary work if the value has not changed
	if (HorizontalFieldOfView == NewHorizontalFieldOfView) { return; }

	// Otherwise, update the value clamped to sensible values
	HorizontalFieldOfView = FMath::Clamp(NewHorizontalFieldOfView, 0.0F, 360.0F);

	// Rebuild the sample directions if the BeginPlay has already been called
	if (HasBegunPlay())
	{
		RebuildSampleDirections();
	}
}

float ULidarSensor::GetHorizontalResolution() const
{
	return HorizontalResolution;
}

void ULidarSensor::SetHorizontalResolution(float NewHorizontalResolution)
{
	// Avoid doing unnecessary work if the value has not changed
	if (HorizontalResolution == NewHorizontalResolution) { return; }

	// Otherwise, update the value clamped to sensible values
	HorizontalResolution = FMath::Max(0.01F, NewHorizontalResolution);

	// Rebuild the sample directions if the BeginPlay has already been called
	if (HasBegunPlay())
	{
		RebuildSampleDirections();
	}
}

float ULidarSensor::GetVerticalFieldOfView() const
{
	return VerticalFieldOfView;
}

void ULidarSensor::SetVerticalFieldOfView(float NewVerticalFieldOfView)
{
	// Avoid doing unnecessary work if the value has not changed
	if (VerticalFieldOfView == NewVerticalFieldOfView) { return; }

	// Otherwise, update the value clamped to sensible values
	VerticalFieldOfView = FMath::Clamp(NewVerticalFieldOfView, 0.01F, 360.0F);

	// Rebuild the sample directions if the BeginPlay has already been called
	if (HasBegunPlay())
	{
		RebuildSampleDirections();
	}
}

float ULidarSensor::GetVerticalResolution() const
{
	return VerticalResolution;
}

void ULidarSensor::SetVerticalResolution(float NewVerticalResolution)
{
	// Avoid doing unnecessary work if the value has not changed
	if (VerticalResolution == NewVerticalResolution) { return; }

	// Otherwise, update the value clamped to sensible values
	VerticalResolution = FMath::Max(0.0F, NewVerticalResolution);

	// Rebuild the sample directions if the BeginPlay has already been called
	if (HasBegunPlay())
	{
		RebuildSampleDirections();
	}
}

const TArray<FVector3f>& ULidarSensor::GetSampleDirections() const
{
	return SampleDirections;
}

ELidarScanMode ULidarSensor::GetScanMode() const
{
	return ScanMode;
}

void ULidarSensor::SetScanMode(ELidarScanMode NewScanMode)
{
	// Avoid doing unnecessary work if the mode has not changed
	if (ScanMode == NewScanMode) { return; }

	// Update the mode
	ScanMode = NewScanMode;

	// Reinitialize the scan strategy if BeginPlay has already been called
	if (HasBegunPlay())
	{
		ReinitializeScanStrategy();
	}
}

const TArray<FLidarPoint>& ULidarSensor::GetScanData() const
{
	return ScanData;
}

void ULidarSensor::ExecuteScan_Implementation()
{
	// Allow the strategy defined by ScanMode to execute the actual LiDAR scan
	if (ScanStrategy)
	{
		ScanData = ScanStrategy->ExecuteScan(*this);
	}

	// TEMPORARY: debug draw scan data as spheres
	if (const UWorld* World{ GetWorld() })
	{
		for (const FLidarPoint& Point : ScanData)
		{
			DrawDebugSphere(World, FVector{ Point.XYZ }, 1.0F, 4, Point.RGB);
		}
	}
}

void ULidarSensor::RebuildSampleDirections()
{
	// Calculate the number of samples per axis
	const int32 HorizontalSamples{ GetHorizontalSamples() };
	const int32 VerticalSamples{ GetVerticalSamples() };

	// Preemptively reserve memory for the sample directions
	// based on the total number of samples to prevent reallocations
	const int32 TotalSamples{ GetHorizontalSamples() * GetVerticalSamples() };
	SampleDirections.Reset(TotalSamples);

	// Iterate each sample
	for (int32 v{ 0 }; v < VerticalSamples; ++v)
	{
		// Calculate the pitch for the entire row
		const float Pitch{ -0.5F * VerticalFieldOfView + (v + 0.5F) * VerticalResolution };

		for (int32 h{ 0 }; h < HorizontalSamples; ++h)
		{
			// Calculate the yaw for the sample
			const float Yaw{ -0.5F * HorizontalFieldOfView + (h + 0.5F) * HorizontalResolution };

			// Construct the direction unit vector for the sample from the pitch and yaw
			const FVector SampleDirection{ FRotator{ Pitch, Yaw, 0.0F }.Vector() };

			// Add the sample direction to the array
			SampleDirections.Emplace(SampleDirection);
		}
	}
}

void ULidarSensor::ReinitializeScanStrategy()
{
	// Cleanup the previous scan strategy
	ScanStrategy.Reset();

	// Initialize the new scan strategy according to the current scan mode
	switch (ScanMode)
	{
	case ELidarScanMode::ParallelFor:
	{
		ScanStrategy = MakeUnique<uesensors::lidar::ParallelForStrategy>();
		UE_LOG(LogLiDARSensor, Log, TEXT("Initialized ParallelFor scan strategy for LiDAR sensor \"%s\"."), *GetName());

		break;
	}

	case ELidarScanMode::ComputeShader:
	{
		if (false)	// Currently unimplemented
		{
			ScanStrategy = MakeUnique<uesensors::lidar::ComputeShaderStrategy>();
			UE_LOG(LogLiDARSensor, Log, TEXT("Initialized ComputeShader scan strategy for LiDAR sensor \"%s\"."), *GetName());
		}

		else
		{
			UE_LOG(LogLiDARSensor, Warning, TEXT("Compute shaders not supported on hardware; falling back to ParallelFor scan strategy for LiDAR sensor \"%s\"."), *GetName());
			ScanStrategy = MakeUnique<uesensors::lidar::ParallelForStrategy>();
			ScanMode = ELidarScanMode::ParallelFor;
		}

		break;
	}

	case ELidarScanMode::RayTracing:
	{
		if (IsRayTracingEnabled())
		{
			ScanStrategy = MakeUnique<uesensors::lidar::RayTracingStrategy>();
			UE_LOG(LogLiDARSensor, Log, TEXT("Initialized RayTracing scan strategy for LiDAR sensor \"%s\"."), *GetName());
		}

		else
		{
			UE_LOG(LogLiDARSensor, Warning, TEXT("Ray tracing not supported on hardware; falling back to ParallelFor scan strategy for LiDAR sensor \"%s\"."), *GetName());
			ScanStrategy = MakeUnique<uesensors::lidar::ParallelForStrategy>();
			ScanMode = ELidarScanMode::ParallelFor;
		}

		break;
	}

	// In the event of an invalid strategy, simply default to ParallelForLineTrace
	default:
	{
		UE_LOG(LogLiDARSensor, Warning, TEXT("Invalid LiDAR scan strategy selected; falling back to ParallelFor scan strategy for LiDAR sensor \"%s\"."), *GetName());
		ScanStrategy = MakeUnique<uesensors::lidar::ParallelForStrategy>();
		break;
	}
	}

	// Ensure that the initialized scan strategy is valid
	check(ScanStrategy);
}
