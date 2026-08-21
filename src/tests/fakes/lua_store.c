// The Lua store as the resume info uses it, which is only something to empty
// when a playthrough starts. Standing the real one up would bring the whole Lua
// state with it, and the resume info is not tested for any of that.

#include <trx/game/lua/store.h>

void LUA_Store_ClearGame(void)
{
}
