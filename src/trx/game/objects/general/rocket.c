#include <trx/game/creature.h>
#include <trx/game/effects.h>
#include <trx/game/gun/misc.h>
#include <trx/game/gun/vars.h>
#include <trx/game/lara.h>
#include <trx/game/math.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>
#include <trx/game/stats.h>
#include <trx/version.h>

#define M_BLAST_RADIUS WALL_L // = 1024
#define M_SPEED (WALL_L / 2) // = 512
#define M_SPEED_UW (STEP_L / 2) // = 128

static void M_Explode(const int16_t rocket_item_num, const XYZ_32 pos)
{
    const ITEM *const rocket_item = Item_Get(rocket_item_num);
    const int16_t effect_num = Effect_Create(rocket_item->room_num);
    if (effect_num != NO_EFFECT) {
        EFFECT *const effect = Effect_Get(effect_num);
        effect->pos = pos;
        effect->speed = 0;
        effect->frame_num = 0;
        effect->counter = 0;
        effect->object_id = O_EXPLOSION_1;
    }

    const XYZ_32 *const sfx_pos =
        g_TRVersion == 3 ? &rocket_item->pos : nullptr;
    Sound_Effect(SFX_EXPLOSION_1, sfx_pos, 0x1800000 | SPM_PITCH);
    Sound_Effect(SFX_EXPLOSION_2, sfx_pos, SPM_NORMAL);
    Item_Kill(rocket_item_num);
}

static bool M_CanExplodeTarget(const ITEM *const item)
{
    const OBJECT *const object = Object_Get(item->object_id);
    const ITEM_ACTION action = ItemAction_ToGameID(ITEM_ACTION_FINISH_LEVEL);
    for (int32_t i = 0; i < object->anim_count; i++) {
        const ANIM *const anim = Object_GetAnim(object, i);
        if (Anim_HasFXCommand(anim, action)) {
            return false;
        }
    }

    return true;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    const XYZ_32 old_pos = item->pos;

    const ROOM *const room = Room_Get(item->room_num);
    if (room->flags.underwater) {
        if (item->speed < M_SPEED_UW) {
            item->speed += (item->speed >> 2) + 4;
            CLAMPG(item->speed, M_SPEED_UW);
        } else {
            item->speed -= item->speed >> 2;
        }
        item->rot.z += DEG_1 * ((item->speed >> 3) + 3);
    } else {
        if (item->speed < M_SPEED) {
            item->speed += (item->speed >> 2) + 4;
        }
        const int16_t rot = item->rot.z;
        item->rot.z += DEG_1 * ((item->speed >> 3) + 7);
    }

    // TODO: dynamics

    const int32_t speed = (item->speed * Math_Cos(item->rot.x)) >> W2V_SHIFT;
    item->pos.x += (speed * Math_Sin(item->rot.y)) >> W2V_SHIFT;
    item->pos.y -= (item->speed * Math_Sin(item->rot.x)) >> W2V_SHIFT;
    item->pos.z += (speed * Math_Cos(item->rot.y)) >> W2V_SHIFT;

    int16_t room_num = item->room_num;
    const SECTOR *const sector =
        Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);
    item->floor = Room_GetHeight(sector, item->pos.x, item->pos.y, item->pos.z);
    Item_UpdateRoom(item_num, room_num);

    bool explode = false;
    int32_t radius = 0;
    if (item->pos.y >= item->floor
        || item->pos.y
            <= Room_GetCeiling(sector, item->pos.x, item->pos.y, item->pos.z)) {
        radius = M_BLAST_RADIUS;
        explode = true;
    }

    if (Gun_SmashItems(old_pos, item->pos, nullptr) == PROJECTILE_HIT_STOP) {
        explode = true;
    }

    for (int16_t target_item_num = Room_Get(item->room_num)->item_num;
         target_item_num != NO_ITEM;
         target_item_num = Item_Get(target_item_num)->next_item) {
        ITEM *const target_item = Item_Get(target_item_num);
        const OBJECT *const target_obj = Object_Get(target_item->object_id);
        if (target_item == Lara_GetItem()) {
            continue;
        }
        if (!target_item->collidable) {
            continue;
        }

        if (target_item->status == IS_INVISIBLE
            || target_obj->collision_func == nullptr) {
            continue;
        }

        if (!Creature_IsTargetable(target_item)
            && !Creature_IsFloating(target_item)) {
            continue;
        }

        const ANIM_FRAME *const frame = Item_GetBestFrame(target_item);
        const BOUNDS_16 *const bounds = &frame->bounds;

        const int32_t cdy = item->pos.y - target_item->pos.y;
        if (cdy + radius < bounds->min.y || cdy - radius > bounds->max.y) {
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
        if ((rx + radius < bounds->min.x && sx + radius < bounds->min.x)
            || (rx - radius > bounds->max.x && sx - radius > bounds->max.x)) {
            continue;
        }

        const int32_t rz = (sy * cdx + cy * cdz) >> W2V_SHIFT;
        const int32_t sz = (sy * odx + cy * odz) >> W2V_SHIFT;
        if ((rz + radius < bounds->min.z && sz + radius < bounds->min.z)
            || (rz - radius > bounds->max.z && sz - radius > bounds->max.z)) {
            continue;
        }

        explode = true;

        if (target_item->status != IS_ACTIVE) {
            continue;
        }

        Gun_HitTarget(
            target_item, nullptr, nullptr, g_Weapons[LGT_ROCKET].damage);
        Stats_AddAmmoHits();

        if (target_item->hit_points <= 0 && M_CanExplodeTarget(target_item)) {
            Creature_Die(target_item_num, true);
        }
    }

    if (explode) {
        M_Explode(item_num, old_pos);
    }
}

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->save_position = true;
}

REGISTER_OBJECT(O_ROCKET, M_Setup)
