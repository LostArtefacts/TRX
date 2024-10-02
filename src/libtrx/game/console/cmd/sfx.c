#include "game/console/cmd/sfx.h"

#include "game/console/common.h"
#include "game/game_string.h"
#include "game/sound.h"
#include "memory.h"
#include "strings.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

static char *M_CreateRangeString(void);
static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *ctx);

static char *M_CreateRangeString(void)
{
    size_t buffer_size = 64;
    char *result = Memory_Alloc(buffer_size);

    int32_t prev = -1;
    int32_t start = -1;
    for (int32_t i = 0; i <= SFX_NUMBER_OF; i++) {
        const bool valid = Sound_IsAvailable(i);

        if (valid && start == -1) {
            start = i;
        }
        if (!valid && start != -1) {
            char temp[32];
            if (start == prev) {
                sprintf(temp, "%d, ", prev);
            } else {
                sprintf(temp, "%d-%d, ", start, prev);
            }

            const int32_t len = strlen(temp);
            if (strlen(result) + len >= buffer_size) {
                buffer_size *= 2;
                result = Memory_Realloc(result, buffer_size);
            }

            strcat(result, temp);
            start = -1;
        }

        if (valid) {
            prev = i;
        }
    }

    // Remove the trailing comma and space
    result[strlen(result) - 2] = '\0';

    return result;
}

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *const ctx)
{
    if (String_IsEmpty(ctx->args)) {
        char *ranges = M_CreateRangeString();
        Console_Log(GS(OSD_SOUND_AVAILABLE_SAMPLES), ranges);
        Memory_FreePointer(&ranges);
        return CR_SUCCESS;
    }

    int32_t sfx_id;
    if (!String_ParseInteger(ctx->args, &sfx_id)) {
        return CR_BAD_INVOCATION;
    }

    if (!Sound_IsAvailable(sfx_id)) {
        Console_Log(GS(OSD_INVALID_SAMPLE), sfx_id);
        return CR_FAILURE;
    }

    Console_Log(GS(OSD_SOUND_PLAYING_SAMPLE), sfx_id);
    Sound_Effect(sfx_id, NULL, SPM_ALWAYS);
    return CR_SUCCESS;
}

CONSOLE_COMMAND g_Console_Cmd_SFX = {
    .prefix = "sfx",
    .proc = M_Entrypoint,
};
