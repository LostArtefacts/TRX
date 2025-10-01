#include "game/los.h"

#include "game/items.h"
#include "game/objects.h"
#include "utils.h"

#include <math.h>

// This routine transforms the world-space LOS segment [start,target] into the
// object's local coordinates (undoing its translation and Y-rotation), then
// performs a slab intersection test against that local AABB. The first
// smashable item hit is returned, or NO_ITEM if none.
//
// (AABB = Axis-Aligned Bounding Box. It's the rectangular box defined by
// bounds->min/max along X,Y,Z in the object's local space (no rotation).)
//
// @param start  World-space ray origin
// @param target World-space ray end
// @param item   Item to check
// @return       Whether the item collides with the segment
static bool M_ItemIntersectSegment(
    const XYZ_32 start, const XYZ_32 target, const ITEM *const item,
    XYZ_32 *const out_hit_pos)
{
    const double dx = target.x - start.x;
    const double dy = target.y - start.y;
    const double dz = target.z - start.z;

    // Translate into object-local space
    const double ox = start.x - item->pos.x;
    const double oy = start.y - item->pos.y;
    const double oz = start.z - item->pos.z;

    // Unrotate by -rot.y around Y axis
    const float c = cosf(-item->rot.y * M_PI / DEG_180);
    const float s = sinf(-item->rot.y * M_PI / DEG_180);
    const double lx = ox * c + oz * s;
    const double ly = oy;
    const double lz = oz * c - ox * s;
    const double ldx = dx * c + dz * s;
    const double ldy = dy;
    const double ldz = dz * c - dx * s;

    // Local AABB extents from item's bounds
    const BOUNDS_16 *const orig_bounds = Item_GetBoundsAccurate(item);
    BOUNDS_16 patched_bounds = *orig_bounds;

    const BOUNDS_16 *const bounds = &patched_bounds;

    // Parametric interval [t0..t1] in Q14 fixed-point
    double t0 = 0.0;
    double t1 = 1.0;

    // X slab
    if (ldx != 0) {
        double t_near = (double)(bounds->min.x - lx) / ldx;
        double t_far = (double)(bounds->max.x - lx) / ldx;
        if (t_near > t_far) {
            SWAP(t_near, t_far);
        }
        if (t_near > t1 || t_far < t0) {
            return false;
        }
        CLAMPL(t0, t_near);
        CLAMPG(t1, t_far);
    } else if (lx < bounds->min.x || lx > bounds->max.x) {
        return false;
    }

    // Y slab
    if (ldy != 0) {
        double t_near = (double)(bounds->min.y - ly) / ldy;
        double t_far = (double)(bounds->max.y - ly) / ldy;
        if (t_near > t_far) {
            SWAP(t_near, t_far);
        }
        if (t_near > t1 || t_far < t0) {
            return false;
        }
        CLAMPL(t0, t_near);
        CLAMPG(t1, t_far);
    } else if (ly < bounds->min.y || ly > bounds->max.y) {
        return false;
    }

    // Z slab
    if (ldz != 0) {
        double t_near = (double)(bounds->min.z - lz) / ldz;
        double t_far = (double)(bounds->max.z - lz) / ldz;
        if (t_near > t_far) {
            SWAP(t_near, t_far);
        }
        if (t_near > t1 || t_far < t0) {
            return false;
        }
        CLAMPL(t0, t_near);
        CLAMPG(t1, t_far);
    } else if (lz < bounds->min.z || lz > bounds->max.z) {
        return false;
    }

    if (out_hit_pos != nullptr) {
        // world-space hit position = start + t0 * (target-start)
        out_hit_pos->x = start.x + t0 * dx;
        out_hit_pos->y = start.y + t0 * dy;
        out_hit_pos->z = start.z + t0 * dz;
    }

    return true;
}

// Smashes all objects along the ray path.
//
// @param start  World-space ray origin
// @param target World-space ray end
// @return       First smashable item's index, or NO_ITEM if none hit
int32_t LOS_CheckSmashable(
    const XYZ_32 start, const XYZ_32 target, XYZ_32 *const out_hit_pos)
{
    int32_t best_dist = INT32_MAX;
    int16_t best_item_num = NO_ITEM;

    for (int16_t item_num = 0; item_num < Item_GetTotalCount(); item_num++) {
        const ITEM *const item = Item_Get(item_num);
        if (item->status == IS_DEACTIVATED) {
            continue;
        }
        if (!Object_IsType(item->object_id, g_SmashableObjects)
            && !Object_IsType(item->object_id, g_ShatterableObjects)) {
            continue;
        }

        XYZ_32 hit_pos;
        if (!M_ItemIntersectSegment(start, target, item, &hit_pos)) {
            continue;
        }

        // Ray segment intersects the object's local AABB
        const int32_t dist = Item_GetDistance(item, &start);
        if (dist < best_dist) {
            best_dist = dist;
            best_item_num = item_num;
            if (out_hit_pos != nullptr) {
                *out_hit_pos = hit_pos;
            }
        }
    }

    return best_item_num;
}
