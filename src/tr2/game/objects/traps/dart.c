#include "game/objects/traps/dart.h"

#include "game/effects.h"
#include "game/items.h"
#include "game/lara/control.h"
#include "game/objects/common.h"
#include "game/random.h"
#include "game/room.h"
#include "game/sound.h"
#include "game/spawn.h"
#include "global/vars.h"

#include <libtrx/game/lara/common.h>

#define DART_DAMAGE 50

static void M_Hit(int16_t item_num);

static void M_Hit(const int16_t item_num)
{
    const ITEM *const item = Item_Get(item_num);
    Item_Kill(item_num);
    Sound_Effect(SFX_CIRCLE_BLADE_HIT, &item->pos, SPM_NORMAL);

    const int16_t effect_num = Effect_Create(item->room_num);
    if (effect_num != NO_EFFECT) {
        EFFECT *const effect = Effect_Get(effect_num);
        effect->object_id = O_RICOCHET;
        effect->pos = item->pos;
        effect->rot = item->rot;
        effect->room_num = item->room_num;
        effect->speed = 0;
        effect->counter = 6;
        effect->frame_num = -3 * Random_GetControl() / 0x8000;
    }
}

void Dart_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (item->touch_bits != 0) {
        Lara_TakeDamage(DART_DAMAGE, true);
        const ITEM *const lara_item = Lara_GetItem();
        Spawn_Blood(
            item->pos.x, item->pos.y, item->pos.z, lara_item->speed,
            lara_item->rot.y, lara_item->room_num);
    }

    Item_Animate(item);

    int16_t room_num = item->room_num;
    const SECTOR *const sector =
        Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);
    if (item->room_num != room_num) {
        Item_NewRoom(item_num, room_num);
    }
    const int32_t height =
        Room_GetHeight(sector, item->pos.x, item->pos.y, item->pos.z);

    item->rot.x += 4096;
    item->floor = height;
    if (item->pos.y >= height) {
        M_Hit(item_num);
    }
}

void Dart_Setup(void)
{
    OBJECT *const obj = Object_Get(O_DART);
    obj->control_func = Dart_Control;
    obj->collision_func = Object_Collision;
    obj->shadow_size = 128;
}
