#include <libtrx/game/gun/misc.h>
#include <libtrx/game/gun/vars.h>
#include <libtrx/game/lara.h>
#include <libtrx/game/math.h>
#include <libtrx/game/objects/vars.h>
#include <libtrx/game/rooms.h>
#include <libtrx/game/spawn.h>
#include <libtrx/game/stats.h>

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    const XYZ_32 old_pos = item->pos;

    if (!(Room_Get(item->room_num)->flags & RF_UNDERWATER)) {
        item->fall_speed += GRAVITY / 2;
    }

    item->pos.x += (item->speed * Math_Sin(item->rot.y)) >> W2V_SHIFT;
    item->pos.z += (item->speed * Math_Cos(item->rot.y)) >> W2V_SHIFT;
    item->pos.y += item->fall_speed;

    int16_t room_num = item->room_num;
    const SECTOR *const sector =
        Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);
    item->floor = Room_GetHeight(sector, item->pos.x, item->pos.y, item->pos.z);
    Item_UpdateRoom(item_num, room_num);

    bool hit = false;
    if (Gun_SmashItems(old_pos, item->pos, nullptr) == PROJECTILE_HIT_STOP) {
        hit = true;
    }

    for (int16_t target_num = Room_Get(item->room_num)->item_num;
         target_num != NO_ITEM; target_num = Item_Get(target_num)->next_item) {
        ITEM *const target_item = Item_Get(target_num);
        const OBJECT *const target_obj = Object_Get(target_item->object_id);

        if (target_item == Lara_GetItem() || item_num == target_num) {
            continue;
        }

        if (!target_item->collidable) {
            continue;
        }

        if (!Creature_IsTargetable(target_item)) {
            continue;
        }

        const ANIM_FRAME *const frame = Item_GetBestFrame(target_item);
        if (frame == nullptr) {
            continue;
        }
        const BOUNDS_16 *const bounds = &frame->bounds;

        const int32_t cdy = item->pos.y - target_item->pos.y;
        if (cdy < bounds->min.y || cdy > bounds->max.y) {
            continue;
        }

        const int32_t cy = Math_Cos(target_item->rot.y);
        const int32_t sy = Math_Sin(target_item->rot.y);
        const int32_t cdx = item->pos.x - target_item->pos.x;
        const int32_t cdz = item->pos.z - target_item->pos.z;
        const int32_t odx = old_pos.x - target_item->pos.x;
        const int32_t odz = old_pos.z - target_item->pos.z;

        const int32_t rx = (cy * cdx - sy * cdz) >> W2V_SHIFT;
        const int32_t sx = (cy * odx - sy * odz) >> W2V_SHIFT;
        if ((rx < bounds->min.x && sx < bounds->min.x)
            || (rx > bounds->max.x && sx > bounds->max.x)) {
            continue;
        }

        const int32_t rz = (sy * cdx + cy * cdz) >> W2V_SHIFT;
        const int32_t sz = (sy * odx + cy * odz) >> W2V_SHIFT;
        if ((rz < bounds->min.z && sz < bounds->min.z)
            || (rz > bounds->max.z && sz > bounds->max.z)) {
            continue;
        }

        if (target_obj->intelligent && target_item->status == IS_ACTIVE) {
            Spawn_BloodBath(
                item->pos.x, item->pos.y, item->pos.z, 0, 0, item->room_num, 5);
            Gun_HitTarget(target_item, nullptr, g_Weapons[LGT_HARPOON].damage);
            Stats_AddAmmoHits();
        }
        hit = true;
        break;
    }

    if (!hit) {
        const int32_t ceiling =
            Room_GetCeiling(sector, item->pos.x, item->pos.y, item->pos.z);
        if (item->pos.y >= item->floor || item->pos.y <= ceiling) {
            hit = true;
        }
    }

    if (hit) {
        Item_Kill(item_num);
    } else if (Room_Get(item->room_num)->flags & RF_UNDERWATER) {
        Spawn_Bubble(&item->pos, item->room_num);
    }
}

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->save_position = true;
}

REGISTER_OBJECT(O_HARPOON_BOLT, M_Setup)
