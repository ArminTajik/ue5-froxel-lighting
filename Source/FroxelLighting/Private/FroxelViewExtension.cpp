#include "FroxelViewExtension.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "GlobalShader.h"
#include "HAL/IConsoleManager.h"
#include "FroxelLighting.h"
#include "FroxelBuildCS.h"
#include "RHIGPUReadback.h"
#include "RHICommandList.h"

DEFINE_GPU_STAT(Froxel);

void FFroxelViewExtension::BuildFroxelGrid(FRDGBuilder& GraphBuilder, FSceneView& InView) {

    const ERDGPassFlags RDGPassFlags = ERDGPassFlags::Compute | ERDGPassFlags::NeverCull;
    UE_LOG(LogFroxelLighting, VeryVerbose, TEXT("[Froxel] BuildFroxelGrid called"));
    extern TAutoConsoleVariable<int32> CVarFroxelGridX;
    extern TAutoConsoleVariable<int32> CVarFroxelGridY;
    extern TAutoConsoleVariable<int32> CVarFroxelGridZ;

    const uint32 GX = static_cast<uint32>(FMath::Max(1, CVarFroxelGridX.GetValueOnRenderThread()));
    const uint32 GY = static_cast<uint32>(FMath::Max(1, CVarFroxelGridY.GetValueOnRenderThread()));
    const uint32 GZ = static_cast<uint32>(FMath::Max(1, CVarFroxelGridZ.GetValueOnRenderThread()));
    const uint32 GCOUNT = GX * GY * GZ;
    const uint32 NumBytes = GCOUNT * sizeof(uint32);

    FRDGBufferRef FroxelBuffer = GraphBuilder.CreateBuffer(
        FRDGBufferDesc::CreateStructuredDesc(sizeof(uint32), GCOUNT),
        TEXT("FroxelGrid")
        );

    FRDGBufferUAVRef FroxelUAV = GraphBuilder.CreateUAV(FroxelBuffer);
    FRDGBufferSRVRef FroxelSRV = GraphBuilder.CreateSRV(FroxelBuffer);

    AddClearUAVPass(GraphBuilder, FroxelUAV, 0u, RDGPassFlags);

    TShaderMapRef<FFroxelBuildCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

    auto* Params = GraphBuilder.AllocParameters<FFroxelBuildCS::FParameters>();


    
    Params->GridX = GX;
    Params->GridY = GY;
    Params->GridZ = GZ;
    Params->GridCount = GCOUNT;

    Params->FroxelGrid = FroxelUAV;

    const FIntVector Groups(FComputeShaderUtils::GetGroupCount(
        FIntVector(static_cast<int32>(GX), static_cast<int32>(GY), static_cast<int32>(GZ)),
        FIntVector(8, 8, 8)));

    UE_LOG(LogFroxelLighting, Log, TEXT("Groups = (%d,%d,%d), Grid = (%d,%d,%d)"),
           Groups.X, Groups.Y, Groups.Z, GX, GY, GZ);

    RDG_GPU_STAT_SCOPE(GraphBuilder, Froxel);
    RDG_EVENT_SCOPE(GraphBuilder, "FroxelScope");

    GraphBuilder.AddPass(
        RDG_EVENT_NAME("Froxel: BeforeBuildGrid"),
        ERDGPassFlags::None,
        [](FRHICommandListImmediate& RHICmdList) {
            SCOPED_DRAW_EVENT(RHICmdList, FroxelAfterBuildGrid);
        });

    
    FComputeShaderUtils::AddPass(GraphBuilder,
                                 RDG_EVENT_NAME("Froxel: BuildGrid [%u x %u x %u]", GX, GY, GZ),
                                 RDGPassFlags,
                                 ComputeShader,
                                 Params,
                                 Groups);

    GraphBuilder.AddPass(
        RDG_EVENT_NAME("Froxel: AfterBuildGrid"),
        ERDGPassFlags::None,
        [](FRHICommandListImmediate& RHICmdList) {
            SCOPED_DRAW_EVENT(RHICmdList, FroxelAfterBuildGrid);
        });

    FFroxelReadbackEntry ReadbackEntry;
    ReadbackEntry.Readback = MakeUnique<FRHIGPUBufferReadback>(TEXT("FroxelReadback"));
    ReadbackEntry.NumBytes = NumBytes;
    ReadbackEntry.GCount = GCOUNT;
    ReadbackEntry.Timestamp = FPlatformTime::Seconds();

    AddEnqueueCopyPass(GraphBuilder, ReadbackEntry.Readback.Get(), FroxelBuffer, NumBytes);

    this->PendingReadbacks.Add(MoveTemp(ReadbackEntry));

    GraphBuilder.AddPass(
        RDG_EVENT_NAME("Froxel: ReadbackResolve"),
        ERDGPassFlags::None,
        [this](FRHICommandListImmediate& RHICmdList) {
            for (int32 i = PendingReadbacks.Num() - 1; i >= 0; --i) {
                if (const FFroxelReadbackEntry& Entry = PendingReadbacks[i]; Entry.Readback->IsReady()) {
                    const uint32* Data = static_cast<const uint32*>(Entry.Readback->Lock(Entry.NumBytes));
                    UE_LOG(LogFroxelLighting, Log, TEXT("Froxel[0]=%u mid=%u last=%u"),
                           Data[0], Data[Entry.GCount / 2], Data[Entry.GCount - 1]);
                    Entry.Readback->Unlock();

                    PendingReadbacks.RemoveAtSwap(i);
                }
            }
        });

    UE_LOG(LogFroxelLighting, VeryVerbose, TEXT("[Froxel] BuildFroxelGrid finished"));
}


void FFroxelViewExtension::PreRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView) {
    FSceneViewExtensionBase::PreRenderView_RenderThread(GraphBuilder, InView);

    BuildFroxelGrid(GraphBuilder, InView);
}