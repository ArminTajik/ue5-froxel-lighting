#pragma once
#include "CoreMinimal.h"
#include "Data/FroxelData.h"

class FFroxelUniformParameters;

class FFroxelUtils {
public:
    FORCEINLINE uint32 GetFroxelIndex(const uint32 X, const uint32 Y, const uint32 Z, const uint32 GridX,
                                      const uint32 GridY, const uint32 GridZ);

    static TUniformBufferRef<FFroxelUniformParameters> CreateSharedUB(FRDGBuilder& GraphBuilder,
                                                                      const FSceneView& InView,
                                                                      FIntVector GridSize, int32 NumLights);
    static FFroxelLists CreateFroxelLists(FRDGBuilder& GraphBuilder, FIntVector Grid, int32 MaxLightsPerFroxel);

};