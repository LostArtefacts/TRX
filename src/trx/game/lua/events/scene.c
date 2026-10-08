#include <trx/game/lua/events/scene.h>

#include <trx/game/lua/events.h>

static bool m_Painting = false;

void LUA_Scene_Paint(void)
{
    LUA_Scene_SetPainting(true);
    LUA_FireEvent(LUA_EVENT_SCENE_PAINT);
    LUA_Scene_SetPainting(false);
}

void LUA_Scene_SetPainting(const bool painting)
{
    m_Painting = painting;
}

bool LUA_Scene_IsPainting(void)
{
    return m_Painting;
}
