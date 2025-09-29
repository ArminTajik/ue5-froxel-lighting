#pragma once
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"

/*
 * Wrapper for FroxelBuild.usf
*/
class FFroxelBuildCS : public FGlobalShader {
    DECLARE_GLOBAL_SHADER(FFroxelBuildCS);
    SHADER_USE_PARAMETER_STRUCT(FFroxelBuildCS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters,)
        SHADER_PARAMETER(uint, GridX)
        SHADER_PARAMETER(uint, GridY)
        SHADER_PARAMETER(uint, GridZ)
        SHADER_PARAMETER(uint, GridCount)
        SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<uint>, FroxelGrid)
    END_SHADER_PARAMETER_STRUCT()

public:
    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Params) {
        return true; // compile for all RHIs
    }
};