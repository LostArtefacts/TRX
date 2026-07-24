#include <trx/core/utils.h>
#include <trx/game/creature.h>
#include <trx/game/items/carrier.h>
#include <trx/game/lara.h>
#include <trx/game/objects/common.h>
#include <trx/game/objects/property.h>
#include <trx/game/pathing.h>
#include <trx/game/random.h>
#include <trx/game/sound.h>
#include <trx/game/spawn.h>

// clang-format off
#define M_HIT_POINTS     5
#define M_DAMAGE         25
#define M_TURN           (DEG_1 * 8) // = 1456
#define M_RADIUS         (WALL_L / 10) // = 102
#define M_ATTACK_2_RANGE SQUARE(WALL_L / 2) // = 262144
#define M_ATTACK_3_RANGE SQUARE(WALL_L / 5) // = 41616
// clang-format on

typedef enum {
    M_STATE_EMPTY,
    M_STATE_STOP,
    M_STATE_WALK_1,
    M_STATE_WALK_2,
    M_STATE_ATTACK_1,
    M_STATE_ATTACK_2,
    M_STATE_ATTACK_3,
    M_STATE_DEATH,
} M_STATE;

typedef enum {
    M_ANIM_LEAP = 2,
} M_ANIM;

static const BITE m_SpiderBite = {
    .pos = { .x = 0, .y = 0, .z = 41 },
    .mesh_num = 1,
};

static int32_t M_GetDamage(const ITEM *const item)
{
    TRX_VALUE damage = {};
    if (ObjectProperty_GetItemValue(item, "damage", &damage)) {
        return damage.as_int;
    }

    return M_DAMAGE;
}

static void M_Leap(const int16_t item_num, const int16_t angle)
{
    ITEM *const item = Item_Get(item_num);
    const XYZ_32 old_pos = item->pos;
    const int16_t old_room_num = item->room_num;

    Creature_Animate(item_num, angle, 0);
    if (item->pos.y > old_pos.y - STEP_L * 3 / 2) {
        return;
    }

    item->pos = old_pos;
    Item_UpdateRoom(item_num, old_room_num);
    Item_SwitchToAnim(item, M_ANIM_LEAP, 0);
    item->current_anim_state = M_STATE_ATTACK_2;
    Creature_Animate(item_num, angle, 0);
}

static void M_Control(const int16_t item_num)
{
    if (!Creature_Activate(item_num)) {
        return;
    }

    int16_t angle = 0;
    ITEM *const item = Item_Get(item_num);
    CREATURE *const creature = item->creature_data;

    if (item->hit_points > 0) {
        AI_INFO info;
        Creature_AIInfo(item, &info);
        Creature_Mood(item, &info, false);

        angle = Creature_Turn(item, M_TURN);

        switch (item->current_anim_state) {
        case M_STATE_STOP:
            creature->flags = 0;
            if (creature->mood == MOOD_BORED) {
                if (Random_GetControl() < 256) {
                    item->goal_anim_state = M_STATE_WALK_1;
                }
            } else if (info.ahead && item->touch_bits != 0) {
                item->goal_anim_state = M_STATE_ATTACK_1;
            } else if (creature->mood == MOOD_STALK) {
                item->goal_anim_state = M_STATE_WALK_1;
            } else if (
                creature->mood == MOOD_ESCAPE
                || creature->mood == MOOD_ATTACK) {
                item->goal_anim_state = M_STATE_WALK_2;
            }
            break;

        case M_STATE_WALK_1:
            if (creature->mood == MOOD_BORED) {
                if (Random_GetControl() < 256) {
                    item->goal_anim_state = M_STATE_STOP;
                }
            } else if (
                creature->mood == MOOD_ESCAPE
                || creature->mood == MOOD_ATTACK) {
                item->goal_anim_state = M_STATE_WALK_2;
            }
            break;

        case M_STATE_WALK_2:
            creature->flags = 0;
            if (creature->mood == MOOD_BORED || creature->mood == MOOD_STALK) {
                item->goal_anim_state = M_STATE_WALK_1;
            } else if (info.ahead && item->touch_bits != 0) {
                item->goal_anim_state = M_STATE_STOP;
            } else if (info.ahead && info.distance < M_ATTACK_3_RANGE) {
                item->goal_anim_state = M_STATE_ATTACK_3;
            } else if (info.ahead && info.distance < M_ATTACK_2_RANGE) {
                item->goal_anim_state = M_STATE_ATTACK_2;
            }
            break;

        case M_STATE_ATTACK_1:
        case M_STATE_ATTACK_2:
        case M_STATE_ATTACK_3:
            if (creature->flags == 0 && item->touch_bits != 0) {
                Creature_Effect(item, &m_SpiderBite, Spawn_Blood);
                Lara_TakeDamage(M_GetDamage(item), true);
                creature->flags = 1;
            }
            break;

        default:
            break;
        }
    } else if (Item_Shatter(item_num, -1, 0)) {
        LOT_DisableBaddieAI(item_num);
        Item_Destroy(item_num);
        Item_SetFinished(item, true);
        Sound_Effect(SFX_SPIDER_EXPLODE, &item->pos, SPM_NORMAL);
        Carrier_TestItemDrops(item_num);
        return;
    }

    M_Leap(item_num, angle);
}

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->control_func = M_Control;
    obj->collision_func = Creature_Collision;

    obj->radius = M_RADIUS;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->lot_setup = LOT_Setup(LOT_SETUP_JUMPER);

    obj->intelligent = true;
    obj->save_position = true;
    obj->save_hitpoints = true;
    obj->save_flags = true;
    obj->save_anim = true;

    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY_INT(
            "max_hit_points", M_HIT_POINTS, "Maximum hit points."),
        OBJECT_PROPERTY_INT(
            "damage", M_DAMAGE, "Damage dealt by bite attacks."));
}

REGISTER_OBJECT(O_SPIDER, M_Setup)
