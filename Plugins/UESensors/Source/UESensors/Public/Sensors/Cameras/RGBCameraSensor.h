#pragma once

#include "CoreMinimal.h"
#include "BaseCameraSensor.h"
#include "RGBCameraSensor.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UESENSORS_API URGBCameraSensor : public UBaseCameraSensor
{
	GENERATED_BODY()

public:
	URGBCameraSensor();
};
