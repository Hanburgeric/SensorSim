#include "UESensorShaders.h"

// UE
#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "FUESensorShadersModule"

void FUESensorShadersModule::StartupModule()
{
	// Map virtual shader source directory to the actual shader source directory
	const FString ShaderDirectory{
		FPaths::Combine(
			IPluginManager::Get().FindPlugin(TEXT(""))->GetBaseDir(),
			TEXT("")
		)
	};

	AddShaderSourceDirectoryMapping(TEXT(""), ShaderDirectory);
}

void FUESensorShadersModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FUESensorShadersModule, UESensorShaders)
