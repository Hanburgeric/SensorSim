// Copyright Epic Games, Inc. All Rights Reserved.

#include "SensorSimWheelFront.h"
#include "UObject/ConstructorHelpers.h"

USensorSimWheelFront::USensorSimWheelFront()
{
	AxleType = EAxleType::Front;
	bAffectedBySteering = true;
	MaxSteerAngle = 40.f;
}