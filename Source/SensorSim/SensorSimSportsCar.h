#pragma once

#include "CoreMinimal.h"
#include "SensorSimPawn.h"
#include "SensorSimSportsCar.generated.h"

// Forward declarations
class ULidarSensor;

UCLASS(Abstract)
class SENSORSIM_API ASensorSimSportsCar : public ASensorSimPawn
{
	GENERATED_BODY()
	
public:
	ASensorSimSportsCar();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<ULidarSensor> Lidar{ nullptr };
};
