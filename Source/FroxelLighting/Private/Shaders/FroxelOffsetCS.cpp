#include "FroxelOffsetCS.h"

IMPLEMENT_GLOBAL_SHADER(FFroxelOffsetPassACS, "/FroxelLighting/FroxelOffsetPassA.usf", "MainCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FFroxelOffsetPassBCS, "/FroxelLighting/FroxelOffsetPassB.usf", "MainCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FFroxelOffsetPassCCS, "/FroxelLighting/FroxelOffsetPassC.usf", "MainCS", SF_Compute);
