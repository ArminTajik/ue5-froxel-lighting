#pragma once
#include "FroxelUtils.h"
#include "SceneViewExtension.h"
#include "RHI.h"
#include "RHIResources.h"
#include "Data/FroxelData.h"


class FFroxelUniformParameters;

class FFroxelViewExtension final : public FSceneViewExtensionBase {
public:
    FFroxelViewExtension(const FAutoRegister& AutoReg) :
        FSceneViewExtensionBase(AutoReg) {
    }

    virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override;

    virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override {
    }

    virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override {
    }

    virtual void PreRenderViewFamily_RenderThread(FRDGBuilder& GraphBuilder, FSceneViewFamily& InViewFamily) override;


    virtual void PreRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView) override {
    }

    virtual void PostRenderBasePassDeferred_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView,
                                                         const FRenderTargetBindingSlots& RenderTargets,
                                                         TRDGUniformBufferRef<FSceneTextureUniformParameters>
                                                         SceneTextures) override;

    virtual void PostRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView) override {
    }


    virtual void PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& InView,
                                                 const FPostProcessingInputs& Inputs) override {
    }

    virtual void SubscribeToPostProcessingPass(EPostProcessingPass Pass, const FSceneView& InView,
                                               FPostProcessingPassDelegateArray& InOutPassCallbacks,
                                               bool bIsPassEnabled) override;

private:
    static void GetFrameLights(FSceneViewFamily& InViewFamily, TArray<FFroxelLightData>& OutLights);
    void CountLightPerFroxel(FRDGBuilder& GraphBuilder, const FSceneView& InView);
    void ComputeFroxelOffset(FRDGBuilder& GraphBuilder, const FSceneView& InView);
    void ComputeLightIndices(FRDGBuilder& GraphBuilder, const FSceneView& InView);

    void BuildFroxelGrid(FRDGBuilder& GraphBuilder, const FSceneView& InView);
    FScreenPassTexture VisualizeFroxelOverlayPS(FRDGBuilder& GraphBuilder,
                                                const FSceneView& View, const FPostProcessMaterialInputs& Inputs);

    void VisualizeFroxelGrid(FRDGBuilder& GraphBuilder, const FSceneView& InView);


    // Build light grid data ---------------------------------
    FCriticalSection LightBufferMutex; // to protect access to light buffers by game thread and render thread

    TArray<FFroxelLightData> FrameLights_GT; // accessed by game thread
    uint32 NumLights_GT = 0;

    FIntVector GridSize_GT = FIntVector(16, 16, 16);
    uint32 GridCount_GT = 4096;

    TArray<FFroxelLightData> FrameLights_RT; // accessed by render thread
    uint32 NumLights_RT = 0;

    FIntVector GridSize_RT = FIntVector(16, 16, 16);
    uint32 GridCount_RT = 4096;

    FRDGBufferRef LightsSB = nullptr; // structured buffer for light data

    FFroxelLists FroxelLists; // froxel light lists (Counts, Offsets, LightIndices, TotalIndices)
    TUniformBufferRef<FFroxelUniformParameters> SharedUB; // shared froxel parameters


    // Testing -----------------------------------------
    // TArray<FFroxelReadbackEntry> PendingReadbacks;

};