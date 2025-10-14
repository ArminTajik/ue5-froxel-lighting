#pragma once
BEGIN_UNIFORM_BUFFER_STRUCT(FFroxelUniformParameters,)
    SHADER_PARAMETER(FIntVector4, GridSize) // (NumX, NumY, NumZ, NumFroxels)

    SHADER_PARAMETER(FIntPoint, ViewportSize)
    SHADER_PARAMETER(FIntPoint, TileSizePx)
    SHADER_PARAMETER(FVector2f, InvTilePx)

    SHADER_PARAMETER(float, NearCm) // camera near in cm
    SHADER_PARAMETER(float, FarCm) // camera far in cm

    SHADER_PARAMETER(uint32, NumLights) // number of lights in the view
    SHADER_PARAMETER(uint32, MaxLightsPerFroxel)
END_UNIFORM_BUFFER_STRUCT();