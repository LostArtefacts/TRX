#include <trx/config.h>
#include <trx/core/math.h>
#include <trx/core/utils.h>
#include <trx/game/creature.h>
#include <trx/game/effects.h>
#include <trx/game/fx/water.h>
#include <trx/game/gun/common.h>
#include <trx/game/gun/misc.h>
#include <trx/game/gun/registry.h>
#include <trx/game/gun/smashing.h>
#include <trx/game/items.h>
#include <trx/game/lara.h>
#include <trx/game/objects/families.h>
#include <trx/game/output/lights.h>
#include <trx/game/output/state.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>
#include <trx/game/sparks.h>
#include <trx/game/spawn.h>
#include <trx/game/stats.h>
#include <trx/version.h>

#define M_BLAST_RADIUS WALL_L // = 1024
#define M_SPEED (WALL_L / 2) // = 512
#define M_SPEED_UW (STEP_L / 2) // = 128

static void M_SetTR3ProjectileShade(ITEM *const item)
{
    if (item == nullptr) {
        return;
    }

    // OG TR3 uses `item->shade = -0x3DF0` on projectiles; in TRX any negative
    // shade forces the dynamic/smoothed lighting path.
    item->shade.value_1 = -1;
    item->shade.value_2 = -1;
}

static void M_Explode(const int16_t rocket_item_num, const XYZ_32 pos)
{
    const ITEM *const rocket_item = Item_Get(rocket_item_num);
    const ROOM *const room = Room_Get(rocket_item->room_num);
    const bool is_underwater = room != nullptr && room->flags.underwater;

    if (g_TRVersion == 3) {
        if (is_underwater) {
            Sparks_TriggerUnderwaterExplosion(rocket_item);
        } else {
            Sparks_TriggerExplosionSparks(pos, 3, -2, 0, rocket_item->room_num);
            for (int32_t i = 0; i < 2; i++) {
                Sparks_TriggerExplosionSparks(
                    pos, 3, -1, 0, rocket_item->room_num);
            }
        }
    } else {
        const int16_t effect_num = Effect_Create(rocket_item->room_num);
        if (effect_num != NO_EFFECT) {
            EFFECT *const effect = Effect_Get(effect_num);
            effect->pos = pos;
            effect->speed = 0;
            effect->frame_num = 0;
            effect->counter = 0;
            effect->object_id = O_EXPLOSION_1;
        }
    }

    const XYZ_32 *const sfx_pos =
        g_TRVersion >= 3 ? &rocket_item->pos : nullptr;
    const uint32_t flags =
        g_TRVersion >= 3 ? (0x1800000 | SPM_PITCH) : SPM_NORMAL;
    Sound_Effect(SFX_EXPLOSION_1, sfx_pos, flags);
    Sound_Effect(SFX_EXPLOSION_2, sfx_pos, SPM_NORMAL);
    Item_Destroy(rocket_item_num);

    Creature_AlertNearbyGuards(rocket_item);
}

static bool M_CanExplodeTarget(const ITEM *const item)
{
    const OBJECT *const object = Object_Get(item->object_id);
    if (object->can_be_exploded_func != nullptr) {
        return object->can_be_exploded_func(item);
    }

    const ITEM_ACTION_SLOT action =
        ItemAction_IDToSlot(ITEM_ACTION_FINISH_LEVEL);
    for (int32_t i = 0; i < object->anim_count; i++) {
        const ANIM *const anim = Object_GetAnim(object, i);
        if (Anim_HasFXCommand(anim, action)) {
            return false;
        }
    }

    return true;
}

static bool M_TryExplodeItem(
    const ITEM *const projectile_item, const GAME_VECTOR old_pos,
    const int16_t target_item_num, const int32_t radius)
{
    ITEM *const target_item = Item_Get(target_item_num);
    const OBJECT *const target_obj = Object_Get(target_item->object_id);
    if (target_item == Lara_GetItem()) {
        return false;
    }
    if (!target_item->is_collidable) {
        return false;
    }

    if (!target_item->is_visible || target_obj->collision_func == nullptr) {
        return false;
    }

    if (!Item_CanBeProjectileTarget(target_item)) {
        return false;
    }

    const ANIM_FRAME *const frame = Item_GetBestFrame(target_item);
    const BOUNDS_16 *const bounds = &frame->bounds;

    const int32_t cdy = projectile_item->pos.y - target_item->pos.y;
    if (cdy + radius < bounds->min.y || cdy - radius > bounds->max.y) {
        return false;
    }

    const XYZ_32 now = XYZ_32_UnrotateYaw(
        XYZ_32_Subtract(projectile_item->pos, target_item->pos),
        target_item->rot.y);
    const XYZ_32 old = XYZ_32_UnrotateYaw(
        XYZ_32_Subtract(old_pos.pos, target_item->pos), target_item->rot.y);

    if ((now.x + radius < bounds->min.x && old.x + radius < bounds->min.x)
        || (now.x - radius > bounds->max.x && old.x - radius > bounds->max.x)) {
        return false;
    }

    if ((now.z + radius < bounds->min.z && old.z + radius < bounds->min.z)
        || (now.z - radius > bounds->max.z && old.z - radius > bounds->max.z)) {
        return false;
    }

    if (!Item_CanTakeDamage(target_item)) {
        return false;
    }

    const GAME_VECTOR hit_pos = {
        .pos = projectile_item->pos,
        .room_num = projectile_item->room_num,
    };
    Gun_HitTarget(
        target_item, &old_pos, &hit_pos,
        Gun_Registry_Get(Gun_GetTypeForProjectile(projectile_item->object_id))
            ->damage);
    Stats_AddAmmoHits();

    if (Gun_GetSmashPolicy(target_item) == GUN_SMASH_POLICY_HEAVY) {
        if (ObjectFamily_Has(
                projectile_item->object_id, OBJ_FAMILY_HEAVY_MISSILE)) {
            Gun_SmashItem(target_item_num);
        }
    } else if (Gun_GetSmashPolicy(target_item) != GUN_SMASH_POLICY_NONE) {
        Gun_SmashItem(target_item_num);
    } else if (
        target_item->hit_points <= 0 && M_CanExplodeTarget(target_item)) {
        Creature_Die(target_item_num, true);
    }
    return true;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    const GAME_VECTOR old_pos = {
        .pos = item->pos,
        .room_num = item->room_num,
    };

    const ROOM *const room = Room_Get(item->room_num);
    const bool was_underwater = room != nullptr && room->flags.underwater;
    if (was_underwater) {
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
        item->rot.z += DEG_1 * ((item->speed >> 2) + 7);
    }

    if (g_TRVersion == 3) {
        M_SetTR3ProjectileShade(item);

        const XYZ_32 back_128 =
            XYZ_32_FromYawPitch(item->rot.y, item->rot.x, -128);
        const int32_t back_dist = -1536 - (Random_GetControl() & 0x1FF);
        const XYZ_32 back_vel =
            XYZ_32_FromYawPitch(item->rot.y, item->rot.x, back_dist);

        const int32_t time4 = Output_GetTimeInGame() * 4;
        if ((time4 & 4) != 0) {
            Sparks_TriggerRocketFlame(
                back_128, XYZ_32_Subtract(back_vel, back_128), item_num,
                item->room_num);
        }

        Sparks_TriggerRocketSmoke(
            (XYZ_32) {
                .x = item->pos.x + back_128.x,
                .y = item->pos.y + back_128.y,
                .z = item->pos.z + back_128.z,
            },
            -1, item->room_num);

        if (was_underwater) {
            const XYZ_32 bubble_pos = {
                .x = item->pos.x + back_128.x,
                .y = item->pos.y + back_128.y,
                .z = item->pos.z + back_128.z,
            };
            Spawn_BubbleEx(&bubble_pos, item->room_num, 4, 8);
        }

        if (g_Config.visuals.enable_gun_lighting) {
            const int32_t rnd = Random_GetControl();
            const XYZ_32 light_pos = {
                .x = item->pos.x + back_128.x + (rnd & 0xF) - 8,
                .y = item->pos.y + back_128.y + ((rnd >> 4) & 0xF) - 8,
                .z = item->pos.z + back_128.z + ((rnd >> 8) & 0xF) - 8,
            };
            const int32_t c = Random_GetControl();
            const RGB_888 color = {
                .r = (uint8_t)((c & 0x1F) + 224),
                .g = (uint8_t)(((c >> 5) & 0x3F) + 128),
                .b = (uint8_t)((c >> 11) & 0x3F),
            };
            Output_AddDynamicLightRGB(light_pos, 14, color);
        }
    }

    item->pos = XYZ_32_Add(
        item->pos, XYZ_32_FromYawPitch(item->rot.y, item->rot.x, item->speed));

    int16_t room_num = item->room_num;
    const SECTOR *const sector = Room_GetSector(item->pos, &room_num);
    item->floor = Room_GetHeight(sector, item->pos);
    Item_UpdateRoom(item_num, room_num);

    if (g_TRVersion == 3) {
        const ROOM *const new_room = Room_Get(item->room_num);
        const bool is_underwater =
            new_room != nullptr && new_room->flags.underwater;
        if (is_underwater && !was_underwater) {
            FX_Water_SetupSplash(&(FX_WATER_SPLASH_SETUP) {
                .pos = { .x = item->pos.x,
                         .y = new_room->max_ceiling,
                         .z = item->pos.z },
                .inner_xz_off = 16,
                .inner_xz_size = 12,
                .inner_y_size = -96,
                .inner_xz_vel = 160,
                .inner_y_vel = -0x4000,
                .inner_gravity = 128,
                .inner_friction = 7,
                .middle_xz_off = 24,
                .middle_xz_size = 24,
                .middle_y_size = -64,
                .middle_xz_vel = 224,
                .middle_y_vel = -0x2000,
                .middle_gravity = 72,
                .middle_friction = 8,
                .outer_xz_off = 32,
                .outer_xz_size = 32,
                .outer_xz_vel = 272,
                .outer_friction = 9,
            });
        }
    }

    bool explode = false;
    int32_t radius = 0;
    if (item->pos.y >= item->floor
        || item->pos.y <= Room_GetCeiling(sector, item->pos)) {
        radius = M_BLAST_RADIUS;
        explode = true;
    }

    const GAME_VECTOR new_pos = {
        .pos = item->pos,
        .room_num = item->room_num,
    };
    if (Gun_SmashItems(old_pos, new_pos, nullptr, item->object_id)
        == PROJECTILE_HIT_STOP) {
        explode = true;
    }

    if (g_Config.gameplay.projectile_area_damage
        == PROJECTILE_AREA_DAMAGE_MULTI_SWEEP) {
        Room_GetNearbyRooms(item->pos, radius * 4, radius * 4, item->room_num);
        for (int32_t i = 0; i < Room_DrawGetCount(); i++) {
            const ROOM *const nearby_room = Room_Get(Room_DrawGetRoom(i));
            for (int16_t target_item_num = nearby_room->item_num;
                 target_item_num != NO_ITEM;
                 target_item_num = Item_Get(target_item_num)->next_item) {
                if (!M_TryExplodeItem(item, old_pos, target_item_num, radius)) {
                    continue;
                }

                if (!explode) {
                    explode = true;
                    radius = WALL_L;
                    i = -1;
                    break;
                }
            }
        }
    } else {
        for (int16_t target_item_num = room->item_num;
             target_item_num != NO_ITEM;
             target_item_num = Item_Get(target_item_num)->next_item) {
            if (M_TryExplodeItem(item, old_pos, target_item_num, radius)) {
                explode = true;
            }
        }
    }

    if (explode) {
        if (was_underwater) {
            item->pos = old_pos.pos;
            Item_UpdateRoom(item_num, old_pos.room_num);
        }
        M_Explode(item_num, old_pos.pos);
    }
}

static void M_SetupCommon(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->save_position = true;
}

static void M_Setup(OBJECT *const obj)
{
    M_SetupCommon(obj);
}

static void M_SetupHeavy(OBJECT *const obj)
{
    if (!Object_Get(O_ROCKET)->loaded) {
        return;
    }

    M_SetupCommon(obj);
    IGNORE(Object_BorrowContent(O_HEAVY_ROCKET, O_ROCKET));
}

REGISTER_OBJECT(O_ROCKET, M_Setup)
REGISTER_OBJECT(O_HEAVY_ROCKET, M_SetupHeavy)
