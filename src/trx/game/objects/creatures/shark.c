#include <trx/core/utils.h>
#include <trx/game/creature.h>
#include <trx/game/items/carrier.h>
#include <trx/game/lara.h>
#include <trx/game/objects.h>
#include <trx/game/objects/property.h>
#include <trx/game/pathing.h>
#include <trx/game/random.h>
#include <trx/game/spawn.h>

// clang-format off
#define M_HIT_POINTS      30
#define M_DAMAGE          400
#define M_TOUCH_BITS      0b00110100'00000000 // = 0x3400
#define M_RADIUS          (WALL_L / 3) // = 341
#define M_SWIM_2_RANGE    SQUARE(WALL_L * 3) // = 9437184
#define M_ATTACK_1_RANGE  SQUARE(WALL_L * 3 / 4) // = 589824
#define M_ATTACK_2_RANGE  SQUARE(WALL_L * 4 / 3) // = 1863225
#define M_SWIM_1_TURN     (DEG_1 / 2) // = 91
#define M_SWIM_2_TURN     (DEG_1 * 2) // = 364
#define M_ATTACK_1_CHANCE 0x800
// clang-format on

typedef enum {
    M_STATE_STOP,
    M_STATE_SWIM_1,
    M_STATE_SWIM_2,
    M_STATE_ATTACK_1,
    M_STATE_ATTACK_2,
    M_STATE_DEATH,
    M_STATE_KILL,
} M_STATE;

typedef enum {
    // clang-format off
    M_ANIM_DEATH = 4,
    M_ANIM_KILL  = 19,
    // clang-format on
} M_ANIM;

typedef struct {
    int32_t damage;
} M_PRIV;

static const BITE m_SharkBite = {
    .pos = { .x = 17, .y = -22, .z = 344 },
    .mesh_num = 12,
};

static void M_Control(const int16_t item_num)
{
    if (!Creature_Activate(item_num)) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    const M_PRIV *const p = item->priv;
    CREATURE *const creature = item->creature_data;

    const ITEM *const lara_item = Lara_GetItem();
    const bool lara_was_alive = lara_item->hit_points > 0;

    if (item->hit_points <= 0) {
        if (item->current_anim_state != M_STATE_DEATH) {
            Item_SwitchToAnim(item, M_ANIM_DEATH, 0);
            item->current_anim_state = M_STATE_DEATH;
            Carrier_TestItemDrops(item_num);
        }
        Creature_Float(item_num);
    } else {
        AI_INFO info;
        Creature_AIInfo(item, &info);
        Creature_Mood(item, &info, true);

        int16_t head = 0;
        int16_t angle = Creature_Turn(item, creature->maximum_turn);

        switch (item->current_anim_state) {
        case M_STATE_STOP:
            creature->flags = 0;
            creature->maximum_turn = 0;
            if (info.ahead && info.distance < M_ATTACK_1_RANGE
                && info.zone_num == info.enemy_zone_num) {
                item->goal_anim_state = M_STATE_ATTACK_1;
            } else {
                item->goal_anim_state = M_STATE_SWIM_1;
            }
            break;

        case M_STATE_SWIM_1:
            creature->maximum_turn = M_SWIM_1_TURN;
            if (creature->mood == MOOD_BORED) {
            } else if (info.ahead && info.distance < M_ATTACK_1_RANGE) {
                item->goal_anim_state = M_STATE_STOP;
            } else if (
                creature->mood == MOOD_ESCAPE || info.distance > M_SWIM_2_RANGE
                || !info.ahead) {
                item->goal_anim_state = M_STATE_SWIM_2;
            }
            break;

        case M_STATE_SWIM_2:
            creature->flags = 0;
            creature->maximum_turn = M_SWIM_2_TURN;
            if (creature->mood == MOOD_BORED) {
                item->goal_anim_state = M_STATE_SWIM_1;
            } else if (creature->mood == MOOD_ESCAPE) {
            } else if (
                info.ahead && info.distance < M_ATTACK_2_RANGE
                && info.zone_num == info.enemy_zone_num) {
                if (Random_GetControl() < M_ATTACK_1_CHANCE) {
                    item->goal_anim_state = M_STATE_STOP;
                } else if (info.distance < M_ATTACK_1_RANGE) {
                    item->goal_anim_state = M_STATE_ATTACK_2;
                }
            }
            break;

        case M_STATE_ATTACK_1:
        case M_STATE_ATTACK_2:
            if (info.ahead) {
                head = info.angle;
            }

            if (creature->flags == 0
                && (item->touch_bits & M_TOUCH_BITS) != 0) {
                Lara_TakeDamage(p->damage, true);
                Creature_Effect(item, &m_SharkBite, Spawn_Blood);
                creature->flags = 1;
            }
            break;

        default:
            break;
        }

        if (lara_was_alive && lara_item->hit_points <= 0) {
            Creature_SpecialKill(
                item, M_ANIM_KILL, M_STATE_KILL, LS_EXTRA_SHARK_KILL);
        } else if (item->current_anim_state == M_STATE_KILL) {
            Item_Animate(item);
        } else {
            Creature_Head(item, head);
            Creature_Animate(item_num, angle, 0);
            Creature_Underwater(item, M_RADIUS);
        }
    }
}

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->priv_size = sizeof(M_PRIV);
    obj->control_func = M_Control;
    obj->draw_func = Object_DrawUnclippedItem;
    obj->collision_func = Creature_Collision;

    obj->radius = M_RADIUS;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->pivot_length = 200;
    obj->lot_setup = LOT_Setup(LOT_SETUP_FLYER);
    obj->lot_setup.block_mask = BOX_BLOCKABLE;

    obj->intelligent = true;
    obj->save_position = true;
    obj->save_hitpoints = true;
    obj->save_flags = true;
    obj->save_anim = true;

    Object_GetBone(obj, 9)->rot.y = true;
    OBJECT_PROPERTIES(
        obj, ITEM_PROPERTY_MAX_HIT_POINTS(M_HIT_POINTS),
        OBJECT_PROPERTY(
            M_PRIV, damage, M_DAMAGE, "Damage dealt by bite attacks."));
}

REGISTER_OBJECT(O_SHARK, M_Setup)
