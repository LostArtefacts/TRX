#include "game/objects/traps/propeller.h"

#include "game/items.h"
#include "game/lara/control.h"
#include "game/random.h"
#include "game/sound.h"
#include "game/spawn.h"

#include <libtrx/game/lara/common.h>

#define PROPELLER_DAMAGE 200

typedef enum {
    // clang-format off
    PROPELLER_STATE_ON  = 0,
    PROPELLER_STATE_OFF = 1,
    // clang-format on
} PROPELLER_STATE;

void Propeller_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    if (Item_IsTriggerActive(item) && !(item->flags & IF_ONE_SHOT)) {
        item->goal_anim_state = PROPELLER_STATE_ON;

        if ((item->touch_bits & 6) != 0) {
            Lara_TakeDamage(PROPELLER_DAMAGE, true);

            const ITEM *const lara_item = Lara_GetItem();
            Spawn_BloodBath(
                lara_item->pos.x, lara_item->pos.y - WALL_L / 2,
                lara_item->pos.z, Random_GetDraw() >> 10, item->rot.y + 0x4000,
                lara_item->room_num, 3);

            if (item->object_id == O_POWER_SAW) {
                Sound_Effect(SFX_SAW_STOP, &item->pos, SPM_NORMAL);
            }
        } else if (item->object_id == O_POWER_SAW) {
            Sound_Effect(SFX_SAW_REVVING, &item->pos, SPM_NORMAL);
        } else if (item->object_id == O_PROPELLER_1) {
            Sound_Effect(SFX_AIRPLANE_IDLE, &item->pos, SPM_NORMAL);
        } else if (item->object_id == O_PROPELLER_2) {
            Sound_Effect(SFX_UNDERWATER_FAN_ON, &item->pos, SPM_UNDERWATER);
        } else {
            Sound_Effect(SFX_SMALL_FAN_ON, &item->pos, SPM_NORMAL);
        }
    } else if (item->goal_anim_state != PROPELLER_STATE_OFF) {
        if (item->object_id == O_PROPELLER_1) {
            Sound_Effect(SFX_AIRPLANE_IDLE, &item->pos, SPM_NORMAL);
        } else if (item->object_id == O_PROPELLER_2) {
            // NOTE: this sound effect is not present in the OG files
            Sound_Effect(216, &item->pos, SPM_UNDERWATER);
        }
        item->goal_anim_state = PROPELLER_STATE_OFF;
    }

    Item_Animate(item);

    if (item->status == IS_DEACTIVATED) {
        Item_RemoveActive(item_num);
        if (item->object_id != O_POWER_SAW) {
            item->collidable = 0;
        }
    }
}

void Propeller_Setup(OBJECT *const obj)
{
    obj->control_func = Propeller_Control;
    obj->collision_func = Object_Collision_Trap;
    obj->save_flags = 1;
    obj->save_anim = 1;
}
