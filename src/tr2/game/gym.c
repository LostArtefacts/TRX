#include "game/gym.h"

#include "game/game_flow.h"

static bool m_IsInventoryOpenEnabled = true;

bool Gym_IsAccessible(void)
{
    return g_GameFlow.gym_enabled && GF_GetGymLevel() != nullptr;
}

void Gym_SetInventoryOpenEnabled(const bool enabled)
{
    m_IsInventoryOpenEnabled = enabled;
}

bool Gym_IsInventoryOpenEnabled(void)
{
    return m_IsInventoryOpenEnabled;
}
