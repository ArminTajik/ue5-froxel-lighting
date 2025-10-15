#include "FroxelUtils.h"
#include "RenderGraphBuilder.h"
#include "Shaders/FroxelCommon.h"


extern TAutoConsoleVariable<int32> CVarFroxelFarClipCm;
extern TAutoConsoleVariable<int32> CVarFroxelMaxLightsPerFroxel;


uint32 FFroxelUtils::GetFroxelIndex(const uint32 X, const uint32 Y, const uint32 Z, const uint32 GridX,
                                    const uint32 GridY, const uint32 GridZ) {
    check(X < GridX && Y < GridY && Z < GridZ);
    return X + Y * GridX + Z * GridX * GridY;
}

TUniformBufferRef<FFroxelUniformParameters> FFroxelUtils::CreateSharedUB(
    FRDGBuilder& GraphBuilder, const FSceneView& InView, const FIntVector GridSize, int32 NumLights) {

    const int32 GX = GridSize.X;
    const int32 GY = GridSize.Y;
    const int32 GZ = GridSize.Z;
    const int32 GCOUNT = GX * GY * GZ;

    FFroxelUniformParameters* Params = GraphBuilder.AllocParameters<FFroxelUniformParameters>();
    Params->GridSize = FIntVector4(GX, GY, GZ, GCOUNT);
    Params->NearCm = InView.NearClippingDistance;
    Params->FarCm = FMath::Max(1000, CVarFroxelFarClipCm.GetValueOnRenderThread());

    Params->NumLights = NumLights;
    Params->MaxLightsPerFroxel = FMath::Max(1, CVarFroxelMaxLightsPerFroxel.GetValueOnRenderThread());
    Params->ViewportSize = InView.UnscaledViewRect.Size();
    Params->ViewRectMinPx = InView.UnscaledViewRect.Min;

    FVector2f TileSizeF = FVector2f(Params->ViewportSize) / FVector2f(GX, GY);
    TileSizeF = FVector2f(
        FMath::Max(TileSizeF.X, 1.0f),
        FMath::Max(TileSizeF.Y, 1.0f)
        );
    FIntPoint TileSize = FIntPoint(
        FMath::FloorToInt(TileSizeF.X),
        FMath::FloorToInt(TileSizeF.Y)
        );
    Params->TileSizePx = TileSize;
    Params->InvTilePx = FVector2f(1.0f) / FVector2f(TileSize);
    Params->ViewMatrix = FMatrix44f(InView.ViewMatrices.GetViewMatrix());
    Params->ProjectionMatrix = FMatrix44f(InView.ViewMatrices.GetProjectionMatrix());
    

    // Create an immediate UB for this **frame**
    TUniformBufferRef<FFroxelUniformParameters> FroxelUB =
        TUniformBufferRef<FFroxelUniformParameters>::CreateUniformBufferImmediate(
            *Params, UniformBuffer_SingleFrame);

    return FroxelUB;
}


FFroxelLists FFroxelUtils::CreateFroxelLists(FRDGBuilder& GraphBuilder, FIntVector Grid, int32 MaxLightsPerFroxel) {
    FFroxelLists Lists;

    const uint32 FroxelCount = Grid.X * Grid.Y * Grid.Z;
    Lists.FroxelCount = FroxelCount;

    FRDGBufferDesc CountsDesc = FRDGBufferDesc::CreateStructuredDesc(sizeof(uint32), FroxelCount);
    CountsDesc.Usage |= BUF_ShaderResource | BUF_UnorderedAccess;
    Lists.Counts = GraphBuilder.CreateBuffer(CountsDesc, TEXT("FroxelLists.Counts"));

    FRDGBufferDesc OffsetsDesc = FRDGBufferDesc::CreateStructuredDesc(sizeof(uint32), FroxelCount);
    OffsetsDesc.Usage |= BUF_ShaderResource | BUF_UnorderedAccess;
    Lists.Offsets = GraphBuilder.CreateBuffer(OffsetsDesc, TEXT("FroxelLists.Offsets"));

    FRDGBufferDesc TotalIndicesDesc = FRDGBufferDesc::CreateStructuredDesc(sizeof(uint32), 1);
    TotalIndicesDesc.Usage |= BUF_ShaderResource | BUF_UnorderedAccess;
    Lists.TotalIndices = GraphBuilder.CreateBuffer(TotalIndicesDesc, TEXT("FroxelLists.TotalIndices"));

    FRDGBufferDesc LightIndicesDesc = FRDGBufferDesc::CreateStructuredDesc(
        sizeof(uint32), FroxelCount * MaxLightsPerFroxel);
    LightIndicesDesc.Usage |= BUF_ShaderResource | BUF_UnorderedAccess;
    Lists.LightIndices = GraphBuilder.CreateBuffer(LightIndicesDesc, TEXT("FroxelLists.LightIndices"));

    return Lists;
}