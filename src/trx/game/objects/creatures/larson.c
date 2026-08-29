#include <trx/core/utils.h>
#include <trx/game/const.h>
#include <trx/game/creature.h>
#include <trx/game/music.h>
#include <trx/game/objects.h>
#include <trx/game/objects/property.h>
#include <trx/game/pathing.h>
#include <trx/game/random.h>

// clang-format off
#define M_HIT_POINTS  50
#define M_DAMAGE      50
#define M_WALK_TURN   (DEG_1 * 3) // = 546
#define M_RUN_TURN    (DEG_1 * 6) // = 1092
#define M_WALK_RANGE  SQUARE(WALL_L * 3) // = 9437184
#define M_POSE_CHANCE 0x60 // = 96
// clang-format on

typedef enum {
    M_STATE_EMPTY,
    M_STATE_STOP,
    M_STATE_WALK,
    M_STATE_RUN,
    M_STATE_AIM,
    M_STATE_DEATH,
    M_STATE_POSE,
    M_STATE_SHOOT,
} M_STATE;

typedef enum {
    M_ANIM_DEATH = 15,
} M_ANIM;

typedef struct {
    int32_t damage;
} M_PRIV;

static const CREATURE_GUN m_LarsonGun = {
    .muzzle = { .pos = { -60, 170, 0 }, .mesh_num = 14, },
};

static void M_HandleSave(ITEM *const item, const SAVEGAME_STAGE stage)
{
    if (stage == SAVEGAME_STAGE_AFTER_LOAD) {
        if (item->hit_points <= 0) {
            Music_GetTrackState(Music_IDToSlot(MX_LARSON_SPEECH))->is_one_shot =
                true;
        }
    }
}

static void M_Control(const int16_t item_num)
{
    if (!Creature_Activate(item_num)) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    const M_PRIV *const p = item->priv;
    CREATURE *const person = item->creature_data;
    int16_t head = 0;
    int16_t angle = 0;
    int16_t tilt = 0;

    if (item->hit_points <= 0) {
        if (item->current_anim_state != M_STATE_DEATH) {
            item->current_anim_state = M_STATE_DEATH;
            Item_SwitchToAnim(item, M_ANIM_DEATH, 0);
        }
    } else {
        AI_INFO info;
        Creature_AIInfo(item, &info);

        if (info.ahead) {
            head = info.angle;
        }

        Creature_Mood(item, &info, false);

        angle = Creature_Turn(item, person->maximum_turn);

        switch (item->current_anim_state) {
        case M_STATE_STOP:
            if (item->required_anim_state) {
                item->goal_anim_state = item->required_anim_state;
            } else if (person->mood == MOOD_BORED) {
                item->goal_anim_state = Random_GetControl() < M_POSE_CHANCE
                    ? M_STATE_POSE
                    : M_STATE_WALK;
            } else if (person->mood == MOOD_ESCAPE) {
                item->goal_anim_state = M_STATE_RUN;
            } else {
                item->goal_anim_state = M_STATE_WALK;
            }
            break;

        case M_STATE_POSE:
            if (person->mood != MOOD_BORED) {
                item->goal_anim_state = M_STATE_STOP;
            } else if (Random_GetControl() < M_POSE_CHANCE) {
                item->required_anim_state = M_STATE_WALK;
                item->goal_anim_state = M_STATE_STOP;
            }
            break;

        case M_STATE_WALK:
            person->maximum_turn = M_WALK_TURN;
            if (person->mood == MOOD_BORED
                && Random_GetControl() < M_POSE_CHANCE) {
                item->required_anim_state = M_STATE_POSE;
                item->goal_anim_state = M_STATE_STOP;
            } else if (person->mood == MOOD_ESCAPE) {
                item->required_anim_state = M_STATE_RUN;
                item->goal_anim_state = M_STATE_STOP;
            } else if (Creature_CanTargetEnemy(item, &info)) {
                item->required_anim_state = M_STATE_AIM;
                item->goal_anim_state = M_STATE_STOP;
            } else if (!info.ahead || info.distance > M_WALK_RANGE) {
                item->required_anim_state = M_STATE_RUN;
                item->goal_anim_state = M_STATE_STOP;
            }
            break;

        case M_STATE_RUN:
            person->maximum_turn = M_RUN_TURN;
            tilt = angle / 2;
            if (person->mood == MOOD_BORED
                && Random_GetControl() < M_POSE_CHANCE) {
                item->required_anim_state = M_STATE_POSE;
                item->goal_anim_state = M_STATE_STOP;
            } else if (Creature_CanTargetEnemy(item, &info)) {
                item->required_anim_state = M_STATE_AIM;
                item->goal_anim_state = M_STATE_STOP;
            } else if (info.ahead && info.distance < M_WALK_RANGE) {
                item->required_anim_state = M_STATE_WALK;
                item->goal_anim_state = M_STATE_STOP;
            }
            break;

        case M_STATE_AIM:
            if (item->required_anim_state) {
                item->goal_anim_state = item->required_anim_state;
            } else if (Creature_CanTargetEnemy(item, &info)) {
                item->goal_anim_state = M_STATE_SHOOT;
            } else {
                item->goal_anim_state = M_STATE_STOP;
            }
            break;

        case M_STATE_SHOOT:
            if (!item->required_anim_state) {
                Creature_Shoot(item, &info, &m_LarsonGun, head, p->damage);
                item->required_anim_state = M_STATE_AIM;
            }
            if (person->mood == MOOD_ESCAPE) {
                item->required_anim_state = M_STATE_STOP;
            }
            break;
        }
    }

    Creature_Tilt(item, tilt);
    Creature_Head(item, head);

    Creature_Animate(item_num, angle, 0);
}

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->priv_size = sizeof(M_PRIV);
    obj->initialise_func = Creature_Initialise;
    obj->handle_save_func = M_HandleSave;
    obj->control_func = M_Control;
    obj->collision_func = Creature_Collision;
    obj->shadow_size = UNIT_SHADOW / 2;

    obj->radius = WALL_L / 10;
    obj->smartness = 0x7FFF;
    obj->intelligent = true;
    obj->save_position = true;
    obj->save_hitpoints = true;
    obj->save_anim = true;
    obj->save_flags = true;

    Object_GetBone(obj, 6)->rot.y = true;
    OBJECT_PROPERTIES(
        obj, ITEM_PROPERTY_MAX_HIT_POINTS(M_HIT_POINTS),
        OBJECT_PROPERTY(
            M_PRIV, damage, M_DAMAGE, "Damage dealt by Larson's shot."));
}

REGISTER_OBJECT(O_LARSON, M_Setup)
