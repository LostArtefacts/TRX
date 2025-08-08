#include "game/output/background.h"

#include "debug.h"
#include "filesystem.h"
#include "game/objects.h"
#include "game/output/textures.h"
#include "gfx/context.h"
#include "log.h"
#include "memory.h"
#include "strings.h"
#include "utils.h"

#include <float.h>
#include <stdio.h>
#include <string.h>

#define M_RELATIVE_ERROR(a, b) ABS((a) - (b)) / (b)

typedef struct {
    char *path;
    int32_t width;
    int32_t height;
} M_CANDIDATE;

static GFX_2D_RENDERER *m_Renderer2D = nullptr;
static GFX_2D_SURFACE *m_BackgroundSurface = nullptr;
static BACKGROUND_TYPE m_BackgroundType = BK_TRANSPARENT;

static char *m_LastPath = nullptr;
static VECTOR *m_CachedCandidates = nullptr;
static size_t m_CachedDirLen = 0;
static char *m_CachedScanPath = nullptr;
static char *m_LastCandidateName = nullptr;

static void M_ReleaseBackground(void);
static IMAGE *M_CreateImageFromPath(const char *path);
static float M_GetScreenAspectRatio(void);
static void M_ScanCandidates(const char *path);
static void M_FreeCandidates(void);

static const M_CANDIDATE *M_PickBestCandidate(float screen_ratio);
static bool M_LoadCandidate(const M_CANDIDATE *candidate);
static bool M_LoadMainCandidate(const char *path);

static void M_ReleaseBackground(void)
{
    if (m_BackgroundSurface != nullptr) {
        GFX_2D_Surface_Free(m_BackgroundSurface);
        m_BackgroundSurface = nullptr;
    }
}

static IMAGE *M_CreateImageFromPath(const char *const path)
{
    return Image_CreateFromFileInto(
        path, Viewport_GetWidth(VIEWPORT_GAME),
        Viewport_GetHeight(VIEWPORT_GAME), IMAGE_FIT_SMART);
}

static float M_GetScreenAspectRatio(void)
{
    return Viewport_GetWidth(VIEWPORT_GAME)
        / (float)Viewport_GetHeight(VIEWPORT_GAME);
}

static void M_ScanCandidates(const char *const base_image_path)
{
    LOG_INFO("Searching for background images");
    VECTOR *candidates = nullptr;

    const char *last_slash = strrchr(base_image_path, '/');
    const char *last_backslash = strrchr(base_image_path, '\\');
    const char *last_sep =
        last_slash > last_backslash ? last_slash : last_backslash;
    size_t dir_len;
    char *dir_path;
    if (last_sep != nullptr) {
        dir_len = last_sep - base_image_path;
        dir_path = String_Format("%.*s", (int)dir_len, base_image_path);
    } else {
        dir_len = 0;
        dir_path = Memory_DupStr(".");
    }

    const char *file_name = last_sep ? last_sep + 1 : base_image_path;
    const char *ext_ptr = strrchr(file_name, '.');

    void *const dir_handle = File_OpenDirectory(dir_path);
    if (dir_handle == nullptr) {
        goto finish;
    }

    candidates = Vector_Create(sizeof(M_CANDIDATE));
    const char *entry;
    while ((entry = File_ReadDirectory(dir_handle)) != nullptr) {
        // Match the file itself, and assume it's of 16:9 aspect ratio.
        if (String_Equivalent(entry, file_name)) {
            Vector_Add(
                candidates,
                &(M_CANDIDATE) {
                    .path = String_Format("%s/%s", dir_path, file_name),
                    .width = 16,
                    .height = 9,
                });
        }

        // Match directories with pattern: <width>x<height>
        int32_t w = 0, h = 0;
        if (sscanf(entry, "%dx%d", &w, &h) == 2) {
            const char *const candidate_path =
                String_FormatStatic("%s/%s/%s", dir_path, entry, file_name);
            if (File_Exists(candidate_path)) {
                Vector_Add(
                    candidates,
                    &(M_CANDIDATE) {
                        .path = Memory_DupStr(candidate_path),
                        .width = w,
                        .height = h,
                    });
            }
        }
    }
    File_CloseDirectory(dir_handle);

    for (int32_t i = 0; i < candidates->count; i++) {
        const M_CANDIDATE *const candidate = Vector_Get(candidates, i);
        LOG_INFO(
            "%d. %s (%d:%d)", i + 1, candidate->path, candidate->width,
            candidate->height);
    }

finish:
    m_CachedScanPath = dir_path;
    m_CachedCandidates = candidates;
}

static void M_FreeCandidates(void)
{
    if (m_CachedCandidates == nullptr) {
        return;
    }
    for (int32_t i = 0; i < m_CachedCandidates->count; i++) {
        M_CANDIDATE *const candidate = Vector_Get(m_CachedCandidates, i);
        Memory_Free(candidate->path);
    }
    Vector_Free(m_CachedCandidates);
    m_CachedCandidates = nullptr;
    Memory_FreePointer(&m_CachedScanPath);
}

static const M_CANDIDATE *M_PickBestCandidate(const float screen_ratio)
{
    if (m_CachedCandidates == nullptr) {
        return nullptr;
    }
    int32_t best_idx = -1;
    float best_err = FLT_MAX;
    const M_CANDIDATE *const raw = Vector_GetData(m_CachedCandidates);
    for (int32_t i = 0; i < m_CachedCandidates->count; i++) {
        const float candidate_ratio = raw[i].width / (float)raw[i].height;
        const float err = M_RELATIVE_ERROR(candidate_ratio, screen_ratio);
        if (err < best_err) {
            best_err = err;
            best_idx = i;
        }
    }
    return best_idx >= 0 ? &raw[best_idx] : nullptr;
}

static bool M_LoadCandidate(const M_CANDIDATE *const candidate)
{
    LOG_INFO("Loading background image from %s", candidate->path);
    IMAGE *const img = M_CreateImageFromPath(candidate->path);
    if (img == nullptr) {
        return false;
    }
    Output_LoadBackgroundFromImage(img);
    Image_Free(img);
    Memory_FreePointer(&m_LastCandidateName);
    m_LastCandidateName = Memory_DupStr(candidate->path);
    return true;
}

static bool M_LoadMainCandidate(const char *const path)
{
    IMAGE *const img = M_CreateImageFromPath(path);
    if (img == nullptr) {
        return false;
    }
    Output_LoadBackgroundFromImage(img);
    Image_Free(img);
    Memory_FreePointer(&m_LastCandidateName);
    m_LastCandidateName = Memory_DupStr(path);
    return true;
}

void Output_InitBackground(void)
{
    m_Renderer2D = GFX_2D_Renderer_Create();
}

void Output_ShutdownBackground(void)
{
    M_ReleaseBackground();
    if (m_Renderer2D != nullptr) {
        GFX_2D_Renderer_Destroy(m_Renderer2D);
        m_Renderer2D = nullptr;
    }
    Output_ClearLastBackgroundPath();
}

bool Output_LoadBackgroundFromFile(const char *const path)
{
    bool result = false;

    if (m_LastPath == nullptr || !String_Equivalent(path, m_LastPath)) {
        M_FreeCandidates();
        M_ScanCandidates(path);
    }
    const M_CANDIDATE *const best =
        M_PickBestCandidate(M_GetScreenAspectRatio());
    if (best != nullptr && M_LoadCandidate(best)) {
        result = true;
    }
    if (!result) {
        result = M_LoadMainCandidate(path);
    }
    if (result) {
        char *prev = m_LastPath;
        m_LastPath = Memory_DupStr(path);
        Memory_FreePointer(&prev);
    }
    return result;
}

void Output_ReloadBackgroundImage(void)
{
    if (Output_GetBackgroundType() == BK_OBJECT) {
        Output_LoadBackgroundFromObject();
        return;
    }

    if (m_LastPath == nullptr) {
        Output_UnloadBackground();
        return;
    }

    const M_CANDIDATE *best = M_PickBestCandidate(M_GetScreenAspectRatio());
    if (best != nullptr) {
        if (m_LastCandidateName != nullptr
            && String_Equivalent(best->path, m_LastCandidateName)) {
            return;
        }
        if (M_LoadCandidate(best)) {
            return;
        }
    }

    Output_UnloadBackground();
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

#if TR_VERSION == 1
    m_BackgroundType = BK_TRANSPARENT;
#else
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
#endif
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

char *Output_GetLastBackgroundPath(void)
{
    return m_LastPath;
}

void Output_ClearLastBackgroundPath(void)
{
    M_FreeCandidates();
    Memory_FreePointer(&m_LastPath);
    Memory_FreePointer(&m_LastCandidateName);
}
