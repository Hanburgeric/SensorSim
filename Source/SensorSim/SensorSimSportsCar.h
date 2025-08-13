// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SensorSimPawn.h"
#include "SensorSimSportsCar.generated.h"

/**
 *  Sports car wheeled vehicle implementation
 */
UCLASS(abstract)
class SENSORSIM_API ASensorSimSportsCar : public ASensorSimPawn
{
	GENERATED_BODY()
	
public:

	ASensorSimSportsCar();
};
