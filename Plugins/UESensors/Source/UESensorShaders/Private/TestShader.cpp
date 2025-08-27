#include "TestShader.h"

// Define the shader implementation
IMPLEMENT_GLOBAL_SHADER(FTestShader, "/Plugin/UESensors/TestShader.usf", "MainCS", SF_Compute);

float FTestShaderRunner::Execute(float InputValue, float Multiplier)
{
    // For now, just do it on CPU to verify the infrastructure works
    float Result = InputValue * Multiplier;

    UE_LOG(LogTemp, Warning, TEXT("TestShader: %.2f * %.2f = %.2f (CPU calculation for now)"),
        InputValue, Multiplier, Result);

    // Try to verify shader compilation at least
    ENQUEUE_RENDER_COMMAND(TestShaderCompilation)(
        [](FRHICommandListImmediate& RHICmdList)
        {
            TShaderMapRef<FTestShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
            if (ComputeShader.IsValid())
            {
                UE_LOG(LogTemp, Warning, TEXT("TestShader compiled successfully!"));
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("TestShader failed to compile!"));
            }
        }
        );

    FlushRenderingCommands();

    return Result;
}
