#pragma once
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "FroxelCommon.h"

/*
 * Wrapper for Froxel visualization compute shader
*/
class FFroxelBuildOverlayCS : public FGlobalShader {
    DECLARE_GLOBAL_SHADER(FFroxelBuildOverlayCS);
    SHADER_USE_PARAMETER_STRUCT(FFroxelBuildOverlayCS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters,)
        SHADER_PARAMETER_STRUCT_REF(FFroxelUniformParameters, FroxelUB)
        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutTex)
    END_SHADER_PARAMETER_STRUCT()

    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Params) {
        return true;
    }
};