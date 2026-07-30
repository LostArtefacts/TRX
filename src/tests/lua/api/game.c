// The game flow surface. The assertions live in game.lua;
// this stands up the world they run against.

#include <fakes/game.h>
#include <harness/lua_surface.h>

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "game",
        .tests = "api/game",
        .push_fake = FakeGame_PushLua,
    };
    return LuaSurface_Run(&test);
}
