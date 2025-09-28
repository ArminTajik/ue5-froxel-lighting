#pragma once
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"

/*
 * Wrapper for FroxelClear.usf
*/
class FFroxelClearCS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FFroxelClearCS);
    SHADER_USE_PARAMETER_STRUCT(FFroxelClearCS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER(uint32, OutputWidth)
        SHADER_PARAMETER(uint32, OutputHeight)
        SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<uint>, OutData)
    END_SHADER_PARAMETER_STRUCT()
public:
    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Params)
    {
        return true; // compile for all RHIs
    }
};