#pragma once
#include "CoreMinimal.h"

class FFroxelUniformParameters;
struct FFroxelLists;

class FFroxelUtils {
public:
    FORCEINLINE uint32 GetFroxelIndex(const uint32 X, const uint32 Y, const uint32 Z, const uint32 GridX,
                                      const uint32 GridY, const uint32 GridZ);

    static TUniformBufferRef<FFroxelUniformParameters> CreateFroxelUB(FRDGBuilder& GraphBuilder, FIntVector GridSize,
                                                                      const FSceneView& InView);
    static FFroxelLists CreateFroxelLists(FRDGBuilder& GraphBuilder, FIntVector Grid, int32 MaxLightsPerFroxel);

};