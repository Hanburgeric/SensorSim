#include "Sensors/LiDAR/BaseLiDARSensor.h"

UBaseLiDARSensor::UBaseLiDARSensor()
{
}

void UBaseLiDARSensor::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	// Get the name of the changed property and, if it exists,
	// the member property to which it belongs
	const FName PropertyName{ PropertyChangedEvent.GetPropertyName() };
	const FName MemberPropertyName{ PropertyChangedEvent.GetMemberPropertyName() };

	// If an axis property was changed, then the number of samples or
	// the angular resolution may need to be recalculated
	if (MemberPropertyName == GET_MEMBER_NAME_CHECKED(UBaseLiDARSensor, HorizontalAxisProperties)
		|| MemberPropertyName == GET_MEMBER_NAME_CHECKED(UBaseLiDARSensor, VerticalAxisProperties))
	{
		// Determine which axis had its field of view changed
		FLiDARAxisProperties& ChangedAxis{
			MemberPropertyName == GET_MEMBER_NAME_CHECKED(UBaseLiDARSensor, HorizontalAxisProperties)
			? HorizontalAxisProperties
			: VerticalAxisProperties
		};

		// Booleans to determine whether the number of samples or
		// the angular resolution needs updating
		bool bIsNumberOfSamplesDirty{ false };
		bool bIsAngularResolutionDirty{ false };

		// If the field of view has changed, then change whichever variable is appropriate
		// according to bUseAngularResolution
		if (PropertyName == GET_MEMBER_NAME_CHECKED(FLiDARAxisProperties, FieldOfView))
		{
			if (ChangedAxis.bUseAngularResolution)
			{
				bIsNumberOfSamplesDirty = true;
			}
			else
			{
				bIsAngularResolutionDirty = true;
			}
		}

		// If the number of samples has changed, then the angular resolution needs updating
		else if (PropertyName == GET_MEMBER_NAME_CHECKED(FLiDARAxisProperties, NumberOfSamples))
		{
			bIsAngularResolutionDirty = true;
		}

		// If the angular resolution has changed, then the number of samples needs updating
		else if (PropertyName == GET_MEMBER_NAME_CHECKED(FLiDARAxisProperties, AngularResolution))
		{
			bIsNumberOfSamplesDirty = true;
		}

		// A sanity check to ensure that only one of either the number of samples or
		// the angular resolution needs updating at the same time
		ensure(!(bIsNumberOfSamplesDirty && bIsAngularResolutionDirty));
		
		// Recalculate the number of samples
		if (bIsNumberOfSamplesDirty)
		{
			// Disallow division by zero
			if (ChangedAxis.AngularResolution > 0.0F)
			{
				ChangedAxis.NumberOfSamples =
					static_cast<int32>(ChangedAxis.FieldOfView / ChangedAxis.AngularResolution);
			}
			else
			{
				ChangedAxis.NumberOfSamples = 0;
			}
		}

		// Recalculate the angular resolution
		if (bIsAngularResolutionDirty)
		{
			// Disallow division by zero
			if (ChangedAxis.NumberOfSamples > 0)
			{
				ChangedAxis.AngularResolution =
					ChangedAxis.FieldOfView / static_cast<float>(ChangedAxis.NumberOfSamples);
			}
			else
			{
				ChangedAxis.AngularResolution = 0.0F;
			}
		}
	}
	
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void UBaseLiDARSensor::BeginPlay()
{
	Super::BeginPlay();

	// Shortened variable names for convenience
	const FLiDARAxisProperties& H{ HorizontalAxisProperties };
	const FLiDARAxisProperties& V{ VerticalAxisProperties };
	
	// Reserve memory upfront for the axis defined laser directions to prevent reallocations
	AxisDefinedLaserDirections.Reserve(H.NumberOfSamples * V.NumberOfSamples);

	// Build array using the horizontal and vertical axis properties
	for (int32 v{ 0 }; v < V.NumberOfSamples; ++v)
	{
		// Calculate the pitch for the entire row
		const float Pitch{
			V.FieldOfViewOffset
			- 0.5F * V.FieldOfView
			+ (v + 0.5F) * V.AngularResolution
		};
		
		for (int32 h{ 0 }; h < H.NumberOfSamples; ++h)
		{
			// Calculate the yaw for the laser
			const float Yaw{
				H.FieldOfViewOffset
				- 0.5F * H.FieldOfView
				+ (h + 0.5F) * H.AngularResolution
			};

			// Construct rotator from pitch and yaw
			const FRotator Rotation{ Pitch, Yaw, 0.0F };

			// Convert the rotator to a unit vector and add it to the array
			AxisDefinedLaserDirections.Emplace(Rotation.Vector());
		}
	}

	// Normalize additional laser directions
	for (FVector& LaserDirection : AdditionalLaserDirections)
	{
		LaserDirection = LaserDirection.GetSafeNormal();
	}
	
	// Combine the two arrays to yield all directions in which lasers will be emitted
	LaserDirections.Append(AxisDefinedLaserDirections);
	LaserDirections.Append(AdditionalLaserDirections);
}
