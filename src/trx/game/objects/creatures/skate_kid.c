#include <trx/config.h>
#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/core/log.h>
#include <trx/core/utils.h>
#include <trx/game/creature.h>
#include <trx/game/music.h>
#include <trx/game/objects/common.h>
#include <trx/game/objects/property.h>
#include <trx/game/pathing.h>
#include <trx/game/random.h>

// clang-format off
#define M_HIT_POINTS        125
#define M_STOP_SHOT_DAMAGE  50
#define M_SKATE_SHOT_DAMAGE 40
#define M_RADIUS            (WALL_L / 5) // = 204
#define M_STOP_RANGE        SQUARE(WALL_L * 4) // = 16777216
#define M_DONT_STOP_RANGE   SQUARE(WALL_L * 5 / 2) // = 6553600
#define M_TOO_CLOSE         SQUARE(WALL_L) // = 1048576
#define M_SKATE_TURN        (DEG_1 * 4) // = 728
#define M_PUSH_CHANCE       0x200
#define M_SKATE_CHANCE      0x400
#define M_SMARTNESS         0x7FFF
#define M_SPEECH_HITPOINTS  120
// clang-format on

typedef enum {
    M_STATE_STOP,
    M_STATE_SHOOT_1,
    M_STATE_SKATE,
    M_STATE_PUSH,
    M_STATE_SHOOT_2,
    M_STATE_DEATH,
} M_STATE;

typedef enum {
    M_ANIM_DEATH = 13,
} M_ANIM;

typedef struct {
    int32_t stop_shot_damage;
    int32_t skate_shot_damage;
    int16_t skateboard_item_num;
    bool speech_started;
} M_PRIV;

static const CREATURE_GUN m_KidGun1 = {
    .muzzle = { .pos = { 0, 150, 34 }, .mesh_num = 7 },
};
static const CREATURE_GUN m_KidGun2 = {
    .muzzle = { .pos = { 0, 150, 37 }, .mesh_num = 4 },
};

static RESULT M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    MUST(JSON_READ_OPT(io, "speech_started", &p->speech_started));
    return OK;
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_WRITE(io, "speech_started", p->speech_started);
}

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    p->skateboard_item_num = NO_ITEM;

    Creature_Initialise(item_num);
    item->current_anim_state = M_STATE_SKATE;

    if (!Object_Get(O_SKATEBOARD)->loaded) {
        return;
    }

    const int16_t skateboard_item_num = Item_Create();
    if (skateboard_item_num == NO_ITEM) {
        LOG_WARNING("Failed to create skateboard item for skate kid.");
        return;
    }

    ITEM *const skateboard_item = Item_Get(skateboard_item_num);
    skateboard_item->object_id = O_SKATEBOARD;
    skateboard_item->pos = item->pos;
    skateboard_item->rot = item->rot;
    skateboard_item->room_num = item->room_num;
    Item_SetVisible(skateboard_item, item->is_visible);
    Item_SetFinished(skateboard_item, item->is_finished);
    skateboard_item->is_collidable = false;
    skateboard_item->shade.value_1 = -1;
    Item_Initialise(skateboard_item_num);

    p->skateboard_item_num = skateboard_item_num;
}

static void M_Control(const int16_t item_num)
{
    if (!Creature_Activate(item_num)) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    CREATURE *const kid = item->creature_data;
    int16_t head = 0;
    int16_t angle = 0;

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

        angle = Creature_Turn(item, M_SKATE_TURN);

        if (item->hit_points < M_SPEECH_HITPOINTS && !p->speech_started) {
            const MUSIC_PLAY_MODE mode =
                g_Config.audio.fix_speeches_killing_music ? MPM_OVERLAY
                                                          : MPM_NO_REPEAT;
            Music_Play(MX_SKATEKID_SPEECH, mode);
            p->speech_started = true;
        }

        switch (item->current_anim_state) {
        case M_STATE_STOP:
            kid->flags = 0;
            if (item->required_anim_state) {
                item->goal_anim_state = item->required_anim_state;
            } else if (Creature_CanTargetEnemy(item, &info)) {
                item->goal_anim_state = M_STATE_SHOOT_1;
            } else {
                item->goal_anim_state = M_STATE_SKATE;
            }
            break;

        case M_STATE_SKATE:
            kid->flags = 0;
            if (Random_GetControl() < M_PUSH_CHANCE) {
                item->goal_anim_state = M_STATE_PUSH;
            } else if (Creature_CanTargetEnemy(item, &info)) {
                if (info.distance > M_DONT_STOP_RANGE
                    && info.distance < M_STOP_RANGE
                    && kid->mood != MOOD_ESCAPE) {
                    item->goal_anim_state = M_STATE_STOP;
                } else {
                    item->goal_anim_state = M_STATE_SHOOT_2;
                }
            }
            break;

        case M_STATE_PUSH:
            if (Random_GetControl() < M_SKATE_CHANCE) {
                item->goal_anim_state = M_STATE_SKATE;
            }
            break;

        case M_STATE_SHOOT_1:
        case M_STATE_SHOOT_2:
            if (!kid->flags && Creature_CanTargetEnemy(item, &info)) {
                const int32_t damage =
                    item->current_anim_state == M_STATE_SHOOT_1
                    ? p->stop_shot_damage
                    : p->skate_shot_damage;
                Creature_Shoot(item, &info, &m_KidGun1, head, damage);

                Creature_Shoot(item, &info, &m_KidGun2, head, damage);

                kid->flags = 1;
            }
            if (kid->mood == MOOD_ESCAPE || info.distance < M_TOO_CLOSE) {
                item->required_anim_state = M_STATE_SKATE;
            }
            break;
        }
    }

    Creature_Head(item, head);
    Creature_Animate(item_num, angle, 0);

    if (p->skateboard_item_num != NO_ITEM) {
        ITEM *const skateboard_item = Item_Get(p->skateboard_item_num);
        skateboard_item->pos = item->pos;
        skateboard_item->rot = item->rot;
        Item_SetVisible(skateboard_item, item->is_visible);
        Item_SetFinished(skateboard_item, item->is_finished);
        Item_UpdateRoom(p->skateboard_item_num, item->room_num);

        const int16_t relative_anim = Item_GetRelativeAnim(item);
        const int16_t relative_frame = Item_GetRelativeFrame(item);
        Item_SwitchToObjAnim(
            skateboard_item, relative_anim, relative_frame, O_SKATEBOARD);
    }
}

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise_func = M_Initialise;
    obj->control_func = M_Control;
    obj->collision_func = Creature_Collision;
    obj->priv_size = sizeof(M_PRIV);
    obj->shadow_size = UNIT_SHADOW / 2;

    obj->radius = M_RADIUS;
    obj->smartness = M_SMARTNESS;
    obj->intelligent = true;
    obj->save_position = true;
    obj->save_hitpoints = true;
    obj->save_anim = true;
    obj->save_flags = true;
    obj->priv_load_func = M_LoadPriv;
    obj->priv_save_func = M_SavePriv;

    Object_GetBone(obj, 0)->rot.y = true;

    if (!Object_Get(O_SKATEBOARD)->loaded) {
        LOG_WARNING(
            "Skateboard object (%d) is not loaded and so will not be drawn.",
            O_SKATEBOARD);
    }
    OBJECT_PROPERTIES(
        obj, ITEM_PROPERTY_MAX_HIT_POINTS(M_HIT_POINTS),
        OBJECT_PROPERTY(
            M_PRIV, stop_shot_damage, M_STOP_SHOT_DAMAGE,
            "Damage dealt by shots while stopped."),
        OBJECT_PROPERTY(
            M_PRIV, skate_shot_damage, M_SKATE_SHOT_DAMAGE,
            "Damage dealt by shots while skating."));
}

static void M_SetupSkateboard(OBJECT *const obj)
{
    obj->control_func = nullptr;
}

REGISTER_OBJECT(O_SKATE_KID, M_Setup)
REGISTER_OBJECT(O_SKATEBOARD, M_SetupSkateboard)
