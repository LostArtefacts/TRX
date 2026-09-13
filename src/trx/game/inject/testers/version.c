#include <trx/core/file.h>
#include <trx/game/inject.h>
#include <trx/version.h>

static bool M_TestGameVersion(
    const INJECTION_CONTEXT *const ctx, const INJECTION *const injection)
{
    const int32_t version = File_ReadS32(injection->fp);
    return version == g_TRVersion;
}

REGISTER_INJECT_TESTER(ITT_GAME_VERSION, M_TestGameVersion)
