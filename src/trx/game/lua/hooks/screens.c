#include <trx/core/subsystem.h>
#include <trx/game/lua/hooks/common.h>
#include <trx/game/ui/dialogs/takeover.h>

static bool M_Offer(const UI_TAKEOVER screen, const int32_t arg)
{
    return LUA_Hooks_CallBool(
        LUA_HOOK_SCREEN_OPEN, 0, false, LUA_ARG_INT(screen), LUA_ARG_INT(arg));
}

static void M_Release(const UI_TAKEOVER screen)
{
    LUA_Hooks_Call(LUA_HOOK_SCREEN_RELEASE, 0, 0, LUA_ARG_INT(screen));
}

static void M_Init(void)
{
    UI_Takeover_SetHooks((UI_TAKEOVER_HOOKS) {
        .offer = M_Offer,
        .release = M_Release,
    });
}

REGISTER_SUBSYSTEM(.init = M_Init)
