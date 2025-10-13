#include "game/carrier.h"
#include "game/creature.h"
#include "game/game.h"
#include "game/game_buf.h"
#include "game/lara.h"
#include "game/math.h"
#include "game/objects/common.h"
#include "game/savegame.h"
#include "game/stats.h"
#include "utils.h"

#define MUMMY_HITPOINTS 18

typedef enum {
    MUMMY_STATE_EMPTY = 0,
    MUMMY_STATE_STOP = 1,
    MUMMY_STATE_DEATH = 2,
} MUMMY_STATE;

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    item->touch_bits = 0;
    item->mesh_bits = 0xFFFF87FF;
    item->data = GameBuf_Alloc(sizeof(int16_t), GBUF_CREATURE_DATA);
    *(int16_t *)item->data = 0;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    int16_t head = 0;

    if (item->current_anim_state == MUMMY_STATE_STOP) {
        const ITEM *const lara_item = Lara_GetItem();
        head =
            Math_Atan(
                lara_item->pos.z - item->pos.z, lara_item->pos.x - item->pos.x)
            - item->rot.y;
        CLAMP(head, -FRONT_ARC, FRONT_ARC);

        if (item->hit_points <= 0 || item->touch_bits) {
            item->goal_anim_state = MUMMY_STATE_DEATH;
        }
    }

    Creature_Head(item, head);
    Item_Animate(item);

    if (item->status == IS_DEACTIVATED) {
        // Count kill if Lara touches mummy and it falls.
        if (item->hit_points > 0) {
            Stats_AddKill();
        }
        Item_RemoveActive(item_num);
        if (item->hit_points != DONT_TARGET) {
            Carrier_TestItemDrops(item_num);
        }
        item->hit_points = DONT_TARGET;
    }
}

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise_func = M_Initialise;
    obj->control_func = M_Control;
    obj->collision_func = Object_Collision;
    obj->hit_points = MUMMY_HITPOINTS;
    obj->save_flags = true;
    obj->save_hitpoints = true;
    obj->save_anim = true;

    Object_GetBone(obj, 2)->rot.y = true;
}

REGISTER_OBJECT(O_MUMMY, M_Setup)
