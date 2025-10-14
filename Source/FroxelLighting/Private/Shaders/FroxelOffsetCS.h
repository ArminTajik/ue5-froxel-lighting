#pragma once
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "FroxelCommon.h"
#include "ScreenPass.h"

/**
 * Wrapper for Froxel OffsetPassA.usf compute shader that computes the froxel offsets based on the light counts per z-slice.
**/
class FFroxelOffsetPassACS : public FGlobalShader {
    DECLARE_GLOBAL_SHADER(FFroxelOffsetPassACS);
    SHADER_USE_PARAMETER_STRUCT(FFroxelOffsetPassACS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters,)
        SHADER_PARAMETER_STRUCT_REF(FFroxelUniformParameters, FroxelSharedUB)
        SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<uint>, FroxelLightCounts)
        SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<uint>, FroxelLightOffsets)
        SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<uint>, ZSliceTotals)
    END_SHADER_PARAMETER_STRUCT()

    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters&) {
        return true; // compile for all RHIs
    }
};


/**
 * Wrapper for Froxel OffsetPassB.usf compute shader that computes the total number of lights of each z-slice.
**/
class FFroxelOffsetPassBCS : public FGlobalShader {
    DECLARE_GLOBAL_SHADER(FFroxelOffsetPassBCS);
    SHADER_USE_PARAMETER_STRUCT(FFroxelOffsetPassBCS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters,)
        SHADER_PARAMETER_STRUCT_REF(FFroxelUniformParameters, FroxelSharedUB)
        SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<uint>, ZSliceTotals)
        SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<uint>, ZSliceOffsets)
    END_SHADER_PARAMETER_STRUCT()

    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters&) {
        return true;
    }
};

/**
 * Wrapper for Froxel OffsetPassC.usf compute shader that adds the z-slice offsets to each froxel in the z-slice.
**/
class FFroxelOffsetPassCCS : public FGlobalShader {
    DECLARE_GLOBAL_SHADER(FFroxelOffsetPassCCS);
    SHADER_USE_PARAMETER_STRUCT(FFroxelOffsetPassCCS, FGlobalShader);
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters,)
        SHADER_PARAMETER_STRUCT_REF(FFroxelUniformParameters, FroxelSharedUB)
        SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<uint>, ZSliceOffsets)
        SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<uint>, FroxelLightOffsets)
    END_SHADER_PARAMETER_STRUCT()

    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters&) {
        return true;
    }
};