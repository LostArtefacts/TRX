#include <trx/core/utils.h>
#include <trx/game/creature.h>
#include <trx/game/lara.h>
#include <trx/game/objects/common.h>
#include <trx/game/objects/property.h>
#include <trx/game/random.h>
#include <trx/game/sound.h>
#include <trx/game/spawn.h>

// clang-format off
#define M_HIT_POINTS        30
#define M_BIFF_DAMAGE       150
#define M_BIFF_ENEMY_DAMAGE 5
#define M_RADIUS            (WALL_L / 10) // = 102
#define M_WALK_TURN         (DEG_1 * 3) // = 546
#define M_RUN_TURN          (DEG_1 * 4) // = 728
#define M_RUN_TURN_FAST     (DEG_1 * 5) // = 910
#define M_CLOSE_RANGE       SQUARE(WALL_L / 2) // = 262144
#define M_LONG_RANGE        SQUARE(WALL_L) // = 1048576
#define M_ATTACK_5_RANGE    SQUARE(WALL_L * 3) // = 9437184
#define M_WALK_RANGE        SQUARE(WALL_L * 2) // = 4194304
#define M_HIT_RANGE         (STEP_L * 2) // = 512
#define M_TOUCH_BITS        0b01000000'00000000 // = 0x4000
// clang-format on

typedef enum {
    M_STATE_EMPTY,
    M_STATE_WAIT_1,
    M_STATE_WALK,
    M_STATE_RUN,
    M_STATE_ATTACK_1,
    M_STATE_ATTACK_2,
    M_STATE_ATTACK_3,
    M_STATE_ATTACK_4,
    M_STATE_AIM_3,
    M_STATE_DEATH,
    M_STATE_ATTACK_5,
    M_STATE_WAIT_2,
} M_STATE;

typedef enum {
    M_ANIM_DEATH = 20,
} M_ANIM;

static const BITE m_MonkHit = {
    .pos = { .x = -23, .y = 16, .z = 265 },
    .mesh_num = 14,
};

static int32_t M_GetDamage(
    const ITEM *const item, const char *const key, const int32_t default_value)
{
    OBJECT_PROPERTY_VALUE damage = {};
    if (ObjectProperty_GetItemValue(item, key, &damage)) {
        return damage.as_int;
    }

    return default_value;
}

static void M_Control(const int16_t item_num)
{
    if (!Creature_Activate(item_num)) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    CREATURE *const creature = item->creature_data;

    int16_t head = 0;
    int16_t tilt = 0;
    int16_t angle = 0;

    if (item->hit_points <= 0) {
        if (item->current_anim_state != M_STATE_DEATH) {
            Item_SwitchToAnim(
                item, Random_GetControl() / 0x4000 + M_ANIM_DEATH, 0);
            item->current_anim_state = M_STATE_DEATH;
        }
    } else {
        AI_INFO info;
        Creature_AIInfo(item, &info);
        Creature_Mood(item, &info, true);

        angle = Creature_Turn(item, creature->maximum_turn);
        if (info.ahead) {
            head = info.angle;
        }

        const LARA_INFO *const lara = Lara_GetLaraInfo();
        switch (item->current_anim_state) {
        case M_STATE_WAIT_1:
            creature->flags &= 0xFFF;
            if (!Creature_IsHostile(item) && info.ahead
                && lara->target == item) {
            } else if (creature->mood == MOOD_BORED) {
                item->goal_anim_state = M_STATE_WALK;
            } else if (creature->mood == MOOD_ESCAPE) {
                item->goal_anim_state = M_STATE_RUN;
            } else if (info.ahead && info.distance < M_CLOSE_RANGE) {
                if (Random_GetControl() < 0x7000) {
                    item->goal_anim_state = M_STATE_ATTACK_1;
                } else {
                    item->goal_anim_state = M_STATE_WAIT_2;
                }
            } else if (!info.ahead) {
                item->goal_anim_state = M_STATE_RUN;
            } else if (info.distance < M_LONG_RANGE) {
                item->goal_anim_state = M_STATE_ATTACK_4;
            } else if (info.distance < M_WALK_RANGE) {
                item->goal_anim_state = M_STATE_WALK;
            } else {
                item->goal_anim_state = M_STATE_RUN;
            }
            break;

        case M_STATE_WAIT_2:
            creature->flags &= 0xFFF;
            if (!Creature_IsHostile(item) && info.ahead
                && lara->target == item) {
            } else if (creature->mood == MOOD_BORED) {
                item->goal_anim_state = M_STATE_WALK;
            } else if (creature->mood == MOOD_ESCAPE) {
                item->goal_anim_state = M_STATE_RUN;
            } else if (info.ahead && info.distance < M_CLOSE_RANGE) {
                const int16_t random = Random_GetControl();
                if (random < 0x3000) {
                    item->goal_anim_state = M_STATE_ATTACK_2;
                } else if (random < 0x6000) {
                    item->goal_anim_state = M_STATE_AIM_3;
                } else {
                    item->goal_anim_state = M_STATE_WAIT_1;
                }
            } else if (info.ahead && info.distance < M_WALK_RANGE) {
                item->goal_anim_state = M_STATE_WALK;
            } else {
                item->goal_anim_state = M_STATE_RUN;
            }
            break;

        case M_STATE_WALK:
            creature->maximum_turn = M_WALK_TURN;
            if (creature->mood == MOOD_BORED) {
                if (!Creature_IsHostile(item) && info.ahead
                    && lara->target == item) {
                    if (Random_GetControl() < 0x4000) {
                        item->goal_anim_state = M_STATE_WAIT_1;
                    } else {
                        item->goal_anim_state = M_STATE_WAIT_2;
                    }
                }
            } else if (creature->mood == MOOD_ESCAPE) {
                item->goal_anim_state = M_STATE_RUN;
            } else if (info.ahead && info.distance < M_CLOSE_RANGE) {
                if (Random_GetControl() < 0x4000) {
                    item->goal_anim_state = M_STATE_WAIT_1;
                } else {
                    item->goal_anim_state = M_STATE_WAIT_2;
                }
            } else if (!info.ahead || info.distance > M_WALK_RANGE) {
                item->goal_anim_state = M_STATE_RUN;
            }
            break;

        case M_STATE_RUN:
            creature->flags &= 0xFFF;
            creature->maximum_turn = M_RUN_TURN;
            if (Creature_IsHostile(item)) {
                creature->maximum_turn = M_RUN_TURN_FAST;
            }
            tilt = angle / 4;
            if (creature->mood == MOOD_BORED) {
                item->goal_anim_state = M_STATE_WAIT_1;
            } else if (creature->mood == MOOD_ESCAPE) {
            } else if (info.ahead && info.distance < M_CLOSE_RANGE) {
                if (Random_GetControl() < 0x4000) {
                    item->goal_anim_state = M_STATE_WAIT_1;
                } else {
                    item->goal_anim_state = M_STATE_WAIT_2;
                }
            } else if (info.ahead && info.distance < M_ATTACK_5_RANGE) {
                item->goal_anim_state = M_STATE_ATTACK_5;
            }
            break;

        case M_STATE_AIM_3:
            if (!info.ahead || info.distance > M_CLOSE_RANGE) {
                item->goal_anim_state = M_STATE_WAIT_2;
            } else {
                item->goal_anim_state = M_STATE_ATTACK_3;
            }
            break;

        case M_STATE_ATTACK_1:
        case M_STATE_ATTACK_2:
        case M_STATE_ATTACK_3:
        case M_STATE_ATTACK_4:
        case M_STATE_ATTACK_5:
            if (creature->enemy == Lara_GetItem()) {
                if ((creature->flags & 0xF000) == 0
                    && (item->touch_bits & M_TOUCH_BITS) != 0) {
                    Lara_TakeDamage(
                        M_GetDamage(item, "damage", M_BIFF_DAMAGE), true);
                    Sound_Effect(SFX_MONK_CRUNCH, &item->pos, SPM_NORMAL);
                    Creature_Effect(item, &m_MonkHit, Spawn_Blood);
                    creature->flags |= 0x1000;
                }
            } else if (
                (creature->flags & 0xF000) == 0 && creature->enemy != nullptr) {
                const int32_t dx = ABS(creature->enemy->pos.x - item->pos.x);
                const int32_t dy = ABS(creature->enemy->pos.y - item->pos.y);
                const int32_t dz = ABS(creature->enemy->pos.z - item->pos.z);
                if (dx < M_HIT_RANGE && dy < M_HIT_RANGE && dz < M_HIT_RANGE) {
                    Item_TakeDamage(
                        creature->enemy,
                        M_GetDamage(item, "enemy_damage", M_BIFF_ENEMY_DAMAGE),
                        IDF_NONE, item);
                    Sound_Effect(SFX_MONK_CRUNCH, &item->pos, SPM_NORMAL);
                    creature->flags |= 0x1000;
                }
            }
            break;

        default:
            break;
        }
    }

    Creature_Tilt(item, tilt);
    Creature_Head(item, head);
    Creature_Animate(item_num, angle, 0);
}

static void M_SetupBase(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->collision_func = Creature_Collision;

    obj->radius = M_RADIUS;
    obj->shadow_size = UNIT_SHADOW / 2;

    obj->intelligent = true;
    obj->save_position = true;
    obj->save_hitpoints = true;
    obj->save_flags = true;
    obj->save_anim = true;

    Object_GetBone(obj, 6)->rot.y = true;
    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY_INT(
            "max_hit_points", M_HIT_POINTS, "Maximum hit points."),
        OBJECT_PROPERTY_INT(
            "damage", M_BIFF_DAMAGE, "Damage dealt by melee attacks."),
        OBJECT_PROPERTY_INT(
            "enemy_damage", M_BIFF_ENEMY_DAMAGE,
            "Damage dealt by melee attacks against non-player targets."));
}

static void M_Setup1(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }
    M_SetupBase(obj);
    obj->pivot_length = 0;
}

static void M_Setup2(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }
    M_SetupBase(obj);
}

static void M_Setup3(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }
    M_SetupBase(obj);
    obj->pivot_length = 0;
    obj->shadow_size = 0;
}

REGISTER_OBJECT(O_MONK_1, M_Setup1)
REGISTER_OBJECT(O_MONK_2, M_Setup2)
REGISTER_OBJECT(O_MONK_3, M_Setup3)
