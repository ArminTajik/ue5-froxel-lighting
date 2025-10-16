#pragma once


class FRHIGPUBufferReadback;

struct FFroxelReadbackEntry {
    TUniquePtr<FRHIGPUBufferReadback> Readback;
    uint32 NumBytes = 0;
    uint32 GCount = 0;
    double Timestamp;
};

struct FFroxelLightData {
    FVector3f PositionWS;
    float Radius;
    // FLinearColor Color;
    // float Intensity;
};

struct FFroxelLists {

    FRDGBufferRef Offsets; // uint, size = FroxelCount
    FRDGBufferRef Counts; // uint, size = FroxelCount
    FRDGBufferRef TotalIndices; // uint (single uint)
    FRDGBufferRef LightIndices; // uint SoA, size = FroxelCount
    uint32 FroxelCount;
};
