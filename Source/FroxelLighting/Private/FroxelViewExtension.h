#pragma once
#include "RHIGPUReadback.h"
#include "SceneViewExtension.h"


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

    virtual void PreRenderViewFamily_RenderThread(FRDGBuilder& GraphBuilder, FSceneViewFamily& InViewFamily) override {
    }

    virtual void PreRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView) override;


    virtual void PreInitViews_RenderThread(FRDGBuilder& GraphBuilder) override {
    }

    virtual void PreRenderBasePass_RenderThread(FRDGBuilder& GraphBuilder, bool bDepthBufferIsPopulated) override {
    }

    virtual void PostRenderBasePassDeferred_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView,
                                                         const FRenderTargetBindingSlots& RenderTargets,
                                                         TRDGUniformBufferRef<FSceneTextureUniformParameters>
                                                         SceneTextures) override {
    }


    virtual void PostRenderBasePassMobile_RenderThread(FRHICommandList& RHICmdList, FSceneView& InView) override {
    }

    virtual void PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& InView,
                                                 const FPostProcessingInputs& Inputs) override {
    };

    virtual void PrePostProcessPassMobile_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& InView,
                                                       const FMobilePostProcessingInputs& Inputs) override {
    };

    virtual void SubscribeToPostProcessingPass(EPostProcessingPass Pass, const FSceneView& InView,
                                               FPostProcessingPassDelegateArray& InOutPassCallbacks,
                                               bool bIsPassEnabled) override {
    }

    virtual void PostRenderViewFamily_RenderThread(FRDGBuilder& GraphBuilder, FSceneViewFamily& InViewFamily) override {
    }

    virtual void PostRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView) override {
    }

    virtual int32 GetPriority() const override {
        return 0;
    }

    virtual bool IsActiveThisFrame_Internal(const FSceneViewExtensionContext& Context) const override {
        return true;
    }

private:
    void BuildFroxelGrid(FRDGBuilder& GraphBuilder, FSceneView& InView);

    TArray<FFroxelReadbackEntry> PendingReadbacks;
};