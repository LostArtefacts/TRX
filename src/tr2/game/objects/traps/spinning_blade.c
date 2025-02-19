#include "game/items.h"
#include "game/lara/control.h"
#include "game/objects/common.h"
#include "game/room.h"
#include "game/sound.h"
#include "game/spawn.h"
#include "global/vars.h"

#include <libtrx/game/lara/common.h>
#include <libtrx/game/math.h>

#define SPINNING_BLADE_DAMAGE 100

typedef enum {
    // clang-format off
    SPINNING_BLADE_STATE_EMPTY = 0,
    SPINNING_BLADE_STATE_STOP  = 1,
    SPINNING_BLADE_STATE_SPIN  = 2,
    // clang-format on
} SPINNING_BLADE_STATE;

typedef enum {
    // clang-format off
    SPINNING_BLADE_ANIM_SPIN_FAST = 0,
    SPINNING_BLADE_ANIM_SPIN_SLOW = 1,
    SPINNING_BLADE_ANIM_SPIN_END  = 2,
    SPINNING_BLADE_ANIM_STOP      = 3,
    // clang-format on
} SPINNING_BLADE_ANIM;

static void M_Setup(OBJECT *obj);
static void M_Initialise(int16_t item_num);
static void M_Control(int16_t item_num);

static void M_Setup(OBJECT *const obj)
{
    obj->initialise_func = M_Initialise;
    obj->control_func = M_Control;
    obj->collision_func = Object_Collision;
    obj->save_position = 1;
    obj->save_flags = 1;
    obj->save_anim = 1;
}

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    const OBJECT *const obj = Object_Get(item->object_id);
    Item_SwitchToAnim(item, SPINNING_BLADE_ANIM_STOP, 0);
    item->current_anim_state = SPINNING_BLADE_STATE_STOP;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    bool spinning = false;

    if (item->current_anim_state == SPINNING_BLADE_STATE_SPIN) {
        if (item->goal_anim_state != SPINNING_BLADE_STATE_STOP) {
            const int32_t x = item->pos.x
                + ((WALL_L * 3 / 2 * Math_Sin(item->rot.y)) >> W2V_SHIFT);
            const int32_t z = item->pos.z
                + ((WALL_L * 3 / 2 * Math_Cos(item->rot.y)) >> W2V_SHIFT);

            int16_t room_num = item->room_num;
            const SECTOR *const sector =
                Room_GetSector(x, item->pos.y, z, &room_num);
            if (Room_GetHeight(sector, x, item->pos.y, z) == NO_HEIGHT) {
                item->goal_anim_state = SPINNING_BLADE_STATE_STOP;
            }
        }

        spinning = true;

        if (item->touch_bits != 0) {
            Lara_TakeDamage(SPINNING_BLADE_DAMAGE, true);

            const ITEM *const lara_item = Lara_GetItem();
            Spawn_BloodBath(
                lara_item->pos.x, lara_item->pos.y - WALL_L / 2,
                lara_item->pos.z, item->speed * 2, lara_item->rot.y,
                lara_item->room_num, 2);
        }

        Sound_Effect(SFX_ROLLING_BLADE, &item->pos, SPM_NORMAL);
    } else {
        if (Item_IsTriggerActive(item)) {
            item->goal_anim_state = SPINNING_BLADE_STATE_SPIN;
        }
        spinning = false;
    }

    Item_Animate(item);

    int16_t room_num = item->room_num;
    const SECTOR *const sector =
        Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);
    const int32_t height =
        Room_GetHeight(sector, item->pos.x, item->pos.y, item->pos.z);
    item->pos.y = height;
    item->floor = height;
    if (room_num != item->room_num) {
        Item_NewRoom(item_num, room_num);
    }

    if (spinning) {
        if (item->current_anim_state == SPINNING_BLADE_STATE_STOP) {
            item->rot.y += DEG_180;
        }
    }
}

REGISTER_OBJECT(O_SPINNING_BLADE, M_Setup)
