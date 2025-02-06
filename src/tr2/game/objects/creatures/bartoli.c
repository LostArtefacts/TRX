#include "game/objects/creatures/bartoli.h"

#include "game/camera.h"
#include "game/items.h"
#include "game/lot.h"
#include "game/spawn.h"
#include "global/vars.h"

#include <libtrx/debug.h>
#include <libtrx/utils.h>

#define BOOM_TIME 130
#define BARTOLI_RANGE (WALL_L * 5) // = 5120

static void M_CreateBoom(GAME_OBJECT_ID obj_id, const ITEM *origin_item);
static void M_ConvertBartoliToDragon(const int16_t item_num);
static bool M_CheckLaraProximity(const ITEM *origin_item);

static void M_CreateBoom(
    const GAME_OBJECT_ID obj_id, const ITEM *const origin_item)
{
    const int16_t item_num = Item_Create();
    if (item_num == NO_ITEM) {
        return;
    }

    ITEM *const sphere_item = Item_Get(item_num);
    sphere_item->object_id = obj_id;
    sphere_item->pos.x = origin_item->pos.x;
    sphere_item->pos.y = origin_item->pos.y + 256;
    sphere_item->pos.z = origin_item->pos.z;
    sphere_item->room_num = origin_item->room_num;
    sphere_item->shade.value_1 = -1;
    Item_Initialise(item_num);
    Item_AddActive(item_num);
    sphere_item->status = IS_ACTIVE;
}

static void M_ConvertBartoliToDragon(const int16_t item_num)
{
    const ITEM *const item_bartoli = Item_Get(item_num);

    const int16_t item_dragon_back_num = (intptr_t)item_bartoli->data;
    ITEM *const item_dragon_back = Item_Get(item_dragon_back_num);
    const int16_t item_dragon_front_num = (intptr_t)item_dragon_back->data;
    ITEM *const item_dragon_front = Item_Get(item_dragon_front_num);

    item_dragon_back->touch_bits = 0;
    item_dragon_front->touch_bits = 0;

    LOT_EnableBaddieAI(item_dragon_front_num, true);
    Item_AddActive(item_dragon_front_num);
    Item_AddActive(item_dragon_back_num);

    item_dragon_back->status = IS_ACTIVE;
    Item_Kill(item_num);
}

static bool M_CheckLaraProximity(const ITEM *const origin_item)
{
    const int32_t dx = ABS(g_LaraItem->pos.x - origin_item->pos.x);
    const int32_t dz = ABS(g_LaraItem->pos.z - origin_item->pos.z);
    return dx < BARTOLI_RANGE && dz < BARTOLI_RANGE;
}

void Bartoli_Setup(void)
{
    OBJECT *const obj = Object_Get(O_BARTOLI);
    if (!obj->loaded) {
        return;
    }

    obj->initialise = Bartoli_Initialise;
    obj->control = Bartoli_Control;

    obj->save_flags = 1;
    obj->save_anim = 1;
}

void Bartoli_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    item->pos.x -= 2 * STEP_L;
    item->pos.z -= 2 * STEP_L;

    const int16_t item_dragon_back_num = Item_CreateLevelItem();
    const int16_t item_dragon_front_num = Item_CreateLevelItem();
    ASSERT(item_dragon_back_num != NO_ITEM);
    ASSERT(item_dragon_front_num != NO_ITEM);

    ITEM *const item_dragon_back = Item_Get(item_dragon_back_num);
    item_dragon_back->object_id = O_DRAGON_BACK;
    item_dragon_back->pos.x = item->pos.x;
    item_dragon_back->pos.y = item->pos.y;
    item_dragon_back->pos.z = item->pos.z;
    item_dragon_back->rot.y = item->rot.y;
    item_dragon_back->room_num = item->room_num;
    item_dragon_back->flags = IF_INVISIBLE;
    item_dragon_back->shade.value_1 = -1;
    Item_Initialise(item_dragon_back_num);
    item_dragon_back->mesh_bits = 0x1FFFFF;

    ITEM *const item_dragon_front = Item_Get(item_dragon_front_num);
    item_dragon_front->object_id = O_DRAGON_FRONT;
    item_dragon_front->pos.x = item->pos.x;
    item_dragon_front->pos.y = item->pos.y;
    item_dragon_front->pos.z = item->pos.z;
    item_dragon_front->rot.y = item->rot.y;
    item_dragon_front->room_num = item->room_num;
    item_dragon_front->flags = IF_INVISIBLE;
    item_dragon_front->shade.value_1 = -1;
    Item_Initialise(item_dragon_front_num);
    item_dragon_back->data = (void *)(intptr_t)item_dragon_front_num;

    item->data = (void *)(intptr_t)item_dragon_back_num;
}

void Bartoli_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    if (item->timer == 0) {
        if (M_CheckLaraProximity(item)) {
            item->timer = 1;
        }
        return;
    }

    item->timer++;
    if ((item->timer & 7) == 0) {
        g_Camera.bounce = item->timer;
    }

    Spawn_MysticLight(item_num);
    Item_Animate(item);

    if (item->timer == BOOM_TIME + 0) {
        M_CreateBoom(O_SPHERE_OF_DOOM_1, item);
    } else if (item->timer == BOOM_TIME + 10) {
        M_CreateBoom(O_SPHERE_OF_DOOM_2, item);
    } else if (item->timer == BOOM_TIME + 20) {
        M_CreateBoom(O_SPHERE_OF_DOOM_3, item);
    } else if (item->timer >= BOOM_TIME + 20) {
        M_ConvertBartoliToDragon(item_num);
    }
}
