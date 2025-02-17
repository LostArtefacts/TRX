#include "game/objects/creatures/eel.h"

#include "game/creature.h"
#include "game/items.h"
#include "game/lara/control.h"
#include "game/objects/common.h"
#include "game/spawn.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/game/math.h>
#include <libtrx/utils.h>

// clang-format off
#define EEL_HITPOINTS  5
#define EEL_TOUCH_BITS 0b00000001'10000000 // = 0x180
#define EEL_DAMAGE     50
#define EEL_ANGLE      (DEG_1 * 10) // = 1820
#define EEL_RANGE      (WALL_L * 2) // = 2048
#define EEL_MOVE       (WALL_L / 10) // = 102
#define EEL_TURN       (DEG_1 / 2) // = 91
#define EEL_LENGTH     (WALL_L / 2) // = 512
#define EEL_SLIDE      (EEL_RANGE - EEL_LENGTH) // = 1536
// clang-format on

typedef enum {
    // clang-format off
    EEL_STATE_EMPTY  = 0,
    EEL_STATE_ATTACK = 1,
    EEL_STATE_STOP   = 2,
    EEL_STATE_DEATH  = 3,
    // clang-format on
} EEL_STATE;

typedef enum {
    EEL_ANIM_DEATH = 3,
} EEL_ANIM;

static const BITE m_EelBite = {
    .pos = { .x = 7, .y = 157, .z = 333 },
    .mesh_num = 7,
};

void Eel_Setup(void)
{
    OBJECT *const obj = Object_Get(O_EEL);
    if (!obj->loaded) {
        return;
    }

    obj->control_func = Eel_Control;
    obj->collision_func = Creature_Collision;

    obj->hit_points = EEL_HITPOINTS;

    obj->save_hitpoints = 1;
    obj->save_flags = 1;
    obj->save_anim = 1;
}

void Eel_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    int32_t pos = (int32_t)(intptr_t)item->data;
    item->pos.x -= (pos * Math_Sin(item->rot.y)) >> W2V_SHIFT;
    item->pos.z -= (pos * Math_Cos(item->rot.y)) >> W2V_SHIFT;

    if (item->hit_points <= 0) {
        if (pos < EEL_SLIDE) {
            pos += EEL_MOVE;
        }
        if (item->current_anim_state != EEL_STATE_DEATH) {
            Item_SwitchToAnim(item, EEL_ANIM_DEATH, 0);
            item->current_anim_state = EEL_STATE_DEATH;
        }
    } else {
        const int32_t dx = g_LaraItem->pos.x - item->pos.x;
        const int32_t dz = g_LaraItem->pos.z - item->pos.z;
        const int16_t quadrant = (item->rot.y + DEG_45) & 0xC000;
        const int16_t angle = Math_Atan(dz, dx);
        const int32_t distance = Math_Sqrt(SQUARE(dx) + SQUARE(dz));

        switch (item->current_anim_state) {
        case EEL_STATE_STOP:
            if (pos > 0) {
                pos -= EEL_MOVE;
            }
            if (distance <= EEL_RANGE && ABS(angle - quadrant) < EEL_ANGLE) {
                item->goal_anim_state = EEL_STATE_ATTACK;
            }
            break;

        case EEL_STATE_ATTACK:
            if (pos < distance - EEL_LENGTH) {
                pos += EEL_MOVE;
            }
            if (angle < item->rot.y - EEL_TURN) {
                item->rot.y -= EEL_TURN;
            } else if (angle > item->rot.y + EEL_TURN) {
                item->rot.y += EEL_TURN;
            }
            if (item->required_anim_state == EEL_STATE_EMPTY
                && (item->touch_bits & EEL_TOUCH_BITS) != 0) {
                Lara_TakeDamage(EEL_DAMAGE, true);
                Creature_Effect(item, &m_EelBite, Spawn_Blood);
                item->required_anim_state = EEL_STATE_STOP;
            }
            break;
        }
    }

    item->pos.x += (pos * Math_Sin(item->rot.y)) >> W2V_SHIFT;
    item->pos.z += (pos * Math_Cos(item->rot.y)) >> W2V_SHIFT;
    item->data = (void *)(intptr_t)pos;
    Item_Animate(item);
}
