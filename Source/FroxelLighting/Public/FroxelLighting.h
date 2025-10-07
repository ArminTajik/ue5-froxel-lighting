#pragma once
#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "ProfilingDebugging/RealtimeGPUProfiler.h"
class FFroxelViewExtension;
DECLARE_LOG_CATEGORY_EXTERN(LogFroxelLighting, Log, All);
DECLARE_GPU_STAT_NAMED_EXTERN(Froxel, TEXT("Froxel"));

// Module interface. Loads shaders and sets up mappings on startup.
class FFroxelLightingModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    TSharedPtr<FFroxelViewExtension, ESPMode::ThreadSafe> ViewExt;
};