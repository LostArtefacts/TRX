#include "math/math_misc.h"

#include "global/const.h"
#include "global/types.h"
#include "math/math.h"
#include "util.h"

void Math_GetVectorAngles(int32_t x, int32_t y, int32_t z, int16_t *dest)
{
    dest[0] = Math_Atan(z, x);

    while ((int16_t)x != x || (int16_t)y != y || (int16_t)z != z) {
        x >>= 2;
        y >>= 2;
        z >>= 2;
    }

    PHD_ANGLE pitch = Math_Atan(Math_Sqrt(SQUARE(x) + SQUARE(z)), y);
    if ((y > 0 && pitch > 0) || (y < 0 && pitch < 0)) {
        pitch = -pitch;
    }

    dest[1] = pitch;
}

int32_t Math_AngleInCone(int32_t angle1, int32_t angle2, int32_t cone)
{
    const int32_t diff = ((int)(angle1 - angle2 + PHD_180)) % PHD_360 - PHD_180;
    return ABS(diff) < cone;
}

int32_t Math_AngleMean(int32_t angle1, int32_t angle2, double ratio)
{
    int32_t diff = angle2 - angle1;

    if (diff > PHD_180) {
        diff -= PHD_360;
    } else if (diff < -PHD_180) {
        diff += PHD_360;
    }

    int32_t result = angle1 + diff * ratio;

    result %= PHD_360;
    if (result < 0) {
        result += PHD_360;
    }

    return result;
}
