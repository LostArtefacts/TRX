#include "math/math_misc.h"

#include "global/const.h"
#include "global/types.h"
#include "math/math.h"

#include <libtrx/utils.h>

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

DIRECTION Math_GetDirectionCone(const int16_t angle, const int16_t cone)
{
    if (angle >= -cone && angle <= cone) {
        return DIR_NORTH;
    } else if (angle >= PHD_90 - cone && angle <= PHD_90 + cone) {
        return DIR_WEST;
    } else if (angle >= PHD_180 - cone || angle <= -PHD_180 + cone) {
        return DIR_SOUTH;
    } else if (angle >= -PHD_90 - cone && angle <= -PHD_90 + cone) {
        return DIR_EAST;
    }
    return DIR_UNKNOWN;
}

int16_t Math_DirectionToAngle(const DIRECTION dir)
{
    switch (dir) {
    case DIR_NORTH:
        return 0;
    case DIR_WEST:
        return PHD_90;
    case DIR_SOUTH:
        return PHD_180;
    case DIR_EAST:
        return -PHD_90;
    default:
        return 0;
    }
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

int32_t XYZ_32_GetDistance(const XYZ_32 *const pos1, const XYZ_32 *const pos2)
{
    // clang-format off
    return Math_Sqrt(
        SQUARE(pos1->x - pos2->x) +
        SQUARE(pos1->y - pos2->y) +
        SQUARE(pos1->z - pos2->z)
    );
    // clang-format on
}

bool XYZ_32_AreEquivalent(const XYZ_32 *const pos1, const XYZ_32 *const pos2)
{
    return pos1->x == pos2->x && pos1->y == pos2->y && pos1->z == pos2->z;
}
