#include <trx/game/effects.h>
#include <trx/game/lara.h>
#include <trx/game/objects/common.h>
#include <trx/game/output/sources/poly_fx.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>
#include <trx/game/sparks.h>
#include <trx/game/spawn.h>
#include <trx/version.h>

#define M_DART_DAMAGE 50
#define M_POISON_DART_DAMAGE 25
#define M_POISON_AMOUNT 160
#define M_PITCH (DEG_45 / 2)

static void M_DamageLara(const ITEM *const item)
{
    const bool is_poison = item->object_id == O_POISON_DART;
    const int32_t damage = is_poison ? M_POISON_DART_DAMAGE : M_DART_DAMAGE;
    Lara_TakeDamage(damage, true);
    if (is_poison) {
        LARA_INFO *const lara = Lara_GetLaraInfo();
        lara->poison_timer += M_POISON_AMOUNT;
    }

    const ITEM *const lara_item = Lara_GetItem();
    Spawn_Blood(
        item->pos.x, item->pos.y, item->pos.z, lara_item->speed,
        lara_item->rot.y, lara_item->room_num);
}

static void M_Hit(const int16_t item_num, const XYZ_32 pos)
{
    const ITEM *const item = Item_Get(item_num);
    Item_Kill(item_num);

    if (item->object_id == O_POISON_DART) {
        for (int32_t i = 0; i < 4; i++) {
            Sparks_TriggerDartSmoke(pos, (XZ_32) {}, true);
        }
        return;
    }

    Sound_Effect(SFX_PROJECTILE_HIT, &item->pos, SPM_NORMAL);

    int16_t room_num = item->room_num;
    Room_GetSector(pos.x, pos.y, pos.z, &room_num);

    const int16_t effect_num = Effect_Create(room_num);
    if (effect_num != NO_EFFECT) {
        EFFECT *const effect = Effect_Get(effect_num);
        effect->object_id = O_RICOCHET;
        effect->pos = pos;
        effect->rot = item->rot;
        effect->counter = 6;
        effect->frame_num = -3 * Random_GetControl() / 0x8000;
    }
}

static void M_Animate(ITEM *const item)
{
    if (item->object_id != O_POISON_DART) {
        Item_Animate(item);
        return;
    }

    const int32_t speed = (item->speed * Math_Cos(item->rot.x)) >> W2V_SHIFT;
    item->pos.x += (speed * Math_Sin(item->rot.y)) >> W2V_SHIFT;
    item->pos.y -= (item->speed * Math_Sin(item->rot.x)) >> W2V_SHIFT;
    item->pos.z += (speed * Math_Cos(item->rot.y)) >> W2V_SHIFT;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (item->touch_bits != 0) {
        M_DamageLara(item);
        if (item->object_id == O_POISON_DART) {
            Item_Kill(item_num);
            return;
        }
    }

    const GAME_VECTOR old_pos = { .pos = item->pos,
                                  .room_num = item->room_num };
    M_Animate(item);

    int16_t room_num = item->room_num;
    const SECTOR *const sector =
        Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);
    Item_UpdateRoom(item_num, room_num);
    const int32_t height =
        Room_GetHeight(sector, item->pos.x, item->pos.y, item->pos.z);

    if (item->object_id == O_DISC) {
        item->rot.x += M_PITCH;
    }
    item->floor = height;
    const GAME_VECTOR new_pos = { .pos = item->pos,
                                  .room_num = item->room_num };
    if (item->pos.y >= height) {
        M_Hit(item_num, Spawn_GetRayPos(old_pos, new_pos, STEP_L / 12));
    }
}

static bool M_DrawPoisonDart(const ITEM *const item)
{
    const XYZ_32 origin = item->interp.result.pos;
    const XYZ_16 rot = item->interp.result.rot;
    const int32_t size = (-96 * Math_Cos(rot.x)) >> W2V_SHIFT;
    const XYZ_32 to = {
        .x = origin.x + ((size * Math_Sin(rot.y)) >> W2V_SHIFT),
        .y = origin.y + ((96 * Math_Sin(rot.x)) >> W2V_SHIFT),
        .z = origin.z + ((size * Math_Cos(rot.y)) >> W2V_SHIFT),
    };
    const RGBA_8888 color = { 0x78, 0x3C, 0x14, 0xFF };
    OutputSource_PolyFX_StageLineSegment(origin, color, to, color, 2.0f);
    return true;
}

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->collision_func = Object_Collision;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->save_flags = true;
}

static void M_SetupPoisonDart(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->collision_func = Object_Collision;
    obj->draw_func = M_DrawPoisonDart;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->save_flags = true;
}

REGISTER_OBJECT(O_DART, M_Setup)
REGISTER_OBJECT(O_DISC, M_Setup)
REGISTER_OBJECT(O_POISON_DART, M_SetupPoisonDart)
