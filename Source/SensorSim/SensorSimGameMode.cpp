// Copyright Epic Games, Inc. All Rights Reserved.

#include "SensorSimGameMode.h"
#include "SensorSimPlayerController.h"

ASensorSimGameMode::ASensorSimGameMode()
{
	PlayerControllerClass = ASensorSimPlayerController::StaticClass();
}
