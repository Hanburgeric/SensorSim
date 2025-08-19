#pragma once

#include "CoreMinimal.h"
#include "BaseCameraSensor.h"
#include "DepthCameraSensor.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UESENSORS_API UDepthCameraSensor : public UBaseCameraSensor
{
	GENERATED_BODY()

public:
	UDepthCameraSensor();
};
