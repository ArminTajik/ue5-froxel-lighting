#pragma once
// #include "GlobalShader.h"
// #include "ShaderParameterStruct.h"
// #include "FroxelCommon.h"
// #include "ScreenPass.h"
// #include "SceneRenderTargetParameters.h"
//
// /**
//  * Pixel shader to visualize froxel grid overlay.
// **/
// class FFroxelVisualizePS : public FGlobalShader {
//
//     DECLARE_GLOBAL_SHADER(FFroxelVisualizePS);
//     SHADER_USE_PARAMETER_STRUCT(FFroxelVisualizePS, FGlobalShader);
//
//     BEGIN_SHADER_PARAMETER_STRUCT(FParameters,)
//         SHADER_PARAMETER_STRUCT_REF(FFroxelUniformParameters, FroxelSharedUB)
//         SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Input)
//         SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
//         SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FSceneTextureUniformParameters, SceneTextures)
//
//
//         SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, SceneColorCopy)
//         SHADER_PARAMETER_SAMPLER(SamplerState, SceneColorCopySampler)
//         SHADER_PARAMETER(FScreenTransform, SvPositionToInputTextureUV)
//
//         SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, SceneDepthCopy)
//         SHADER_PARAMETER_SAMPLER(SamplerState, SceneDepthCopySampler)
//         // SHADER_PARAMETER(FScreenTransform, SvPositionToDepthUV)
//     
//         // bindings for the output
//         RENDER_TARGET_BINDING_SLOTS()
//     END_SHADER_PARAMETER_STRUCT()
//
//     static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Params) {
//         return true;
//     }
//
//     static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters,
//                                              FShaderCompilerEnvironment& OutEnvironment) {
//         FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
//         //OutEnvironment.SetDefine(TEXT("OVERLAY_PS"), 1); // for later when possibly the shaders are merged.
//     }
// };