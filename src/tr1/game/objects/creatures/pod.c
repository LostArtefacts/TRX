#include "game/objects/creatures/pod.h"

#include "game/items.h"
#include "game/lot.h"
#include "game/objects/common.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/game/game_buf.h>
#include <libtrx/utils.h>

#define POD_EXPLODE_DIST (WALL_L * 4) // = 4096

typedef enum {
    POD_STATE_SET = 0,
    POD_STATE_EXPLODE = 1,
} POD_STATE;

void Pod_Setup(OBJECT *obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise = Pod_Initialise;
    obj->control = Pod_Control;
    obj->collision = Object_Collision;
    obj->save_anim = 1;
    obj->save_flags = 1;
}

void Pod_Initialise(int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    const int16_t bug_item_num = Item_CreateLevelItem();
    if (bug_item_num != NO_ITEM) {
        ITEM *const bug = Item_Get(bug_item_num);

        switch ((item->flags & IF_CODE_BITS) >> 9) {
        case 1:
            bug->object_id = O_WARRIOR_2;
            break;
        case 2:
            bug->object_id = O_CENTAUR;
            break;
        case 4:
            bug->object_id = O_TORSO;
            break;
        case 8:
            bug->object_id = O_WARRIOR_3;
            break;
        default:
            bug->object_id = O_WARRIOR_1;
            break;
        }

        bug->room_num = item->room_num;
        bug->pos.x = item->pos.x;
        bug->pos.y = item->pos.y;
        bug->pos.z = item->pos.z;
        bug->rot.y = item->rot.y;
        bug->flags = IF_INVISIBLE;
        bug->shade.value_1 = -1;

        Item_Initialise(bug_item_num);

        item->data = GameBuf_Alloc(sizeof(int16_t), GBUF_CREATURE_DATA);
        *(int16_t *)item->data = bug_item_num;
    }

    item->flags = 0;
    item->mesh_bits = 0xFF0001FF;
}

void Pod_Control(int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    if (item->goal_anim_state != POD_STATE_EXPLODE) {
        int32_t explode = 0;

        if (item->flags & IF_ONE_SHOT) {
            explode = 1;
        } else if (item->object_id == O_BIG_POD) {
            explode = 1;
        } else {
            int32_t x = g_LaraItem->pos.x - item->pos.x;
            int32_t y = g_LaraItem->pos.y - item->pos.y;
            int32_t z = g_LaraItem->pos.z - item->pos.z;
            if (ABS(x) < POD_EXPLODE_DIST && ABS(y) < POD_EXPLODE_DIST
                && ABS(z) < POD_EXPLODE_DIST) {
                explode = 1;
            }
        }

        if (explode) {
            item->goal_anim_state = POD_STATE_EXPLODE;
            item->mesh_bits = 0xFFFFFF;
            item->collidable = 0;
            Item_Explode(item_num, 0xFFFE00, 0);

            int16_t bug_item_num = *(int16_t *)item->data;
            ITEM *const bug = Item_Get(bug_item_num);
            if (Object_Get(bug->object_id)->loaded) {
                bug->touch_bits = 0;
                Item_AddActive(bug_item_num);
                if (LOT_EnableBaddieAI(bug_item_num, 0)) {
                    bug->status = IS_ACTIVE;
                } else {
                    bug->status = IS_INVISIBLE;
                }
            }
            item->status = IS_DEACTIVATED;
        }
    }

    Item_Animate(item);
}
