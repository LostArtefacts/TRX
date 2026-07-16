#include <trx/game/creature.h>
#include <trx/game/lara.h>
#include <trx/game/objects/common.h>
#include <trx/game/objects/property.h>
#include <trx/game/pathing.h>
#include <trx/game/spawn.h>

// clang-format off
#define M_HIT_POINTS 10
#define M_DAMAGE     5
#define M_RADIUS     (WALL_L / 10) // = 102
#define M_TURN       (DEG_1 * 90) // = 16380
// clang-format on

typedef enum {
    M_STATE_EMPTY,
    M_STATE_MOVE,
    M_STATE_STOP,
} M_STATE;

static int32_t M_GetDamage(const ITEM *const item)
{
    OBJECT_PROPERTY_VALUE damage = {};
    if (ObjectProperty_GetItemValue(item, "damage", &damage)) {
        return damage.as_int;
    }

    return M_DAMAGE;
}

static void M_Control(const int16_t item_num)
{
    if (!Creature_Activate(item_num)) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    CREATURE *const creature = item->creature_data;

    if (item->hit_points <= 0) {
        if (Item_Shatter(item_num, -1, 0)) {
            LOT_DisableBaddieAI(item_num);
            Item_Kill(item_num);
            item->status = IS_DEACTIVATED;
        }
    } else {
        AI_INFO info;
        Creature_AIInfo(item, &info);
        Creature_Mood(item, &info, false);

        int16_t angle = Creature_Turn(item, M_TURN);

        switch (item->current_anim_state) {
        case M_STATE_STOP:
            if (creature->mood != MOOD_BORED) {
                item->goal_anim_state = M_STATE_MOVE;
            }
            break;

        case M_STATE_MOVE:
            if (creature->mood == MOOD_BORED || item->touch_bits != 0) {
                item->goal_anim_state = M_STATE_STOP;
            }
            break;
        }

        if (item->touch_bits != 0) {
            Lara_TakeDamage(M_GetDamage(item), true);
        }

        Creature_Head(item, 0);
        Creature_Animate(item_num, angle, 0);
        Creature_Underwater(item, 0);
    }
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
    obj->lot_setup = LOT_Setup(LOT_SETUP_FLYER);

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
            "damage", M_DAMAGE, "Damage dealt by the jelly sting."));
}

REGISTER_OBJECT(O_JELLY, M_Setup)
