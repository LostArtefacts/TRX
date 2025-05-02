#include "game/items/col.h"

#include "game/objects.h"
#include "game/rooms.h"
#include "utils.h"

static BOUNDS_16 m_NullBounds = {};
static BOUNDS_16 m_InterpolatedBounds = {};

int16_t Item_GetHeight(const ITEM *const item)
{
    int16_t room_num = item->room_num;
    const SECTOR *const sector =
        Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);
    const int32_t height =
        Room_GetHeight(sector, item->pos.x, item->pos.y, item->pos.z);

    return height;
}

int32_t Item_GetDistance(const ITEM *const item, const XYZ_32 *const target)
{
    return XYZ_32_GetDistance(&item->pos, target);
}

void Item_Translate(
    ITEM *const item, const int32_t x, const int32_t y, const int32_t z)
{
    const int32_t c = Math_Cos(item->rot.y);
    const int32_t s = Math_Sin(item->rot.y);
    item->pos.x += ((c * x + s * z) >> W2V_SHIFT);
    item->pos.y += y;
    item->pos.z += ((c * z - s * x) >> W2V_SHIFT);
}

bool Item_Test3DRange(
    const int32_t x, const int32_t y, const int32_t z, const int32_t range)
{
    return ABS(x) < range && ABS(y) < range && ABS(z) < range
        && (SQUARE(x) + SQUARE(y) + SQUARE(z) < SQUARE(range));
}

bool Item_TestBoundsCollide(
    const ITEM *const src_item, const ITEM *const dst_item,
    const int32_t radius)
{
    const BOUNDS_16 *const src_bounds = &Item_GetBestFrame(src_item)->bounds;
    const BOUNDS_16 *const dst_bounds = &Item_GetBestFrame(dst_item)->bounds;

    if (src_item->pos.y + src_bounds->min.y
            >= dst_item->pos.y + dst_bounds->max.y
        || src_item->pos.y + src_bounds->max.y
            <= dst_item->pos.y + dst_bounds->min.y) {
        return false;
    }

    const int32_t c = Math_Cos(src_item->rot.y);
    const int32_t s = Math_Sin(src_item->rot.y);
    const int32_t dx = dst_item->pos.x - src_item->pos.x;
    const int32_t dz = dst_item->pos.z - src_item->pos.z;
    const int32_t rx = (c * dx - s * dz) >> W2V_SHIFT;
    const int32_t rz = (c * dz + s * dx) >> W2V_SHIFT;

    // clang-format off
    return (
        rx >= src_bounds->min.x - radius &&
        rx <= src_bounds->max.x + radius &&
        rz >= src_bounds->min.z - radius &&
        rz <= src_bounds->max.z + radius);
    // clang-format on
}

const BOUNDS_16 *Item_GetBoundsAccurate(const ITEM *const item)
{
    int32_t rate;
    ANIM_FRAME *frames[2];
    const int32_t frac = Item_GetFrames(item, frames, &rate);
    if (frames[0] == nullptr) {
        return &m_NullBounds;
    }

    if (frac == 0) {
        return &frames[0]->bounds;
    }

#define CALC(target, b1, b2, prop)                                             \
    target->prop = (b1)->prop + ((((b2)->prop - (b1)->prop) * frac) / rate);
    BOUNDS_16 *const result = &m_InterpolatedBounds;
    CALC(result, &frames[0]->bounds, &frames[1]->bounds, min.x);
    CALC(result, &frames[0]->bounds, &frames[1]->bounds, max.x);
    CALC(result, &frames[0]->bounds, &frames[1]->bounds, min.y);
    CALC(result, &frames[0]->bounds, &frames[1]->bounds, max.y);
    CALC(result, &frames[0]->bounds, &frames[1]->bounds, min.z);
    CALC(result, &frames[0]->bounds, &frames[1]->bounds, max.z);
#undef CALC

    return result;
}
