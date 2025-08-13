// Copyright Epic Games, Inc. All Rights Reserved.

#include "SensorSimWheelRear.h"
#include "UObject/ConstructorHelpers.h"

USensorSimWheelRear::USensorSimWheelRear()
{
	AxleType = EAxleType::Rear;
	bAffectedByHandbrake = true;
	bAffectedByEngine = true;
}