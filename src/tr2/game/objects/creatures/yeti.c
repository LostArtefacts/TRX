#include "game/objects/creatures/yeti.h"

#include "game/creature.h"
#include "game/lara/control.h"
#include "game/objects/common.h"
#include "game/random.h"
#include "game/spawn.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/utils.h>

// clang-format off
#define YETI_HITPOINTS        30
#define YETI_TOUCH_BITS_R     0b00000111'00000000 // = 0x0700
#define YETI_TOUCH_BITS_L     0b00111000'00000000 // = 0x3800
#define YETI_TOUCH_BITS_LR    (YETI_TOUCH_BITS_R | YETI_TOUCH_BITS_L) // = 0x3F00
#define YETI_RADIUS           (WALL_L / 8) // = 128
#define YETI_WALK_TURN        (DEG_1 * 4) // = 728
#define YETI_RUN_TURN         (DEG_1 * 6) // = 1092
#define YETI_VAULT_SHIFT      300
#define YETI_ATTACK_1_RANGE   SQUARE(WALL_L / 2) // = 262144
#define YETI_ATTACK_2_RANGE   SQUARE(WALL_L * 2 / 3) // = 465124
#define YETI_ATTACK_3_RANGE   SQUARE(WALL_L * 2) // = 4194304
#define YETI_RUN_RANGE        SQUARE(WALL_L * 2) // = 4194304
#define YETI_PUNCH_DAMAGE     100
#define YETI_THUMP_DAMAGE     150
#define YETI_CHARGE_DAMAGE    200
#define YETI_ATTACK_1_CHANCE  0x4000
#define YETI_WAIT_1_CHANCE    0x100
#define YETI_WAIT_2_CHANCE    (YETI_WAIT_1_CHANCE + 0x100) // = 512
#define YETI_WALK_CHANCE      (YETI_WAIT_2_CHANCE + 0x100) // = 768
#define YETI_STOP_ROAR_CHANCE 0x200
// clang-format on

typedef enum {
    YETI_STATE_EMPTY = 0,
    YETI_STATE_RUN = 1,
    YETI_STATE_STOP = 2,
    YETI_STATE_WALK = 3,
    YETI_STATE_ATTACK_1 = 4,
    YETI_STATE_ATTACK_2 = 5,
    YETI_STATE_ATTACK_3 = 6,
    YETI_STATE_WAIT_1 = 7,
    YETI_STATE_DEATH = 8,
    YETI_STATE_WAIT_2 = 9,
    YETI_STATE_CLIMB_1 = 10,
    YETI_STATE_CLIMB_2 = 11,
    YETI_STATE_CLIMB_3 = 12,
    YETI_STATE_FALL = 13,
    YETI_STATE_KILL = 14,
} YETI_STATE;

typedef enum {
    YETI_ANIM_KILL = 36,
    YETI_ANIM_FALL = 35,
    YETI_ANIM_CLIMB_1 = 34,
    YETI_ANIM_CLIMB_2 = 33,
    YETI_ANIM_CLIMB_3 = 32,
    YETI_ANIM_DEATH = 31,
} YETI_ANIM;

static const BITE m_YetiBiteL = {
    .pos = { .x = 12, .y = 101, .z = 19 },
    .mesh_num = 13,
};

static const BITE m_YetiBiteR = {
    .pos = { .x = 12, .y = 101, .z = 19 },
    .mesh_num = 10,
};

void Yeti_Setup(void)
{
    OBJECT *const obj = Object_Get(O_YETI);
    if (!obj->loaded) {
        return;
    }

    obj->control_func = Yeti_Control;
    obj->collision_func = Creature_Collision;

    obj->hit_points = YETI_HITPOINTS;
    obj->radius = YETI_RADIUS;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->pivot_length = 100;

    obj->intelligent = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_flags = 1;
    obj->save_anim = 1;

    Object_GetBone(obj, 6)->rot_y = true;
    Object_GetBone(obj, 14)->rot_y = true;
}

void Yeti_Control(const int16_t item_num)
{
    if (!Creature_Activate(item_num)) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    CREATURE *const creature = item->data;

    int16_t head = 0;
    int16_t body = 0;
    int16_t angle = 0;
    const bool lara_alive = g_LaraItem->hit_points > 0;

    if (item->hit_points > 0) {
        AI_INFO info;
        Creature_AIInfo(item, &info);
        Creature_Mood(item, &info, MOOD_ATTACK);

        if (info.ahead) {
            head = info.angle;
        }
        angle = Creature_Turn(item, creature->maximum_turn);

        switch (item->current_anim_state) {
        case YETI_STATE_STOP:
            creature->flags = 0;
            creature->maximum_turn = 0;
            if (creature->mood == MOOD_ESCAPE) {
                item->goal_anim_state = YETI_STATE_RUN;
            } else if (item->required_anim_state != YETI_STATE_EMPTY) {
                item->goal_anim_state = item->required_anim_state;
            } else if (creature->mood == MOOD_BORED) {
                const int32_t random = Random_GetControl();
                if (random < YETI_WAIT_1_CHANCE || !lara_alive) {
                    item->goal_anim_state = YETI_STATE_WAIT_1;
                } else if (random < YETI_WAIT_2_CHANCE) {
                    item->goal_anim_state = YETI_STATE_WAIT_2;
                } else if (random < YETI_WALK_CHANCE) {
                    item->goal_anim_state = YETI_STATE_WALK;
                }
            } else if (
                info.ahead != 0 && info.distance < YETI_ATTACK_1_RANGE
                && Random_GetControl() < YETI_ATTACK_1_CHANCE) {
                item->goal_anim_state = YETI_STATE_ATTACK_1;
                break;
            } else if (info.ahead != 0 && info.distance < YETI_ATTACK_2_RANGE) {
                item->goal_anim_state = YETI_STATE_ATTACK_2;
            } else if (creature->mood == MOOD_STALK) {
                item->goal_anim_state = YETI_STATE_WALK;
            } else {
                item->goal_anim_state = YETI_STATE_RUN;
            }
            break;

        case YETI_STATE_WAIT_1:
            if (creature->mood == MOOD_ESCAPE || item->hit_status) {
                item->goal_anim_state = YETI_STATE_STOP;
            } else if (creature->mood == MOOD_BORED) {
                if (lara_alive) {
                    const int32_t random = Random_GetControl();
                    if (random < YETI_WAIT_1_CHANCE) {
                        item->goal_anim_state = YETI_STATE_STOP;
                    } else if (random < YETI_WAIT_2_CHANCE) {
                        item->goal_anim_state = YETI_STATE_WAIT_2;
                    } else if (random < YETI_WALK_CHANCE) {
                        item->goal_anim_state = YETI_STATE_STOP;
                        item->required_anim_state = YETI_STATE_WALK;
                    }
                }
            } else if (Random_GetControl() < YETI_STOP_ROAR_CHANCE) {
                item->goal_anim_state = YETI_STATE_STOP;
            }
            break;

        case YETI_STATE_WAIT_2:
            if (creature->mood == MOOD_ESCAPE || item->hit_status) {
                item->goal_anim_state = YETI_STATE_STOP;
            } else if (creature->mood == MOOD_BORED) {
                const int32_t random = Random_GetControl();
                if (random < YETI_WAIT_1_CHANCE || !lara_alive) {
                    item->goal_anim_state = YETI_STATE_WAIT_1;
                } else if (random < YETI_WAIT_2_CHANCE) {
                    item->goal_anim_state = YETI_STATE_STOP;
                } else if (random < YETI_WALK_CHANCE) {
                    item->goal_anim_state = YETI_STATE_STOP;
                    item->required_anim_state = YETI_STATE_WALK;
                }
            } else if (Random_GetControl() < YETI_STOP_ROAR_CHANCE) {
                item->goal_anim_state = YETI_STATE_STOP;
            }
            break;

        case YETI_STATE_WALK:
            creature->maximum_turn = YETI_WALK_TURN;
            if (creature->mood == MOOD_ESCAPE) {
                item->goal_anim_state = YETI_STATE_RUN;
            } else if (creature->mood == MOOD_BORED) {
                const int32_t random = Random_GetControl();
                if (random < YETI_WAIT_1_CHANCE || !lara_alive) {
                    item->goal_anim_state = YETI_STATE_STOP;
                    item->required_anim_state = YETI_STATE_WAIT_1;
                } else if (random < YETI_WAIT_2_CHANCE) {
                    item->goal_anim_state = YETI_STATE_STOP;
                    item->required_anim_state = YETI_STATE_WAIT_2;
                } else if (random < YETI_WALK_CHANCE) {
                    item->goal_anim_state = YETI_STATE_STOP;
                }
            } else if (creature->mood == MOOD_ATTACK) {
                if (info.ahead != 0 && info.distance < YETI_ATTACK_2_RANGE) {
                    item->goal_anim_state = YETI_STATE_STOP;
                } else if (info.distance > YETI_RUN_RANGE) {
                    item->goal_anim_state = YETI_STATE_RUN;
                }
            }
            break;

        case YETI_STATE_RUN:
            creature->flags = 0;
            creature->maximum_turn = YETI_RUN_TURN;
            if (creature->mood == MOOD_ESCAPE) {
                break;
            } else if (creature->mood == MOOD_BORED) {
                item->goal_anim_state = YETI_STATE_WALK;
            } else if (info.ahead != 0 && info.distance < YETI_ATTACK_2_RANGE) {
                item->goal_anim_state = YETI_STATE_STOP;
            } else if (info.ahead != 0 && info.distance < YETI_ATTACK_3_RANGE) {
                item->goal_anim_state = YETI_STATE_ATTACK_3;
            } else if (creature->mood == MOOD_STALK) {
                item->goal_anim_state = YETI_STATE_WALK;
            }
            break;

        case YETI_STATE_ATTACK_1:
            body = head;
            head = 0;
            if (creature->flags == 0
                && (item->touch_bits & YETI_TOUCH_BITS_R) != 0) {
                Creature_Effect(item, &m_YetiBiteR, Spawn_Blood);
                Lara_TakeDamage(YETI_PUNCH_DAMAGE, true);
                creature->flags = 1;
                break;
            }
            break;

        case YETI_STATE_ATTACK_2:
            body = head;
            head = 0;

            creature->maximum_turn = YETI_WALK_TURN;
            if (creature->flags == 0
                && (item->touch_bits & YETI_TOUCH_BITS_LR) != 0) {
                if ((item->touch_bits & YETI_TOUCH_BITS_L) != 0) {
                    Creature_Effect(item, &m_YetiBiteL, Spawn_Blood);
                }
                if ((item->touch_bits & YETI_TOUCH_BITS_R) != 0) {
                    Creature_Effect(item, &m_YetiBiteR, Spawn_Blood);
                }
                Lara_TakeDamage(YETI_THUMP_DAMAGE, true);
                creature->flags = 1;
            }
            break;

        case YETI_STATE_ATTACK_3:
            body = head;
            head = 0;

            if (creature->flags != 0
                && (item->touch_bits & YETI_TOUCH_BITS_LR) != 0) {
                if ((item->touch_bits & YETI_TOUCH_BITS_L) != 0) {
                    Creature_Effect(item, &m_YetiBiteL, Spawn_Blood);
                }
                if ((item->touch_bits & YETI_TOUCH_BITS_R) != 0) {
                    Creature_Effect(item, &m_YetiBiteR, Spawn_Blood);
                }
                Lara_TakeDamage(YETI_CHARGE_DAMAGE, true);
                creature->flags = 1;
            }
            break;

        default:
            break;
        }
    } else if (item->current_anim_state != YETI_STATE_DEATH) {
        Item_SwitchToAnim(item, YETI_ANIM_DEATH, 0);
        item->current_anim_state = YETI_STATE_DEATH;
    }

    if (lara_alive && g_LaraItem->hit_points <= 0) {
        Creature_Kill(
            item, YETI_ANIM_KILL, YETI_STATE_KILL, LA_EXTRA_YETI_KILL);
        return;
    }

    Creature_Head(item, body);
    Creature_Neck(item, head);
    if (item->current_anim_state >= YETI_STATE_CLIMB_1) {
        Creature_Animate(item_num, angle, 0);
    } else {
        switch (Creature_Vault(item_num, angle, 2, YETI_VAULT_SHIFT)) {
        case -4:
            Item_SwitchToAnim(item, YETI_ANIM_FALL, 0);
            item->current_anim_state = YETI_STATE_FALL;
            break;

        case 2:
            Item_SwitchToAnim(item, YETI_ANIM_CLIMB_1, 0);
            item->current_anim_state = YETI_STATE_CLIMB_1;
            break;

        case 3:
            Item_SwitchToAnim(item, YETI_ANIM_CLIMB_2, 0);
            item->current_anim_state = YETI_STATE_CLIMB_2;
            break;

        case 4:
            Item_SwitchToAnim(item, YETI_ANIM_CLIMB_3, 0);
            item->current_anim_state = YETI_STATE_CLIMB_3;
            break;

        default:
            return;
        }
    }
}
