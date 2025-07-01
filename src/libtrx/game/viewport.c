#include "game/viewport.h"

int16_t Viewport_GetEffectiveFOV(void)
{
    return Viewport_GetSystemFOV() != -1 ? Viewport_GetSystemFOV()
                                         : Viewport_GetUserFOV();
}
