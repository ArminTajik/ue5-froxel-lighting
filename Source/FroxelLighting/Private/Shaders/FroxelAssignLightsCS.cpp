#include "FroxelAssignLightsCS.h"

IMPLEMENT_GLOBAL_SHADER(FFroxelTotalIndicesCS, "/FroxelLighting/FroxelTotalIndices.usf", "MainCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FFroxelAssignLightsCS, "/FroxelLighting/FroxelAssignLights.usf", "MainCS", SF_Compute);
