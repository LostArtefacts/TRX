#include "game/carrier.h"
#include "game/creature.h"
#include "game/game.h"
#include "game/items.h"
#include "game/objects/common.h"
#include "game/savegame.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/game/game_buf.h>
#include <libtrx/game/math.h>
#include <libtrx/utils.h>

#define MUMMY_HITPOINTS 18

typedef enum {
    MUMMY_STATE_EMPTY = 0,
    MUMMY_STATE_STOP = 1,
    MUMMY_STATE_DEATH = 2,
} MUMMY_STATE;

static void M_Setup(OBJECT *obj);
static void M_Initialise(int16_t item_num);
static void M_Control(int16_t item_num);

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise_func = M_Initialise;
    obj->control_func = M_Control;
    obj->collision_func = Object_Collision;
    obj->hit_points = MUMMY_HITPOINTS;
    obj->save_flags = 1;
    obj->save_hitpoints = 1;
    obj->save_anim = 1;

    Object_GetBone(obj, 2)->rot_y = true;
}

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
        head = Math_Atan(
                   g_LaraItem->pos.z - item->pos.z,
                   g_LaraItem->pos.x - item->pos.x)
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
            Savegame_GetCurrentInfo(Game_GetCurrentLevel())->stats.kill_count++;
        }
        Item_RemoveActive(item_num);
        if (item->hit_points != DONT_TARGET) {
            Carrier_TestItemDrops(item_num);
        }
        item->hit_points = DONT_TARGET;
    }
}

REGISTER_OBJECT(O_MUMMY, M_Setup)
