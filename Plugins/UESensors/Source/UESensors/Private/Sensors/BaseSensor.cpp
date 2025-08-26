#include "Sensors/BaseSensor.h"

DEFINE_LOG_CATEGORY(LogBaseSensor);

UBaseSensor::UBaseSensor()
{
	// Enable ticking to allow the sensor to perform scans periodically
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UBaseSensor::BeginPlay()
{
	Super::BeginPlay();

	if (bSensorEnabled)
	{
		// Schedule the first scan if the sensor is enabled on play
		if (const UWorld* World{ GetWorld() })
		{
			NextScanTime = World->GetTimeSeconds() + ScanPeriod;
		}

		// Allow derived classes to perform additional initialization
		OnEnableSensor();
	}
}

void UBaseSensor::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Only perform scans if the sensor is enabled
	if (bSensorEnabled)
	{
		if (const UWorld* World{ GetWorld() })
		{
			// Check if enough time has passed for the next scan to occur
			const double CurrentTime{ World->GetTimeSeconds() };
			if (CurrentTime >= NextScanTime)
			{
				// Execute the scan; derived classes should implement this function
				PerformScan();

				// Schedule the next scan
				if (ScanPeriod > 0.0)
				{
					// Catch up if the scan has fallen behind schedule (e.g. due to frame drops or long scan operations)
					// to maintain the correct average scan frequency, even if individual scans are delayed as a result
					while (NextScanTime <= CurrentTime)
					{
						NextScanTime += ScanPeriod;
					}
				}
			}
		}
	}
}

void UBaseSensor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Get the name of the property that changed
	const FName PropertyName{ PropertyChangedEvent.GetPropertyName() };

	// Handle enabling/disabling of the sensor in the editor; note that unlike SetScanFrequency and SetScanPeriod,
	// SetSensorEnabled is not idempotent due to the early return -- as such, the callbacks are called directly
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UBaseSensor, bSensorEnabled))
	{
		if (bSensorEnabled)
		{
			// Schedule the next scan
			if (const UWorld* World{ GetWorld() })
			{
				NextScanTime = World->GetTimeSeconds() + ScanPeriod;
			}

			// Allow derived classes to perform additional initialization
			OnEnableSensor();
		}
		else
		{
			// Allow derived classes to perform additional cleanup
			OnDisableSensor();
		}
	}

	// Frequency and period are inversely related; update one if the other changes
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(UBaseSensor, ScanFrequency))
	{
		SetScanFrequency(ScanFrequency);
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(UBaseSensor, ScanPeriod))
	{
		SetScanPeriod(ScanPeriod);
	}
}

bool UBaseSensor::GetSensorEnabled() const
{
	return bSensorEnabled;
}

void UBaseSensor::SetSensorEnabled(bool bNewSensorEnabled)
{
	// Avoid doing unnecessary work if the sensor state has not changed
	if (bSensorEnabled == bNewSensorEnabled) { return; }

	// Update the sensor state
	bSensorEnabled = bNewSensorEnabled;

	if (bSensorEnabled)
	{
		// Schedule the next scan
		if (const UWorld* World{ GetWorld() })
		{
			NextScanTime = World->GetTimeSeconds() + ScanPeriod;
		}

		// Allow derived classes to perform additional initialization
		OnEnableSensor();
	}
	else
	{
		// Allow derived classes to perform additional cleanup
		OnDisableSensor();
	}

	// Notify any listeners that the sensor state has changed
	OnSetSensorEnabled.Broadcast(bSensorEnabled);
}

double UBaseSensor::GetScanFrequency() const
{
	return ScanFrequency;
}

void UBaseSensor::SetScanFrequency(double NewScanFrequency)
{
	// Clamp the frequency to sensible values
	ScanFrequency = FMath::Clamp(NewScanFrequency, 0.01, 100.0);

	// Synchronize the period accordingly
	ScanPeriod = 1.0 / ScanFrequency;

	// If the sensor is enabled, immediately apply the new timing
	// by scheduling the next scan
	if (bSensorEnabled)
	{
		if (const UWorld* World{ GetWorld() })
		{
			NextScanTime = World->GetTimeSeconds() + ScanPeriod;
		}
	}
}

double UBaseSensor::GetScanPeriod() const
{
	return ScanPeriod;
}

void UBaseSensor::SetScanPeriod(double NewScanPeriod)
{
	// Clamp the period to sensible values
	ScanPeriod = FMath::Clamp(NewScanPeriod, 0.01, 100.0);

	// Synchronize the frequency accordingly
	ScanFrequency = 1.0 / ScanPeriod;

	// If the sensor is enabled, immediately apply the new timing
	// by scheduling the next scan
	if (bSensorEnabled)
	{
		if (const UWorld* World{ GetWorld() })
		{
			NextScanTime = World->GetTimeSeconds() + ScanPeriod;
		}
	}
}

double UBaseSensor::GetNextScanTime() const
{
	return NextScanTime;
}

void UBaseSensor::OnEnableSensor_Implementation()
{
	// If additional initialization must be performed on enabling the sensor,
	// override and implement this function
}

void UBaseSensor::OnDisableSensor_Implementation()
{
	// If additional cleanup must be performed on disabling the sensor,
	// override and implement this function
}

void UBaseSensor::PerformScan_Implementation()
{
	// Derived classes SHOULD implement this function; while not strictly enforced,
	// warn the user if this is not the case, in case this decision is unintentional
	UE_LOG(
		LogBaseSensor, Warning,
		TEXT("Sensor \"%s\" has not implemented the PerformScan function."),
		*GetName()
	)
}
