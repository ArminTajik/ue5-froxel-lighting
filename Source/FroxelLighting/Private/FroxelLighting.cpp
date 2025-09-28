#include "FroxelLighting.h"

#include "FroxelClearCS.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphResources.h"
#include "RenderGraphUtils.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"
#include "HAL/IConsoleManager.h"

#define LOCTEXT_NAMESPACE "FFroxelLightingModule" // for the sake of convention

DEFINE_LOG_CATEGORY(LogFroxelLighting);

static int32 GFroxelClearW = 128;
static int32 GFroxelClearH = 128;

static void FroxelClearTest() {
    ENQUEUE_RENDER_COMMAND(FroxelClearDispatch)(
        [](FRHICommandListImmediate& RHICmdList) {

            FRDGBuilder GraphBuilder(RHICmdList);

            const uint32 W = FMath::Max(1, GFroxelClearW);
            const uint32 H = FMath::Max(1, GFroxelClearH);
            const uint32 Count = W * H;

            const FRDGBufferRef OutBuffer = GraphBuilder.CreateBuffer(
                FRDGBufferDesc::CreateStructuredDesc(sizeof(uint32), Count),
                TEXT("FroxelTest.Out"));

            FRDGBufferUAVRef OutUAV = GraphBuilder.CreateUAV(OutBuffer);

            AddClearUAVPass(GraphBuilder, OutUAV, 0u);

            const TShaderMapRef<FFroxelClearCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

            FFroxelClearCS::FParameters* Params = GraphBuilder.AllocParameters<FFroxelClearCS::FParameters>();

            Params->OutputWidth = W;
            Params->OutputHeight = H;
            Params->OutData = OutUAV;

            const FIntVector GroupCounts(
                FMath::DivideAndRoundUp(static_cast<int32>(W), 8),
                FMath::DivideAndRoundUp(static_cast<int32>(H), 8),
                1);

            FComputeShaderUtils::AddPass(
                GraphBuilder,
                RDG_EVENT_NAME("FroxelClearCS W%d H%d", W, H),
                ComputeShader,
                Params,
                GroupCounts);

            GraphBuilder.Execute();

            UE_LOG(LogFroxelLighting, Log, TEXT("Success at dispatching FroxelClearCS (%ux%u)"), W, H);

        }
        );
}

static TAutoConsoleVariable<int32> CVarFroxelClearEveryFrame(
    TEXT("r.Froxel.ClearEveryFrame"),
    1,
    TEXT("Run FroxelClearCS once per frame from a view extension (0 for off, 1 for on state)."),
    ECVF_Default);

static TAutoConsoleVariable<int32> CVarFroxelW(
    TEXT("r.Froxel.ClearW"),
    128,
    TEXT("FroxelClear width."),
    ECVF_Default
    );

static TAutoConsoleVariable<int32> CVarFroxelH(
    TEXT("r.Froxel.ClearH"),
    128,
    TEXT("FroxelClear height."),
    ECVF_Default
    );

static FAutoConsoleCommand GCmdFroxelClearTest(
    TEXT("froxel.ClearTest"),
    TEXT("Dispatch FroxelClearCS once (usage: froxel.ClearTest)"),
    FConsoleCommandDelegate::CreateStatic(&FroxelClearTest));


void FFroxelLightingModule::StartupModule() {
    // Making sure the shaders' dir is mapped correctly
    const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("FroxelLighting"));
    if (Plugin.IsValid()) {
        const FString ShaderDir = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Shaders"));
        AddShaderSourceDirectoryMapping(TEXT("/FroxelLighting"), ShaderDir);
        UE_LOG(LogFroxelLighting, Log, TEXT("Shader dir mapped: %s"), *ShaderDir);
    } else {
        UE_LOG(LogFroxelLighting, Warning, TEXT("Plugin not found via IPluginManager."));
    }
}

void FFroxelLightingModule::ShutdownModule() {

}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FFroxelLightingModule, FroxelLighting)