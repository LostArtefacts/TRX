#include <trx/core/subsystem.h>
#include <trx/game/lua/events.h>
#include <trx/game/ui/dialogs/takeover.h>

static bool M_Offer(const UI_TAKEOVER screen, const int32_t arg)
{
    const LUA_EVENT_ARG args[] = {
        { .type = LUA_EVENT_ARG_INT32, .value.i32 = screen },
        { .type = LUA_EVENT_ARG_INT32, .value.i32 = arg },
    };
    return LUA_FireEventEx(LUA_EVENT_SCREEN_OPEN, args, 2);
}

static void M_Release(const UI_TAKEOVER screen)
{
    LUA_FireEventInt32(LUA_EVENT_SCREEN_RELEASE, screen);
}

static void M_Init(void)
{
    UI_Takeover_SetHooks((UI_TAKEOVER_HOOKS) {
        .offer = M_Offer,
        .release = M_Release,
    });
}

REGISTER_SUBSYSTEM(.init = M_Init)
