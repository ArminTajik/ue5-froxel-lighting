#pragma once
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "FroxelCommon.h"
#include "ScreenPass.h"

/**
 * Wrapper for FroxelCount.usf compute shader that counts the number of lights per froxel.
**/
class FFroxelCountCS : public FGlobalShader {
    DECLARE_GLOBAL_SHADER(FFroxelCountCS);
    SHADER_USE_PARAMETER_STRUCT(FFroxelCountCS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters,)
        SHADER_PARAMETER_STRUCT_REF(FFroxelUniformParameters, FroxelSharedUB)
        SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)

        SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<FFroxelLightData>, Lights)
        SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<uint>, FroxelLightCounts)
    END_SHADER_PARAMETER_STRUCT()

    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters&) {
        return true; // compile for all RHIs
    }
};