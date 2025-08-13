#pragma once

#include "Modules/ModuleManager.h"

class FUESensorsModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
