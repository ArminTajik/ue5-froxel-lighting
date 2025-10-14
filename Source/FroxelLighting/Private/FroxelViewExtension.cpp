#include "FroxelViewExtension.h"
#include "FroxelLighting.h"
#include "FroxelUtils.h"

// Shaders
#include "Shaders/FroxelOverlay.h"
#include "Shaders/FroxelBuildGridCS.h"
#include "Shaders/FroxelCountCS.h"
#include "Shaders/FroxelOffsetCS.h"
#include "Shaders/FroxelAssignLightsCS.h"

// Unreal Engine
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "GlobalShader.h"
#include "HAL/IConsoleManager.h"
#include "PixelShaderUtils.h"
#include "RHICommandList.h"
#include "RHIResources.h"
#include "RHI.h"
#include "RHIGPUReadback.h"
#include "SceneRenderTargetParameters.h"
#include "SceneViewExtension.h"
#include "Components/LightComponent.h"
#include "Data/FroxelData.h"
#include "PostProcess/PostProcessMaterialInputs.h"


DEFINE_GPU_STAT(Froxel);

// Console variables --------------------------------------------
extern TAutoConsoleVariable<int32> CVarFroxelGridX;
extern TAutoConsoleVariable<int32> CVarFroxelGridY;
extern TAutoConsoleVariable<int32> CVarFroxelGridZ;
extern TAutoConsoleVariable<int32> CVarFroxelVisualize;
extern TAutoConsoleVariable<int32> CVarFroxelMaxLightsPerFroxel;
extern TAutoConsoleVariable<int32> CVarFroxelFarClipCm;
extern TAutoConsoleVariable<int32> CVarFroxelZMode;

// FFroxelViewExtension implementation --------------------------------------------
void FFroxelViewExtension::SetupViewFamily(FSceneViewFamily& InViewFamily) {
    FSceneViewExtensionBase::SetupViewFamily(InViewFamily);

    if (const FSceneInterface* Scene = InViewFamily.Scene; !Scene || !Scene->HasAnyLights())
        return;

    TArray<FFroxelLightData> FrameLights;
    GetFrameLights(InViewFamily, FrameLights);
    {
        // writer thread (GT)
        FScopeLock Lock(&LightBufferMutex); // locking to avoid race condition with render thread
        this->FrameLights_GT = MoveTemp(FrameLights);
        UE_LOG(LogFroxelLighting, VeryVerbose, TEXT("Captured %d lights for froxelization"), FrameLights_GT.Num());
    }
}

void FFroxelViewExtension::PreRenderViewFamily_RenderThread(FRDGBuilder& GraphBuilder, FSceneViewFamily& InViewFamily) {
    FSceneViewExtensionBase::PreRenderViewFamily_RenderThread(GraphBuilder, InViewFamily);

    const int32 GX = FMath::Max(1, CVarFroxelGridX.GetValueOnRenderThread());
    const int32 GY = FMath::Max(1, CVarFroxelGridY.GetValueOnRenderThread());
    const int32 GZ = FMath::Max(1, CVarFroxelGridZ.GetValueOnRenderThread());
    GridSize_RT = FIntVector(GX, GY, GZ);
    GridCount_RT = GX * GY * GZ;

    {
        // reader thread (RT)
        FScopeLock Lock(&LightBufferMutex); // locking to avoid race condition with game thread
        FrameLights_RT = FrameLights_GT;
        NumLights_RT = FrameLights_RT.Num();
    }
}


void FFroxelViewExtension::PreRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView) {
    FSceneViewExtensionBase::PreRenderView_RenderThread(GraphBuilder, InView);

}

void FFroxelViewExtension::PostRenderBasePassDeferred_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView,
                                                                   const FRenderTargetBindingSlots& RenderTargets,
                                                                   TRDGUniformBufferRef<FSceneTextureUniformParameters>
                                                                   SceneTextures) {
    FSceneViewExtensionBase::
        PostRenderBasePassDeferred_RenderThread(GraphBuilder, InView, RenderTargets, SceneTextures);

    // early out
    if (NumLights_RT == 0) {
        UE_LOG(LogFroxelLighting, VeryVerbose, TEXT("No lights to froxelize"));
        return;
    }

    TArray<FFroxelLightData> LightsInView;
    LightsInView.Reserve(NumLights_RT);
    for (const auto& L : FrameLights_RT) {
        if (InView.ViewFrustum.IntersectSphere(FVector(L.PositionWS), L.Radius)) {
            LightsInView.Add(L);
        }
    }

    const uint32 NumLightsInView = LightsInView.Num();
    if (NumLightsInView == 0) {
        UE_LOG(LogFroxelLighting, VeryVerbose, TEXT("No lights in view frustum"));
        return;
    }

    constexpr uint32 Stride = sizeof(FFroxelLightData);

    LightsSB = CreateStructuredBuffer(
        GraphBuilder,
        TEXT("LightsSB"),
        Stride,
        NumLightsInView,
        LightsInView.GetData(),
        Stride * NumLightsInView);


    RDG_GPU_STAT_SCOPE(GraphBuilder, Froxel);
    RDG_EVENT_SCOPE(GraphBuilder, "FroxelScope");
    
    FroxelLists = FFroxelUtils::CreateFroxelLists(GraphBuilder, GridSize_RT,
                                                  CVarFroxelMaxLightsPerFroxel.GetValueOnRenderThread());
    SharedUB = FFroxelUtils::CreateSharedUB(GraphBuilder, InView, GridSize_RT, NumLightsInView);
    CountLightPerFroxel(GraphBuilder, InView);
    ComputeFroxelOffset(GraphBuilder, InView);
    ComputeLightIndices(GraphBuilder, InView);
}


void FFroxelViewExtension::SubscribeToPostProcessingPass(EPostProcessingPass Pass, const FSceneView& InView,
                                                         FPostProcessingPassDelegateArray& InOutPassCallbacks,
                                                         bool bIsPassEnabled) {
    FSceneViewExtensionBase::SubscribeToPostProcessingPass(Pass, InView, InOutPassCallbacks, bIsPassEnabled);

    if (CVarFroxelVisualize.GetValueOnRenderThread() == 0) {
        return;
    }
    
    if (Pass == EPostProcessingPass::Tonemap && bIsPassEnabled) {
        InOutPassCallbacks.Add(
            FPostProcessingPassDelegate::CreateRaw(this, &FFroxelViewExtension::VisualizeFroxelOverlayPS));
    }

}

void FFroxelViewExtension::GetFrameLights(FSceneViewFamily& InViewFamily, TArray<FFroxelLightData>& OutLights) {
    const FSceneInterface* SceneInterface = InViewFamily.Scene;

    if (!SceneInterface)
        return;

    const UWorld* World = SceneInterface->GetWorld();
    if (!World)
        return;

    for (TObjectIterator<ULightComponent> It; It; ++It) {
        const ULightComponent* LC = *It;
        if (!LC->IsRegistered()
            || LC->GetWorld() != World
            || !LC->bAffectsWorld
            || !LC->IsVisible()
        )
            continue;

        if (LC->GetLightType() == LightType_Directional)
            continue;

        FSphere BoundingSphere = LC->GetBoundingSphere();
        if (BoundingSphere.W < KINDA_SMALL_NUMBER)
            continue;

        FFroxelLightData Ld;
        Ld.PositionWS = FVector3f(BoundingSphere.Center);
        Ld.Radius = BoundingSphere.W;

        OutLights.Add(Ld);
    }

}

// Passes -------------------------------------------------------------

/**
 * Count the number of lights affecting each froxel.
 * The result is stored in FroxelLightCountsUAV buffer and returned as an RDG buffer to pass to the next stage.
 **/
void FFroxelViewExtension::CountLightPerFroxel(FRDGBuilder& GraphBuilder, FSceneView& InView) {

    if (!LightsSB) {
        UE_LOG(LogFroxelLighting, VeryVerbose, TEXT("Lights is null"));
        return;
    }
    if (!FroxelLists.Counts) {
        UE_LOG(LogFroxelLighting, VeryVerbose, TEXT("FroxelLightCounts is null"));
        return;
    }


    
    FRDGBufferUAVRef FroxelLightCountsUAV = GraphBuilder.CreateUAV(FroxelLists.Counts);
    AddClearUAVPass(GraphBuilder, FroxelLightCountsUAV, 0u);

    auto* Params = GraphBuilder.AllocParameters<FFroxelCountCS::FParameters>();

    if (!InView.ViewUniformBuffer) {
        UE_LOG(LogFroxelLighting, VeryVerbose, TEXT("ViewUniformBuffer is null"));
        return;
    }
    Params->FroxelSharedUB = SharedUB;
    Params->View = InView.ViewUniformBuffer;
    Params->Lights = GraphBuilder.CreateSRV(LightsSB);
    Params->FroxelLightCounts = FroxelLightCountsUAV;

    const TShaderMapRef<FFroxelCountCS> ComputeShader(GetGlobalShaderMap(InView.GetFeatureLevel()));
    const FIntVector Groups(FComputeShaderUtils::GetGroupCount(NumLights_RT, 64));

    FComputeShaderUtils::AddPass(
        GraphBuilder,
        RDG_EVENT_NAME("Froxel Ligthing Build Grid (NumLights = %u)", NumLights_RT),
        ERDGPassFlags::Compute,
        ComputeShader,
        Params,
        Groups);

}

/**
 * Compute the offsets into the global light index list for each froxel.
 * The result is stored in FroxelLightOffsetsUAV buffer.
 * This step is divided into 3 passes:
 * 1. Prefix sum (scan) each group of froxels in a z-slice.
 * 2. Prefix sum (scan) each z-slice group sums.
 * 3. Add the z-slice group sums to each froxel in the z-slice.
 **/
void FFroxelViewExtension::ComputeFroxelOffset(FRDGBuilder& GraphBuilder, FSceneView& InView) {

    if (!FroxelLists.Counts) {
        UE_LOG(LogFroxelLighting, VeryVerbose, TEXT("FroxelLightCounts is null"));
        return;
    }
    if (!FroxelLists.Offsets) {
        UE_LOG(LogFroxelLighting, VeryVerbose, TEXT("FroxelLightOffsets is null"));
        return;
    }
    const int32 GX = GridSize_RT.X;
    const int32 GY = GridSize_RT.Y;
    const int32 GZ = GridSize_RT.Z;
    const uint32 ZSliceSize = GX * GY;

    FRDGBufferRef ZSliceTotals = GraphBuilder.CreateBuffer(
        FRDGBufferDesc::CreateStructuredDesc(sizeof(uint32), GZ),
        TEXT("ZSliceTotals")
        );

    FRDGBufferRef ZSliceOffsets = GraphBuilder.CreateBuffer(
        FRDGBufferDesc::CreateStructuredDesc(sizeof(uint32), GZ),
        TEXT("ZSliceOffsets")
        );

    // Pass A: prefix sum (Blelloch scan) each group of froxels in a z-slice.
    {
        const FRDGBufferUAVRef ZSliceTotalsUAV = GraphBuilder.CreateUAV(ZSliceTotals);
        const FRDGBufferUAVRef FroxelLightOffsetsUAV = GraphBuilder.CreateUAV(FroxelLists.Offsets);
        AddClearUAVPass(GraphBuilder, ZSliceTotalsUAV, 0u);
        AddClearUAVPass(GraphBuilder, FroxelLightOffsetsUAV, 0u);

        auto* Params = GraphBuilder.AllocParameters<FFroxelOffsetPassACS::FParameters>();
        Params->FroxelSharedUB = SharedUB;
        Params->FroxelLightCounts = GraphBuilder.CreateSRV(FroxelLists.Counts);
        Params->FroxelLightOffsets = FroxelLightOffsetsUAV;
        Params->ZSliceTotals = ZSliceTotalsUAV;
        const TShaderMapRef<FFroxelOffsetPassACS> ComputeShader(GetGlobalShaderMap(InView.GetFeatureLevel()));
        const FIntVector Groups(FComputeShaderUtils::GetGroupCount(ZSliceSize, 256));

        FComputeShaderUtils::AddPass(
            GraphBuilder,
            RDG_EVENT_NAME("Froxel Ligthing: Compute Offset Pass A"),
            ERDGPassFlags::Compute,
            ComputeShader,
            Params,
            Groups
            );
    }

    // Pass B: prefix sum (Blelloch scan) each z-slice group sums.
    {
        const FRDGBufferUAVRef ZSliceOffsetsUAV = GraphBuilder.CreateUAV(ZSliceOffsets);
        AddClearUAVPass(GraphBuilder, ZSliceOffsetsUAV, 0u);

        auto* Params = GraphBuilder.AllocParameters<FFroxelOffsetPassBCS::FParameters>();
        Params->FroxelSharedUB = SharedUB;
        Params->ZSliceTotals = GraphBuilder.CreateSRV(ZSliceTotals);
        Params->ZSliceOffsets = ZSliceOffsetsUAV;

        const TShaderMapRef<FFroxelOffsetPassBCS> ComputeShader(GetGlobalShaderMap(InView.GetFeatureLevel()));
        const FIntVector Groups(FComputeShaderUtils::GetGroupCount(GZ, 1024));

        FComputeShaderUtils::AddPass(
            GraphBuilder,
            RDG_EVENT_NAME("Froxel Ligthing: Compute Offset Pass B"),
            ERDGPassFlags::Compute,
            ComputeShader,
            Params,
            Groups
            );
    }

    {
        // Pass C: Add the z-slice group sums to each froxel in the z-slice.
        auto* Params = GraphBuilder.AllocParameters<FFroxelOffsetPassCCS::FParameters>();
        Params->FroxelSharedUB = SharedUB;
        Params->ZSliceOffsets = GraphBuilder.CreateSRV(ZSliceOffsets);
        Params->FroxelLightOffsets = GraphBuilder.CreateUAV(FroxelLists.Offsets);
        TShaderMapRef<FFroxelOffsetPassCCS> ComputeShader(GetGlobalShaderMap(InView.GetFeatureLevel()));
        const FIntVector Groups(FComputeShaderUtils::GetGroupCount(ZSliceSize, 1024));

        FComputeShaderUtils::AddPass(
            GraphBuilder,
            RDG_EVENT_NAME("Froxel Ligthing: Compute Offset Pass C"),
            ERDGPassFlags::Compute,
            ComputeShader,
            Params,
            Groups
            );
    }
}

/**
 * Compute the list of light indices for each froxel.
 * The result is stored in FroxelLists.LightIndices buffer.
 **/
void FFroxelViewExtension::ComputeLightIndices(FRDGBuilder& GraphBuilder, FSceneView& InView) {
    if (!LightsSB) {
        UE_LOG(LogFroxelLighting, VeryVerbose, TEXT("Lights is null"));
        return;
    }
    if (!FroxelLists.Counts) {
        UE_LOG(LogFroxelLighting, VeryVerbose, TEXT("FroxelLightCounts is null"));
        return;
    }
    if (!FroxelLists.Offsets) {
        UE_LOG(LogFroxelLighting, VeryVerbose, TEXT("FroxelLightOffsets is null"));
        return;
    }
    if (!FroxelLists.LightIndices) {
        UE_LOG(LogFroxelLighting, VeryVerbose, TEXT("FroxelLightIndices is null"));
        return;
    }
    if (!FroxelLists.TotalIndices) {
        UE_LOG(LogFroxelLighting, VeryVerbose, TEXT("TotalIndices is null"));
        return;
    }
    
    const int32 GX = GridSize_RT.X;
    const int32 GY = GridSize_RT.Y;
    const int32 GZ = GridSize_RT.Z;
    const uint32 GCOUNT = GridCount_RT;
    
    // Compute total number of indices
    {
        FRDGBufferUAVRef TotalIndicesUAV = GraphBuilder.CreateUAV(FroxelLists.TotalIndices);
        AddClearUAVPass(GraphBuilder, TotalIndicesUAV, 0u);
    
        auto* Params = GraphBuilder.AllocParameters<FFroxelTotalIndicesCS::FParameters>();
        Params->FroxelSharedUB = SharedUB;
        Params->FroxelLightCounts = GraphBuilder.CreateSRV(FroxelLists.Counts);
        Params->FroxelLightOffsets = GraphBuilder.CreateSRV(FroxelLists.Offsets);
        Params->TotalIndices = TotalIndicesUAV;
    
        const TShaderMapRef<FFroxelTotalIndicesCS> ComputeShader(GetGlobalShaderMap(InView.GetFeatureLevel()));
        const FIntVector Groups = FIntVector(1, 1, 1);
    
        FComputeShaderUtils::AddPass(
            GraphBuilder,
            RDG_EVENT_NAME("Froxel Ligthing: Compute Total Indices"),
            ERDGPassFlags::Compute,
            ComputeShader,
            Params,
            Groups
            );
    }
    
    // compute light indices
    {
    
        FRDGBufferUAVRef FroxelLightIndicesUAV = GraphBuilder.CreateUAV(FroxelLists.LightIndices);
        FRDGBufferUAVRef FroxelHeadsUAV = GraphBuilder.CreateUAV(FroxelLists.Counts); // reusing counts buffer as heads
        AddClearUAVPass(GraphBuilder, FroxelLightIndicesUAV, 0u);
        AddClearUAVPass(GraphBuilder, FroxelHeadsUAV, 0u);
    
        auto* Params = GraphBuilder.AllocParameters<FFroxelAssignLightsCS::FParameters>();
    
        Params->FroxelSharedUB = SharedUB;
        Params->View = InView.ViewUniformBuffer;
        Params->Lights = GraphBuilder.CreateSRV(LightsSB);
        Params->FroxelLightOffsets = GraphBuilder.CreateSRV(FroxelLists.Offsets);
        Params->TotalIndices = GraphBuilder.CreateSRV(FroxelLists.TotalIndices);
        Params->FroxelHeads = FroxelHeadsUAV;
        Params->FroxelLightIndices  = FroxelLightIndicesUAV;
    
        const TShaderMapRef<FFroxelAssignLightsCS> ComputeShader(GetGlobalShaderMap(InView.GetFeatureLevel()));
        const FIntVector Groups(FComputeShaderUtils::GetGroupCount(NumLights_RT, 64));
    
        FComputeShaderUtils::AddPass(
            GraphBuilder,
            RDG_EVENT_NAME("Froxel Ligthing: Assign Lights (NumLights = %u)", NumLights_RT),
            ERDGPassFlags::Compute,
            ComputeShader,
            Params,
            Groups);
    }
}

/**
 * Build a texture overlay visualizing the froxel grid.
 * Each froxel is shown as a colored box.
 * The overlay texture is returned as an RDG texture.
 **/
FScreenPassTexture FFroxelViewExtension::VisualizeFroxelOverlayPS(
    FRDGBuilder& GraphBuilder,
    const FSceneView& View,
    const FPostProcessMaterialInputs& Inputs) {

    FScreenPassTexture InputSceneColor(Inputs.GetInput(EPostProcessMaterialInput::SceneColor));
    //
    // // FRDGTextureRef OverlayTex = BuildFroxelOverlay(GraphBuilder, View);
    // // if (!OverlayTex)
    // //     return InputSceneColor;
    //
    // const FIntRect ViewRect = View.UnconstrainedViewRect;
    // const FScreenPassTextureViewport InputViewport(InputSceneColor);
    // FRDGTextureRef DepthTextureRef = Inputs.SceneTextures.SceneTextures->GetParameters()->SceneDepthTexture;
    //
    // auto* Params = GraphBuilder.AllocParameters<FFroxelOverlayPS::FParameters>();
    // // Params->FroxelUB = SharedUB;
    // // Params->Input = GetScreenPassTextureViewportParameters(InputViewport);
    // // Params->View = View.ViewUniformBuffer;
    // // Params->SceneTextures = Inputs.SceneTextures.SceneTextures;
    // Params->SceneColorCopy = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(InputSceneColor.Texture));
    // Params->SceneColorCopySampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
    // Params->SceneDepthCopy = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(DepthTextureRef));
    // Params->SceneDepthCopySampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
    // Params->SvPositionToInputTextureUV =
    //     FScreenTransform::ChangeTextureBasisFromTo(InputViewport, FScreenTransform::ETextureBasis::TexelPosition,
    //                                                FScreenTransform::ETextureBasis::ViewportUV) *
    //     FScreenTransform::ChangeTextureBasisFromTo(InputViewport, FScreenTransform::ETextureBasis::ViewportUV,
    //                                                FScreenTransform::ETextureBasis::TextureUV);
    // Params->RenderTargets[0] = FRenderTargetBinding(InputSceneColor.Texture, ERenderTargetLoadAction::ELoad);
    //
    // // Params->SvPositionToDepthUV = FScreenTransform::ChangeTextureBasisFromTo(InputViewport, FScreenTransform::ETextureBasis::TexelPosition,
    // //                                                FScreenTransform::ETextureBasis::ViewportUV) *
    // //     FScreenTransform::ChangeTextureBasisFromTo(InputViewport, FScreenTransform::ETextureBasis::ViewportUV,
    // //                                                FScreenTransform::ETextureBasis::TextureUV);
    //
    // FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
    // TShaderMapRef<FFroxelOverlayPS> PixelShader(GlobalShaderMap);
    //
    // FPixelShaderUtils::AddFullscreenPass(
    //     GraphBuilder,
    //     GlobalShaderMap,
    //     RDG_EVENT_NAME("FroxelOverlay (FPixelShaderUtils)"),
    //     PixelShader,
    //     Params,
    //     ViewRect);
    //
    return InputSceneColor;
}


// temp for testing
void FFroxelViewExtension::BuildFroxelGrid(FRDGBuilder& GraphBuilder, const FSceneView& InView) {

    const ERDGPassFlags RDGPassFlags = ERDGPassFlags::Compute | ERDGPassFlags::NeverCull;
    UE_LOG(LogFroxelLighting, VeryVerbose, TEXT("BuildFroxelGrid called"));

    FRDGBufferRef FroxelBuffer = GraphBuilder.CreateBuffer(
        FRDGBufferDesc::CreateStructuredDesc(sizeof(uint32), GridCount_RT),
        TEXT("FroxelGrid")
        );

    FRDGBufferUAVRef FroxelUAV = GraphBuilder.CreateUAV(FroxelBuffer);
    FRDGBufferSRVRef FroxelSRV = GraphBuilder.CreateSRV(FroxelBuffer);

    AddClearUAVPass(GraphBuilder, FroxelUAV, 0u, RDGPassFlags);

    TShaderMapRef<FFroxelBuildGridCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

    auto* Params = GraphBuilder.AllocParameters<FFroxelBuildGridCS::FParameters>();

    // Params->FroxelUB = SharedUB;
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
        GridSize_RT,
        FIntVector(8, 8, 8)));

    // UE_LOG(LogFroxelLighting, VeryVerbose, TEXT("Groups = (%d,%d,%d), Grid = (%d,%d,%d)"),
    //        Groups.X, Groups.Y, Groups.Z, GX, GY, GZ);

    RDG_GPU_STAT_SCOPE(GraphBuilder, Froxel);
    RDG_EVENT_SCOPE(GraphBuilder, "FroxelScope");

    FComputeShaderUtils::AddPass(GraphBuilder,
                                 RDG_EVENT_NAME("Froxel: BuildGrid [%u x %u x %u]", GridSize_RT.X, GridSize_RT.Y,
                                                GridSize_RT.Z),
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

// Temp for testing
void FFroxelViewExtension::VisualizeFroxelGrid(FRDGBuilder& GraphBuilder, const FSceneView& InView) {

    // extern TAutoConsoleVariable<int32> CVarFroxelVisualize;
    // UE_LOG(LogFroxelLighting, VeryVerbose, TEXT("[Froxel] VisualizeFroxelGrid called"));
    // if (CVarFroxelVisualize.GetValueOnRenderThread() == 0) {
    //     return;
    // }
    //
    // if (!InView.Family)
    //     return;
    //
    // if (InView.bIsSceneCapture ||
    //     InView.bIsReflectionCapture ||
    //     InView.bIsPlanarReflection ||
    //     InView.bIsVirtualTexture) {
    //     return;
    // }
    //
    // if (!InView.IsPrimarySceneView())
    //     return;
    //
    // // FRDGTextureRef OverlayTex = BuildFroxelOverlay(GraphBuilder, InView);
    // // if (!OverlayTex)
    // //     return;
    //
    // // Get back buffer as RDG texture
    // if (!InView.bIsViewInfo || !InView.Family || !InView.Family->RenderTarget)
    //     return;
    //
    // FRHITexture* BackBufferRHI = InView.Family->RenderTarget->GetRenderTargetTexture();
    // if (!BackBufferRHI)
    //     return;
    //
    // FRDGTextureRef BackBufferRDG = GraphBuilder.FindExternalTexture(BackBufferRHI);
    // if (!BackBufferRDG) {
    //     BackBufferRDG = GraphBuilder.RegisterExternalTexture(CreateRenderTarget(BackBufferRHI, TEXT("BackBuffer")));
    // }
    //
    // //
    // // const FRDGTextureDesc CopyDesc = BackBufferRDG->Desc;
    // // FRDGTextureRef SceneColorCopyTex = GraphBuilder.CreateTexture(CopyDesc, TEXT("SceneColorCopy"));
    // // AddCopyTexturePass(GraphBuilder, BackBufferRDG, SceneColorCopyTex);
    //
    // const FIntRect ViewRect = InView.UnscaledViewRect;
    // const FIntPoint ViewSize = ViewRect.Size();
    //
    // // FScreenPassTexture InputSPT(SceneColorCopyTex, ViewRect);
    // // const FScreenPassTextureViewport InputViewport(InputSPT);
    //
    // auto* Params = GraphBuilder.AllocParameters<FFroxelOverlayPS::FParameters>();
    //
    // FRDGTexture* SceneColorRDG = GraphBuilder.
    //     FindExternalTexture(InView.Family->RenderTarget->GetRenderTargetTexture());
    // if (!SceneColorRDG)
    //     return;
    //
    // // Params->FroxelUB = SharedUB;
    //
    // // Params->SceneColorCopy = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(SceneColorCopyTex));
    // // Params->SceneColorCopySampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
    //
    // const FScreenPassTextureViewport InputViewport(SceneColorRDG, ViewRect);
    // // Params->Input = GetScreenPassTextureViewportParameters(InputViewport);
    // // Params->View = InView.ViewUniformBuffer;
    //
    // const FSceneTextureShaderParameters SceneTexParams = GetSceneTextureShaderParameters(InView);
    // if (!SceneTexParams.SceneTextures)
    //     return;
    // // Params->SceneTextures = SceneTexParams.SceneTextures;
    //
    // Params->SvPositionToInputTextureUV =
    //     // SV_Position -> ViewportUV -> TextureUV
    //     FScreenTransform::ChangeTextureBasisFromTo(InputViewport, FScreenTransform::ETextureBasis::TexelPosition,
    //                                                FScreenTransform::ETextureBasis::ViewportUV) *
    //     FScreenTransform::ChangeTextureBasisFromTo(InputViewport, FScreenTransform::ETextureBasis::ViewportUV,
    //                                                FScreenTransform::ETextureBasis::TextureUV);
    // Params->RenderTargets[0] = FRenderTargetBinding(BackBufferRDG, ERenderTargetLoadAction::ELoad);
    //
    // FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
    //
    // TShaderMapRef<FFroxelOverlayPS> PixelShader(GlobalShaderMap);
    // // GetScreenPassTextureViewportParameters(
    // FPixelShaderUtils::AddFullscreenPass(
    //     GraphBuilder,
    //     GlobalShaderMap,
    //     RDG_EVENT_NAME("FroxelOverlay (FPixelShaderUtils)"),
    //     PixelShader,
    //     Params,
    //     ViewRect);
}