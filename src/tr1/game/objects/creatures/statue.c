#include "game/objects/creatures/statue.h"

#include "game/items.h"
#include "game/lot.h"
#include "game/objects/common.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/debug.h>
#include <libtrx/game/game_buf.h>
#include <libtrx/utils.h>

#define STATUE_EXPLODE_DIST (WALL_L * 7 / 2) // = 3584
#define CENTAUR_REARING_ANIM 7
#define CENTAUR_REARING_FRAME 36

void Statue_Setup(OBJECT *obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise = Statue_Initialise;
    obj->control = Statue_Control;
    obj->collision = Object_Collision;
    obj->save_anim = 1;
    obj->save_flags = 1;
}

void Statue_Initialise(int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    const int16_t centaur_item_num = Item_CreateLevelItem();
    ASSERT(centaur_item_num != NO_ITEM);

    ITEM *const centaur = Item_Get(centaur_item_num);
    centaur->object_id = O_CENTAUR;
    centaur->room_num = item->room_num;
    centaur->pos.x = item->pos.x;
    centaur->pos.y = item->pos.y;
    centaur->pos.z = item->pos.z;
    centaur->flags = IF_INVISIBLE;
    centaur->shade.value_1 = -1;

    Item_Initialise(centaur_item_num);

    Item_SwitchToAnim(centaur, CENTAUR_REARING_ANIM, CENTAUR_REARING_FRAME);
    centaur->current_anim_state = Item_GetAnim(centaur)->current_anim_state;
    centaur->goal_anim_state = centaur->current_anim_state;
    centaur->rot.y = item->rot.y;

    item->data = GameBuf_Alloc(sizeof(int16_t), GBUF_CREATURE_DATA);
    *(int16_t *)item->data = centaur_item_num;
}

void Statue_Control(int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (item->flags & IF_KILLED) {
        return;
    }

    int32_t x = g_LaraItem->pos.x - item->pos.x;
    int32_t y = g_LaraItem->pos.y - item->pos.y;
    int32_t z = g_LaraItem->pos.z - item->pos.z;

    if (y > -WALL_L && y < WALL_L
        && SQUARE(x) + SQUARE(z) < SQUARE(STATUE_EXPLODE_DIST)) {
        Item_Explode(item_num, -1, 0);
        Item_Kill(item_num);
        item->status = IS_DEACTIVATED;

        int16_t centaur_item_num = *(int16_t *)item->data;
        ITEM *const centaur = Item_Get(centaur_item_num);
        centaur->touch_bits = 0;
        Item_AddActive(centaur_item_num);
        LOT_EnableBaddieAI(centaur_item_num, 1);
        centaur->status = IS_ACTIVE;

        Sound_Effect(SFX_ATLANTEAN_EXPLODE, &centaur->pos, SPM_NORMAL);
    }
}
