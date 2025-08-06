#include "game/output/common.h"

#include "game/inventory_ring.h"
#include "game/level.h"
#include "game/output.h"
#include "game/random.h"
#include "game/shell.h"
#include "game/viewport.h"
#include "global/vars.h"

#include <libtrx/benchmark.h>
#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/game/math.h>
#include <libtrx/game/matrix.h>
#include <libtrx/game/output/mesh_batcher/batcher.h>
#include <libtrx/game/output/scene_compositor.h>
#include <libtrx/game/output/sources/lightnings.h>
#include <libtrx/game/output/sources/misc.h>
#include <libtrx/game/output/sources/objects.h>
#include <libtrx/game/output/sources/rooms.h>
#include <libtrx/game/output/sources/rooms_debug.h>
#include <libtrx/game/output/sources/shadows.h>
#include <libtrx/game/output/sources/sprites.h>
#include <libtrx/game/output/sources/ui.h>
#include <libtrx/game/output/textures.h>
#include <libtrx/game/scaler.h>
#include <libtrx/gfx/context.h>
#include <libtrx/log.h>
#include <libtrx/memory.h>
#include <libtrx/strings.h>
#include <libtrx/utils.h>

#define M_MAX_ROOM_LIGHT_UNIT (0x2000 / (WIBBLE_SIZE / 2))

static MESH_BATCHER *m_Batcher = nullptr;
static OUTPUT_SHADER *m_Shader = nullptr;
static GFX_2D_RENDERER *m_Renderer2D = nullptr;

static int32_t m_WibbleOffset = 0;
static int32_t m_ViewportWidth = 0;
static int32_t m_ViewportHeight = 0;
static GFX_2D_SURFACE *m_BackgroundSurface = nullptr;

static int32_t m_RoomLightShades[RLM_NUMBER_OF] = {};
static ROOM_LIGHT_TABLE m_RoomLightTables[WIBBLE_SIZE] = {};
static BACKGROUND_TYPE m_BackgroundType = BK_TRANSPARENT;

static bool m_IsSunsetEnabled = false;
static int32_t m_SunsetTimer = 0;

static void M_ReleaseBackground(void);

static void M_ReleaseBackground(void)
{
    if (m_BackgroundSurface != nullptr) {
        GFX_2D_Surface_Free(m_BackgroundSurface);
        m_BackgroundSurface = nullptr;
    }
}

void Output_Init(void)
{
    m_Renderer2D = GFX_2D_Renderer_Create();

    for (int32_t i = 0; i < WIBBLE_SIZE; i++) {
        for (int32_t j = 0; j < WIBBLE_SIZE; j++) {
            m_RoomLightTables[i].table[j] = (j - (WIBBLE_SIZE / 2)) * i
                * M_MAX_ROOM_LIGHT_UNIT / (WIBBLE_SIZE - 1);
        }
    }

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

    M_ReleaseBackground();

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

    if (m_BackgroundSurface != nullptr
        && (Viewport_GetWidth(VIEWPORT_GAME) != m_ViewportWidth
            || Viewport_GetHeight(VIEWPORT_GAME) != m_ViewportHeight)) {
        M_ReleaseBackground();
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
    if (Output_GetBackgroundType() == BK_OBJECT) {
        Output_UnloadBackground();
    }
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

BACKGROUND_TYPE Output_GetBackgroundType(void)
{
    return m_BackgroundType;
}

bool Output_LoadBackgroundFromImage(const IMAGE *const image)
{
    M_ReleaseBackground();
    m_BackgroundType = BK_IMAGE;
    m_BackgroundSurface = GFX_2D_Surface_CreateFromImage(image);
    GFX_2D_Renderer_SetRepeat(m_Renderer2D, 1, 1);
    GFX_2D_Renderer_SetTextureSize(m_Renderer2D, nullptr);
    GFX_2D_Renderer_Upload(
        m_Renderer2D, &m_BackgroundSurface->desc, m_BackgroundSurface->buffer);
    GFX_2D_Renderer_SetEffect(m_Renderer2D, GFX_2D_EFFECT_NONE);
    return true;
}

void Output_LoadBackgroundFromObject(void)
{
    M_ReleaseBackground();

    const OBJECT *const obj = Object_Get(O_INV_BACKGROUND);
    if (!obj->loaded) {
        return;
    }

    const OBJECT_MESH *const mesh = Object_GetMesh(obj->mesh_idx);
    if (mesh->num_tex_face4s < 1) {
        return;
    }

    const int32_t texture_idx = mesh->tex_face4s[0].texture_idx;
    const OBJECT_TEXTURE *const texture = Output_GetObjectTexture(texture_idx);
    ASSERT(texture != nullptr);
    const int32_t repeat_y = 6;
    const int32_t repeat_x = repeat_y * Viewport_GetWidth(VIEWPORT_GAME)
        / (float)Viewport_GetHeight(VIEWPORT_GAME);
    m_BackgroundType = BK_OBJECT;

    const RGBA_8888 *const page = Output_GetTexturePage32(texture->tex_page);
    if (page == nullptr) {
        return;
    }

    GFX_2D_SURFACE_DESC desc = {
        .width = TEXTURE_PAGE_WIDTH,
        .height = TEXTURE_PAGE_HEIGHT,
        .bit_count = 32,
        .tex_format = GL_RGBA,
        .tex_type = GL_UNSIGNED_INT_8_8_8_8_REV,
        .uv = {
            {
                texture->uv[0].u / 256.0f / TEXTURE_PAGE_WIDTH,
                texture->uv[0].v / 256.0f / TEXTURE_PAGE_HEIGHT},
            {
                texture->uv[1].u / 256.0f / TEXTURE_PAGE_WIDTH,
                texture->uv[1].v / 256.0f / TEXTURE_PAGE_HEIGHT},
            {
                texture->uv[2].u / 256.0f / TEXTURE_PAGE_WIDTH,
                texture->uv[2].v / 256.0f / TEXTURE_PAGE_HEIGHT},
            {
                texture->uv[3].u / 256.0f / TEXTURE_PAGE_WIDTH,
                texture->uv[3].v / 256.0f / TEXTURE_PAGE_HEIGHT},
        },
        .pitch = TEXTURE_PAGE_WIDTH * 2,
    };
    const OUTPUT_TEXTURE_SIZE size = Output_Textures_GetAtlasSize(texture_idx);
    GFX_2D_Renderer_Upload(m_Renderer2D, &desc, (uint8_t *)page);
    GFX_2D_Renderer_SetRepeat(m_Renderer2D, repeat_x, repeat_y);
    GFX_2D_Renderer_SetTextureSize(
        m_Renderer2D,
        &(GFX_TEXTURE_SIZE) {
            .x0 = size.x0,
            .y0 = size.y0,
            .x1 = size.x1,
            .y1 = size.y1,
        });
    GFX_2D_Renderer_SetEffect(m_Renderer2D, GFX_2D_EFFECT_VIGNETTE);
}

void Output_UnloadBackground(void)
{
    M_ReleaseBackground();
    m_BackgroundType = BK_TRANSPARENT;
    Output_ClearLastBackgroundPath();
}

void Output_DrawBackground(void)
{
    if (m_BackgroundType == BK_TRANSPARENT) {
        return;
    }
    GFX_2D_Renderer_Render(m_Renderer2D);
}

int32_t Output_GetObjectBounds(const BOUNDS_16 *const bounds)
{
    return 1;
}

void Output_SetSunsetEnabled(const bool enabled)
{
    m_IsSunsetEnabled = enabled;
}

void Output_SetSunsetTimer(const int32_t timer)
{
    m_SunsetTimer = timer;
}

int32_t Output_GetSunsetDuration(void)
{
    return 20 * 60 * LOGIC_FPS; // = 20 minutes / 36000 frames
}

int32_t Output_GetSunsetTimer(void)
{
    return m_SunsetTimer;
}

int32_t Output_GetRoomLightShade(const ROOM_LIGHT_MODE mode)
{
    return m_RoomLightShades[mode];
}

void Output_LightRoomVertices(const ROOM *const room)
{
    const ROOM_LIGHT_TABLE *const light_table =
        &m_RoomLightTables[m_RoomLightShades[room->light_mode]];
    for (int32_t i = 0; i < room->mesh.num_vertices; i++) {
        ROOM_VERTEX *const vtx = &room->mesh.vertices[i];
        const int32_t wibble =
            light_table->table[vtx->light_table_value % WIBBLE_SIZE];
        vtx->light_adder = vtx->light_base + wibble;
    }
}

void Output_AnimateShades(const int32_t num_frames)
{
    m_WibbleOffset += num_frames;
    m_WibbleOffset %= WIBBLE_SIZE;
    m_RoomLightShades[RLM_FLICKER] = Random_GetDraw() % WIBBLE_SIZE;
    m_RoomLightShades[RLM_GLOW] = (WIBBLE_SIZE - 1)
            * (Math_Sin((m_WibbleOffset * DEG_360) / WIBBLE_SIZE) + 0x4000)
        >> 15;

    if (m_IsSunsetEnabled) {
        m_SunsetTimer += num_frames;
        CLAMPG(m_SunsetTimer, Output_GetSunsetDuration());
        m_RoomLightShades[RLM_SUNSET] =
            m_SunsetTimer * (WIBBLE_SIZE - 1) / Output_GetSunsetDuration();
    }
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
