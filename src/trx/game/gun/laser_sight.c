#include <trx/game/gun/laser_sight.h>

#include <trx/game/collision/los.h>
#include <trx/game/fx/laser_sight.h>
#include <trx/game/gun/misc.h>
#include <trx/game/gun/registry.h>
#include <trx/game/gun/smashing.h>
#include <trx/game/items.h>
#include <trx/game/objects/families.h>
#include <trx/game/rooms.h>
#include <trx/game/spawn.h>
#include <trx/game/stats.h>

// The share of the line that the dot stands back from where the line ends,
// so that it does not sink into what it is on.
#define M_DOT_PULL_SHIFT 5

// Returns the creature nearest the start that the line meets before it ends,
// or nullptr where it meets none, and shortens the line to where it meets
// the creature.
static ITEM *M_FindCreature(const GAME_VECTOR start, GAME_VECTOR *const end)
{
    ITEM *best = nullptr;
    int32_t best_dist = INT32_MAX;
    for (int16_t item_num = 0; item_num < Item_GetTotalCount(); item_num++) {
        ITEM *const item = Item_Get(item_num);
        if (item->is_finished || item->hit_points <= 0 || !Item_IsInPlay(item)
            || !ObjectFamily_Has(item->object_id, OBJ_FAMILY_CREATURE)) {
            continue;
        }
        GAME_VECTOR hit = *end;
        if (!LOS_CheckItemIntersectSegment(&start, &hit, item)) {
            continue;
        }
        const int32_t dist = XYZ_32_GetDistance(start.pos, hit.pos);
        if (dist < best_dist) {
            best_dist = dist;
            best = item;
            *end = hit;
        }
    }
    return best;
}

static XYZ_32 M_PullBack(const XYZ_32 start, const XYZ_32 end)
{
    return (XYZ_32) {
        .x = end.x - ((end.x - start.x) >> M_DOT_PULL_SHIFT),
        .y = end.y - ((end.y - start.y) >> M_DOT_PULL_SHIFT),
        .z = end.z - ((end.z - start.z) >> M_DOT_PULL_SHIFT),
    };
}

void Gun_LaserSight_Aim(
    const LARA_GUN_TYPE gun_type, const GAME_VECTOR start, GAME_VECTOR target,
    const bool fire)
{
    const bool reaches = LOS_Check(&start, &target, true);
    ITEM *const creature = M_FindCreature(start, &target);
    Room_GetSector(target.pos, &target.room_num);

    if (fire && !Gun_ShootStatic(start, target)) {
        if (Gun_SmashItems(start, target, &target.pos, NO_OBJECT)
            == PROJECTILE_HIT_STOP) {
            Room_GetSector(target.pos, &target.room_num);
        } else if (creature != nullptr) {
            Stats_AddAmmoHits();
            Gun_HitTarget(
                creature, &start, &target, Gun_Registry_Get(gun_type)->damage);
        } else if (!reaches) {
            Spawn_RicochetRay(start, target, 8, false);
        }
    }

    FX_LaserSight_SetDot(M_PullBack(start.pos, target.pos));
}
