using UnrealBuildTool;

public class FroxelLighting : ModuleRules
{
    public FroxelLighting(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "RenderCore",
            "RHI"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "CoreUObject",
            "Engine",
            "Projects",
            "Renderer",
            "RenderCore",
            "RHI"
        });

        // including shader files in the package
        if (Target.Platform == UnrealTargetPlatform.Mac || Target.Platform == UnrealTargetPlatform.Win64)
        {
            RuntimeDependencies.Add("$(PluginDir)/Shaders/**", StagedFileType.NonUFS);
        }
    }
}