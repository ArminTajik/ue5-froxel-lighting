#include "FroxelLighting.h"

#include "FroxelViewExtension.h"
#include "SceneViewExtension.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"
#include "HAL/IConsoleManager.h"
#include "ProfilingDebugging/RealtimeGPUProfiler.h"

#define LOCTEXT_NAMESPACE "FFroxelLightingModule" // for the sake of convention

DEFINE_LOG_CATEGORY(LogFroxelLighting);

// Console variables and commands for testing ----------------------

TAutoConsoleVariable<int32> CVarFroxelGridX(
    TEXT("r.Froxel.GridX"),
    16,
    TEXT("Froxel grid dimension X."),
    ECVF_Default);

TAutoConsoleVariable<int32> CVarFroxelGridY(
    TEXT("r.Froxel.GridY"),
    16,
    TEXT("Froxel grid dimension Y."),
    ECVF_Default);

TAutoConsoleVariable<int32> CVarFroxelGridZ(
    TEXT("r.Froxel.GridZ"),
    16,
    TEXT("Froxel grid dimension Z."),
    ECVF_Default);

// Module implementation ---------------------------------------

void FFroxelLightingModule::StartupModule() {
    // Making sure the shaders' dir is mapped correctly
    if (const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("FroxelLighting")); Plugin.IsValid()) {
        const FString ShaderDir = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Shaders"));
        AddShaderSourceDirectoryMapping(TEXT("/FroxelLighting"), ShaderDir);
        UE_LOG(LogFroxelLighting, Log, TEXT("Shader dir mapped: %s"), *ShaderDir);
    } else {
        UE_LOG(LogFroxelLighting, Warning, TEXT("Plugin not found via IPluginManager."));
    }

    auto RegisterVE = [this]() {
        ViewExt = FSceneViewExtensions::NewExtension<FFroxelViewExtension>();
        UE_LOG(LogFroxelLighting, Log, TEXT("[Froxel] ViewExtension registered (post engine init)"));
    };

    if (GEngine) {
        RegisterVE();
    } else {
        FCoreDelegates::OnPostEngineInit.AddLambda(RegisterVE);
    }
}

void FFroxelLightingModule::ShutdownModule() {
    ViewExt.Reset();
    UE_LOG(LogFroxelLighting, Log, TEXT("FroxelLighting module shutdown"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FFroxelLightingModule, FroxelLighting)
