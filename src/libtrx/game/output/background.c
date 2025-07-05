#include "game/output/background.h"

#include "debug.h"
#include "filesystem.h"
#include "game/output/common.h"
#include "game/viewport.h"
#include "log.h"
#include "memory.h"
#include "strings.h"
#include "utils.h"
#include "vector.h"

#include <float.h>
#include <stdio.h>
#include <string.h>

#define M_RELATIVE_ERROR(a, b) ABS((a) - (b)) / (b)

typedef struct {
    char *path;
    int32_t width;
    int32_t height;
} M_CANDIDATE;

static VIEWPORT_RECT m_LastViewport = { .width = -1, .height = -1 };
static char *m_LastPath = nullptr;
static VECTOR *m_CachedCandidates = nullptr;
static size_t m_CachedDirLen = 0;
static char *m_CachedScanPath = nullptr;
static char *m_LastCandidateName = nullptr;

static IMAGE *M_CreateImageFromPath(const char *path);
static float M_GetScreenAspectRatio(void);
static void M_ScanCandidates(const char *path);
static void M_FreeCandidates(void);

static const M_CANDIDATE *M_PickBestCandidate(float screen_ratio);
static bool M_LoadCandidate(const M_CANDIDATE *candidate);
static bool M_LoadMainCandidate(const char *path);

static IMAGE *M_CreateImageFromPath(const char *const path)
{
    if (TR_VERSION == 1) {
        return Image_CreateFromFileInto(
            path, Viewport_GetWidth(VIEWPORT_GAME),
            Viewport_GetHeight(VIEWPORT_GAME), IMAGE_FIT_SMART);
    } else {
        return Image_CreateFromFile(path);
    }
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
    m_LastViewport = Viewport_GetRect(VIEWPORT_GAME);
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
            && m_LastViewport.width == Viewport_GetWidth(VIEWPORT_GAME)
            && m_LastViewport.height == Viewport_GetHeight(VIEWPORT_GAME)
            && String_Equivalent(best->path, m_LastCandidateName)) {
            return;
        }
        if (M_LoadCandidate(best)) {
            return;
        }
    }

    Output_UnloadBackground();
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
