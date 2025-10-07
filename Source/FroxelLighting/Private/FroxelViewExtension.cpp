#include "FroxelViewExtension.h"
#include "FroxelLighting.h"
#include "FroxelUtils.h"
#include "Shaders/FroxelOverlay.h"
#include "Shaders/FroxelBuildGridCS.h"

#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "GlobalShader.h"
#include "HAL/IConsoleManager.h"
#include "PixelShaderUtils.h"
#include "RHIGPUReadback.h"
#include "RHICommandList.h"
#include "Shaders/FroxelBuildOverlayCS.h"
#include "RHIResources.h"
#include "RHI.h"
#include "SceneRendererInterface.h"
#include "SceneRenderTargetParameters.h"
#include "SceneUniformBuffer.h"
#include "SceneViewExtension.h"
#include "PostProcess/PostProcessMaterialInputs.h"

DEFINE_GPU_STAT(Froxel);

extern TAutoConsoleVariable<int32> CVarFroxelGridX;
extern TAutoConsoleVariable<int32> CVarFroxelGridY;
extern TAutoConsoleVariable<int32> CVarFroxelGridZ;
extern TAutoConsoleVariable<int32> CVarFroxelVisualize;
extern TAutoConsoleVariable<int32> CVarFroxelMaxLightsPerFroxel;
extern TAutoConsoleVariable<int32> CVarFroxelFarClipCm;
extern TAutoConsoleVariable<int32> CVarFroxelZMode;



void FFroxelViewExtension::PreRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView) {
    FSceneViewExtensionBase::PreRenderView_RenderThread(GraphBuilder, InView);

    //  BuildFroxelGrid(GraphBuilder, InView);
}

void FFroxelViewExtension::PostRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView) {
    FSceneViewExtensionBase::PostRenderView_RenderThread(GraphBuilder, InView);
    // VisualizeFroxelGrid(GraphBuilder, InView);
}


void FFroxelViewExtension::SubscribeToPostProcessingPass(EPostProcessingPass Pass, const FSceneView& InView,
                                                         FPostProcessingPassDelegateArray& InOutPassCallbacks,
                                                         bool bIsPassEnabled) {
    FSceneViewExtensionBase::SubscribeToPostProcessingPass(Pass, InView, InOutPassCallbacks, bIsPassEnabled);

    UE_LOG(LogFroxelLighting, VeryVerbose, TEXT("[Froxel] VisualizeFroxelGrid called"));
    if (CVarFroxelVisualize.GetValueOnRenderThread() == 0) {
        return;
    }

    if (Pass == EPostProcessingPass::Tonemap && bIsPassEnabled) {
        InOutPassCallbacks.Add(
            FPostProcessingPassDelegate::CreateRaw(this, &FFroxelViewExtension::AddFroxelOverlayPass));
    }

}


void FFroxelViewExtension::BuildFroxelGrid(FRDGBuilder& GraphBuilder, const FSceneView& InView) {

    const ERDGPassFlags RDGPassFlags = ERDGPassFlags::Compute | ERDGPassFlags::NeverCull;
    UE_LOG(LogFroxelLighting, VeryVerbose, TEXT("[Froxel] BuildFroxelGrid called"));

    const int32 GX = FMath::Max(1, CVarFroxelGridX.GetValueOnRenderThread());
    const int32 GY = FMath::Max(1, CVarFroxelGridY.GetValueOnRenderThread());
    const int32 GZ = FMath::Max(1, CVarFroxelGridZ.GetValueOnRenderThread());
    const int32 GCOUNT = GX * GY * GZ;
    FIntVector GridSize = FIntVector(GX, GY, GZ);

    FRDGBufferRef FroxelBuffer = GraphBuilder.CreateBuffer(
        FRDGBufferDesc::CreateStructuredDesc(sizeof(uint32), GCOUNT),
        TEXT("FroxelGrid")
        );

    FRDGBufferUAVRef FroxelUAV = GraphBuilder.CreateUAV(FroxelBuffer);
    FRDGBufferSRVRef FroxelSRV = GraphBuilder.CreateSRV(FroxelBuffer);

    AddClearUAVPass(GraphBuilder, FroxelUAV, 0u, RDGPassFlags);

    TShaderMapRef<FFroxelBuildGridCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

    auto* Params = GraphBuilder.AllocParameters<FFroxelBuildGridCS::FParameters>();

    FroxelUB = FFroxelUtils::CreateFroxelUB(GraphBuilder, GridSize, InView);
    Params->FroxelUB = FroxelUB;
    Params->FroxelGrid = FroxelUAV;

    // SharedParams = GraphBuilder.AllocParameters<FFroxelSharedParameters>();
    // SharedParams->GridSize = FIntVector4(GX, GY, GZ, GCOUNT);
    // SharedParams->FroxelGridUAV = FroxelUAV;
    // SharedParams->InvProj = FMatrix44f(InView.ViewMatrices.GetInvProjectionMatrix());
    // Params->DepthParams = FVector2f(1.0f / InView.NearClippingDistance, (1.0f / InView.cli) - (1.0f / NearZ));

    // SharedUB = GraphBuilder.CreateUniformBuffer(SharedParams);
    // Params->Shared = SharedUB;

    // get RHICmdList

    const FIntVector Groups(FComputeShaderUtils::GetGroupCount(
        FIntVector(static_cast<int32>(GX), static_cast<int32>(GY), static_cast<int32>(GZ)),
        FIntVector(8, 8, 8)));

    // UE_LOG(LogFroxelLighting, VeryVerbose, TEXT("Groups = (%d,%d,%d), Grid = (%d,%d,%d)"),
    //        Groups.X, Groups.Y, Groups.Z, GX, GY, GZ);

    RDG_GPU_STAT_SCOPE(GraphBuilder, Froxel);
    RDG_EVENT_SCOPE(GraphBuilder, "FroxelScope");

    FComputeShaderUtils::AddPass(GraphBuilder,
                                 RDG_EVENT_NAME("Froxel: BuildGrid [%u x %u x %u]", GX, GY, GZ),
                                 RDGPassFlags,
                                 ComputeShader,
                                 Params,
                                 Groups);

    // GraphBuilder.AddPass(
    //     RDG_EVENT_NAME("Froxel: AfterBuildGrid"),
    //     ERDGPassFlags::None,
    //     [](FRHICommandListImmediate& RHICmdList) {
    //         SCOPED_DRAW_EVENT(RHICmdList, FroxelAfterBuildGrid);
    //     });

    // FFroxelReadbackEntry ReadbackEntry;
    // ReadbackEntry.Readback = MakeUnique<FRHIGPUBufferReadback>(TEXT("FroxelReadback"));
    // ReadbackEntry.NumBytes = NumBytes;
    // ReadbackEntry.GCount = GCOUNT;
    // ReadbackEntry.Timestamp = FPlatformTime::Seconds();
    //
    // AddEnqueueCopyPass(GraphBuilder, ReadbackEntry.Readback.Get(), FroxelBuffer, NumBytes);
    //
    // this->PendingReadbacks.Add(MoveTemp(ReadbackEntry));

    // GraphBuilder.AddPass(
    //     RDG_EVENT_NAME("Froxel: ReadbackResolve"),
    //     ERDGPassFlags::None,
    //     [this](FRHICommandListImmediate& RHICmdList) {
    //         for (int32 i = PendingReadbacks.Num() - 1; i >= 0; --i) {
    //             if (const FFroxelReadbackEntry& Entry = PendingReadbacks[i]; Entry.Readback->IsReady()) {
    //                 const uint32* Data = static_cast<const uint32*>(Entry.Readback->Lock(Entry.NumBytes));
    //                 UE_LOG(LogFroxelLighting, VeryVerbose, TEXT("Froxel[0]=%u mid=%u last=%u"),
    //                        Data[0], Data[Entry.GCount / 2], Data[Entry.GCount - 1]);
    //                 Entry.Readback->Unlock();
    //
    //                 PendingReadbacks.RemoveAtSwap(i);
    //             }
    //         }
    //     });

    UE_LOG(LogFroxelLighting, VeryVerbose, TEXT("[Froxel] BuildFroxelGrid finished"));
}


FRDGTextureRef FFroxelViewExtension::BuildFroxelOverlay(FRDGBuilder& GraphBuilder, const FSceneView& InView) {

    // Size of the visualization texture (match viewport)

    RDG_EVENT_SCOPE(GraphBuilder, "FroxelOverlay");
    const FIntPoint OutputSize = InView.UnscaledViewRect.Size();

    FRDGTextureDesc Desc = FRDGTextureDesc::Create2D(
        OutputSize,
        PF_FloatRGBA,
        FClearValueBinding::Transparent,
        TexCreate_ShaderResource | TexCreate_UAV | TexCreate_RenderTargetable);

    FRDGTextureRef OverlayTex = GraphBuilder.CreateTexture(Desc, TEXT("FroxelOverlay"));

    // Run visualization compute shader
    TShaderMapRef<FFroxelBuildOverlayCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

    auto* Params = GraphBuilder.AllocParameters<FFroxelBuildOverlayCS::FParameters>();
    // PassParameters->SharedParameters.GridSize   = FIntVector4(OutputSize.X, OutputSize.Y, 1, 0);
    // PassParameters->Shared.DepthParams = FVector2f(0,0); // fill with real if needed
    // Params->ViewportSize = OutputSize;
    Params->OutTex = GraphBuilder.CreateUAV(OverlayTex);

    FIntVector GroupCount = FComputeShaderUtils::GetGroupCount(OutputSize, FIntPoint(8, 8));

    FComputeShaderUtils::AddPass(
        GraphBuilder,
        RDG_EVENT_NAME("FroxelVisualizeCS"),
        ComputeShader,
        Params,
        GroupCount);

    return OverlayTex;
}

FScreenPassTexture FFroxelViewExtension::AddFroxelOverlayPass(
    FRDGBuilder& GraphBuilder,
    const FSceneView& View,
    const FPostProcessMaterialInputs& Inputs) {

    FScreenPassTexture InputSceneColor(Inputs.GetInput(EPostProcessMaterialInput::SceneColor));

    // FRDGTextureRef OverlayTex = BuildFroxelOverlay(GraphBuilder, View);
    // if (!OverlayTex)
    //     return InputSceneColor;

    const FIntRect ViewRect = View.UnconstrainedViewRect;
    const FScreenPassTextureViewport InputViewport(InputSceneColor);
    FRDGTextureRef DepthTextureRef = Inputs.SceneTextures.SceneTextures->GetParameters()->SceneDepthTexture;

    const int32 GX = FMath::Max(1, CVarFroxelGridX.GetValueOnRenderThread());
    const int32 GY = FMath::Max(1, CVarFroxelGridY.GetValueOnRenderThread());
    const int32 GZ = FMath::Max(1, CVarFroxelGridZ.GetValueOnRenderThread());
    FIntVector GridSize = FIntVector(GX, GY, GZ);

    auto* Params = GraphBuilder.AllocParameters<FFroxelOverlayPS::FParameters>();
    Params->FroxelUB = FFroxelUtils::CreateFroxelUB(GraphBuilder, GridSize, View);
    Params->Input = GetScreenPassTextureViewportParameters(InputViewport);
    Params->View = View.ViewUniformBuffer;
    Params->SceneTextures = Inputs.SceneTextures.SceneTextures;
    Params->SceneColorCopy = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(InputSceneColor.Texture));
    Params->SceneColorCopySampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
    Params->SceneDepthCopy = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(DepthTextureRef));
    Params->SceneDepthCopySampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
    Params->SvPositionToInputTextureUV =
        FScreenTransform::ChangeTextureBasisFromTo(InputViewport, FScreenTransform::ETextureBasis::TexelPosition,
                                                   FScreenTransform::ETextureBasis::ViewportUV) *
        FScreenTransform::ChangeTextureBasisFromTo(InputViewport, FScreenTransform::ETextureBasis::ViewportUV,
                                                   FScreenTransform::ETextureBasis::TextureUV);
    Params->RenderTargets[0] = FRenderTargetBinding(InputSceneColor.Texture, ERenderTargetLoadAction::ELoad);

    // Params->SvPositionToDepthUV = FScreenTransform::ChangeTextureBasisFromTo(InputViewport, FScreenTransform::ETextureBasis::TexelPosition,
    //                                                FScreenTransform::ETextureBasis::ViewportUV) *
    //     FScreenTransform::ChangeTextureBasisFromTo(InputViewport, FScreenTransform::ETextureBasis::ViewportUV,
    //                                                FScreenTransform::ETextureBasis::TextureUV);

    FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
    TShaderMapRef<FFroxelOverlayPS> PixelShader(GlobalShaderMap);

    FPixelShaderUtils::AddFullscreenPass(
        GraphBuilder,
        GlobalShaderMap,
        RDG_EVENT_NAME("FroxelOverlay (FPixelShaderUtils)"),
        PixelShader,
        Params,
        ViewRect);

    return InputSceneColor;
}


void FFroxelViewExtension::VisualizeFroxelGrid(FRDGBuilder& GraphBuilder, const FSceneView& InView) {

    extern TAutoConsoleVariable<int32> CVarFroxelVisualize;
    UE_LOG(LogFroxelLighting, VeryVerbose, TEXT("[Froxel] VisualizeFroxelGrid called"));
    if (CVarFroxelVisualize.GetValueOnRenderThread() == 0) {
        return;
    }

    if (!InView.Family)
        return;

    if (InView.bIsSceneCapture ||
        InView.bIsReflectionCapture ||
        InView.bIsPlanarReflection ||
        InView.bIsVirtualTexture) {
        return;
    }

    if (!InView.IsPrimarySceneView())
        return;

    FRDGTextureRef OverlayTex = BuildFroxelOverlay(GraphBuilder, InView);
    if (!OverlayTex)
        return;

    // Get back buffer as RDG texture
    if (!InView.bIsViewInfo || !InView.Family || !InView.Family->RenderTarget)
        return;

    FRHITexture* BackBufferRHI = InView.Family->RenderTarget->GetRenderTargetTexture();
    if (!BackBufferRHI)
        return;

    FRDGTextureRef BackBufferRDG = GraphBuilder.FindExternalTexture(BackBufferRHI);
    if (!BackBufferRDG) {
        BackBufferRDG = GraphBuilder.RegisterExternalTexture(CreateRenderTarget(BackBufferRHI, TEXT("BackBuffer")));
    }

    //
    // const FRDGTextureDesc CopyDesc = BackBufferRDG->Desc;
    // FRDGTextureRef SceneColorCopyTex = GraphBuilder.CreateTexture(CopyDesc, TEXT("SceneColorCopy"));
    // AddCopyTexturePass(GraphBuilder, BackBufferRDG, SceneColorCopyTex);

    const FIntRect ViewRect = InView.UnscaledViewRect;
    const FIntPoint ViewSize = ViewRect.Size();

    // FScreenPassTexture InputSPT(SceneColorCopyTex, ViewRect);
    // const FScreenPassTextureViewport InputViewport(InputSPT);

    auto* Params = GraphBuilder.AllocParameters<FFroxelOverlayPS::FParameters>();

    // FRDGTexture* SceneColorRDG = GraphBuilder.
    //     FindExternalTexture(InView.Family->RenderTarget->GetRenderTargetTexture());
    // if (!SceneColorRDG)
    //     return;

    const int32 GX = FMath::Max(1, CVarFroxelGridX.GetValueOnRenderThread());
    const int32 GY = FMath::Max(1, CVarFroxelGridY.GetValueOnRenderThread());
    const int32 GZ = FMath::Max(1, CVarFroxelGridZ.GetValueOnRenderThread());
    FIntVector GridSize = FIntVector(GX, GY, GZ);

    Params->FroxelUB = FFroxelUtils::CreateFroxelUB(GraphBuilder, GridSize, InView);

    // Params->SceneColorCopy = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(SceneColorCopyTex));
    // Params->SceneColorCopySampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();

    const FScreenPassTextureViewport InputViewport(OverlayTex, ViewRect);
    Params->Input = GetScreenPassTextureViewportParameters(InputViewport);
    Params->View = InView.ViewUniformBuffer;

    const FSceneTextureShaderParameters SceneTexParams = GetSceneTextureShaderParameters(InView);
    if (!SceneTexParams.SceneTextures)
        return;
    Params->SceneTextures = SceneTexParams.SceneTextures;

    Params->SvPositionToInputTextureUV =
        // SV_Position (pixel space) -> Input ViewportUV -> Input TextureUV
        FScreenTransform::ChangeTextureBasisFromTo(InputViewport, FScreenTransform::ETextureBasis::TexelPosition,
                                                   FScreenTransform::ETextureBasis::ViewportUV) *
        FScreenTransform::ChangeTextureBasisFromTo(InputViewport, FScreenTransform::ETextureBasis::ViewportUV,
                                                   FScreenTransform::ETextureBasis::TextureUV);
    Params->RenderTargets[0] = FRenderTargetBinding(BackBufferRDG, ERenderTargetLoadAction::ELoad);

    FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);

    TShaderMapRef<FFroxelOverlayPS> PixelShader(GlobalShaderMap);
    // GetScreenPassTextureViewportParameters(
    FPixelShaderUtils::AddFullscreenPass(
        GraphBuilder,
        GlobalShaderMap,
        RDG_EVENT_NAME("FroxelOverlay (FPixelShaderUtils)"),
        PixelShader,
        Params,
        ViewRect);
}