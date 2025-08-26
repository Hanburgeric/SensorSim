#include "Sensors/LiDAR/LidarSensor.h"

// UESensors
#include "Sensors/LiDAR/LidarStrategy.h"
#include "Sensors/LiDAR/ParallelForLineTraceStrategy.h"
#include "Sensors/LiDAR/ComputeShaderStrategy.h"

DEFINE_LOG_CATEGORY(LogLiDARSensor);

void ULidarSensor::BeginPlay()
{
	Super::BeginPlay();

	// Initialize sample directions for the first time
	RebuildSampleDirections();

	// Initialize LiDAR strategy
	ReinitializeStrategy();
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

		// By this point, the value of LidarStrategy has already changed;
		// since SetLidarStrategy is not idempotent due to the early return, the callback is called directly
		if (PropertyName == GET_MEMBER_NAME_CHECKED(ULidarSensor, LidarStrategy))
		{
			ReinitializeStrategy();
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

const TArray<FVector>& ULidarSensor::GetSampleDirections() const
{
	return SampleDirections;
}

ELidarStrategy ULidarSensor::GetLidarStrategy() const
{
	return LidarStrategy;
}

void ULidarSensor::SetLidarStrategy(ELidarStrategy NewLidarStrategy)
{
	// Avoid doing unnecessary work if the strategy has not changed
	if (LidarStrategy == NewLidarStrategy) { return; }

	// Update the strategy
	LidarStrategy = NewLidarStrategy;

	// Reinitialize the strategy if the BeginPlay has already been called
	if (HasBegunPlay())
	{
		ReinitializeStrategy();
	}
}

void ULidarSensor::PerformScan_Implementation()
{
	if (Strategy)
	{
		Strategy->PerformScan();
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

void ULidarSensor::ReinitializeStrategy()
{
	// Cleanup the previous strategy
	Strategy.Reset();

	// Initialize the new strategy
	switch (LidarStrategy)
	{
	case ELidarStrategy::ParallelForLineTrace:
	{
		Strategy = MakeUnique<uesensors::lidar::ParallelForLineTraceStrategy>();
		break;
	}

	case ELidarStrategy::ComputeShader:
	{
		Strategy = MakeUnique<uesensors::lidar::ComputeShaderStrategy>();
		break;
	}

	// In the event of an invalid strategy, simply default to ParallelForLineTrace
	default:
	{
		UE_LOG(LogLiDARSensor, Warning, TEXT("Invalid strategy selected; defaulting to ParallelFor + Line Trace"));
		Strategy = MakeUnique<uesensors::lidar::ParallelForLineTraceStrategy>();
		break;
	}
	}

	// Ensure that the strategy is valid
	check(Strategy);
}
