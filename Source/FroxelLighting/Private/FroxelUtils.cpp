#include "FroxelUtils.h"
#include "RenderGraphBuilder.h"
#include "Shaders/FroxelOverlay.h"


extern TAutoConsoleVariable<int32> CVarFroxelFarClipCm;


struct FFroxelLists {
    FRDGBufferRef Offsets; // uint
    FRDGBufferRef Counts; // uint
    FRDGBufferRef LightIndices; // uint
    uint32 FroxelCount;
};


uint32 FFroxelUtils::GetFroxelIndex(const uint32 X, const uint32 Y, const uint32 Z, const uint32 GridX,
                                    const uint32 GridY, const uint32 GridZ) {
    check(X < GridX && Y < GridY && Z < GridZ);
    return X + Y * GridX + Z * GridX * GridY;
}

TUniformBufferRef<FFroxelUniformParameters> FFroxelUtils::CreateFroxelUB(
    FRDGBuilder& GraphBuilder, FIntVector GridSize, const FSceneView& InView) {
    const int32 GX = GridSize.X;
    const int32 GY = GridSize.Y;
    const int32 GZ = GridSize.Z;
    const int32 GCOUNT = GX * GY * GZ;

    FFroxelUniformParameters* FroxelParams = GraphBuilder.AllocParameters<FFroxelUniformParameters>();
    FroxelParams->GridSize = FIntVector4(GX, GY, GZ, GCOUNT);
    FroxelParams->NearCm = InView.NearClippingDistance;
    FroxelParams->FarCm = FMath::Max(1000, CVarFroxelFarClipCm.GetValueOnRenderThread());

    // Create an immediate UB for this pass/frame
    TUniformBufferRef<FFroxelUniformParameters> FroxelUB =
        TUniformBufferRef<FFroxelUniformParameters>::CreateUniformBufferImmediate(
            *FroxelParams, UniformBuffer_SingleDraw);

    return FroxelUB;
}


FFroxelLists FFroxelUtils::CreateFroxelLists(FRDGBuilder& GraphBuilder, FIntVector Grid, int32 MaxLightsPerFroxel) {
    FFroxelLists Lists;

    uint32 FroxelCount = Grid.X * Grid.Y * Grid.Z;
    Lists.FroxelCount = FroxelCount;

    Lists.Offsets = GraphBuilder.CreateBuffer(
        FRDGBufferDesc::CreateStructuredDesc(sizeof(uint32), FroxelCount),
        TEXT("FroxelLists.Offsets"));

    Lists.Counts = GraphBuilder.CreateBuffer(
        FRDGBufferDesc::CreateStructuredDesc(sizeof(uint32), FroxelCount),
        TEXT("FroxelLists.Counts"));

    Lists.LightIndices = GraphBuilder.CreateBuffer(
        FRDGBufferDesc::CreateStructuredDesc(sizeof(uint32), FroxelCount * MaxLightsPerFroxel),
        TEXT("FroxelLists.LightIndices"));

    return Lists;
}