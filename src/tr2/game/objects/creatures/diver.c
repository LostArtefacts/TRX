#include "game/objects/creatures/diver.h"

#include "game/creature.h"
#include "game/los.h"
#include "global/const.h"
#include "global/funcs.h"
#include "global/vars.h"

static BITE m_DiverBite = { .pos = { .x = 17, .y = 164, .z = 44, }, .mesh_num = 18 };

typedef enum {
    DIVER_ANIM_EMPTY = 0,
    DIVER_ANIM_SWIM_1 = 1,
    DIVER_ANIM_SWIM_2 = 2,
    DIVER_ANIM_SHOOT_1 = 3,
    DIVER_ANIM_AIM_1 = 4,
    DIVER_ANIM_NULL_1 = 5,
    DIVER_ANIM_AIM_2 = 6,
    DIVER_ANIM_SHOOT_2 = 7,
    DIVER_ANIM_NULL_2 = 8,
    DIVER_ANIM_DEATH = 9,
} DIVER_ANIM;

#define DIVER_SWIM_TURN (3 * PHD_DEGREE) // = 546
#define DIVER_FRONT_ARC PHD_45
#define DIVER_DIE_ANIM 16

void __cdecl Diver_Control(int16_t item_num)
{
    if (!Creature_Activate(item_num)) {
        return;
    }

    ITEM *const item = &g_Items[item_num];
    CREATURE *const creature = item->data;

    if (item->hit_points <= 0) {
        if (item->current_anim_state != DIVER_ANIM_DEATH) {
            item->anim_num = g_Objects[O_DIVER].anim_idx + DIVER_DIE_ANIM;
            item->frame_num = g_Anims[item->anim_num].frame_base;
            item->current_anim_state = DIVER_ANIM_DEATH;
        }
        Creature_Float(item_num);
        return;
    }

    AI_INFO info;
    Creature_AIInfo(item, &info);
    Creature_Mood(item, &info, MOOD_BORED);

    bool shoot;
    if (g_Lara.water_status == LWS_ABOVE_WATER) {
        GAME_VECTOR start;
        start.pos.x = item->pos.x;
        start.pos.y = item->pos.y - STEP_L;
        start.pos.z = item->pos.z;
        start.room_num = item->room_num;

        GAME_VECTOR target;
        target.pos.x = g_LaraItem->pos.x;
        target.pos.y = g_LaraItem->pos.y - (LARA_HEIGHT - 150);
        target.pos.z = g_LaraItem->pos.z;
        target.room_num = g_LaraItem->room_num;
        shoot = LOS_Check(&start, &target);

        if (shoot) {
            creature->target.x = g_LaraItem->pos.x;
            creature->target.y = g_LaraItem->pos.y;
            creature->target.z = g_LaraItem->pos.z;
        }

        if (info.angle < -DIVER_FRONT_ARC || info.angle > DIVER_FRONT_ARC) {
            shoot = false;
        }
    } else if (info.angle > -DIVER_FRONT_ARC && info.angle < DIVER_FRONT_ARC) {
        GAME_VECTOR start;
        start.pos.x = item->pos.x;
        start.pos.y = item->pos.y;
        start.pos.z = item->pos.z;
        start.room_num = item->room_num;

        GAME_VECTOR target;
        target.pos.x = g_LaraItem->pos.x;
        target.pos.y = g_LaraItem->pos.y;
        target.pos.z = g_LaraItem->pos.z;
        target.room_num = g_LaraItem->room_num;

        shoot = LOS_Check(&start, &target);
    } else {
        shoot = false;
    }

    int16_t head = 0;
    int16_t neck = 0;
    int16_t angle = Creature_Turn(item, creature->maximum_turn);
    int32_t water_level =
        Diver_GetWaterSurface(
            item->pos.x, item->pos.y, item->pos.z, item->room_num)
        + 512;

    switch (item->current_anim_state) {
    case DIVER_ANIM_SWIM_1:
        creature->maximum_turn = DIVER_SWIM_TURN;
        if (shoot) {
            neck = -info.angle;
        }
        if (creature->target.y < water_level
            && item->pos.y < water_level + creature->lot.fly) {
            item->goal_anim_state = DIVER_ANIM_SWIM_2;
        } else if (creature->mood != MOOD_ESCAPE && shoot) {
            item->goal_anim_state = DIVER_ANIM_AIM_1;
        }
        break;

    case DIVER_ANIM_SWIM_2:
        creature->maximum_turn = DIVER_SWIM_TURN;
        if (shoot) {
            head = info.angle;
        }
        if (creature->target.y > water_level) {
            item->goal_anim_state = DIVER_ANIM_SWIM_1;
        } else if (creature->mood != MOOD_ESCAPE && shoot) {
            item->goal_anim_state = DIVER_ANIM_AIM_2;
        }
        break;

    case DIVER_ANIM_SHOOT_1:
        if (shoot) {
            neck = -info.angle;
        }
        if (!creature->flags) {
            Creature_Effect(item, &m_DiverBite, Diver_Harpoon);
            creature->flags = 1;
        }
        break;

    case DIVER_ANIM_SHOOT_2:
        if (shoot) {
            head = info.angle;
        }
        if (!creature->flags) {
            Creature_Effect(item, &m_DiverBite, Diver_Harpoon);
            creature->flags = 1;
        }
        break;

    case DIVER_ANIM_AIM_1:
        creature->flags = 0;
        if (shoot) {
            neck = -info.angle;
        }
        if (!shoot || creature->mood == MOOD_ESCAPE
            || (creature->target.y < water_level
                && item->pos.y < water_level + creature->lot.fly)) {
            item->goal_anim_state = DIVER_ANIM_SWIM_1;
        } else {
            item->goal_anim_state = DIVER_ANIM_SHOOT_1;
        }
        break;

    case DIVER_ANIM_AIM_2:
        creature->flags = 0;
        if (shoot) {
            head = info.angle;
        }
        if (!shoot || creature->mood == MOOD_ESCAPE
            || creature->target.y > water_level) {
            item->goal_anim_state = DIVER_ANIM_SWIM_2;
        } else {
            item->goal_anim_state = DIVER_ANIM_SHOOT_2;
        }
        break;

    default:
        break;
    }

    Creature_Head(item, head);
    Creature_Neck(item, neck);

    Creature_Animate(item_num, angle, 0);

    switch (item->current_anim_state) {
    case DIVER_ANIM_SWIM_1:
    case DIVER_ANIM_AIM_1:
    case DIVER_ANIM_SHOOT_1:
        Creature_Underwater(item, WALL_L / 2);
        break;

    default:
        item->pos.y = water_level - WALL_L / 2;
        break;
    }
}
