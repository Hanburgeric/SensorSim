#pragma once

#include "CoreMinimal.h"
#include "ShaderParameterStruct.h"

class FTestShader : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FTestShader);
    SHADER_USE_PARAMETER_STRUCT(FTestShader, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER(float, InputValue)
        SHADER_PARAMETER(float, Multiplier)
        SHADER_PARAMETER_UAV(RWBuffer<float>, OutputValue)
    END_SHADER_PARAMETER_STRUCT()

public:
    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    {
        return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
    }
};

// Simple runner class
class UESENSORSHADERS_API FTestShaderRunner
{
public:
    static float Execute(float InputValue, float Multiplier);
};
