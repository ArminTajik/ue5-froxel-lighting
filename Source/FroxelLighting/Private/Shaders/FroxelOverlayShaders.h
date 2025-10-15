#pragma once
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "FroxelCommon.h"
#include "ScreenPass.h"
#include "SceneRenderTargetParameters.h"
// BEGIN_UNIFORM_BUFFER_STRUCT(FFroxelOverlayUniformParameters,)
//
//     SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Input)
//     SHADER_PARAMETER(FScreenTransform, SvPositionToInputTextureUV)
//
//     SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, SceneColorCopy)
//     SHADER_PARAMETER_SAMPLER(SamplerState, SceneColorCopySampler)
//
//     SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, SceneDepthCopy)
//     SHADER_PARAMETER_SAMPLER(SamplerState, SceneDepthCopySampler)
//
//     SHADER_PARAMETER(float, OverlayOpacity)
// END_UNIFORM_BUFFER_STRUCT();

// /**
//  * Pixel shader to visualize froxel grid overlay.
// **/
// class FFroxelOverlayBasePS : public FGlobalShader {
//
//     DECLARE_GLOBAL_SHADER(FFroxelOverlayBasePS);
//     SHADER_USE_PARAMETER_STRUCT(FFroxelOverlayBasePS, FGlobalShader);
//
//    
// };

/**
 * Pixel shader to visualize froxel grid overlay with hash pattern.
 **/
class FFroxelOverlayHashPS : public FGlobalShader {
    DECLARE_GLOBAL_SHADER(FFroxelOverlayHashPS);
    SHADER_USE_PARAMETER_STRUCT(FFroxelOverlayHashPS, FGlobalShader);
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters,)
        SHADER_PARAMETER_STRUCT_REF(FFroxelUniformParameters, FroxelSharedUB)
        // SHADER_PARAMETER_STRUCT_REF(FFroxelOverlayUniformParameters, FroxelOverlayUB)
        SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
        SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Input)
        SHADER_PARAMETER(FScreenTransform, SvPositionToInputTextureUV)

        SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, SceneColorCopy)
        SHADER_PARAMETER_SAMPLER(SamplerState, SceneColorCopySampler)

        SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, SceneDepthCopy)
        SHADER_PARAMETER_SAMPLER(SamplerState, SceneDepthCopySampler)

        SHADER_PARAMETER(float, OverlayOpacity)


        RENDER_TARGET_BINDING_SLOTS()

    END_SHADER_PARAMETER_STRUCT()

    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Params) {
        return true;
    }

};

/**
 * Pixel shader to visualize froxel grid overlay with solid color.
 **/
class FFroxelOverlayHeatmapPS : public FGlobalShader {
    DECLARE_GLOBAL_SHADER(FFroxelOverlayHeatmapPS);
    SHADER_USE_PARAMETER_STRUCT(FFroxelOverlayHeatmapPS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters,)
        SHADER_PARAMETER_STRUCT_REF(FFroxelUniformParameters, FroxelSharedUB)
        //         // SHADER_PARAMETER_STRUCT_REF(FFroxelOverlayUniformParameters, FroxelOverlayUB)
        SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
        SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Input)
        SHADER_PARAMETER(FScreenTransform, SvPositionToInputTextureUV)
        
        SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, SceneColorCopy)
        SHADER_PARAMETER_SAMPLER(SamplerState, SceneColorCopySampler)
        
        SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, SceneDepthCopy)
        SHADER_PARAMETER_SAMPLER(SamplerState, SceneDepthCopySampler)
        
        SHADER_PARAMETER(float, OverlayOpacity)
        SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<uint>, FroxelLightCounts)
        SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<uint>, FroxelLightOffsets)
        SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<uint>, FroxelLightIndices)
        RENDER_TARGET_BINDING_SLOTS()
        //
    END_SHADER_PARAMETER_STRUCT()

    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Params) {
        return true;
    }

    static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters,
                                             FShaderCompilerEnvironment& OutEnvironment) {
        FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
        OutEnvironment.SetDefine(TEXT("USE_FROXEL_COUNTS"), 1);
    }
};