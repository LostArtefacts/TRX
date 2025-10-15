#include "game/effects.h"
#include "game/lara.h"
#include "game/objects/common.h"
#include "game/random.h"
#include "game/rooms.h"
#include "game/sound.h"
#include "game/spawn.h"
#include "version.h"

#define M_DAMAGE 50
#define M_PITCH (DEG_45 / 2)

static void M_Hit(const int16_t item_num, const XYZ_32 pos)
{
    const ITEM *const item = Item_Get(item_num);
    Item_Kill(item_num);
    Sound_Effect(SFX_DARTS_HIT, &item->pos, SPM_NORMAL);

    const int16_t effect_num = Effect_Create(item->room_num);
    if (effect_num != NO_EFFECT) {
        EFFECT *const effect = Effect_Get(effect_num);
        effect->object_id = O_RICOCHET;
        effect->pos.x = pos.x;
        effect->pos.y = item->pos.y;
        effect->pos.z = pos.z;
        effect->rot = item->rot;
        effect->room_num = item->room_num;
        effect->speed = 0;
        effect->counter = 6;
        effect->frame_num = -3 * Random_GetControl() / 0x8000;
    }
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (item->touch_bits != 0) {
        Lara_TakeDamage(M_DAMAGE, true);
        const ITEM *const lara_item = Lara_GetItem();
        Spawn_Blood(
            item->pos.x, item->pos.y, item->pos.z, lara_item->speed,
            lara_item->rot.y, lara_item->room_num);
    }

    const XYZ_32 old_pos = item->pos;
    Item_Animate(item);

    int16_t room_num = item->room_num;
    const SECTOR *const sector =
        Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);
    Item_UpdateRoom(item_num, room_num);
    const int32_t height =
        Room_GetHeight(sector, item->pos.x, item->pos.y, item->pos.z);

    if (g_TRVersion == 2) {
        item->rot.x += M_PITCH;
    }
    item->floor = height;
    if (item->pos.y >= height) {
        M_Hit(item_num, g_TRVersion == 1 ? old_pos : item->pos);
    }
}

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->collision_func = Object_Collision;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->save_flags = true;
}

REGISTER_OBJECT(O_DART, M_Setup)
