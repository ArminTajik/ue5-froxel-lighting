#include "FroxelLighting.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"

#define LOCTEXT_NAMESPACE "FFroxelLightingModule" // for the sake of convention

DEFINE_LOG_CATEGORY(LogFroxelLighting);

void FFroxelLightingModule::StartupModule()
{
    // Making sure the shaders' dir is mapped correctly
    const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("FroxelLighting"));
    if (Plugin.IsValid())
    {
        const FString ShaderDir = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Shaders"));
        AddShaderSourceDirectoryMapping(TEXT("/FroxelLighting"), ShaderDir);
        UE_LOG(LogFroxelLighting, Log, TEXT("Shader dir mapped: %s"), *ShaderDir);
    }
    else
    {
        UE_LOG(LogFroxelLighting, Warning, TEXT("Plugin not found via IPluginManager."));
    }
}

void FFroxelLightingModule::ShutdownModule()
{
 
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FFroxelLightingModule, FroxelLighting)