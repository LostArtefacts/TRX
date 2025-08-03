#include "game/output.h"

#include "game/level.h"
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
#include "game/output/textures.h"
#include "game/shell.h"
#include "game/viewport.h"
#include "global/vars.h"

#include <libtrx/config.h>

static MESH_BATCHER *m_Batcher = nullptr;
static OUTPUT_SHADER *m_Shader = nullptr;
static GFX_2D_RENDERER *m_Renderer2D = nullptr;

static int32_t m_ViewportWidth = 0;
static int32_t m_ViewportHeight = 0;
static GFX_2D_SURFACE *m_PictureSurface = nullptr;

static void M_DownloadBackdropSurface(const IMAGE *image);
static void M_ReleaseSurfaces(void);

static void M_DownloadBackdropSurface(const IMAGE *const image)
{
    GFX_2D_Surface_Free(m_PictureSurface);
    m_PictureSurface = nullptr;
    if (image == nullptr) {
        return;
    }
    m_PictureSurface = GFX_2D_Surface_CreateFromImage(image);
    GFX_2D_Renderer_Upload(
        m_Renderer2D, &m_PictureSurface->desc, m_PictureSurface->buffer);
}

static void M_ReleaseSurfaces(void)
{
    if (m_PictureSurface != nullptr) {
        GFX_2D_Surface_Free(m_PictureSurface);
        m_PictureSurface = nullptr;
    }
}

bool Output_Init(void)
{
    m_Renderer2D = GFX_2D_Renderer_Create();

    Output_ApplyRenderSettings();

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
    return true;
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

    M_ReleaseSurfaces();

    if (m_Renderer2D != nullptr) {
        GFX_2D_Renderer_Destroy(m_Renderer2D);
        m_Renderer2D = nullptr;
    }
    GFX_Context_Detach();
    Output_ClearLastBackgroundPath();
}

void Output_ApplyLevelSettings(void)
{
    Output_SetWaterColor(Level_GetWaterColor());
    Output_SetFogStart(Level_GetFogStart() * WALL_L);
    Output_SetFogEnd(Level_GetFogEnd() * WALL_L);
}

void Output_ApplyRenderSettings(void)
{
    Output_Textures_ApplyRenderSettings();
    Output_ApplyLevelSettings();

    if (m_Shader == nullptr) {
        return;
    }

    if (m_PictureSurface != nullptr
        && (Viewport_GetWidth(VIEWPORT_GAME) != m_ViewportWidth
            || Viewport_GetHeight(VIEWPORT_GAME) != m_ViewportHeight)) {
        GFX_2D_Surface_Free(m_PictureSurface);
        m_PictureSurface = nullptr;
    }

    m_ViewportWidth = Viewport_GetWidth(VIEWPORT_GAME);
    m_ViewportHeight = Viewport_GetHeight(VIEWPORT_GAME);

    GFX_Context_SetVSync(g_Config.rendering.enable_vsync);
    GFX_Context_SetDisplayFilter(g_Config.rendering.upscaling_filter);
    GFX_Context_SetWireframeMode(g_Config.rendering.enable_wireframe);
    GFX_Context_SetLineWidth(g_Config.rendering.wireframe_width);

    const char *const last_path = Output_GetLastBackgroundPath();
    if (last_path != nullptr) {
        Output_LoadBackgroundFromFile(last_path);
    }
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
}

void Output_DispatchRoomFlip(const ROOM *room)
{
    OutputSource_Rooms_ObserveRoomFlip(room);
    OutputSource_RoomsDebug_ObserveRoomFlip(room);
}

void Output_DispatchObjectMeshSwap(
    const OBJECT_MESH *const mesh_1, const OBJECT_MESH *const mesh_2)
{
    OutputSource_Objects_ObserveObjectMeshSwap(mesh_1, mesh_2);
}

void Output_DispatchObjectMeshUpdate(const OBJECT_MESH *const mesh)
{
    OutputSource_Objects_ObserveObjectMeshUpdate(mesh);
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

void Output_Flush(void)
{
    SceneCompositor_Flush();
}

void Output_EndScene(void)
{
    SceneCompositor_EndScene();
}

void Output_FlipScreen(void)
{
    GFX_Context_SwapBuffers();
}

bool Output_LoadBackgroundFromImage(const IMAGE *const image)
{
    M_DownloadBackdropSurface(image);
    return true;
}

void Output_LoadBackgroundFromObject(void)
{
    // TR1 doesn't have inventory background object.
    Output_UnloadBackground();
}

void Output_UnloadBackground(void)
{
    M_DownloadBackdropSurface(nullptr);
    Output_ClearLastBackgroundPath();
}

void Output_DrawBackground(void)
{
    if (m_PictureSurface == nullptr) {
        return;
    }
    GFX_2D_Renderer_Render(m_Renderer2D);
}

void Output_DrawPolyList(void)
{
    // TODO: remove
}

BACKGROUND_TYPE Output_GetBackgroundType(void)
{
    return BK_TRANSPARENT;
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
