#include <trx/config.h>
#include <trx/core/utils.h>
#include <trx/game/camera.h>
#include <trx/game/collision/los.h>
#include <trx/game/creature.h>
#include <trx/game/music.h>
#include <trx/game/objects.h>
#include <trx/game/objects/property.h>
#include <trx/game/pathing.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>

// clang-format off
#define M_HIT_POINTS    70
#define M_POSE_CHANCE   0x60 // = 96
#define M_SHOT_DAMAGE   50
#define M_WALK_TURN     (DEG_1 * 3) // = 546
#define M_RUN_TURN      (DEG_1 * 6) // = 1092
#define M_WALK_RANGE    SQUARE(WALL_L * 3) // = 9437184
#define M_WIMP_CHANCE   0x2000
#define M_RUN_HITPOINTS 40
#define M_DISAPPEAR     10
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
    M_ANIM_DEATH = 12,
} M_ANIM;

typedef struct {
    int32_t damage;
} M_PRIV;

static const CREATURE_GUN m_PierreGun1 = {
    .muzzle = { .pos = { 60, 200, 0 }, .mesh_num = 11, },
};
static const CREATURE_GUN m_PierreGun2 = {
    .muzzle = { .pos = { -57, 200, 0 }, .mesh_num = 14, },
};
static int16_t m_PierreItemNum = NO_ITEM;

static bool M_CanDropItems(const ITEM *const item)
{
    return item->hit_points <= 0 && item->trigger.spent;
}

static void M_HandleSave(ITEM *const item, const SAVEGAME_STAGE stage)
{
    if (stage == SAVEGAME_STAGE_AFTER_LOAD) {
        if (item->hit_points <= 0 && item->trigger.spent) {
            Music_GetTrackState(Music_IDToSlot(MX_PIERRE_SPEECH))->is_one_shot =
                true;
        }
    }
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    const M_PRIV *const p = item->priv;

    if (g_Config.gameplay.change_pierre_spawn) {
        if (m_PierreItemNum == NO_ITEM) {
            m_PierreItemNum = item_num;
        } else if (m_PierreItemNum != item_num) {
            ITEM *old_pierre = Item_Get(m_PierreItemNum);
            if (old_pierre->trigger.spent) {
                if (!item->trigger.spent) {
                    Item_Destroy(item_num);
                }
            } else {
                Item_Destroy(m_PierreItemNum);
                m_PierreItemNum = item_num;
            }
        }
    } else {
        if (m_PierreItemNum == NO_ITEM) {
            m_PierreItemNum = item_num;
        } else if (m_PierreItemNum != item_num) {
            if (item->trigger.spent) {
                Item_Destroy(m_PierreItemNum);
            } else {
                Item_Destroy(item_num);
            }
        }
    }

    if (!Creature_Activate(item_num)) {
        return;
    }

    CREATURE *const pierre = item->creature_data;
    int16_t head = 0;
    int16_t angle = 0;
    int16_t tilt = 0;

    if (item->hit_points <= M_RUN_HITPOINTS && !item->trigger.spent) {
        item->hit_points = M_RUN_HITPOINTS;
        pierre->flags++;
    }

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

        if (pierre->flags) {
            info.enemy_zone_num = -1;
            item->hit_status = true;
        }
        Creature_Mood(item, &info, false);

        angle = Creature_Turn(item, pierre->maximum_turn);

        switch (item->current_anim_state) {
        case M_STATE_STOP:
            if (item->required_anim_state) {
                item->goal_anim_state = item->required_anim_state;
            } else if (pierre->mood == MOOD_BORED) {
                item->goal_anim_state = Random_GetControl() < M_POSE_CHANCE
                    ? M_STATE_POSE
                    : M_STATE_WALK;
            } else if (pierre->mood == MOOD_ESCAPE) {
                item->goal_anim_state = M_STATE_RUN;
            } else {
                item->goal_anim_state = M_STATE_WALK;
            }
            break;

        case M_STATE_POSE:
            if (pierre->mood != MOOD_BORED) {
                item->goal_anim_state = M_STATE_STOP;
            } else if (Random_GetControl() < M_POSE_CHANCE) {
                item->required_anim_state = M_STATE_WALK;
                item->goal_anim_state = M_STATE_STOP;
            }
            break;

        case M_STATE_WALK:
            pierre->maximum_turn = M_WALK_TURN;
            if (pierre->mood == MOOD_BORED
                && Random_GetControl() < M_POSE_CHANCE) {
                item->required_anim_state = M_STATE_POSE;
                item->goal_anim_state = M_STATE_STOP;
            } else if (pierre->mood == MOOD_ESCAPE) {
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
            pierre->maximum_turn = M_RUN_TURN;
            tilt = angle / 2;
            if (pierre->mood == MOOD_BORED
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
                Creature_Shoot(item, &info, &m_PierreGun1, head, p->damage / 2);
                Creature_Shoot(item, &info, &m_PierreGun2, head, p->damage / 2);
                item->required_anim_state = M_STATE_AIM;
            }
            if (pierre->mood == MOOD_ESCAPE
                && Random_GetControl() > M_WIMP_CHANCE) {
                item->required_anim_state = M_STATE_STOP;
            }
            break;
        }
    }

    Creature_Tilt(item, tilt);
    Creature_Head(item, head);
    Creature_Animate(item_num, angle, 0);

    if (pierre->flags) {
        GAME_VECTOR target;
        target.x = item->pos.x;
        target.y = item->pos.y - WALL_L;
        target.z = item->pos.z;

        GAME_VECTOR start;
        start.x = g_Camera.pos.x;
        start.y = g_Camera.pos.y;
        start.z = g_Camera.pos.z;
        start.room_num = g_Camera.pos.room_num;

        if (LOS_Check(&start, &target, true)) {
            pierre->flags = 1;
        } else if (pierre->flags > M_DISAPPEAR) {
            item->hit_points = 0;
            LOT_DisableBaddieAI(item_num);
            Item_Destroy(item_num);
            m_PierreItemNum = NO_ITEM;
        }
    }

    const int32_t wh = Room_GetWaterHeight(item->pos, item->room_num);
    if (wh != NO_HEIGHT) {
        item->hit_points = 0;
        LOT_DisableBaddieAI(item_num);
        Item_Destroy(item_num);
        m_PierreItemNum = NO_ITEM;
    }
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
    obj->can_drop_items_func = M_CanDropItems;

    obj->shadow_size = UNIT_SHADOW / 2;

    obj->radius = WALL_L / 10;
    obj->smartness = 0x7FFF;

    obj->intelligent = true;
    obj->save_position = true;
    obj->save_hitpoints = true;
    obj->save_anim = true;
    obj->save_flags = true;

    m_PierreItemNum = NO_ITEM;

    Object_GetBone(obj, 6)->rot.y = true;
    OBJECT_PROPERTIES(
        obj, ITEM_PROPERTY_MAX_HIT_POINTS(M_HIT_POINTS),
        OBJECT_PROPERTY(
            M_PRIV, damage, M_SHOT_DAMAGE, "Damage dealt by shots."));
}

REGISTER_OBJECT(O_PIERRE, M_Setup)
