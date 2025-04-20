#include "game/game_flow.h"

#include <libtrx/game/gym.h>

bool Gym_IsAccessible(void)
{
    return g_GameFlow.gym_enabled && GF_GetGymLevel() != nullptr;
}
