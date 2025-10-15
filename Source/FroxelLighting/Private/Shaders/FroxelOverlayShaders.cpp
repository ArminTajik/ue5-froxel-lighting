#include "FroxelOverlayShaders.h"

// IMPLEMENT_STATIC_UNIFORM_BUFFER_SLOT(FroxelOverlay);
// IMPLEMENT_STATIC_UNIFORM_BUFFER_STRUCT(FFroxelOverlayUniformParameters, "FroxelOverlay", FroxelOverlay);

IMPLEMENT_GLOBAL_SHADER(FFroxelOverlayHashPS, "/FroxelLighting/FroxelOverlayHash.usf", "MainPS", SF_Pixel);
IMPLEMENT_GLOBAL_SHADER(FFroxelOverlayHeatmapPS, "/FroxelLighting/FroxelOverlayHeatmap.usf", "MainPS", SF_Pixel);

