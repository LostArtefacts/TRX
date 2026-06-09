#include <trx/core/memory.h>
#include <trx/core/strings.h>
#include <trx/game/console/common.h>
#include <trx/game/console/registry.h>
#include <trx/game/game_strings/entries.h>
#include <trx/game/sound.h>

static bool M_IsAvailableSample(const int32_t value, void *)
{
    return Sound_IsAvailable_Direct(value);
}

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *const ctx)
{
    if (String_IsEmpty(ctx->args)) {
        char *ranges = String_FormatRanges(
            0, Sound_GetMaxDirectSampleID(), M_IsAvailableSample, nullptr);
        Console_Log(GS("general/osd/sound_available_samples"), ranges);
        Memory_FreePointer(&ranges);
        return CR_SUCCESS;
    }

    SAMPLE_ID sfx_id;
    if (!String_ParseInteger(ctx->args, &sfx_id)) {
        return CR_BAD_INVOCATION;
    }

    if (!Sound_IsAvailable_Direct(sfx_id)) {
        Console_LogError(GS("general/osd/invalid_sample"), sfx_id);
        return CR_FAILURE;
    }

    Console_Log(GS("general/osd/sound_playing_sample"), sfx_id);
    Sound_Effect_Direct(sfx_id, nullptr, SPM_ALWAYS);
    return CR_SUCCESS;
}

REGISTER_CONSOLE_COMMAND("sfx", M_Entrypoint, GS_ID("console/cmd/sfx/help"))
