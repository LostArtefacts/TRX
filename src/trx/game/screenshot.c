#include <trx/game/screenshot.h>

#include <trx/core/filesystem.h>
#include <trx/core/memory.h>
#include <trx/core/strings.h>
#include <trx/game/clock.h>
#include <trx/game/events.h>
#include <trx/game/game.h>
#include <trx/game/game_flow/common.h>
#include <trx/game/output.h>
#include <trx/game/shell.h>

#include <stdio.h>
#include <string.h>

static char *M_GetScreenshotTitle(void)
{
    const GF_LEVEL *const level = GF_GetCurrentLevel();
    if (level == nullptr) {
        return Memory_DupStr("Intro");
    }

    if (level->title != nullptr && strlen(level->title) > 0) {
        char *clean_level_title = String_ToFileName(level->title);
        if (clean_level_title != nullptr && strlen(clean_level_title) > 0) {
            return clean_level_title;
        }
        Memory_FreePointer(&clean_level_title);
    }

    // If title totally invalid, name it based on level number
    const char *const fmt = "Level_%d";
    const size_t result_size = snprintf(nullptr, 0, fmt, level->num) + 1;
    char *result = Memory_Alloc(result_size);
    snprintf(result, result_size, fmt, level->num);
    return result;
}

static char *M_GetScreenshotBaseName(void)
{
    char *screenshot_title = M_GetScreenshotTitle();

    // Get timestamp
    char date_time[30];
    Clock_GetDateTime(date_time, 30);

    // Full screenshot name
    char *const result = String_Format("%s_%s", date_time, screenshot_title);
    Memory_FreePointer(&screenshot_title);
    return result;
}

static const char *M_GetScreenshotFileExt(const SCREENSHOT_FORMAT format)
{
    switch (format) {
    case SCREENSHOT_FORMAT_JPEG:
        return "jpg";
    case SCREENSHOT_FORMAT_PNG:
        return "png";
    default:
        return "jpg";
    }
}

static char *M_GetScreenshotPath(const SCREENSHOT_FORMAT format)
{
    char *base_name = M_GetScreenshotBaseName();
    const char *const ext = M_GetScreenshotFileExt(format);
    char *rel_path = String_Format("%s.%s", base_name, ext);

    const char *resolved =
        GamePath_PeekResolve(GAME_DYNAMIC_PATH_SCREENSHOT_WRITE_FILE, rel_path);
    Memory_FreePointer(&rel_path);
    if (resolved == nullptr) {
        Memory_FreePointer(&base_name);
        return nullptr;
    }
    char *full_path = Memory_DupStr(resolved);
    SHOULD(FS_EnsureParentDirectories(full_path));
    if (FS_Exists(full_path)) {
        for (int i = 2; i < 100; i++) {
            Memory_FreePointer(&full_path);
            rel_path = String_Format("%s_%d.%s", base_name, i, ext);
            resolved = GamePath_PeekResolve(
                GAME_DYNAMIC_PATH_SCREENSHOT_WRITE_FILE, rel_path);
            Memory_FreePointer(&rel_path);
            if (resolved == nullptr) {
                Memory_FreePointer(&base_name);
                return nullptr;
            }
            full_path = Memory_DupStr(resolved);
            if (!FS_Exists(full_path)) {
                break;
            }
        }
    }

    Memory_FreePointer(&base_name);
    return full_path;
}

void Screenshot_Make(const SCREENSHOT_FORMAT format)
{
    char *full_path = M_GetScreenshotPath(format);
    Output_MakeScreenshot(full_path);
    GameEvent_Fire((EVENT) {
        .name = GAME_EVENT_SCREENSHOT,
        .data = full_path,
    });
    Memory_FreePointer(&full_path);
}

void Screenshot_MakeToPath(const char *const path)
{
    Output_MakeScreenshot(path);
}
