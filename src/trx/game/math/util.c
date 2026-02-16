#include <trx/game/math.h>
#include <trx/utils.h>

#include <math.h>

uint32_t Math_Sqrt(uint32_t n)
{
    uint32_t result = 0;
    uint32_t base = 0x40000000;
    do {
        do {
            uint32_t based_result = base + result;
            result >>= 1;
            if (based_result > n) {
                break;
            }
            n -= based_result;
            result |= base;

            base >>= 2;
        } while (base);

        base >>= 2;
    } while (base);

    return result;
}

uint64_t Math_Sqrt64(uint64_t n)
{
    uint64_t result = 0;
    uint64_t bit = 1ULL << 62;

    while (bit > n) {
        bit >>= 2;
    }

    while (bit != 0) {
        if (n >= result + bit) {
            n -= result + bit;
            result = (result >> 1) + bit;
        } else {
            result >>= 1;
        }
        bit >>= 2;
    }

    return result;
}

void Math_GetVectorAngles(
    const int32_t x, const int32_t y, const int32_t z, int16_t *const dest)
{
    dest[0] = XYZ_32_GetYaw((XYZ_32) { x, y, z });
    dest[1] = XYZ_32_GetPitch((XYZ_32) { x, y, z });
}

int32_t Math_AngleInCone(int32_t angle1, int32_t angle2, int32_t cone)
{
    const int32_t diff = ((int)(angle1 - angle2 + DEG_180)) % DEG_360 - DEG_180;
    return ABS(diff) < cone;
}

DIRECTION Math_GetDirection(const int16_t angle)
{
    return (uint16_t)(angle + DEG_45) / DEG_90;
}

DIRECTION Math_GetDirectionCone(const int16_t angle, const int16_t cone)
{
    if (angle >= -cone && angle <= cone) {
        return DIR_NORTH;
    } else if (angle >= DEG_90 - cone && angle <= DEG_90 + cone) {
        return DIR_EAST;
    } else if (angle >= DEG_180 - cone || angle <= -DEG_180 + cone) {
        return DIR_SOUTH;
    } else if (angle >= -DEG_90 - cone && angle <= -DEG_90 + cone) {
        return DIR_WEST;
    }
    return DIR_UNKNOWN;
}

int16_t Math_DirectionToAngle(const DIRECTION dir)
{
    switch (dir) {
    case DIR_NORTH:
        return 0;
    case DIR_EAST:
        return DEG_90;
    case DIR_SOUTH:
        return -DEG_180;
    case DIR_WEST:
        return -DEG_90;
    default:
        return 0;
    }
}

int32_t Math_AngleMean(int32_t angle1, int32_t angle2, double ratio)
{
    int32_t diff = angle2 - angle1;

    if (diff > DEG_180) {
        diff -= DEG_360;
    } else if (diff < -DEG_180) {
        diff += DEG_360;
    }

    int32_t result = angle1 + diff * ratio;

    result %= DEG_360;
    if (result < 0) {
        result += DEG_360;
    }

    return result;
}

int32_t Math_FloorDiv(const int32_t x, const int32_t divisor)
{
    return (x >= 0) ? x / divisor : -((-x + divisor - 1) / divisor);
}

int32_t Math_GCD(int32_t a, int32_t b)
{
    while (b != 0) {
        int32_t t = b;
        b = a % b;
        a = t;
    }
    return a;
}

int16_t XYZ_32_GetYaw(const XYZ_32 pos)
{
    return Math_Atan(pos.z, pos.x);
}

int16_t XYZ_32_GetPitch(XYZ_32 pos)
{
    // make sure SQUARE() doesn't get out of bounds
    while ((int16_t)pos.x != pos.x || (int16_t)pos.y != pos.y
           || (int16_t)pos.z != pos.z) {
        pos.x >>= 1;
        pos.y >>= 1;
        pos.z >>= 1;
    }
    return Math_Atan(Math_Sqrt(SQUARE(pos.x) + SQUARE(pos.z)), -pos.y);
}

int32_t XYZ_32_GetDistance(const XYZ_32 *const pos1, const XYZ_32 *const pos2)
{
    int64_t x = (int64_t)pos1->x - pos2->x;
    int64_t y = (int64_t)pos1->y - pos2->y;
    int64_t z = (int64_t)pos1->z - pos2->z;

    int32_t scale = 0;
    while ((int32_t)x != x || (int32_t)y != y || (int32_t)z != z) {
        scale++;
        x >>= 1;
        y >>= 1;
        z >>= 1;
    }

    const uint64_t dist = Math_Sqrt64(
        SQUARE((uint64_t)ABS(x)) + SQUARE((uint64_t)ABS(y))
        + SQUARE((uint64_t)ABS(z)));
    if (dist > ((uint64_t)INT32_MAX >> scale)) {
        return INT32_MAX;
    }
    return (int32_t)(dist << scale);
}

bool XYZ_32_IsNearby(
    const XYZ_32 *const pos1, const XYZ_32 *const pos2, const int32_t distance)
{
    const XYZ_32 delta = {
        .x = pos1->x - pos2->x,
        .y = pos1->y - pos2->y,
        .z = pos1->z - pos2->z,
    };
    return delta.x > -distance && delta.x < distance && delta.y > -distance
        && delta.y < distance && delta.z > -distance && delta.z < distance;
}

int32_t XYZ_32_GetLength(const XYZ_32 pos)
{
    return Math_Sqrt(SQUARE(pos.x) + SQUARE(pos.y) + SQUARE(pos.z));
}

int64_t XYZ_32_GetLength2_64(const XYZ_32 pos)
{
    return SQUARE((int64_t)pos.x) + SQUARE((int64_t)pos.y)
        + SQUARE((int64_t)pos.z);
}

bool XYZ_32_AreEquivalent(const XYZ_32 *const pos1, const XYZ_32 *const pos2)
{
    return pos1->x == pos2->x && pos1->y == pos2->y && pos1->z == pos2->z;
}

bool XYZ_16_AreEquivalent(const XYZ_16 *const rot1, const XYZ_16 *const rot2)
{
    return rot1->x == rot2->x && rot1->y == rot2->y && rot1->z == rot2->z;
}

XYZ_32 XYZ_32_From16(const XYZ_16 src)
{
    return (XYZ_32) { src.x, src.y, src.z };
}

XYZ_32 XYZ_32_OffsetYaw(
    const XYZ_32 src, const int16_t yaw, const int32_t distance)
{
    return (XYZ_32) {
        .x = src.x + ((distance * Math_Sin(yaw)) >> W2V_SHIFT),
        .y = src.y,
        .z = src.z + ((distance * Math_Cos(yaw)) >> W2V_SHIFT),
    };
}

XYZ_32 XYZ_32_FromYawPitch(
    const int16_t yaw, const int16_t pitch, const int32_t distance)
{
    const int32_t cx = Math_Cos(pitch);
    const int32_t sx = Math_Sin(pitch);
    const int32_t cy = Math_Cos(yaw);
    const int32_t sy = Math_Sin(yaw);

    const int32_t horz = (distance * cx) >> W2V_SHIFT;
    return (XYZ_32) {
        .x = (int32_t)(((int64_t)horz * sy) >> W2V_SHIFT),
        .y = -(int32_t)(((int64_t)distance * sx) >> W2V_SHIFT),
        .z = (int32_t)(((int64_t)horz * cy) >> W2V_SHIFT),
    };
}

int64_t XYZ_32_DotProduct_64(const XYZ_32 a, const XYZ_32 b)
{
    return (int64_t)a.x * b.x + (int64_t)a.y * b.y + (int64_t)a.z * b.z;
}

bool XYZ_32_ProjectPointOntoAxis(
    const XYZ_32 origin, const XYZ_32 axis, const int64_t axis_len2,
    XYZ_32 *const pos)
{
    // Finds the value `t` such that the point
    //   origin + t * axis
    // is the closest point on the line to the original *pos, and then writes
    // that point back into *pos.
    //
    // Example:
    // - origin = (0, 0, 0)
    // - axis   = (1, 0, 0)  // line is the X axis
    // - *pos   = (5, 2, -3)
    // The closest point on the X axis is (5, 0, 0), so after the call:
    // - *pos = (5, 0, 0)

    if (axis_len2 == 0) {
        return false;
    }

    const XYZ_32 offset = {
        .x = pos->x - origin.x,
        .y = pos->y - origin.y,
        .z = pos->z - origin.z,
    };

    const int64_t t_num = XYZ_32_DotProduct_64(offset, axis);
    *pos = (XYZ_32) {
        .x = origin.x + (int32_t)((t_num * axis.x) / axis_len2),
        .y = origin.y + (int32_t)((t_num * axis.y) / axis_len2),
        .z = origin.z + (int32_t)((t_num * axis.z) / axis_len2),
    };
    return true;
}

float XYZ_F_DotProduct(const XYZ_F a, const XYZ_F b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

float XYZ_F_Length2(const XYZ_F pos)
{
    return XYZ_F_DotProduct(pos, pos);
}

float XYZ_F_Length(const XYZ_F pos)
{
    return sqrtf(XYZ_F_Length2(pos));
}

XYZ_F XYZ_F_Subtract(const XYZ_F a, const XYZ_F b)
{
    return (XYZ_F) {
        .x = a.x - b.x,
        .y = a.y - b.y,
        .z = a.z - b.z,
    };
}

bool Bounds32_Intersect(const BOUNDS_32 *const a, const BOUNDS_32 *const b)
{
    return !(
        a->min.x > b->max.x || a->max.x < b->min.x || a->min.y > b->max.y
        || a->max.y < b->min.y || a->min.z > b->max.z || a->max.z < b->min.z);
}
