#include "game/math_misc.h"

#include "game/math.h"

#include <libtrx/utils.h>

void __cdecl Math_GetVectorAngles(
    int32_t x, int32_t y, int32_t z, int16_t *dest)
{
    dest[0] = Math_Atan(z, x);

    while ((int16_t)x != x || (int16_t)y != y || (int16_t)z != z) {
        x >>= 2;
        y >>= 2;
        z >>= 2;
    }

    int16_t pitch = Math_Atan(Math_Sqrt(SQUARE(x) + SQUARE(z)), y);
    if ((y > 0 && pitch > 0) || (y < 0 && pitch < 0)) {
        pitch = -pitch;
    }

    dest[1] = pitch;
}
