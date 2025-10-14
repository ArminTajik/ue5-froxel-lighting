#pragma once
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "FroxelCommon.h"
#include "ScreenPass.h"


/**
 * Wrapper for FroxelAssignLights.usf compute shader that computes the total number of assigned lights.
**/
class FFroxelTotalIndicesCS : public FGlobalShader {
    DECLARE_GLOBAL_SHADER(FFroxelTotalIndicesCS);
    SHADER_USE_PARAMETER_STRUCT(FFroxelTotalIndicesCS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters,)
        SHADER_PARAMETER_STRUCT_REF(FFroxelUniformParameters, FroxelSharedUB)
        SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<uint>, FroxelLightCounts)
        SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<uint>, FroxelLightOffsets)
        SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<uint>, TotalIndices)

    END_SHADER_PARAMETER_STRUCT()

    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters&) {
        return true;
    }
};

/**
 * Wrapper for FroxelAssignLights.usf compute shader that assigns lights to froxels.
**/
class FFroxelAssignLightsCS : public FGlobalShader {
    DECLARE_GLOBAL_SHADER(FFroxelAssignLightsCS);
    SHADER_USE_PARAMETER_STRUCT(FFroxelAssignLightsCS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters,)
        SHADER_PARAMETER_STRUCT_REF(FFroxelUniformParameters, FroxelSharedUB)
        SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)

        SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<FFroxelLightData>, Lights)
        SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<uint>, FroxelLightOffsets)
        SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<uint>, TotalIndices)

        SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<uint>, FroxelHeads)
        SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<uint>, FroxelLightIndices)
    END_SHADER_PARAMETER_STRUCT()

    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters&) {
        return true;
    }
};