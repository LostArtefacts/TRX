#include <trx/core/utils.h>
#include <trx/game/const.h>
#include <trx/game/creature.h>
#include <trx/game/music.h>
#include <trx/game/objects.h>
#include <trx/game/objects/property.h>
#include <trx/game/pathing.h>
#include <trx/game/spawn.h>

// clang-format off
#define M_HIT_POINTS 200
#define M_DAMAGE     150
#define M_RADIUS     (WALL_L / 10) // = 102
#define M_WALK_TURN  (DEG_1 * 3) // = 546
#define M_RUN_TURN   (DEG_1 * 6) // = 1092
#define M_WALK_RANGE SQUARE(WALL_L * 4) // = 16777216
// clang-format on

typedef enum {
    M_STATE_EMPTY,
    M_STATE_STOP,
    M_STATE_WALK,
    M_STATE_RUN,
    M_STATE_AIM,
    M_STATE_DEATH,
    M_STATE_SHOOT,
} M_STATE;

typedef enum {
    M_ANIM_DEATH = 14,
} M_ANIM;

typedef struct {
    int32_t damage;
} M_PRIV;

static const CREATURE_GUN m_BaldyGun = {
    .muzzle = { .pos = { -20, 440, 20 }, .mesh_num = 9 },
};

static void M_Initialise(const int16_t item_num)
{
    Creature_Initialise(item_num);
    Item_Get(item_num)->current_anim_state = M_STATE_RUN;
}

static void M_HandleSave(ITEM *const item, const SAVEGAME_STAGE stage)
{
    if (stage == SAVEGAME_STAGE_AFTER_LOAD) {
        if (item->hit_points <= 0) {
            Music_GetTrackState(Music_IDToSlot(MX_BALDY_SPEECH))->is_one_shot =
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
    CREATURE *const baldy = item->creature_data;
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

        Creature_Mood(item, &info, true);

        angle = Creature_Turn(item, baldy->maximum_turn);

        switch (item->current_anim_state) {
        case M_STATE_STOP:
            if (item->required_anim_state) {
                item->goal_anim_state = item->required_anim_state;
            } else if (Creature_CanTargetEnemy(item, &info)) {
                item->goal_anim_state = M_STATE_AIM;
            } else if (baldy->mood == MOOD_BORED) {
                item->goal_anim_state = M_STATE_WALK;
            } else {
                item->goal_anim_state = M_STATE_RUN;
            }
            break;

        case M_STATE_WALK:
            baldy->maximum_turn = M_WALK_TURN;
            if (baldy->mood == MOOD_ESCAPE || !info.ahead) {
                item->required_anim_state = M_STATE_RUN;
                item->goal_anim_state = M_STATE_STOP;
            } else if (Creature_CanTargetEnemy(item, &info)) {
                item->required_anim_state = M_STATE_AIM;
                item->goal_anim_state = M_STATE_STOP;
            } else if (info.distance > M_WALK_RANGE) {
                item->required_anim_state = M_STATE_RUN;
                item->goal_anim_state = M_STATE_STOP;
            }
            break;

        case M_STATE_RUN:
            baldy->maximum_turn = M_RUN_TURN;
            tilt = angle / 2;
            if (baldy->mood != MOOD_ESCAPE || info.ahead) {
                if (Creature_CanTargetEnemy(item, &info)) {
                    item->required_anim_state = M_STATE_AIM;
                    item->goal_anim_state = M_STATE_STOP;
                } else if (info.ahead && info.distance < M_WALK_RANGE) {
                    item->required_anim_state = M_STATE_WALK;
                    item->goal_anim_state = M_STATE_STOP;
                }
            }
            break;

        case M_STATE_AIM:
            baldy->flags = 0;
            if (item->required_anim_state) {
                item->goal_anim_state = M_STATE_STOP;
            } else if (Creature_CanTargetEnemy(item, &info)) {
                item->goal_anim_state = M_STATE_SHOOT;
            } else {
                item->goal_anim_state = M_STATE_STOP;
            }
            break;

        case M_STATE_SHOOT:
            if (!baldy->flags) {
                info.distance /= 2;
                Creature_Shoot(item, &info, &m_BaldyGun, head, p->damage);
                baldy->flags = 1;
            }
            if (baldy->mood == MOOD_ESCAPE) {
                item->required_anim_state = M_STATE_RUN;
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
    obj->initialise_func = M_Initialise;
    obj->handle_save_func = M_HandleSave;
    obj->control_func = M_Control;
    obj->collision_func = Creature_Collision;
    obj->shadow_size = UNIT_SHADOW / 2;

    obj->radius = M_RADIUS;
    obj->smartness = 0x7FFF;
    obj->intelligent = true;
    obj->save_position = true;
    obj->save_hitpoints = true;
    obj->save_anim = true;
    obj->save_flags = true;

    Object_GetBone(obj, 0)->rot.y = true;
    OBJECT_PROPERTIES(
        obj, ITEM_PROPERTY_MAX_HIT_POINTS(M_HIT_POINTS),
        OBJECT_PROPERTY(
            M_PRIV, damage, M_DAMAGE, "Damage dealt by Baldy's shot."));
}

REGISTER_OBJECT(O_BALDY, M_Setup)
