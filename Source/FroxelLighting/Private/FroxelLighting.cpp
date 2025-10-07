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

TAutoConsoleVariable<int32> CVarFroxelVisualize(
    TEXT("r.Froxel.Visualize"),
    1,
    TEXT("Visualize Froxel grid (0: off, 1: on)."),
    ECVF_Default);

TAutoConsoleVariable<int32> CVarFroxelMaxLightsPerFroxel(
        TEXT("r.Froxel.MaxLightsPerFroxel"),
        32,
        TEXT("Maximum number of lights that can be assigned to a single froxel."),
        ECVF_Default);

TAutoConsoleVariable<int32> CVarFroxelFarClipCm(
        TEXT("r.Froxel.FarClipCm"),
        100000,
        TEXT("Froxel grid far clipping plane in cm."),
        ECVF_Default);

TAutoConsoleVariable<int32> CVarFroxelZMode(
        TEXT("r.Froxel.ZMode"),
        1,
        TEXT("Froxel grid Z distribution mode (0: linear, 1: logarithmic)."),
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
        UE_LOG(LogFroxelLighting, Log, TEXT("ViewExtension registered (post engine init)"));
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
