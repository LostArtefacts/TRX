#include "game/gym.h"

static bool m_IsInventoryOpenEnabled = true;

void Gym_SetInventoryOpenEnabled(const bool enabled)
{
    m_IsInventoryOpenEnabled = enabled;
}

bool Gym_IsInventoryOpenEnabled(void)
{
    return m_IsInventoryOpenEnabled;
}
