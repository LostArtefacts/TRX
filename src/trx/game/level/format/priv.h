#pragma once

#include <trx/core/file.h>
#include <trx/core/result.h>
#include <trx/core/utils.h>
#include <trx/game/level/format/format.h>

void Level_Format_RegisterLoader(
    int32_t priority, const LEVEL_FORMAT_LOADER *loader);

#define REGISTER_LEVEL_FORMAT_LOADER(priority_, loader_)                       \
    __attribute__((__constructor__)) static void CONCAT(                       \
        M_RegisterLevelFormatLoader_, __LINE__)(void)                          \
    {                                                                          \
        Level_Format_RegisterLoader(priority_, &(loader_));                    \
    }

// Helper control-flow macros
// =============================================================================
// If the read fails, leave the enclosing function with ERR. The file reads
// hand back a plain bool, so this cannot be MUST.
#define LEVEL_FORMAT_TRY_OR_FAIL(call_)                                        \
    do {                                                                       \
        if (!(call_)) {                                                        \
            return ERR;                                                        \
        }                                                                      \
    } while (0)

#define LEVEL_FORMAT_SKIP_OR_FAIL(size_)                                       \
    LEVEL_FORMAT_TRY_OR_FAIL(File_TrySkip(file, size_))

#define LEVEL_FORMAT_SKIP_ARR_S32_OR_FAIL(size_)                               \
    do {                                                                       \
        int32_t count;                                                         \
        LEVEL_FORMAT_TRY_OR_FAIL(File_TryReadS32(file, &count));               \
        LEVEL_FORMAT_SKIP_OR_FAIL(count * (size_));                            \
    } while (0)

#define LEVEL_FORMAT_SKIP_ARR_U16_OR_FAIL(size_)                               \
    do {                                                                       \
        uint16_t count;                                                        \
        LEVEL_FORMAT_TRY_OR_FAIL(File_TryReadU16(file, &count));               \
        LEVEL_FORMAT_SKIP_OR_FAIL(count * (size_));                            \
    } while (0)
