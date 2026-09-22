#include <trx/core/utils.h>
#include <trx/game/creature.h>
#include <trx/game/lara.h>
#include <trx/game/objects/common.h>
#include <trx/game/objects/property.h>
#include <trx/game/random.h>
#include <trx/game/spawn.h>

// clang-format off
#define M_PIVOT                300
#define M_RADIUS               (WALL_L / 3) // = 341
#define M_HIT_POINTS           16
#define M_LUNGE_RANGE          SQUARE(WALL_L) // = 0x100000
#define M_LUNGE_TOUCH_BITS     0x6648
#define M_DOG_LUNGE_DAMAGE     50
#define M_JACKAL_LUNGE_DAMAGE  20
#define M_STALK_RANGE          SQUARE(WALL_L + (WALL_L / 2)) // = 0x240000
#define M_STALK_TURN           (3 * DEG_1)
#define M_DOG_BITE_RANGE       SQUARE(WALL_L * 5 / 12) // = 0x2C4E4
#define M_JACKAL_BITE_RANGE    SQUARE(WALL_L / 3) // = 0x1C639
#define M_BITE_TOUCH_BITS      0x48
#define M_DOG_BITE_DAMAGE      12
#define M_JACKAL_BITE_DAMAGE   10
#define M_RUN_TURN             (6 * DEG_1)
#define M_STAT_TURN            (1 * DEG_1)
#define M_WALK_TURN            (3 * DEG_1)
#define M_MINIMUM_SLEEP_TIME   (30 * 10) // = 300
#define M_SLEEP_CHANCE         0x100 // = 256
#define M_SLEEP_2_STAND_CHANCE 0x80 // = 128
#define M_STAT_CHANCE          0x100 // = 256
#define M_WALK_CHANCE          0x1000 // = 4096
#define M_AWARE_RANGE          SQUARE(3 * WALL_L) // = 0x900000
#define M_ANIM_DEATH_COUNT     4
// clang-format on

typedef struct {
    bool use_idle_pose;
    int32_t lunge_damage;
    int32_t bite_damage;
} M_PRIV;

typedef enum {
    M_STATE_NULL,
    M_STATE_STOP,
    M_STATE_WALK,
    M_STATE_RUN,
    M_STATE_JUMP,
    M_STATE_STALK,
    M_STATE_ATTACK_1,
    M_STATE_HOWL,
    M_STATE_SLEEP,
    M_STATE_CROUCH,
    M_STATE_TURN,
    M_STATE_DEATH,
    M_STATE_ATTACK_2
} M_STATE;

typedef enum {
    // clang-format off
    M_ANIM_POSE    = 1,
    M_ANIM_STOP    = 8,
    M_ANIM_DEATH_1 = 20,
    M_ANIM_DEATH_2 = 21,
    M_ANIM_DEATH_3 = 22,
    // clang-format on
} M_ANIM;

static const BITE m_DogBite = {
    .pos = { .x = 0, .y = 0, .z = 100 },
    .mesh_num = 3,
};

static const M_ANIM m_DeathAnims[M_ANIM_DEATH_COUNT] = {
    M_ANIM_DEATH_1,
    M_ANIM_DEATH_2,
    M_ANIM_DEATH_3,
    M_ANIM_DEATH_1,
};

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    Item_SwitchToAnim(item, M_ANIM_STOP, 0);
    item->current_anim_state = M_STATE_STOP;
}

static void M_SetUseIdlePose(ITEM *const item, const TRX_VALUE *const in)
{
    if (item->hit_points <= 0 || item->is_simulated
        || !Item_TestFrameEqual(item, 0)) {
        return;
    }

    if (in->as_bool && !Item_TestAnimEqual(item, M_ANIM_STOP)) {
        return;
    }

    if (!in->as_bool && !Item_TestAnimEqual(item, M_ANIM_POSE)) {
        return;
    }

    Item_SetVisible(item, in->as_bool);
    Item_SwitchToAnim(item, in->as_bool ? M_ANIM_POSE : M_ANIM_STOP, 0);
    item->current_anim_state = M_STATE_STOP;
}

static int32_t M_GetBiteRange(const ITEM *const item)
{
    return item->object_id == O_JACKAL ? M_JACKAL_BITE_RANGE : M_DOG_BITE_RANGE;
}

static void M_DoBlood(const ITEM *const item)
{
    if (item->object_id == O_JACKAL) {
        Creature_EffectEx(item, &m_DogBite, 2, -1, Spawn_Blood);
    } else {
        Creature_Effect(item, &m_DogBite, Spawn_Blood);
    }
}

static void M_Control(const int16_t item_num)
{
    if (!Creature_Activate(item_num)) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    const M_PRIV *const p = item->priv;
    CREATURE *const creature = item->creature_data;
    int16_t angle = 0;
    int16_t head = 0;
    int16_t x_head = 0;
    int16_t torso_y = 0;

    ITEM *const lara_item = Lara_GetItem();

    if (item->hit_points <= 0) {
        if (item->current_anim_state != M_STATE_DEATH) {
            const int32_t death_anim = Random_GetControl() % M_ANIM_DEATH_COUNT;
            Item_SwitchToAnim(item, m_DeathAnims[death_anim], 0);
            item->current_anim_state = M_STATE_DEATH;
        }
    } else {
        if (item->ai_bits != 0) {
            Creature_GetAITarget(creature);
        } else {
            creature->enemy = lara_item;
        }

        AI_INFO info;
        Creature_AIInfo(item, &info);

        int32_t dist;
        if (creature->enemy == lara_item) {
            dist = info.distance;
        } else {
            dist = XYZ_32_GetLength2((XYZ_32) {
                .x = lara_item->pos.x - item->pos.x,
                .y = 0,
                .z = lara_item->pos.z - item->pos.z,
            });
        }

        if (info.ahead) {
            head = info.angle;
            x_head = info.x_angle;
        }

        Creature_Mood(item, &info, true);
        if (creature->mood == MOOD_BORED) {
            creature->maximum_turn >>= 1;
        }

        angle = Creature_Turn(item, creature->maximum_turn);
        torso_y = angle << 2;

        if (creature->hurt_by_lara
            || (dist < M_AWARE_RANGE && (item->ai_bits & AI_MODIFY) == 0)) {
            Creature_AlertAllGuards(item_num);
            item->ai_bits &= ~AI_MODIFY;
        }

        const int16_t rnd = Random_GetControl();
        const int16_t frame = Item_GetRelativeFrame(item);

        switch (item->current_anim_state) {
        case M_STATE_NULL:
        case M_STATE_SLEEP:
            head = 0;
            x_head = 0;
            if (creature->mood != MOOD_BORED && item->ai_bits != AI_MODIFY) {
                item->goal_anim_state = M_STATE_STOP;
            } else {
                creature->flags++;
                creature->maximum_turn = 0;
                if (creature->flags > M_MINIMUM_SLEEP_TIME
                    && rnd < M_SLEEP_2_STAND_CHANCE) {
                    item->goal_anim_state = M_STATE_STOP;
                }
            }
            break;

        case M_STATE_CROUCH:
        case M_STATE_STOP:
            if (item->current_anim_state == M_STATE_CROUCH
                && item->required_anim_state != M_STATE_NULL) {
                item->goal_anim_state = item->required_anim_state;
                break;
            }

            creature->maximum_turn = 0;

            if ((item->ai_bits & AI_GUARD) != 0) {
                head = Creature_AIGuard(creature);

                if ((Random_GetControl() & 0xFF) == 0) {
                    if (item->current_anim_state == M_STATE_STOP) {
                        item->goal_anim_state = M_STATE_CROUCH;
                    } else {
                        item->goal_anim_state = M_STATE_STOP;
                    }
                }
            } else if (
                item->current_anim_state == M_STATE_CROUCH
                && rnd < M_SLEEP_2_STAND_CHANCE) {
                item->goal_anim_state = M_STATE_STOP;
            } else if ((item->ai_bits & AI_PATROL_1) != 0) {
                if (item->current_anim_state == M_STATE_STOP) {
                    item->goal_anim_state = M_STATE_WALK;
                } else {
                    item->goal_anim_state = M_STATE_STOP;
                }
            } else if (creature->mood == MOOD_ESCAPE) {
                LARA_INFO *const lara = Lara_GetLaraInfo();
                if (lara->target == item || !info.ahead || item->hit_status) {
                    item->required_anim_state = M_STATE_RUN;
                    item->goal_anim_state = M_STATE_CROUCH;
                } else {
                    item->goal_anim_state = M_STATE_STOP;
                }
            } else if (creature->mood == MOOD_BORED) {
                creature->flags = 0;
                creature->maximum_turn = M_STAT_TURN;

                if (rnd < M_SLEEP_CHANCE && (item->ai_bits & AI_MODIFY) != 0
                    && item->current_anim_state == M_STATE_STOP) {
                    item->goal_anim_state = M_STATE_SLEEP;
                    creature->flags = 0;
                } else if (rnd < M_WALK_CHANCE) {
                    if (item->current_anim_state == M_STATE_STOP) {
                        item->goal_anim_state = M_STATE_WALK;
                    } else {
                        item->goal_anim_state = M_STATE_STOP;
                    }
                } else if ((rnd & 0x1F) == 0) {
                    item->goal_anim_state = M_STATE_HOWL;
                }
            } else {
                item->required_anim_state = M_STATE_RUN;

                if (item->current_anim_state == M_STATE_STOP) {
                    item->goal_anim_state = M_STATE_CROUCH;
                }
            }

            break;

        case M_STATE_WALK:
            creature->maximum_turn = M_WALK_TURN;
            if ((item->ai_bits & AI_PATROL_1) != 0) {
                item->goal_anim_state = M_STATE_WALK;
            } else if (creature->mood == MOOD_BORED && rnd < M_STAT_CHANCE) {
                item->goal_anim_state = M_STATE_STOP;
            } else {
                item->goal_anim_state = M_STATE_STALK;
            }
            break;

        case M_STATE_RUN:
            creature->maximum_turn = M_RUN_TURN;
            if (creature->mood == MOOD_ESCAPE) {
                LARA_INFO *const lara = Lara_GetLaraInfo();
                if (lara->target != item && info.ahead) {
                    item->goal_anim_state = M_STATE_CROUCH;
                }
            } else if (creature->mood == MOOD_BORED) {
                item->goal_anim_state = M_STATE_CROUCH;
            } else if (info.bite && info.distance < M_LUNGE_RANGE) {
                item->goal_anim_state = M_STATE_ATTACK_1;
            } else if (info.distance < M_STALK_RANGE) {
                item->required_anim_state = M_STATE_STALK;
                item->goal_anim_state = M_STATE_CROUCH;
            }
            break;

        case M_STATE_STALK:
            creature->maximum_turn = M_STALK_TURN;
            if (creature->mood == MOOD_BORED) {
                item->goal_anim_state = M_STATE_CROUCH;
            } else if (creature->mood == MOOD_ESCAPE) {
                item->goal_anim_state = M_STATE_RUN;
            } else if (info.bite && info.distance < M_GetBiteRange(item)) {
                item->goal_anim_state = M_STATE_ATTACK_2;
                item->required_anim_state = M_STATE_STALK;
            } else if (info.distance > M_STALK_RANGE || item->hit_status) {
                item->goal_anim_state = M_STATE_RUN;
            }
            break;

        case M_STATE_ATTACK_1:
            if (info.bite && (item->touch_bits & M_LUNGE_TOUCH_BITS) != 0
                && frame >= 4 && frame <= 14) {
                M_DoBlood(item);
                Lara_TakeDamage(p->lunge_damage, true);
            }
            item->goal_anim_state = M_STATE_RUN;
            break;

        case M_STATE_HOWL:
            head = 0;
            x_head = 0;
            break;

        case M_STATE_ATTACK_2:
            if (info.bite && (item->touch_bits & M_BITE_TOUCH_BITS) != 0
                && ((frame >= 9 && frame <= 12)
                    || (frame >= 22 && frame <= 25))) {
                M_DoBlood(item);
                Lara_TakeDamage(p->bite_damage, true);
            }
            break;
        }
    }

    Creature_Tilt(item, 0);
    int16_t joint = 0;
    if (item->object_id == O_JACKAL) {
        Creature_Joint(item, joint++, torso_y);
    }
    Creature_Joint(item, joint++, head);
    Creature_Joint(item, joint++, x_head);
    Creature_Animate(item_num, angle, 0);
}

static void M_SetupCommon(OBJECT *const obj)
{
    obj->priv_size = sizeof(M_PRIV);
    obj->initialise_func = M_Initialise;
    obj->control_func = M_Control;
    obj->collision_func = Creature_Collision;

    obj->shadow_size = UNIT_SHADOW / 2;
    obj->pivot_length = M_PIVOT;
    obj->radius = M_RADIUS;

    obj->intelligent = true;
    obj->save_position = true;
    obj->save_hitpoints = true;
    obj->save_flags = true;
    obj->save_anim = true;

    Object_GetBone(obj, 2)->rot.y = true;
    Object_GetBone(obj, 2)->rot.x = true;
}

static void M_SetupDog(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    M_SetupCommon(obj);

    OBJECT_PROPERTIES(
        obj, ITEM_PROPERTY_MAX_HIT_POINTS(M_HIT_POINTS),
        OBJECT_PROPERTY(
            M_PRIV, lunge_damage, M_DOG_LUNGE_DAMAGE,
            "Damage dealt by the lunge attack."),
        OBJECT_PROPERTY(
            M_PRIV, bite_damage, M_DOG_BITE_DAMAGE,
            "Damage dealt by the bite attack."));
}

static void M_SetupJackal(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    M_SetupCommon(obj);
    Object_GetBone(obj, 0)->rot.y = true;

    OBJECT_PROPERTIES(
        obj, ITEM_PROPERTY_MAX_HIT_POINTS(M_HIT_POINTS),
        OBJECT_PROPERTY(
            M_PRIV, lunge_damage, M_JACKAL_LUNGE_DAMAGE,
            "Damage dealt by the lunge attack."),
        OBJECT_PROPERTY(
            M_PRIV, bite_damage, M_JACKAL_BITE_DAMAGE,
            "Damage dealt by the bite attack."),
        OBJECT_PROPERTY_SETTER(
            M_PRIV, use_idle_pose, false, nullptr, M_SetUseIdlePose,
            "Whether the creature is posed and visible before being "
            "activated."));
}

REGISTER_OBJECT(O_PATROL_DOG, M_SetupDog)
REGISTER_OBJECT(O_HUSKIE, M_SetupDog)
REGISTER_OBJECT(O_JACKAL, M_SetupJackal)
