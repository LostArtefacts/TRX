#include "game/output/common.h"

#include "config.h"
#include "game/level.h"
#include "game/output/background.h"
#include "game/output/func.h"
#include "game/output/lights.h"
#include "game/output/mesh_batcher/batcher.h"
#include "game/output/scene_compositor.h"
#include "game/output/sources/lightnings.h"
#include "game/output/sources/misc.h"
#include "game/output/sources/objects.h"
#include "game/output/sources/rooms.h"
#include "game/output/sources/rooms_debug.h"
#include "game/output/sources/shadows.h"
#include "game/output/sources/sprites.h"
#include "game/output/sources/ui.h"
#include "game/output/state.h"
#include "game/output/textures.h"
#include "game/output/warmup.h"
#include "game/shell.h"
#include "gfx/context.h"

static MESH_BATCHER *m_Batcher = nullptr;
static OUTPUT_SHADER *m_Shader = nullptr;

void Output_Init(void)
{
    SceneCompositor_Init();
    Output_Textures_Init();

    m_Shader = Output_Shader_Create("shaders/meshes.glsl");
    m_Batcher = MeshBatcher_Create();
    SceneCompositor_AddSource(MeshBatcher_AsSource(m_Batcher));
    OutputSource_Rooms_Init(m_Batcher);
    OutputSource_RoomsDebug_Init();
    OutputSource_Objects_Init(m_Batcher);
    OutputSource_Sprites_Init(m_Batcher);
    OutputSource_Lightnings_Init();
    OutputSource_Shadows_Init(m_Batcher);
    OutputSource_Misc_Init();
    OutputSource_UI_Init();

    Output_InitLight();
    Output_InitBackground();
    Output_WarmUp();

    Output_ApplyRenderSettings();
}

void Output_Shutdown(void)
{
    SceneCompositor_Shutdown();
    OutputSource_Rooms_Shutdown();
    OutputSource_RoomsDebug_Shutdown();
    OutputSource_Objects_Shutdown();
    OutputSource_Sprites_Shutdown();
    OutputSource_Lightnings_Shutdown();
    OutputSource_Shadows_Shutdown();
    OutputSource_Misc_Shutdown();

    if (m_Shader != nullptr) {
        Output_Shader_Free(m_Shader);
        m_Shader = nullptr;
    }
    if (m_Batcher != nullptr) {
        MeshBatcher_Destroy(m_Batcher);
        m_Batcher = nullptr;
    }

    Output_Textures_Shutdown();
    Output_ShutdownLight();
    Output_ShutdownBackground();

    GFX_Context_Detach();
}

bool Output_IsHeadless(void)
{
    return Shell_GetArgs()->headless;
}

OUTPUT_SHADER *Output_GetMeshShader(void)
{
    return m_Shader;
}

void Output_BeginScene(void)
{
    Output_ApplyFOV();
    GFX_Context_Clear();
    GFX_Track_Reset();
    GFX_Context_SetWireframeMode(g_Config.rendering.enable_wireframe);
    SceneCompositor_BeginScene();
}

void Output_EndScene(void)
{
    SceneCompositor_EndScene();
}

void Output_Flush(void)
{
    SceneCompositor_Flush();
}

void Output_FlipScreen(void)
{
    GFX_Context_SwapBuffers();
}

void Output_SwitchViewport(const VIEWPORT_SPACE space)
{
    if (space == VIEWPORT_GAME) {
        GFX_Renderer_BindGeometryFbo();
    } else if (space == VIEWPORT_UI) {
        GFX_Renderer_BindUiFbo();
    }
    GFX_Context_SwitchToViewport(space);
    GFX_Context_Clear();
    glClear(GL_DEPTH_BUFFER_BIT);
}

void Output_ApplyRenderSettings(void)
{
    Output_Textures_ApplyRenderSettings();
    Output_ApplyLevelSettings();

    if (m_Shader == nullptr) {
        return;
    }

    GFX_Context_SetVSync(g_Config.rendering.enable_vsync);
    GFX_Context_SetDisplayFilter(g_Config.rendering.upscaling_filter);
    GFX_Context_SetWireframeMode(g_Config.rendering.enable_wireframe);
    GFX_Context_SetLineWidth(g_Config.rendering.wireframe_width);
}

void Output_ApplyLevelSettings(void)
{
    Output_SetWaterColor(Level_GetWaterColor());
    Output_SetFogColor(Level_GetFogColor());
    Output_SetFogStart(Level_GetFogStart() * WALL_L);
    Output_SetFogEnd(Level_GetFogEnd() * WALL_L);
}

void Output_DispatchLevelLoad(void)
{
    Output_Textures_ObserveLevelLoad();

    OutputSource_Objects_ObserveLevelLoad();
    OutputSource_Rooms_ObserveLevelLoad();
    OutputSource_RoomsDebug_ObserveLevelLoad();
    OutputSource_Sprites_ObserveLevelLoad();

    MeshBatcher_Seal(m_Batcher);

    Output_ApplyLevelSettings();
}

void Output_DispatchLevelUnload(void)
{
    OutputSource_Objects_ObserveLevelUnload();
    OutputSource_Rooms_ObserveLevelUnload();
    OutputSource_RoomsDebug_ObserveLevelUnload();
    OutputSource_Sprites_ObserveLevelUnload();
    if (Output_GetBackgroundType() == BK_PATTERN_STATIC
        || Output_GetBackgroundType() == BK_PATTERN_WAVE) {
        Output_UnloadBackground();
    }
}

void Output_DispatchRoomFlip(const ROOM *room)
{
    OutputSource_Rooms_ObserveRoomFlip(room);
    OutputSource_RoomsDebug_ObserveRoomFlip(room);
}

void Output_DispatchObjectMeshSwap(
    const int32_t mesh_idx_1, const int32_t mesh_idx_2)
{
    OutputSource_Objects_ObserveObjectMeshSwap(mesh_idx_1, mesh_idx_2);
}

void Output_DispatchObjectMeshUpdate(const int32_t mesh_idx)
{
    OutputSource_Objects_ObserveObjectMeshUpdate(mesh_idx);
}
