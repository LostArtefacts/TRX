#include "game/creature.h"
#include "game/objects/common.h"
#include "game/random.h"
#include "game/sound.h"
#include "game/spawn.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/utils.h>

// clang-format off
#define WINSTON_RADIUS     (WALL_L / 10) // = 102
#define WINSTON_STOP_RANGE SQUARE(WALL_L * 3 / 2) // = 2359296
// clang-format on

typedef enum {
    // clang-format off
    WINSTON_STATE_EMPTY = 0,
    WINSTON_STATE_STOP  = 1,
    WINSTON_STATE_WALK  = 2,
    // clang-format on
} WINSTON_STATE;

static void M_Setup(OBJECT *obj);
static void M_Control(int16_t item_num);

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->control_func = M_Control;
    obj->collision_func = Object_Collision;

    obj->hit_points = DONT_TARGET;
    obj->radius = WINSTON_RADIUS;
    obj->shadow_size = UNIT_SHADOW / 4;

    obj->intelligent = 1;
    obj->save_position = 1;
    obj->save_flags = 1;
    obj->save_anim = 1;
}

static void M_Control(const int16_t item_num)
{
    if (!Creature_Activate(item_num)) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    CREATURE *const creature = item->data;

    AI_INFO info;
    Creature_AIInfo(item, &info);
    Creature_Mood(item, &info, MOOD_ATTACK);

    int16_t angle = Creature_Turn(item, creature->maximum_turn);

    if (item->current_anim_state == WINSTON_STATE_STOP) {
        if (item->goal_anim_state != WINSTON_STATE_WALK
            && (info.distance > WINSTON_STOP_RANGE || info.ahead == 0)) {
            item->goal_anim_state = WINSTON_STATE_WALK;
            Sound_Effect(SFX_WINSTON_GRUNT_2, &item->pos, SPM_NORMAL);
        }
    } else if (info.distance < WINSTON_STOP_RANGE) {
        if (info.ahead != 0) {
            item->goal_anim_state = WINSTON_STATE_STOP;
            if ((creature->flags & 1) != 0) {
                creature->flags--;
            }
        } else if ((creature->flags & 1) == 0) {
            Sound_Effect(SFX_WINSTON_GRUNT_1, &item->pos, SPM_NORMAL);
            Sound_Effect(SFX_WINSTON_CUPS, &item->pos, SPM_NORMAL);
            creature->flags |= 1;
        }
    }

    if (item->touch_bits != 0 && (creature->flags & 2) == 0) {
        Sound_Effect(SFX_WINSTON_GRUNT_3, &item->pos, SPM_NORMAL);
        Sound_Effect(SFX_WINSTON_CUPS, &item->pos, SPM_NORMAL);
        creature->flags |= 2;
    } else if (item->touch_bits == 0 && (creature->flags & 2) != 0) {
        creature->flags -= 2;
    }

    if (Random_GetDraw() < 0x100) {
        Sound_Effect(SFX_WINSTON_CUPS, &item->pos, SPM_NORMAL);
    }

    Creature_Animate(item_num, angle, 0);
}

REGISTER_OBJECT(O_WINSTON, M_Setup)
