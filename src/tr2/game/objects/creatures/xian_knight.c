#include "game/objects/creatures/xian_knight.h"

#include "game/creature.h"
#include "game/effects.h"
#include "game/items.h"
#include "game/lara/control.h"
#include "game/lot.h"
#include "game/objects/common.h"
#include "game/objects/creatures/xian_common.h"
#include "game/random.h"
#include "game/sound.h"
#include "game/spawn.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/debug.h>
#include <libtrx/utils.h>

// clang-format off
#define XIAN_KNIGHT_HITPOINTS      80
#define XIAN_KNIGHT_TOUCH_BITS     0b11000000'00000000 // = 0xC000
#define XIAN_KNIGHT_RADIUS         (WALL_L / 5) // = 204
#define XIAN_KNIGHT_HACK_DAMAGE    300
#define XIAN_KNIGHT_WALK_TURN      (DEG_1 * 5) // = 910
#define XIAN_KNIGHT_FLY_TURN       (DEG_1 * 4) // = 728
#define XIAN_KNIGHT_ATTACK_1_RANGE SQUARE(WALL_L) // = 1048576
#define XIAN_KNIGHT_ATTACK_3_RANGE SQUARE(WALL_L * 2) // = 4194304
// clang-format on

typedef enum {
    // clang-format off
    XIAN_KNIGHT_STATE_EMPTY   = 0,
    XIAN_KNIGHT_STATE_STOP    = 1,
    XIAN_KNIGHT_STATE_WALK    = 2,
    XIAN_KNIGHT_STATE_AIM_1   = 3,
    XIAN_KNIGHT_STATE_SLASH_1 = 4,
    XIAN_KNIGHT_STATE_AIM_2   = 5,
    XIAN_KNIGHT_STATE_SLASH_2 = 6,
    XIAN_KNIGHT_STATE_WAIT    = 7,
    XIAN_KNIGHT_STATE_FLY     = 8,
    XIAN_KNIGHT_STATE_START   = 9,
    XIAN_KNIGHT_STATE_AIM_3   = 10,
    XIAN_KNIGHT_STATE_SLASH_3 = 11,
    XIAN_KNIGHT_STATE_DEATH   = 12,
    // clang-format on
} XIAN_KNIGHT_STATE;

static const BITE m_XianKnightSword = {
    .pos = { .x = 0, .y = 37, .z = 550 },
    .mesh_num = 15,
};

static void M_Initialise(int16_t item_num);

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    item->status = IS_INACTIVE;
    item->mesh_bits = 0;
}

void XianKnight_SparkleTrail(const ITEM *const item)
{
    const int16_t effect_num = Effect_Create(item->room_num);
    if (effect_num != NO_EFFECT) {
        EFFECT *const effect = Effect_Get(effect_num);
        effect->object_id = O_TWINKLE;
        effect->pos.x = item->pos.x + (Random_GetDraw() << 8 >> 15) - 128;
        effect->pos.y = item->pos.y + (Random_GetDraw() << 8 >> 15) - 256;
        effect->pos.z = item->pos.z + (Random_GetDraw() << 8 >> 15) - 128;
        effect->room_num = item->room_num;
        effect->counter = -30;
        effect->frame_num = 0;
    }
    Sound_Effect(SFX_WARRIOR_HOVER, &item->pos, SPM_NORMAL);
}

void XianKnight_Setup(void)
{
    OBJECT *const obj = Object_Get(O_XIAN_KNIGHT);
    if (!obj->loaded) {
        return;
    }

    ASSERT(Object_Get(O_XIAN_KNIGHT_STATUE)->loaded);

    obj->initialise_func = M_Initialise;
    obj->draw_func = XianWarrior_Draw;
    obj->control_func = XianKnight_Control;
    obj->collision_func = Creature_Collision;

    obj->hit_points = XIAN_KNIGHT_HITPOINTS;
    obj->radius = XIAN_KNIGHT_RADIUS;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->pivot_length = 0;

    obj->intelligent = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_flags = 1;
    obj->save_anim = 1;

    Object_GetBone(obj, 6)->rot_y = true;
    Object_GetBone(obj, 16)->rot_y = true;
}

void XianKnight_Control(const int16_t item_num)
{
    if (!Creature_Activate(item_num)) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    CREATURE *const creature = item->data;

    int16_t head = 0;
    int16_t neck = 0;
    int16_t angle = 0;

    if (item->hit_points <= 0) {
        item->current_anim_state = XIAN_KNIGHT_STATE_DEATH;
        item->mesh_bits >>= 1;
        if (item->mesh_bits == 0) {
            Sound_Effect(SFX_EXPLOSION_1, nullptr, SPM_NORMAL);
            item->mesh_bits = -1;
            item->object_id = O_XIAN_KNIGHT_STATUE;
            Item_Explode(item_num, -1, 0);
            item->object_id = O_XIAN_KNIGHT;
            LOT_DisableBaddieAI(item_num);
            Item_Kill(item_num);
            item->status = IS_DEACTIVATED;
            item->flags |= IF_ONE_SHOT;
        }
        return;
    }

    creature->lot.step = STEP_L;
    creature->lot.drop = -STEP_L;
    creature->lot.fly = 0;
    AI_INFO info;
    Creature_AIInfo(item, &info);
    if (item->current_anim_state == XIAN_KNIGHT_STATE_FLY
        && info.zone_num != info.enemy_zone_num) {
        creature->lot.step = WALL_L * 20;
        creature->lot.drop = -WALL_L * 20;
        creature->lot.fly = STEP_L / 4;
        Creature_AIInfo(item, &info);
    }
    Creature_Mood(item, &info, MOOD_ATTACK);

    angle = Creature_Turn(item, creature->maximum_turn);
    if (item->current_anim_state != XIAN_KNIGHT_STATE_START) {
        item->mesh_bits = -1;
    }

    switch (item->current_anim_state) {
    case XIAN_KNIGHT_STATE_START:
        if (creature->flags == 0) {
            item->mesh_bits = (item->mesh_bits << 1) | 1;
            creature->flags = 3;
        } else {
            creature->flags--;
        }
        break;

    case XIAN_KNIGHT_STATE_STOP:
        creature->maximum_turn = 0;
        if (info.ahead != 0) {
            neck = info.angle;
        }
        if (g_LaraItem->hit_points <= 0) {
            item->goal_anim_state = XIAN_KNIGHT_STATE_WAIT;
        } else if (
            info.bite != 0 && info.distance < XIAN_KNIGHT_ATTACK_1_RANGE) {
            if (Random_GetControl() < 0x4000) {
                item->goal_anim_state = XIAN_KNIGHT_STATE_AIM_1;
            } else {
                item->goal_anim_state = XIAN_KNIGHT_STATE_AIM_2;
            }
        } else if (info.zone_num != info.enemy_zone_num) {
            item->goal_anim_state = XIAN_KNIGHT_STATE_FLY;
        } else {
            item->goal_anim_state = XIAN_KNIGHT_STATE_WALK;
        }
        break;

    case XIAN_KNIGHT_STATE_WALK:
        creature->maximum_turn = XIAN_KNIGHT_WALK_TURN;
        if (info.ahead != 0) {
            neck = info.angle;
        }
        if (g_LaraItem->hit_points <= 0) {
            item->goal_anim_state = XIAN_KNIGHT_STATE_STOP;
        } else if (
            info.bite != 0 && info.distance < XIAN_KNIGHT_ATTACK_3_RANGE) {
            item->goal_anim_state = XIAN_KNIGHT_STATE_AIM_3;
        } else if (info.zone_num != info.enemy_zone_num) {
            item->goal_anim_state = XIAN_KNIGHT_STATE_STOP;
        }
        break;

    case XIAN_KNIGHT_STATE_FLY:
        creature->maximum_turn = XIAN_KNIGHT_FLY_TURN;
        if (info.ahead != 0) {
            neck = info.angle;
        }
        XianKnight_SparkleTrail(item);
        if (creature->lot.fly == 0) {
            item->goal_anim_state = XIAN_KNIGHT_STATE_STOP;
        }
        break;

    case XIAN_KNIGHT_STATE_AIM_1:
        creature->flags = 0;
        if (info.ahead != 0) {
            head = info.angle;
        }
        if (info.bite != 0 && info.distance < XIAN_KNIGHT_ATTACK_1_RANGE) {
            item->goal_anim_state = XIAN_KNIGHT_STATE_SLASH_1;
        } else {
            item->goal_anim_state = XIAN_KNIGHT_STATE_STOP;
        }
        break;

    case XIAN_KNIGHT_STATE_AIM_2:
        creature->flags = 0;
        if (info.ahead != 0) {
            head = info.angle;
        }
        if (info.bite != 0 && info.distance < XIAN_KNIGHT_ATTACK_1_RANGE) {
            item->goal_anim_state = XIAN_KNIGHT_STATE_SLASH_2;
        } else {
            item->goal_anim_state = XIAN_KNIGHT_STATE_STOP;
        }
        break;

    case XIAN_KNIGHT_STATE_AIM_3:
        creature->flags = 0;
        if (info.ahead != 0) {
            head = info.angle;
        }
        if (info.bite != 0 && info.distance < XIAN_KNIGHT_ATTACK_3_RANGE) {
            item->goal_anim_state = XIAN_KNIGHT_STATE_SLASH_3;
        } else {
            item->goal_anim_state = XIAN_KNIGHT_STATE_WALK;
        }
        break;

    case XIAN_KNIGHT_STATE_SLASH_1:
    case XIAN_KNIGHT_STATE_SLASH_2:
    case XIAN_KNIGHT_STATE_SLASH_3:
        if (info.ahead != 0) {
            head = info.angle;
        }
        if (creature->flags == 0
            && (item->touch_bits & XIAN_KNIGHT_TOUCH_BITS) != 0) {
            Lara_TakeDamage(XIAN_KNIGHT_HACK_DAMAGE, true);
            Creature_Effect(item, &m_XianKnightSword, Spawn_Blood);
            creature->flags = 1;
        }
        break;

    default:
        break;
    }

    Creature_Tilt(item, 0);
    Creature_Head(item, head);
    Creature_Neck(item, neck);
    Creature_Animate(item_num, angle, 0);
}
