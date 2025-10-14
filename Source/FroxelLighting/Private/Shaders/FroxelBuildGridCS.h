#pragma once
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "FroxelCommon.h"

/*
 * Wrapper for FroxelBuild.usf
*/
class FFroxelBuildGridCS : public FGlobalShader {
    DECLARE_GLOBAL_SHADER(FFroxelBuildGridCS);
    SHADER_USE_PARAMETER_STRUCT(FFroxelBuildGridCS, FGlobalShader);


    BEGIN_SHADER_PARAMETER_STRUCT(FParameters,)
        // SHADER_PARAMETER_STRUCT_REF(FFroxelUniformParameters, FroxelUB)
        SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<uint>, FroxelGrid)
    END_SHADER_PARAMETER_STRUCT()

    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
        return true; // compile for all RHIs
    }
};