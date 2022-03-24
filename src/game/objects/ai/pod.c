#include "game/objects/ai/pod.h"

#include "game/collide.h"
#include "game/control.h"
#include "game/gamebuf.h"
#include "game/items.h"
#include "game/lot.h"
#include "game/objects/effects/body_part.h"
#include "game/sound.h"
#include "global/vars.h"

void SetupPod(OBJECT_INFO *obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise = InitialisePod;
    obj->control = PodControl;
    obj->collision = ObjectCollision;
    obj->save_anim = 1;
    obj->save_flags = 1;
}

void SetupBigPod(OBJECT_INFO *obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise = InitialisePod;
    obj->control = PodControl;
    obj->collision = ObjectCollision;
    obj->save_anim = 1;
    obj->save_flags = 1;
}

void InitialisePod(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];

    int16_t bug_item_num = CreateItem();
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
            bug->object_number = O_ABORTION;
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
        bug->pos.y_rot = item->pos.y_rot;
        bug->flags = IF_NOT_VISIBLE;
        bug->shade = -1;

        InitialiseItem(bug_item_num);

        item->data = GameBuf_Alloc(sizeof(int16_t), GBUF_CREATURE_DATA);
        *(int16_t *)item->data = bug_item_num;

        g_LevelItemCount++;
    }

    item->flags = 0;
    item->mesh_bits = 0xFF0001FF;
}

void PodControl(int16_t item_num)
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
            ExplodingDeath(item_num, 0xFFFE00, 0);

            int16_t bug_item_num = *(int16_t *)item->data;
            ITEM_INFO *bug = &g_Items[bug_item_num];
            if (g_Objects[bug->object_number].loaded) {
                bug->touch_bits = 0;
                AddActiveItem(bug_item_num);
                if (EnableBaddieAI(bug_item_num, 0)) {
                    bug->status = IS_ACTIVE;
                } else {
                    bug->status = IS_INVISIBLE;
                }
            }
            item->status = IS_DEACTIVATED;
        }
    }

    AnimateItem(item);
}
