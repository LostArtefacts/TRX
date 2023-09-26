#include "game/objects/creatures/mummy.h"

#include "game/carrier.h"
#include "game/creature.h"
#include "game/gamebuf.h"
#include "game/items.h"
#include "game/objects/common.h"
#include "global/const.h"
#include "global/vars.h"
#include "math/math.h"
#include "util.h"

#define MUMMY_HITPOINTS 18

typedef enum {
    MUMMY_EMPTY = 0,
    MUMMY_STOP = 1,
    MUMMY_DEATH = 2,
} MUMMY_ANIM;

void Mummy_Setup(OBJECT_INFO *obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise = Mummy_Initialise;
    obj->control = Mummy_Control;
    obj->collision = Object_Collision;
    obj->hit_points = MUMMY_HITPOINTS;
    obj->save_flags = 1;
    obj->save_hitpoints = 1;
    obj->save_anim = 1;
    g_AnimBones[obj->bone_index + 8] |= BEB_ROT_Y;
}

void Mummy_Initialise(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];
    item->touch_bits = 0;
    item->mesh_bits = 0xFFFF87FF;
    item->data = GameBuf_Alloc(sizeof(int16_t), GBUF_CREATURE_DATA);
    *(int16_t *)item->data = 0;
}

void Mummy_Control(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];
    int16_t head = 0;

    if (item->current_anim_state == MUMMY_STOP) {
        head = Math_Atan(
                   g_LaraItem->pos.z - item->pos.z,
                   g_LaraItem->pos.x - item->pos.x)
            - item->pos.y_rot;
        CLAMP(head, -FRONT_ARC, FRONT_ARC);

        if (item->hit_points <= 0 || item->touch_bits) {
            item->goal_anim_state = MUMMY_DEATH;
        }
    }

    Creature_Head(item, head);
    Item_Animate(item);

    if (item->status == IS_DEACTIVATED) {
        // Count kill if Lara touches mummy and it falls.
        if (item->hit_points > 0) {
            g_GameInfo.current[g_CurrentLevel].stats.kill_count++;
        }
        Item_RemoveActive(item_num);
        if (item->hit_points != DONT_TARGET) {
            Carrier_TestItemDrops(item_num);
        }
        item->hit_points = DONT_TARGET;
    }
}
