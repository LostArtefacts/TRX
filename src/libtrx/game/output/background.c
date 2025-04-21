#include "game/output/background.h"

#include "debug.h"
#include "filesystem.h"
#include "game/viewport.h"
#include "log.h"
#include "memory.h"
#include "strings.h"
#include "utils.h"
#include "vector.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define M_RELATIVE_ERROR(a, b) ABS((a) - (b)) / (b)

typedef struct {
    char *file_name;
    float diff;
} M_CANDIDATE;

static char *m_LastPath = nullptr;

static IMAGE *M_CreateImageFromPath(const char *path);
static float M_GetScreenAspectRatio(void);
static int M_CompareCandidates(const void *a, const void *b);
static VECTOR *M_ScanCandidates(const char *path, size_t *dir_len_out);
static bool M_TryLoadCandidates(
    const char *path, VECTOR *candidates, size_t dir_len);
static void M_FreeCandidates(VECTOR *candidates);

static IMAGE *M_CreateImageFromPath(const char *const path)
{
    if (TR_VERSION == 1) {
        return Image_CreateFromFileInto(
            path, Viewport_GetWidth(), Viewport_GetHeight(), IMAGE_FIT_SMART);
    } else {
        return Image_CreateFromFile(path);
    }
}

static float M_GetScreenAspectRatio(void)
{
    return Viewport_GetWidth() / (float)Viewport_GetHeight();
}

static int M_CompareCandidates(const void *const a, const void *const b)
{
    const M_CANDIDATE *const c1 = a;
    const M_CANDIDATE *const c2 = b;
    if (c1->diff < c2->diff) {
        return -1;
    }
    if (c1->diff > c2->diff) {
        return 1;
    }
    return 0;
}

static VECTOR *M_ScanCandidates(
    const char *const path, size_t *const dir_len_out)
{
    VECTOR *candidates = nullptr;

    const char *last_slash = strrchr(path, '/');
    const char *last_backslash = strrchr(path, '\\');
    const char *last_sep =
        last_slash > last_backslash ? last_slash : last_backslash;
    size_t dir_len;
    char *dir_path;
    if (last_sep != nullptr) {
        dir_len = last_sep - path;
        dir_path = String_Format("%.*s", (int)dir_len, path);
    } else {
        dir_len = 0;
        dir_path = Memory_DupStr(".");
    }

    const char *file_name = last_sep ? last_sep + 1 : path;
    const char *ext_ptr = strrchr(file_name, '.');
    const float screen_ratio = M_GetScreenAspectRatio();

    void *const dir_handle = File_OpenDirectory(dir_path);
    if (dir_handle == nullptr) {
        goto finish;
    }

    candidates = Vector_Create(sizeof(M_CANDIDATE));
    const char *entry;
    while ((entry = File_ReadDirectory(dir_handle)) != nullptr) {
        // Match the file itself, and assume it's of 16:9 aspect ratio.
        if (String_Equivalent(entry, file_name)) {
            const float ratio = 16.0f / 9.0f;
            Vector_Add(
                candidates,
                &(M_CANDIDATE) {
                    .file_name = Memory_DupStr(file_name),
                    .diff = M_RELATIVE_ERROR(ratio, screen_ratio),
                });
        }

        // Match directories with pattern: <width>x<height>
        int32_t w = 0, h = 0;
        if (sscanf(entry, "%dx%d", &w, &h) == 2) {
            const float ratio = w / (float)h;
            Vector_Add(
                candidates,
                &(M_CANDIDATE) {
                    .file_name = String_Format("%s/%s", entry, file_name),
                    .diff = M_RELATIVE_ERROR(ratio, screen_ratio),
                });
        }
    }
    File_CloseDirectory(dir_handle);

finish:
    *dir_len_out = dir_len;
    Memory_FreePointer(&dir_path);
    return candidates;
}

static bool M_TryLoadCandidates(
    const char *const path, VECTOR *const candidates, const size_t dir_len)
{
    for (int32_t i = 0; i < candidates->count; i++) {
        const M_CANDIDATE *candidate = Vector_Get(candidates, i);
        char *full_path;
        if (dir_len > 0) {
            full_path = String_Format(
                "%.*s/%s", (int32_t)dir_len, path, candidate->file_name);
        } else {
            full_path = String_Format("%s", candidate->file_name);
        }
        IMAGE *const image = M_CreateImageFromPath(full_path);
        Memory_FreePointer(&full_path);
        if (image != nullptr) {
            Output_LoadBackgroundFromImage(image);
            Image_Free(image);
            return true;
        }
    }
    return false;
}

static void M_FreeCandidates(VECTOR *const candidates)
{
    if (candidates == nullptr) {
        return;
    }
    for (int32_t i = 0; i < candidates->count; i++) {
        M_CANDIDATE *const candidate = Vector_Get(candidates, i);
        Memory_Free(candidate->file_name);
    }
    Vector_Free(candidates);
}

bool Output_LoadBackgroundFromFile(const char *const path)
{
    LOG_INFO("Loading image %s", path);
    size_t dir_len = 0;
    bool result = false;

    // Try aspect-ratio specific directories.
    VECTOR *candidates = M_ScanCandidates(path, &dir_len);
    if (candidates != nullptr) {
        M_CANDIDATE *raw_candidates = Vector_GetData(candidates);
        qsort(
            raw_candidates, candidates->count, sizeof(M_CANDIDATE),
            M_CompareCandidates);
        for (int32_t i = 0; i < candidates->count; i++) {
            const M_CANDIDATE *const candidate = Vector_Get(candidates, i);
            LOG_INFO(
                "Found candidate %s (diff=%.02f)", candidate->file_name,
                candidate->diff);
        }
        if (M_TryLoadCandidates(path, candidates, dir_len)) {
            result = true;
        }
    }

    if (!result) {
        // Fallback to the main image.
        IMAGE *const image = M_CreateImageFromPath(path);
        if (image != nullptr) {
            result = true;
            Output_LoadBackgroundFromImage(image);
            Image_Free(image);
        }
    }

    M_FreeCandidates(candidates);
    if (result) {
        char *prev = m_LastPath;
        m_LastPath = Memory_DupStr(path);
        Memory_FreePointer(&prev);
    }
    return result;
}

char *Output_GetLastBackgroundPath(void)
{
    return m_LastPath;
}

void Output_ClearLastBackgroundPath(void)
{
    Memory_FreePointer(&m_LastPath);
}
