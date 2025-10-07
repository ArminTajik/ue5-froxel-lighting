#pragma once
BEGIN_UNIFORM_BUFFER_STRUCT(FFroxelUniformParameters,)
    SHADER_PARAMETER(FIntVector4, GridSize) // (NumX, NumY, NumZ, NumFroxels)
    // SHADER_PARAMETER(FVector2f, DepthParams) // (1/ZNear, 1/ZFar - 1/ZNear)
    // SHADER_PARAMETER(FMatrix44f, InvProj) // inverse projection matrix
    // SHADER_PARAMETER(FIntPoint, TileSize) // (TileWidthPx, TileHeightPx) for screen-space binning
    // SHADER_PARAMETER(float, MaxViewDepth) // linear view depth range to quantize
    SHADER_PARAMETER(float, NearCm) // camera near in cm
    SHADER_PARAMETER(float, FarCm) // camera far in cm
END_UNIFORM_BUFFER_STRUCT();