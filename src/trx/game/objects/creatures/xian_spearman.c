#include <trx/core/utils.h>
#include <trx/debug.h>
#include <trx/game/creature.h>
#include <trx/game/interpolation.h>
#include <trx/game/items/carrier.h>
#include <trx/game/lara.h>
#include <trx/game/objects.h>
#include <trx/game/objects/creatures/xian_common.h>
#include <trx/game/objects/property.h>
#include <trx/game/pathing.h>
#include <trx/game/random.h>
#include <trx/game/sound.h>
#include <trx/game/spawn.h>

// clang-format off
#define M_HIT_POINTS     100
#define M_HIT_1_DAMAGE   75
#define M_HIT_2_DAMAGE   75
#define M_HIT_5_DAMAGE   75
#define M_HIT_6_DAMAGE   120
#define M_RADIUS         (WALL_L / 5) // = 204
#define M_TOUCH_L_BITS   0b00000000'00001000'00000000 // = 0x00800
#define M_TOUCH_R_BITS   0b00000100'00000000'00000000 // = 0x40000
#define M_WALK_TURN      (DEG_1 * 3) // = 546
#define M_RUN_TURN       (DEG_1 * 5) // = 910
#define M_ATTACK_1_RANGE SQUARE(WALL_L) // = 1048576
#define M_ATTACK_2_RANGE SQUARE(WALL_L * 3 / 2) // = 2359296
#define M_ATTACK_3_RANGE SQUARE(WALL_L * 2) // = 4194304
#define M_ATTACK_4_RANGE SQUARE(WALL_L * 2) // = 4194304
#define M_ATTACK_5_RANGE SQUARE(WALL_L) // = 1048576
#define M_ATTACK_6_RANGE SQUARE(WALL_L * 2) // = 4194304
#define M_RUN_RANGE      SQUARE(WALL_L * 3) // = 9437184
#define M_STOP_CHANCE    0x200
#define M_WALK_CHANCE    (M_STOP_CHANCE + 0x200) // = 0x400
// clang-format on

typedef enum {
    M_STATE_EMPTY,
    M_STATE_STOP,
    M_STATE_STOP_2,
    M_STATE_WALK,
    M_STATE_RUN,
    M_STATE_AIM_1,
    M_STATE_HIT_1,
    M_STATE_AIM_2,
    M_STATE_HIT_2,
    M_STATE_AIM_3,
    M_STATE_HIT_3,
    M_STATE_AIM_4,
    M_STATE_HIT_4,
    M_STATE_AIM_5,
    M_STATE_HIT_5,
    M_STATE_AIM_6,
    M_STATE_HIT_6,
    M_STATE_DEATH,
    M_STATE_START,
    M_STATE_KILL,
} M_STATE;

typedef enum {
    // clang-format off
    M_ANIM_DEATH = 0,
    M_ANIM_START = 48,
    M_ANIM_KILL  = 49,
    // clang-format on
} M_ANIM;

static const BITE m_XianSpearmanLeftSpear = {
    .pos = { .x = 0, .y = 0, .z = 920 },
    .mesh_num = 11,
};

static const BITE m_XianSpearmanRightSpear = {
    .pos = { .x = 0, .y = 0, .z = 920 },
    .mesh_num = 18,
};

static int32_t M_GetDamage(
    const ITEM *const item, const char *const key, const int32_t default_value)
{
    TRX_VALUE damage = {};
    if (ObjectProperty_GetItemValue(item, key, &damage)) {
        return damage.as_int;
    }

    return default_value;
}

static void M_DoDamage(
    const ITEM *const item, CREATURE *const creature, const int32_t damage)
{
    if ((creature->flags & 1) == 0
        && (item->touch_bits & M_TOUCH_R_BITS) != 0) {
        Lara_TakeDamage(damage, true);
        Creature_Effect(item, &m_XianSpearmanRightSpear, Spawn_Blood);
        creature->flags |= 1;
        Sound_Effect(SFX_CRUNCH_2, &item->pos, SPM_NORMAL);
    }

    if ((creature->flags & 2) == 0
        && (item->touch_bits & M_TOUCH_L_BITS) != 0) {
        Lara_TakeDamage(damage, true);
        Creature_Effect(item, &m_XianSpearmanLeftSpear, Spawn_Blood);
        creature->flags |= 2;
        Sound_Effect(SFX_CRUNCH_2, &item->pos, SPM_NORMAL);
    }
}

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    Item_SwitchToAnim(item, M_ANIM_START, 0);
    item->goal_anim_state = M_STATE_START;
    item->current_anim_state = M_STATE_START;
    item->status = IS_INACTIVE;
    item->mesh_bits = 0;
}

static void M_Control(const int16_t item_num)
{
    if (!Creature_Activate(item_num)) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    CREATURE *const creature = item->creature_data;

    int16_t head = 0;
    int16_t neck = 0;
    int16_t angle = 0;

    const ITEM *const lara_item = Lara_GetItem();
    const bool lara_was_alive = lara_item->hit_points > 0;

    if (item->hit_points <= 0) {
        item->current_anim_state = M_STATE_DEATH;
        item->mesh_bits >>= 1;
        item->enable_interpolation = false;
        if (item->mesh_bits == 0) {
            Sound_Effect(SFX_EXPLOSION_1, nullptr, SPM_NORMAL);
            item->mesh_bits = -1;
            item->object_id = O_XIAN_SPEARMAN_STATUE;
            Item_Shatter(item_num, -1, 0);
            item->object_id = O_XIAN_SPEARMAN;
            LOT_DisableBaddieAI(item_num);
            Item_Destroy(item_num);
            item->status = IS_DEACTIVATED;
            item->flags |= IF_ONE_SHOT;
            Carrier_TestItemDrops(item_num);
        }
        return;
    }

    AI_INFO info;
    Creature_AIInfo(item, &info);
    Creature_Mood(item, &info, true);

    angle = Creature_Turn(item, creature->maximum_turn);
    if (item->current_anim_state != M_STATE_START) {
        item->mesh_bits = -1;
    }

    switch (item->current_anim_state) {
    case M_STATE_START:
        if (creature->flags == 0) {
            item->mesh_bits = (item->mesh_bits << 1) | 1;
            creature->flags = 3;
        } else {
            creature->flags--;
        }
        break;

    case M_STATE_STOP:
        if (info.ahead) {
            neck = info.angle;
        }
        creature->maximum_turn = 0;
        if (creature->mood == MOOD_BORED) {
            const int32_t random = Random_GetControl();
            if (random < M_STOP_CHANCE) {
                item->goal_anim_state = M_STATE_STOP_2;
            } else if (random < M_WALK_CHANCE) {
                item->goal_anim_state = M_STATE_WALK;
            }
        } else if (info.ahead && info.distance < M_ATTACK_1_RANGE) {
            item->goal_anim_state = M_STATE_AIM_1;
        } else {
            item->goal_anim_state = M_STATE_WALK;
        }
        break;

    case M_STATE_STOP_2:
        if (info.ahead) {
            neck = info.angle;
        }
        creature->maximum_turn = 0;
        if (creature->mood == MOOD_ESCAPE) {
            item->goal_anim_state = M_STATE_WALK;
        } else if (creature->mood == MOOD_BORED) {
            const int32_t random = Random_GetControl();
            if (random < M_STOP_CHANCE) {
                item->goal_anim_state = M_STATE_STOP;
            } else if (random < M_WALK_CHANCE) {
                item->goal_anim_state = M_STATE_WALK;
            }
        } else if (info.ahead && info.distance < M_ATTACK_5_RANGE) {
            item->goal_anim_state = M_STATE_AIM_5;
        } else {
            item->goal_anim_state = M_STATE_WALK;
        }
        break;

    case M_STATE_WALK:
        if (info.ahead) {
            neck = info.angle;
        }
        creature->maximum_turn = M_WALK_TURN;
        if (creature->mood == MOOD_ESCAPE) {
            item->goal_anim_state = M_STATE_RUN;
        } else if (creature->mood == MOOD_BORED) {
            const int32_t random = Random_GetControl();
            if (random < M_STOP_CHANCE) {
                item->goal_anim_state = M_STATE_STOP;
            } else if (random < M_WALK_CHANCE) {
                item->goal_anim_state = M_STATE_STOP_2;
            }
        } else if (info.ahead && info.distance < M_ATTACK_4_RANGE) {
            if (info.distance < M_ATTACK_2_RANGE) {
                item->goal_anim_state = M_STATE_AIM_2;
            } else {
                if (Random_GetControl() < 0x4000) {
                    item->goal_anim_state = M_STATE_AIM_3;
                } else {
                    item->goal_anim_state = M_STATE_AIM_4;
                }
            }
        } else if (!info.ahead || info.distance > M_RUN_RANGE) {
            item->goal_anim_state = M_STATE_RUN;
        }
        break;

    case M_STATE_RUN:
        if (info.ahead) {
            neck = info.angle;
        }
        creature->maximum_turn = M_RUN_TURN;
        if (creature->mood == MOOD_ESCAPE) {
        } else if (creature->mood == MOOD_BORED) {
            if (Random_GetControl() < 0x4000) {
                item->goal_anim_state = M_STATE_STOP;
            } else {
                item->goal_anim_state = M_STATE_STOP_2;
            }
        } else if (info.ahead && info.distance < M_ATTACK_6_RANGE) {
            item->goal_anim_state = M_STATE_AIM_6;
        }
        break;

    case M_STATE_AIM_1:
        if (info.ahead) {
            head = info.angle;
        }
        creature->flags = 0;
        if (!info.ahead || info.distance > M_ATTACK_1_RANGE) {
            item->goal_anim_state = M_STATE_STOP;
        } else {
            item->goal_anim_state = M_STATE_HIT_1;
        }
        break;

    case M_STATE_AIM_2:
        if (info.ahead) {
            head = info.angle;
        }
        creature->flags = 0;
        if (!info.ahead || info.distance > M_ATTACK_2_RANGE) {
            item->goal_anim_state = M_STATE_WALK;
        } else {
            item->goal_anim_state = M_STATE_HIT_2;
        }
        break;

    case M_STATE_AIM_3:
        if (info.ahead) {
            head = info.angle;
        }
        creature->flags = 0;
        if (!info.ahead || info.distance > M_ATTACK_3_RANGE) {
            item->goal_anim_state = M_STATE_WALK;
        } else {
            item->goal_anim_state = M_STATE_HIT_2;
        }
        break;

    case M_STATE_AIM_4:
        if (info.ahead) {
            head = info.angle;
        }
        creature->flags = 0;
        if (!info.ahead || info.distance > M_ATTACK_4_RANGE) {
            item->goal_anim_state = M_STATE_WALK;
        } else {
            item->goal_anim_state = M_STATE_HIT_2;
        }
        break;

    case M_STATE_AIM_5:
        if (info.ahead) {
            head = info.angle;
        }
        creature->flags = 0;
        if (!info.ahead || info.distance > M_ATTACK_5_RANGE) {
            item->goal_anim_state = M_STATE_STOP_2;
        } else {
            item->goal_anim_state = M_STATE_HIT_5;
        }
        break;

    case M_STATE_AIM_6:
        if (info.ahead) {
            head = info.angle;
        }
        creature->flags = 0;
        if (!info.ahead || info.distance > M_ATTACK_6_RANGE) {
            item->goal_anim_state = M_STATE_RUN;
        } else {
            item->goal_anim_state = M_STATE_HIT_6;
        }
        break;

    case M_STATE_HIT_1:
        M_DoDamage(
            item, creature, M_GetDamage(item, "hit_1_damage", M_HIT_1_DAMAGE));
        break;

    case M_STATE_HIT_2:
    case M_STATE_HIT_3:
    case M_STATE_HIT_4:
        if (info.ahead) {
            head = info.angle;
        }
        M_DoDamage(
            item, creature, M_GetDamage(item, "hit_2_damage", M_HIT_2_DAMAGE));
        if (info.ahead && info.distance < M_ATTACK_1_RANGE) {
            const int32_t random = Random_GetControl();
            if (random < 0x4000) {
                item->goal_anim_state = M_STATE_STOP;
            } else {
                item->goal_anim_state = M_STATE_STOP_2;
            }
        } else {
            item->goal_anim_state = M_STATE_WALK;
        }
        break;

    case M_STATE_HIT_5:
        if (info.ahead) {
            head = info.angle;
        }
        M_DoDamage(
            item, creature, M_GetDamage(item, "hit_5_damage", M_HIT_5_DAMAGE));
        if (info.ahead && info.distance < M_ATTACK_1_RANGE) {
            item->goal_anim_state = M_STATE_STOP;
        } else {
            item->goal_anim_state = M_STATE_STOP_2;
        }
        break;

    case M_STATE_HIT_6:
        if (info.ahead) {
            head = info.angle;
        }
        M_DoDamage(
            item, creature, M_GetDamage(item, "hit_6_damage", M_HIT_6_DAMAGE));
        if (info.ahead && info.distance < M_ATTACK_1_RANGE) {
            const int32_t random = Random_GetControl();
            if (random < 0x4000) {
                item->goal_anim_state = M_STATE_STOP;
            } else {
                item->goal_anim_state = M_STATE_STOP_2;
            }
        } else if (info.ahead && info.distance < M_ATTACK_4_RANGE) {
            item->goal_anim_state = M_STATE_WALK;
        } else {
            item->goal_anim_state = M_STATE_RUN;
        }
        break;

    default:
        break;
    }

    if (lara_was_alive && lara_item->hit_points <= 0) {
        Creature_SpecialKill(
            item, M_ANIM_KILL, M_STATE_KILL, LS_EXTRA_GUARD_KILL);
        return;
    }

    Creature_Tilt(item, 0);
    Creature_Head(item, head);
    Creature_Neck(item, neck);
    Creature_Animate(item_num, angle, 0);
}

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    SOFT_ASSERT(
        Object_Get(O_XIAN_SPEARMAN_STATUE)->loaded,
        "Xian spearman statue object missing");

    obj->initialise_func = M_Initialise;
    obj->draw_func = XianWarrior_Draw;
    obj->control_func = M_Control;
    obj->collision_func = Creature_Collision;

    obj->radius = M_RADIUS;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->pivot_length = 0;

    obj->intelligent = true;
    obj->save_position = true;
    obj->save_hitpoints = true;
    obj->save_flags = true;
    obj->save_anim = true;

    Object_GetBone(obj, 6)->rot.y = true;
    Object_GetBone(obj, 12)->rot.y = true;
    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY_INT(
            "max_hit_points", M_HIT_POINTS, "Maximum hit points."),
        OBJECT_PROPERTY_INT(
            "hit_1_damage", M_HIT_1_DAMAGE, "Damage dealt by attack 1."),
        OBJECT_PROPERTY_INT(
            "hit_2_damage", M_HIT_2_DAMAGE, "Damage dealt by attacks 2 to 4."),
        OBJECT_PROPERTY_INT(
            "hit_5_damage", M_HIT_5_DAMAGE, "Damage dealt by attack 5."),
        OBJECT_PROPERTY_INT(
            "hit_6_damage", M_HIT_6_DAMAGE, "Damage dealt by attack 6."));
}

REGISTER_OBJECT(O_XIAN_SPEARMAN, M_Setup)
