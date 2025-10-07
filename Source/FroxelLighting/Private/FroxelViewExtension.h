#pragma once
#include "RHIGPUReadback.h"
#include "SceneViewExtension.h"
#include "RHI.h"
#include "RHIResources.h"


class FFroxelUniformParameters;
class FFroxelSharedParameters;

struct FFroxelReadbackEntry {
    TUniquePtr<FRHIGPUBufferReadback> Readback;
    uint32 NumBytes = 0;
    uint32 GCount = 0;
    double Timestamp;
};

class FFroxelViewExtension final : public FSceneViewExtensionBase {
public:
    FFroxelViewExtension(const FAutoRegister& AutoReg) :
        FSceneViewExtensionBase(AutoReg) {
    }

    virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override {
    }

    virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override {
    }

    virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override {
    }

    virtual void PreRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView) override;

    virtual void PostRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView) override;

    virtual void PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& InView,
                                                 const FPostProcessingInputs& Inputs) override {
    }

    virtual void SubscribeToPostProcessingPass(EPostProcessingPass Pass, const FSceneView& InView,
                                               FPostProcessingPassDelegateArray& InOutPassCallbacks,
                                               bool bIsPassEnabled) override;

private:
    void BuildFroxelGrid(FRDGBuilder& GraphBuilder, const FSceneView& InView);
    FScreenPassTexture AddFroxelOverlayPass(FRDGBuilder& GraphBuilder,
                                            const FSceneView& View, const FPostProcessMaterialInputs& Inputs);

    FRDGTextureRef BuildFroxelOverlay(FRDGBuilder& GraphBuilder, const FSceneView& InView);
    void VisualizeFroxelGrid(FRDGBuilder& GraphBuilder, const FSceneView& InView);

    TArray<FFroxelReadbackEntry> PendingReadbacks;

    TUniformBufferRef<FFroxelUniformParameters> FroxelUB;
    
    //
    // FFroxelSharedParameters* SharedParams = nullptr;
    // TRDGUniformBufferRef<FFroxelSharedParameters> SharedUB = nullptr;
};