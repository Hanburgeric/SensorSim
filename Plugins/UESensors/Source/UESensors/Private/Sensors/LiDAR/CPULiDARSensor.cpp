#include "Sensors/LiDAR/CPULiDARSensor.h"

UCPULiDARSensor::UCPULiDARSensor()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UCPULiDARSensor::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// TEMPORARY: debug draw all laser directions
	if (const UWorld* World{ GetWorld() })
	{
		for (const FVector& LaserDirection : LaserDirections)
		{
			const FVector LaserStart{ GetComponentLocation() };
			const FVector LaserEnd{
				LaserStart + GetComponentRotation().RotateVector(LaserDirection) * DebugDrawDistance
			};
		
			DrawDebugLine(
				World, LaserStart, LaserEnd,
				FColor::Red, false, -1.0F, 0U, 0.0F
			);
		}
	}
}
