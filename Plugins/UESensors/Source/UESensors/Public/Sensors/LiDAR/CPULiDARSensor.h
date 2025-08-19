#pragma once

#include "CoreMinimal.h"
#include "BaseLiDARSensor.h"
#include "CPULiDARSensor.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UESENSORS_API UCPULiDARSensor : public UBaseLiDARSensor
{
	GENERATED_BODY()

public:
	UCPULiDARSensor();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DebugDrawDistance{ 1000.0F };
};
