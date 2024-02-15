#include "game/objects/creatures/pod.h"

#include "game/effects/exploding_death.h"
#include "game/gamebuf.h"
#include "game/items.h"
#include "game/lot.h"
#include "game/objects/common.h"
#include "global/const.h"
#include "global/vars.h"
#include "util.h"

#define POD_EXPLODE_DIST (WALL_L * 4) // = 4096

typedef enum {
    POD_SET = 0,
    POD_EXPLODE = 1,
} POD_ANIM;

void Pod_Setup(OBJECT_INFO *obj)
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

void Pod_SetupBig(OBJECT_INFO *obj)
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
    ITEM_INFO *item = &g_Items[item_num];

    int16_t bug_item_num = Item_Create();
    if (bug_item_num != NO_ITEM) {
        ITEM_INFO *bug = &g_Items[bug_item_num];

        switch ((item->flags & IF_CODE_BITS) >> 9) {
        case 1:
            bug->object_number = O_WARRIOR2;
            break;
        case 2:
            bug->object_number = O_CENTAUR;
            break;
        case 4:
            bug->object_number = O_TORSO;
            break;
        case 8:
            bug->object_number = O_WARRIOR3;
            break;
        default:
            bug->object_number = O_WARRIOR1;
            break;
        }

        bug->room_number = item->room_number;
        bug->pos.x = item->pos.x;
        bug->pos.y = item->pos.y;
        bug->pos.z = item->pos.z;
        bug->rot.y = item->rot.y;
        bug->flags = IF_NOT_VISIBLE;
        bug->shade = -1;

        Item_Initialise(bug_item_num);

        item->data = GameBuf_Alloc(sizeof(int16_t), GBUF_CREATURE_DATA);
        *(int16_t *)item->data = bug_item_num;

        g_LevelItemCount++;
    }

    item->flags = 0;
    item->mesh_bits = 0xFF0001FF;
}

void Pod_Control(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];

    if (item->goal_anim_state != POD_EXPLODE) {
        int32_t explode = 0;

        if (item->flags & IF_ONESHOT) {
            explode = 1;
        } else if (item->object_number == O_BIG_POD) {
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
            item->goal_anim_state = POD_EXPLODE;
            item->mesh_bits = 0xFFFFFF;
            item->collidable = 0;
            Effect_ExplodingDeath(item_num, 0xFFFE00, 0);

            int16_t bug_item_num = *(int16_t *)item->data;
            ITEM_INFO *bug = &g_Items[bug_item_num];
            if (g_Objects[bug->object_number].loaded) {
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
